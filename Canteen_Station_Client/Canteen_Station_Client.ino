/**
 * ============================================================================
 * Project: Meal Subsidy Management System (Tuesday 35-Baht Quota)
 * System: Vendor Station Client & Dynamic Theme Suite
 * Version: 119.0.0 (Offline Safety Queue: Serve Students During Link Loss)
 * Release Date: กันยายน 2569 (September 2026)
 * 
 * Developer: กิตติพันธ์ รัตนคร (Kittiphan Rattanakorn)
 * Role: นักวิชาการคอมพิวเตอร์ (Computer Technical Officer)
 * Organization: มหาวิทยาลัยมหาจุฬาลงกรณราชวิทยาลัย วิทยาเขตแพร่
 * 
 * Target Board: ESP32-S3 N16R8 + 2.8" ST7789V TFT (320x240) + RC522 + RGB LED
 *
 * SPI BUS MAP (สำคัญ: จอกับเครื่องอ่านบัตรต้องอยู่คนละบัส)
 *   - TFT ST7789V : HSPI (SPI3_HOST) ผ่าน SPI_TFT  -> SCLK 14 / MOSI 13 / CS 10
 *   - RC522       : FSPI (SPI2_HOST) ผ่าน SPI ตัวมาตรฐานที่ไลบรารี MFRC522 เรียกใช้
 *     บน ESP32-S3 ตัวแปร SPI มาตรฐานผูกกับ FSPI อยู่แล้ว ถ้าปล่อยให้จอใช้ FSPI ด้วย
 *     ทั้งสองอุปกรณ์จะแย่ง peripheral เดียวกันคนละขา ทำให้จอเพี้ยนหลัง PCD_Init()
 * ============================================================================
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <MFRC522.h>
#include <time.h>
#include <sys/time.h>

#define APP_VERSION         "120.0.1"
#define DEV_NAME            "Kittiphan Rattanakorn"
#define DEV_ROLE            "Computer Technical Officer"
#define DEV_INSTITUTION     "MCU Phrae Campus"

// Pin Configuration สำหรับ ESP32-S3 DevKitC-1
#define TFT_CS              10
#define TFT_DC              11
#define TFT_RST             12
#define TFT_MOSI            13
#define TFT_SCLK            14
#define TFT_BLK             15

#define RC522_SS            7
#define RC522_SCK           4
#define RC522_MOSI          5
#define RC522_MISO          21  
#define RC522_RST           17  

#define BUZZER_PIN          38  
#define BTN_PIN             2
#define BATTERY_ADC_PIN     3   
#define RGB_LED_PIN         48  

// ============================================================================
// DYNAMIC THEME ENGINE (คาลิเบทตาม TFT Color Calibration Tool)
// ============================================================================
bool isStationDarkMode = true;

// สีหลักในโหมดมืด (Dark Mode) อ้างอิงจากไฟล์คาลิเบท
#define DARK_BG             0x0000  
#define DARK_CARD_BG        0x10A4  
#define DARK_CARD_BORDER    0x2965  
#define DARK_TEXT_MAIN      0xFFFF  
#define DARK_TEXT_MUTED     0x8410  
#define DARK_ACCENT_GREEN   0x07E0  
#define DARK_ACCENT_CYAN    0x07FF  
#define DARK_ACCENT_YELLOW  0xFFE0  
#define DARK_ACCENT_ROSE    0xF800  

// สีหลักในโหมดสว่าง (Light Mode)
#define LIGHT_BG            0xF7BF  
#define LIGHT_CARD_BG       0xFFFF  
#define LIGHT_CARD_BORDER   0xCE79  
#define LIGHT_TEXT_MAIN     0x0841  
#define LIGHT_TEXT_MUTED    0x632C  
#define LIGHT_ACCENT_GREEN  0x04A6  
#define LIGHT_ACCENT_CYAN   0x0318  
#define LIGHT_ACCENT_YELLOW 0xCBA0  
#define LIGHT_ACCENT_ROSE   0xD800  

#ifndef ST77XX_ORANGE
  #define ST77XX_ORANGE     0xFD20
#endif
#ifndef BENTO_NEON_YELLOW
  #define BENTO_NEON_YELLOW 0xFFE0
#endif

uint16_t getStBg()          { return isStationDarkMode ? DARK_BG : LIGHT_BG; }
uint16_t getStCardBg()      { return isStationDarkMode ? DARK_CARD_BG : LIGHT_CARD_BG; }
uint16_t getStCardBorder()  { return isStationDarkMode ? DARK_CARD_BORDER : LIGHT_CARD_BORDER; }
uint16_t getStTextMain()    { return isStationDarkMode ? DARK_TEXT_MAIN : LIGHT_TEXT_MAIN; }
uint16_t getStTextMuted()   { return isStationDarkMode ? DARK_TEXT_MUTED : LIGHT_TEXT_MUTED; }
uint16_t getStGreen()       { return isStationDarkMode ? DARK_ACCENT_GREEN : LIGHT_ACCENT_GREEN; }
uint16_t getStCyan()        { return isStationDarkMode ? DARK_ACCENT_CYAN : LIGHT_ACCENT_CYAN; }
uint16_t getStYellow()      { return isStationDarkMode ? DARK_ACCENT_YELLOW : LIGHT_ACCENT_YELLOW; }
uint16_t getStRose()        { return isStationDarkMode ? DARK_ACCENT_ROSE : LIGHT_ACCENT_ROSE; }

SPIClass SPI_TFT(HSPI);   // จอแยกไปบัส HSPI เพื่อไม่ชนกับ RC522 ที่ใช้ SPI (FSPI)
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI_TFT, TFT_CS, TFT_DC, TFT_RST);
MFRC522 mfrc522(RC522_SS, RC522_RST);
Preferences stationPrefs;

uint8_t currentStationId = 1;
char dynamicShopLabel[32];

uint8_t broadcastAddress[]   = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t hostMacAddress[6]    = {0};
bool hostPeerReady           = false;
const uint8_t ESPNOW_CHANNEL = 1;

// ============================================================================
// ESP-NOW PROTOCOL
// ============================================================================
enum MsgType : uint8_t {
  MSG_HEARTBEAT = 1,
  MSG_SCAN_REQ  = 2,
  MSG_SCAN_RESP = 3,
  MSG_CONFIG    = 4,
  MSG_ROSTER    = 5    // แม่ข่ายผลักบัญชีสิทธิ์ย่อมาให้เก็บไว้ตรวจเองตอนลิงก์ขาด
};

// ---------------------------------------------------------------------------
// บังคับให้ ESP-NOW ใช้อัตราส่งแบบ Long Range (250 kbps) รับสัญญาณอ่อนได้ดีขึ้นมาก
// ต้องตั้งเป็นค่าเดียวกันกับไฟล์ของเครื่องแม่ข่าย และแฟลชทั้งสองฝั่ง
// ถ้าเปิดข้างเดียวทั้งสองเครื่องจะคุยกันไม่รู้เรื่อง รายละเอียดอยู่ใน README
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

// คำสั่งโหมดการแสดงผลจากเครื่องแม่ข่าย
//   darkMode    แม่ข่ายเป็นเจ้าของ สถานีเปลี่ยนตามเสมอเมื่อค่าไม่ตรงกัน
//   screenOn    ทำตามเฉพาะตอน modeSeq เปลี่ยน สถานียังกดปิด/เปิดจอเองได้ภายหลัง
//   screensaver เช่นเดียวกับ screenOn
//   modeSeq     แม่ข่ายเพิ่มค่านี้ทุกครั้งที่เจ้าหน้าที่เปลี่ยนโหมดการแสดงผล
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
// บัญชีสิทธิ์ย่อ (roster) ที่แม่ข่ายผลักมาให้ — ต้องตรงกับฝั่งแม่ข่ายทุกไบต์
//
// เก็บเป็นค่าแฮช 32 บิตของเลขบัตรคู่กับสถานะใช้สิทธิ์ จึงกินแค่ 5 ไบต์ต่อคน
// เครื่องนี้ไม่เคยได้รับเลขบัตรจริงหรือชื่อนิสิตเลย ถึงเครื่องหายก็ไม่มีข้อมูลส่วนบุคคลติดไป
// ---------------------------------------------------------------------------
#define ROSTER_ENTRIES_PER_PKT 38
#define ROSTER_FLAG_FULL_BEGIN 0x01
#define ROSTER_FLAG_FULL_END   0x02
#define ROSTER_FLAG_DELTA      0x04

typedef struct __attribute__((packed)) {
  uint32_t hash;
  uint8_t  state;   // 0 = ยังไม่ใช้สิทธิ์, 1 = ใช้สิทธิ์แล้ววันนี้
} RosterEntry;

typedef struct __attribute__((packed)) {
  uint8_t  magic;
  uint8_t  version;
  uint8_t  msgType;
  uint8_t  stationId;
  uint16_t rosterVer;
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

enum AppState { 
  STATE_STANDBY, 
  STATE_SCANNING_SENT, 
  STATE_RESULT_DISPLAY, 
  STATE_STATUS, 
  STATE_SCREENSAVER, 
  STATE_CREDIT, 
  STATE_CONFIG_ID,
  STATE_SYNCING        // กำลังส่งรายการที่บันทึกไว้ตอนขาดการเชื่อมต่อเข้าระบบ
};
AppState currentState = STATE_STANDBY;

int currentStationPage              = 1;  
bool isScreenOn                     = true;
bool isHostOnline                   = false;
bool isTimeSynced                   = false;

int lastHostRssi                    = -60;
unsigned long lastHostAckTime       = 0;
unsigned long lastActivityTime      = 0;
unsigned long lastClockRefresh      = 0;
unsigned long lastHeartbeatTime     = 0;
unsigned long stateHoldUntil        = 0;
unsigned long lastRc522HealthCheck  = 0;
unsigned long nextHeartbeatInterval = 6000;

uint32_t totalScansToday            = 0;   // จำนวนครั้งที่แตะบัตรทั้งหมด นับตั้งแต่เปิดเครื่อง
uint32_t totalRejectToday           = 0;   // ที่ไม่ผ่าน (บัตรซ้ำ/ไม่อยู่ในทะเบียน/นอกเวลา)
uint32_t totalSuccessToday          = 0;   // ยอดที่จ่ายจริง ซิงค์จากแม่ข่ายจึงถูกต้องแม้รีบูต

// รายการที่จ่ายสำเร็จล่าสุดของสถานีนี้ เก็บในแรมเพื่อแสดงบนหน้า 2
#define STN_FEED_SIZE 3
struct StationTap {
  char id[16];
  char time[12];
};
StationTap stnFeed[STN_FEED_SIZE] = {};
uint8_t stnFeedCount = 0;
uint8_t stnFeedHead  = 0;
char lastPayoutTime[12] = "--:--";

// ---------------------------------------------------------------------------
// คิวออฟไลน์: เมื่อแม่ข่ายไม่ตอบ สถานีจะบันทึกการแตะบัตรลงหน่วยความจำถาวร
// แล้วให้แม่ค้าจ่ายอาหารไปก่อน พอลิงก์กลับมาจึงส่งเข้าระบบเองโดยใช้เวลาตอนแตะจริง
// ข้อมูลอยู่ใน NVS จึงไม่หายแม้ไฟดับหรือรีบูตกลางคัน
// ---------------------------------------------------------------------------
#define OFFLINE_QUEUE_MAX 48
struct OfflineTap {
  char uid[16];
  char time[20];        // "YYYY-MM-DD HH:MM:SS"
};
OfflineTap offlineQueue[OFFLINE_QUEUE_MAX];
uint8_t offlineCount = 0;

bool syncInProgress          = false;
bool syncQuiet               = false;   // ซิงค์เงียบ ๆ ขณะพักหน้าจอหรือจอดับ
AppState syncReturnState     = STATE_STANDBY;
int syncReturnPage           = 1;
uint8_t syncTotal            = 0;
uint8_t syncDone             = 0;
uint8_t syncRejected         = 0;
uint8_t syncRetry            = 0;
uint16_t syncSeq             = 0;
unsigned long syncSentAt     = 0;
volatile bool syncAckReceived = false;

// ---------------------------------------------------------------------------
// บัญชีสิทธิ์ย่อที่แม่ข่ายผลักมาให้ ใช้ตรวจสิทธิ์เองตอนลิงก์ขาด
//
// เดิมตอนแม่ข่ายไม่ตอบ เครื่องนี้รับบัตร "ทุกใบ" เข้าคิวโดยไม่ตรวจอะไรเลย
// บัตรที่ไม่ได้ลงทะเบียนหรือบัตรที่ใช้สิทธิ์ไปแล้วก็ได้อาหารไปก่อน แล้วค่อยไปตกตอนซิงค์
// ซึ่งสายเกินกว่าจะเรียกคืนได้ ตอนนี้ตรวจกับบัญชีนี้ก่อนตั้งแต่ตอนแตะ
//
// หลักการสำคัญ: ถ้า "ไม่มั่นใจ" ให้ปล่อยผ่านเสมอ (fail open)
// เพราะการปฏิเสธนิสิตที่มีสิทธิ์จริงเสียหายกว่าการปล่อยบัตรแปลกปลอมผ่านไปหนึ่งใบ
// ซึ่งแม่ข่ายจะปัดตกตอนซิงค์อยู่ดี กรณีที่ถือว่าไม่มั่นใจคือ ยังไม่เคยได้รับบัญชี
// กำลังรับชุดใหม่อยู่ หรือบัญชีใหญ่เกินที่เก็บไหว
// ---------------------------------------------------------------------------
#define ROSTER_MAX 600
enum RosterVerdict { ROSTER_UNKNOWN, ROSTER_ELIGIBLE, ROSTER_CLAIMED, ROSTER_NOT_FOUND };

RosterEntry stationRoster[ROSTER_MAX];
volatile uint16_t rosterCount     = 0;
volatile uint16_t rosterVer       = 0;    // 0 = ยังไม่มีบัญชีที่เชื่อถือได้
volatile uint32_t rosterDate      = 0;    // วันที่ของบัญชีชุดที่ถืออยู่ (YYYYMMDD)
volatile bool     rosterBuilding  = false;
volatile bool     rosterTruncated = false;
volatile uint16_t rosterFillNext  = 0;
volatile uint8_t  rosterNextChunk = 0;
volatile bool     rosterDirty     = false;
unsigned long rosterSaveAt        = 0;
const unsigned long ROSTER_SAVE_GAP_MS = 300000;   // เขียนลง NVS อย่างมาก 5 นาทีครั้ง
unsigned long syncRetryNotBefore = 0;   // กันการวนลองซิงค์รัวเมื่อแม่ข่ายหายอีก

// ภาพรวมทั้งโรงอาหารที่แม่ข่ายฝากมากับ heartbeat
bool hasSystemInfo   = false;
uint16_t sysUsed     = 0;
uint16_t sysTotal    = 0;
bool sysServiceOpen  = false;
char sysWindow[16]   = "--:--";
volatile bool pendingSysInfo = false;
char pendingSysMsg[32] = {0};
String lastProcessedUID             = "";
unsigned long lastProcessedTime     = 0;

volatile bool lastSendFailed        = false;  // ชิปรายงานว่าส่งแพ็กเก็ตล่าสุดไม่ถึง
volatile bool hasNewPacket          = false;
HostResponsePacket receivedPacketBuffer;

volatile bool hasServedCountUpdate  = false;
volatile uint16_t pendingServedCount = 0;
volatile bool pendingTimeSync = false;
char pendingHostTime[24] = {0};

volatile bool pendingConfigUpdate = false;
volatile uint8_t cfgDark        = 1;
volatile uint8_t cfgScreenOn    = 1;
volatile uint8_t cfgScreensaver = 0;
volatile uint8_t cfgModeSeq     = 0;
bool hasAppliedModeSeq   = false;
uint8_t lastAppliedModeSeq = 0;
portMUX_TYPE espnowMux = portMUX_INITIALIZER_UNLOCKED;

unsigned long lastHeaderRefresh = 0;
const unsigned long HEADER_REFRESH_MS = 1000;

uint16_t nextScanSeq = 1;
uint16_t pendingScanSeq = 0;
StationPacket pendingScanPacket = {};
uint8_t scanRetryCount = 0;
const uint8_t SCAN_MAX_RETRY = 6;   // เดิม 3 ครั้ง ใช้เวลาแค่ 1.35 วินาทีจาก timeout 3 วินาที
unsigned long lastScanSendTime = 0;

const unsigned long TIMEOUT_SCREENSAVER = 300000; 
const unsigned long MULTI_CLICK_GAP     = 320;
const unsigned long STATION_ID_HOLD_MS  = 3000;
const unsigned long CONFIG_AUTO_SAVE_MS = 3000;
const unsigned long COOLDOWN_MS         = 3500;
const unsigned long SCAN_TIMEOUT_MS         = 3000;
// เมื่อรู้อยู่แล้วว่าแม่ข่ายหลุด ไม่ต้องให้นิสิตยืนรอครบสามวินาทีทุกคน
const unsigned long SCAN_TIMEOUT_OFFLINE_MS = 1500;
const unsigned long SYNC_ACK_TIMEOUT_MS     = 2000;
const unsigned long BASE_HEARTBEAT      = 6000;
const unsigned long HOST_OFFLINE_TIMEOUT = 15000;

float stationCpuLoad = 8.0;
float stationCpuTemperature = 0.0;
unsigned long lastCpuMeasureTime = 0;
unsigned long loopCounter = 0;
unsigned long lastCpuDisplayUpdate = 0;
float lastDisplayedCpuLoad = -999.0;
float lastDisplayedCpuTemperature = -999.0;

// Forward Declarations
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);
#else
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len);
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
#endif

void renderScreensaver(bool fullRedraw);
void renderDeveloperCredit();
void displayTapCardStandby();
void displayStatsDashboard(bool fullRedraw);
String maskStudentId(const String &id);
void pushStationTap(const char *studentId);
void applySystemSummary(const char *msg);
String getFullTimeStr();
void loadOfflineQueue();
void saveOfflineQueue();
int findOfflineTap(const String &uid);
bool enqueueOfflineTap(const String &uid);
void popOfflineTap();
void displayOfflineSaved(const String &uid, bool alreadySaved);
void displaySyncProgress(uint8_t done, uint8_t total);
void displaySyncDone(uint8_t ok, uint8_t rejected);
void startOfflineSync();
void sendOfflineTap();
void finishOfflineSync(bool aborted);
void displayStatusScreen(bool fullRedraw);
void displayScanningUID(String uid);
void displayResult(String status, String name, String id, String refNo, String claimTime, String msg);
void displayOfflineAlert();
uint32_t uidHash32(const String &uid);
RosterVerdict rosterLookup(const String &uid);
void rosterMarkClaimedLocal(const String &uid);
void loadRoster();
void saveRosterIfDue(bool force);
void stampRosterVer(StationPacket &pkt);
// เช่นเดียวกับฝั่งแม่ข่าย ฟังก์ชันที่รับโครงสร้างของสเก็ตช์เป็นพารามิเตอร์
// ต้องมีการประกาศล่วงหน้าไว้หลังนิยามโครงสร้างเสมอ
void rosterApplyPacket(const HostRosterPacket &r);
uint32_t stationTodayYmd();
void displayOfflineRejected(const String &uid, bool alreadyUsed);
void playBootAnimation();
void runStationIdConfigMode();
void handlePhysicalButton();
void sendCardToHost(String uid);
void checkRC522();
float calculateCpuLoad();
float getChipTemperature();
void setLedColor(uint8_t r, uint8_t g, uint8_t b);
void ledStandby();
void ledApproved();
void ledDuplicate();
void ledRejected();
void ledOffline();
void ledOff();
float readBatteryVoltage(bool forceFresh = false);
int getBatteryPercentage(float voltage);
void drawStationCard(int x, int y, int w, int h, uint16_t borderColor, uint16_t bgColor);
void drawStationPillBadge(int x, int y, int w, int h, const char* text, uint16_t fgColor, uint16_t bgColor);
void drawStationBatteryHUD(int x, int y, bool force);
void drawStationTopBar(String title);
void updateStationHeaderStatus(bool force);
uint8_t fitTextSize(const char* text, int maxWidth, uint8_t maxSize);
void drawFitCenteredText(int x, int y, int w, int h, const char* text, uint8_t maxSize, uint16_t fg, uint16_t bg);
void showStationPage(int page, bool fullRedraw);
void applyHostConfig();
void showThemeLockedNotice();
void drawStationBottomBar(String instruction);
void drawRfidTapIcon(int cx, int cy, uint16_t cardColor, uint16_t waveColor, uint16_t bgColor);
void soundWelcome();
void soundCreditJingle();
void soundSuccess();
void soundAlarm();
void soundError();
void soundClick();
void soundScreensaverBeep();
void soundHomeBeep();
void soundThemeSwitch();
void updateShopLabel();
String maskUID(String uid);
bool isValidMac(const uint8_t *mac);
bool ensureHostPeer();
bool sendToHost(const uint8_t *data, size_t len);
void applyEspNowRate(const uint8_t *peerAddr);
int calculateSignalQuality(int rssi);
uint16_t getSignalColor(int rssi, bool isOnline);
int getActiveSignalBarsCount(int rssi, bool isOnline);
void drawSignalBars(int x, int y, int rssi, bool isOnline, uint16_t bg);
void syncInternalClock(const char* timeStr);
String getTimeOnlyStr();
String getDateFormattedStr();
void setScreenPower(bool powerOn);
void wakeScreenIfNeeded();

float calculateCpuLoad() {
  loopCounter++;
  unsigned long now = millis();
  if (lastCpuMeasureTime == 0) lastCpuMeasureTime = now;

  if (now - lastCpuMeasureTime >= 1000) {
    const float maxExpectedLoops = 460.0f;
    float idleRatio = (float)loopCounter / maxExpectedLoops;
    if (idleRatio > 1.0f) idleRatio = 1.0f;
    stationCpuLoad = (1.0f - idleRatio) * 100.0f;
    if (stationCpuLoad < 0.0f) stationCpuLoad = 0.0f;
    if (stationCpuLoad > 100.0f) stationCpuLoad = 100.0f;
    loopCounter = 0;
    lastCpuMeasureTime = now;
  }
  return stationCpuLoad;
}

float getChipTemperature() {
  stationCpuTemperature = temperatureRead();
  return stationCpuTemperature;
}

void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  neopixelWrite(RGB_LED_PIN, r, g, b);
}
void ledStandby()   { setLedColor(0, 10, 25); }   
void ledApproved()  { setLedColor(0, 65, 0); }     
void ledDuplicate() { setLedColor(65, 25, 0); }   
void ledRejected()  { setLedColor(65, 0, 0); }    
void ledOffline()   { setLedColor(65, 0, 0); }    
void ledOff()       { setLedColor(0, 0, 0); }

float cachedBatteryVoltage        = 0.0f;
unsigned long lastBatteryReadMs   = 0;
const unsigned long BATTERY_CACHE_MS = 3000;

// อ่านแรงดันแบตเตอรี่แบบมีแคช 3 วินาที และไม่ใช้ delay() ที่บล็อกลูปหลัก
float readBatteryVoltage(bool forceFresh) {
  unsigned long now = millis();
  if (!forceFresh && lastBatteryReadMs != 0 && (now - lastBatteryReadMs) < BATTERY_CACHE_MS) {
    return cachedBatteryVoltage;
  }
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(BATTERY_ADC_PIN);
    delayMicroseconds(300);
  }
  float avgRaw = sum / 8.0f;
  float pinVoltage = (avgRaw / 4095.0f) * 3.3f;
  cachedBatteryVoltage = pinVoltage * 2.0f;
  lastBatteryReadMs = (millis() == 0) ? 1 : millis();
  return cachedBatteryVoltage;
}

int getBatteryPercentage(float voltage) {
  if (voltage >= 4.15f) return 100;
  if (voltage <= 3.30f) return 0;
  int percent = (int)((voltage - 3.30f) / (4.15f - 3.30f) * 100.0f);
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  return percent;
}

// เลือกขนาดฟอนต์ที่ใหญ่ที่สุดที่ยังพอดีกับความกว้างที่กำหนด (ฟอนต์ GFX = 6px/ตัว ต่อ 1 size)
uint8_t fitTextSize(const char* text, int maxWidth, uint8_t maxSize) {
  int len = (int)strlen(text);
  if (len <= 0) return 1;
  for (uint8_t sz = maxSize; sz > 1; sz--) {
    if (len * 6 * (int)sz <= maxWidth) return sz;
  }
  return 1;
}

// วางข้อความกึ่งกลางกรอบ (x,y,w,h) โดยย่อขนาดอัตโนมัติถ้าล้นกรอบ
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

void drawStationCard(int x, int y, int w, int h, uint16_t borderColor, uint16_t bgColor) {
  tft.fillRoundRect(x, y, w, h, 6, bgColor);
  tft.drawRoundRect(x, y, w, h, 6, borderColor);
}

void drawStationPillBadge(int x, int y, int w, int h, const char* text, uint16_t fgColor, uint16_t bgColor) {
  tft.fillRoundRect(x, y, w, h, 3, bgColor);
  tft.setTextColor(fgColor, bgColor);
  tft.setTextSize(1);
  int textLen = strlen(text) * 6;
  int posX = x + (w - textLen) / 2;
  tft.setCursor(max(x + 2, posX), y + (h - 8) / 2);
  tft.print(text);
}

void drawStationBatteryHUD(int x, int y, bool force = false) {
  float volt = readBatteryVoltage();
  int pct = getBatteryPercentage(volt);

  static int lastDrawnPct = -999;
  static bool lastDrawnCharging = false;
  bool charging = (volt > 4.05f);
  if (!force && pct == lastDrawnPct && charging == lastDrawnCharging) return;
  lastDrawnPct = pct;
  lastDrawnCharging = charging;

  tft.fillRect(x, y, 52, 18, getStBg());
  tft.drawRect(x, y + 2, 44, 14, getStTextMain());
  tft.fillRect(x + 44, y + 6, 3, 6, getStTextMain());

  int fillWidth = (pct * 40) / 100;
  if (fillWidth < 1 && pct > 0) fillWidth = 1;

  uint16_t fillColor = (pct > 20) ? getStGreen() : getStRose();
  if (charging) fillColor = getStCyan();

  tft.fillRect(x + 2, y + 4, fillWidth, 10, fillColor);

  String pctStr = String(pct) + "%";
  int textWidth = pctStr.length() * 6;
  int textX = x + 2 + (40 - textWidth) / 2; 
  int textY = y + 5;

  tft.setTextSize(1);
  uint16_t textBg = (fillWidth > 22) ? fillColor : getStBg();
  tft.setTextColor((fillWidth > 22) ? 0x0000 : getStTextMain(), textBg);
  tft.setCursor(textX, textY);
  tft.print(pctStr);
}

void drawStationTopBar(String title) {
  tft.fillRect(0, 0, 320, 24, getStBg());
  tft.drawFastHLine(0, 24, 320, getStCardBorder());
  tft.setTextColor(getStCyan(), getStBg());
  tft.setTextSize(1);
  tft.setCursor(8, 8);
  // หัวข้อยาวสุด 30 ตัวอักษร เพื่อไม่ให้ทับบล็อกสถานะสัญญาณที่ x=190
  if (title.length() > 30) title = title.substring(0, 30);
  tft.print(title);
  updateStationHeaderStatus(true);
}

// สัญลักษณ์แตะบัตร RFID: ตัวบัตรพร้อมชิป และคลื่นสัญญาณสามชั้นแบบ contactless
// วาดด้วยพรีมิทีฟของ Adafruit GFX ล้วน ๆ จึงไม่กินแฟลชเพิ่มเหมือนการฝังบิตแมป
// drawCircleHelper ใช้บิต 0x2 (เสี้ยวบนขวา) และ 0x4 (เสี้ยวล่างขวา) รวมกันเป็นครึ่งขวา
void drawRfidTapIcon(int cx, int cy, uint16_t cardColor, uint16_t waveColor, uint16_t bgColor) {
  // ตัวบัตร วาดสองชั้นให้เส้นหนาขึ้นเพื่อให้มองเห็นชัดจากระยะไกล
  tft.drawRoundRect(cx - 33, cy - 15, 42, 30, 5, cardColor);
  tft.drawRoundRect(cx - 32, cy - 14, 40, 28, 4, cardColor);

  // ชิปสัมผัสบนหน้าบัตร
  tft.fillRoundRect(cx - 27, cy - 8, 12, 10, 2, cardColor);
  tft.drawFastHLine(cx - 27, cy - 4, 12, bgColor);
  tft.drawFastVLine(cx - 21, cy - 8, 10, bgColor);

  // คลื่นสัญญาณสามชั้น ไล่รัศมีออกไปทางขวา
  for (int i = 0; i < 3; i++) {
    int r = 9 + i * 6;
    tft.drawCircleHelper(cx + 14, cy, r, 0x6, waveColor);
    tft.drawCircleHelper(cx + 14, cy, r + 1, 0x6, waveColor);
  }
}

void drawStationBottomBar(String instruction) {
  tft.fillRect(0, 218, 320, 22, getStBg());
  tft.drawFastHLine(0, 218, 320, getStCardBorder());
  tft.setTextColor(getStTextMuted(), getStBg());
  tft.setTextSize(1);
  tft.setCursor(12, 224);
  tft.println(instruction);
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

void soundSuccess() { tone(BUZZER_PIN, 1800, 100); delay(120); tone(BUZZER_PIN, 2400, 150); }
void soundAlarm() {
  for (int i = 0; i < 3; i++) { tone(BUZZER_PIN, 2500, 100); delay(110); tone(BUZZER_PIN, 1200, 100); delay(110); }
  noTone(BUZZER_PIN);
}
void soundError() { tone(BUZZER_PIN, 600, 300); delay(350); tone(BUZZER_PIN, 400, 400); }
void soundClick() { tone(BUZZER_PIN, 2000, 40); }
void soundScreensaverBeep() { tone(BUZZER_PIN, 2400, 50); delay(70); tone(BUZZER_PIN, 1800, 70); }
void soundHomeBeep() { tone(BUZZER_PIN, 1800, 70); delay(80); tone(BUZZER_PIN, 2500, 100); }
void soundThemeSwitch() {
  tone(BUZZER_PIN, 1500, 60); delay(70);
  tone(BUZZER_PIN, 2200, 80); delay(90);
  noTone(BUZZER_PIN);
}

// จอแจ้งว่าบันทึกการแตะไว้แล้ว ให้แม่ค้าจ่ายอาหารไปก่อนได้
// ใช้โทนฟ้าเพื่อให้แยกจากเขียว(ผ่าน) ส้ม(ซ้ำ) และแดง(ไม่ผ่าน) ได้ชัดเจน
void displayOfflineSaved(const String &uid, bool alreadySaved) {
  wakeScreenIfNeeded();
  setLedColor(0, 40, 55);

  const uint16_t screenBg = 0x0209;
  const uint16_t cardBg   = 0x0126;
  const uint16_t banner   = alreadySaved ? ST77XX_ORANGE : 0x07FF;
  const uint16_t muted    = alreadySaved ? 0xFDC0 : 0x9EFF;

  tft.fillScreen(screenBg);
  tft.fillRect(0, 0, 320, 34, banner);
  drawFitCenteredText(0, 0, 320, 34,
                      alreadySaved ? "! ALREADY SAVED OFFLINE !" : "SAVED - SERVE THE STUDENT",
                      2, 0x0000, banner);

  tft.fillRoundRect(8, 40, 304, 142, 8, cardBg);
  tft.drawRoundRect(8, 40, 304, 142, 8, banner);
  tft.drawRoundRect(9, 41, 302, 140, 7, banner);

  tft.setTextSize(1);
  tft.setTextColor(muted, cardBg);
  tft.setCursor(20, 50);  tft.print("SERVICE STATION:");
  tft.setCursor(156, 50); tft.print("CARD UID (ENCRYPTED):");

  tft.setTextSize(2);
  tft.setTextColor(0xFFFF, cardBg);
  tft.setCursor(20, 62);  tft.printf("STATION 0%d", currentStationId);
  tft.setCursor(156, 62); tft.print(maskUID(uid));

  tft.drawFastHLine(20, 86, 280, banner);

  tft.setTextSize(1);
  tft.setTextColor(muted, cardBg);
  tft.setCursor(20, 94);
  tft.print("RECORDS WAITING TO SYNC:");

  tft.setTextSize(3);
  tft.setTextColor(0xFFFF, cardBg);
  tft.setCursor(20, 108);
  tft.printf("%u", (unsigned)offlineCount);
  tft.setTextSize(1);
  tft.printf(" / %u", (unsigned)OFFLINE_QUEUE_MAX);

  tft.setTextColor(muted, cardBg);
  tft.setCursor(20, 146);
  tft.print(alreadySaved ? "THIS CARD IS ALREADY IN THE QUEUE"
                         : "SENT AUTOMATICALLY WHEN THE LINK IS BACK");
  tft.setCursor(20, 162);
  tft.print("NOTHING IS LOST IF THE POWER GOES OFF");

  tft.fillRoundRect(16, 188, 288, 30, 6, banner);
  drawFitCenteredText(16, 188, 288, 30,
                      alreadySaved ? "NO SECOND MEAL FOR THIS CARD" : "HOST UNREACHABLE - RECORD KEPT ON THIS DEVICE",
                      1, 0x0000, banner);

  drawStationBottomBar("OFFLINE MODE | RECORD SAVED LOCALLY");
  if (alreadySaved) soundAlarm(); else soundSuccess();
}

void displaySyncProgress(uint8_t done, uint8_t total) {
  tft.fillScreen(getStBg());
  drawStationTopBar("SYNCING OFFLINE RECORDS");
  drawStationCard(16, 46, 288, 140, getStCyan(), getStCardBg());

  drawFitCenteredText(28, 60, 264, 16, "SENDING SAVED RECORDS TO THE HOST", 1,
                      getStTextMuted(), getStCardBg());

  char buf[16];
  snprintf(buf, sizeof(buf), "%u / %u", (unsigned)done, (unsigned)total);
  drawFitCenteredText(28, 86, 264, 32, buf, 4, getStCyan(), getStCardBg());

  int pct = (total > 0) ? (int)((uint32_t)done * 100 / total) : 0;
  tft.drawRoundRect(40, 132, 240, 12, 5, getStCardBorder());
  int w = (pct * 236) / 100;
  if (w > 0) tft.fillRoundRect(42, 134, w, 8, 4, getStCyan());

  drawFitCenteredText(28, 156, 264, 16, "PLEASE DO NOT TURN OFF THE DEVICE", 1,
                      getStTextMuted(), getStCardBg());
  drawStationBottomBar("SYNCING | PLEASE WAIT");
}

void displaySyncDone(uint8_t ok, uint8_t rejected) {
  tft.fillScreen(getStBg());
  drawStationTopBar("OFFLINE SYNC COMPLETE");
  drawStationCard(16, 50, 288, 132, getStGreen(), getStCardBg());

  drawFitCenteredText(28, 64, 264, 20, "SAVED RECORDS HAVE BEEN SENT", 2,
                      getStTextMain(), getStCardBg());

  char buf[40];
  snprintf(buf, sizeof(buf), "ACCEPTED %u", (unsigned)ok);
  drawFitCenteredText(28, 100, 264, 24, buf, 3, getStGreen(), getStCardBg());

  snprintf(buf, sizeof(buf), "REJECTED AS DUPLICATE: %u", (unsigned)rejected);
  drawFitCenteredText(28, 140, 264, 16, buf, 1,
                      rejected > 0 ? getStRose() : getStTextMuted(), getStCardBg());

  drawStationBottomBar("RETURNING TO THE MAIN PAGE...");
  soundSuccess();
}


// จอปฏิเสธบัตรขณะลิงก์ขาด — ตัดสินจากบัญชีสิทธิ์ที่แม่ข่ายผลักมาเก็บไว้ล่วงหน้า
// ต้องอ่านออกจากระยะที่แม่ค้ายืนอยู่ และต้องบอกชัดว่า "อย่าจ่ายอาหาร"
// เพราะเคสนี้ต่างจากจอฟ้าที่แปลว่าบันทึกไว้แล้วให้จ่ายได้เลย
void displayOfflineRejected(const String &uid, bool alreadyUsed) {
  wakeScreenIfNeeded();
  if (alreadyUsed) ledDuplicate(); else ledRejected();

  const uint16_t screenBg = alreadyUsed ? 0x2960 : 0x3000;
  const uint16_t cardBg   = alreadyUsed ? 0x4140 : 0x5000;
  const uint16_t banner   = alreadyUsed ? ST77XX_ORANGE : 0xF800;
  const uint16_t muted    = alreadyUsed ? 0xFDC0 : 0xFCAE;

  tft.fillScreen(screenBg);
  tft.fillRect(0, 0, 320, 34, banner);
  drawFitCenteredText(0, 0, 320, 34,
                      alreadyUsed ? "ALREADY USED TODAY" : "CARD NOT IN THE LIST",
                      2, 0x0000, banner);

  tft.fillRoundRect(8, 40, 304, 142, 8, cardBg);
  tft.drawRoundRect(8, 40, 304, 142, 8, banner);
  tft.drawRoundRect(9, 41, 302, 140, 7, banner);

  tft.setTextSize(1);
  tft.setTextColor(muted, cardBg);
  tft.setCursor(20, 50);  tft.print("SERVICE STATION:");
  tft.setCursor(156, 50); tft.print("CARD UID (ENCRYPTED):");

  tft.setTextSize(2);
  tft.setTextColor(0xFFFF, cardBg);
  tft.setCursor(20, 62);  tft.printf("STATION 0%d", currentStationId);
  tft.setCursor(156, 62); tft.print(maskUID(uid));

  tft.drawFastHLine(20, 86, 280, banner);

  tft.setTextSize(1);
  tft.setTextColor(muted, cardBg);
  tft.setCursor(20, 94);
  tft.print("OFFLINE ELIGIBILITY CHECK:");

  tft.setTextSize(3);
  tft.setTextColor(0xFFFF, cardBg);
  tft.setCursor(20, 108);
  tft.print("REJECTED");

  tft.setTextSize(1);
  tft.setTextColor(muted, cardBg);
  tft.setCursor(20, 146);
  tft.print(alreadyUsed ? "THIS CARD ALREADY CLAIMED TODAY"
                        : "THIS CARD IS NOT REGISTERED");
  tft.setCursor(20, 162);
  tft.print("NOTHING WAS RECORDED ON THIS DEVICE");

  tft.fillRoundRect(16, 188, 288, 30, 6, banner);
  drawFitCenteredText(16, 188, 288, 30, "DO NOT SERVE THE STUDENT", 1, 0x0000, banner);

  drawStationBottomBar("OFFLINE MODE | CHECKED AGAINST SAVED LIST");
  if (alreadyUsed) soundAlarm(); else soundError();
}

void displayOfflineAlert() {
  ledOffline();
  tft.fillScreen(0x8000);

  drawStationCard(10, 16, 300, 208, 0xF800, 0x4800);
  tft.fillRoundRect(24, 28, 272, 24, 3, 0xF800);
  drawFitCenteredText(24, 28, 272, 24, "GATEWAY OFFLINE LINK", 1, 0xFFFF, 0xF800);

  tft.setTextColor(0xFCAE, 0x4800);
  tft.setTextSize(1);
  tft.setCursor(24, 68);
  tft.println("COMMUNICATION ERROR:");
  tft.setTextColor(0xF800, 0x4800);
  tft.setTextSize(2);
  tft.setCursor(24, 84);
  tft.println("NO HOST RESPONSE");

  tft.setTextColor(0xFCAE, 0x4800);
  tft.setTextSize(1);
  tft.setCursor(24, 124);
  tft.println("ACTION REQUIRED:");
  tft.setTextColor(0xFFFF, 0x4800);
  tft.setTextSize(1);
  tft.setCursor(24, 142);
  tft.println("Please check Central Host Server (Ch 1)");

  drawStationBottomBar("PRESS BUTTON TO RETRY CONNECTION");
  soundError();
}

void updateShopLabel() {
  snprintf(dynamicShopLabel, sizeof(dynamicShopLabel), "STATION 0%d", currentStationId);
}

// ปิดบังรหัสนิสิตให้เหลือห้าหลักแรก ใช้กฎเดียวกับจอสาธารณะของแม่ข่าย
String maskStudentId(const String &id) {
  if (id.length() == 0 || id == "-") return "-";
  if (id.length() <= 5) return id;
  String out = id.substring(0, 5);
  for (size_t i = 5; i < id.length(); i++) out += '*';
  return out;
}

void saveOfflineQueue() {
  stationPrefs.begin("st_offq", false);
  stationPrefs.putUChar("n", offlineCount);
  if (offlineCount > 0) stationPrefs.putBytes("q", offlineQueue, offlineCount * sizeof(OfflineTap));
  else stationPrefs.remove("q");
  stationPrefs.end();
}

void loadOfflineQueue() {
  stationPrefs.begin("st_offq", true);
  offlineCount = stationPrefs.getUChar("n", 0);
  if (offlineCount > OFFLINE_QUEUE_MAX) offlineCount = 0;
  if (offlineCount > 0) {
    size_t need = offlineCount * sizeof(OfflineTap);
    if (stationPrefs.getBytesLength("q") == need) stationPrefs.getBytes("q", offlineQueue, need);
    else offlineCount = 0;   // ข้อมูลไม่ครบ ทิ้งทั้งคิวดีกว่าส่งของเสียเข้าระบบ
  }
  stationPrefs.end();
}

int findOfflineTap(const String &uid) {
  for (int i = 0; i < offlineCount; i++) {
    if (uid.equals(offlineQueue[i].uid)) return i;
  }
  return -1;
}

bool enqueueOfflineTap(const String &uid) {
  if (offlineCount >= OFFLINE_QUEUE_MAX) return false;
  OfflineTap &e = offlineQueue[offlineCount];
  strncpy(e.uid, uid.c_str(), sizeof(e.uid) - 1);
  e.uid[sizeof(e.uid) - 1] = '\0';
  String t = getFullTimeStr();
  strncpy(e.time, t.c_str(), sizeof(e.time) - 1);
  e.time[sizeof(e.time) - 1] = '\0';
  offlineCount++;
  saveOfflineQueue();
  return true;
}

void popOfflineTap() {
  if (offlineCount == 0) return;
  for (int i = 1; i < offlineCount; i++) offlineQueue[i - 1] = offlineQueue[i];
  offlineCount--;
  saveOfflineQueue();
}


// ---------------------------------------------------------------------------
// บัญชีสิทธิ์ย่อ
// ---------------------------------------------------------------------------

// FNV-1a 32 บิต ต้องให้ผลเท่ากันเป๊ะกับฝั่งแม่ข่าย ห้ามแก้ข้างเดียว
uint32_t uidHash32(const String &uid) {
  uint32_t h = 2166136261UL;
  for (unsigned int i = 0; i < uid.length(); i++) {
    h ^= (uint8_t)uid[i];
    h *= 16777619UL;
  }
  return h;
}

// วันที่ของวันนี้ตามนาฬิกาที่ซิงค์มาจากแม่ข่าย คืน 0 เมื่อยังไม่เคยซิงค์
uint32_t stationTodayYmd() {
  struct tm timeinfo;
  if (!isTimeSynced || !getLocalTime(&timeinfo)) return 0;
  return (uint32_t)(timeinfo.tm_year + 1900) * 10000UL +
         (uint32_t)(timeinfo.tm_mon + 1) * 100UL + (uint32_t)timeinfo.tm_mday;
}

RosterVerdict rosterLookup(const String &uid) {
  // ไม่มั่นใจเมื่อไร ปล่อยผ่านเมื่อนั้น
  if (rosterVer == 0 || rosterBuilding || rosterTruncated) return ROSTER_UNKNOWN;
  if (uid.length() == 0) return ROSTER_UNKNOWN;

  uint32_t h = uidHash32(uid);
  uint16_t n = rosterCount;
  for (uint16_t i = 0; i < n; i++) {
    if (stationRoster[i].hash == h) {
      if (!stationRoster[i].state) return ROSTER_ELIGIBLE;

      // บัญชีที่ค้างมาจากเมื่อวาน (เช่น เปิดเครื่องตอนเช้าแล้วแม่ข่ายยังไม่ขึ้น)
      // ยังบอกได้ว่าบัตรใบไหน "อยู่ในทะเบียน" เพราะทะเบียนไม่ได้เปลี่ยนรายวัน
      // แต่สถานะ "ใช้สิทธิ์แล้ว" ของเมื่อวานใช้ตัดสินวันนี้ไม่ได้ ต้องถือว่ายังไม่ใช้
      // ไม่งั้นนิสิตที่กินเมื่อวานจะถูกปฏิเสธทั้งหมดในเช้าวันถัดไป
      uint32_t today = stationTodayYmd();
      if (today == 0 || rosterDate == 0 || rosterDate != today) return ROSTER_ELIGIBLE;
      return ROSTER_CLAIMED;
    }
  }
  return ROSTER_NOT_FOUND;
}

// ทำเครื่องหมายว่าบัตรใบนี้ใช้สิทธิ์ไปแล้วในบัญชีของเครื่องนี้เอง
// ใช้ตอนรับการแตะเข้าคิวออฟไลน์สำเร็จ เพื่อให้การแตะซ้ำถูกปัดตกทันที
void rosterMarkClaimedLocal(const String &uid) {
  if (rosterVer == 0 || rosterBuilding) return;
  uint32_t h = uidHash32(uid);
  uint16_t n = rosterCount;
  for (uint16_t i = 0; i < n; i++) {
    if (stationRoster[i].hash == h) {
      if (stationRoster[i].state != 1) {
        stationRoster[i].state = 1;
        rosterDirty = true;
      }
      return;
    }
  }
}

void loadRoster() {
  stationPrefs.begin("st_rost", true);
  uint16_t ver = stationPrefs.getUShort("v", 0);
  uint16_t n   = stationPrefs.getUShort("n", 0);
  uint32_t dt  = stationPrefs.getULong("dt", 0);
  if (ver != 0 && n > 0 && n <= ROSTER_MAX) {
    size_t need = (size_t)n * sizeof(RosterEntry);
    if (stationPrefs.getBytesLength("d") == need) {
      stationPrefs.getBytes("d", stationRoster, need);
      rosterCount = n;
      rosterVer = ver;
      rosterDate = dt;
    }
  }
  stationPrefs.end();
}

// เขียนลง NVS แบบหน่วงเวลา บัญชีเปลี่ยนบ่อยมาก (ทุกครั้งที่มีคนใช้สิทธิ์)
// ถ้าเขียนทุกครั้งจะกินอายุแฟลชโดยไม่จำเป็น เพราะข้อมูลนี้เป็นแค่สำเนาไว้กันรีบูต
void saveRosterIfDue(bool force) {
  if (!rosterDirty) return;
  if (!force && (long)(millis() - rosterSaveAt) < 0) return;
  if (rosterBuilding) return;

  uint16_t n = rosterCount;
  uint16_t v = rosterVer;
  stationPrefs.begin("st_rost", false);
  if (v == 0 || n == 0) {
    stationPrefs.remove("d");
    stationPrefs.putUShort("v", 0);
    stationPrefs.putUShort("n", 0);
  } else {
    stationPrefs.putBytes("d", stationRoster, (size_t)n * sizeof(RosterEntry));
    stationPrefs.putUShort("n", n);
    stationPrefs.putUShort("v", v);
    stationPrefs.putULong("dt", rosterDate);
  }
  stationPrefs.end();

  rosterDirty = false;
  rosterSaveAt = millis() + ROSTER_SAVE_GAP_MS;
}

// ฝากเลขรุ่นบัญชีที่เครื่องนี้ถืออยู่ไปกับ heartbeat ผ่านช่อง offlineTime ที่ว่างอยู่
// แม่ข่ายเทียบกับเลขรุ่นของตัวเอง ถ้าไม่ตรงจะผลักบัญชีชุดเต็มมาให้ใหม่ภายในไม่กี่วินาที
// เติม '!' ต่อท้ายเมื่อบัญชีใหญ่เกินที่เครื่องนี้เก็บไหว แม่ข่ายจะได้เอาไปขึ้นเตือนบนแดชบอร์ด
// แทนที่จะรายงานรุ่น 0 ไปเรื่อย ๆ ซึ่งจะทำให้แม่ข่ายผลักบัญชีชุดเต็มมาใหม่ทุก heartbeat ไม่จบ
void stampRosterVer(StationPacket &pkt) {
  snprintf(pkt.offlineTime, sizeof(pkt.offlineTime), "R:%u%s",
           (unsigned)rosterVer, rosterTruncated ? "!" : "");
}

// เรียกจากคอลแบ็ก ESP-NOW เท่านั้น งานทั้งหมดเป็น memcpy สั้น ๆ ไม่มีการเขียนแฟลช
void rosterApplyPacket(const HostRosterPacket &r) {
  if (r.flags & ROSTER_FLAG_DELTA) {
    // รับก็ต่อเมื่อเลขรุ่นต่อกันพอดี ถ้าพลาดไปก้อนหนึ่งแปลว่าบัญชีที่ถืออยู่ไม่ครบแล้ว
    // ทิ้งทั้งบัญชีแล้วกลับไปปล่อยผ่านชั่วคราว ดีกว่าตัดสินด้วยข้อมูลที่รู้ว่าผิด
    // heartbeat รอบถัดไปจะรายงานรุ่น 0 แล้วแม่ข่ายจะผลักชุดเต็มมาทับให้เอง
    if (rosterVer == 0 || r.rosterVer != (uint16_t)(rosterVer + 1)) {
      if (rosterVer != 0) { rosterVer = 0; rosterDirty = true; }
      return;
    }
    if (r.entryCount >= 1) {
      uint32_t h = r.entries[0].hash;
      uint16_t n = rosterCount;
      for (uint16_t i = 0; i < n; i++) {
        if (stationRoster[i].hash == h) { stationRoster[i].state = r.entries[0].state; break; }
      }
    }
    rosterVer = r.rosterVer;
    rosterDate = r.rosterDate;
    rosterDirty = true;
    return;
  }

  // ชุดเต็ม: ก้อนแรกตั้งต้นใหม่ ก้อนถัดไปต้องมาตามลำดับ ไม่งั้นทิ้งแล้วรอรอบใหม่
  if (r.flags & ROSTER_FLAG_FULL_BEGIN) {
    rosterBuilding  = true;
    rosterFillNext  = 0;
    rosterNextChunk = 0;
    rosterTruncated = (r.totalEntries > ROSTER_MAX);
  } else if (!rosterBuilding || r.chunkIndex != rosterNextChunk) {
    return;
  }

  if (r.chunkIndex != rosterNextChunk) return;

  uint8_t n = r.entryCount;
  if (n > ROSTER_ENTRIES_PER_PKT) n = ROSTER_ENTRIES_PER_PKT;
  for (uint8_t k = 0; k < n; k++) {
    if (rosterFillNext >= ROSTER_MAX) { rosterTruncated = true; break; }
    stationRoster[rosterFillNext++] = r.entries[k];
  }
  rosterNextChunk++;

  if (r.flags & ROSTER_FLAG_FULL_END) {
    rosterCount    = rosterFillNext;
    rosterVer      = r.rosterVer;   // รับรุ่นไว้เสมอ ไม่งั้นแม่ข่ายจะผลักชุดเต็มมาซ้ำไม่จบ
    rosterDate     = r.rosterDate;  // เก็บไม่ครบให้กันไว้ที่ rosterTruncated แทน
    rosterBuilding = false;
    rosterDirty    = true;
  }
}

void pushStationTap(const char *studentId) {
  StationTap &e = stnFeed[stnFeedHead];
  String masked = maskStudentId(String(studentId));
  strncpy(e.id, masked.c_str(), sizeof(e.id) - 1);
  e.id[sizeof(e.id) - 1] = '\0';
  String t = getTimeOnlyStr();
  strncpy(e.time, t.c_str(), sizeof(e.time) - 1);
  e.time[sizeof(e.time) - 1] = '\0';
  strncpy(lastPayoutTime, e.time, sizeof(lastPayoutTime) - 1);
  lastPayoutTime[sizeof(lastPayoutTime) - 1] = '\0';
  stnFeedHead = (stnFeedHead + 1) % STN_FEED_SIZE;
  if (stnFeedCount < STN_FEED_SIZE) stnFeedCount++;
}

// แกะสรุปภาพรวมที่แม่ข่ายฝากมา รูปแบบ "268/412/1/1000-1330"
// ถ้าแม่ข่ายเป็นเฟิร์มแวร์รุ่นเก่าช่องนี้จะว่าง แกะไม่ผ่านแล้วหน้าจอจะแสดงขีดแทน
void applySystemSummary(const char *msg) {
  int u = 0, t = 0, open = 0, w1 = 0, w2 = 0;
  if (sscanf(msg, "%d/%d/%d/%d-%d", &u, &t, &open, &w1, &w2) == 5) {
    sysUsed = (uint16_t)u;
    sysTotal = (uint16_t)t;
    sysServiceOpen = (open != 0);
    snprintf(sysWindow, sizeof(sysWindow), "%02d:%02d-%02d:%02d", w1 / 100, w1 % 100, w2 / 100, w2 % 100);
    hasSystemInfo = true;
  }
}

String maskUID(String uid) {
  if (uid.length() < 4 || uid == "-") return uid;
  return uid.substring(0, uid.length() - 4) + "****";
}

bool isValidMac(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) if (mac[i] != 0x00 && mac[i] != 0xFF) return true;
  return false;
}

bool ensureHostPeer() {
  if (!isValidMac(hostMacAddress)) return false;
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, hostMacAddress, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;
  if (esp_now_is_peer_exist(hostMacAddress)) { hostPeerReady = true; return true; }
  esp_err_t err = esp_now_add_peer(&peerInfo);
  if (err == ESP_OK || err == ESP_ERR_ESPNOW_EXIST) {
    hostPeerReady = true;
    applyEspNowRate(hostMacAddress);
    return true;
  }
  hostPeerReady = false;
  return false;
}

// เดิมถ้า unicast ล้มเหลวจะยิงเป็น broadcast แทน ซึ่งทำให้แย่ลงไม่ใช่ดีขึ้น
// เพราะ broadcast ไม่มี ACK ระดับ MAC จึงไม่มีการส่งซ้ำอัตโนมัติของชิป
// ขณะที่ unicast มี ARQ ในตัว ตอนนี้ใช้ broadcast เฉพาะตอนยังไม่รู้จัก MAC ของแม่ข่าย
bool sendToHost(const uint8_t *data, size_t len) {
  if (ensureHostPeer()) {
    return esp_now_send(hostMacAddress, data, len) == ESP_OK;
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

int calculateSignalQuality(int rssi) {
  if (rssi <= -100) return 0;
  if (rssi >= -50) return 100;
  return 2 * (rssi + 100);
}

uint16_t getSignalColor(int rssi, bool isOnline) {
  if (!isOnline) return getStRose();
  int q = calculateSignalQuality(rssi);
  if (q >= 60) return getStGreen();
  if (q >= 35) return getStYellow();
  return getStRose();
}

void drawSignalBars(int x, int y, int rssi, bool isOnline, uint16_t bg) {
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
      tft.drawRect(barX, barY, 3, barH, getStCardBorder());
    }
  }
}

// แถบสถานะมุมขวาบน: ความแรงสัญญาณ ESP-NOW + ระดับแบตเตอรี่
// เดิมฟังก์ชันนี้เขียนไว้แต่ไม่เคยถูกเรียก ทำให้สถานี "ไม่เคย" แสดงสถานะลิงก์เลย
void updateStationHeaderStatus(bool force = false) {
  static int lastDrawnRssi = -999;
  static bool lastDrawnOnline = false;
  static int lastBars = -1;

  int rssi = lastHostRssi;
  bool online = isHostOnline;

  int currentBars = 0;
  int q = calculateSignalQuality(rssi);
  if (online) {
    if (q >= 85) currentBars = 4;
    else if (q >= 60) currentBars = 3;
    else if (q >= 35) currentBars = 2;
    else currentBars = 1;
  }

  bool signalChanged = force || (online != lastDrawnOnline) || (currentBars != lastBars) ||
                       (abs(rssi - lastDrawnRssi) >= 3);

  if (signalChanged) {
    lastDrawnRssi = rssi;
    lastDrawnOnline = online;
    lastBars = currentBars;

    tft.fillRect(190, 3, 66, 18, getStBg());
    tft.setTextSize(1);
    if (online) {
      tft.setTextColor(getStTextMuted(), getStBg());
      tft.setCursor(194, 8);
      tft.printf("%ddB", rssi);
      drawSignalBars(228, 6, rssi, true, getStBg());
    } else {
      tft.setTextColor(getStRose(), getStBg());
      tft.setCursor(192, 8);
      tft.print("OFFLINE");
      drawSignalBars(228, 6, -100, false, getStBg());
    }
  }

  drawStationBatteryHUD(260, 3, force);
}

void syncInternalClock(const char* timeStr) {
  int year, month, day, hour, minute, second;
  if (sscanf(timeStr, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
    struct tm tmInfo;
    tmInfo.tm_year = year - 1900; tmInfo.tm_mon  = month - 1; tmInfo.tm_mday = day;
    tmInfo.tm_hour = hour; tmInfo.tm_min  = minute; tmInfo.tm_sec  = second; tmInfo.tm_isdst = 0;
    time_t epoch = mktime(&tmInfo);
    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    isTimeSynced = true;
  }
}

String getTimeOnlyStr() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo) || !isTimeSynced) {
    unsigned long sec = millis() / 1000;
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", (sec / 3600) % 24, (sec / 60) % 60, sec % 60);
    return String(buffer);
  }
  char buffer[16];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  return String(buffer);
}

// เวลาเต็มรูปแบบสำหรับบันทึกลงคิวออฟไลน์ คืนสตริงว่างถ้ายังไม่เคยซิงค์เวลากับแม่ข่าย
// แล้วแม่ข่ายจะใช้เวลาของตัวเองตอนรับรายการแทน
String getFullTimeStr() {
  struct tm timeinfo;
  if (!isTimeSynced || !getLocalTime(&timeinfo)) return String("");
  char buffer[24];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

String getDateFormattedStr() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo) || !isTimeSynced) return "SUN, 14 SEP 2026";
  const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  char buffer[40];
  snprintf(buffer, sizeof(buffer), "%s, %02d %s %04d",
           days[timeinfo.tm_wday], timeinfo.tm_mday, months[timeinfo.tm_mon], timeinfo.tm_year + 1900);
  return String(buffer);
}

void setScreenPower(bool powerOn) {
  isScreenOn = powerOn;
  digitalWrite(TFT_BLK, isScreenOn ? HIGH : LOW);
}

void wakeScreenIfNeeded() {
  if (!isScreenOn) setScreenPower(true);
}

void playBootAnimation() {
  setLedColor(0, 30, 60);
  tft.fillScreen(getStBg());
  int centerX = 160; int centerY = 110;
  for (int r = 10; r <= 85; r += 9) {
    tft.drawCircle(centerX, centerY, r, getStCyan());
    tft.drawCircle(centerX, centerY, r + 2, getStGreen());
    delay(20);
  }
  tft.fillScreen(getStBg());
  tft.drawRect(2, 2, 316, 236, getStCardBorder());
  tft.drawRect(4, 4, 312, 232, getStCyan());
  
  tft.setTextColor(getStYellow(), getStBg());
  tft.setTextSize(2);
  tft.setCursor(22, 30);
  tft.println("MCU PHRAE SMART CANTEEN");
  
  tft.setTextColor(getStTextMain(), getStBg());
  tft.setTextSize(1);
  tft.setCursor(44, 60);
  tft.println("STATION TERMINAL CLIENT SYSTEM");

  drawStationCard(50, 95, 220, 48, getStGreen(), getStCardBg());
  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(2);
  tft.setCursor(85, 110);
  tft.printf("STATION 0%d", currentStationId);

  tft.setTextColor(getStGreen(), getStBg());
  tft.setTextSize(1);
  tft.setCursor(68, 175);
  tft.println("ESP-NOW Radio: Locked CH 1 (Stable)");

  soundWelcome();
  ledStandby();
  delay(300);
}

void renderDeveloperCredit() {
  tft.fillScreen(getStBg());
  drawStationTopBar("SYSTEM ARCHITECTURE & CREDITS");

  drawStationCard(10, 36, 300, 166, getStCyan(), getStCardBg());

  tft.setTextColor(getStCyan(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(22, 48);
  tft.println("SYSTEM DEVELOPER:");

  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(2);
  tft.setCursor(22, 64);
  tft.println(DEV_NAME);

  tft.setTextColor(getStYellow(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(22, 92);
  tft.println(DEV_ROLE);

  tft.setTextColor(getStGreen(), getStCardBg());
  tft.setCursor(22, 110);
  tft.println("Mahachulalongkornrajavidyalaya Phrae");

  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setCursor(22, 134); tft.printf("Firmware: v%s (Bento Edition)\n", APP_VERSION);
  tft.setCursor(22, 150); tft.printf("Hardware: ESP32-S3 + ST7789V + RC522\n");
  tft.setCursor(22, 166); tft.println("Display : 2.8\" ST7789V 320x240 Modular Bento");

  drawStationBottomBar("PRESS BUTTON TO RETURN TO DASHBOARD");
}

void displayTapCardStandby() {
  ledStandby();
  tft.fillScreen(getStBg());

  drawStationTopBar(String(dynamicShopLabel) + " TERMINAL");

  // การ์ดบน: ป้ายสถานะ สัญลักษณ์แตะบัตร และคำสั่งหลัก
  drawStationCard(10, 30, 300, 116, getStGreen(), getStCardBg());
  drawStationPillBadge(26, 36, 268, 18, "READY FOR RFID CARD TAP",
                       isStationDarkMode ? 0x0000 : 0xFFFF, getStGreen());

  drawRfidTapIcon(160, 82, getStTextMain(), getStGreen(), getStCardBg());

  // "TAP CARD HERE" ขนาด 3 กว้าง 234px จัดกึ่งกลางจอ 320px
  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(3);
  tft.setCursor(43, 114);
  tft.print("TAP CARD HERE");

  // การ์ดล่าง: สิทธิ์ต่อวัน ยกตัวเลขขึ้นมาเป็นขนาด 3 ให้อ่านได้จากระยะไกล
  drawStationCard(10, 152, 300, 46, getStYellow(), getStCardBg());

  // "SUBSIDY QUOTA" ขนาด 1 กว้าง 78px จัดกึ่งกลางการ์ดกว้าง 300px
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(121, 158);
  tft.print("SUBSIDY QUOTA");

  // ตัวเลขสิทธิ์ ขนาด 3 กว้าง 216px จัดกึ่งกลางเช่นกัน
  tft.setTextColor(getStYellow(), getStCardBg());
  tft.setTextSize(3);
  tft.setCursor(52, 170);
  tft.print("35 THB / DAY");

  // บรรทัดล่างสุด: ปกติบอกโปรโตคอล แต่ถ้ามีรายการค้างจะเตือนว่ากำลังทำงานแบบออฟไลน์
  if (offlineCount > 0) {
    char offBuf[52];
    snprintf(offBuf, sizeof(offBuf), "OFFLINE MODE - %u RECORDS WAITING TO SYNC",
             (unsigned)offlineCount);
    drawFitCenteredText(0, 200, 320, 12, offBuf, 1, getStYellow(), getStBg());
  } else {
    drawFitCenteredText(0, 200, 320, 12, "Protocol: ESP-NOW Channel 1 Secured", 1,
                        getStTextMuted(), getStBg());
  }

  drawStationBottomBar("PAGE 1/3 | PRESS BUTTON TO CYCLE");
}

// หน้า 2: ยอดของร้านนี้ ภาพรวมทั้งโรงอาหาร และรายการที่จ่ายล่าสุด
// แยกส่วนคงที่กับส่วนที่เปลี่ยนค่า เพื่อให้รีเฟรชทุก heartbeat ได้โดยจอไม่กะพริบ
void displayStatsDashboard(bool fullRedraw) {
  ledStandby();

  if (fullRedraw) {
    tft.fillScreen(getStBg());
    drawStationTopBar(String(dynamicShopLabel) + " STATS");

    drawStationCard(6, 30, 150, 112, getStGreen(), getStCardBg());
    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.setTextSize(1);
    tft.setCursor(14, 38);
    tft.print("THIS STATION");
    tft.setCursor(14, 104);
    tft.print("TOTAL PAYOUT");

    drawStationCard(164, 30, 150, 112, getStCardBorder(), getStCardBg());
    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.setCursor(172, 38);
    tft.print("WHOLE CANTEEN");

    drawStationCard(6, 148, 308, 66, getStCardBorder(), getStCardBg());
    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.setCursor(16, 152);
    tft.print("LAST PAYOUTS AT THIS STATION");

    drawStationBottomBar("PAGE 2/3 | PRESS BUTTON TO CYCLE");
  }

  // ---- ยอดของร้านนี้ ----
  tft.fillRect(12, 48, 138, 34, getStCardBg());
  tft.setTextColor(getStGreen(), getStCardBg());
  tft.setTextSize(3);
  tft.setCursor(14, 52);
  tft.printf("%u", (unsigned)totalSuccessToday);
  tft.setTextSize(1);
  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.print(" pax");

  tft.fillRect(12, 82, 138, 18, getStCardBg());
  tft.setTextColor(getStYellow(), getStCardBg());
  tft.setTextSize(2);
  tft.setCursor(14, 84);
  tft.printf("%u B.", (unsigned)(totalSuccessToday * 35));

  tft.fillRect(12, 116, 138, 12, getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(14, 118);
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.print("FAIL ");
  tft.setTextColor(totalRejectToday > 0 ? getStRose() : getStTextMuted(), getStCardBg());
  tft.printf("%u", (unsigned)totalRejectToday);
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.print("  LAST ");
  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.print(lastPayoutTime);

  // ---- ภาพรวมทั้งโรงอาหาร (แม่ข่ายฝากมากับ heartbeat) ----
  tft.fillRect(170, 48, 138, 34, getStCardBg());
  if (hasSystemInfo) {
    tft.setTextColor(getStCyan(), getStCardBg());
    tft.setTextSize(3);
    tft.setCursor(172, 52);
    tft.printf("%u", (unsigned)sysUsed);
    tft.setTextSize(1);
    tft.setTextColor(getStTextMain(), getStCardBg());
    tft.printf(" /%u", (unsigned)sysTotal);
  } else {
    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.setTextSize(2);
    tft.setCursor(172, 58);
    tft.print("-- / --");
  }

  int pct = (hasSystemInfo && sysTotal > 0) ? (int)((uint32_t)sysUsed * 100 / sysTotal) : 0;
  if (pct > 100) pct = 100;
  tft.fillRect(172, 86, 134, 8, getStCardBg());
  tft.drawRoundRect(172, 86, 134, 8, 3, getStCardBorder());
  int fillW = (pct * 130) / 100;
  if (fillW > 0) tft.fillRoundRect(174, 88, fillW, 4, 2, getStCyan());

  tft.fillRect(170, 98, 138, 12, getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(172, 100);
  if (!hasSystemInfo) {
    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.print("WAITING FOR HOST");
  } else if (sysServiceOpen) {
    tft.setTextColor(getStGreen(), getStCardBg());
    tft.printf("OPEN %s", sysWindow);
  } else {
    tft.setTextColor(getStRose(), getStCardBg());
    tft.printf("CLOSED %s", sysWindow);
  }

  tft.fillRect(170, 114, 138, 12, getStCardBg());
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setCursor(172, 116);
  if (hasSystemInfo) tft.printf("%d%% OF ELIGIBLE", pct);
  else tft.print("SYNCING...");

  // ---- รายการที่จ่ายล่าสุดของสถานีนี้ ----
  tft.fillRect(12, 162, 296, 50, getStCardBg());
  if (stnFeedCount == 0) {
    tft.setTextSize(1);
    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.setCursor(103, 182);
    tft.print("NO PAYOUT YET TODAY");
  } else {
    for (int i = 0; i < stnFeedCount; i++) {
      int idx = (stnFeedHead - 1 - i + STN_FEED_SIZE * 2) % STN_FEED_SIZE;
      int y = 164 + i * 16;
      tft.setTextSize(1);
      tft.setTextColor(getStTextMuted(), getStCardBg());
      tft.setCursor(16, y + 4);
      tft.print(stnFeed[idx].time);

      tft.setTextSize(2);
      tft.setTextColor(getStTextMain(), getStCardBg());
      tft.setCursor(74, y);
      tft.print(stnFeed[idx].id);

      tft.setTextSize(1);
      tft.setTextColor(getStYellow(), getStCardBg());
      tft.setCursor(262, y + 4);
      tft.print("35 B.");
    }
  }
}

void displayScanningUID(String uid) {
  wakeScreenIfNeeded();
  soundClick();
  setLedColor(30, 30, 30);
  tft.fillScreen(getStBg());

  drawStationTopBar("PROCESSING CARD TAP");

  drawStationCard(10, 36, 300, 168, getStCyan(), getStCardBg());
  drawStationPillBadge(24, 48, 272, 22, "RFID CARD DETECTED", isStationDarkMode ? 0x0000 : 0xFFFF, getStCyan());

  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(24, 84);
  tft.println("ENCRYPTED CARD UID:");

  tft.setTextColor(getStCyan(), getStCardBg());
  tft.setTextSize(2);
  tft.setCursor(24, 104);
  tft.println(maskUID(uid));

  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(24, 148);
  tft.println("Transmitting payload to Central Host...");

  drawStationBottomBar("PLEASE WAIT FOR VERIFICATION");
}

void renderScreensaver(bool fullRedraw) {
  int usedCount = totalSuccessToday;
  static String lastStationClock = "";

  if (fullRedraw) {
    ledOff();
    tft.fillScreen(getStBg());
    lastStationClock = "";
    drawStationTopBar("SYSTEM STANDBY");

    drawStationCard(20, 36, 280, 162, getStCardBorder(), getStCardBg());

    String dateStr = getDateFormattedStr();
    int xDate = max(24, (320 - (int)dateStr.length() * 12) / 2);
    tft.setTextColor(getStCyan(), getStCardBg());
    tft.setTextSize(2);
    tft.setCursor(xDate, 108);
    tft.print(dateStr);

    char statBuf[48];
    snprintf(statBuf, sizeof(statBuf), "SERVED: %3d STUDENTS (%5d THB)", (int)usedCount, (int)usedCount * 35);
    int statLen = strlen(statBuf) * 6;
    int posX = max(24, (320 - statLen) / 2);
    tft.setTextColor(getStGreen(), getStCardBg());
    tft.setTextSize(1);
    tft.setCursor(posX, 142);
    tft.print(statBuf);

    float hVolt = readBatteryVoltage();
    char statBuf2[48];
    snprintf(statBuf2, sizeof(statBuf2), "BATT: %d%% | CORE: %.1fC | HEAP: %uKB",
             getBatteryPercentage(hVolt), getChipTemperature(), (unsigned)(ESP.getFreeHeap() / 1024));
    int statLen2 = strlen(statBuf2) * 6;
    int posX2 = max(24, (320 - statLen2) / 2);
    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.setCursor(posX2, 162);
    tft.print(statBuf2);

    drawStationBottomBar("TAP RFID CARD OR PRESS BUTTON TO WAKE");
  }

  String curTime = getTimeOnlyStr();
  if (fullRedraw || curTime != lastStationClock) {
    lastStationClock = curTime;
    tft.fillRect(60, 50, 200, 38, getStCardBg());
    tft.setTextColor(getStTextMain(), getStCardBg());
    tft.setTextSize(4);
    tft.setCursor(64, 54);
    tft.print(curTime);
  }
}

void displayStatusScreen(bool fullRedraw) {
  if (fullRedraw) {
    ledStandby();
    tft.fillScreen(getStBg());
    drawStationTopBar("DIAGNOSTICS & HARDWARE");

    drawStationCard(6, 30, 308, 172, getStCardBorder(), getStCardBg());

    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.setTextSize(1);
    tft.setCursor(16, 42);  tft.println("HOST LINK STATUS :");
    tft.setCursor(16, 64);  tft.println("STATION IDENTIFIER:");
    tft.setCursor(16, 86);  tft.println("CORE TEMPERATURE :");
    tft.setCursor(16, 108); tft.println("BATTERY VOLTAGE  :");
    tft.setCursor(16, 130); tft.println("TODAY SERVED     :");
    tft.setCursor(16, 152); tft.println("STATION MAC      :");
    tft.setCursor(16, 174); tft.println("OFFLINE CARD LIST:");

    tft.setTextColor(getStCyan(), getStCardBg());
    tft.setCursor(140, 64);  tft.printf("STATION 0%d (Active)", currentStationId);

    tft.setTextColor(getStTextMain(), getStCardBg());
    tft.setCursor(140, 152); tft.println(WiFi.macAddress());

    drawStationBottomBar("PAGE 3/3 | PRESS BUTTON TO CYCLE");
  }

  float chipT = getChipTemperature();
  float cpuL = calculateCpuLoad();

  bool updateDynamic = fullRedraw ||
                       (millis() - lastCpuDisplayUpdate >= 500) ||
                       fabs(chipT - lastDisplayedCpuTemperature) >= 0.1f ||
                       fabs(cpuL - lastDisplayedCpuLoad) >= 1.0f;
  if (updateDynamic) {
    lastCpuDisplayUpdate = millis();
    lastDisplayedCpuTemperature = chipT;
    lastDisplayedCpuLoad = cpuL;

    tft.fillRect(140, 40, 166, 14, getStCardBg());
    tft.setTextSize(1);
    tft.setCursor(140, 42);
    if (isHostOnline) {
      tft.setTextColor(getStGreen(), getStCardBg());
      tft.printf("ONLINE (CH 1 | %ddB)", lastHostRssi);
    } else {
      tft.setTextColor(getStRose(), getStCardBg());
      tft.print("OFFLINE (No Gateway)");
    }

    tft.fillRect(140, 84, 166, 14, getStCardBg());
    tft.setCursor(140, 86);
    tft.setTextColor((chipT < 65.0) ? getStGreen() : getStYellow(), getStCardBg());
    tft.printf("%.1f C (CPU: %.0f%%)", chipT, cpuL);

    // เดิมหน้านี้มีหัวข้อ BATTERY VOLTAGE / TODAY SERVED แต่ไม่เคยพิมพ์ค่าออกมา
    float volt = readBatteryVoltage();
    int pct = getBatteryPercentage(volt);
    tft.fillRect(140, 106, 166, 14, getStCardBg());
    tft.setCursor(140, 108);
    tft.setTextColor((pct > 20) ? getStGreen() : getStRose(), getStCardBg());
    tft.printf("%.2f V (%d%%)", volt, pct);

    tft.fillRect(140, 128, 166, 14, getStCardBg());
    tft.setCursor(140, 130);
    tft.setTextColor(getStTextMain(), getStCardBg());
    tft.printf("%u pax = %u THB", (unsigned)totalSuccessToday, (unsigned)(totalSuccessToday * 35));

    // บัญชีสิทธิ์ที่แม่ข่ายผลักมาให้ เจ้าหน้าที่ต้องดูออกว่าเครื่องนี้ตรวจบัตรเองได้หรือยัง
    // ถ้าไม่พร้อม แปลว่าตอนลิงก์ขาดเครื่องจะรับบัตรทุกใบไว้ก่อนเหมือนเฟิร์มแวร์รุ่นก่อน
    tft.fillRect(140, 172, 166, 14, getStCardBg());
    tft.setCursor(140, 174);
    if (rosterTruncated) {
      tft.setTextColor(getStRose(), getStCardBg());
      tft.print("TOO BIG - ACCEPT ALL");
    } else if (rosterBuilding) {
      tft.setTextColor(getStYellow(), getStCardBg());
      tft.print("RECEIVING...");
    } else if (rosterVer == 0) {
      tft.setTextColor(getStYellow(), getStCardBg());
      tft.print("NOT LOADED YET");
    } else {
      tft.setTextColor(getStGreen(), getStCardBg());
      tft.printf("READY (%u CARDS)", (unsigned)rosterCount);
    }

    updateStationHeaderStatus(false);
  }
}

// เปลี่ยนหน้าแบบรวมศูนย์ จุดสำคัญคือต้องตั้ง currentState ให้ตรงกับหน้าที่แสดงอยู่
// ของเดิมเปลี่ยนแค่ currentStationPage แต่ไม่เคยตั้ง STATE_STATUS ทำให้หน้า 3 ค้างนิ่ง
void showStationPage(int page, bool fullRedraw) {
  if (page < 1 || page > 3) page = 1;
  currentStationPage = page;
  currentState = (page == 3) ? STATE_STATUS : STATE_STANDBY;
  if (page == 1)      displayTapCardStandby();
  else if (page == 2) displayStatsDashboard(fullRedraw);
  else                displayStatusScreen(fullRedraw);
}

// วาดหน้าปัจจุบันใหม่หลังเปลี่ยนธีม โดยไม่เปลี่ยนสถานะที่ค้างอยู่
void redrawCurrentScreen() {
  if (!isScreenOn) return;
  if (currentState == STATE_SCREENSAVER)   renderScreensaver(true);
  else if (currentState == STATE_CREDIT)   renderDeveloperCredit();
  else if (currentState == STATE_STANDBY ||
           currentState == STATE_STATUS)   showStationPage(currentStationPage, true);
}

// ทำตามคำสั่งโหมดการแสดงผลจากเครื่องแม่ข่าย (MSG_CONFIG)
//
// ธีมถูกบังคับให้ตรงกับแม่ข่ายเสมอ ส่วนไฟหน้าจอและโหมดพักหน้าจอจะทำตาม
// เฉพาะตอนที่ modeSeq เปลี่ยน คือตอนที่เจ้าหน้าที่เปลี่ยนโหมดที่เครื่องแม่ข่ายจริง ๆ
// สถานีจึงยังกดปิดจอเองได้โดยไม่ถูกแม่ข่ายสั่งเปิดกลับทุก 6 วินาที
void applyHostConfig() {
  uint8_t wantDark, wantScreenOn, wantSaver, seq;
  portENTER_CRITICAL(&espnowMux);
  pendingConfigUpdate = false;
  wantDark     = cfgDark;
  wantScreenOn = cfgScreenOn;
  wantSaver    = cfgScreensaver;
  seq          = cfgModeSeq;
  portEXIT_CRITICAL(&espnowMux);

  bool needRedraw = false;

  if ((wantDark != 0) != isStationDarkMode) {
    isStationDarkMode = (wantDark != 0);
    stationPrefs.begin("st_cfg", false);
    stationPrefs.putBool("dark", isStationDarkMode);
    stationPrefs.end();
    if (isScreenOn) soundThemeSwitch();
    needRedraw = true;
  }

  bool newCommand = (!hasAppliedModeSeq || seq != lastAppliedModeSeq);

  // กำลังแสดงผลการแตะบัตรอยู่ อย่าเพิ่งเปลี่ยนโหมด รอให้จอผลลัพธ์หมดเวลาก่อน
  // แล้วค่อยทำตามในรอบถัดไป (ยังไม่จด lastAppliedModeSeq)
  bool busy = (currentState == STATE_SCANNING_SENT ||
               currentState == STATE_RESULT_DISPLAY ||
               currentState == STATE_CONFIG_ID ||
               currentState == STATE_SYNCING);
  if (newCommand && busy) {
    if (needRedraw) redrawCurrentScreen();
    return;
  }

  if (newCommand) {
    hasAppliedModeSeq = true;
    lastAppliedModeSeq = seq;

    if ((wantScreenOn != 0) != isScreenOn) {
      setScreenPower(wantScreenOn != 0);
      if (!isScreenOn) { ledOff(); return; }
      needRedraw = true;
    }

    bool inSaver = (currentState == STATE_SCREENSAVER);
    if ((wantSaver != 0) && !inSaver) {
      currentState = STATE_SCREENSAVER;
      lastActivityTime = millis();
      if (isScreenOn) renderScreensaver(true);
      return;
    }
    if ((wantSaver == 0) && inSaver) {
      lastActivityTime = millis();
      if (isScreenOn) { soundHomeBeep(); showStationPage(1, true); }
      else currentState = STATE_STANDBY;
      return;
    }
  }

  if (needRedraw) redrawCurrentScreen();
}

void sendOfflineTap() {
  if (offlineCount == 0) return;
  memset(&pendingScanPacket, 0, sizeof(pendingScanPacket));
  pendingScanPacket.magic = ESPNOW_PROTO_MAGIC;
  pendingScanPacket.version = ESPNOW_PROTO_VER;
  pendingScanPacket.msgType = MSG_SCAN_REQ;
  pendingScanPacket.stationId = currentStationId;

  if (nextScanSeq == 0) nextScanSeq = 1;
  syncSeq = nextScanSeq++;
  if (nextScanSeq == 0) nextScanSeq = 1;
  pendingScanPacket.seq = syncSeq;

  strncpy(pendingScanPacket.uid, offlineQueue[0].uid, sizeof(pendingScanPacket.uid) - 1);
  strncpy(pendingScanPacket.offlineTime, offlineQueue[0].time, sizeof(pendingScanPacket.offlineTime) - 1);
  pendingScanPacket.systemVoltage = readBatteryVoltage();

  sendToHost((uint8_t *)&pendingScanPacket, sizeof(StationPacket));
  syncSentAt = millis();
}

void startOfflineSync() {
  syncReturnState = (currentState == STATE_SCREENSAVER) ? STATE_SCREENSAVER : STATE_STANDBY;
  syncReturnPage  = currentStationPage;
  syncQuiet       = (currentState == STATE_SCREENSAVER) || !isScreenOn;
  syncTotal       = offlineCount;
  syncDone = 0; syncRejected = 0; syncRetry = 0;
  syncAckReceived = false;
  syncInProgress  = true;
  currentState    = STATE_SYNCING;
  if (!syncQuiet) displaySyncProgress(0, syncTotal);
  sendOfflineTap();
}

void finishOfflineSync(bool aborted) {
  syncInProgress = false;
  lastActivityTime = millis();
  // แม่ข่ายหายไปอีก เก็บคิวที่เหลือไว้แล้วเว้นช่วงก่อนลองใหม่ ไม่วนรัว
  if (aborted) syncRetryNotBefore = millis() + 15000;

  if (syncReturnState == STATE_SCREENSAVER) {
    currentState = STATE_SCREENSAVER;
    if (isScreenOn) renderScreensaver(true);
    return;
  }
  if (!syncQuiet && isScreenOn && !aborted && (syncDone + syncRejected) > 0) {
    displaySyncDone(syncDone, syncRejected);
    delay(2200);
  }
  showStationPage(syncReturnPage, true);
}

// ธีมถูกกำหนดจากเครื่องแม่ข่าย การกดสามครั้งที่สถานีจึงแจ้งให้ทราบแทนการสลับเอง
void showThemeLockedNotice() {
  tft.fillScreen(getStBg());
  drawStationTopBar("DISPLAY MODE");
  drawStationCard(16, 52, 288, 128, getStCyan(), getStCardBg());
  drawFitCenteredText(28, 70, 264, 24, "THEME IS SET BY THE HOST", 2, getStTextMain(), getStCardBg());
  drawFitCenteredText(28, 104, 264, 16, "ALL STATIONS SHARE ONE DISPLAY MODE", 1, getStTextMuted(), getStCardBg());
  drawFitCenteredText(28, 132, 264, 16, "PRESS 3x AT THE HOST TERMINAL TO SWITCH", 1, getStTextMuted(), getStCardBg());
  drawStationBottomBar("RETURNING TO THE PREVIOUS PAGE...");
  delay(1400);
  showStationPage(currentStationPage, true);
}

// ============================================================================
// LIVE SCAN ALERT (FULL-SCREEN COLOR TAKEOVER FOR STATION)
// ============================================================================
void displayResult(String status, String name, String id, String refNo, String claimTime, String msg) {
  wakeScreenIfNeeded();

  uint16_t screenBg;
  uint16_t cardBg;
  uint16_t bannerBg;
  uint16_t bannerFg;
  uint16_t accentColor;
  uint16_t textColor = 0xFFFF; // Pure White
  uint16_t textMuted;
  const char* headerTitle;
  const char* footerDesc;

  // เสียงถูกย้ายไปเล่น "หลัง" วาดจอเสร็จ เพราะ tone()+delay() เดิมบล็อกนานถึง 750 ms
  // ทำให้หน้าจอผลลัพธ์ขึ้นช้ากว่าที่ผู้ใช้แตะบัตรเกือบหนึ่งวินาที
  enum { SFX_SUCCESS, SFX_ALARM, SFX_ERROR } sfx;

  if (status == "SUCCESS") {
    ledApproved();
    sfx = SFX_SUCCESS;
    screenBg    = 0x02E5;             // สีพื้นหลังเฉดเขียวเข้ม
    cardBg      = 0x01C3;             // สีพื้นการ์ดโทนเขียวมรกตลึก
    bannerBg    = DARK_ACCENT_GREEN;  // แบนเนอร์สีเขียวนีออน (0x07E0)
    bannerFg    = 0x0000;             // ตัวอักษรสีดำ
    accentColor = DARK_ACCENT_GREEN;  
    textMuted   = 0x87F0;             // ข้อความกำกับสีเขียวมิ้นต์
    headerTitle = ">>> APPROVED: 35B QUOTA <<<";
    footerDesc  = "TRANSACTION VERIFIED | DAILY QUOTA APPLIED";
  } 
  else if (status == "ALREADY_USED") {
    ledDuplicate();
    sfx = SFX_ALARM;
    screenBg    = 0x8200;             // สีพื้นหลังเฉดส้มอิฐเข้ม
    cardBg      = 0x4900;             // สีพื้นการ์ดโทนส้มเข้ม
    bannerBg    = ST77XX_ORANGE;      // แบนเนอร์สีส้มสด (0xFD20)
    bannerFg    = 0x0000;             // ตัวอักษรสีดำ
    accentColor = ST77XX_ORANGE;
    textMuted   = 0xFDC0;             // ข้อความกำกับสีส้มอ่อน
    headerTitle = "! DUPLICATE: ALREADY CLAIMED !";
    footerDesc  = "QUOTA ALREADY CONSUMED FOR TODAY";
  } 
  else if (status == "TIME_CLOSED") {
    ledDuplicate();
    sfx = SFX_ERROR;
    screenBg    = 0x6200;             // พื้นหลังโทนส้มอมน้ำตาล
    cardBg      = 0x3900;             // พื้นหลังการ์ดโทนน้ำตาลเข้ม
    bannerBg    = DARK_ACCENT_YELLOW; // แถบแบนเนอร์สีเหลืองเตือนภัย (0xFFE0 ตามไฟล์คาลิเบท)
    bannerFg    = 0x0000;             
    accentColor = DARK_ACCENT_YELLOW; // ขอบการ์ดและเส้นคั่นสีเหลืองเตือนภัย (0xFFE0)
    textMuted   = 0xFEE0;             // ข้อความกำกับสีเหลืองอ่อน
    headerTitle = "! SERVICE HOURS ARE CLOSED !";
    footerDesc  = "CARD SCANNED OUTSIDE SERVICE WINDOW";
  } 
  else {
    ledRejected();
    sfx = SFX_ERROR;
    screenBg    = 0x8000;             // สีพื้นหลังเฉดแดงทึบเข้ม
    cardBg      = 0x4800;             // สีพื้นการ์ดโทนแดงเข้ม
    bannerBg    = DARK_ACCENT_ROSE;   // แบนเนอร์สีแดงสด (0xF800 ตามไฟล์คาลิเบท)
    bannerFg    = 0xFFFF;             // ตัวอักษรสีขาว
    accentColor = DARK_ACCENT_ROSE;   
    textMuted   = 0xFCAE;             // ข้อความกำกับสีชมพูอ่อน
    headerTitle = "X REJECTED: UNREGISTERED X";
    footerDesc  = "CARD NOT FOUND IN STUDENT DIRECTORY";
  }

  tft.fillScreen(screenBg);

  tft.fillRect(0, 0, 320, 34, bannerBg);
  // ย่อฟอนต์อัตโนมัติ: ข้อความอย่าง "! DUPLICATE: ALREADY CLAIMED !" ยาว 360px เกินจอ 320px
  drawFitCenteredText(0, 0, 320, 34, headerTitle, 2, bannerFg, bannerBg);

  tft.fillRoundRect(8, 40, 304, 192, 8, cardBg);
  tft.drawRoundRect(8, 40, 304, 192, 8, accentColor);
  tft.drawRoundRect(9, 41, 302, 190, 7, accentColor);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 50);
  tft.print("SERVICE STATION:");
  tft.setCursor(156, 50);
  tft.print("CARD UID (ENCRYPTED):");

  tft.setTextSize(2);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 62);
  tft.printf("STATION 0%d", currentStationId);
  tft.setCursor(156, 62);
  tft.println(maskUID(lastProcessedUID));

  tft.drawFastHLine(20, 84, 280, accentColor);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 90);
  tft.print("BENEFICIARY STUDENT ID:");

  String cleanId = (id != "-" && id.length() > 0) ? id : "UNKNOWN";
  tft.setTextSize(3);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 102);
  tft.print(cleanId);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 132);
  tft.print("BENEFICIARY NAME:");

  tft.setTextSize(2);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 144);
  String displayName = (name != "-" && name.length() > 0) ? name : "UNREGISTERED STUDENT";
  if (displayName.length() > 22) displayName = displayName.substring(0, 22);
  tft.print(displayName);

  tft.fillRoundRect(16, 186, 288, 36, 6, bannerBg);
  drawFitCenteredText(16, 186, 288, 36, footerDesc, 1, bannerFg, bannerBg);

  // จอพร้อมแล้วค่อยเล่นเสียงแจ้งเตือน
  if (sfx == SFX_SUCCESS)      soundSuccess();
  else if (sfx == SFX_ALARM)   soundAlarm();
  else                         soundError();
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  const uint8_t *mac = recv_info->src_addr;
  lastHostRssi = recv_info->rx_ctrl->rssi;

  if (len == sizeof(HostConfigPacket)) {
    HostConfigPacket cfg;
    memcpy(&cfg, data, sizeof(cfg));
    if (cfg.magic != ESPNOW_PROTO_MAGIC || cfg.version != ESPNOW_PROTO_VER) return;
    if (cfg.msgType != MSG_CONFIG) return;
    // stationId == 0 คือ broadcast ถึงทุกสถานี
    if (cfg.stationId != 0 && cfg.stationId != currentStationId) return;
    portENTER_CRITICAL_ISR(&espnowMux);
    cfgDark        = cfg.darkMode ? 1 : 0;
    cfgScreenOn    = cfg.screenOn ? 1 : 0;
    cfgScreensaver = cfg.screensaver ? 1 : 0;
    cfgModeSeq     = cfg.modeSeq;
    pendingConfigUpdate = true;
    portEXIT_CRITICAL_ISR(&espnowMux);
    return;
  }

  if (len == sizeof(HostRosterPacket)) {
    HostRosterPacket r;
    memcpy(&r, data, sizeof(r));
    if (r.magic != ESPNOW_PROTO_MAGIC || r.version != ESPNOW_PROTO_VER) return;
    if (r.msgType != MSG_ROSTER) return;
    if (r.stationId != 0 && r.stationId != currentStationId) return;
    rosterApplyPacket(r);
    return;
  }

  if (len != sizeof(HostResponsePacket)) return;

  HostResponsePacket pkt;
  memcpy(&pkt, data, sizeof(pkt));
  if (pkt.magic != ESPNOW_PROTO_MAGIC || pkt.version != ESPNOW_PROTO_VER) return;
  if (pkt.stationId != currentStationId) return;

  if (memcmp(hostMacAddress, mac, 6) != 0) {
    memcpy(hostMacAddress, mac, 6);
    hostPeerReady = false;
  }

  lastHostAckTime = millis();
  isHostOnline = true;

  if (pkt.claimTime[0] != '\0' && strcmp(pkt.claimTime, "-") != 0) {
    portENTER_CRITICAL_ISR(&espnowMux);
    strncpy(pendingHostTime, pkt.claimTime, sizeof(pendingHostTime) - 1);
    pendingHostTime[sizeof(pendingHostTime) - 1] = '\0';
    pendingTimeSync = true;
    portEXIT_CRITICAL_ISR(&espnowMux);
  }

  if (pkt.msgType == MSG_HEARTBEAT) {
    pendingServedCount = pkt.servedCount;
    hasServedCountUpdate = true;
    portENTER_CRITICAL_ISR(&espnowMux);
    strncpy(pendingSysMsg, pkt.message, sizeof(pendingSysMsg) - 1);
    pendingSysMsg[sizeof(pendingSysMsg) - 1] = '\0';
    pendingSysInfo = true;
    portEXIT_CRITICAL_ISR(&espnowMux);
    return;
  }

  if (pkt.msgType == MSG_SCAN_RESP) {
    if (currentState == STATE_SCANNING_SENT && pkt.seq == pendingScanSeq) {
      memcpy(&receivedPacketBuffer, &pkt, sizeof(pkt));
      hasNewPacket = true;
    } else if (currentState == STATE_SYNCING && pkt.seq == syncSeq) {
      memcpy(&receivedPacketBuffer, &pkt, sizeof(pkt));
      syncAckReceived = true;
    }
  }
}
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  (void)tx_info;
  if (status != ESP_NOW_SEND_SUCCESS) lastSendFailed = true;
}
#else
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  lastHostRssi = -60;

  if (len == sizeof(HostConfigPacket)) {
    HostConfigPacket cfg;
    memcpy(&cfg, data, sizeof(cfg));
    if (cfg.magic != ESPNOW_PROTO_MAGIC || cfg.version != ESPNOW_PROTO_VER) return;
    if (cfg.msgType != MSG_CONFIG) return;
    // stationId == 0 คือ broadcast ถึงทุกสถานี
    if (cfg.stationId != 0 && cfg.stationId != currentStationId) return;
    portENTER_CRITICAL_ISR(&espnowMux);
    cfgDark        = cfg.darkMode ? 1 : 0;
    cfgScreenOn    = cfg.screenOn ? 1 : 0;
    cfgScreensaver = cfg.screensaver ? 1 : 0;
    cfgModeSeq     = cfg.modeSeq;
    pendingConfigUpdate = true;
    portEXIT_CRITICAL_ISR(&espnowMux);
    return;
  }

  if (len == sizeof(HostRosterPacket)) {
    HostRosterPacket r;
    memcpy(&r, data, sizeof(r));
    if (r.magic != ESPNOW_PROTO_MAGIC || r.version != ESPNOW_PROTO_VER) return;
    if (r.msgType != MSG_ROSTER) return;
    if (r.stationId != 0 && r.stationId != currentStationId) return;
    rosterApplyPacket(r);
    return;
  }

  if (len != sizeof(HostResponsePacket)) return;

  HostResponsePacket pkt;
  memcpy(&pkt, data, sizeof(pkt));
  if (pkt.magic != ESPNOW_PROTO_MAGIC || pkt.version != ESPNOW_PROTO_VER) return;
  if (pkt.stationId != currentStationId) return;

  if (memcmp(hostMacAddress, mac, 6) != 0) {
    memcpy(hostMacAddress, mac, 6);
    hostPeerReady = false;
  }

  lastHostAckTime = millis();
  isHostOnline = true;

  if (pkt.claimTime[0] != '\0' && strcmp(pkt.claimTime, "-") != 0) {
    portENTER_CRITICAL_ISR(&espnowMux);
    strncpy(pendingHostTime, pkt.claimTime, sizeof(pendingHostTime) - 1);
    pendingHostTime[sizeof(pendingHostTime) - 1] = '\0';
    pendingTimeSync = true;
    portEXIT_CRITICAL_ISR(&espnowMux);
  }

  if (pkt.msgType == MSG_HEARTBEAT) {
    pendingServedCount = pkt.servedCount;
    hasServedCountUpdate = true;
    portENTER_CRITICAL_ISR(&espnowMux);
    strncpy(pendingSysMsg, pkt.message, sizeof(pendingSysMsg) - 1);
    pendingSysMsg[sizeof(pendingSysMsg) - 1] = '\0';
    pendingSysInfo = true;
    portEXIT_CRITICAL_ISR(&espnowMux);
  } else if (pkt.msgType == MSG_SCAN_RESP) {
    if (currentState == STATE_SCANNING_SENT && pkt.seq == pendingScanSeq) {
      memcpy(&receivedPacketBuffer, &pkt, sizeof(pkt));
      hasNewPacket = true;
    } else if (currentState == STATE_SYNCING && pkt.seq == syncSeq) {
      memcpy(&receivedPacketBuffer, &pkt, sizeof(pkt));
      syncAckReceived = true;
    }
  }
}
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  (void)mac_addr;
  if (status != ESP_NOW_SEND_SUCCESS) lastSendFailed = true;
}
#endif

void drawStationIdConfigProgress(unsigned long elapsedMs, bool holdToEnter) {
  const unsigned long totalMs = STATION_ID_HOLD_MS;
  if (elapsedMs > totalMs) elapsedMs = totalMs;
  int w = (int)((elapsedMs * 260UL) / totalMs);
  if (w < 0) w = 0;
  if (w > 260) w = 260;

  tft.fillRect(28, 166, 264, 16, getStCardBg());
  tft.drawRoundRect(30, 168, 260, 12, 5, getStCardBorder());
  if (w > 2) tft.fillRoundRect(30, 168, w, 12, 5, getStCyan());

  unsigned long remain = (totalMs > elapsedMs) ? (totalMs - elapsedMs) : 0;
  unsigned long sec = (remain + 999) / 1000;
  tft.fillRect(28, 186, 264, 16, getStCardBg());
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(46, 190);
  if (holdToEnter) {
    if (remain > 0) tft.printf("KEEP HOLDING... %lu SEC", sec);
    else tft.print("RELEASE TO ENTER ID SETUP");
  } else {
    if (remain > 0) tft.printf("AUTO SAVE IN %lu SEC", sec);
    else tft.print("SAVING...");
  }
}

void saveStationId(uint8_t selectedId) {
  currentStationId = selectedId;
  stationPrefs.begin("st_cfg", false);
  stationPrefs.putUChar("id", currentStationId);
  stationPrefs.end();
  updateShopLabel();

  nextHeartbeatInterval = BASE_HEARTBEAT + (currentStationId * 350) + random(0, 200);
  StationPacket hbPkt = {};
  hbPkt.magic = ESPNOW_PROTO_MAGIC;
  hbPkt.version = ESPNOW_PROTO_VER;
  hbPkt.msgType = MSG_HEARTBEAT;
  hbPkt.stationId = currentStationId;
  hbPkt.seq = 0;
  hbPkt.systemVoltage = readBatteryVoltage();
  stampRosterVer(hbPkt);
  sendToHost((uint8_t *)&hbPkt, sizeof(StationPacket));
}

void runStationIdConfigMode() {
  wakeScreenIfNeeded();
  currentState = STATE_CONFIG_ID;
  lastActivityTime = millis();
  soundClick();
  setLedColor(50, 0, 50);

  uint8_t selectedId = currentStationId;
  unsigned long lastActionTime = millis();
  int lastShownSec = -1;

  tft.fillScreen(getStBg());
  drawStationTopBar("STATION ID SETUP");
  drawStationCard(16, 36, 288, 166, getStYellow(), getStCardBg());

  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(32, 48);
  tft.println("SELECT STATION ID");

  auto drawSelectedId = [&]() {
    tft.fillRect(130, 70, 64, 40, getStCardBg());
    tft.setTextColor(getStGreen(), getStCardBg());
    tft.setTextSize(4);
    tft.setCursor(140, 72);
    tft.printf("0%d", selectedId);
  };

  drawSelectedId();

  drawStationPillBadge(34, 124, 252, 22, "TAP = NEXT ID (01-04)",
                       (isStationDarkMode ? 0x0000 : 0xFFFF), getStCyan());
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(44, 151);
  tft.print("RELEASE / WAIT 3 SEC TO SAVE");
  drawStationBottomBar("ID SETUP | AUTO SAVE");

  auto drawCountdown = [&](int sec) {
    tft.fillRect(70, 178, 180, 16, getStCardBg());
    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.setTextSize(1);
    tft.setCursor(82, 182);
    if (sec > 0) tft.printf("AUTO SAVE IN %d SEC", sec);
    else tft.print("SAVING...");
  };

  while (millis() - lastActionTime < CONFIG_AUTO_SAVE_MS) {
    if (digitalRead(BTN_PIN) == LOW) {
      selectedId = (selectedId % 4) + 1;
      soundClick();
      lastActionTime = millis();
      lastShownSec = -1;
      drawSelectedId();
      while (digitalRead(BTN_PIN) == LOW) delay(10);
    }

    unsigned long elapsed = millis() - lastActionTime;
    int sec = (int)((CONFIG_AUTO_SAVE_MS - elapsed + 999) / 1000);
    if (sec < 0) sec = 0;
    if (sec != lastShownSec) {
      drawCountdown(sec);
      lastShownSec = sec;
    }
    delay(10);
  }

  drawCountdown(0);
  saveStationId(selectedId);
  soundSuccess();

  tft.fillScreen(getStBg());
  drawStationCard(20, 70, 280, 100, getStGreen(), getStCardBg());
  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(2);
  tft.setCursor(48, 98);
  tft.println("STATION ID SAVED");
  tft.setTextColor(getStCyan(), getStCardBg());
  tft.setTextSize(3);
  tft.setCursor(130, 126);
  tft.printf("0%d", currentStationId);
  delay(700);

  lastActivityTime = millis();
  showStationPage(currentStationPage, true);
}

void handlePhysicalButton() {
  static unsigned long btnPressStart = 0;
  static unsigned long lastReleaseTime = 0;
  static bool btnWasPressed = false;
  static int clickCount = 0;
  static bool holdUiShown = false;

  bool btnState = (digitalRead(BTN_PIN) == LOW);

  if (btnState && !btnWasPressed) {
    btnPressStart = millis();
    btnWasPressed = true;
    holdUiShown = false;
  }

  if (btnState && btnWasPressed && (currentState == STATE_STANDBY || currentState == STATE_STATUS)) {
    unsigned long held = millis() - btnPressStart;
    if (held >= 200 && !holdUiShown) {
      wakeScreenIfNeeded();
      tft.fillScreen(getStBg());
      drawStationTopBar("STATION ID SETUP");
      drawStationCard(16, 36, 288, 166, getStYellow(), getStCardBg());
      tft.setTextColor(getStTextMain(), getStCardBg());
      tft.setTextSize(2);
      tft.setCursor(44, 64);
      tft.println("HOLD TO CONFIGURE");
      tft.setTextColor(getStTextMuted(), getStCardBg());
      tft.setTextSize(1);
      tft.setCursor(60, 96);
      tft.println("KEEP BUTTON PRESSED FOR 3 SEC");
      drawStationIdConfigProgress(held, true);
      drawStationBottomBar("RELEASE AFTER 3 SEC TO SET ID");
      holdUiShown = true;
    } else if (holdUiShown) {
      drawStationIdConfigProgress(held, true);
      if (held >= STATION_ID_HOLD_MS) {
        while (digitalRead(BTN_PIN) == LOW) delay(10);
        runStationIdConfigMode();
        clickCount = 0;
        btnWasPressed = false;
        holdUiShown = false;
        return;
      }
    }
  }

  if (!btnState && btnWasPressed) {
    unsigned long pressDuration = millis() - btnPressStart;
    btnWasPressed = false;
    lastActivityTime = millis();

    // ระหว่างส่งรายการออฟไลน์เข้าระบบ อย่าให้การกดปุ่มมาตัดกลางคัน
    if (currentState == STATE_SYNCING) { clickCount = 0; return; }

    if (holdUiShown) {
      holdUiShown = false;
      if (pressDuration < STATION_ID_HOLD_MS) {
        showStationPage(currentStationPage, true);
      }
      clickCount = 0;
      return;
    }

    if (currentState == STATE_SCREENSAVER || currentState == STATE_CREDIT || !isScreenOn) {
      wakeScreenIfNeeded();
      soundHomeBeep();
      showStationPage(1, true);
      clickCount = 0;
      return;
    }

    if (pressDuration >= 5000) {
      wakeScreenIfNeeded();
      currentState = STATE_CREDIT;
      soundCreditJingle();
      renderDeveloperCredit();
      clickCount = 0;
      return;
    }
    else if (pressDuration >= 1500) {
      setScreenPower(!isScreenOn);
      if (!isScreenOn) ledOff(); else ledStandby();
      soundClick();
      clickCount = 0;
      return;
    }
    else if (pressDuration >= 25) {
      clickCount++;
      lastReleaseTime = millis();
    }
  }

  if (clickCount > 0 && (millis() - lastReleaseTime > MULTI_CLICK_GAP)) {
    if (clickCount == 1) {
      soundClick();
      showStationPage((currentStationPage % 3) + 1, true);
    }
    else if (clickCount == 2) {
      currentState = STATE_SCREENSAVER;
      soundScreensaverBeep();
      renderScreensaver(true);
    }
    else if (clickCount == 3) {
      soundClick();
      if (isScreenOn) showThemeLockedNotice();
    }
    clickCount = 0;
  }
}

void sendCardToHost(String uid) {
  totalScansToday++;
  lastActivityTime = millis();

  displayScanningUID(uid);

  memset(&pendingScanPacket, 0, sizeof(pendingScanPacket));
  pendingScanPacket.magic = ESPNOW_PROTO_MAGIC;
  pendingScanPacket.version = ESPNOW_PROTO_VER;
  pendingScanPacket.msgType = MSG_SCAN_REQ;
  pendingScanPacket.stationId = currentStationId;

  if (nextScanSeq == 0) nextScanSeq = 1;
  pendingScanSeq = nextScanSeq++;
  if (nextScanSeq == 0) nextScanSeq = 1;

  pendingScanPacket.seq = pendingScanSeq;
  strncpy(pendingScanPacket.uid, uid.c_str(), sizeof(pendingScanPacket.uid) - 1);
  pendingScanPacket.offlineTime[0] = '\0';
  pendingScanPacket.systemVoltage = readBatteryVoltage();

  sendToHost((uint8_t *)&pendingScanPacket, sizeof(pendingScanPacket));
  scanRetryCount = 0;
  lastScanSendTime = millis();

  currentState = STATE_SCANNING_SENT;
  stateHoldUntil = millis() + (isHostOnline ? SCAN_TIMEOUT_MS : SCAN_TIMEOUT_OFFLINE_MS);
}

void checkRC522() {
  if (currentState == STATE_SCANNING_SENT || currentState == STATE_RESULT_DISPLAY ||
      currentState == STATE_SYNCING) return;

  if (millis() - lastRc522HealthCheck > 10000) {
    lastRc522HealthCheck = millis();
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (v == 0x00 || v == 0xFF) {
      mfrc522.PCD_Init();
    }
  }

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  uint32_t cardId = 0;
  if (mfrc522.uid.size >= 4) {
    cardId = ((uint32_t)mfrc522.uid.uidByte[3] << 24) |
             ((uint32_t)mfrc522.uid.uidByte[2] << 16) |
             ((uint32_t)mfrc522.uid.uidByte[1] << 8)  |
             ((uint32_t)mfrc522.uid.uidByte[0]);
  }

  char buf[16];
  snprintf(buf, sizeof(buf), "%010lu", cardId);
  String uidStr = String(buf);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  if (uidStr.length() > 0) {
    if (uidStr == lastProcessedUID && (millis() - lastProcessedTime < COOLDOWN_MS)) return;
    lastProcessedUID = uidStr;
    lastProcessedTime = millis();
    sendCardToHost(uidStr);
  }
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT); noTone(BUZZER_PIN);
  pinMode(TFT_BLK, OUTPUT); setScreenPower(true);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(RGB_LED_PIN, OUTPUT);
  
  analogSetAttenuation(ADC_11db);

  // จอใช้ HSPI, RC522 ใช้ SPI มาตรฐาน (FSPI) — คนละ peripheral จึงไม่ชนกัน
  SPI_TFT.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  SPI.begin(RC522_SCK, RC522_MISO, RC522_MOSI, RC522_SS);

  tft.init(240, 320); 
  tft.setRotation(1); 
  tft.setTextWrap(false);

  // คาลิเบทพาเนลจอตามไฟล์ TFT Color Calibration Tool
  tft.invertDisplay(false);

  mfrc522.PCD_Init();

  stationPrefs.begin("st_cfg", false);
  currentStationId = stationPrefs.getUChar("id", 1);
  isStationDarkMode = stationPrefs.getBool("dark", true);
  stationPrefs.end();
  updateShopLabel();

  stationPrefs.begin("st_stats", false);
  totalSuccessToday = stationPrefs.getUInt("served", 0);
  stationPrefs.end();

  loadOfflineQueue();
  loadRoster();

  nextHeartbeatInterval = BASE_HEARTBEAT + (currentStationId * 350) + random(0, 200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
  // เพิ่ม WIFI_PROTOCOL_LR เพื่อให้คุยกับแม่ข่ายในโหมดระยะไกลได้
  esp_wifi_set_protocol(WIFI_IF_STA,
                        WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);
  // 80 = 20 dBm ซึ่งเป็นค่าสูงสุดของ ESP32-S3 เดิมตั้งไว้ 68 = 17 dBm
  esp_wifi_set_max_tx_power(80);

  esp_now_init();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  applyEspNowRate(broadcastAddress);

  playBootAnimation();
  lastActivityTime = millis();
  showStationPage(1, true);

  for (int i = 0; i < 3; i++) {
    StationPacket hbPkt = {};
    hbPkt.magic = ESPNOW_PROTO_MAGIC;
    hbPkt.version = ESPNOW_PROTO_VER;
    hbPkt.msgType = MSG_HEARTBEAT;
    hbPkt.stationId = currentStationId;
    hbPkt.seq = 0;
    hbPkt.systemVoltage = readBatteryVoltage();
    stampRosterVer(hbPkt);
    sendToHost((uint8_t *)&hbPkt, sizeof(StationPacket));
    delay(30);
  }

  lastHeartbeatTime = millis();
}

void loop() {
  handlePhysicalButton();
  calculateCpuLoad();
  saveRosterIfDue(false);   // บันทึกบัญชีสิทธิ์ลง NVS แบบหน่วงเวลา กันเสียของตอนรีบูต

  if (currentState == STATE_STATUS && isScreenOn && (millis() - lastCpuDisplayUpdate >= 500)) {
    displayStatusScreen(false);
  }

  if (hasServedCountUpdate) {
    hasServedCountUpdate = false;
    uint32_t hostServed = pendingServedCount;
    // เขียน NVS เฉพาะตอนค่าจริง ๆ เปลี่ยน เพื่อลดการสึกหรอของแฟลช (heartbeat มาทุก 6 วินาที)
    if (hostServed != totalSuccessToday) {
      totalSuccessToday = hostServed;
      stationPrefs.begin("st_stats", false);
      stationPrefs.putUInt("served", totalSuccessToday);
      stationPrefs.end();
    }

    if (currentState == STATE_STANDBY && currentStationPage == 2 && isScreenOn) {
      displayStatsDashboard(false);
    }
  }

  if (pendingSysInfo) {
    char msgCopy[32];
    portENTER_CRITICAL(&espnowMux);
    strncpy(msgCopy, pendingSysMsg, sizeof(msgCopy) - 1);
    msgCopy[sizeof(msgCopy) - 1] = '\0';
    pendingSysInfo = false;
    portEXIT_CRITICAL(&espnowMux);
    applySystemSummary(msgCopy);
    if (currentState == STATE_STANDBY && currentStationPage == 2 && isScreenOn) {
      displayStatsDashboard(false);
    }
  }

  if (pendingConfigUpdate) applyHostConfig();

  // รีเฟรชแถบสถานะลิงก์/แบตเตอรี่มุมขวาบนของหน้าปกติ
  if (isScreenOn && (currentState == STATE_STANDBY || currentState == STATE_STATUS) &&
      (millis() - lastHeaderRefresh >= HEADER_REFRESH_MS)) {
    lastHeaderRefresh = millis();
    updateStationHeaderStatus(false);
  }

  // ยิงซ้ำถี่ขึ้นช่วงแรกแล้วค่อยห่างออก และถ้าชิปบอกว่าส่งไม่ถึงก็ยิงซ้ำทันที
  // ไม่ต้องรอครบช่วงเวลา รวมแล้วได้ 7 ครั้งภายใน timeout 3 วินาทีเท่าเดิม
  if (currentState == STATE_SCANNING_SENT && !hasNewPacket && scanRetryCount < SCAN_MAX_RETRY) {
    unsigned long gap = 120UL + (unsigned long)scanRetryCount * 150UL;
    unsigned long since = millis() - lastScanSendTime;
    if ((lastSendFailed && since >= 60) || since >= gap) {
      lastSendFailed = false;
      sendToHost((uint8_t *)&pendingScanPacket, sizeof(StationPacket));
      scanRetryCount++;
      lastScanSendTime = millis();
    }
  }

  if (hasNewPacket) {
    hasNewPacket = false;
    // นับยอดที่นี่ ไม่ใช่ซ่อนไว้ใน displayResult() ซึ่งเป็นฟังก์ชันวาดจอ
    if (strcmp(receivedPacketBuffer.status, "SUCCESS") == 0) {
      totalSuccessToday++;
      pushStationTap(receivedPacketBuffer.studentId);
    } else {
      totalRejectToday++;
    }
    displayResult(String(receivedPacketBuffer.status), 
                  String(receivedPacketBuffer.name), 
                  String(receivedPacketBuffer.studentId), 
                  String(receivedPacketBuffer.refNo), 
                  String(receivedPacketBuffer.claimTime), 
                  String(receivedPacketBuffer.message));
    scanRetryCount = SCAN_MAX_RETRY;
    currentState = STATE_RESULT_DISPLAY;
    stateHoldUntil = millis() + 4000;
  }

  if (currentState == STATE_SCANNING_SENT && millis() > stateHoldUntil) {
    // เดิมขึ้นจอแดงแล้วปฏิเสธนิสิตไปเฉย ๆ โดยไม่เหลือร่องรอยอะไรไว้
    // ตอนนี้บันทึกการแตะลงหน่วยความจำถาวรก่อน แล้วให้แม่ค้าจ่ายอาหารไปได้เลย
    String tappedUid = String(pendingScanPacket.uid);
    tappedUid.trim();

    // ตรวจกับบัญชีสิทธิ์ที่แม่ข่ายผลักมาเก็บไว้ก่อนลิงก์ขาด
    // ถ้าบัญชีใช้การไม่ได้ (ยังไม่เคยได้รับ กำลังรับชุดใหม่ หรือใหญ่เกินเก็บ)
    // จะได้ ROSTER_UNKNOWN แล้วปล่อยผ่านเหมือนพฤติกรรมเดิมทุกประการ
    RosterVerdict verdict = rosterLookup(tappedUid);

    if (findOfflineTap(tappedUid) >= 0) {
      displayOfflineSaved(tappedUid, true);
    } else if (verdict == ROSTER_NOT_FOUND) {
      totalRejectToday++;
      displayOfflineRejected(tappedUid, false);
    } else if (verdict == ROSTER_CLAIMED) {
      totalRejectToday++;
      displayOfflineRejected(tappedUid, true);
    } else if (enqueueOfflineTap(tappedUid)) {
      rosterMarkClaimedLocal(tappedUid);
      displayOfflineSaved(tappedUid, false);
    } else {
      displayOfflineAlert();   // คิวเต็ม รับเพิ่มไม่ได้จริง ๆ
    }
    currentState = STATE_RESULT_DISPLAY;
    stateHoldUntil = millis() + 3200;
  }

  // ลิงก์กลับมาแล้วและยังมีรายการค้าง ส่งเข้าระบบเองโดยไม่ต้องให้ใครสั่ง
  if (currentState != STATE_SYNCING && offlineCount > 0 && isHostOnline &&
      (long)(millis() - syncRetryNotBefore) >= 0 &&
      (currentState == STATE_STANDBY || currentState == STATE_STATUS ||
       currentState == STATE_SCREENSAVER)) {
    startOfflineSync();
  }

  if (currentState == STATE_SYNCING) {
    if (syncAckReceived) {
      syncAckReceived = false;
      if (strcmp(receivedPacketBuffer.status, "SUCCESS") == 0) {
        syncDone++;
        totalSuccessToday++;
        pushStationTap(receivedPacketBuffer.studentId);
      } else {
        syncRejected++;   // แม่ข่ายตอบว่าซ้ำหรือไม่พบบัตร ถือว่าส่งถึงแล้วเช่นกัน
      }
      popOfflineTap();
      syncRetry = 0;
      if (offlineCount > 0) {
        if (!syncQuiet && isScreenOn) displaySyncProgress(syncDone + syncRejected, syncTotal);
        sendOfflineTap();
      } else {
        finishOfflineSync(false);
      }
    } else if (millis() - syncSentAt > SYNC_ACK_TIMEOUT_MS) {
      syncRetry++;
      if (syncRetry >= 3) finishOfflineSync(true);
      else sendOfflineTap();
    }
  }

  if (currentState == STATE_RESULT_DISPLAY && millis() > stateHoldUntil) {
    showStationPage(1, true);
  }

  if (millis() - lastHostAckTime > HOST_OFFLINE_TIMEOUT) {
    if (isHostOnline) {
      isHostOnline = false;
    }
  }

  if (pendingTimeSync) {
    char timeCopy[24];
    portENTER_CRITICAL(&espnowMux);
    strncpy(timeCopy, pendingHostTime, sizeof(timeCopy) - 1);
    timeCopy[sizeof(timeCopy) - 1] = '\0';
    pendingTimeSync = false;
    portEXIT_CRITICAL(&espnowMux);
    syncInternalClock(timeCopy);
  }

  if (millis() - lastHeartbeatTime > nextHeartbeatInterval) {
    lastHeartbeatTime = millis();
    nextHeartbeatInterval = BASE_HEARTBEAT + (currentStationId * 350) + random(0, 200);

    StationPacket hbPkt = {};
    hbPkt.magic = ESPNOW_PROTO_MAGIC;
    hbPkt.version = ESPNOW_PROTO_VER;
    hbPkt.msgType = MSG_HEARTBEAT;
    hbPkt.stationId = currentStationId;
    hbPkt.seq = 0;
    hbPkt.systemVoltage = readBatteryVoltage(true);
    stampRosterVer(hbPkt);
    sendToHost((uint8_t *)&hbPkt, sizeof(StationPacket));
  }

  if (currentState != STATE_SCREENSAVER && currentState != STATE_CREDIT &&
      currentState != STATE_SCANNING_SENT && currentState != STATE_RESULT_DISPLAY &&
      currentState != STATE_SYNCING &&
      (millis() - lastActivityTime >= TIMEOUT_SCREENSAVER)) {
    currentState = STATE_SCREENSAVER;
    renderScreensaver(true);
  }

  if (currentState == STATE_SCREENSAVER && isScreenOn && (millis() - lastClockRefresh >= 1000)) {
    lastClockRefresh = millis();
    renderScreensaver(false);
  }

  checkRC522();

  delay(2);
}