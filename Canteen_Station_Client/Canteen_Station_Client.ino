/**
 * ============================================================================
 * Project: Meal Subsidy Management System (Tuesday 35-Baht Quota)
 * System: Vendor Station Client & Dynamic Theme Suite
 * Version: 118.0.0 (Hardened: HSPI Display Bus, Live Header, Theme Sync)
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

#define APP_VERSION         "118.0.0"
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

typedef struct __attribute__((packed)) {
  uint8_t magic;
  uint8_t version;
  uint8_t msgType;
  uint8_t stationId;
  uint8_t darkMode;
} HostConfigPacket;

static_assert(sizeof(StationPacket) == 50, "StationPacket size mismatch");
static_assert(sizeof(HostResponsePacket) == 200, "HostResponsePacket size mismatch");
static_assert(sizeof(HostConfigPacket) == 5, "HostConfigPacket size mismatch");

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

volatile bool pendingThemeUpdate = false;
volatile uint8_t pendingThemeDark = 1;
portMUX_TYPE espnowMux = portMUX_INITIALIZER_UNLOCKED;

unsigned long lastHeaderRefresh = 0;
const unsigned long HEADER_REFRESH_MS = 1000;

uint16_t nextScanSeq = 1;
uint16_t pendingScanSeq = 0;
StationPacket pendingScanPacket = {};
uint8_t scanRetryCount = 0;
unsigned long lastScanSendTime = 0;

const unsigned long TIMEOUT_SCREENSAVER = 300000; 
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
void displayScanningUID(String uid);
void displayResult(String status, String name, String id, String refNo, String claimTime, String msg);
void displayOfflineAlert();
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
void applyPendingTheme();
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

  drawStationCard(16, 36, 288, 128, getStGreen(), getStCardBg());
  drawStationPillBadge(32, 48, 256, 20, "READY FOR RFID CARD TAP", isStationDarkMode ? 0x0000 : 0xFFFF, getStGreen());

  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(3);
  tft.setCursor(48, 86);
  tft.println("TAP CARD HERE");

  tft.setTextColor(getStYellow(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(68, 126);
  tft.println("Subsidy Quota: 35 THB / Day");

  drawStationCard(16, 172, 288, 38, getStCardBorder(), getStCardBg());
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(28, 184);
  tft.println("Protocol: ESP-NOW Channel 1 Secured");

  drawStationBottomBar("PAGE 1/3 | PRESS BUTTON TO CYCLE");
}

void displayStatsDashboard() {
  ledStandby();
  tft.fillScreen(getStBg());

  drawStationTopBar(String(dynamicShopLabel) + " STATS");

  drawStationCard(6, 30, 150, 130, getStGreen(), getStCardBg());
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(14, 38);
  tft.println("TODAY SERVED");
  drawStationPillBadge(88, 36, 62, 14, isHostOnline ? "ONLINE" : "OFFLINE", isHostOnline ? (isStationDarkMode ? 0x0000 : 0xFFFF) : 0xFFFF, isHostOnline ? getStGreen() : getStRose());

  tft.setTextColor(getStGreen(), getStCardBg());
  tft.setTextSize(3);
  tft.setCursor(14, 62);
  tft.printf("%u", (unsigned)totalSuccessToday);
  tft.setTextSize(1);
  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.print(" pax");

  tft.setTextColor(getStYellow(), getStCardBg());
  tft.setTextSize(2);
  tft.setCursor(14, 110);
  tft.printf("%u B.", (unsigned)(totalSuccessToday * 35));
  tft.setTextSize(1);
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setCursor(14, 136);
  tft.println("Total Payout");

  drawStationCard(164, 30, 150, 130, getStCardBorder(), getStCardBg());
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(172, 38);
  tft.println("SYSTEM STATUS");

  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(172, 60); tft.println("QUOTA : 35 THB");
  tft.setCursor(172, 78); tft.println("LIMIT : 1 TIME");
  tft.setCursor(172, 96); tft.println("RADIO : ESP-NOW");
  tft.setTextColor(getStCyan(), getStCardBg());
  tft.setCursor(172, 118); tft.println("CH 1 LOCKED");

  drawStationBottomBar("PAGE 2/3 | PRESS BUTTON TO CYCLE");
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
  else if (page == 2) displayStatsDashboard();
  else                displayStatusScreen(fullRedraw);
}

// รับคำสั่งสลับธีมจากเครื่องแม่ข่าย (MSG_CONFIG) แล้ววาดหน้าปัจจุบันใหม่
void applyPendingTheme() {
  bool wantDark;
  portENTER_CRITICAL(&espnowMux);
  pendingThemeUpdate = false;
  wantDark = (pendingThemeDark != 0);
  portEXIT_CRITICAL(&espnowMux);

  if (wantDark == isStationDarkMode) return;
  isStationDarkMode = wantDark;

  stationPrefs.begin("st_cfg", false);
  stationPrefs.putBool("dark", isStationDarkMode);
  stationPrefs.end();

  if (!isScreenOn) return;
  soundThemeSwitch();
  if (currentState == STATE_SCREENSAVER)   renderScreensaver(true);
  else if (currentState == STATE_CREDIT)   renderDeveloperCredit();
  else if (currentState == STATE_STANDBY ||
           currentState == STATE_STATUS)   showStationPage(currentStationPage, true);
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
    pendingThemeDark = cfg.darkMode ? 1 : 0;
    pendingThemeUpdate = true;
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
    portENTER_CRITICAL_ISR(&espnowMux);
    strncpy(pendingHostTime, pkt.claimTime, sizeof(pendingHostTime) - 1);
    pendingHostTime[sizeof(pendingHostTime) - 1] = '\0';
    pendingTimeSync = true;
    portEXIT_CRITICAL_ISR(&espnowMux);
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

  if (len == sizeof(HostConfigPacket)) {
    HostConfigPacket cfg;
    memcpy(&cfg, data, sizeof(cfg));
    if (cfg.magic != ESPNOW_PROTO_MAGIC || cfg.version != ESPNOW_PROTO_VER) return;
    if (cfg.msgType != MSG_CONFIG) return;
    // stationId == 0 คือ broadcast ถึงทุกสถานี
    if (cfg.stationId != 0 && cfg.stationId != currentStationId) return;
    portENTER_CRITICAL_ISR(&espnowMux);
    pendingThemeDark = cfg.darkMode ? 1 : 0;
    pendingThemeUpdate = true;
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
    portENTER_CRITICAL_ISR(&espnowMux);
    strncpy(pendingHostTime, pkt.claimTime, sizeof(pendingHostTime) - 1);
    pendingHostTime[sizeof(pendingHostTime) - 1] = '\0';
    pendingTimeSync = true;
    portEXIT_CRITICAL_ISR(&espnowMux);
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
      isStationDarkMode = !isStationDarkMode;
      stationPrefs.begin("st_cfg", false);
      stationPrefs.putBool("dark", isStationDarkMode);
      stationPrefs.end();
      soundThemeSwitch();
      if (isScreenOn) {
        if (currentState == STATE_SCREENSAVER)      renderScreensaver(true);
        else if (currentState == STATE_CREDIT)      renderDeveloperCredit();
        else if (currentState == STATE_STANDBY ||
                 currentState == STATE_STATUS)      showStationPage(currentStationPage, true);
      }
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

  nextHeartbeatInterval = BASE_HEARTBEAT + (currentStationId * 350) + random(0, 200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
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
  showStationPage(1, true);

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
    uint32_t hostServed = pendingServedCount;
    // เขียน NVS เฉพาะตอนค่าจริง ๆ เปลี่ยน เพื่อลดการสึกหรอของแฟลช (heartbeat มาทุก 6 วินาที)
    if (hostServed != totalSuccessToday) {
      totalSuccessToday = hostServed;
      stationPrefs.begin("st_stats", false);
      stationPrefs.putUInt("served", totalSuccessToday);
      stationPrefs.end();
    }

    if (currentState == STATE_STANDBY && currentStationPage == 2 && isScreenOn) {
      tft.fillRect(12, 60, 90, 28, getStCardBg());
      tft.setTextColor(getStGreen(), getStCardBg());
      tft.setTextSize(3);
      tft.setCursor(14, 62);
      tft.printf("%u", (unsigned)totalSuccessToday);
      tft.fillRect(12, 108, 130, 22, getStCardBg());
      tft.setTextColor(getStYellow(), getStCardBg());
      tft.setTextSize(2);
      tft.setCursor(14, 110);
      tft.printf("%u B.", (unsigned)(totalSuccessToday * 35));
    }
  }

  if (pendingThemeUpdate) applyPendingTheme();

  // รีเฟรชแถบสถานะลิงก์/แบตเตอรี่มุมขวาบนของหน้าปกติ
  if (isScreenOn && (currentState == STATE_STANDBY || currentState == STATE_STATUS) &&
      (millis() - lastHeaderRefresh >= HEADER_REFRESH_MS)) {
    lastHeaderRefresh = millis();
    updateStationHeaderStatus(false);
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
    // นับยอดที่นี่ ไม่ใช่ซ่อนไว้ใน displayResult() ซึ่งเป็นฟังก์ชันวาดจอ
    if (strcmp(receivedPacketBuffer.status, "SUCCESS") == 0) totalSuccessToday++;
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
    sendToHost((uint8_t *)&hbPkt, sizeof(StationPacket));
  }

  if (currentState != STATE_SCREENSAVER && currentState != STATE_CREDIT && 
      currentState != STATE_SCANNING_SENT && currentState != STATE_RESULT_DISPLAY && 
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