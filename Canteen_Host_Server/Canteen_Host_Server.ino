/**
 * ============================================================================
 * Project: Meal Subsidy Management System (Tuesday 35-Baht Quota)
 * System: Central Host Server & Gateway Monitor
 * Version: 108.0.0 (Hardened: Safe CSV, Session Security, Live Bento Portal)
 * Release Date: กันยายน 2569 (September 2026)
 * 
 * Developer: กิตติพันธ์ รัตนคร (Kittiphan Rattanakorn)
 * Role: นักวิชาการคอมพิวเตอร์ (Computer Technical Officer)
 * Organization: มหาวิทยาลัยมหาจุฬาลงกรณราชวิทยาลัย วิทยาเขตแพร่
 * 
 * Hardware Target: 
 *   - MCU: ESP32-S3 DevKitC-1 (N16R8: 16MB Flash, 8MB Octal PSRAM)
 *   - Display: 2.8 Inch ST7789V 14P SPI (320x240 Resolution)
 *   - Real-Time Clock: DS3231 Precision I2C RTC
 *   - LED: Built-in WS2812 RGB on GPIO 48
 * ============================================================================
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <esp_random.h>

// ---------------------------------------------------------------------------
// เครื่องพิมพ์ความร้อน 58 มม. (ESC/POS)
//
//   ENABLE_THERMAL_PRINTER 1 = เปิดใช้งาน  ·  0 = ปิดทั้งระบบ
//   PRINTER_TRANSPORT_UART 0 = ต่อผ่าน USB OTG  ·  1 = ต่อผ่าน UART TTL GPIO17/18
//
// **ค่าเริ่มต้นคือปิด** เพราะสแต็ก USB host กินแฟลชราว 20-30 KB และไดรเวอร์
// คลาสปริ้นเตอร์ยังไม่เคยทดสอบกับเครื่องจริง เมื่อใดที่ต่อเครื่องพิมพ์แล้ว
// ให้เปลี่ยนบรรทัดล่างเป็น 1 แล้วอัปโหลดใหม่ ฟีเจอร์กลับมาครบทันที
// ตอนปิดอยู่ ปุ่มบนหน้าเว็บจะแจ้งว่าปิดใช้งานไว้ในเฟิร์มแวร์ ไม่ได้พังแต่อย่างใด
//
// อ่านข้อกำหนดการต่อไฟและการตั้งค่า USB Mode ได้ที่หัวไฟล์ ThermalPrinter.h
// ---------------------------------------------------------------------------
#define ENABLE_THERMAL_PRINTER 0
#define PRINTER_TRANSPORT_UART 0
#include "ThermalPrinter.h"

#define APP_VERSION         "111.1.0"
#define DEV_NAME            "Kittiphan Rattanakorn"
#define DEV_ROLE            "Computer Technical Officer"
#define DEV_INSTITUTION     "MCU Phrae Campus"

// Pin Configuration สำหรับ ESP32-S3 DevKitC-1 N16R8
#define TFT_CS              10
#define TFT_DC              11
#define TFT_RST             12
#define TFT_MOSI            13
#define TFT_SCLK            14
#define TFT_BLK             15

#define BUZZER_PIN          6
#define BTN_PIN             2
#define BATTERY_ADC_PIN     1   // ADC1_CH0 ปลอดภัยต่อการใช้พร้อม Wi-Fi
#define RGB_LED_PIN         48  // Onboard WS2812 RGB

#define I2C_SDA_PIN         4   // DS3231 SDA
#define I2C_SCL_PIN         5   // DS3231 SCL

// ============================================================================
// COLOR MACROS & THEME ENGINE
// ============================================================================
#define BENTO_BG            0x0000
#define BENTO_CARD_BG       0x10A4
#define BENTO_CARD_BORDER   0x2965
#define BENTO_NEON_GREEN    0x07E0
#define BENTO_NEON_CYAN     0x07FF
#define BENTO_NEON_YELLOW   0xFFE0
#define BENTO_NEON_ROSE     0xF800  // สีแดงแท้ (Pure Red)
#define BENTO_TEXT_MUTED    0x8410
#define BENTO_WHITE         0xFFFF

#ifndef ST77XX_NAVY
  #define ST77XX_NAVY       0x000F
#endif
#ifndef ST77XX_DARKGREY
  #define ST77XX_DARKGREY   0x2124
#endif
#ifndef ST77XX_DARKGREEN
  #define ST77XX_DARKGREEN  0x03E0
#endif
#ifndef ST77XX_LIGHTGREY
  #define ST77XX_LIGHTGREY  0xC618
#endif
#ifndef ST77XX_ORANGE
  #define ST77XX_ORANGE     0xFD20
#endif

bool isTftDarkMode = true;

uint16_t getTftBg()          { return isTftDarkMode ? BENTO_BG : 0xF7BF; }
uint16_t getTftCardBg()      { return isTftDarkMode ? BENTO_CARD_BG : BENTO_WHITE; }
uint16_t getTftCardBorder()  { return isTftDarkMode ? BENTO_CARD_BORDER : 0xCE79; }
uint16_t getTftTextMain()    { return isTftDarkMode ? BENTO_WHITE : 0x0841; }
uint16_t getTftTextMuted()   { return isTftDarkMode ? BENTO_TEXT_MUTED : 0x632C; }
uint16_t getTftAccentGreen() { return isTftDarkMode ? BENTO_NEON_GREEN : 0x04A6; }
uint16_t getTftAccentCyan()  { return isTftDarkMode ? BENTO_NEON_CYAN : 0x0318; }
uint16_t getTftAccentYellow(){ return isTftDarkMode ? BENTO_NEON_YELLOW : 0xCBA0; }
uint16_t getTftAccentRose()  { return isTftDarkMode ? BENTO_NEON_ROSE : 0xD800; }

SPIClass SPI_TFT(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI_TFT, TFT_CS, TFT_DC, TFT_RST);
RTC_DS3231 rtc;

WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

const byte DNS_PORT = 53;
const char* default_ap_ssid = "MCU_CANTEEN_HOST";
const char* default_ap_pass = "12345678";
const char* mdns_hostname   = "canteen";

uint8_t broadcastAddress[]   = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const uint8_t ESPNOW_CHANNEL = 1;

bool timeWindowEnabled = true;
// หน้าจอสาธารณะ /display สำหรับต่อออกมอนิเตอร์จอใหญ่ให้นิสิตและร้านค้าดู
bool publicDisplayEnabled = true;
bool printerAutoSlip     = true;   // พิมพ์สลิปอัตโนมัติทุกครั้งที่ตัดสิทธิ์สำเร็จ
int serviceStartHour   = 10;
int serviceStartMin    = 0;
int serviceEndHour     = 13;
int serviceEndMin      = 30;

struct AdminUser {
  String username;
  String password;
  String displayName;
};
std::vector<AdminUser> adminUsers;

struct ActiveSession {
  String token;
  String username;
  unsigned long expiry;
};
ActiveSession activeSessions[3];

enum MsgType : uint8_t { MSG_HEARTBEAT = 1, MSG_SCAN_REQ = 2, MSG_SCAN_RESP = 3, MSG_CONFIG = 4,
                        MSG_ROSTER = 5 };

// ---------------------------------------------------------------------------
// บังคับให้ ESP-NOW ใช้อัตราส่งแบบ Long Range (250 kbps) ซึ่งรับสัญญาณอ่อนได้ดีขึ้นมาก
// แลกกับความเร็วที่ระบบนี้ไม่ต้องการอยู่แล้ว เพราะแพ็กเก็ตใหญ่สุดแค่ 200 ไบต์
//
// ค่าเริ่มต้นคือปิดไว้ เพราะถ้าเปิดข้างเดียวทั้งสองเครื่องจะคุยกันไม่รู้เรื่อง
// วิธีเปิด: แฟลชทั้งสองฝั่งด้วยค่า 0 ให้ระบบทำงานปกติก่อน แล้วค่อยเปลี่ยนเป็น 1
// ทั้งสองไฟล์แล้วแฟลชใหม่ทั้งคู่ในเวลาที่ไม่มีนิสิตใช้บริการ
// ถ้าคอมไพล์ไม่ผ่านกับ core ที่ใช้อยู่ ให้ตั้งกลับเป็น 0
// ---------------------------------------------------------------------------
#define ESPNOW_FORCE_LONG_RANGE_RATE 0

#define ESPNOW_PROTO_MAGIC 0xCA
#define ESPNOW_PROTO_VER   2

typedef struct __attribute__((packed)) {
  uint8_t magic;
  uint8_t version;
  uint8_t msgType;
  uint8_t stationId;
  uint16_t seq;
  char uid[16];
  char offlineTime[24];
  float systemVoltage;
} StationPacket;

typedef struct __attribute__((packed)) {
  uint8_t magic;
  uint8_t version;
  uint8_t msgType;
  uint8_t stationId;
  uint16_t seq;
  char status[16];
  char studentId[16];
  char name[64];
  char refNo[36];
  char claimTime[24];
  char message[32];
  int amount;
  uint16_t servedCount;
} HostResponsePacket;

// คำสั่งโหมดการแสดงผลที่แม่ข่ายส่งให้สถานี
//   darkMode    บังคับเสมอ สถานีจะเปลี่ยนตามทุกครั้งที่ค่าไม่ตรงกัน
//   screenOn    สั่งเฉพาะตอน modeSeq เปลี่ยน สถานียังปิด/เปิดจอเองได้ภายหลัง
//   screensaver เช่นเดียวกับ screenOn
//   modeSeq     เพิ่มขึ้นทุกครั้งที่เจ้าหน้าที่เปลี่ยนโหมดที่เครื่องแม่ข่าย
typedef struct __attribute__((packed)) {
  uint8_t magic;
  uint8_t version;
  uint8_t msgType;
  uint8_t stationId;
  uint8_t darkMode;
  uint8_t screenOn;
  uint8_t screensaver;
  uint8_t modeSeq;
} HostConfigPacket;

// ---------------------------------------------------------------------------
// บัญชีสิทธิ์ย่อ (roster) ที่แม่ข่ายผลักไปเก็บไว้ที่สถานี
//
// เดิมตอนลิงก์ขาด สถานีรับบัตร "ทุกใบ" เข้าคิวออฟไลน์โดยไม่ตรวจอะไรเลย
// บัตรที่ไม่ได้ลงทะเบียนหรือบัตรที่ใช้สิทธิ์ไปแล้วก็ได้รับอาหารไปก่อน
// แล้วค่อยไปตกตอนซิงค์ ซึ่งสายเกินกว่าจะเรียกคืนได้
//
// แก้ด้วยการส่งบัญชีย่อไปเก็บไว้ที่สถานีล่วงหน้า เก็บเป็นค่าแฮช 32 บิตของเลขบัตร
// คู่กับสถานะว่าใช้สิทธิ์ไปแล้วหรือยัง จึงใช้แค่ 5 ไบต์ต่อคน (นิสิต 600 คน = 3 KB)
// สถานีไม่เคยได้รับเลขบัตรจริงหรือชื่อนิสิตเลย ถึงเครื่องหายก็ไม่มีข้อมูลส่วนบุคคลติดไป
//
// ค่าแฮชชนกันได้ตามทฤษฎี (FNV-1a 32 บิต นิสิต 600 คน โอกาสราว 0.004%)
// ผลของการชนคือบัตรแปลกปลอมใบหนึ่งผ่านด่านออฟไลน์ไปได้ ซึ่งแม่ข่ายจะปัดตกตอนซิงค์
// เท่ากับกลับไปเท่าพฤติกรรมเดิมเฉพาะบัตรใบนั้น ไม่ได้แย่ลงกว่าเดิม
// ---------------------------------------------------------------------------
#define ROSTER_ENTRIES_PER_PKT 38
#define ROSTER_FLAG_FULL_BEGIN 0x01   // ชุดเต็ม เริ่มนับใหม่ทั้งบัญชี
#define ROSTER_FLAG_FULL_END   0x02   // ชุดเต็ม ก้อนสุดท้ายแล้ว
#define ROSTER_FLAG_DELTA      0x04   // อัปเดตทีละรายการ

typedef struct __attribute__((packed)) {
  uint32_t hash;    // FNV-1a 32 บิตของเลขบัตร
  uint8_t  state;   // 0 = ยังไม่ใช้สิทธิ์, 1 = ใช้สิทธิ์แล้ววันนี้
} RosterEntry;

typedef struct __attribute__((packed)) {
  uint8_t  magic;
  uint8_t  version;
  uint8_t  msgType;      // MSG_ROSTER
  uint8_t  stationId;    // 0 = ทุกสถานี
  uint16_t rosterVer;    // เลขรุ่นของบัญชี ใช้เทียบว่าสถานีตามทันหรือยัง
  uint16_t totalEntries;
  uint32_t rosterDate;   // วันที่ของบัญชีแบบ YYYYMMDD ใช้กันข้อมูลข้ามวันค้างเครื่อง
  uint8_t  chunkIndex;
  uint8_t  chunkCount;
  uint8_t  entryCount;
  uint8_t  flags;
  RosterEntry entries[ROSTER_ENTRIES_PER_PKT];
} HostRosterPacket;

static_assert(sizeof(RosterEntry) == 5, "RosterEntry size mismatch");
static_assert(sizeof(HostRosterPacket) == 206, "HostRosterPacket size mismatch");

static_assert(sizeof(StationPacket) == 50, "StationPacket size mismatch");
static_assert(sizeof(HostResponsePacket) == 200, "HostResponsePacket size mismatch");
static_assert(sizeof(HostConfigPacket) == 8, "HostConfigPacket size mismatch");

struct StationNode {
  bool isOnline = false;
  int rssi = -100;
  float systemVoltage = 0.0;
  unsigned long lastSeen = 0;
  uint8_t mac[6] = {0};
};
StationNode stationNodes[4];
bool stationPeerReady[4] = {false, false, false, false};
volatile bool hbAckPending[4] = {false, false, false, false};

// ส่งคำตอบการสแกนซ้ำเมื่อชิปรายงานว่าส่งไม่ถึง
volatile bool respSendFailed = false;
HostResponsePacket lastRespPacket = {};
uint8_t lastRespStation = 0;
uint8_t lastRespRetryLeft = 0;
unsigned long lastRespRetryAt = 0;

uint16_t lastScanSeq[4] = {0, 0, 0, 0};
bool hasLastScanSeq[4] = {false, false, false, false};

// สถานะการผลักบัญชีสิทธิ์ไปยังแต่ละสถานี
uint16_t rosterVer = 1;                       // 0 สงวนไว้แปลว่า "ยังไม่มีบัญชี"
std::vector<RosterEntry> rosterSnapshot;      // ภาพนิ่งที่กำลังทยอยส่ง
uint16_t rosterSnapshotVer = 0;
uint16_t stationRosterVer[4] = {0, 0, 0, 0};  // เลขรุ่นที่แต่ละสถานีรายงานกลับมา
bool stationRosterTooBig[4] = {false, false, false, false};  // สถานีบอกว่าบัญชีใหญ่เกินเก็บไหว
bool stationRosterCapable[4] = {false, false, false, false}; // สถานีรุ่นนี้รองรับบัญชีสิทธิ์หรือไม่
bool     rosterPushActive[4] = {false, false, false, false};
uint8_t  rosterPushChunk[4]  = {0, 0, 0, 0};
unsigned long rosterPushNextAt = 0;
const unsigned long ROSTER_CHUNK_GAP_MS = 30;
HostResponsePacket lastScanResponse[4] = {};

struct Student {
  String studentId;
  String fullName;
  String uid;
  String originalUid;
  bool claimed;
  String claimTime;
  String refNo;
  int station;
  bool isTempCard;
};

struct Shop {
  String name;
  String vendor;
};

std::vector<Student> db;
Shop shops[4] = {
  {"ร้านที่ 1", "นางสาวณัฐกฤตา สุพิทิพย์"},
  {"ร้านที่ 2", "นางสาวปียาวัน เหมืองหม้อ"},
  {"ร้านที่ 3", "นางฉวีวรรณ วงศ์นาม"},
  {"ร้านที่ 4", "น.ส.พัชรินทร์ ชำนาญใช้"}
};

// ============================================================================
// Forward Declarations
// ============================================================================
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
#else
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len);
#endif

void renderHostPage(bool fullRedraw);
void renderScreensaver(bool fullRedraw);
void renderDeveloperCredit();
void displayHostLiveScan(String uid, String studentId, String status, int stId);
void playBootAnimation();
String getLoginHTML(const String &errorMsg = "");
String getHTML();
void sendJson(bool ok, const String &message);
bool requireAuth(bool isApi = true);
String htmlEscape(const String &raw);
String jsonEscape(const String &raw);
String csvQuote(const String &field);
int parseCsvLine(const String &line, String *out, int maxFields);
uint8_t fitTextSize(const char* text, int maxWidth, uint8_t maxSize);
void drawFitCenteredText(int x, int y, int w, int h, const char* text, uint8_t maxSize, uint16_t fg, uint16_t bg);
void handleDashboardAPI();
void handleDisplayAPI();
String getDisplayHTML();
String maskStudentId(const String &id);
void pushDisplayEvent(const String &studentId, uint8_t station, const String &timeStr);
void handleManualClaim();
void handleDailyReset();
void handleSaveShops();
void saveDatabaseToFS();
void loadDatabaseFromFS();
void saveAdminsToFS();
void loadAdminsFromFS();
void saveShopsToFS();
void loadShopsFromFS();
void restoreDailyLogs();
void appendLogToFS(String studentId, String fullName, String uid, String refNo, String timestamp, int station, String type);
bool isAuthenticated();
void redirectToLogin();
void handleCaptivePortal();
String generateSessionToken();
String generateRefNo(int stationId);
void processScanRequest(const uint8_t* mac, StationPacket pkt, int rssi);
float calculateCpuLoad();
float getChipTemperature();
float readHostBatteryVoltage(bool forceFresh = false);
int getHostBatteryPercentage(float voltage);
void drawBentoCard(int x, int y, int w, int h, uint16_t borderColor, uint16_t bgColor);
void drawBentoPillBadge(int x, int y, int w, int h, const char* text, uint16_t fgColor, uint16_t bgColor);
void drawHostBatteryHUD(int x, int y);
void drawMiniBattery(int x, int y, int pct);
void drawHostTopBar(String title);
void drawBentoBottomBar(String instruction);
String maskUID(String uid);
uint16_t getStationServedCount(uint8_t stationId);
String getRealTimeStr();
String getDateFormattedStr();
String getTimeOnlyStr();
bool isWithinServiceTime();
bool ensureStationPeer(uint8_t stationId);
bool sendToStation(uint8_t stationId, const uint8_t *data, size_t len);
// ต้องประกาศไว้ตรงนี้ (หลังนิยาม HostConfigPacket) มิฉะนั้น Arduino IDE จะสร้าง
// ต้นแบบฟังก์ชันให้เองแล้ววางไว้เหนือจุดที่นิยามโครงสร้าง ทำให้คอมไพล์ไม่ผ่านด้วย
// ข้อความ "variable or field 'fillStationConfig' declared void"
static void fillStationConfig(HostConfigPacket &cfg, uint8_t stationId);
void sendStationConfig(uint8_t stationId);
void broadcastStationConfig();
void setHostScreenPower(bool on);
void announceHostMode();
void fillSystemSummary(HostResponsePacket &pkt);
uint32_t uidHash32(const String &uid);
void bumpRosterVer();
void rebuildRosterSnapshot();
void startRosterPush(uint8_t stationId);
void serviceRosterPush();
void sendRosterDelta(const String &uid, uint8_t state);
void noteStationRosterVer(uint8_t stationId, const char *stamp);
uint16_t countRosterEligible();
uint32_t todayYmd();
void applyEspNowRate(const uint8_t *peerAddr);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);
#else
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
#endif
void setLedColor(uint8_t r, uint8_t g, uint8_t b);
void ledStandby();
void ledApproved();
void ledDuplicate();
void ledRejected();
void ledOff();
void soundWelcome();
void soundCreditJingle();
void soundBeep();
void soundScreensaverBeep();
void soundHomeBeep();
void soundScanSuccess();
void soundThemeSwitch();
void mergeImportedStudents();
void handleFileUpload();
void handleGetStudentsAPI();
void handleExportCSV();
void handleSetRTCTime();
void handleDownloadArchive();
void handleDeleteArchive();
void handleSaveStudent();
void handleSaveTempCard();
void handleRemoveTempCard();
void handleDeleteStudent();
void handleSaveAdmin();
void handleDeleteAdmin();
void handleHostButton();
String asciiSafe(const String &raw, const String &fallback);
String asciiName(const String &raw, const String &fallback);
String buildClaimSlip(const Student &s);
String buildDailySlip();
String buildTestSlip();
bool printClaimSlip(const Student &s);
void handlePrintTest();
void handlePrintSlip();
void handlePrintDaily();
void handleSavePrinterSettings();

struct ScanQueueItem {
  uint8_t mac[6];
  StationPacket pkt;
  int rssi;
};
QueueHandle_t scanQueue = NULL;

File fsUploadFile;
String lastScannedUID       = "-";
String lastScannedStudentId = "-";
String lastScannedRefNo     = "-";   // เก็บเลขอ้างอิงจริงของรายการล่าสุด
int lastScannedStation      = 0;
String lastScannedStatus    = "READY";
uint32_t transactionCounter = 0;

// ป้องกันการเดารหัสผ่านแบบสุ่มซ้ำ ๆ ที่หน้า /login
int loginFailCount           = 0;
unsigned long loginLockUntil = 0;
const int LOGIN_MAX_FAILS         = 5;
const unsigned long LOGIN_LOCK_MS = 60000;

// คิววนของรายการที่ตัดสิทธิ์สำเร็จ ใช้แสดงบนหน้าจอสาธารณะเท่านั้น
// เก็บเฉพาะรหัสนิสิตที่ปิดบังแล้ว เวลา และหมายเลขจุดบริการ — ไม่มีชื่อและไม่มีเลขบัตร
#define DISPLAY_FEED_SIZE 12
struct DisplayEvent {
  char maskedId[20];
  char timeStr[12];
  uint8_t station;
};
DisplayEvent displayFeed[DISPLAY_FEED_SIZE] = {};
uint8_t displayFeedCount = 0;
uint8_t displayFeedHead  = 0;

int currentHostPage         = 0;
const int TOTAL_PAGES       = 3;
bool isScreensaverActive    = false;
bool isHostScreenOn         = true;   // ไฟหน้าจอของเครื่องแม่ข่าย
uint8_t hostModeSeq         = 0;      // นับทุกครั้งที่โหมดการแสดงผลเปลี่ยน
bool isCreditActive         = false;
bool isLiveScanDisplaying   = false;
unsigned long liveScanHoldUntil = 0;
unsigned long lastActivity  = 0;
const unsigned long TIMEOUT_SCREENSAVER = 300000;
const unsigned long DOUBLE_CLICK_GAP    = 280;
const unsigned long STATION_OFFLINE_TIMEOUT = 15000;

unsigned long lastMonitorCheck = 0;
unsigned long lastClockRefresh = 0;

float currentCpuLoad = 8.0;
unsigned long lastCpuMeasureTime = 0;
unsigned long loopCounter = 0;

float calculateCpuLoad() {
  loopCounter++;
  unsigned long now = millis();
  if (now - lastCpuMeasureTime >= 1000) {
    const unsigned long maxExpectedLoops = 460;
    float idleRatio = (float)loopCounter / (float)maxExpectedLoops;
    if (idleRatio > 1.0f) idleRatio = 1.0f;
    currentCpuLoad = (1.0f - idleRatio) * 100.0f;
    if (currentCpuLoad < 4.0f) currentCpuLoad = 4.0f;
    if (currentCpuLoad > 95.0f) currentCpuLoad = 95.0f;
    loopCounter = 0;
    lastCpuMeasureTime = now;
  }
  return currentCpuLoad;
}

float getChipTemperature() {
  return temperatureRead();
}

void soundThemeSwitch() {
  tone(BUZZER_PIN, 1500, 60); delay(70);
  tone(BUZZER_PIN, 2200, 80); delay(90);
  noTone(BUZZER_PIN);
}

void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  neopixelWrite(RGB_LED_PIN, r, g, b);
}
void ledStandby()   { setLedColor(0, 10, 25); }
void ledApproved()  { setLedColor(0, 65, 0); }
void ledDuplicate() { setLedColor(65, 25, 0); }
void ledRejected()  { setLedColor(65, 0, 0); }
void ledOff()       { setLedColor(0, 0, 0); }

float cachedHostVoltage            = 0.0f;
unsigned long lastHostBatteryReadMs = 0;
const unsigned long BATTERY_CACHE_MS = 3000;

// เดิมฟังก์ชันนี้ใช้ delay(2) แปดรอบ = บล็อกลูปหลัก 16 ms ทุกครั้งที่วาดแถบบน
float readHostBatteryVoltage(bool forceFresh) {
  unsigned long now = millis();
  if (!forceFresh && lastHostBatteryReadMs != 0 && (now - lastHostBatteryReadMs) < BATTERY_CACHE_MS) {
    return cachedHostVoltage;
  }
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(BATTERY_ADC_PIN);
    delayMicroseconds(300);
  }
  float avgRaw = sum / 8.0f;
  float pinVoltage = (avgRaw / 4095.0f) * 3.3f;
  cachedHostVoltage = pinVoltage * 2.0f;
  lastHostBatteryReadMs = (millis() == 0) ? 1 : millis();
  return cachedHostVoltage;
}

int getHostBatteryPercentage(float voltage) {
  if (voltage >= 4.15f) return 100;
  if (voltage <= 3.30f) return 0;
  int percent = (int)((voltage - 3.30f) / (4.15f - 3.30f) * 100.0f);
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  return percent;
}

// ---------------------------------------------------------------------------
// ตัวช่วยความปลอดภัยของสตริง: กันชื่อที่มี < > & " ' ทำให้ HTML/JSON/CSV พัง
// ---------------------------------------------------------------------------
String htmlEscape(const String &raw) {
  String out;
  out.reserve(raw.length() + 8);
  for (size_t i = 0; i < raw.length(); i++) {
    char c = raw[i];
    switch (c) {
      case '&':  out += "&amp;";  break;
      case '<':  out += "&lt;";   break;
      case '>':  out += "&gt;";   break;
      case '"':  out += "&quot;"; break;
      case '\'': out += "&#39;";  break;
      default:   out += c;        break;
    }
  }
  return out;
}

String jsonEscape(const String &raw) {
  String out;
  out.reserve(raw.length() + 8);
  for (size_t i = 0; i < raw.length(); i++) {
    char c = raw[i];
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else if ((uint8_t)c < 0x20) { /* ตัดอักขระควบคุมทิ้ง */ }
    else out += c;
  }
  return out;
}

// ครอบฟิลด์ด้วยเครื่องหมายคำพูดเสมอ เพื่อให้ชื่อที่มีลูกน้ำไม่ทำให้ระเบียนเพี้ยน
String csvQuote(const String &field) {
  String out = "\"";
  for (size_t i = 0; i < field.length(); i++) {
    char c = field[i];
    if (c == '"') out += "\"\"";
    else if (c == '\r' || c == '\n') out += ' ';
    else out += c;
  }
  out += "\"";
  return out;
}

// แยกฟิลด์ CSV โดยเข้าใจเครื่องหมายคำพูด (อ่านไฟล์รุ่นเก่าที่ไม่มีคำพูดได้ด้วย)
int parseCsvLine(const String &line, String *out, int maxFields) {
  int count = 0;
  String cur = "";
  bool inQuotes = false;
  for (size_t i = 0; i < line.length(); i++) {
    char c = line[i];
    if (inQuotes) {
      if (c == '"') {
        if (i + 1 < line.length() && line[i + 1] == '"') { cur += '"'; i++; }
        else inQuotes = false;
      } else cur += c;
    } else {
      if (c == '"') inQuotes = true;
      else if (c == ',') {
        if (count < maxFields) { cur.trim(); out[count++] = cur; }
        cur = "";
        if (count >= maxFields) return count;
      } else cur += c;
    }
  }
  if (count < maxFields) { cur.trim(); out[count++] = cur; }
  return count;
}

// เลือกขนาดฟอนต์ที่ใหญ่ที่สุดที่ยังพอดีกรอบ (ฟอนต์ GFX กว้าง 6px ต่อ 1 size)
uint8_t fitTextSize(const char* text, int maxWidth, uint8_t maxSize) {
  int len = (int)strlen(text);
  if (len <= 0) return 1;
  for (uint8_t sz = maxSize; sz > 1; sz--) {
    if (len * 6 * (int)sz <= maxWidth) return sz;
  }
  return 1;
}

void drawFitCenteredText(int x, int y, int w, int h, const char* text, uint8_t maxSize, uint16_t fg, uint16_t bg) {
  uint8_t sz = fitTextSize(text, w - 6, maxSize);
  int textW = (int)strlen(text) * 6 * (int)sz;
  int textX = x + (w - textW) / 2;
  if (textX < x + 2) textX = x + 2;
  int textY = y + (h - 8 * (int)sz) / 2;
  if (textY < y) textY = y;
  tft.setTextSize(sz);
  tft.setTextColor(fg, bg);
  tft.setCursor(textX, textY);
  tft.print(text);
}

// (ชิ้นส่วนพื้นฐานของจอ TFT อยู่ใน HostDisplay.h)

String maskUID(String uid) {
  if (uid.length() < 4 || uid == "-") return uid;
  return uid.substring(0, uid.length() - 4) + "****";
}

// ปิดบังรหัสนิสิตสำหรับจอสาธารณะ: เหลือห้าหลักแรกพอให้เจ้าตัวจำได้ว่าเป็นของตน
String maskStudentId(const String &id) {
  if (id.length() == 0 || id == "-") return "-";
  if (id.length() <= 5) return id;
  String out = id.substring(0, 5);
  for (size_t i = 5; i < id.length(); i++) out += '*';
  return out;
}

void pushDisplayEvent(const String &studentId, uint8_t station, const String &timeStr) {
  DisplayEvent &e = displayFeed[displayFeedHead];
  String masked = maskStudentId(studentId);
  strncpy(e.maskedId, masked.c_str(), sizeof(e.maskedId) - 1);
  e.maskedId[sizeof(e.maskedId) - 1] = '\0';
  String t = (timeStr.length() >= 8) ? timeStr.substring(timeStr.length() - 8) : getTimeOnlyStr();
  strncpy(e.timeStr, t.c_str(), sizeof(e.timeStr) - 1);
  e.timeStr[sizeof(e.timeStr) - 1] = '\0';
  e.station = station;
  displayFeedHead = (displayFeedHead + 1) % DISPLAY_FEED_SIZE;
  if (displayFeedCount < DISPLAY_FEED_SIZE) displayFeedCount++;
}

uint16_t getStationServedCount(uint8_t stationId) {
  uint16_t count = 0;
  for (const auto& s : db) {
    if (s.claimed && s.station == stationId) count++;
  }
  return count;
}

String getRealTimeStr() {
  DateTime now = rtc.now();
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  return String(buffer);
}

String getDateFormattedStr() {
  DateTime now = rtc.now();
  const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char buffer[40];
  snprintf(buffer, sizeof(buffer), "%s, %02d %s %04d",
           days[now.dayOfTheWeek()], now.day(), months[now.month() - 1], now.year());
  return String(buffer);
}

String getTimeOnlyStr() {
  DateTime now = rtc.now();
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
           now.hour(), now.minute(), now.second());
  return String(buffer);
}

bool isWithinServiceTime() {
  if (!timeWindowEnabled) return true;
  DateTime now = rtc.now();
  int curMins = now.hour() * 60 + now.minute();
  int startMins = serviceStartHour * 60 + serviceStartMin;
  int endMins = serviceEndHour * 60 + serviceEndMin;
  return (curMins >= startMins && curMins <= endMins);
}

bool ensureStationPeer(uint8_t stationId) {
  if (stationId < 1 || stationId > 4) return false;
  StationNode &node = stationNodes[stationId - 1];
  bool nonZero = false;
  for (int i = 0; i < 6; i++) if (node.mac[i] != 0x00) { nonZero = true; break; }
  if (!nonZero) return false;

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, node.mac, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.ifidx = WIFI_IF_AP;
  peerInfo.encrypt = false;

  if (esp_now_is_peer_exist(node.mac)) {
    stationPeerReady[stationId - 1] = true;
    return true;
  }


  esp_err_t err = esp_now_add_peer(&peerInfo);
  if (err == ESP_OK || err == ESP_ERR_ESPNOW_EXIST) {
    stationPeerReady[stationId - 1] = true;
    applyEspNowRate(node.mac);
    return true;
  }
  stationPeerReady[stationId - 1] = false;
  return false;
}

// เดิมถ้า unicast ล้มเหลวจะยิงเป็น broadcast แทน ซึ่งทำให้แย่ลงไม่ใช่ดีขึ้น
// เพราะ broadcast ไม่มี ACK ระดับ MAC จึงไม่มีการส่งซ้ำอัตโนมัติของชิป
// ขณะที่ unicast มี ARQ ในตัว ตอนนี้ใช้ broadcast เฉพาะตอนยังไม่รู้จัก MAC ของสถานี
bool sendToStation(uint8_t stationId, const uint8_t *data, size_t len) {
  if (stationId >= 1 && stationId <= 4 && ensureStationPeer(stationId)) {
    return esp_now_send(stationNodes[stationId - 1].mac, data, len) == ESP_OK;
  }
  return esp_now_send(broadcastAddress, data, len) == ESP_OK;
}

// ตั้งอัตราส่งของ peer ให้เป็นโหมดระยะไกล (เรียกทุกครั้งหลังเพิ่ม peer)
void applyEspNowRate(const uint8_t *peerAddr) {
#if ESPNOW_FORCE_LONG_RANGE_RATE && ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_now_rate_config_t rateCfg = {};
  rateCfg.phymode = WIFI_PHY_MODE_LR;
  rateCfg.rate = WIFI_PHY_RATE_LORA_250K;
  rateCfg.ersu = false;
  rateCfg.dcm = false;
  esp_now_set_peer_rate_config(peerAddr, &rateCfg);
#else
  (void)peerAddr;
#endif
}

// ชิปบอกได้ทันทีว่าส่งถึงหรือไม่ ใช้จังหวะนี้ยิงคำตอบการสแกนซ้ำแทนการรอ timeout
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  (void)tx_info;
#else
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  (void)mac_addr;
#endif
  if (status != ESP_NOW_SEND_SUCCESS) respSendFailed = true;
}

static void fillStationConfig(HostConfigPacket &cfg, uint8_t stationId) {
  cfg.magic = ESPNOW_PROTO_MAGIC;
  cfg.version = ESPNOW_PROTO_VER;
  cfg.msgType = MSG_CONFIG;
  cfg.stationId = stationId;
  cfg.darkMode = isTftDarkMode ? 1 : 0;
  cfg.screenOn = isHostScreenOn ? 1 : 0;
  cfg.screensaver = isScreensaverActive ? 1 : 0;
  cfg.modeSeq = hostModeSeq;
}

// ฝากภาพรวมทั้งระบบไปกับช่องที่ไม่ได้ใช้ของแพ็กเก็ต heartbeat (message[32])
// รูปแบบ "ใช้สิทธิ์/ผู้มีสิทธิ์ทั้งหมด/เปิดบริการ/เวลาเปิด-เวลาปิด" เช่น "268/412/1/1000-1330"
// ขนาดแพ็กเก็ตไม่เปลี่ยน เฟิร์มแวร์สถานีรุ่นเก่าไม่อ่านช่องนี้ จึงอัปเดตทีละเครื่องได้
void fillSystemSummary(HostResponsePacket &pkt) {
  int used = 0;
  for (const auto &st : db) if (st.claimed) used++;
  snprintf(pkt.message, sizeof(pkt.message), "%d/%d/%d/%02d%02d-%02d%02d",
           used, (int)db.size(), isWithinServiceTime() ? 1 : 0,
           serviceStartHour, serviceStartMin, serviceEndHour, serviceEndMin);
}

void sendStationConfig(uint8_t stationId) {
  HostConfigPacket cfg = {};
  fillStationConfig(cfg, stationId);
  sendToStation(stationId, (uint8_t *)&cfg, sizeof(cfg));
}

void broadcastStationConfig() {
  HostConfigPacket cfg = {};
  fillStationConfig(cfg, 0);
  esp_now_send(broadcastAddress, (uint8_t *)&cfg, sizeof(cfg));
}


// ---------------------------------------------------------------------------
// บัญชีสิทธิ์ย่อที่ผลักไปเก็บที่สถานี เพื่อให้ตรวจสิทธิ์ได้เองตอนลิงก์ขาด
// ---------------------------------------------------------------------------

// FNV-1a 32 บิต ต้องให้ผลเท่ากันเป๊ะทั้งสองฝั่ง ห้ามแก้ข้างเดียว
uint32_t uidHash32(const String &uid) {
  uint32_t h = 2166136261UL;
  for (unsigned int i = 0; i < uid.length(); i++) {
    h ^= (uint8_t)uid[i];
    h *= 16777619UL;
  }
  return h;
}

// เรียกทุกครั้งที่ "ชุดผู้มีสิทธิ์" หรือ "สถานะใช้สิทธิ์" เปลี่ยน
// สถานีรายงานเลขรุ่นที่ตัวเองถืออยู่มากับ heartbeat ถ้าไม่ตรงแม่ข่ายจะผลักชุดเต็มให้ใหม่
void bumpRosterVer() {
  rosterVer++;
  if (rosterVer == 0) rosterVer = 1;   // 0 สงวนไว้แปลว่ายังไม่มีบัญชี
}

// วันที่ของวันนี้แบบ YYYYMMDD ติดไปกับบัญชีทุกชุด
uint32_t todayYmd() {
  DateTime now = rtc.now();
  return (uint32_t)now.year() * 10000UL + (uint32_t)now.month() * 100UL + (uint32_t)now.day();
}

uint16_t countRosterEligible() {
  uint16_t n = 0;
  for (const auto &st : db) {
    if (st.uid.length() > 0) n++;
  }
  return n;
}

void rebuildRosterSnapshot() {
  rosterSnapshot.clear();
  rosterSnapshot.reserve(db.size());
  for (const auto &st : db) {
    if (st.uid.length() == 0) continue;   // ไม่มีบัตรผูกอยู่ ไม่ต้องส่งไปกินที่
    RosterEntry e;
    e.hash  = uidHash32(st.uid);
    e.state = st.claimed ? 1 : 0;
    rosterSnapshot.push_back(e);
  }
  rosterSnapshotVer = rosterVer;
}

void startRosterPush(uint8_t stationId) {
  if (stationId < 1 || stationId > 4) return;
  if (rosterSnapshotVer != rosterVer) rebuildRosterSnapshot();
  rosterPushActive[stationId - 1] = true;
  rosterPushChunk[stationId - 1]  = 0;
}

// ส่งทีละก้อน ก้อนละ 30 ms เพื่อไม่ให้คิวส่งของ ESP-NOW ล้นและไม่แย่งจังหวะการสแกนสด
void serviceRosterPush() {
  if ((long)(millis() - rosterPushNextAt) < 0) return;

  int total = (int)rosterSnapshot.size();
  int chunkCount = (total + ROSTER_ENTRIES_PER_PKT - 1) / ROSTER_ENTRIES_PER_PKT;
  if (chunkCount == 0) chunkCount = 1;   // บัญชีว่างก็ยังต้องบอกสถานีว่าว่าง

  for (int i = 0; i < 4; i++) {
    if (!rosterPushActive[i]) continue;

    // ระหว่างทยอยส่ง ถ้าบัญชีเปลี่ยนไปแล้วให้ตั้งต้นใหม่ ไม่งั้นสถานีจะได้ของปนรุ่น
    if (rosterSnapshotVer != rosterVer) {
      rebuildRosterSnapshot();
      rosterPushChunk[i] = 0;
      total = (int)rosterSnapshot.size();
      chunkCount = (total + ROSTER_ENTRIES_PER_PKT - 1) / ROSTER_ENTRIES_PER_PKT;
      if (chunkCount == 0) chunkCount = 1;
    }

    int idx = rosterPushChunk[i];
    HostRosterPacket pkt = {};
    pkt.magic        = ESPNOW_PROTO_MAGIC;
    pkt.version      = ESPNOW_PROTO_VER;
    pkt.msgType      = MSG_ROSTER;
    pkt.stationId    = i + 1;
    pkt.rosterVer    = rosterSnapshotVer;
    pkt.totalEntries = (uint16_t)total;
    pkt.rosterDate   = todayYmd();
    pkt.chunkIndex   = (uint8_t)idx;
    pkt.chunkCount   = (uint8_t)chunkCount;

    int from = idx * ROSTER_ENTRIES_PER_PKT;
    int n = total - from;
    if (n < 0) n = 0;
    if (n > ROSTER_ENTRIES_PER_PKT) n = ROSTER_ENTRIES_PER_PKT;
    pkt.entryCount = (uint8_t)n;
    for (int k = 0; k < n; k++) pkt.entries[k] = rosterSnapshot[from + k];

    if (idx == 0) pkt.flags |= ROSTER_FLAG_FULL_BEGIN;
    if (idx == chunkCount - 1) pkt.flags |= ROSTER_FLAG_FULL_END;

    sendToStation(i + 1, (uint8_t *)&pkt, sizeof(pkt));

    rosterPushChunk[i]++;
    if (rosterPushChunk[i] >= chunkCount) rosterPushActive[i] = false;

    rosterPushNextAt = millis() + ROSTER_CHUNK_GAP_MS;
    return;   // ก้อนเดียวต่อรอบ วนไปสถานีถัดไปในรอบหน้า
  }
}

// อัปเดตทีละรายการตอนมีคนใช้สิทธิ์ ถูกกว่าการผลักบัญชีทั้งชุดใหม่ 268 ครั้งต่อวัน
// สถานีจะรับก็ต่อเมื่อเลขรุ่นที่ถืออยู่เป็น rosterVer - 1 พอดี ถ้าพลาดไปก้อนหนึ่ง
// เลขรุ่นจะไม่ตรงกันและ heartbeat รอบถัดไปจะดึงชุดเต็มมาทับเอง
void sendRosterDelta(const String &uid, uint8_t state) {
  if (uid.length() == 0) return;

  RosterEntry e;
  e.hash = uidHash32(uid);
  e.state = state;

  // ให้ภาพนิ่งที่ค้างอยู่ตรงกับความจริงด้วย เผื่อกำลังทยอยส่งให้สถานีอื่นอยู่
  if (rosterSnapshotVer == rosterVer - 1) {
    for (auto &entry : rosterSnapshot) {
      if (entry.hash == e.hash) { entry.state = state; break; }
    }
    rosterSnapshotVer = rosterVer;
  }

  for (int i = 0; i < 4; i++) {
    if (!stationNodes[i].isOnline || !stationRosterCapable[i]) continue;
    HostRosterPacket pkt = {};
    pkt.magic        = ESPNOW_PROTO_MAGIC;
    pkt.version      = ESPNOW_PROTO_VER;
    pkt.msgType      = MSG_ROSTER;
    pkt.stationId    = i + 1;
    pkt.rosterVer    = rosterVer;
    pkt.totalEntries = 0;
    pkt.rosterDate   = todayYmd();
    pkt.chunkIndex   = 0;
    pkt.chunkCount   = 1;
    pkt.entryCount   = 1;
    pkt.flags        = ROSTER_FLAG_DELTA;
    pkt.entries[0]   = e;
    sendToStation(i + 1, (uint8_t *)&pkt, sizeof(pkt));
  }
}

// สถานีฝากเลขรุ่นบัญชีที่ตัวเองถืออยู่มากับช่อง offlineTime ของ heartbeat
// (ช่องนั้นว่างอยู่แล้วในข้อความชนิดนี้ จึงไม่ต้องขยายขนาดแพ็กเก็ต
//  และเฟิร์มแวร์สถานีรุ่นเก่าที่ไม่ได้ฝากอะไรมาจะถูกมองว่าเป็นรุ่น 0 = ยังไม่มีบัญชี)
void noteStationRosterVer(uint8_t stationId, const char *stamp) {
  if (stationId < 1 || stationId > 4) return;

  // เฟิร์มแวร์สถานีรุ่นก่อน 120.0.0 ไม่ได้ฝากอะไรมาในช่องนี้เลย (เป็นศูนย์ล้วน)
  // ต้องแยกให้ออกจากสถานีรุ่นใหม่ที่ยังไม่มีบัญชีซึ่งจะฝาก "R:0" มา
  // ไม่งั้นจะไล่ผลักบัญชีไปให้เครื่องที่รับไม่เป็นทุก heartbeat ไม่มีวันจบ
  bool capable = (stamp && stamp[0] == 'R' && stamp[1] == ':');
  uint16_t reported = 0;
  bool tooBig = false;
  if (capable) {
    long v = atol(stamp + 2);       // atol หยุดเองเมื่อเจอ '!' ที่ต่อท้าย
    if (v > 0 && v <= 65535) reported = (uint16_t)v;
    tooBig = (strchr(stamp, '!') != NULL);
  }
  stationRosterCapable[stationId - 1] = capable;
  stationRosterVer[stationId - 1] = reported;
  stationRosterTooBig[stationId - 1] = tooBig;

  if (capable && reported != rosterVer && !rosterPushActive[stationId - 1]) {
    startRosterPush(stationId);
  }
}

void setHostScreenPower(bool on) {
  isHostScreenOn = on;
  digitalWrite(TFT_BLK, on ? HIGH : LOW);
}

// เรียกทุกครั้งที่โหมดการแสดงผลของแม่ข่ายเปลี่ยน เพื่อให้สถานีเปลี่ยนตามทันที
// ส่งสามครั้งห่างกันเล็กน้อยเพราะ ESP-NOW แบบ broadcast ไม่มีการยืนยันการรับ
void announceHostMode() {
  hostModeSeq++;
  for (int i = 0; i < 3; i++) {
    broadcastStationConfig();
    delay(8);
  }
}

int calculateSignalQuality(int rssi) {
  if (rssi <= -100) return 0;
  if (rssi >= -50) return 100;
  return 2 * (rssi + 100);
}

uint16_t getSignalColor(int rssi, bool isOnline) {
  if (!isOnline) return getTftAccentRose();
  int q = calculateSignalQuality(rssi);
  if (q >= 60) return getTftAccentGreen();
  if (q >= 35) return getTftAccentYellow();
  return getTftAccentRose();
}

void drawSignalBars(int x, int y, int rssi, bool isOnline, uint16_t bg = BENTO_CARD_BG) {
  int q = calculateSignalQuality(rssi);
  int activeBars = 0;
  if (isOnline) {
    if (q >= 85) activeBars = 4;
    else if (q >= 60) activeBars = 3;
    else if (q >= 35) activeBars = 2;
    else activeBars = 1;
  }

  uint16_t barColor = getSignalColor(rssi, isOnline);
  int heights[4] = {3, 6, 9, 12};

  for (int b = 0; b < 4; b++) {
    int barX = x + (b * 5);
    int barH = heights[b];
    int barY = y + (12 - barH);

    tft.fillRect(barX, y, 4, 12, bg);
    if (b < activeBars) {
      tft.fillRect(barX, barY, 3, barH, barColor);
    } else {
      tft.drawRect(barX, barY, 3, barH, getTftCardBorder());
    }
  }
}

void soundWelcome() {
  int melody[] = { 1046, 1318, 1568, 2093 };
  int noteDurations[] = { 90, 90, 110, 260 };
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], noteDurations[i]);
    delay(noteDurations[i] * 1.15);
  }
  noTone(BUZZER_PIN);
}

void soundCreditJingle() {
  int melody[] = { 1568, 1760, 2093, 2637 };
  int noteDurations[] = { 80, 80, 100, 300 };
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], noteDurations[i]);
    delay(noteDurations[i] * 1.15);
  }
  noTone(BUZZER_PIN);
}

void soundBeep() { tone(BUZZER_PIN, 2200, 50); }
void soundScreensaverBeep() { tone(BUZZER_PIN, 2400, 50); delay(70); tone(BUZZER_PIN, 1800, 70); }
void soundHomeBeep() { tone(BUZZER_PIN, 1800, 70); delay(80); tone(BUZZER_PIN, 2500, 100); }
void soundScanSuccess() { tone(BUZZER_PIN, 1800, 80); delay(100); tone(BUZZER_PIN, 2400, 100); }

// เดิมตอบกลับเป็นหน้า HTML ที่เรียก alert() แล้วรีโหลดทั้งหน้า ซึ่งทำให้เสียตำแหน่ง
// แท็บที่ค้างอยู่ และข้อความที่มีเครื่องหมาย ' จะทำให้สคริปต์พัง ตอนนี้ตอบเป็น JSON
void sendJson(bool ok, const String &message) {
  String out = "{\"ok\":";
  out += ok ? "true" : "false";
  out += ",\"msg\":\"";
  out += jsonEscape(message);
  out += "\"}";
  server.send(ok ? 200 : 400, "application/json; charset=utf-8", out);
}

bool requireAuth(bool isApi) {
  if (isAuthenticated()) return true;
  if (isApi) server.send(401, "application/json; charset=utf-8", "{\"ok\":false,\"msg\":\"UNAUTHORIZED\"}");
  else redirectToLogin();
  return false;
}

String generateRefNo(int stationId) {
  transactionCounter++;
  DateTime now = rtc.now();
  char buf[32];
  snprintf(buf, sizeof(buf), "TXN-%04d%02d%02d-%02d%02d%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  return String(buf) + "-ST" + String(stationId) + "-" + String(transactionCounter % 10000);
}

void saveAdminsToFS() {
  File file = LittleFS.open("/admins.json", "w");
  if (!file) return;
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  for (const auto& u : adminUsers) {
    JsonObject obj = array.createNestedObject();
    obj["user"] = u.username;
    obj["pass"] = u.password;
    obj["name"] = u.displayName;
  }
  serializeJson(doc, file);
  file.close();
}

void loadAdminsFromFS() {
  adminUsers.clear();
  if (LittleFS.exists("/admins.json")) {
    File file = LittleFS.open("/admins.json", "r");
    if (file) {
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, file);
      JsonArray array = doc.as<JsonArray>();
      for (JsonObject obj : array) {
        AdminUser u;
        u.username = obj["user"].as<String>();
        u.password = obj["pass"].as<String>();
        u.displayName = obj["name"].as<String>();
        adminUsers.push_back(u);
        if (adminUsers.size() >= 3) break;
      }
      file.close();
    }
  }

  if (adminUsers.empty()) {
    AdminUser def;
    def.username = "admin";
    def.password = "admin1234";
    def.displayName = "ผู้ดูแลระบบหลัก (Officer 01)";
    adminUsers.push_back(def);
    saveAdminsToFS();
  }
}

// random() ไม่ได้ถูก seed จึงให้ผลชุดเดิมทุกครั้งที่บูต ทำให้เดา session token ได้
// เปลี่ยนมาใช้ตัวสร้างเลขสุ่มฮาร์ดแวร์ของ ESP32 (128 บิต)
String generateSessionToken() {
  char buf[33];
  for (int i = 0; i < 4; i++) {
    snprintf(buf + (i * 8), 9, "%08x", (unsigned)esp_random());
  }
  buf[32] = '\0';
  return String(buf);
}

bool isAuthenticated() {
  if (!server.hasHeader("Cookie")) return false;
  String cookie = server.header("Cookie");
  int idx = cookie.indexOf("CANTEEN_SESSION=");
  if (idx == -1) return false;
  String token = cookie.substring(idx + 16);
  int semi = token.indexOf(';');
  if (semi != -1) token = token.substring(0, semi);
  token.trim();

  for (int i = 0; i < 3; i++) {
    if (activeSessions[i].token.length() > 0 && activeSessions[i].token == token) {
      // เทียบแบบ wrap-safe แทน millis() < expiry ที่พังเมื่อ millis() ล้นที่ 49 วัน
      if ((long)(millis() - activeSessions[i].expiry) < 0) {
        activeSessions[i].expiry = millis() + 7200000UL;
        return true;
      } else {
        activeSessions[i].token = "";
        activeSessions[i].username = "";
      }
    }
  }
  return false;
}

void redirectToLogin() {
  server.sendHeader("Location", "/login", true);
  server.send(302, "text/plain", "");
}

// ปลายทางของทุกคำขอที่ไม่ตรงเส้นทางใด รวมถึง URL ที่ระบบปฏิบัติการใช้ตรวจว่า
// เครือข่ายนี้ออกอินเทอร์เน็ตได้หรือไม่ (generate_204, hotspot-detect.html,
// connecttest.txt, ncsi.txt และอื่น ๆ) การตอบ 302 ทำให้เครื่องรู้ว่าติดหน้าล็อกอิน
// แล้วเปิดหน้าต่างพอร์ทัลขึ้นมาเอง
void handleCaptivePortal() {
  String target = "http://" + WiFi.softAPIP().toString() + "/login";
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.sendHeader("Location", target, true);
  server.send(302, "text/html; charset=utf-8",
              "<!DOCTYPE html><html><head><meta charset='utf-8'>"
              "<meta http-equiv='refresh' content='0; url=" + target + "'></head>"
              "<body>Redirecting to <a href='" + target + "'>" + target + "</a></body></html>");
}

// หมายเหตุ: ฟังก์ชันวาดจอ TFT ทั้งหมดย้ายไปอยู่ใน HostDisplay.h
// (ถูก #include ไว้ท้ายไฟล์ก่อน setup())

void handleHostButton() {
  static unsigned long btnPressStartTime = 0;
  static unsigned long lastReleaseTime   = 0;
  static bool btnWasPressed              = false;
  static int clickCount                  = 0;

  bool isPressed = (digitalRead(BTN_PIN) == LOW);
  if (isPressed && !btnWasPressed) {
    btnPressStartTime = millis(); btnWasPressed = true;
  } 
  else if (!isPressed && btnWasPressed) {
    unsigned long pressDuration = millis() - btnPressStartTime;
    btnWasPressed = false; lastActivity = millis();
    
    // จอดับอยู่: กดปุ่มใดก็ตามคือสั่งเปิดจอ และสั่งให้สถานีเปิดตาม
    if (!isHostScreenOn) {
      setHostScreenPower(true);
      soundHomeBeep();
      announceHostMode();
      renderHostPage(true);
      clickCount = 0;
      return;
    }

    if (isLiveScanDisplaying) {
      isLiveScanDisplaying = false;
      renderHostPage(true);
      return;
    }

    if (isScreensaverActive || isCreditActive) {
      bool wasSaver = isScreensaverActive;
      isScreensaverActive = false;
      isCreditActive = false;
      currentHostPage = 0;
      soundHomeBeep();
      renderHostPage(true);
      if (wasSaver) announceHostMode();
      clickCount = 0;
      return;
    }

    if (pressDuration >= 5000) {
      isCreditActive = true; 
      isScreensaverActive = false;
      soundCreditJingle(); 
      renderDeveloperCredit(); 
      clickCount = 0;
    }
    else if (pressDuration >= 1500) {
      // กดค้าง 1.5 วินาที = ปิดไฟหน้าจอ และสั่งให้ทุกสถานีดับจอตาม
      soundBeep();
      setHostScreenPower(false);
      ledOff();
      announceHostMode();
      clickCount = 0;
    }
    else if (pressDuration >= 30) {
      clickCount++; 
      lastReleaseTime = millis();
    }
  }

  if (!isScreensaverActive && !isCreditActive && !isLiveScanDisplaying && clickCount > 0 && (millis() - lastReleaseTime > DOUBLE_CLICK_GAP)) {
    if (clickCount == 1) {
      currentHostPage = (currentHostPage + 1) % TOTAL_PAGES;
      soundBeep();
      renderHostPage(true);
    }
    else if (clickCount == 2) {
      isScreensaverActive = true;
      soundScreensaverBeep();
      renderScreensaver(true);
      announceHostMode();
    }
    else if (clickCount >= 3) {
      isTftDarkMode = !isTftDarkMode;
      preferences.begin("sys_cfg", false);
      preferences.putBool("tft_dark", isTftDarkMode);
      preferences.end();
      soundThemeSwitch();
      renderHostPage(true);
      announceHostMode();
    }
    clickCount = 0;
  }
}

// ============================================================================
// ESP-NOW & SERVER APIs
// ============================================================================
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  const uint8_t *mac = recv_info->src_addr;
  int currentRssi = recv_info->rx_ctrl->rssi;
#else
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  int currentRssi = -60;
#endif

  if (len != sizeof(StationPacket)) return;

  StationPacket pkt;
  memcpy(&pkt, data, sizeof(StationPacket));
  if (pkt.magic != ESPNOW_PROTO_MAGIC || pkt.version != ESPNOW_PROTO_VER) return;
  int stId = pkt.stationId;

  if (stId >= 1 && stId <= 4) {
    for (int i = 0; i < 4; i++) {
      if (i != (stId - 1) &&
          stationNodes[i].isOnline &&
          memcmp(stationNodes[i].mac, mac, 6) == 0) {
        stationNodes[i] = StationNode();
        stationPeerReady[i] = false;
        hbAckPending[i] = false;
      }
    }

    stationNodes[stId - 1].isOnline = true;
    stationNodes[stId - 1].rssi = currentRssi;
    stationNodes[stId - 1].systemVoltage = pkt.systemVoltage;
    stationNodes[stId - 1].lastSeen = millis();
    bool macChanged = (memcmp(stationNodes[stId - 1].mac, mac, 6) != 0);
    if (macChanged) stationPeerReady[stId - 1] = false;
    memcpy(stationNodes[stId - 1].mac, mac, 6);
  }

  if (pkt.msgType == MSG_HEARTBEAT) {
    if (stId >= 1 && stId <= 4) {
      hbAckPending[stId - 1] = true;
      noteStationRosterVer(stId, pkt.offlineTime);
    }
    return;
  }

  if (pkt.msgType == MSG_SCAN_REQ && scanQueue != NULL) {
    ScanQueueItem item;
    memcpy(item.mac, mac, 6);
    memcpy(&item.pkt, &pkt, sizeof(StationPacket));
    item.rssi = currentRssi;
    xQueueSend(scanQueue, &item, 0);
  }
}

void processScanRequest(const uint8_t* mac, StationPacket pkt, int rssi) {
  lastActivity = millis();
  bool wasScreensaver = isScreensaverActive;
  bool wasScreenOff = !isHostScreenOn;
  isScreensaverActive = false;
  isCreditActive = false;
  if (wasScreenOff) setHostScreenPower(true);

  String uid = String(pkt.uid);
  uid.trim();

  // รายการที่สถานีบันทึกไว้ตอนขาดการเชื่อมต่อ แล้วส่งตามมาทีหลัง
  // ใช้เวลาที่นิสิตแตะบัตรจริงเป็นเวลารับสิทธิ์ ไม่ใช่เวลาที่ซิงค์
  String offlineStamp = String(pkt.offlineTime);
  offlineStamp.trim();
  bool isOfflineSync = (offlineStamp.length() >= 10);

  lastScannedUID = uid;
  lastScannedStation = pkt.stationId;
  lastScannedRefNo = "-";

  if (pkt.stationId >= 1 && pkt.stationId <= 4 &&
      pkt.seq != 0 &&
      hasLastScanSeq[pkt.stationId - 1] &&
      lastScanSeq[pkt.stationId - 1] == pkt.seq) {
    sendToStation(pkt.stationId,
                  (uint8_t *)&lastScanResponse[pkt.stationId - 1],
                  sizeof(HostResponsePacket));
    if (wasScreensaver) renderHostPage(true);
    if (wasScreensaver || wasScreenOff) announceHostMode();
    return;
  }

  HostResponsePacket resp = {};
  resp.magic = ESPNOW_PROTO_MAGIC;
  resp.version = ESPNOW_PROTO_VER;
  resp.msgType = MSG_SCAN_RESP;
  resp.stationId = pkt.stationId;
  resp.seq = pkt.seq;
  resp.amount = 35;

  // รายการย้อนหลังต้องไม่ถูกปฏิเสธเพราะซิงค์หลังปิดบริการ ในเมื่อตอนแตะบัตรยังเปิดอยู่
  if (!isOfflineSync && !isWithinServiceTime()) {
    strncpy(resp.status, "TIME_CLOSED", sizeof(resp.status) - 1);
    strncpy(resp.studentId, "-", sizeof(resp.studentId) - 1);
    strncpy(resp.name, "SERVICE CLOSED", sizeof(resp.name) - 1);
    strncpy(resp.refNo, "-", sizeof(resp.refNo) - 1);
    strncpy(resp.claimTime, getRealTimeStr().c_str(), sizeof(resp.claimTime) - 1);
    strncpy(resp.message, "OUT OF TIME", sizeof(resp.message) - 1);
    lastScannedStatus = "TIME_CLOSED";
    resp.servedCount = getStationServedCount(pkt.stationId);
    if (pkt.stationId >= 1 && pkt.stationId <= 4 && pkt.seq != 0) {
      lastScanSeq[pkt.stationId - 1] = pkt.seq;
      hasLastScanSeq[pkt.stationId - 1] = true;
      memcpy(&lastScanResponse[pkt.stationId - 1], &resp, sizeof(resp));
    }
    sendToStation(pkt.stationId, (uint8_t *)&resp, sizeof(HostResponsePacket));
    displayHostLiveScan(lastScannedUID, "-", lastScannedStatus, pkt.stationId);
    if (wasScreensaver || wasScreenOff) announceHostMode();
    return;
  }

  bool found = false;
  Student* matchedStudent = nullptr;

  for (auto& s : db) {
    if (s.uid == uid) {
      found = true;
      matchedStudent = &s;
      lastScannedStudentId = s.studentId;
      strncpy(resp.studentId, s.studentId.c_str(), sizeof(resp.studentId) - 1);

      bool isNameAscii = true;
      for (size_t i = 0; i < s.fullName.length(); i++) {
        if ((uint8_t)s.fullName[i] >= 128) { isNameAscii = false; break; }
      }
      if (isNameAscii && s.fullName.length() > 0) {
        strncpy(resp.name, s.fullName.c_str(), sizeof(resp.name) - 1);
      } else {
        String safeName = "STUDENT " + s.studentId;
        strncpy(resp.name, safeName.c_str(), sizeof(resp.name) - 1);
      }

      if (s.claimed) {
        strncpy(resp.status, "ALREADY_USED", sizeof(resp.status) - 1);
        strncpy(resp.refNo, s.refNo.c_str(), sizeof(resp.refNo) - 1);
        strncpy(resp.claimTime, s.claimTime.c_str(), sizeof(resp.claimTime) - 1);
        String msg = "SHOP 0" + String(s.station);
        strncpy(resp.message, msg.c_str(), sizeof(resp.message) - 1);
        lastScannedStatus = "DUPLICATE";
        lastScannedRefNo = s.refNo;
      } else {
        String currentTimestamp = isOfflineSync ? offlineStamp : getRealTimeStr();
        String currentRefNo = generateRefNo(pkt.stationId);

        s.claimed = true;
        s.station = pkt.stationId;
        s.claimTime = currentTimestamp;
        s.refNo = currentRefNo;
        lastScannedStatus = "APPROVED";
        lastScannedRefNo = currentRefNo;

        strncpy(resp.status, "SUCCESS", sizeof(resp.status) - 1);
        strncpy(resp.refNo, currentRefNo.c_str(), sizeof(resp.refNo) - 1);
        strncpy(resp.claimTime, currentTimestamp.c_str(), sizeof(resp.claimTime) - 1);
        strncpy(resp.message, "APPROVED 35B", sizeof(resp.message) - 1);
      }
      break;
    }
  }

  if (!found) {
    lastScannedStudentId = "-";
    strncpy(resp.status, "NOT_FOUND", sizeof(resp.status) - 1);
    strncpy(resp.studentId, "-", sizeof(resp.studentId) - 1);
    strncpy(resp.name, "UNKNOWN CARD", sizeof(resp.name) - 1);
    strncpy(resp.refNo, "-", sizeof(resp.refNo) - 1);
    strncpy(resp.claimTime, "-", sizeof(resp.claimTime) - 1);
    strncpy(resp.message, "CARD NOT FOUND", sizeof(resp.message) - 1);
    lastScannedStatus = "NOT FOUND";
  }

  resp.servedCount = getStationServedCount(pkt.stationId);
  if (pkt.stationId >= 1 && pkt.stationId <= 4 && pkt.seq != 0) {
    lastScanSeq[pkt.stationId - 1] = pkt.seq;
    hasLastScanSeq[pkt.stationId - 1] = true;
    memcpy(&lastScanResponse[pkt.stationId - 1], &resp, sizeof(resp));
  }
  sendToStation(pkt.stationId, (uint8_t *)&resp, sizeof(HostResponsePacket));
  memcpy(&lastRespPacket, &resp, sizeof(resp));
  lastRespStation = pkt.stationId;
  lastRespRetryLeft = 2;
  lastRespRetryAt = millis() + 220;
  respSendFailed = false;
  if (isOfflineSync) {
    // ซิงค์ย้อนหลังอาจมาทีละหลายสิบรายการติดกัน วาดจอใหม่อย่างมากวินาทีครึ่งครั้ง
    // ไม่อย่างนั้นจอจะกะพริบรัวและหน่วงการตอบกลับของแม่ข่ายไปด้วย
    static unsigned long lastSyncRedraw = 0;
    if (millis() - lastSyncRedraw > 1500) {
      lastSyncRedraw = millis();
      renderHostPage(true);
    }
  } else {
    displayHostLiveScan(lastScannedUID, lastScannedStudentId, lastScannedStatus, pkt.stationId);
    // มีคนมาใช้บริการแล้ว ปลุกทุกสถานีออกจากโหมดพักหน้าจอพร้อมกัน
    if (wasScreensaver || wasScreenOff) announceHostMode();
  }

  if (found && matchedStudent && strcmp(resp.status, "SUCCESS") == 0) {
    String claimType = matchedStudent->isTempCard ? "Temp Card" : "Normal";
    if (isOfflineSync) claimType += " (Offline)";
    appendLogToFS(matchedStudent->studentId, matchedStudent->fullName, matchedStudent->uid, matchedStudent->refNo, matchedStudent->claimTime, pkt.stationId, claimType);
    pushDisplayEvent(matchedStudent->studentId, pkt.stationId, matchedStudent->claimTime);

    // พิมพ์สลิปอัตโนมัติ — ข้ามรายการที่ซิงค์ย้อนหลัง เพราะนิสิตรับอาหารและกลับไปแล้ว
    // ตั้งแต่ตอนที่ลิงก์ขาด การพิมพ์ทีละหลายสิบใบจะล้นคิวและเปลืองกระดาษเปล่า
    // ยอดของรายการเหล่านั้นยังอยู่ครบในใบสรุปประจำวันและไฟล์ CSV
    if (printerAutoSlip && !isOfflineSync) {
      printClaimSlip(*matchedStudent);
    }

    // บอกทุกสถานีว่าบัตรใบนี้ใช้สิทธิ์ไปแล้ว เพื่อให้ปัดตกได้เองถ้าลิงก์ขาดหลังจากนี้
    // ส่งทีละรายการแทนการผลักบัญชีทั้งชุด ซึ่งถ้าทำทุกครั้งจะกินอากาศวันละ 268 รอบ
    // กรณีบัตรสำรองไม่ส่ง เพราะอีกสองบรรทัดถัดไปเลขบัตรของนิสิตคนนี้กำลังจะเปลี่ยน
    // แล้ว saveDatabaseToFS() จะผลักบัญชีชุดเต็มตามไปเองอยู่แล้ว
    if (!matchedStudent->isTempCard) {
      bumpRosterVer();
      sendRosterDelta(matchedStudent->uid, 1);
    }
    
    if (matchedStudent->isTempCard) {
      if (matchedStudent->originalUid != "") {
        matchedStudent->uid = matchedStudent->originalUid;
        matchedStudent->originalUid = "";
      } else {
        matchedStudent->uid = "";
      }
      saveDatabaseToFS();
    }

    if (!isOfflineSync) soundScanSuccess();
  }
}

// แดชบอร์ดสดของเว็บพอร์ทัล: ตัวเลขโควตา ฮาร์ดแวร์แม่ข่าย และสถานะจุดบริการทั้ง 4
void handleDashboardAPI() {
  if (!requireAuth()) return;

  int usedCount = 0;
  int shopCounts[4] = {0, 0, 0, 0};
  int tempWaiting = 0;
  for (const auto& st : db) {
    if (st.claimed) {
      usedCount++;
      if (st.station >= 1 && st.station <= 4) shopCounts[st.station - 1]++;
    }
    if (st.isTempCard && !st.claimed) tempWaiting++;
  }

  int total = (int)db.size();
  int quotaPct = (total > 0) ? (usedCount * 100) / total : 0;
  float volt = readHostBatteryVoltage();

  char win[16];
  snprintf(win, sizeof(win), "%02d:%02d-%02d:%02d", serviceStartHour, serviceStartMin, serviceEndHour, serviceEndMin);

  String j = "{";
  j += "\"temp\":" + String(getChipTemperature(), 1);
  j += ",\"cpu\":" + String(calculateCpuLoad(), 1);
  j += ",\"heap\":" + String((unsigned)(ESP.getFreeHeap() / 1024));
  j += ",\"uptime\":" + String((unsigned long)(millis() / 1000));
  j += ",\"battPct\":" + String(getHostBatteryPercentage(volt));
  j += ",\"battVolt\":" + String(volt, 2);
  j += ",\"used\":" + String(usedCount);
  j += ",\"total\":" + String(total);
  j += ",\"remaining\":" + String(total - usedCount);
  j += ",\"disbursed\":" + String(usedCount * 35);
  j += ",\"quotaPct\":" + String(quotaPct);
  j += ",\"tempWaiting\":" + String(tempWaiting);
  j += ",\"officers\":" + String((unsigned)adminUsers.size());
  j += ",\"clock\":\"" + jsonEscape(getRealTimeStr()) + "\"";
  j += ",\"serviceOpen\":" + String(isWithinServiceTime() ? "true" : "false");
  j += ",\"window\":\"" + String(win) + "\"";

  j += ",\"roster\":{\"ver\":" + String((unsigned)rosterVer);
  j += ",\"entries\":" + String((unsigned)countRosterEligible()) + "}";

  j += ",\"printer\":{\"enabled\":" + String(ENABLE_THERMAL_PRINTER ? "true" : "false");
  j += ",\"ready\":" + String(printerIsConnected() ? "true" : "false");
  j += ",\"auto\":" + String(printerAutoSlip ? "true" : "false");
  j += ",\"queue\":" + String((unsigned)printerQueueDepth());
  j += ",\"status\":\"" + jsonEscape(printerStatusText()) + "\"}";

  j += ",\"shops\":[";
  for (int i = 0; i < 4; i++) {
    if (i) j += ",";
    j += "{\"name\":\"" + jsonEscape(shops[i].name) + "\"";
    j += ",\"vendor\":\"" + jsonEscape(shops[i].vendor) + "\"";
    j += ",\"count\":" + String(shopCounts[i]);
    j += ",\"amount\":" + String(shopCounts[i] * 35) + "}";
  }
  j += "]";

  j += ",\"stations\":[";
  for (int i = 0; i < 4; i++) {
    if (i) j += ",";
    bool on = stationNodes[i].isOnline;
    j += "{\"id\":" + String(i + 1);
    j += ",\"online\":" + String(on ? "true" : "false");
    j += ",\"rssi\":" + String(on ? stationNodes[i].rssi : -100);
    j += ",\"quality\":" + String(on ? calculateSignalQuality(stationNodes[i].rssi) : 0);
    j += ",\"battPct\":" + String(on ? getHostBatteryPercentage(stationNodes[i].systemVoltage) : 0);
    j += ",\"volt\":" + String(on ? stationNodes[i].systemVoltage : 0.0f, 2);
    j += ",\"ageSec\":" + String(on ? (unsigned long)((millis() - stationNodes[i].lastSeen) / 1000UL) : 0UL);
    j += ",\"rosterVer\":" + String((unsigned)stationRosterVer[i]);
    j += ",\"rosterOk\":" + String((on && stationRosterCapable[i] && stationRosterVer[i] == rosterVer && !stationRosterTooBig[i]) ? "true" : "false");
    j += ",\"rosterTooBig\":" + String(stationRosterTooBig[i] ? "true" : "false");
    j += ",\"rosterCapable\":" + String(stationRosterCapable[i] ? "true" : "false");
    j += "}";
  }
  j += "]";

  j += ",\"lastScan\":{";
  j += "\"uid\":\"" + jsonEscape(maskUID(lastScannedUID)) + "\"";
  j += ",\"id\":\"" + jsonEscape(lastScannedStudentId) + "\"";
  j += ",\"ref\":\"" + jsonEscape(lastScannedRefNo) + "\"";
  j += ",\"status\":\"" + jsonEscape(lastScannedStatus) + "\"";
  j += ",\"station\":" + String(lastScannedStation) + "}";
  j += "}";

  server.send(200, "application/json; charset=utf-8", j);
}

// ---------------------------------------------------------------------------
// ข้อมูลสำหรับจอสาธารณะ — เปิดได้โดยไม่ต้องเข้าสู่ระบบ จึงส่งเฉพาะข้อมูลที่
// เปิดเผยได้: ยอดรวม สถานะร้าน และรายการที่ปิดบังรหัสนิสิตแล้ว
// ไม่มีชื่อนิสิต ไม่มีเลขบัตร ไม่มีเลขอ้างอิง และไม่มีข้อมูลฮาร์ดแวร์แม่ข่าย
// ---------------------------------------------------------------------------
void handleDisplayAPI() {
  if (!publicDisplayEnabled) {
    server.send(403, "application/json; charset=utf-8", "{\"ok\":false,\"msg\":\"DISPLAY_DISABLED\"}");
    return;
  }

  int usedCount = 0;
  int shopCounts[4] = {0, 0, 0, 0};
  for (const auto& st : db) {
    if (st.claimed) {
      usedCount++;
      if (st.station >= 1 && st.station <= 4) shopCounts[st.station - 1]++;
    }
  }
  int total = (int)db.size();
  int quotaPct = (total > 0) ? (usedCount * 100) / total : 0;

  char win[16];
  snprintf(win, sizeof(win), "%02d:%02d-%02d:%02d", serviceStartHour, serviceStartMin, serviceEndHour, serviceEndMin);

  String j = "{\"ok\":true";
  j += ",\"clock\":\"" + jsonEscape(getTimeOnlyStr()) + "\"";
  j += ",\"date\":\"" + jsonEscape(getDateFormattedStr()) + "\"";
  j += ",\"serviceOpen\":" + String(isWithinServiceTime() ? "true" : "false");
  j += ",\"window\":\"" + String(win) + "\"";
  j += ",\"used\":" + String(usedCount);
  j += ",\"total\":" + String(total);
  j += ",\"remaining\":" + String(total - usedCount);
  j += ",\"disbursed\":" + String(usedCount * 35);
  j += ",\"quotaPct\":" + String(quotaPct);

  j += ",\"shops\":[";
  for (int i = 0; i < 4; i++) {
    if (i) j += ",";
    j += "{\"name\":\"" + jsonEscape(shops[i].name) + "\"";
    j += ",\"count\":" + String(shopCounts[i]);
    j += ",\"amount\":" + String(shopCounts[i] * 35);
    j += ",\"online\":" + String(stationNodes[i].isOnline ? "true" : "false") + "}";
  }
  j += "]";

  // เรียงจากรายการใหม่สุดไปเก่าสุด
  j += ",\"events\":[";
  for (int i = 0; i < displayFeedCount; i++) {
    int idx = (displayFeedHead - 1 - i + DISPLAY_FEED_SIZE * 2) % DISPLAY_FEED_SIZE;
    if (i) j += ",";
    j += "{\"id\":\"" + jsonEscape(String(displayFeed[idx].maskedId)) + "\"";
    j += ",\"time\":\"" + jsonEscape(String(displayFeed[idx].timeStr)) + "\"";
    j += ",\"station\":" + String(displayFeed[idx].station) + "}";
  }
  j += "]}";

  server.send(200, "application/json; charset=utf-8", j);
}

void handleManualClaim() {
  if (!requireAuth()) return;
  String id = server.arg("id"); id.trim();
  int station = server.arg("station").toInt();
  if (station < 1 || station > 4) { sendJson(false, "หมายเลขจุดบริการต้องอยู่ระหว่าง 1-4"); return; }

  for (auto& st : db) {
    if (st.studentId == id) {
      if (st.claimed) { sendJson(false, "นิสิต " + id + " ใช้สิทธิ์ของวันนี้ไปแล้ว"); return; }
      st.claimed = true; st.station = station;
      st.claimTime = getRealTimeStr(); st.refNo = generateRefNo(station);
      lastScannedUID = st.uid; lastScannedStudentId = st.studentId;
      lastScannedStation = station; lastScannedStatus = "APPROVED";
      lastScannedRefNo = st.refNo;
      appendLogToFS(st.studentId, st.fullName, st.uid, st.refNo, st.claimTime, station, st.isTempCard ? "Temp Card" : "Normal");
      pushDisplayEvent(st.studentId, (uint8_t)station, st.claimTime);
      if (printerAutoSlip) printClaimSlip(st);

      if (st.isTempCard) {
        st.uid = st.originalUid;
        st.originalUid = "";
        st.isTempCard = false;
      }
      saveDatabaseToFS();
      renderHostPage(true);
      sendJson(true, "ตัดสิทธิ์ด้วยตนเองให้รหัส " + id + " ที่จุดบริการ " + String(station) + " เรียบร้อย");
      return;
    }
  }
  sendJson(false, "ไม่พบรหัสนิสิต " + id + " ในระบบ");
}

void handleDailyReset() {
  if (!requireAuth()) return;

  if (LittleFS.exists("/daily_log.csv")) {
    DateTime now = rtc.now();
    char arcName[40];
    snprintf(arcName, sizeof(arcName), "/arc_%04d%02d%02d_%02d%02d%02d.csv",
             now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    File src = LittleFS.open("/daily_log.csv", "r");
    File dst = LittleFS.open(arcName, "w");
    if (src && dst) {
      uint8_t buf[256];
      while (src.available()) {
        size_t n = src.read(buf, sizeof(buf));
        dst.write(buf, n);
      }
    }
    if (src) src.close();
    if (dst) dst.close();
    LittleFS.remove("/daily_log.csv");
  }

  for (auto& st : db) {
    if (st.isTempCard) { st.uid = st.originalUid; }
    st.originalUid = "";
    st.claimed = false; st.claimTime = "-"; st.refNo = "-"; st.station = 0; st.isTempCard = false;
  }
  saveDatabaseToFS();
  displayFeedCount = 0;
  displayFeedHead = 0;
  lastScannedUID = "-"; lastScannedStudentId = "-"; lastScannedRefNo = "-";
  lastScannedStation = 0; lastScannedStatus = "RESET";
  renderHostPage(true);
  sendJson(true, "ปิดยอดประจำวันและจัดเก็บประวัติเรียบร้อยแล้ว");
}

void handleSaveShops() {
  if (!requireAuth()) return;
  for (int i = 0; i < 4; i++) {
    String name = server.arg("sname" + String(i));
    String vendor = server.arg("vname" + String(i));
    name.trim(); vendor.trim();
    if (name.length() == 0) { sendJson(false, "กรุณากรอกชื่อร้านที่ " + String(i + 1)); return; }
    if (name.length() > 48 || vendor.length() > 64) { sendJson(false, "ชื่อร้าน/ผู้ประกอบการยาวเกินกำหนด"); return; }
    shops[i].name = name;
    shops[i].vendor = vendor;
  }
  saveShopsToFS();
  renderHostPage(true);
  sendJson(true, "บันทึกข้อมูลร้านค้าเรียบร้อยแล้ว");
}

void handleGetStudentsAPI() {
  if (!requireAuth()) return;
  int page = server.hasArg("page") ? server.arg("page").toInt() : 1;
  int limit = server.hasArg("limit") ? server.arg("limit").toInt() : 20;
  String search = server.hasArg("search") ? server.arg("search") : "";
  search.trim(); search.toUpperCase();
  if (page < 1) page = 1;
  // เดิมไม่จำกัดเพดาน limit ผู้ใช้จึงขอ 1000 แถวได้ แล้วเอกสาร JSON ขนาด 8KB
  // จะล้นจนสตริงที่ส่งกลับไม่สมบูรณ์ ทำให้หน้าเว็บค้าง
  if (limit < 1) limit = 20;
  if (limit > 100) limit = 100;

  std::vector<int> matchedIndices;
  for (size_t i = 0; i < db.size(); i++) {
    if (search.length() == 0) matchedIndices.push_back(i);
    else {
      String sId = db[i].studentId; sId.toUpperCase();
      String fName = db[i].fullName; fName.toUpperCase();
      String rNo = db[i].refNo; rNo.toUpperCase();
      String uId = db[i].uid; uId.toUpperCase();
      if (sId.indexOf(search) != -1 || fName.indexOf(search) != -1 || rNo.indexOf(search) != -1 || uId.indexOf(search) != -1) matchedIndices.push_back(i);
    }
  }

  int totalItems = matchedIndices.size();
  int totalPages = (totalItems + limit - 1) / limit;
  if (totalPages < 1) totalPages = 1;
  if (page > totalPages) page = totalPages;

  int startIdx = (page - 1) * limit;
  int endIdx = min(startIdx + limit, totalItems);

  DynamicJsonDocument doc(1024 + (size_t)limit * 420);
  doc["totalItems"] = totalItems;
  doc["totalPages"] = totalPages;
  doc["currentPage"] = page;
  doc["limit"] = limit;

  JsonArray arr = doc.createNestedArray("students");
  for (int i = startIdx; i < endIdx; i++) {
    int idx = matchedIndices[i];
    JsonObject obj = arr.createNestedObject();
    obj["id"] = db[idx].studentId; obj["name"] = db[idx].fullName;
    obj["uid"] = db[idx].uid; obj["claimed"] = db[idx].claimed;
    obj["time"] = db[idx].claimTime; obj["ref"] = db[idx].refNo;
    obj["station"] = db[idx].station; obj["isTemp"] = db[idx].isTempCard;
    obj["shopName"] = (db[idx].station > 0 && db[idx].station <= 4) ? shops[db[idx].station - 1].name : "-";
  }

  String res;
  serializeJson(doc, res);
  server.send(200, "application/json; charset=utf-8", res);
}

void handleExportCSV() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  int shopCounts[4] = {0, 0, 0, 0};
  int totalClaimed = 0;

  for (const auto& s : db) {
    if (s.claimed && s.station >= 1 && s.station <= 4) {
      shopCounts[s.station - 1]++;
      totalClaimed++;
    }
  }

  String csv;
  csv.reserve(2048 + db.size() * 160);
  csv += "\xEF\xBB\xBF";
  csv += "MCU Phrae Canteen Daily Claim Report (Claimed Only)\n";
  csv += "Export Date," + getRealTimeStr() + "\n\n";

  csv += "--- Summary Payout By Shop ---\n";
  csv += "No.,Shop Name,Vendor,Total Orders,Total Payout (THB)\n";
  for (int i = 0; i < 4; i++) {
    csv += String(i + 1) + "," + csvQuote(shops[i].name) + "," + csvQuote(shops[i].vendor) + ","
        + String(shopCounts[i]) + "," + String(shopCounts[i] * 35) + "\n";
  }
  csv += "Total Payout,,," + String(totalClaimed) + "," + String(totalClaimed * 35) + "\n\n";

  csv += "--- Verified Claim Records ---\n";
  csv += "No.,Student ID,Full Name,Card UID (10-Digit),Reference No.,Claim Timestamp,Shop,Vendor,Amount (THB),Card Type\n";

  int rowNumber = 1;
  for (const auto& s : db) {
    if (!s.claimed) continue;
    String shopName = (s.station >= 1 && s.station <= 4) ? shops[s.station - 1].name : "-";
    String vendorName = (s.station >= 1 && s.station <= 4) ? shops[s.station - 1].vendor : "-";
    String cardType = s.isTempCard ? "Temporary Card" : "Standard Card";

    csv += String(rowNumber++) + ",";
    csv += csvQuote(s.studentId) + ",";
    csv += csvQuote(s.fullName) + ",";
    csv += csvQuote(s.uid) + ",";
    csv += csvQuote(s.refNo) + ",";
    csv += csvQuote(s.claimTime) + ",";
    csv += csvQuote(shopName) + ",";
    csv += csvQuote(vendorName) + ",";
    csv += "35,";
    csv += csvQuote(cardType) + "\n";
  }

  server.sendHeader("Content-Disposition", "attachment; filename=MCU_Canteen_Claim_Report.csv");
  server.send(200, "text/csv; charset=utf-8", csv);
}

void handleSetRTCTime() {
  if (!requireAuth()) return;
  if (server.hasArg("date") && server.hasArg("time")) {
    String dStr = server.arg("date");
    String tStr = server.arg("time");
    int y, m, d, h, mi, sec;
    if (sscanf(dStr.c_str(), "%d-%d-%d", &y, &m, &d) == 3 &&
        sscanf(tStr.c_str(), "%d:%d:%d", &h, &mi, &sec) == 3 &&
        y >= 2000 && y <= 2099 && m >= 1 && m <= 12 && d >= 1 && d <= 31 &&
        h >= 0 && h <= 23 && mi >= 0 && mi <= 59 && sec >= 0 && sec <= 59) {
      rtc.adjust(DateTime(y, m, d, h, mi, sec));
      sendJson(true, "ตั้งค่านาฬิกา RTC DS3231 สำเร็จเรียบร้อย");
      return;
    }
  }
  sendJson(false, "รูปแบบวันที่หรือเวลาไม่ถูกต้อง");
}

// อนุญาตเฉพาะไฟล์ประวัติชื่อ arc_*.csv ที่อยู่ระดับรากเท่านั้น
// เดิมพารามิเตอร์ file ไม่ถูกกรอง จึงดาวน์โหลด /admins.json ที่เก็บรหัสผ่านได้
bool isSafeArchiveName(const String &name) {
  if (!name.startsWith("arc_") || !name.endsWith(".csv")) return false;
  if (name.length() > 48) return false;
  for (size_t i = 0; i < name.length(); i++) {
    char c = name[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
    if (!ok) return false;
  }
  if (name.indexOf("..") != -1) return false;
  return true;
}

void handleDownloadArchive() {
  if (!requireAuth(false)) return;
  String filename = server.arg("file");
  filename.trim();
  if (filename.startsWith("/")) filename = filename.substring(1);
  if (isSafeArchiveName(filename) && LittleFS.exists("/" + filename)) {
    File f = LittleFS.open("/" + filename, "r");
    if (f) {
      server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
      server.streamFile(f, "text/csv");
      f.close();
      return;
    }
  }
  server.send(404, "text/plain", "Archive Not Found");
}

void handleDeleteArchive() {
  if (!requireAuth()) return;
  String filename = server.arg("file");
  filename.trim();
  if (filename.startsWith("/")) filename = filename.substring(1);
  if (isSafeArchiveName(filename) && LittleFS.exists("/" + filename)) {
    LittleFS.remove("/" + filename);
    sendJson(true, "ลบไฟล์ประวัติ " + filename + " เรียบร้อยแล้ว");
    return;
  }
  sendJson(false, "ไม่สามารถลบไฟล์ได้");
}

void handleSaveStudent() {
  if (!requireAuth()) return;
  String oldId = server.arg("oldStudentId"); String sId = server.arg("studentId");
  String name = server.arg("fullName"); String uid = server.arg("uid");
  oldId.trim(); sId.trim(); name.trim(); uid.trim();

  if (sId.length() == 0) { sendJson(false, "กรุณากรอกรหัสนิสิต"); return; }
  if (sId.length() > 24 || name.length() > 96 || uid.length() > 15) {
    sendJson(false, "ข้อมูลยาวเกินกำหนด (รหัส 24 / ชื่อ 96 / UID 15 อักขระ)"); return;
  }
  // กันรหัสนิสิตซ้ำ ซึ่งเดิมเพิ่มซ้ำได้และทำให้การตัดสิทธิ์ไปโดนระเบียนผิดตัว
  for (const auto& s : db) {
    if (s.studentId == sId && sId != oldId) { sendJson(false, "รหัสนิสิต " + sId + " มีอยู่ในระบบแล้ว"); return; }
  }
  if (uid.length() > 0) {
    for (const auto& s : db) {
      if (s.uid == uid && s.studentId != oldId && s.studentId != sId) {
        sendJson(false, "เลขบัตรนี้ถูกผูกกับรหัส " + s.studentId + " อยู่แล้ว"); return;
      }
    }
  }

  if (oldId != "") {
    bool found = false;
    for (auto& s : db) {
      if (s.studentId == oldId) {
        s.studentId = sId; s.fullName = name;
        // แก้ไข UID ของบัตรสำรองต้องไปเขียนที่ originalUid ไม่ใช่ทับบัตรชั่วคราว
        if (s.isTempCard) s.originalUid = uid;
        else s.uid = uid;
        found = true; break;
      }
    }
    if (!found) { sendJson(false, "ไม่พบรหัสนิสิต " + oldId + " ในระบบ"); return; }
  } else {
    Student s; s.studentId = sId; s.fullName = name; s.uid = uid; s.originalUid = ""; s.claimed = false; s.claimTime = "-"; s.refNo = "-"; s.station = 0; s.isTempCard = false;
    db.push_back(s);
  }
  saveDatabaseToFS(); renderHostPage(true);
  sendJson(true, "บันทึกข้อมูลผู้มีสิทธิ์เรียบร้อยแล้ว");
}

void handleSaveTempCard() {
  if (!requireAuth()) return;
  String sId = server.arg("studentId"); String uid = server.arg("uid");
  sId.trim(); uid.trim();
  if (sId.length() == 0 || uid.length() == 0) { sendJson(false, "กรุณากรอกรหัสนิสิตและเลขบัตรสำรอง"); return; }
  if (uid.length() > 15) { sendJson(false, "เลขบัตรยาวเกิน 15 อักขระ"); return; }

  for (const auto& s : db) {
    if (s.uid == uid && s.studentId != sId) {
      sendJson(false, "บัตรใบนี้ถูกผูกกับรหัส " + s.studentId + " อยู่แล้ว"); return;
    }
  }

  bool found = false;
  for (auto& s : db) {
    if (s.studentId == sId) {
      if (s.claimed) { sendJson(false, "นิสิต " + sId + " ใช้สิทธิ์ของวันนี้ไปแล้ว"); return; }
      if (s.originalUid == "") s.originalUid = s.uid;
      s.uid = uid;
      s.isTempCard = true;
      found = true;
      break;
    }
  }
  if (!found) { sendJson(false, "ไม่พบรหัสนิสิต " + sId + " ในระบบ"); return; }
  saveDatabaseToFS(); renderHostPage(true);
  sendJson(true, "ผูกบัตรสำรองให้รหัส " + sId + " เรียบร้อยแล้ว");
}

void handleRemoveTempCard() {
  if (!requireAuth()) return;
  String id = server.arg("id"); id.trim();
  bool found = false;
  for (auto& s : db) {
    if (s.studentId == id) {
      s.uid = s.originalUid;   // ว่างได้ ถ้าเดิมนิสิตยังไม่เคยมีบัตรประจำตัว
      s.originalUid = "";
      s.isTempCard = false;
      found = true;
      break;
    }
  }
  if (!found) { sendJson(false, "ไม่พบรหัสนิสิต " + id + " ในระบบ"); return; }
  saveDatabaseToFS(); renderHostPage(true);
  sendJson(true, "ยกเลิกบัตรสำรองของรหัส " + id + " เรียบร้อยแล้ว");
}

void handleDeleteStudent() {
  if (!requireAuth()) return;
  String id = server.arg("id"); id.trim();
  bool found = false;
  for (auto it = db.begin(); it != db.end(); ++it) {
    if (it->studentId == id) { db.erase(it); found = true; break; }
  }
  if (!found) { sendJson(false, "ไม่พบรหัสนิสิต " + id + " ในระบบ"); return; }
  saveDatabaseToFS(); renderHostPage(true);
  sendJson(true, "ลบรายชื่อรหัส " + id + " เรียบร้อยแล้ว");
}

void handleSaveAdmin() {
  if (!requireAuth()) return;
  String oldUser = server.arg("oldUsername");
  String user = server.arg("username");
  String pass = server.arg("password");
  String dName = server.arg("displayName");
  oldUser.trim(); user.trim(); pass.trim(); dName.trim();

  if (user.length() < 3 || user.length() > 24) { sendJson(false, "ชื่อผู้ใช้ต้องยาว 3-24 อักขระ"); return; }
  if (dName.length() == 0 || dName.length() > 64) { sendJson(false, "กรุณากรอกชื่อ-ตำแหน่ง (ไม่เกิน 64 อักขระ)"); return; }
  if (oldUser == "" && pass.length() < 8) { sendJson(false, "รหัสผ่านต้องยาวอย่างน้อย 8 อักขระ"); return; }
  if (pass.length() > 0 && pass.length() < 8) { sendJson(false, "รหัสผ่านต้องยาวอย่างน้อย 8 อักขระ"); return; }
  for (const auto& u : adminUsers) {
    if (u.username == user && u.username != oldUser) { sendJson(false, "ชื่อผู้ใช้นี้มีในระบบแล้ว"); return; }
  }

  if (oldUser != "") {
    for (auto& u : adminUsers) {
      if (u.username == oldUser) {
        u.username = user;
        if (pass.length() > 0) u.password = pass;
        u.displayName = dName;
        break;
      }
    }
  } else {
    if (adminUsers.size() >= 3) {
      sendJson(false, "ระบบจำกัดเจ้าหน้าที่ไม่เกิน 3 ท่าน");
      return;
    }
    AdminUser nu;
    nu.username = user;
    nu.password = pass;
    nu.displayName = dName;
    adminUsers.push_back(nu);
  }
  saveAdminsToFS();
  sendJson(true, "บันทึกข้อมูลเจ้าหน้าที่เรียบร้อยแล้ว");
}

void handleDeleteAdmin() {
  if (!requireAuth()) return;
  if (adminUsers.size() <= 1) {
    sendJson(false, "ไม่สามารถลบได้ ต้องมีเจ้าหน้าที่อย่างน้อย 1 ท่านในระบบ");
    return;
  }
  String user = server.arg("user"); user.trim();
  bool found = false;
  for (auto it = adminUsers.begin(); it != adminUsers.end(); ++it) {
    if (it->username == user) {
      // ปิด session ที่ยังเปิดค้างของบัญชีที่ถูกลบทันที
      for (int i = 0; i < 3; i++) {
        if (activeSessions[i].username == user) { activeSessions[i].token = ""; activeSessions[i].username = ""; }
      }
      adminUsers.erase(it);
      found = true;
      break;
    }
  }
  if (!found) { sendJson(false, "ไม่พบผู้ใช้ " + user + " ในระบบ"); return; }
  saveAdminsToFS();
  sendJson(true, "ลบเจ้าหน้าที่เรียบร้อยแล้ว");
}

void saveShopsToFS() {
  File file = LittleFS.open("/shops.json", "w");
  if (!file) return;
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  for (int i = 0; i < 4; i++) {
    JsonObject obj = array.createNestedObject();
    obj["name"] = shops[i].name; obj["vendor"] = shops[i].vendor;
  }
  serializeJson(doc, file); file.close();
}

void loadShopsFromFS() {
  if (!LittleFS.exists("/shops.json")) { saveShopsToFS(); return; }
  File file = LittleFS.open("/shops.json", "r");
  if (!file) return;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, file);
  JsonArray array = doc.as<JsonArray>();
  for (int i = 0; i < 4 && i < array.size(); i++) {
    shops[i].name = array[i]["name"].as<String>();
    shops[i].vendor = array[i]["vendor"].as<String>();
  }
  file.close();
}

void appendLogToFS(String studentId, String fullName, String uid, String refNo, String timestamp, int station, String type) {
  File logFile = LittleFS.open("/daily_log.csv", FILE_APPEND);
  if (logFile) {
    // ครอบทุกฟิลด์ด้วยเครื่องหมายคำพูด ชื่อที่มีลูกน้ำจึงไม่ดันฟิลด์อื่นเพี้ยน
    String line = csvQuote(studentId) + "," + csvQuote(fullName) + "," + csvQuote(uid) + "," +
                  csvQuote(refNo) + "," + csvQuote(timestamp) + "," + String(station) + ",35," + csvQuote(type);
    logFile.println(line);
    logFile.close();
  }
}

void restoreDailyLogs() {
  if (!LittleFS.exists("/daily_log.csv")) return;
  File logFile = LittleFS.open("/daily_log.csv", "r");
  if (!logFile) return;
  while (logFile.available()) {
    String line = logFile.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    String f[8];
    int n = parseCsvLine(line, f, 8);
    if (n >= 6) {
      String sId = f[0];
      String ref = f[3];
      String tStamp = f[4];
      int st = f[5].toInt();
      String type = (n >= 8) ? f[7] : "Normal";
      for (auto& s : db) {
        if (s.studentId == sId) {
          s.claimed = true; s.refNo = ref; s.claimTime = tStamp; s.station = st;
          break;
        }
      }
    }
  }
  logFile.close();
}

void saveDatabaseToFS() {
  // ทุกเส้นทางที่แก้ "ชุดผู้มีสิทธิ์" (เพิ่ม ลบ แก้ไข ผูกบัตรสำรอง นำเข้า CSV ปิดยอด)
  // ผ่านฟังก์ชันนี้ทั้งหมด จึงเป็นจุดเดียวที่ต้องเลื่อนเลขรุ่นบัญชีและผลักของใหม่ให้สถานี
  bumpRosterVer();
  for (int i = 0; i < 4; i++) {
    if (stationNodes[i].isOnline && stationRosterCapable[i]) startRosterPush(i + 1);
  }

  File file = LittleFS.open("/students.csv", "w");
  if (!file) return;
  file.println("studentId,fullName,uid");
  for (const auto& s : db) {
    String permUid = s.isTempCard ? s.originalUid : s.uid;
    file.println(csvQuote(s.studentId) + "," + csvQuote(s.fullName) + "," + csvQuote(permUid));
  }
  file.close();
}

void loadDatabaseFromFS() {
  db.clear();
  if (!LittleFS.exists("/students.csv")) return;
  File file = LittleFS.open("/students.csv", "r");
  if (!file) return;
  bool isHeader = true;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (isHeader) { isHeader = false; if (line.indexOf("studentId") != -1) continue; }
    String f[3];
    int n = parseCsvLine(line, f, 3);
    if (n >= 1 && f[0].length() > 0) {
      Student s;
      s.studentId = f[0];
      s.fullName = (n >= 2) ? f[1] : "";
      s.uid = (n >= 3) ? f[2] : "";
      s.originalUid = "";
      s.claimed = false; s.claimTime = "-"; s.refNo = "-"; s.station = 0; s.isTempCard = false;
      db.push_back(s);
    }
  }
  file.close();
  restoreDailyLogs();
}

void mergeImportedStudents() {
  if (!requireAuth()) return;
  if (!LittleFS.exists("/temp_import.csv")) { sendJson(false, "ไม่พบไฟล์ที่อัปโหลด"); return; }
  File file = LittleFS.open("/temp_import.csv", "r");
  if (!file) { sendJson(false, "เปิดไฟล์ที่อัปโหลดไม่สำเร็จ"); return; }

  bool isHeader = true;
  int newCount = 0;
  int updatedCount = 0;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    if (isHeader) {
      isHeader = false;
      if (line.indexOf("studentId") != -1 || line.indexOf("รหัส") != -1) continue;
    }

    String f[3];
    int n = parseCsvLine(line, f, 3);
    {
      String sId = f[0];
      String fName = (n >= 2) ? f[1] : "";
      String uId = (n >= 3) ? f[2] : "";

      if (sId.length() == 0) continue;

      bool exists = false;
      for (auto& s : db) {
        if (s.studentId == sId) {
          if (fName.length() > 0) s.fullName = fName;
          if (uId.length() > 0) {
            // ถ้ากำลังถือบัตรสำรองอยู่ ให้ไปอัปเดตบัตรประจำตัวจริงแทน
            if (s.isTempCard) s.originalUid = uId;
            else { s.uid = uId; s.originalUid = ""; }
          }
          exists = true;
          updatedCount++;
          break;
        }
      }

      if (!exists) {
        Student s;
        s.studentId = sId;
        s.fullName = fName;
        s.uid = uId;
        s.originalUid = "";
        s.claimed = false;
        s.claimTime = "-";
        s.refNo = "-";
        s.station = 0;
        s.isTempCard = false;
        db.push_back(s);
        newCount++;
      }
    }
  }
  file.close();
  LittleFS.remove("/temp_import.csv");

  saveDatabaseToFS();
  renderHostPage(true);

  String msg = "นำเข้าสำเร็จ — เพิ่มใหม่ " + String(newCount) + " รายการ, ปรับปรุง " + String(updatedCount) + " รายการ";
  sendJson(true, msg);
}

void handleFileUpload() {
  if (!isAuthenticated()) return;
  // หมายเหตุ: ตัวจัดการอัปโหลดตอบกลับไม่ได้ ผลลัพธ์จริงถูกแจ้งใน mergeImportedStudents()
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    LittleFS.remove("/temp_import.csv");
    fsUploadFile = LittleFS.open("/temp_import.csv", "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) fsUploadFile.close();
  }
}

// หมายเหตุ: เนื้อหาสลิปและปลายทาง API ของเครื่องพิมพ์ย้ายไปอยู่ใน HostSlips.h
// (ถูก #include ไว้ท้ายไฟล์ก่อน setup())

// ============================================================================
// ส่วนที่แยกออกไปเป็นไฟล์ของตัวเอง ทั้งหมดถูก #include ตรงนี้เพราะโค้ดข้างใน
// อ้างถึงตัวแปรส่วนกลางและฟังก์ชันช่วยเหลือที่ประกาศไว้ข้างบน
// **อย่าย้ายบรรทัด #include เหล่านี้ขึ้นไปไว้บนสุด**
// ============================================================================
#include "HostDisplay.h"
#include "HostSlips.h"

// ============================================================================
// หน้าเว็บทั้งหมดอยู่ในไฟล์ WebPortal.h (แท็บถัดไปใน Arduino IDE)
// วางไว้ตรงนี้เพราะโค้ดข้างในอ้างถึงตัวแปรส่วนกลางและฟังก์ชันช่วยเหลือข้างบน
// ============================================================================
#include "WebPortal.h"

// ============================================================================
// SETUP FUNCTION
// ============================================================================
void setup() {
  pinMode(TFT_BLK, OUTPUT); digitalWrite(TFT_BLK, HIGH);
  pinMode(BUZZER_PIN, OUTPUT); noTone(BUZZER_PIN);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(RGB_LED_PIN, OUTPUT);
  analogSetAttenuation(ADC_11db);

  SPI_TFT.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(240, 320); 
  tft.setRotation(1); 
  tft.setTextWrap(false);
  tft.invertDisplay(false);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (!rtc.begin()) tone(BUZZER_PIN, 1000, 200);
  if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  scanQueue = xQueueCreate(16, sizeof(ScanQueueItem));

  LittleFS.begin(true);
  loadShopsFromFS();
  loadDatabaseFromFS();
  loadAdminsFromFS();

  preferences.begin("sys_cfg", false);
  timeWindowEnabled = preferences.getBool("win_en", true);
  serviceStartHour  = preferences.getInt("st_h", 10);
  serviceStartMin   = preferences.getInt("st_m", 0);
  serviceEndHour    = preferences.getInt("end_h", 13);
  serviceEndMin     = preferences.getInt("end_m", 30);
  isTftDarkMode     = preferences.getBool("tft_dark", true);
  publicDisplayEnabled = preferences.getBool("pub_disp", true);
  printerAutoSlip      = preferences.getBool("prn_auto", true);
  preferences.end();

  // เริ่มสแต็กเครื่องพิมพ์ก่อนเปิด Wi-Fi เผื่อฝั่ง USB host ต้องใช้เวลาจับอุปกรณ์
  printerBegin();

  WiFi.mode(WIFI_AP);
  // เดิมรับได้ 4 เครื่อง ซึ่งจอสาธารณะจะกินไปหนึ่งช่อง เหลือให้เจ้าหน้าที่แค่สาม
  WiFi.softAP(default_ap_ssid, default_ap_pass, ESPNOW_CHANNEL, 0, 8);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
  // เพิ่ม WIFI_PROTOCOL_LR เพื่อให้คุยกับสถานีในโหมดระยะไกลได้ การใส่เพิ่มเฉย ๆ
  // ปลอดภัยเพราะยังคง 11b/g/n ไว้ โทรศัพท์และคอมพิวเตอร์จึงต่อ Wi-Fi ได้ตามปกติ
  esp_wifi_set_protocol(WIFI_IF_AP,
                        WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);
  // 80 = 20 dBm ซึ่งเป็นค่าสูงสุดของ ESP32-S3 เดิมตั้งไว้ 68 = 17 dBm
  esp_wifi_set_max_tx_power(80);

  // ตั้งค่าที่ captive portal ต้องการ: ตอบทุกโดเมนมาที่ตัวเอง และ TTL = 0
  // เพื่อไม่ให้โทรศัพท์จำการชี้โดเมนนี้ไว้หลังตัดการเชื่อมต่อไปแล้ว
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.setTTL(0);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  esp_now_init();
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.ifidx = WIFI_IF_AP;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  applyEspNowRate(broadcastAddress);

  delay(50);
  broadcastStationConfig();

  MDNS.begin(mdns_hostname);
  MDNS.addService("http", "tcp", 80);

  const char * headerkeys[] = {"Cookie"};
  server.collectHeaders(headerkeys, 1);

  server.on("/login", HTTP_GET, []() {
    String err = "";
    String code = server.arg("error");
    if (code == "1") err = "ชื่อผู้ใช้หรือรหัสผ่านไม่ถูกต้อง กรุณาลองใหม่อีกครั้ง";
    else if (code == "lock") err = "ป้อนรหัสผิดหลายครั้งเกินกำหนด กรุณารอ 1 นาทีแล้วลองใหม่";
    server.send(200, "text/html; charset=utf-8", getLoginHTML(err));
  });

  server.on("/login", HTTP_POST, []() {
    // หน่วงเวลาเมื่อพิมพ์รหัสผิดติดกันหลายครั้ง เพื่อกันการไล่เดารหัสผ่าน
    if (loginLockUntil != 0 && (long)(millis() - loginLockUntil) < 0) {
      server.sendHeader("Location", "/login?error=lock", true);
      server.send(302, "text/plain", "");
      return;
    }

    String user = server.arg("username");
    String pass = server.arg("password");
    user.trim(); pass.trim();

    bool ok = false;
    String matchedUser = "";
    for (const auto& u : adminUsers) {
      if (u.username == user && u.password == pass) {
        ok = true;
        matchedUser = u.username;
        break;
      }
    }

    if (ok) {
      loginFailCount = 0;
      loginLockUntil = 0;

      String token = generateSessionToken();
      int slot = 0;
      for (int i = 0; i < 3; i++) {
        if (activeSessions[i].token == "" || (long)(millis() - activeSessions[i].expiry) >= 0) {
          slot = i;
          break;
        }
      }
      activeSessions[slot].token = token;
      activeSessions[slot].username = matchedUser;
      activeSessions[slot].expiry = millis() + 7200000UL;

      // SameSite=Strict ปิดช่องทางที่เว็บอื่นยิงคำสั่งเข้ามาพร้อมคุกกี้ของเรา
      server.sendHeader("Set-Cookie",
                        "CANTEEN_SESSION=" + token + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=7200");
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
    } else {
      loginFailCount++;
      if (loginFailCount >= LOGIN_MAX_FAILS) {
        loginFailCount = 0;
        loginLockUntil = millis() + LOGIN_LOCK_MS;
        server.sendHeader("Location", "/login?error=lock", true);
        server.send(302, "text/plain", "");
        return;
      }
      server.sendHeader("Location", "/login?error=1", true);
      server.send(302, "text/plain", "");
    }
  });

  server.on("/logout", HTTP_GET, []() {
    // ล้าง session ฝั่งเซิร์ฟเวอร์ด้วย ไม่ใช่แค่ลบคุกกี้ฝั่งเบราว์เซอร์
    if (server.hasHeader("Cookie")) {
      String cookie = server.header("Cookie");
      int idx = cookie.indexOf("CANTEEN_SESSION=");
      if (idx != -1) {
        String tok = cookie.substring(idx + 16);
        int semi = tok.indexOf(';');
        if (semi != -1) tok = tok.substring(0, semi);
        tok.trim();
        for (int i = 0; i < 3; i++) {
          if (activeSessions[i].token == tok) { activeSessions[i].token = ""; activeSessions[i].username = ""; }
        }
      }
    }
    server.sendHeader("Set-Cookie", "CANTEEN_SESSION=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
    server.sendHeader("Location", "/login", true);
    server.send(302, "text/plain", "");
  });

  server.on("/", HTTP_GET, []() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    server.send(200, "text/html; charset=utf-8", getHTML());
  });

  // ---------------------------------------------------------------------
  // เส้นทาง API — คำสั่งที่เปลี่ยนแปลงข้อมูลทุกตัวย้ายมาเป็น POST ทั้งหมด
  // เดิมเป็น GET จึงถูกเบราว์เซอร์ prefetch หรือถูกเว็บอื่นยิงเข้ามาได้
  // ---------------------------------------------------------------------
  server.on("/api/students", HTTP_GET, handleGetStudentsAPI);
  server.on("/export.csv", HTTP_GET, handleExportCSV);
  server.on("/api/archive/download", HTTP_GET, handleDownloadArchive);
  server.on("/api/dashboard", HTTP_GET, handleDashboardAPI);

  // จอสาธารณะ: เปิดได้โดยไม่ต้องเข้าสู่ระบบ จึงไม่กินช่อง session ของเจ้าหน้าที่
  server.on("/display", HTTP_GET, []() {
    if (!publicDisplayEnabled) { redirectToLogin(); return; }
    server.send(200, "text/html; charset=utf-8", getDisplayHTML());
  });
  server.on("/api/display", HTTP_GET, handleDisplayAPI);
  server.on("/api/system/health", HTTP_GET, handleDashboardAPI);

  server.on("/api/rtc/set", HTTP_POST, handleSetRTCTime);
  server.on("/api/archive/delete", HTTP_POST, handleDeleteArchive);
  server.on("/api/student/save", HTTP_POST, handleSaveStudent);
  server.on("/api/student/delete", HTTP_POST, handleDeleteStudent);
  server.on("/api/tempcard/save", HTTP_POST, handleSaveTempCard);
  server.on("/api/tempcard/remove", HTTP_POST, handleRemoveTempCard);
  server.on("/api/admin/save", HTTP_POST, handleSaveAdmin);
  server.on("/api/admin/delete", HTTP_POST, handleDeleteAdmin);
  server.on("/api/shops/save", HTTP_POST, handleSaveShops);
  server.on("/api/claim/manual", HTTP_POST, handleManualClaim);
  server.on("/api/system/reset", HTTP_POST, handleDailyReset);
  server.on("/api/print/test", HTTP_POST, handlePrintTest);
  server.on("/api/print/slip", HTTP_POST, handlePrintSlip);
  server.on("/api/print/daily", HTTP_POST, handlePrintDaily);
  server.on("/api/settings/printer", HTTP_POST, handleSavePrinterSettings);

  server.on("/api/settings/display", HTTP_POST, []() {
    if (!requireAuth()) return;
    publicDisplayEnabled = (server.arg("enabled") == "1");
    preferences.begin("sys_cfg", false);
    preferences.putBool("pub_disp", publicDisplayEnabled);
    preferences.end();
    sendJson(true, publicDisplayEnabled
                     ? "เปิดหน้าจอสาธารณะแล้ว เปิดดูได้ที่ /display"
                     : "ปิดหน้าจอสาธารณะแล้ว ผู้ที่ไม่ได้เข้าสู่ระบบจะเปิดหน้านี้ไม่ได้");
  });

  server.on("/api/settings/time", HTTP_POST, []() {
    if (!requireAuth()) return;
    int sh = 0, sm = 0, eh = 0, em = 0;
    String startStr = server.arg("start");
    String endStr = server.arg("end");
    if (sscanf(startStr.c_str(), "%d:%d", &sh, &sm) != 2 ||
        sscanf(endStr.c_str(), "%d:%d", &eh, &em) != 2 ||
        sh < 0 || sh > 23 || eh < 0 || eh > 23 || sm < 0 || sm > 59 || em < 0 || em > 59) {
      sendJson(false, "รูปแบบเวลาไม่ถูกต้อง (ต้องเป็น HH:MM)");
      return;
    }
    if ((sh * 60 + sm) >= (eh * 60 + em)) {
      sendJson(false, "เวลาเปิดต้องมาก่อนเวลาปิด");
      return;
    }

    timeWindowEnabled = (server.arg("enabled") == "1");
    serviceStartHour = sh; serviceStartMin = sm;
    serviceEndHour = eh;  serviceEndMin = em;

    preferences.begin("sys_cfg", false);
    preferences.putBool("win_en", timeWindowEnabled);
    preferences.putInt("st_h", serviceStartHour);
    preferences.putInt("st_m", serviceStartMin);
    preferences.putInt("end_h", serviceEndHour);
    preferences.putInt("end_m", serviceEndMin);
    preferences.end();

    renderHostPage(true);
    sendJson(true, "บันทึกกำหนดเวลาให้บริการเรียบร้อยแล้ว");
  });

  server.on("/upload", HTTP_POST, mergeImportedStudents, handleFileUpload);

  // เบราว์เซอร์ขอ favicon เองทุกครั้ง ถ้าปล่อยให้ตกไปที่ onNotFound จะกลายเป็น
  // การโหลดหน้าล็อกอินทั้งหน้ามาเป็นไอคอน เปลืองทั้งเวลาและแรมของแม่ข่าย
  server.on("/favicon.ico", HTTP_GET, []() { server.send(204, "image/x-icon", ""); });

  // ทุก URL ที่ไม่รู้จักถูกพาไปหน้าล็อกอิน ซึ่งเป็นกลไกที่ทำให้โทรศัพท์และคอมพิวเตอร์
  // เด้งหน้าต่าง "เข้าสู่ระบบเครือข่าย" ขึ้นมาเองเมื่อเชื่อมต่อ Wi-Fi ของแม่ข่าย
  // ใช้ URL แบบเต็มเพราะตัวตรวจจับของบางระบบปฏิบัติการดูโฮสต์ในส่วนหัว Location
  server.onNotFound(handleCaptivePortal);

  server.begin();
  playBootAnimation();

  for (int i = 1; i <= 4; i++) {
    HostResponsePacket beacon = {};
    beacon.magic = ESPNOW_PROTO_MAGIC;
    beacon.version = ESPNOW_PROTO_VER;
    beacon.msgType = MSG_HEARTBEAT;
    beacon.stationId = i;
    beacon.seq = 0;
    strcpy(beacon.status, "ONLINE");
    strncpy(beacon.claimTime, getRealTimeStr().c_str(), sizeof(beacon.claimTime) - 1);
    beacon.servedCount = getStationServedCount(i);
    fillSystemSummary(beacon);
    sendToStation(i, (uint8_t *)&beacon, sizeof(HostResponsePacket));
    delay(20);
  }

  lastActivity = millis();
  renderHostPage(true);
  broadcastStationConfig();
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  handleHostButton();
  calculateCpuLoad();
  printerLoop();      // ทยอยปล่อยข้อมูลสลิปออกทีละก้อน ไม่บล็อกลูปหลัก
  serviceRosterPush();// ทยอยผลักบัญชีสิทธิ์ไปยังสถานีที่ยังตามไม่ทัน

  ScanQueueItem item;
  if (scanQueue != NULL && xQueueReceive(scanQueue, &item, 0) == pdTRUE) {
    processScanRequest(item.mac, item.pkt, item.rssi);
  }

  // ถ้าชิปรายงานว่าคำตอบการสแกนส่งไม่ถึง ให้ยิงซ้ำทันทีโดยไม่ต้องรอสถานีถามใหม่
  if (lastRespRetryLeft > 0 && respSendFailed && (long)(millis() - lastRespRetryAt) >= 0) {
    respSendFailed = false;
    lastRespRetryLeft--;
    lastRespRetryAt = millis() + 260;
    sendToStation(lastRespStation, (uint8_t *)&lastRespPacket, sizeof(HostResponsePacket));
  }

  for (int i = 0; i < 4; i++) {
    if (hbAckPending[i]) {
      hbAckPending[i] = false;
      HostResponsePacket ack = {};
      ack.magic = ESPNOW_PROTO_MAGIC;
      ack.version = ESPNOW_PROTO_VER;
      ack.msgType = MSG_HEARTBEAT;
      ack.stationId = i + 1;
      ack.seq = 0;
      strcpy(ack.status, "ONLINE");
      strncpy(ack.claimTime, getRealTimeStr().c_str(), sizeof(ack.claimTime) - 1);
      ack.servedCount = getStationServedCount(i + 1);
      fillSystemSummary(ack);
      sendToStation(i + 1, (uint8_t *)&ack, sizeof(HostResponsePacket));
      // ย้ำโหมดการแสดงผลทุกครั้งที่ตอบ heartbeat เพื่อให้สถานีที่เพิ่งบูต
      // หรือที่พลาดคำสั่ง broadcast ไป กลับมาตรงกับแม่ข่ายภายในไม่กี่วินาที
      sendStationConfig(i + 1);
    }
  }

  if (isLiveScanDisplaying && (long)(millis() - liveScanHoldUntil) > 0) {
    isLiveScanDisplaying = false;
    renderHostPage(true);
  }

  if (isHostScreenOn && !isLiveScanDisplaying && !isScreensaverActive && !isCreditActive &&
      (millis() - lastActivity >= TIMEOUT_SCREENSAVER)) {
    isScreensaverActive = true;
    renderScreensaver(true);
    announceHostMode();
  }

  if (isHostScreenOn && !isLiveScanDisplaying && (isScreensaverActive || currentHostPage == 0 || currentHostPage == 1) && !isCreditActive && (millis() - lastClockRefresh >= 1000)) {
    lastClockRefresh = millis();
    if (isScreensaverActive) renderScreensaver(false);
    else renderHostPage(false);
  }

  if (millis() - lastMonitorCheck > 2500) {
    lastMonitorCheck = millis();
    for (int i = 0; i < 4; i++) {
      if (stationNodes[i].isOnline && (millis() - stationNodes[i].lastSeen > STATION_OFFLINE_TIMEOUT)) {
        stationNodes[i].isOnline = false;
      }
    }
    if (isHostScreenOn && !isLiveScanDisplaying && !isScreensaverActive && !isCreditActive && currentHostPage == 1) {
      renderHostPage(false);
    }
  }

  delay(2);
}