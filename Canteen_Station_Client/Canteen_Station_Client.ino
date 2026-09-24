/**
 * @file      Canteen_Station_Client.ino
 * @brief     เครื่องประจำร้านค้า อ่านบัตร RFID แล้วถามสิทธิ์จากเครื่องแม่ข่าย
 * @version   122.11.0
 * @date      2026-09-23
 * @author    Kittiphan Rattanakorn <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * มหาวิทยาลัยมหาจุฬาลงกรณราชวิทยาลัย วิทยาเขตแพร่
 *
 * @par Description
 * เครื่องนี้ไม่ได้ตัดสินสิทธิ์เอง หน้าที่คืออ่านหมายเลขบัตรแล้วส่งไปถามแม่ข่าย
 * ผ่าน ESP-NOW ช่อง 1 แล้วแสดงคำตอบที่ได้กลับมา
 *
 * ไฟล์นี้เก็บเฉพาะตรรกะระบบ ส่วนที่วาดลงจอถูกแยกออกไปเป็น StationScreen.h
 * ซึ่ง #include ไว้ท้ายไฟล์ก่อน setup()
 *
 * @par Hardware
 * ESP32-S3 DevKitC-1 (N16R8) · จอ ST7789V 2.8" 320x240 บนบัส HSPI ·
 * เครื่องอ่านบัตร RC522 บนบัส FSPI · บัซเซอร์ GPIO 38 · ปุ่มกด GPIO 2 · WS2812 GPIO 48
 *
 * @par Dependencies
 * Adafruit GFX · Adafruit ST7735/ST7789 · MFRC522 ·
 * Arduino ESP32 core 2.x หรือ 3.x
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 122.11.0 | 2026-09-24 | จัดขาใหม่ทั้งบอร์ดให้สายไม่ไขว้กัน ย้าย RC522 มาอยู่ติดกันที่ GPIO4-7 กับ 15 ย้าย MISO ออกจากแถวขวา ย้ายวัดแบตเตอรี่ออกจากขา strapping GPIO3 ไป GPIO8 และใช้ขาบัซเซอร์ ปุ่มกด แบตเตอรี่ ชุดเดียวกับฝั่งแม่ข่าย |
 * | 122.10.0 | 2026-09-23 | ปิดโหมดประหยัดพลังงานของวิทยุด้วย WiFi.setSleep(false) ของเดิมใช้ esp_wifi_set_ps() ซึ่งถูกอีเวนต์ STA_START ของ Arduino core ทับกลับเป็น WIFI_PS_MIN_MODEM วิทยุจึงหลับและรับคำสั่งจากแม่ข่ายได้เฉพาะตอนเพิ่งส่ง heartbeat |
 * | 122.9.0 | 2026-09-23 | ตามการแก้แถบบนของ StationScreen.h ตรรกะในไฟล์นี้ไม่เปลี่ยน |
 * | 122.8.0 | 2026-09-23 | เปลี่ยนป้ายบนแถบบนและหน้าตั้งหมายเลขจาก POINT เป็น STATION |
 * | 122.7.0 | 2026-09-23 | สั่งเปลี่ยนธีมตอนสถานีพักหน้าจออยู่ ไม่เตะออกจากหน้าพักจออีกต่อไป วาดใหม่ตามหน้าที่แสดงอยู่จริงแทนการเรียก showStationPage() เสมอ |
 * | 122.6.2 | 2026-09-23 | ตามการแก้วิธีวาดคลื่นของ StationScreen.h ตรรกะในไฟล์นี้ไม่เปลี่ยน |
 * | 122.6.1 | 2026-09-23 | ตามการย้ายตำแหน่งฟังก์ชันของ StationScreen.h ที่ทำให้คอมไพล์ไม่ผ่าน ตรรกะในไฟล์นี้ไม่เปลี่ยน |
 * | 122.6.0 | 2026-09-23 | บันทึกผลการตรวจสุขภาพ RC522 ไว้ใน isReaderReady ให้สัญลักษณ์แตะบัตรบนหน้าแรกบอกความพร้อมได้จริง และตรวจทะเบียนรุ่นซ้ำหลังสั่งเริ่มใหม่ |
 * | 122.5.0 | 2026-09-23 | ตามการจัดตำแหน่งตัวอักษรของ StationScreen.h ตรรกะในไฟล์นี้ไม่เปลี่ยน |
 * | 122.4.0 | 2026-09-23 | ตามการแก้ของ StationScreen.h ตรรกะในไฟล์นี้ไม่เปลี่ยน |
 * | 122.3.0 | 2026-09-22 | ตามการแก้เรื่องกะพริบของ StationScreen.h ตรรกะในไฟล์นี้ไม่เปลี่ยน |
 * | 122.2.0 | 2026-09-22 | เรียกการวาดซ้ำทุกหนึ่งวินาที และเลิกวาดยอดด้วยพิกัดของตัวเองใน loop() |
 * | 122.1.0 | 2026-09-22 | แยกส่วนวาดจอออกไปเป็น StationScreen.h ตรรกะไม่เปลี่ยน |
 * | 122.0.0 | 2026-09-22 | เริ่มใหม่จากต้นฉบับ ย้ายจอไปบัส HSPI แก้ปัญหาจอเพี้ยนหลัง PCD_Init() รับคำสั่งโหมดการแสดงผลจากแม่ข่าย และตัดการตั้งค่าธีมที่ตัวเครื่องออก |
 * | 117.0.7 | 2026-09-21 | ต้นฉบับที่ใช้เป็นจุดเริ่ม เก็บสำเนาไว้ที่ original/ |
 *
 * @warning  จอต้องอยู่บนบัส HSPI เท่านั้น เพราะไลบรารี MFRC522 ยึดตัวแปร SPI
 *           มาตรฐาน (FSPI) ไว้ ถ้าใช้บัสเดียวกันจอจะเพี้ยนทันทีหลัง PCD_Init()
 * @warning  #include ของ StationScreen.h ต้องอยู่ท้ายไฟล์ก่อน setup()
 * @note     โหมดมืด/สว่าง และการพักหน้าจอ ถูกกำหนดจากเครื่องแม่ข่ายฝ่ายเดียว
 *           ปุ่มที่ตัวเครื่องจึงขึ้นข้อความแจ้งแทนการสลับเอง
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

#define APP_VERSION         "122.11.0"
#define DEV_NAME            "Kittiphan Rattanakorn"
#define DEV_ROLE            "Computer Technical Officer"
#define DEV_INSTITUTION     "MCU Phrae Campus"

// Pin Configuration สำหรับ ESP32-S3 DevKitC-1
// ============================================================================
// การจัดขา (Pin Map) — ESP32-S3 DevKitC-1 N16R8
// ============================================================================
// [122.11.0] แก้: จัดขาใหม่ทั้งหมดให้สายไม่ไขว้กัน
//           อุปกรณ์ภายนอก **ทุกตัวอยู่บนแถวซ้ายของบอร์ดแถวเดียว** ไม่มีอะไรข้ามไปฝั่งขวา
//           และขาของแต่ละโมดูลเรียงติดกันตามลำดับขาบนตัวโมดูลเอง
//           รายละเอียดและผังเต็มอยู่ใน docs/HARDWARE.md
//
//   ตำแหน่ง   ขา        จุดบริการ
//   --------  --------  ----------------------------------------------------
//    1, 2     3V3       ไฟเลี้ยง 3.3V ของทุกโมดูล
//    3        RST       ปุ่มรีเซ็ตของบอร์ด ห้ามต่ออะไร
//    4        GPIO4     RC522  SDA (SS)
//    5        GPIO5     RC522  SCK
//    6        GPIO6     RC522  MOSI
//    7        GPIO7     RC522  MISO
//    8        GPIO15    RC522  RST
//    9        GPIO16    ว่าง คั่นระหว่างกลุ่ม
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

// RC522 — ห้าขาเรียงติดกันที่ตำแหน่ง 4 ถึง 8 ใกล้ขา 3V3 ด้านบน
// ลำดับตรงกับขาบนตัวโมดูล SDA, SCK, MOSI, MISO แล้วข้าม IRQ กับ GND ไป RST
// สายทุกเส้นวิ่งลงทางเดียวกัน ไม่มีเส้นไหนไขว้กัน ส่วน IRQ ไม่ได้ใช้
#define RC522_SS            4
#define RC522_SCK           5
#define RC522_MOSI          6
#define RC522_MISO          7
#define RC522_RST           15

// อุปกรณ์สายเดี่ยว — สามขาติดกันที่ตำแหน่ง 10 ถึง 12
// ชุดนี้ใช้ขาเดียวกับฝั่งแม่ข่ายเป๊ะ สายชุดเดียวใช้ได้ทั้งสองบอร์ด
#define BUZZER_PIN          17
#define BTN_PIN             18
#define BATTERY_ADC_PIN     8   // ADC1_CH7 อ่านได้ขณะเปิด Wi-Fi

#define RGB_LED_PIN         48  // WS2812 บนบอร์ด ไม่ต้องเดินสาย

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

// บน ESP32-S3 ตัวแปร SPI มาตรฐานผูกกับ FSPI (SPI2_HOST) และไลบรารี MFRC522
// เรียกใช้ตัวแปรนั้น จอจึงต้องอยู่บน HSPI (SPI3_HOST) มิฉะนั้นอุปกรณ์สองตัว
// จะแย่ง peripheral เดียวกันคนละขา และจอจะเพี้ยนทันทีหลัง PCD_Init()
SPIClass SPI_TFT(HSPI);
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
  MSG_CONFIG    = 4
};

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

// คำสั่งการแสดงผลที่แม่ข่ายสั่งลงมา — เครื่องแม่ข่ายเป็นผู้กำหนดทั้งหมด
// สถานีไม่ตัดสินใจเรื่องธีมหรือการพักหน้าจอเองเลยแม้แต่กรณีเดียว
//   darkMode    โหมดมืด/สว่าง บังคับเสมอ
//   screenOn    เปิด/ปิดไฟหน้าจอ
//   screensaver เข้า/ออกโหมดพักหน้าจอ
//   modeSeq     เพิ่มขึ้นทุกครั้งที่เจ้าหน้าที่เปลี่ยนค่าที่แม่ข่าย ใช้กันคำสั่งซ้ำ
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
static_assert(sizeof(HostConfigPacket) == 8, "HostConfigPacket size mismatch");

static_assert(sizeof(StationPacket) == 50, "StationPacket size mismatch");
static_assert(sizeof(HostResponsePacket) == 200, "HostResponsePacket size mismatch");

enum AppState { 
  STATE_STANDBY, 
  STATE_SCANNING_SENT, 
  STATE_RESULT_DISPLAY, 
  STATE_STATUS, 
  STATE_SCREENSAVER, 
  STATE_CREDIT, 
  STATE_CONFIG_ID 
};
AppState currentState = STATE_STANDBY;

int currentStationPage              = 1;  
bool isScreenOn                     = true;

// คำสั่งการแสดงผลล่าสุดที่ได้รับจากแม่ข่าย อ่าน/เขียนในคอลแบ็ก ESP-NOW
// จึงต้องกันชนด้วย spinlock แล้วค่อยเอาไปทำจริงในลูปหลัก
volatile uint8_t cfgDark        = 1;
volatile uint8_t cfgScreenOn    = 1;
volatile uint8_t cfgScreensaver = 0;
volatile uint8_t cfgModeSeq     = 0;
volatile bool    pendingConfigUpdate = false;
uint8_t lastAppliedModeSeq      = 255;   // 255 = ยังไม่เคยรับคำสั่งใด ๆ
portMUX_TYPE espnowMux = portMUX_INITIALIZER_UNLOCKED;
bool isHostOnline                   = false;
bool isTimeSynced                   = false;

int lastHostRssi                    = -60;
unsigned long lastHostAckTime       = 0;
unsigned long lastActivityTime      = 0;
unsigned long lastClockRefresh      = 0;
unsigned long lastHeartbeatTime     = 0;
unsigned long stateHoldUntil        = 0;
unsigned long lastRc522HealthCheck  = 0;

// [122.6.0] เพิ่ม: สัญลักษณ์แตะบัตรบนหน้าแรกใช้ค่านี้บอกว่าเครื่องอ่านยังตอบอยู่ไหม
//           ค่าถูกปรับทุกสิบวินาทีโดย checkRC522() ที่อ่าน VersionReg อยู่แล้ว
bool isReaderReady                  = true;
unsigned long nextHeartbeatInterval = 6000;

uint32_t totalScansToday            = 0;
uint32_t totalSuccessToday          = 0;
String lastProcessedUID             = "";
unsigned long lastProcessedTime     = 0;

volatile bool hasNewPacket          = false;
HostResponsePacket receivedPacketBuffer;

volatile bool hasServedCountUpdate  = false;
volatile uint16_t pendingServedCount = 0;
volatile bool pendingTimeSync = false;
char pendingHostTime[24] = {0};

uint16_t nextScanSeq = 1;
uint16_t pendingScanSeq = 0;
StationPacket pendingScanPacket = {};
uint8_t scanRetryCount = 0;
unsigned long lastScanSendTime = 0;

const unsigned long MULTI_CLICK_GAP     = 320;
const unsigned long STATION_ID_HOLD_MS  = 3000;
const unsigned long CONFIG_AUTO_SAVE_MS = 3000;
const unsigned long COOLDOWN_MS         = 3500;
const unsigned long SCAN_TIMEOUT_MS     = 3000;
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
void displayStatsDashboard();
void displayStatusScreen(bool fullRedraw);
uint8_t fitTextSize(const char* text, int maxWidth, uint8_t maxSize);
void drawFitCenteredText(int x, int y, int w, int h, const char* text,
                         uint8_t maxSize, uint16_t fg, uint16_t bg);
void showStationPage(int page, bool fullRedraw);
void applyHostConfig();
void showDisplayLockedNotice();
void refreshStationLiveValues(bool force);
void displayScanningUID(String uid);
void displayResult(String status, String name, String id, String refNo, String claimTime, String msg);
void displayOfflineAlert();
void playBootAnimation();
void drawStationIdConfigProgress(unsigned long elapsedMs, bool holdToEnter);
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
float readBatteryVoltage();
int getBatteryPercentage(float voltage);
void drawStationCard(int x, int y, int w, int h, uint16_t borderColor, uint16_t bgColor);
void drawStationPillBadge(int x, int y, int w, int h, const char* text, uint16_t fgColor, uint16_t bgColor);
void drawStationBatteryHUD(int x, int y);
void drawStationTopBar(String title);
void drawStationBottomBar(String instruction);
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
int calculateSignalQuality(int rssi);
uint16_t getSignalColor(int rssi, bool isOnline);
int getActiveSignalBarsCount(int rssi, bool isOnline);
void drawSignalBars(int x, int y, int rssi, bool isOnline, uint16_t bg);
void updateTopRightHeaderSmooth(int rssi, bool online, bool forceRedraw = false);
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

float readBatteryVoltage() {
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(BATTERY_ADC_PIN);
    delay(2);
  }
  float avgRaw = sum / 8.0f;
  float pinVoltage = (avgRaw / 4095.0f) * 3.3f;
  return pinVoltage * 2.0f;
}

int getBatteryPercentage(float voltage) {
  if (voltage >= 4.15f) return 100;
  if (voltage <= 3.30f) return 0;
  int percent = (int)((voltage - 3.30f) / (4.15f - 3.30f) * 100.0f);
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  return percent;
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


void updateShopLabel() {
  snprintf(dynamicShopLabel, sizeof(dynamicShopLabel), "STATION %d", currentStationId);
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
  if (err == ESP_OK || err == ESP_ERR_ESPNOW_EXIST) { hostPeerReady = true; return true; }
  hostPeerReady = false;
  return false;
}

bool sendToHost(const uint8_t *data, size_t len) {
  if (ensureHostPeer()) {
    esp_err_t err = esp_now_send(hostMacAddress, data, len);
    if (err == ESP_OK) return true;
  }
  return esp_now_send(broadcastAddress, data, len) == ESP_OK;
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

// ---------------------------------------------------------------------------
// ทำตามคำสั่งการแสดงผลของแม่ข่าย เรียกจากลูปหลักเท่านั้น
//
// ธีมถูกบังคับเสมอ ส่วนไฟหน้าจอกับโหมดพักหน้าจอสั่งเฉพาะตอน modeSeq เปลี่ยน
// เพื่อไม่ให้คำสั่งเดิมที่ย้ำมากับ heartbeat ไปรีเซ็ตสิ่งที่กำลังทำอยู่
// ---------------------------------------------------------------------------
// ย่อขนาดฟอนต์ลงจนข้อความพอดีความกว้างที่กำหนด คืนขนาดที่ใช้ได้จริง
// มีไว้เพราะข้อความหลายอันยาวไม่เท่ากัน ถ้าตั้งขนาดตายตัวจะล้นขอบจอ

// วางข้อความกึ่งกลางกรอบ โดยเลือกขนาดฟอนต์ที่ใหญ่ที่สุดที่ยังไม่ล้น

// สลับหน้าแบบรวมศูนย์ ของเดิมเขียนเงื่อนไขสามบรรทัดนี้ซ้ำอยู่สามที่
// และลืมตั้ง currentState ให้ตรงกับหน้าที่แสดงอยู่ ทำให้หน้า 3 ค้างนิ่งไม่อัปเดต

// [122.0.0] เพิ่ม: ทำตามคำสั่งโหมดการแสดงผลที่แม่ข่ายส่งมา
//           ธีมบังคับเสมอ ส่วนไฟจอกับการพักจอสั่งเมื่อค่า modeSeq เปลี่ยน
void applyHostConfig() {
  uint8_t wantDark, wantScreenOn, wantSaver, seq;
  portENTER_CRITICAL(&espnowMux);
  pendingConfigUpdate = false;
  wantDark     = cfgDark;
  wantScreenOn = cfgScreenOn;
  wantSaver    = cfgScreensaver;
  seq          = cfgModeSeq;
  portEXIT_CRITICAL(&espnowMux);

  bool busy = (currentState == STATE_SCANNING_SENT || currentState == STATE_RESULT_DISPLAY ||
               currentState == STATE_CONFIG_ID);

  if ((wantDark != 0) != isStationDarkMode) {
    isStationDarkMode = (wantDark != 0);
    stationPrefs.begin("station_cfg", false);
    stationPrefs.putBool("dark", isStationDarkMode);
    stationPrefs.end();
    // [122.7.0] แก้: ของเดิมเรียก showStationPage() เสมอ ซึ่งตั้ง currentState
    //           เป็น STANDBY หรือ STATUS ทุกครั้ง ถ้าตอนนั้นสถานีพักหน้าจออยู่
    //           การสั่งเปลี่ยนธีมอย่างเดียวจะเตะมันออกจากหน้าพักจอ แล้วไม่กลับเข้าไปอีก
    //           เพราะ modeSeq ไม่ได้เปลี่ยน คำสั่งพักจอจึงไม่ถูกทำซ้ำ
    //           ตอนนี้วาดใหม่ตามหน้าที่แสดงอยู่จริง ไม่เปลี่ยนสถานะของเครื่อง
    if (!busy && isScreenOn) {
      if (currentState == STATE_SCREENSAVER) renderScreensaver(true);
      else                                   showStationPage(currentStationPage, true);
    }
  }

  if (seq == lastAppliedModeSeq) return;   // คำสั่งเดิมที่ย้ำมา ไม่ต้องทำซ้ำ
  lastAppliedModeSeq = seq;
  if (busy) return;

  if ((wantScreenOn != 0) != isScreenOn) {
    setScreenPower(wantScreenOn != 0);
    if (!isScreenOn) ledOff(); else ledStandby();
  }
  if (!isScreenOn) return;

  if (wantSaver && currentState != STATE_SCREENSAVER) {
    currentState = STATE_SCREENSAVER;
    renderScreensaver(true);
  } else if (!wantSaver && currentState == STATE_SCREENSAVER) {
    currentState = STATE_STANDBY;
    showStationPage(currentStationPage, true);
  }
}

// แจ้งว่าเรื่องหน้าจอเป็นของแม่ข่าย ไม่ใช่ความผิดพลาดของเครื่อง










// ============================================================================
// LIVE SCAN ALERT (FULL-SCREEN COLOR TAKEOVER FOR STATION)
// ============================================================================

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  const uint8_t *mac = recv_info->src_addr;
  lastHostRssi = recv_info->rx_ctrl->rssi;

  // คำสั่งการแสดงผลจากแม่ข่าย แยกออกจากแพ็กเก็ตตอบกลับด้วยขนาด
  if (len == sizeof(HostConfigPacket)) {
    HostConfigPacket cfg;
    memcpy(&cfg, data, sizeof(cfg));
    if (cfg.magic != ESPNOW_PROTO_MAGIC || cfg.version != ESPNOW_PROTO_VER) return;
    if (cfg.msgType != MSG_CONFIG) return;
    if (cfg.stationId != 0 && cfg.stationId != currentStationId) return;   // 0 = ทุกสถานี
    portENTER_CRITICAL_ISR(&espnowMux);
    cfgDark        = cfg.darkMode ? 1 : 0;
    cfgScreenOn    = cfg.screenOn ? 1 : 0;
    cfgScreensaver = cfg.screensaver ? 1 : 0;
    cfgModeSeq     = cfg.modeSeq;
    pendingConfigUpdate = true;
    portEXIT_CRITICAL_ISR(&espnowMux);
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
    strncpy(pendingHostTime, pkt.claimTime, sizeof(pendingHostTime) - 1);
    pendingHostTime[sizeof(pendingHostTime) - 1] = '\0';
    pendingTimeSync = true;
  }

  if (pkt.msgType == MSG_HEARTBEAT) {
    pendingServedCount = pkt.servedCount;
    hasServedCountUpdate = true;
    return;
  }

  if (pkt.msgType == MSG_SCAN_RESP) {
    if (currentState == STATE_SCANNING_SENT && pkt.seq == pendingScanSeq) {
      memcpy(&receivedPacketBuffer, &pkt, sizeof(pkt));
      hasNewPacket = true;
    }
  }
}
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  (void)tx_info;
  (void)status;
}
#else
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  lastHostRssi = -60;

  // คำสั่งการแสดงผลจากแม่ข่าย แยกออกจากแพ็กเก็ตตอบกลับด้วยขนาด
  if (len == sizeof(HostConfigPacket)) {
    HostConfigPacket cfg;
    memcpy(&cfg, data, sizeof(cfg));
    if (cfg.magic != ESPNOW_PROTO_MAGIC || cfg.version != ESPNOW_PROTO_VER) return;
    if (cfg.msgType != MSG_CONFIG) return;
    if (cfg.stationId != 0 && cfg.stationId != currentStationId) return;   // 0 = ทุกสถานี
    portENTER_CRITICAL_ISR(&espnowMux);
    cfgDark        = cfg.darkMode ? 1 : 0;
    cfgScreenOn    = cfg.screenOn ? 1 : 0;
    cfgScreensaver = cfg.screensaver ? 1 : 0;
    cfgModeSeq     = cfg.modeSeq;
    pendingConfigUpdate = true;
    portEXIT_CRITICAL_ISR(&espnowMux);
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
    strncpy(pendingHostTime, pkt.claimTime, sizeof(pendingHostTime) - 1);
    pendingHostTime[sizeof(pendingHostTime) - 1] = '\0';
    pendingTimeSync = true;
  }

  if (pkt.msgType == MSG_HEARTBEAT) {
    pendingServedCount = pkt.servedCount;
    hasServedCountUpdate = true;
  } else if (pkt.msgType == MSG_SCAN_RESP &&
             currentState == STATE_SCANNING_SENT &&
             pkt.seq == pendingScanSeq) {
    memcpy(&receivedPacketBuffer, &pkt, sizeof(pkt));
    hasNewPacket = true;
  }
}
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  (void)mac_addr;
  (void)status;
}
#endif


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
  drawStationTopBar("STATION NUMBER");
  drawStationCard(16, 36, 288, 166, getStYellow(), getStCardBg());

  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(32, 48);
  tft.println("CHOOSE THIS STATION");

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
  tft.print("Let go and wait 3 seconds to save");
  drawStationBottomBar("Saves by itself");

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
  tft.println("SAVED");
  tft.setTextColor(getStCyan(), getStCardBg());
  tft.setTextSize(3);
  tft.setCursor(130, 126);
  tft.printf("0%d", currentStationId);
  delay(700);

  currentState = STATE_STANDBY;
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

  if (btnState && btnWasPressed && currentState == STATE_STANDBY) {
    unsigned long held = millis() - btnPressStart;
    if (held >= 200 && !holdUiShown) {
      wakeScreenIfNeeded();
      tft.fillScreen(getStBg());
      drawStationTopBar("STATION NUMBER");
      drawStationCard(16, 36, 288, 166, getStYellow(), getStCardBg());
      tft.setTextColor(getStTextMain(), getStCardBg());
      tft.setTextSize(2);
      tft.setCursor(44, 64);
      tft.println("KEEP HOLDING");
      tft.setTextColor(getStTextMuted(), getStCardBg());
      tft.setTextSize(1);
      tft.setCursor(60, 96);
      tft.println("Hold the button for 3 seconds");
      drawStationIdConfigProgress(held, true);
      drawStationBottomBar("Let go after 3 seconds");
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

    if (holdUiShown) {
      holdUiShown = false;
      if (pressDuration < STATION_ID_HOLD_MS && currentState == STATE_STANDBY) {
        showStationPage(currentStationPage, true);
      }
      clickCount = 0;
      return;
    }

    if (currentState == STATE_SCREENSAVER || currentState == STATE_CREDIT || !isScreenOn) {
      wakeScreenIfNeeded();
      currentState = STATE_STANDBY;
      currentStationPage = 1;
      soundHomeBeep();
      displayTapCardStandby();
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
      // เดิมกดค้างแล้วปิดจอเองได้ ตอนนี้เครื่องแม่ข่ายเป็นผู้กำหนดฝ่ายเดียว
      showDisplayLockedNotice();
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
      currentStationPage = (currentStationPage % 3) + 1;
      soundClick();
      showStationPage(currentStationPage, true);
    }
    else if (clickCount == 2) {
      currentState = STATE_SCREENSAVER;
      soundScreensaverBeep();
      renderScreensaver(true);
    }
    else if (clickCount == 3) {
      // เดิมกดสามครั้งแล้วสลับธีมของเครื่องนี้เองได้
      // ตอนนี้เครื่องแม่ข่ายเป็นผู้กำหนดธีมของทุกจุดบริการฝ่ายเดียว
      // ถ้าปล่อยให้สลับเองที่นี่ แม่ข่ายจะสั่งกลับในไม่กี่วินาที จอจะกระพริบเปล่า ๆ
      showDisplayLockedNotice();
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
  stateHoldUntil = millis() + SCAN_TIMEOUT_MS;
}

void checkRC522() {
  if (currentState == STATE_SCANNING_SENT || currentState == STATE_RESULT_DISPLAY) return;

  if (millis() - lastRc522HealthCheck > 10000) {
    lastRc522HealthCheck = millis();
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (v == 0x00 || v == 0xFF) {
      // [122.6.0] แก้: ของเดิมสั่งเริ่มใหม่แล้วจบ ไม่มีใครรู้ว่าสำเร็จหรือไม่
      //           อ่านซ้ำหลังเริ่มใหม่ แล้วบันทึกผลไว้ให้สัญลักษณ์บนหน้าแรกใช้
      mfrc522.PCD_Init();
      v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    }
    isReaderReady = (v != 0x00 && v != 0xFF);
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

// ============================================================================
// ส่วนที่แยกออกไปเป็นไฟล์ของตัวเอง Arduino IDE จะแสดงเป็นแท็บของสเก็ตช์เดียวกัน
// วางบรรทัด #include ไว้ตรงนี้เพราะโค้ดข้างในอ้างถึงตัวแปรส่วนกลางข้างบน
// **อย่าย้ายขึ้นไปไว้บนสุดของไฟล์** จะคอมไพล์ไม่ผ่าน
// ============================================================================
#include "StationScreen.h"

void setup() {
  pinMode(BUZZER_PIN, OUTPUT); noTone(BUZZER_PIN);
  pinMode(TFT_BLK, OUTPUT); setScreenPower(true);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(RGB_LED_PIN, OUTPUT);
  
  analogSetAttenuation(ADC_11db);

  SPI_TFT.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  SPI.begin(RC522_SCK, RC522_MISO, RC522_MOSI, RC522_SS);

  tft.init(240, 320); 
  tft.setRotation(1); 
  tft.setTextWrap(false);

  // คาลิเบทพาเนลจอตามไฟล์ TFT Color Calibration Tool
  tft.invertDisplay(false);

  mfrc522.PCD_Init();
  // [122.6.0] เพิ่ม: อ่านทะเบียนรุ่นทันทีหลังเริ่มต้น เพื่อให้สัญลักษณ์บนหน้าแรก
  //           บอกความพร้อมได้ถูกต้องตั้งแต่วินาทีแรก ไม่ต้องรอรอบตรวจรอบแรก
  {
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    isReaderReady = (v != 0x00 && v != 0xFF);
  }

  stationPrefs.begin("st_cfg", false);
  currentStationId = stationPrefs.getUChar("id", 1);
  isStationDarkMode = stationPrefs.getBool("dark", true);
  stationPrefs.end();
  updateShopLabel();

  stationPrefs.begin("st_stats", false);
  totalSuccessToday = stationPrefs.getUInt("served", 0);
  stationPrefs.end();

  nextHeartbeatInterval = BASE_HEARTBEAT + (currentStationId * 350) + random(0, 200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // [122.10.0] แก้: ต้นเหตุที่คำสั่งจากแม่ข่ายมาถึงช้าราวห้าวินาที
  //           ต้องเรียก WiFi.setSleep(false) ไม่ใช่ esp_wifi_set_ps() เดี่ยว ๆ
  //
  //           Arduino core เก็บค่าโหมดประหยัดพลังงานที่ต้องการไว้ในตัวแปรของตัวเอง
  //           ค่าเริ่มต้นบน ESP32-S3 คือ WIFI_PS_MIN_MODEM และเมื่ออีเวนต์
  //           ARDUINO_EVENT_WIFI_STA_START มาถึง core จะสั่ง
  //           esp_wifi_set_ps(WiFi.getSleep()) ทับค่าที่เราตั้งไว้
  //           อีเวนต์นั้นเป็นแบบอะซิงโครนัส มักมาถึงหลังบรรทัดนี้ไปแล้ว
  //           ค่าที่เราตั้งจึงถูกเปลี่ยนกลับเป็นโหมดประหยัดพลังงานเงียบ ๆ
  //
  //           ผลคือวิทยุของสถานีหลับเป็นช่วง ๆ รับแพ็กเก็ตที่ส่งมาแบบไม่ได้นัดหมาย
  //           ไม่ค่อยได้ จะได้แน่ ๆ ก็ตอนที่เพิ่งส่ง heartbeat ออกไปเองเท่านั้น
  //           ซึ่งเกิดทุก 6000 + หมายเลขสถานี x 350 มิลลิวินาที ตรงกับที่พบ
  //
  //           WiFi.setSleep(false) เปลี่ยนทั้งค่าที่ core จำไว้และค่าที่ใช้งานจริง
  //           อีเวนต์ที่ตามมาภายหลังจึงสั่งค่าเดิมซ้ำ ไม่ได้ทับให้กลับไปหลับอีก
  WiFi.setSleep(false);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  esp_wifi_set_max_tx_power(68);

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

  playBootAnimation();
  lastActivityTime = millis();
  currentStationPage = 1;
  displayTapCardStandby();

  for (int i = 0; i < 3; i++) {
    StationPacket hbPkt = {};
    hbPkt.magic = ESPNOW_PROTO_MAGIC;
    hbPkt.version = ESPNOW_PROTO_VER;
    hbPkt.msgType = MSG_HEARTBEAT;
    hbPkt.stationId = currentStationId;
    hbPkt.seq = 0;
    hbPkt.systemVoltage = readBatteryVoltage();
    sendToHost((uint8_t *)&hbPkt, sizeof(StationPacket));
    delay(30);
  }

  lastHeartbeatTime = millis();
}

void loop() {
  handlePhysicalButton();
  calculateCpuLoad();

  if (currentState == STATE_STATUS && isScreenOn && (millis() - lastCpuDisplayUpdate >= 500)) {
    displayStatusScreen(false);
  }

  if (hasServedCountUpdate) {
    hasServedCountUpdate = false;
    totalSuccessToday = pendingServedCount;
    stationPrefs.begin("st_stats", false);
    stationPrefs.putUInt("served", totalSuccessToday);
    stationPrefs.end();

    // [122.2.0] แก้: ของเดิมวาดทับด้วยพิกัดและขนาดตัวอักษรของหน้าจอชุดเก่า
    //           พอออกแบบหน้าจอใหม่ ตัวเลขจึงไปโผล่ผิดที่และทับของเดิมไม่มิด
    //           ตอนนี้เรียกตัวที่อยู่ข้างเดียวกับโค้ดวาดหน้านั้นแทน
    refreshStationLiveValues(false);
  }

  if (currentState == STATE_SCANNING_SENT &&
      !hasNewPacket &&
      scanRetryCount < 3 &&
      millis() - lastScanSendTime >= 450) {
    sendToHost((uint8_t *)&pendingScanPacket, sizeof(StationPacket));
    scanRetryCount++;
    lastScanSendTime = millis();
  }

  if (hasNewPacket) {
    hasNewPacket = false;
    displayResult(String(receivedPacketBuffer.status), 
                  String(receivedPacketBuffer.name), 
                  String(receivedPacketBuffer.studentId), 
                  String(receivedPacketBuffer.refNo), 
                  String(receivedPacketBuffer.claimTime), 
                  String(receivedPacketBuffer.message));
    scanRetryCount = 3;
    currentState = STATE_RESULT_DISPLAY;
    stateHoldUntil = millis() + 4000;
  }

  if (currentState == STATE_SCANNING_SENT && millis() > stateHoldUntil) {
    displayOfflineAlert();
    currentState = STATE_RESULT_DISPLAY;
    stateHoldUntil = millis() + 2500;
  }

  if (currentState == STATE_RESULT_DISPLAY && millis() > stateHoldUntil) {
    currentState = STATE_STANDBY;
    currentStationPage = 1;
    displayTapCardStandby();
  }

  if (millis() - lastHostAckTime > HOST_OFFLINE_TIMEOUT) {
    if (isHostOnline) {
      isHostOnline = false;
    }
  }

  if (pendingTimeSync) {
    char timeCopy[24];
    noInterrupts();
    strncpy(timeCopy, pendingHostTime, sizeof(timeCopy) - 1);
    timeCopy[sizeof(timeCopy) - 1] = '\0';
    pendingTimeSync = false;
    interrupts();
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
    hbPkt.systemVoltage = readBatteryVoltage();
    sendToHost((uint8_t *)&hbPkt, sizeof(StationPacket));
  }

  // เดิมสถานีนับถอยหลัง 5 นาทีแล้วเข้าโหมดพักหน้าจอเอง ตอนนี้ตัดออกทั้งหมด
  // เครื่องแม่ข่ายเป็นผู้สั่งเข้า/ออกโหมดพักหน้าจอฝ่ายเดียว ผ่าน MSG_CONFIG
  if (pendingConfigUpdate) applyHostConfig();

  // [122.2.0] เพิ่ม: เดิมหน้าแรกกับหน้าสองวาดครั้งเดียวตอนเข้าหน้านั้น
  //           นาฬิกาจึงค้าง และสถานะออนไลน์/ออฟไลน์ไม่ขยับจนกว่าจะกดเปลี่ยนหน้า
  //           ตอนนี้ตรวจทุกหนึ่งวินาที แล้ววาดซ้ำเฉพาะค่าที่เปลี่ยนจริง
  if (isScreenOn && (millis() - lastClockRefresh >= 1000)) {
    lastClockRefresh = millis();
    if (currentState == STATE_SCREENSAVER) renderScreensaver(false);
    refreshStationLiveValues(false);
  }

  checkRC522();

  delay(2);
}