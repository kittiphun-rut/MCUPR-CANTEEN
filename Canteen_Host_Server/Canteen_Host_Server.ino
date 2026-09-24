/**
 * @file      Canteen_Host_Server.ino
 * @brief     เครื่องแม่ข่ายของระบบสวัสดิการอาหารกลางวัน 35 บาท/คน/วัน
 * @version   113.10.0
 * @date      2026-09-23
 * @author    Kittiphan Rattanakorn <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * มหาวิทยาลัยมหาจุฬาลงกรณราชวิทยาลัย วิทยาเขตแพร่
 *
 * @par Description
 * ถือฐานข้อมูลนิสิตทั้งหมด ตัดสินสิทธิ์ทุกครั้งที่มีคนแตะบัตรที่จุดบริการ
 * บันทึกประวัติลง LittleFS และเปิดเว็บให้เจ้าหน้าที่จัดการข้อมูล
 * คุยกับจุดบริการทั้งสี่จุดผ่าน ESP-NOW ช่อง 1
 *
 * ไฟล์นี้เก็บเฉพาะตรรกะระบบ ส่วนที่วาดลงจอและหน้าเว็บถูกแยกออกไปเป็น
 * HostScreen.h กับ WebDashboard.h ซึ่ง #include ไว้ท้ายไฟล์ก่อน setup()
 *
 * @par Hardware
 * ESP32-S3 DevKitC-1 (N16R8) · จอ ST7789V 2.8" 320x240 บนบัส FSPI ·
 * นาฬิกา DS3231 บน I2C · บัซเซอร์ GPIO 6 · ปุ่มกด GPIO 2 · WS2812 GPIO 48
 *
 * @par Dependencies
 * Adafruit GFX · Adafruit ST7735/ST7789 · ArduinoJson v6 · RTClib ·
 * Arduino ESP32 core 2.x หรือ 3.x
 *
 * @par Build Settings (Arduino IDE)
 * Board: ESP32S3 Dev Module · Flash: 16MB · PSRAM: OPI ·
 * Partition: Huge APP (3MB No OTA/1MB SPIFFS)
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 113.10.0 | 2026-09-24 | จัดขาใหม่ทั้งบอร์ดให้สายไม่ไขว้กัน อุปกรณ์ทุกตัวอยู่บนแถวซ้ายแถวเดียว ย้ายบัซเซอร์ไป GPIO17 ปุ่มกดไป GPIO18 วัดแบตเตอรี่ไป GPIO8 และไฟหน้าจอไป GPIO9 |
 * | 113.9.0 | 2026-09-23 | ย้ำคำสั่งการแสดงผลหกรอบกระจายออกไปราวสองวินาทีโดยไม่หน่วงลูป แทนการย้ำสามรอบติดกันด้วย delay(30) หน้าเว็บจึงตอบกลับทันที |
 * | 113.8.0 | 2026-09-23 | ตามการเปลี่ยนคำของ HostScreen.h และ WebDashboard.h ตรรกะในไฟล์นี้ไม่เปลี่ยน |
 * | 113.7.0 | 2026-09-23 | คำสั่งเปลี่ยนโหมดการแสดงผลไปถึงจุดบริการทันที ส่งยูนิแคสต์ถึงทุกจุดก่อนแล้วค่อยกระจายเสียง และแก้ broadcastStationTheme() ที่ประกอบแพ็กเก็ตเองจนฟิลด์ screenOn, screensaver, modeSeq เป็นศูนย์ติดไปทุกครั้ง |
 * | 113.6.0 | 2026-09-23 | เพิ่มจอสาธารณะสำหรับทีวีในโรงอาหาร /display กับ /api/board เปิดดูได้โดยไม่ต้องเข้าสู่ระบบ ปิดบังรหัสนิสิตเหลือห้าหลักแรก ไม่มีชื่อ ไม่มีหมายเลขบัตร ไม่มีเลขอ้างอิง และไม่มีข้อมูลฮาร์ดแวร์ |
 * | 113.5.0 | 2026-09-23 | ตามการจัดตำแหน่งตัวอักษรของ HostScreen.h ตรรกะในไฟล์นี้ไม่เปลี่ยน |
 * | 113.4.0 | 2026-09-23 | เพิ่มชื่อร้านสำหรับขึ้นจอ (ภาษาอังกฤษ) แยกจากชื่อจริงที่ใช้บนเว็บ |
 * | 113.3.0 | 2026-09-22 | ตามการแก้เรื่องกะพริบของ HostScreen.h ตรรกะในไฟล์นี้ไม่เปลี่ยน |
 * | 113.2.0 | 2026-09-22 | รีเฟรชหน้าจอทุกหน้า ไม่ใช่แค่สองหน้าแรก หน้า SYSTEM จึงไม่ค้างอีกต่อไป |
 * | 113.1.0 | 2026-09-22 | หน้าเว็บเป็นสองภาษา ไทย/อังกฤษ และเสิร์ฟสคริปต์พร้อม charset |
 * | 113.0.0 | 2026-09-22 | เริ่มใหม่จากต้นฉบับ แยกไฟล์จอและเว็บออกมา คุมการแสดงผลของทุกจุดบริการ เพิ่ม /api/dashboard เปลี่ยนเส้นทางที่แก้ข้อมูลเป็น POST ซ่อม CSV โทเคนเซสชัน และเลขอ้างอิงบนจอ |
 * | 107.0.1 | 2026-09-21 | ต้นฉบับที่ใช้เป็นจุดเริ่ม เก็บสำเนาไว้ที่ original/ |
 *
 * @warning  #include ของ HostScreen.h และ WebDashboard.h ต้องอยู่ท้ายไฟล์ก่อน setup()
 *           เพราะโค้ดข้างในอ้างถึงตัวแปรส่วนกลางข้างบน ย้ายขึ้นไปบนสุดแล้วคอมไพล์ไม่ผ่าน
 * @warning  Partition Scheme ต้องลงท้ายด้วย SPIFFS เท่านั้น ถ้าเลือกแบบ FATFS
 *           LittleFS จะเมานต์ไม่ขึ้นและฐานข้อมูลนิสิตทั้งหมดหายไป
 * @note     รหัสผ่านเจ้าหน้าที่เก็บเป็นข้อความธรรมดาใน /admins.json
 *           เครื่องนี้จึงควรอยู่ในพื้นที่ควบคุม
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

#define APP_VERSION         "113.10.0"
#define DEV_NAME            "Kittiphan Rattanakorn"
#define DEV_ROLE            "Computer Technical Officer"
#define DEV_INSTITUTION     "MCU Phrae Campus"

// ============================================================================
// การจัดขา (Pin Map) — ESP32-S3 DevKitC-1 N16R8
// ============================================================================
// [113.10.0] แก้: จัดขาใหม่ทั้งหมดให้สายไม่ไขว้กัน
//           อุปกรณ์ภายนอก **ทุกตัวอยู่บนแถวซ้ายของบอร์ดแถวเดียว** ไม่มีอะไรข้ามไปฝั่งขวา
//           และขาของแต่ละโมดูลเรียงติดกันตามลำดับขาบนตัวโมดูลเอง
//           รายละเอียดและผังเต็มอยู่ใน docs/HARDWARE.md
//
//   ตำแหน่ง   ขา        เครื่องแม่ข่าย
//   --------  --------  ----------------------------------------------------
//    1, 2     3V3       ไฟเลี้ยง 3.3V ของทุกโมดูล
//    3        RST       ปุ่มรีเซ็ตของบอร์ด ห้ามต่ออะไร
//    4        GPIO4     DS3231  SDA
//    5        GPIO5     DS3231  SCL
//    6-9      GPIO6,7,15,16   ว่าง
//   10        GPIO17    บัซเซอร์
//   11        GPIO18    ปุ่มกด (INPUT_PULLUP)
//   12        GPIO8     วัดแรงดันแบตเตอรี่ (ADC1_CH7)
//   13,14     GPIO3,46  ไม่ใช้ เป็นขา strapping
//   15        GPIO9     TFT  BLK
//   16        GPIO10    TFT  CS
//   17        GPIO11    TFT  DC
//   18        GPIO12    TFT  RES
//   19        GPIO13    TFT  SDA (MOSI)
//   20        GPIO14    TFT  SCL (SCLK)
//   21        5V0       ไม่ได้ใช้
//   22        GND       กราวด์ร่วมของทุกโมดูล
//
// แถวขวาไม่ได้ต่อสายอะไรเลย เหลือไว้ทั้งแถว
// ขาที่ห้ามใช้บนรุ่น N16R8: GPIO26-37 (แฟลชกับ PSRAM แบบ OPI),
// GPIO19/20 (USB), GPIO43/44 (UART0), GPIO0/3/45/46 (strapping)

// จอ ST7789 — หกขาเรียงติดกันที่ตำแหน่ง 15 ถึง 20
// เรียงตรงกับลำดับขาบนตัวโมดูลพอดี อ่านจากล่างขึ้นบน
// GND, VCC, SCL, SDA, RES, DC, CS, BLK จึงเสียบตรงลงมาได้โดยไม่ไขว้เลย
#define TFT_BLK             9
#define TFT_CS              10
#define TFT_DC              11
#define TFT_RST             12
#define TFT_MOSI            13
#define TFT_SCLK            14

// DS3231 บน I2C — สองขาติดกันที่ตำแหน่ง 4 และ 5 ใกล้ขา 3V3 ด้านบน
#define I2C_SDA_PIN         4
#define I2C_SCL_PIN         5

// อุปกรณ์สายเดี่ยว — สามขาติดกันที่ตำแหน่ง 10 ถึง 12
// ชุดนี้ใช้ขาเดียวกับฝั่งจุดบริการเป๊ะ สายชุดเดียวใช้ได้ทั้งสองบอร์ด
#define BUZZER_PIN          17
#define BTN_PIN             18
#define BATTERY_ADC_PIN     8   // ADC1_CH7 อ่านได้ขณะเปิด Wi-Fi

#define RGB_LED_PIN         48  // WS2812 บนบอร์ด ไม่ต้องเดินสาย

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

enum MsgType : uint8_t { MSG_HEARTBEAT = 1, MSG_SCAN_REQ = 2, MSG_SCAN_RESP = 3, MSG_CONFIG = 4 };

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

// คำสั่งการแสดงผลที่แม่ข่ายสั่งลงไปยังสถานี — แม่ข่ายเป็นผู้กำหนดฝ่ายเดียว
//   darkMode    โหมดมืด/สว่าง บังคับเสมอ
//   screenOn    เปิด/ปิดไฟหน้าจอของสถานี
//   screensaver สั่งเข้า/ออกโหมดพักหน้าจอ
//   modeSeq     เพิ่มขึ้นทุกครั้งที่ค่าเปลี่ยน สถานีใช้กันการทำคำสั่งเดิมซ้ำ
typedef struct __attribute__((packed)) {
  uint8_t magic;
  uint8_t version;
  uint8_t msgType;
  uint8_t stationId;   // 0 = ทุกสถานี
  uint8_t darkMode;
  uint8_t screenOn;
  uint8_t screensaver;
  uint8_t modeSeq;
} HostConfigPacket;
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

uint16_t lastScanSeq[4] = {0, 0, 0, 0};
bool hasLastScanSeq[4] = {false, false, false, false};
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

// [113.4.0] เพิ่ม: screen คือชื่อสั้น ๆ ภาษาอังกฤษสำหรับขึ้นบนจอ TFT
//           เพราะฟอนต์ในตัวของจอไม่มีตัวอักษรไทย ชื่อไทยจะกลายเป็นสัญลักษณ์มั่ว
//           ส่วน name ยังเป็นชื่อจริงภาษาไทยที่ใช้บนหน้าเว็บและในไฟล์รายงาน
struct Shop {
  String name;
  String vendor;
  String screen;
};

std::vector<Student> db;
// ช่องที่สามคือชื่อสำหรับขึ้นจอ TFT ต้องเป็นภาษาอังกฤษเท่านั้น
// ค่าเริ่มต้นเดิมมีแต่ชื่อไทย พอขึ้นจอจึงกลายเป็นสัญลักษณ์มั่วตั้งแต่ยังไม่ได้ตั้งค่าอะไร
Shop shops[4] = {
  {"ร้านที่ 1", "นางสาวณัฐกฤตา สุพิทิพย์", "Shop 1"},
  {"ร้านที่ 2", "นางสาวปียาวัน เหมืองหม้อ", "Shop 2"},
  {"ร้านที่ 3", "นางฉวีวรรณ วงศ์นาม",       "Shop 3"},
  {"ร้านที่ 4", "น.ส.พัชรินทร์ ชำนาญใช้",   "Shop 4"}
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
String getLoginHTML(bool hasError = false);
String getHTML();
void sendAlert(String message, String redirectUrl = "/");
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
String generateSessionToken();
String generateRefNo(int stationId);
void processScanRequest(const uint8_t* mac, StationPacket pkt, int rssi);
float calculateCpuLoad();
float getChipTemperature();
float readHostBatteryVoltage();
int getHostBatteryPercentage(float voltage);
void drawBentoCard(int x, int y, int w, int h, uint16_t borderColor, uint16_t bgColor);
void drawBentoPillBadge(int x, int y, int w, int h, const char* text, uint16_t fgColor, uint16_t bgColor);
void drawHostBatteryHUD(int x, int y);
void drawMiniBattery(int x, int y, int pct);
void drawHostTopBar(String title, int pageNo = 0);
void drawBentoBottomBar(String instruction);
String maskUID(String uid);
uint16_t getStationServedCount(uint8_t stationId);
String getRealTimeStr();
String getDateFormattedStr();
String getTimeOnlyStr();
bool isWithinServiceTime();
bool ensureStationPeer(uint8_t stationId);
bool sendToStation(uint8_t stationId, const uint8_t *data, size_t len);
void sendStationTheme(uint8_t stationId);
// ต้องประกาศตรงนี้ (หลังนิยาม HostConfigPacket) ไม่งั้น Arduino IDE จะสร้าง
// ต้นแบบฟังก์ชันให้เองแล้ววางไว้เหนือจุดที่นิยามโครงสร้าง คอมไพล์ไม่ผ่านทันที
void fillStationConfig(HostConfigPacket &cfg, uint8_t stationId);
void broadcastStationTheme();
void announceDisplayMode();
String jsonEscape(const String &raw);
void handleDashboardAPI();
void handlePublicBoard();
void handleSetDisplayMode();
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
String csvField(const String &raw);
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
void pushPublicFeed(const String &studentId, const String &claimTime, int shop);
void showBootNetworkStatus();
void handleHostButton();

struct ScanQueueItem {
  uint8_t mac[6];
  StationPacket pkt;
  int rssi;
};
QueueHandle_t scanQueue = NULL;

File fsUploadFile;
String lastScannedUID       = "-";
String lastScannedStudentId = "-";
int lastScannedStation      = 0;
String lastScannedStatus    = "READY";
// [113.6.0] เพิ่ม: รายการล่าสุดสำหรับจอสาธารณะบนทีวี
// เก็บแค่แปดรายการล่าสุดแบบวงแหวน ไม่ต้องไปไล่เรียงฐานข้อมูลทุกครั้งที่ทีวีขอข้อมูล
// **เก็บเฉพาะรหัสที่ปิดบังแล้ว** ไม่เก็บชื่อ ไม่เก็บหมายเลขบัตร ไม่เก็บเลขอ้างอิง
// ข้อมูลที่ไม่ได้เก็บ ย่อมหลุดออกไปทางหน้าเว็บสาธารณะไม่ได้
#define PUBLIC_FEED_SIZE 8
struct PublicFeedItem {
  char maskedId[16];   // ห้าหลักแรกของรหัสนิสิต ที่เหลือเป็นดอกจัน
  char atTime[12];     // เวลาที่รับสิทธิ์ รูปแบบ HH:MM:SS
  uint8_t shop;        // ร้านที่ 1 ถึง 4
};
PublicFeedItem publicFeed[PUBLIC_FEED_SIZE] = {};
int publicFeedCount = 0;   // จำนวนรายการที่มีจริง สูงสุดเท่าขนาดวงแหวน
int publicFeedHead  = 0;   // ตำแหน่งที่จะเขียนรายการถัดไป

bool hostApReady            = false; // ปล่อยสัญญาณ Wi-Fi สำเร็จหรือไม่ ใช้บอกบนจอตอนบูต
// [113.0.0] แก้: เก็บเลขอ้างอิงของการสแกนไว้ ของเดิมสร้างใหม่ทุกครั้งที่วาดจอ
//           ทำให้ตัวนับเดินเรื่อย ๆ และเลขบนจอไม่ตรงกับที่บันทึกไว้
String lastScannedRef       = "-";   // เลขอ้างอิงของการสแกนครั้งล่าสุด
                                     // เก็บไว้ ไม่สร้างใหม่ตอนวาดจอ ไม่งั้นเลขจะเดินทุกครั้งที่รีเฟรช

uint32_t transactionCounter = 0;

int currentHostPage         = 0;
const int TOTAL_PAGES       = 3;
bool isScreensaverActive    = false;
bool stationScreenOn        = true;   // แม่ข่ายสั่งให้ไฟจอของสถานีเปิดอยู่หรือไม่
bool stationScreensaver     = false;  // แม่ข่ายสั่งให้สถานีพักหน้าจออยู่หรือไม่
uint8_t hostModeSeq         = 0;      // นับทุกครั้งที่คำสั่งการแสดงผลเปลี่ยน

// [113.9.0] เพิ่ม: ย้ำคำสั่งการแสดงผลหลายรอบแบบไม่หน่วงลูป
//           ของเดิมย้ำสามรอบติดกันด้วย delay(30) รวมแล้วกินเวลาแค่ 90 มิลลิวินาที
//           ถ้าจุดบริการพลาดช่วงนั้นไปก็ต้องรอ heartbeat รอบถัดไปเป็นวินาที
//           ตอนนี้กระจายการย้ำออกไปราวสองวินาที โดยให้ loop() เป็นคนส่ง
//           หน้าเว็บจึงตอบกลับทันทีโดยไม่ต้องค้างรอ และจอของแม่ข่ายก็ไม่สะดุด
uint8_t  displayAnnounceLeft = 0;
unsigned long displayAnnounceNext = 0;
const uint8_t      DISPLAY_ANNOUNCE_ROUNDS = 6;
const unsigned long DISPLAY_ANNOUNCE_GAP   = 300;
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

float readHostBatteryVoltage() {
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(BATTERY_ADC_PIN);
    delay(2);
  }
  float avgRaw = sum / 8.0f;
  float pinVoltage = (avgRaw / 4095.0f) * 3.3f;
  return pinVoltage * 2.0f;
}

int getHostBatteryPercentage(float voltage) {
  if (voltage >= 4.15f) return 100;
  if (voltage <= 3.30f) return 0;
  int percent = (int)((voltage - 3.30f) / (4.15f - 3.30f) * 100.0f);
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  return percent;
}

// (ย้ายไปอยู่ใน HostScreen.h ซึ่ง #include ไว้ท้ายไฟล์ก่อน setup())

String maskUID(String uid) {
  if (uid.length() < 4 || uid == "-") return uid;
  return uid.substring(0, uid.length() - 4) + "****";
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
    return true;
  }
  stationPeerReady[stationId - 1] = false;
  return false;
}

bool sendToStation(uint8_t stationId, const uint8_t *data, size_t len) {
  if (stationId >= 1 && stationId <= 4 && ensureStationPeer(stationId)) {
    esp_err_t err = esp_now_send(stationNodes[stationId - 1].mac, data, len);
    if (err == ESP_OK) return true;
  }
  return esp_now_send(broadcastAddress, data, len) == ESP_OK;
}

// ประกอบคำสั่งการแสดงผลชุดเดียวใช้ร่วมกันทุกที่ จะได้ไม่มีทางส่งค่าไม่ตรงกัน
// [113.0.0] เพิ่ม: เตรียมแพ็กเก็ตสั่งโหมดการแสดงผลไปยังจุดบริการ
void fillStationConfig(HostConfigPacket &cfg, uint8_t stationId) {
  cfg.magic = ESPNOW_PROTO_MAGIC;
  cfg.version = ESPNOW_PROTO_VER;
  cfg.msgType = MSG_CONFIG;
  cfg.stationId = stationId;
  cfg.darkMode = isTftDarkMode ? 1 : 0;
  cfg.screenOn = stationScreenOn ? 1 : 0;
  cfg.screensaver = stationScreensaver ? 1 : 0;
  cfg.modeSeq = hostModeSeq;
}

void sendStationTheme(uint8_t stationId) {
  HostConfigPacket cfg = {};
  fillStationConfig(cfg, stationId);
  sendToStation(stationId, (uint8_t *)&cfg, sizeof(cfg));
}

// เรียกทุกครั้งที่โหมดการแสดงผลเปลี่ยน ส่งสามรอบห่างกันเล็กน้อยเพราะ ESP-NOW
// ไม่มีการยืนยันการรับในชั้น broadcast ถ้ารอบแรกหายไปยังมีรอบสองและสามตามไป
// [113.0.0] เพิ่ม: ประกาศโหมดการแสดงผลใหม่ให้ทุกจุดบริการพร้อมกัน
//
// [113.7.0] แก้: ของเดิมส่งแต่ broadcast อย่างเดียว คำสั่งจึงไปถึงช้าเป็นหลักวินาที
//           เพิ่ม **ยูนิแคสต์ถึงทุกจุดบริการที่แม่ข่ายรู้จัก MAC แล้ว** ลงไปก่อน
//           ชั้นยูนิแคสต์ของ ESP-NOW มีการตอบรับและส่งซ้ำให้ในตัว จึงเชื่อถือได้กว่ามาก
//           และเป็นเส้นทางเดียวกับที่ใช้ตอบ heartbeat ซึ่งพิสูจน์แล้วว่าถึงแน่นอน
//           ส่วน broadcast ยังส่งต่อไว้เผื่อจุดบริการที่ยังไม่เคยส่งอะไรมา
//           แม่ข่ายจึงยังไม่รู้ MAC ของมัน
// ส่งคำสั่งหนึ่งรอบ ยูนิแคสต์ถึงทุกจุดบริการแล้วตามด้วยการกระจายเสียง
void sendDisplayConfigRound() {
  for (uint8_t id = 1; id <= 4; id++) sendStationTheme(id);
  broadcastStationTheme();
}

void announceDisplayMode() {
  hostModeSeq++;
  sendDisplayConfigRound();                       // รอบแรกส่งทันที
  displayAnnounceLeft = DISPLAY_ANNOUNCE_ROUNDS - 1;
  displayAnnounceNext = millis() + DISPLAY_ANNOUNCE_GAP;
}

// [113.7.0] แก้: ของเดิมประกอบแพ็กเก็ตเองทีละฟิลด์ แล้วตั้งแต่ darkMode ลงไปก็หยุด
//           screenOn, screensaver และ modeSeq จึงเป็นศูนย์ติดไปทุกครั้ง
//           ผลคือคำสั่งพักหน้าจอไม่เคยเดินทางมากับ broadcast เลย
//           เพราะจุดบริการกันคำสั่งซ้ำด้วย modeSeq ที่ค้างอยู่ที่ศูนย์
//           ต้องรอให้ heartbeat รอบถัดไปดึงค่าจริงมาให้ จึงช้าไปหลายวินาที
//           ตอนนี้เรียก fillStationConfig() เหมือนทุกที่ ค่าครบและตรงกันเสมอ
void broadcastStationTheme() {
  HostConfigPacket cfg = {};
  fillStationConfig(cfg, 0);   // 0 = ถึงทุกจุดบริการ
  esp_now_send(broadcastAddress, (uint8_t *)&cfg, sizeof(cfg));
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

void sendAlert(String message, String redirectUrl) {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'></head><body>";
  html += "<script>";
  html += "alert('" + message + "');";
  html += "window.location.href = '" + redirectUrl + "';";
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

// [113.6.0] เพิ่ม: บันทึกหนึ่งรายการลงวงแหวนของจอสาธารณะ
// ปิดบังรหัสนิสิตตั้งแต่ตอนเก็บ ไม่ใช่ตอนแสดงผล จะได้ไม่มีทางลืมปิดบัง
void pushPublicFeed(const String &studentId, const String &claimTime, int shop) {
  PublicFeedItem item = {};

  String masked = studentId;
  if (masked.length() > 5) masked = masked.substring(0, 5) + "*****";
  strncpy(item.maskedId, masked.c_str(), sizeof(item.maskedId) - 1);

  // claimTime เก็บเป็น "วว/ดด/ปปปป ชช:นน:วว" เอาเฉพาะส่วนเวลามาแสดง
  int sp = claimTime.lastIndexOf(' ');
  String t = (sp >= 0) ? claimTime.substring(sp + 1) : claimTime;
  strncpy(item.atTime, t.c_str(), sizeof(item.atTime) - 1);

  item.shop = (shop >= 1 && shop <= 4) ? (uint8_t)shop : 0;

  publicFeed[publicFeedHead] = item;
  publicFeedHead = (publicFeedHead + 1) % PUBLIC_FEED_SIZE;
  if (publicFeedCount < PUBLIC_FEED_SIZE) publicFeedCount++;
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

// [113.0.0] แก้: ใช้ตัวสุ่มในตัวชิปแทน random() ของ Arduino ที่ให้ลำดับเดิม
//           ทุกครั้งที่เปิดเครื่อง ซึ่งแปลว่าโทเคนของวันนี้เดาได้จากของเมื่อวาน
String generateSessionToken() {
  // esp_random() เป็นตัวสุ่มในตัวชิป ไม่ใช่ random() ของ Arduino ที่ให้ลำดับเดิม
  // ทุกครั้งที่เปิดเครื่อง ซึ่งแปลว่าโทเคนของวันนี้เดาได้จากของเมื่อวาน
  String token = "";
  token.reserve(32);
  const char chars[] = "abcdef0123456789";
  for (int i = 0; i < 32; i++) token += chars[esp_random() & 0x0F];
  return token;
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
      if (millis() < activeSessions[i].expiry) {
        activeSessions[i].expiry = millis() + 7200000;
        return true;
      } else {
        activeSessions[i].token = "";
      }
    }
  }
  return false;
}

void redirectToLogin() {
  server.sendHeader("Location", "/login", true);
  server.send(302, "text/plain", "");
}

// (ย้ายไปอยู่ใน WebDashboard.h ซึ่ง #include ไว้ท้ายไฟล์ก่อน setup())

// (ย้ายไปอยู่ใน HostScreen.h ซึ่ง #include ไว้ท้ายไฟล์ก่อน setup())

// ============================================================================
// BENTO TFT SCREENS (320x240 Modular High-Density Display)
// ============================================================================
// (ย้ายไปอยู่ใน HostScreen.h ซึ่ง #include ไว้ท้ายไฟล์ก่อน setup())

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
      clickCount = 0;
      // ปลุกสถานีทุกเครื่องให้ออกจากโหมดพักหน้าจอพร้อมกัน
      if (wasSaver) { stationScreensaver = false; stationScreenOn = true; announceDisplayMode(); }
      return;
    }

    if (pressDuration >= 5000) {
      isCreditActive = true; 
      isScreensaverActive = false;
      soundCreditJingle(); 
      renderDeveloperCredit(); 
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
      // สั่งให้สถานีทุกเครื่องพักหน้าจอตามไปด้วย
      stationScreensaver = true;
      announceDisplayMode();
    }
    else if (clickCount >= 3) {
      isTftDarkMode = !isTftDarkMode;
      preferences.begin("sys_cfg", false);
      preferences.putBool("tft_dark", isTftDarkMode);
      preferences.end();
      soundThemeSwitch();
      renderHostPage(true);
      announceDisplayMode();
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
    if (stId >= 1 && stId <= 4) hbAckPending[stId - 1] = true;
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
  isScreensaverActive = false;
  // มีนิสิตมาแตะบัตรแล้ว ปลุกทุกสถานีออกจากโหมดพักหน้าจอพร้อมกัน
  if (wasScreensaver || stationScreensaver) {
    stationScreensaver = false;
    stationScreenOn = true;
    announceDisplayMode();
  }
  isCreditActive = false;

  String uid = String(pkt.uid);
  uid.trim();

  lastScannedUID = uid;
  lastScannedStation = pkt.stationId;

  if (pkt.stationId >= 1 && pkt.stationId <= 4 &&
      pkt.seq != 0 &&
      hasLastScanSeq[pkt.stationId - 1] &&
      lastScanSeq[pkt.stationId - 1] == pkt.seq) {
    sendToStation(pkt.stationId,
                  (uint8_t *)&lastScanResponse[pkt.stationId - 1],
                  sizeof(HostResponsePacket));
    if (wasScreensaver) renderHostPage(true);
    return;
  }

  HostResponsePacket resp = {};
  resp.magic = ESPNOW_PROTO_MAGIC;
  resp.version = ESPNOW_PROTO_VER;
  resp.msgType = MSG_SCAN_RESP;
  resp.stationId = pkt.stationId;
  resp.seq = pkt.seq;
  resp.amount = 35;

  if (!isWithinServiceTime()) {
    strncpy(resp.status, "TIME_CLOSED", sizeof(resp.status) - 1);
    strncpy(resp.studentId, "-", sizeof(resp.studentId) - 1);
    strncpy(resp.name, "SERVICE CLOSED", sizeof(resp.name) - 1);
    strncpy(resp.refNo, "-", sizeof(resp.refNo) - 1);
    strncpy(resp.claimTime, getRealTimeStr().c_str(), sizeof(resp.claimTime) - 1);
    strncpy(resp.message, "OUT OF TIME", sizeof(resp.message) - 1);
    lastScannedStatus = "TIME_CLOSED";
    lastScannedRef = "-";
    resp.servedCount = getStationServedCount(pkt.stationId);
    if (pkt.stationId >= 1 && pkt.stationId <= 4 && pkt.seq != 0) {
      lastScanSeq[pkt.stationId - 1] = pkt.seq;
      hasLastScanSeq[pkt.stationId - 1] = true;
      memcpy(&lastScanResponse[pkt.stationId - 1], &resp, sizeof(resp));
    }
    sendToStation(pkt.stationId, (uint8_t *)&resp, sizeof(HostResponsePacket));
    displayHostLiveScan(lastScannedUID, "-", lastScannedStatus, pkt.stationId);
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
        String safeName = "Student " + s.studentId;
        strncpy(resp.name, safeName.c_str(), sizeof(resp.name) - 1);
      }

      if (s.claimed) {
        strncpy(resp.status, "ALREADY_USED", sizeof(resp.status) - 1);
        strncpy(resp.refNo, s.refNo.c_str(), sizeof(resp.refNo) - 1);
        strncpy(resp.claimTime, s.claimTime.c_str(), sizeof(resp.claimTime) - 1);
        String msg = "SHOP 0" + String(s.station);
        strncpy(resp.message, msg.c_str(), sizeof(resp.message) - 1);
        lastScannedStatus = "DUPLICATE";
        lastScannedRef = s.refNo;
      } else {
        String currentTimestamp = getRealTimeStr();
        String currentRefNo = generateRefNo(pkt.stationId);

        s.claimed = true;
        s.station = pkt.stationId;
        s.claimTime = currentTimestamp;
        s.refNo = currentRefNo;
        lastScannedStatus = "APPROVED";
        lastScannedRef = currentRefNo;
        pushPublicFeed(s.studentId, currentTimestamp, pkt.stationId);

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
    lastScannedRef = "-";
  }

  resp.servedCount = getStationServedCount(pkt.stationId);
  if (pkt.stationId >= 1 && pkt.stationId <= 4 && pkt.seq != 0) {
    lastScanSeq[pkt.stationId - 1] = pkt.seq;
    hasLastScanSeq[pkt.stationId - 1] = true;
    memcpy(&lastScanResponse[pkt.stationId - 1], &resp, sizeof(resp));
  }
  sendToStation(pkt.stationId, (uint8_t *)&resp, sizeof(HostResponsePacket));
  displayHostLiveScan(lastScannedUID, lastScannedStudentId, lastScannedStatus, pkt.stationId);

  if (found && matchedStudent && strcmp(resp.status, "SUCCESS") == 0) {
    String claimType = matchedStudent->isTempCard ? "Temp Card" : "Normal";
    appendLogToFS(matchedStudent->studentId, matchedStudent->fullName, matchedStudent->uid, matchedStudent->refNo, matchedStudent->claimTime, pkt.stationId, claimType);
    
    if (matchedStudent->isTempCard) {
      if (matchedStudent->originalUid != "") {
        matchedStudent->uid = matchedStudent->originalUid;
        matchedStudent->originalUid = "";
      } else {
        matchedStudent->uid = "";
      }
      saveDatabaseToFS();
    }

    soundScanSuccess();
  }
}

// [113.0.0] แก้: ต่อ JSON เองแทนบัฟเฟอร์ขนาดคงที่ 8 กิโล ซึ่งพอดีเกินไป
//           สำหรับยี่สิบรายชื่อ ถ้าล้นขึ้นมา JSON จะขาดกลางคันแบบเงียบ ๆ
void handleGetStudentsAPI() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  int page = server.hasArg("page") ? server.arg("page").toInt() : 1;
  int limit = server.hasArg("limit") ? server.arg("limit").toInt() : 20;
  String search = server.hasArg("search") ? server.arg("search") : "";
  search.trim(); search.toUpperCase();
  if (page < 1) page = 1;
  if (limit < 1 || limit > 100) limit = 20;

  std::vector<int> matchedIndices;
  for (size_t i = 0; i < db.size(); i++) {
    if (search.length() == 0) matchedIndices.push_back(i);
    else {
      String sId = db[i].studentId; sId.toUpperCase();
      String fName = db[i].fullName; fName.toUpperCase();
      String rNo = db[i].refNo; rNo.toUpperCase();
      String uId = db[i].uid; uId.toUpperCase();
      if (sId.indexOf(search) != -1 || fName.indexOf(search) != -1 ||
          rNo.indexOf(search) != -1 || uId.indexOf(search) != -1) matchedIndices.push_back(i);
    }
  }

  int totalItems = matchedIndices.size();
  int totalPages = (totalItems + limit - 1) / limit;
  if (totalPages < 1) totalPages = 1;
  if (page > totalPages) page = totalPages;

  int startIdx = (page - 1) * limit;
  int endIdx = min(startIdx + limit, totalItems);

  // ต่อ JSON เองแทน DynamicJsonDocument ขนาดคงที่ ของเดิมจองไว้ 8 กิโล
  // ซึ่งพอดีเกินไปสำหรับยี่สิบรายชื่อ ถ้าล้นขึ้นมา JSON จะขาดกลางคันแบบเงียบ ๆ
  String j;
  j.reserve(1024 + (endIdx - startIdx) * 160);
  j = "{\"totalItems\":" + String(totalItems);
  j += ",\"totalPages\":" + String(totalPages);
  j += ",\"currentPage\":" + String(page);
  j += ",\"limit\":" + String(limit);
  j += ",\"students\":[";
  for (int i = startIdx; i < endIdx; i++) {
    int idx = matchedIndices[i];
    const Student &st = db[idx];
    if (i > startIdx) j += ",";
    j += "{\"id\":\"" + jsonEscape(st.studentId) + "\"";
    j += ",\"name\":\"" + jsonEscape(st.fullName) + "\"";
    j += ",\"uid\":\"" + jsonEscape(st.uid) + "\"";
    j += ",\"claimed\":" + String(st.claimed ? "true" : "false");
    j += ",\"time\":\"" + jsonEscape(st.claimTime) + "\"";
    j += ",\"ref\":\"" + jsonEscape(st.refNo) + "\"";
    j += ",\"station\":" + String(st.station);
    j += ",\"isTemp\":" + String(st.isTempCard ? "true" : "false");
    j += ",\"shopName\":\"";
    j += jsonEscape((st.station > 0 && st.station <= 4) ? shops[st.station - 1].name : String("-"));
    j += "\"}";
  }
  j += "]}";

  server.send(200, "application/json; charset=utf-8", j);
}

// ครอบค่าด้วยเครื่องหมายคำพูดแบบที่โปรแกรมตารางเข้าใจ
// ชื่อร้านอย่าง Rice "House", Drinks เคยทำให้คอลัมน์ในไฟล์เลื่อนทั้งแถว
// [113.0.0] แก้: ชื่อที่มีเครื่องหมายคำพูดหรือลูกน้ำเคยทำให้คอลัมน์ในไฟล์เลื่อนทั้งแถว
String csvField(const String &raw) {
  String out = "\"";
  for (unsigned int i = 0; i < raw.length(); i++) {
    char c = raw[i];
    if (c == '"') out += "\"\"";
    else if (c == '\r' || c == '\n') out += ' ';
    else out += c;
  }
  out += "\"";
  return out;
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

  String csv = "\xEF\xBB\xBF";
  csv += "MCU Phrae Canteen Daily Claim Report (Claimed Only)\n";
  csv += "Export Date," + getRealTimeStr() + "\n\n";

  csv += "--- Summary Payout By Shop ---\n";
  csv += "No.,Shop Name,Vendor,Total Orders,Total Payout (THB)\n";
  for (int i = 0; i < 4; i++) {
    csv += String(i + 1) + "," + csvField(shops[i].name) + "," + csvField(shops[i].vendor) + ","
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
    csv += csvField(s.studentId) + ",";
    csv += csvField(s.fullName) + ",";
    csv += csvField(s.uid) + ",";
    csv += csvField(s.refNo) + ",";
    csv += csvField(s.claimTime) + ",";
    csv += csvField(shopName) + ",";
    csv += csvField(vendorName) + ",";
    csv += "35,";
    csv += csvField(cardType) + "\n";
  }

  server.sendHeader("Content-Disposition", "attachment; filename=MCU_Canteen_Claim_Report.csv");
  server.send(200, "text/csv; charset=utf-8", csv);
}

void handleSetRTCTime() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  if (server.hasArg("date") && server.hasArg("time")) {
    String dStr = server.arg("date");
    String tStr = server.arg("time");
    int y, m, d, h, mi, s;
    if (sscanf(dStr.c_str(), "%d-%d-%d", &y, &m, &d) == 3 &&
        sscanf(tStr.c_str(), "%d:%d:%d", &h, &mi, &s) == 3) {
      rtc.adjust(DateTime(y, m, d, h, mi, s));
      sendAlert("ตั้งค่านาฬิกา RTC DS3231 สำเร็จเรียบร้อย!", "/");
      return;
    }
  }
  sendAlert("รูปแบบวันที่หรือเวลาไม่ถูกต้อง", "/");
}

void handleDownloadArchive() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  if (server.hasArg("file")) {
    String filename = server.arg("file");
    if (!filename.startsWith("/")) filename = "/" + filename;
    // รับเฉพาะชื่อไฟล์ประวัติในรากเท่านั้น กันชื่อแบบ ../ ที่ไต่ไปอ่านไฟล์อื่น
    if (filename.startsWith("/arc_") && filename.indexOf("/", 1) < 0 &&
        filename.indexOf("..") < 0 && LittleFS.exists(filename)) {
      File f = LittleFS.open(filename, "r");
      server.streamFile(f, "text/csv");
      f.close();
      return;
    }
  }
  server.send(404, "text/plain", "Archive Not Found");
}

void handleDeleteArchive() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  if (server.hasArg("file")) {
    String filename = server.arg("file");
    if (!filename.startsWith("/")) filename = "/" + filename;
    
    if (filename.startsWith("/arc_") && filename.indexOf("/", 1) < 0 &&
        filename.indexOf("..") < 0 && LittleFS.exists(filename)) {
      LittleFS.remove(filename);
      sendAlert("ลบไฟล์ประวัติ " + filename.substring(1) + " เรียบร้อยแล้ว!", "/");
      return;
    }
  }
  sendAlert("ไม่สามารถลบไฟล์ได้", "/");
}

void handleSaveStudent() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  String oldId = server.arg("oldStudentId"); String sId = server.arg("studentId");
  String name = server.arg("fullName"); String uid = server.arg("uid");
  sId.trim(); name.trim(); uid.trim();
  if (oldId != "") {
    for (auto& s : db) { if (s.studentId == oldId) { s.studentId = sId; s.fullName = name; s.uid = uid; break; } }
  } else {
    Student s; s.studentId = sId; s.fullName = name; s.uid = uid; s.originalUid = ""; s.claimed = false; s.claimTime = "-"; s.refNo = "-"; s.station = 0; s.isTempCard = false;
    db.push_back(s);
  }
  saveDatabaseToFS(); renderHostPage(true); sendAlert("Beneficiary Saved Successfully!", "/");
}

void handleSaveTempCard() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  String sId = server.arg("studentId"); String uid = server.arg("uid");
  sId.trim(); uid.trim(); bool found = false;
  for (auto& s : db) {
    if (s.studentId == sId) {
      if (s.originalUid == "") s.originalUid = s.uid;
      s.uid = uid;
      s.isTempCard = true;
      found = true;
      break;
    }
  }
  if (!found) { sendAlert("Student ID Not Found!", "/"); return; }
  saveDatabaseToFS(); renderHostPage(true); sendAlert("Temporary Card Assigned Successfully!", "/");
}

void handleRemoveTempCard() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  String id = server.arg("id");
  for (auto& s : db) {
    if (s.studentId == id) {
      if (s.originalUid != "") s.uid = s.originalUid;
      s.originalUid = "";
      s.isTempCard = false;
      break;
    }
  }
  saveDatabaseToFS(); renderHostPage(true); sendAlert("Temporary Card Revoked Successfully!", "/");
}

void handleDeleteStudent() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  String id = server.arg("id");
  for (auto it = db.begin(); it != db.end(); ++it) { if (it->studentId == id) { db.erase(it); break; } }
  saveDatabaseToFS(); renderHostPage(true); sendAlert("Beneficiary Removed Successfully!", "/");
}

void handleSaveAdmin() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  String oldUser = server.arg("oldUsername");
  String user = server.arg("username");
  String pass = server.arg("password");
  String dName = server.arg("displayName");
  user.trim(); pass.trim(); dName.trim();

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
      sendAlert("ระบบจำกัดเจ้าหน้าที่ไม่เกิน 3 ท่าน!", "/");
      return;
    }
    for (const auto& u : adminUsers) {
      if (u.username == user) {
        sendAlert("ชื่อผู้ใช้นี้มีในระบบแล้ว!", "/");
        return;
      }
    }
    AdminUser nu;
    nu.username = user;
    nu.password = pass;
    nu.displayName = dName;
    adminUsers.push_back(nu);
  }
  saveAdminsToFS();
  sendAlert("บันทึกข้อมูลเจ้าหน้าที่เรียบร้อยแล้ว!", "/");
}

void handleDeleteAdmin() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  if (adminUsers.size() <= 1) {
    sendAlert("ไม่สามารถลบได้ ต้องมีเจ้าหน้าที่อย่างน้อย 1 ท่านในระบบ!", "/");
    return;
  }
  String user = server.arg("user");
  for (auto it = adminUsers.begin(); it != adminUsers.end(); ++it) {
    if (it->username == user) {
      adminUsers.erase(it);
      break;
    }
  }
  saveAdminsToFS();
  sendAlert("ลบเจ้าหน้าที่เรียบร้อยแล้ว!", "/");
}

void saveShopsToFS() {
  File file = LittleFS.open("/shops.json", "w");
  if (!file) return;
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  for (int i = 0; i < 4; i++) {
    JsonObject obj = array.createNestedObject();
    obj["name"] = shops[i].name; obj["vendor"] = shops[i].vendor;
    obj["screen"] = shops[i].screen;
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
    // ไฟล์ที่บันทึกไว้ก่อนรุ่น 113.4.0 ยังไม่มีช่องนี้ ปล่อยว่างไว้ได้
    // ฝั่งจอจะถอยไปใช้ชื่อร้านที่เป็น ASCII หรือ "Shop N" ให้เอง
    if (array[i]["screen"].is<const char*>()) shops[i].screen = array[i]["screen"].as<String>();
  }
  file.close();
}

void appendLogToFS(String studentId, String fullName, String uid, String refNo, String timestamp, int station, String type) {
  File logFile = LittleFS.open("/daily_log.csv", FILE_APPEND);
  if (logFile) {
    logFile.printf("%s,%s,%s,%s,%s,%d,35,%s\n", studentId.c_str(), fullName.c_str(), uid.c_str(), refNo.c_str(), timestamp.c_str(), station, type.c_str());
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
    int c1 = line.indexOf(','); int c2 = line.indexOf(',', c1 + 1);
    int c3 = line.indexOf(',', c2 + 1); int c4 = line.indexOf(',', c3 + 1);
    int c5 = line.indexOf(',', c4 + 1); int c6 = line.indexOf(',', c5 + 1);
    int c7 = line.indexOf(',', c6 + 1);
    if (c1 != -1 && c2 != -1 && c3 != -1 && c4 != -1 && c5 != -1 && c6 != -1) {
      String sId = line.substring(0, c1);
      String ref = line.substring(c3 + 1, c4);
      String tStamp = line.substring(c4 + 1, c5);
      int st = line.substring(c5 + 1, c6).toInt();
      String type = (c7 != -1) ? line.substring(c7 + 1) : "Normal";
      for (auto& s : db) {
        if (s.studentId == sId) {
          s.claimed = true; s.refNo = ref; s.claimTime = tStamp; s.station = st;
          if (type == "Temp Card") s.isTempCard = true;
          break;
        }
      }
    }
  }
  logFile.close();
}

void saveDatabaseToFS() {
  File file = LittleFS.open("/students.csv", "w");
  if (!file) return;
  file.println("studentId,fullName,uid");
  for (const auto& s : db) {
    String permUid = (s.originalUid != "") ? s.originalUid : s.uid;
    file.printf("%s,%s,%s\n", s.studentId.c_str(), s.fullName.c_str(), permUid.c_str());
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
    if (isHeader) { isHeader = false; continue; }
    int c1 = line.indexOf(','); int c2 = line.indexOf(',', c1 + 1);
    if (c1 != -1 && c2 != -1) {
      Student s;
      s.studentId = line.substring(0, c1);
      s.fullName = line.substring(c1 + 1, c2);
      s.uid = line.substring(c2 + 1);
      s.uid.trim();
      s.originalUid = "";
      s.claimed = false; s.claimTime = "-"; s.refNo = "-"; s.station = 0; s.isTempCard = false;
      db.push_back(s);
    }
  }
  file.close();
  restoreDailyLogs();
}

void mergeImportedStudents() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  if (!LittleFS.exists("/temp_import.csv")) { sendAlert("No file uploaded!", "/"); return; }
  File file = LittleFS.open("/temp_import.csv", "r");
  if (!file) { sendAlert("Failed to open uploaded file!", "/"); return; }

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

    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    if (c1 != -1) {
      String sId = line.substring(0, c1);
      String fName = (c2 != -1) ? line.substring(c1 + 1, c2) : line.substring(c1 + 1);
      String uId = (c2 != -1) ? line.substring(c2 + 1) : "";
      sId.trim(); fName.trim(); uId.trim();

      if (sId.length() == 0) continue;

      bool exists = false;
      for (auto& s : db) {
        if (s.studentId == sId) {
          if (fName.length() > 0) s.fullName = fName;
          if (uId.length() > 0 && !s.isTempCard) {
            s.uid = uId;
            s.originalUid = "";
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

  String msg = "นำเข้าสำเร็จ! เพิ่มใหม่: " + String(newCount) + " รายการ, ปรับปรุง: " + String(updatedCount) + " รายการ";
  sendAlert(msg, "/");
}

void handleFileUpload() {
  if (!isAuthenticated()) return;
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

// ============================================================================
// BENTO WEB PORTAL HTML
// ============================================================================
// (ย้ายไปอยู่ใน WebDashboard.h ซึ่ง #include ไว้ท้ายไฟล์ก่อน setup())


// ============================================================================
// ข้อมูลสดสำหรับแดชบอร์ด
// หน้าเว็บดึงชุดนี้ทุกสองวินาทีแล้ววาดใหม่เฉพาะตัวเลขที่เปลี่ยน ไม่ต้องรีโหลดทั้งหน้า
// ============================================================================

// กันอักขระที่ทำให้ JSON พัง เช่นชื่อร้านที่มีเครื่องหมายคำพูด
// [113.0.0] เพิ่ม: กันอักขระที่ทำให้ JSON พัง เช่นชื่อร้านที่มีเครื่องหมายคำพูด
String jsonEscape(const String &raw) {
  String out;
  out.reserve(raw.length() + 8);
  for (unsigned int i = 0; i < raw.length(); i++) {
    char c = raw[i];
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if (c == '\n') out += "\\n";
    else if (c == '\r') { }
    else if (c == '\t') out += "\\t";
    else if ((uint8_t)c < 0x20) { }
    else out += c;
  }
  return out;
}

// [113.0.0] เพิ่ม: ข้อมูลสดสำหรับแดชบอร์ด หน้าเว็บดึงชุดนี้ทุกสองวินาที
//           แล้ววาดใหม่เฉพาะตัวเลขที่เปลี่ยน ไม่ต้องรีโหลดทั้งหน้า
void handleDashboardAPI() {
  if (!isAuthenticated()) { server.send(401, "application/json", "{}"); return; }

  int served = 0;
  int shopMeals[4] = {0, 0, 0, 0};
  for (const auto &st : db) {
    if (!st.claimed) continue;
    served++;
    if (st.station >= 1 && st.station <= 4) shopMeals[st.station - 1]++;
  }
  int total = (int)db.size();

  char win[16];
  snprintf(win, sizeof(win), "%02d:%02d-%02d:%02d",
           serviceStartHour, serviceStartMin, serviceEndHour, serviceEndMin);

  String j = "{";
  j += "\"clock\":\"" + jsonEscape(getTimeOnlyStr()) + "\"";
  j += ",\"date\":\"" + jsonEscape(getDateFormattedStr()) + "\"";
  j += ",\"open\":" + String(isWithinServiceTime() ? "true" : "false");
  j += ",\"window\":\"" + String(win) + "\"";
  j += ",\"served\":" + String(served);
  j += ",\"total\":" + String(total);
  j += ",\"left\":" + String(total - served);
  j += ",\"amount\":" + String(served * 35);
  j += ",\"dark\":" + String(isTftDarkMode ? "true" : "false");
  j += ",\"saver\":" + String(stationScreensaver ? "true" : "false");

  j += ",\"shops\":[";
  for (int i = 0; i < 4; i++) {
    if (i) j += ",";
    j += "{\"name\":\"" + jsonEscape(shops[i].name) + "\"";
    j += ",\"owner\":\"" + jsonEscape(shops[i].vendor) + "\"";
    j += ",\"screen\":\"" + jsonEscape(shops[i].screen) + "\"";
    j += ",\"meals\":" + String(shopMeals[i]);
    j += ",\"amount\":" + String(shopMeals[i] * 35) + "}";
  }
  j += "]";

  j += ",\"stations\":[";
  for (int i = 0; i < 4; i++) {
    if (i) j += ",";
    bool on = stationNodes[i].isOnline;
    j += "{\"id\":" + String(i + 1);
    j += ",\"online\":" + String(on ? "true" : "false");
    j += ",\"bars\":" + String(on ? calculateSignalQuality(stationNodes[i].rssi) : 0) + "}";
  }
  j += "]}";

  server.send(200, "application/json; charset=utf-8", j);
}

// [113.6.0] เพิ่ม: ข้อมูลสำหรับจอสาธารณะบนทีวี **เปิดดูได้โดยไม่ต้องเข้าสู่ระบบ**
//
// ทุกสิ่งที่ตอบออกไปจากตรงนี้ ถือว่าใครก็อ่านได้ จึงมีแค่ตัวเลขรวมกับรายการที่ปิดบังแล้ว
// สิ่งที่ห้ามมีเด็ดขาด: ชื่อนิสิต หมายเลขบัตร เลขอ้างอิง และข้อมูลฮาร์ดแวร์ของเครื่อง
// รายการล่าสุดถูกปิดบังตั้งแต่ตอนเก็บลงวงแหวนแล้ว ตรงนี้จึงไม่ต้องปิดบังซ้ำ
// และไม่มีทางเผลอหลุดของที่ไม่ได้เก็บไว้ตั้งแต่แรก
void handlePublicBoard() {
  int served = 0;
  int shopMeals[4] = {0, 0, 0, 0};
  for (const auto &st : db) {
    if (!st.claimed) continue;
    served++;
    if (st.station >= 1 && st.station <= 4) shopMeals[st.station - 1]++;
  }
  int total = (int)db.size();

  char win[16];
  snprintf(win, sizeof(win), "%02d:%02d-%02d:%02d",
           serviceStartHour, serviceStartMin, serviceEndHour, serviceEndMin);

  String j = "{";
  j += "\"clock\":\"" + jsonEscape(getTimeOnlyStr()) + "\"";
  j += ",\"date\":\"" + jsonEscape(getDateFormattedStr()) + "\"";
  j += ",\"open\":" + String(isWithinServiceTime() ? "true" : "false");
  j += ",\"window\":\"" + String(win) + "\"";
  j += ",\"served\":" + String(served);
  j += ",\"total\":" + String(total);
  j += ",\"left\":" + String(total - served);
  j += ",\"amount\":" + String(served * 35);
  j += ",\"dark\":" + String(isTftDarkMode ? "true" : "false");

  // ชื่อร้านกับยอดขาย ไม่มีชื่อผู้ประกอบการ เพราะเป็นชื่อคนจริงที่ไม่จำเป็นต้องขึ้นจอใหญ่
  j += ",\"shops\":[";
  for (int i = 0; i < 4; i++) {
    if (i) j += ",";
    j += "{\"name\":\"" + jsonEscape(shops[i].name) + "\"";
    j += ",\"meals\":" + String(shopMeals[i]);
    j += ",\"amount\":" + String(shopMeals[i] * 35) + "}";
  }
  j += "]";

  // รายการล่าสุด เรียงจากใหม่ไปเก่า
  j += ",\"feed\":[";
  for (int n = 0; n < publicFeedCount; n++) {
    int idx = (publicFeedHead - 1 - n + PUBLIC_FEED_SIZE * 2) % PUBLIC_FEED_SIZE;
    if (n) j += ",";
    j += "{\"id\":\"" + String(publicFeed[idx].maskedId) + "\"";
    j += ",\"at\":\"" + String(publicFeed[idx].atTime) + "\"";
    j += ",\"shop\":" + String(publicFeed[idx].shop) + "}";
  }
  j += "]}";

  server.send(200, "application/json; charset=utf-8", j);
}

// เครื่องแม่ข่ายสั่งโหมดการแสดงผลของทุกสถานีจากหน้าเว็บ
// (เดิมสั่งได้จากปุ่มกดบนเครื่องเท่านั้น ซึ่งคนที่มารับช่วงดูแลต่อจะไม่มีทางรู้)
// [113.0.0] เพิ่ม: สั่งโหมดการแสดงผลของทุกจุดบริการจากหน้าเว็บ
//           ของเดิมสั่งได้จากปุ่มบนเครื่องเท่านั้น คนที่มารับช่วงต่อจะไม่มีทางรู้
void handleSetDisplayMode() {
  if (!isAuthenticated()) { server.send(401, "application/json", "{}"); return; }

  if (server.hasArg("dark")) {
    isTftDarkMode = (server.arg("dark") == "1");
    preferences.begin("sys_cfg", false);
    preferences.putBool("tft_dark", isTftDarkMode);
    preferences.end();
  }
  if (server.hasArg("saver")) {
    stationScreensaver = (server.arg("saver") == "1");
    stationScreenOn = true;
    isScreensaverActive = stationScreensaver;
  }
  announceDisplayMode();
  renderHostPage(true);
  server.send(200, "application/json", "{\"ok\":true}");
}

// ============================================================================
// ส่วนที่แยกออกไปเป็นไฟล์ของตัวเอง Arduino IDE จะแสดงเป็นแท็บของสเก็ตช์เดียวกัน
// วางบรรทัด #include ไว้ตรงนี้เพราะโค้ดข้างในอ้างถึงตัวแปรส่วนกลางข้างบน
// **อย่าย้ายขึ้นไปไว้บนสุด**
// ============================================================================
#include "HostScreen.h"
#include "WebDashboard.h"

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
  preferences.end();

  // เคยเจอมาแล้วว่าชื่อ Wi-Fi ไม่โผล่มาเลยทั้งที่เครื่องบูตปกติ
  // ลองซ้ำสามครั้งแล้วจำผลไว้ เพื่อเอาไปขึ้นจอบอกตอนบูตว่าสำเร็จหรือไม่
  WiFi.mode(WIFI_AP);
  bool apReady = false;
  for (int attempt = 0; attempt < 3 && !apReady; attempt++) {
    apReady = WiFi.softAP(default_ap_ssid, default_ap_pass, ESPNOW_CHANNEL, 0, 4);
    if (!apReady) delay(300);
  }
  hostApReady = apReady;
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
  esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  esp_wifi_set_max_tx_power(68);

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  esp_now_init();
  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.ifidx = WIFI_IF_AP;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  delay(50);
  broadcastStationTheme();

  MDNS.begin(mdns_hostname);
  MDNS.addService("http", "tcp", 80);

  const char * headerkeys[] = {"Cookie"};
  server.collectHeaders(headerkeys, 1);

  server.on("/login", HTTP_GET, []() {
    bool hasErr = server.hasArg("error");
    server.send(200, "text/html; charset=utf-8", getLoginHTML(hasErr));
  });

  server.on("/login", HTTP_POST, []() {
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
      String token = generateSessionToken();
      int slot = 0;
      for (int i = 0; i < 3; i++) {
        if (activeSessions[i].token == "" || millis() >= activeSessions[i].expiry) {
          slot = i;
          break;
        }
      }
      activeSessions[slot].token = token;
      activeSessions[slot].username = matchedUser;
      activeSessions[slot].expiry = millis() + 7200000;

      server.sendHeader("Set-Cookie", "CANTEEN_SESSION=" + token + "; Path=/; HttpOnly");
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
    } else {
      server.sendHeader("Location", "/login?error=1", true);
      server.send(302, "text/plain", "");
    }
  });

  server.on("/logout", HTTP_GET, []() {
    server.sendHeader("Set-Cookie", "CANTEEN_SESSION=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
    server.sendHeader("Location", "/login", true);
    server.send(302, "text/plain", "");
  });

  server.on("/", HTTP_GET, []() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    server.send(200, "text/html; charset=utf-8", getHTML());
  });

  // ไฟล์หน้าตาเว็บ เสิร์ฟตรงจากแฟลช ไม่ต้องสร้างสตริงในแรม และเบราว์เซอร์แคชไว้ได้
  server.on("/s.css", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "max-age=86400");
    server.send_P(200, "text/css", DASH_CSS);
  });
  server.on("/a.js", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "max-age=86400");
    // ต้องระบุ charset เพราะในไฟล์นี้มีคำแปลภาษาไทยอยู่ด้วย
    server.send_P(200, "application/javascript; charset=utf-8", DASH_JS);
  });
  // [113.6.0] เพิ่ม: จอสาธารณะสำหรับทีวีในโรงอาหาร **ไม่ต้องเข้าสู่ระบบ**
  //           หน้าเว็บเป็นไฟล์นิ่งในแฟลช ส่งตรงไม่ต้องสร้างสตริงในแรม
  server.on("/display", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "max-age=3600");
    server.send_P(200, "text/html; charset=utf-8", DISPLAY_HTML);
  });
  server.on("/api/board", HTTP_GET, handlePublicBoard);

  server.on("/api/students", HTTP_GET, handleGetStudentsAPI);
  server.on("/api/dashboard", HTTP_GET, handleDashboardAPI);
  server.on("/api/display", HTTP_POST, handleSetDisplayMode);
  server.on("/export.csv", HTTP_GET, handleExportCSV);
  server.on("/api/rtc/set", HTTP_POST, handleSetRTCTime);
  server.on("/api/archive/download", HTTP_GET, handleDownloadArchive);
  server.on("/api/archive/delete", HTTP_POST, handleDeleteArchive);
  server.on("/api/student/save", HTTP_POST, handleSaveStudent);
  server.on("/api/tempcard/save", HTTP_POST, handleSaveTempCard);
  server.on("/api/tempcard/remove", HTTP_POST, handleRemoveTempCard);
  server.on("/api/student/delete", HTTP_POST, handleDeleteStudent);

  server.on("/api/admin/save", HTTP_POST, handleSaveAdmin);
  server.on("/api/admin/delete", HTTP_POST, handleDeleteAdmin);

  server.on("/api/system/health", HTTP_GET, []() {
    DynamicJsonDocument doc(256);
    doc["temp"] = String(getChipTemperature(), 1);
    doc["cpu"]  = String(calculateCpuLoad(), 1);
    doc["heap"] = ESP.getFreeHeap() / 1024;
    doc["uptime"] = millis() / 1000;
    String res;
    serializeJson(doc, res);
    server.send(200, "application/json; charset=utf-8", res);
  });

  server.on("/api/settings/time", HTTP_POST, []() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    timeWindowEnabled = (server.arg("enabled") == "1");
    String startStr = server.arg("start");
    String endStr = server.arg("end");
    sscanf(startStr.c_str(), "%d:%d", &serviceStartHour, &serviceStartMin);
    sscanf(endStr.c_str(), "%d:%d", &serviceEndHour, &serviceEndMin);

    preferences.begin("sys_cfg", false);
    preferences.putBool("win_en", timeWindowEnabled);
    preferences.putInt("st_h", serviceStartHour);
    preferences.putInt("st_m", serviceStartMin);
    preferences.putInt("end_h", serviceEndHour);
    preferences.putInt("end_m", serviceEndMin);
    preferences.end();

    sendAlert("Settings Saved Successfully!", "/");
  });

  server.on("/save-shops", HTTP_POST, []() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    for (int i = 0; i < 4; i++) {
      shops[i].name = server.arg("sname" + String(i));
      shops[i].vendor = server.arg("vname" + String(i));
      shops[i].screen = server.arg("dname" + String(i));
    }
    saveShopsToFS(); renderHostPage(true); sendAlert("Vendors Saved Successfully!", "/");
  });

  server.on("/bind-temp", HTTP_POST, []() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    String id = server.arg("id"); String tempUid = server.arg("uid"); tempUid.trim();
    for (auto& s : db) {
      if (s.studentId == id) {
        if (s.originalUid == "") s.originalUid = s.uid;
        s.uid = tempUid;
        s.isTempCard = true;
        break;
      }
    }
    saveDatabaseToFS(); renderHostPage(true); sendAlert("Temporary Card Assigned Successfully!", "/");
  });

  server.on("/manual-claim", HTTP_POST, []() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    String id = server.arg("id"); int station = server.arg("station").toInt();
    for (auto& s : db) {
      if (s.studentId == id && !s.claimed) {
        s.claimed = true; s.station = station; s.claimTime = getRealTimeStr(); s.refNo = generateRefNo(station);
        lastScannedUID = s.uid; lastScannedStudentId = s.studentId;
        lastScannedStation = station; lastScannedStatus = "APPROVED";
        lastScannedRef = s.refNo;
        pushPublicFeed(s.studentId, s.claimTime, station);
        appendLogToFS(s.studentId, s.fullName, s.uid, s.refNo, s.claimTime, station, s.isTempCard ? "Temp Card" : "Normal");
        
        if (s.isTempCard) {
          if (s.originalUid != "") { s.uid = s.originalUid; s.originalUid = ""; }
          else { s.uid = ""; }
          saveDatabaseToFS();
        }
        break;
      }
    }
    renderHostPage(true); sendAlert("Manual Claim Approved Successfully!", "/");
  });

  server.on("/reset", HTTP_POST, []() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    if (LittleFS.exists("/daily_log.csv")) {
      DateTime now = rtc.now();
      char arcName[40];
      snprintf(arcName, sizeof(arcName), "/arc_%04d%02d%02d_%02d%02d%02d.csv",
               now.year(), now.month(), now.day(),
               now.hour(), now.minute(), now.second());
      File src = LittleFS.open("/daily_log.csv", "r");
      File dst = LittleFS.open(arcName, "w");
      if (src && dst) {
        while (src.available()) dst.write(src.read());
        src.close(); dst.close();
      }
      LittleFS.remove("/daily_log.csv");
    }

    for (auto& s : db) {
      if (s.originalUid != "") {
        s.uid = s.originalUid;
        s.originalUid = "";
      }
      s.claimed = false; s.claimTime = "-"; s.refNo = "-"; s.station = 0; s.isTempCard = false;
    }
    saveDatabaseToFS();
    lastScannedUID = "-"; lastScannedStudentId = "-"; lastScannedStation = 0; lastScannedStatus = "RESET"; lastScannedRef = "-";
    publicFeedCount = 0; publicFeedHead = 0;   // ปิดยอดแล้ว จอทีวีต้องเริ่มนับใหม่ด้วย
    renderHostPage(true); sendAlert("Daily Reset & Archived Successfully!", "/");
  });

  server.on("/upload", HTTP_POST, mergeImportedStudents, handleFileUpload);

  server.on("/generate_204", HTTP_GET, []() { server.sendHeader("Location", "/login", true); server.send(302, "text/plain", ""); });
  server.on("/hotspot-detect.html", HTTP_GET, []() { server.sendHeader("Location", "/login", true); server.send(302, "text/plain", ""); });
  server.onNotFound([]() { server.sendHeader("Location", "/login", true); server.send(302, "text/plain", ""); });

  server.begin();
  playBootAnimation();
  showBootNetworkStatus();

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
    sendToStation(i, (uint8_t *)&beacon, sizeof(HostResponsePacket));
    delay(20);
  }

  lastActivity = millis();
  renderHostPage(true);
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  handleHostButton();
  calculateCpuLoad();

  ScanQueueItem item;
  if (scanQueue != NULL && xQueueReceive(scanQueue, &item, 0) == pdTRUE) {
    processScanRequest(item.mac, item.pkt, item.rssi);
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
      sendToStation(i + 1, (uint8_t *)&ack, sizeof(HostResponsePacket));
      sendStationTheme(i + 1);
    }
  }

  // [113.9.0] เพิ่ม: ย้ำคำสั่งการแสดงผลรอบที่เหลือ กระจายออกไปราวสองวินาที
  //           จุดบริการที่พลาดรอบแรกจึงได้รับภายในไม่ถึงวินาที แทนที่จะรอ heartbeat
  if (displayAnnounceLeft > 0 && millis() >= displayAnnounceNext) {
    displayAnnounceLeft--;
    displayAnnounceNext = millis() + DISPLAY_ANNOUNCE_GAP;
    sendDisplayConfigRound();
  }

  if (isLiveScanDisplaying && millis() > liveScanHoldUntil) {
    isLiveScanDisplaying = false;
    renderHostPage(true);
  }

  if (!isLiveScanDisplaying && !isScreensaverActive && !isCreditActive && (millis() - lastActivity >= TIMEOUT_SCREENSAVER)) {
    isScreensaverActive = true; 
    renderScreensaver(true);
  }

  // [113.2.0] แก้: เดิมรีเฟรชเฉพาะหน้า 1 กับหน้า 2 หน้า SYSTEM จึงค้างอยู่กับที่
  //           ทั้งที่มีอุณหภูมิชิปกับหน่วยความจำว่างที่ควรขยับ ตอนนี้รีเฟรชทุกหน้า
  //           ฟังก์ชันวาดจะเทียบค่าเก่าเองแล้ววาดซ้ำเฉพาะช่องที่เปลี่ยน จอจึงไม่กะพริบ
  if (!isLiveScanDisplaying && !isCreditActive && (millis() - lastClockRefresh >= 1000)) {
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
    // [113.2.0] ย้าย: การวาดซ้ำย้ายไปอยู่กับรอบหนึ่งวินาทีข้างบนแล้ว
    //           ตรงนี้เหลือหน้าที่เดียวคือตัดสถานีที่เงียบหายไปออกจากสถานะออนไลน์
  }

  delay(2);
}