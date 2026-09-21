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

#define APP_VERSION         "108.0.0"
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

struct StationNode {
  bool isOnline = false;
  int rssi = -100;
  float systemVoltage = 0.0;
  unsigned long lastSeen = 0;
  uint8_t mac[6] = {0};
};
StationNode stationNodes[4];
bool stationPeerReady[4] = {false, false, false, false};
bool stationThemeSent[4] = {false, false, false, false};
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
void sendStationTheme(uint8_t stationId);
void broadcastStationTheme();
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

int currentHostPage         = 0;
const int TOTAL_PAGES       = 3;
bool isScreensaverActive    = false;
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

void drawBentoCard(int x, int y, int w, int h, uint16_t borderColor, uint16_t bgColor) {
  tft.fillRoundRect(x, y, w, h, 8, bgColor);
  tft.drawRoundRect(x, y, w, h, 8, borderColor);
}

void drawBentoPillBadge(int x, int y, int w, int h, const char* text, uint16_t fgColor, uint16_t bgColor) {
  tft.fillRoundRect(x, y, w, h, h / 2, bgColor);
  tft.setTextColor(fgColor, bgColor);
  tft.setTextSize(1);
  int textLen = strlen(text) * 6;
  int posX = x + (w - textLen) / 2;
  tft.setCursor(max(x + 2, posX), y + (h - 8) / 2);
  tft.print(text);
}

void drawHostBatteryHUD(int x, int y) {
  float volt = readHostBatteryVoltage();
  int pct = getHostBatteryPercentage(volt);

  tft.fillRect(x, y, 54, 18, getTftBg());
  tft.drawRect(x, y + 2, 44, 14, getTftTextMain());
  tft.fillRect(x + 44, y + 6, 3, 6, getTftTextMain());

  int fillWidth = (pct * 40) / 100;
  if (fillWidth < 1 && pct > 0) fillWidth = 1;

  uint16_t fillColor = (pct > 20) ? getTftAccentGreen() : getTftAccentRose();
  if (volt > 4.0f) fillColor = getTftAccentCyan();

  tft.fillRect(x + 2, y + 4, fillWidth, 10, fillColor);

  String pctStr = String(pct) + "%";
  int textWidth = pctStr.length() * 6;
  int textX = x + 2 + (40 - textWidth) / 2; 
  int textY = y + 6;

  tft.setTextSize(1);
  uint16_t textBg = (fillWidth > 22) ? fillColor : getTftBg();
  tft.setTextColor((fillWidth > 22) ? 0x0000 : getTftTextMain(), textBg);
  tft.setCursor(textX, textY);
  tft.print(pctStr);
}

void drawMiniBattery(int x, int y, int pct) {
  tft.fillRect(x, y, 26, 12, getTftCardBg());
  tft.drawRect(x, y, 22, 12, getTftTextMain());
  tft.fillRect(x + 22, y + 4, 2, 4, getTftTextMain());
  int fillW = (pct * 18) / 100;
  if (fillW < 1 && pct > 0) fillW = 1;
  uint16_t fColor = (pct > 20) ? getTftAccentGreen() : getTftAccentRose();
  tft.fillRect(x + 2, y + 2, fillW, 8, fColor);
}

void drawHostTopBar(String title) {
  tft.fillRect(0, 0, 320, 26, getTftBg());
  tft.drawFastHLine(0, 26, 320, getTftCardBorder());
  tft.setTextColor(getTftAccentCyan(), getTftBg());
  tft.setTextSize(1);
  tft.setCursor(8, 9);
  tft.println(title);
  drawHostBatteryHUD(252, 4);
}

void drawBentoBottomBar(String instruction) {
  tft.fillRect(0, 214, 320, 26, getTftBg());
  tft.drawFastHLine(0, 214, 320, getTftCardBorder());
  tft.setTextColor(getTftTextMuted(), getTftBg());
  tft.setTextSize(1);
  tft.setCursor(12, 222);
  tft.println(instruction);
}

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

void sendStationTheme(uint8_t stationId) {
  HostConfigPacket cfg = {};
  cfg.magic = ESPNOW_PROTO_MAGIC;
  cfg.version = ESPNOW_PROTO_VER;
  cfg.msgType = MSG_CONFIG;
  cfg.stationId = stationId;
  cfg.darkMode = isTftDarkMode ? 1 : 0;
  sendToStation(stationId, (uint8_t *)&cfg, sizeof(cfg));
}

void broadcastStationTheme() {
  HostConfigPacket cfg = {};
  cfg.magic = ESPNOW_PROTO_MAGIC;
  cfg.version = ESPNOW_PROTO_VER;
  cfg.msgType = MSG_CONFIG;
  cfg.stationId = 0;
  cfg.darkMode = isTftDarkMode ? 1 : 0;
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

String getLoginHTML(const String &errorMsg) {
  String html = R"rawliteral(<!DOCTYPE html>
<html lang="th">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>MCU Phrae - Staff Authentication</title>
  <link href="https://fonts.googleapis.com/css2?family=Plus+Jakarta+Sans:wght@400;600;700;800&family=Sarabun:wght@300;400;500;600;700&display=swap" rel="stylesheet">
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Plus Jakarta Sans', 'Sarabun', sans-serif; }
    body { background: radial-gradient(circle at top, #1e293b, #090d16); color: #f8fafc; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 1.5rem; }
    .login-card { background: rgba(30, 41, 59, 0.7); border: 1px solid rgba(255,255,255,0.08); backdrop-filter: blur(16px); border-radius: 2rem; max-width: 26rem; width: 100%; padding: 2.5rem; box-shadow: 0 25px 50px -12px rgba(0,0,0,0.5); }
    .brand-badge { background: #10b981; color: #064e3b; font-size: 0.72rem; font-weight: 800; padding: 0.3rem 0.75rem; border-radius: 9999px; display: inline-block; margin-bottom: 0.9rem; letter-spacing: 0.05em; }
    .title { font-size: 1.45rem; font-weight: 800; color: white; letter-spacing: -0.02em; margin-bottom: 0.3rem; }
    .subtitle { font-size: 0.88rem; color: #94a3b8; margin-bottom: 2rem; }
    .form-group { margin-bottom: 1.25rem; text-align: left; }
    .form-label { display: block; font-size: 0.85rem; font-weight: 600; margin-bottom: 0.45rem; color: #cbd5e1; }
    .form-input { width: 100%; padding: 0.85rem 1.1rem; border-radius: 1rem; border: 1px solid rgba(255,255,255,0.1); background: rgba(15, 23, 42, 0.6); color: white; font-size: 0.95rem; outline: none; transition: all 0.2s; }
    .form-input:focus { border-color: #10b981; box-shadow: 0 0 0 3px rgba(16,185,129,0.2); }
    .btn-login { width: 100%; padding: 0.9rem; border-radius: 1rem; font-size: 0.98rem; font-weight: 700; background: #10b981; color: #064e3b; border: none; cursor: pointer; transition: all 0.2s; margin-top: 0.6rem; }
    .btn-login:hover { background: #059669; color: white; transform: translateY(-1px); }
    .error-box { background: rgba(239, 68, 68, 0.15); border: 1px solid #ef4444; color: #fca5a5; border-radius: 1rem; padding: 0.8rem 1rem; font-size: 0.85rem; font-weight: 600; margin-bottom: 1.25rem; }
    .footer { text-align: center; margin-top: 2rem; font-size: 0.8rem; color: #64748b; line-height: 1.5; }
  </style>
</head>
<body>
  <div class="login-card">
    <div style="text-align: center;">
      <span class="brand-badge">EXECUTIVE BENTO PORTAL</span>
      <h1 class="title">Smart Canteen System</h1>
      <p class="subtitle">ระบบบริหารจัดการอาหารกลางวันนิสิต มจร. แพร่</p>
    </div>
)rawliteral";

  if (errorMsg.length() > 0) {
    html += "<div class='error-box'>⚠️ " + htmlEscape(errorMsg) + "</div>";
  }

  html += R"rawliteral(
    <form method="POST" action="/login">
      <div class="form-group">
        <label class="form-label">ชื่อผู้ใช้ (Username):</label>
        <input type="text" name="username" required class="form-input" placeholder="e.g. admin" autofocus>
      </div>
      <div class="form-group">
        <label class="form-label">รหัสผ่าน (Password):</label>
        <input type="password" name="password" required class="form-input" placeholder="••••••••">
      </div>
      <button type="submit" class="btn-login">เข้าสู่ระบบ (Sign In)</button>
    </form>
    <div class="footer">
      มจร. วิทยาเขตแพร่ (MCU Phrae Campus)<br>
      Authorized Officers Only
    </div>
  </div>
</body>
</html>)rawliteral";
  return html;
}

void playBootAnimation() {
  setLedColor(0, 30, 60);
  tft.fillScreen(ST77XX_BLACK);
  int centerX = 160; int centerY = 110;
  for (int r = 10; r <= 85; r += 9) {
    tft.drawCircle(centerX, centerY, r, ST77XX_BLUE);
    tft.drawCircle(centerX, centerY, r + 2, ST77XX_CYAN);
    delay(20);
  }
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(2, 2, 316, 236, ST77XX_BLUE);
  tft.drawRect(4, 4, 312, 232, ST77XX_CYAN);
  
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(22, 30);
  tft.println("MCU PHRAE SMART CANTEEN");
  
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(44, 60);
  tft.println("CENTRAL ENTERPRISE GATEWAY SYSTEM");

  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setCursor(68, 76);
  tft.printf("ESP32-S3 N16R8 | PSRAM: %d MB\n", ESP.getPsramSize() / 1024 / 1024);

  int barX = 40; int barY = 120; int barW = 240; int barH = 16;
  tft.drawRoundRect(barX - 2, barY - 2, barW + 4, barH + 4, 4, ST77XX_WHITE);
  const char* loadSteps[] = { 
    "Mounting Storage...", 
    "Loading Student DB...", 
    "Initializing RTC DS3231...", 
    "Configuring OPI PSRAM...", 
    "Host System Ready!" 
  };
  for (int step = 0; step < 5; step++) {
    tft.fillRect(barX, barY + 28, barW, 14, ST77XX_BLACK);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(barX, barY + 28);
    tft.println(loadSteps[step]);
    int targetW = (barW * (step + 1)) / 5;
    tft.fillRoundRect(barX, barY, targetW, barH, 2, ST77XX_GREEN);
    delay(140);
  }
  soundWelcome();
  ledStandby();
  delay(200);
}

// ============================================================================
// BENTO TFT SCREENS (320x240 Modular High-Density Display)
// ============================================================================
void renderHostPage(bool fullRedraw) {
  if (isLiveScanDisplaying) return;
  if (isCreditActive) { renderDeveloperCredit(); return; }
  if (isScreensaverActive) { renderScreensaver(fullRedraw); return; }
  if (currentHostPage < 0 || currentHostPage >= TOTAL_PAGES) currentHostPage = 0;

  ledStandby();

  int usedCount = 0;
  int shopCounts[4] = {0, 0, 0, 0};
  for (const auto& s : db) {
    if (s.claimed) {
      usedCount++;
      if (s.station >= 1 && s.station <= 4) shopCounts[s.station - 1]++;
    }
  }

  if (fullRedraw) tft.fillScreen(getTftBg());

  if (currentHostPage == 0) {
    if (fullRedraw) {
      drawHostTopBar("[1/3] BENTO EXECUTIVE OVERVIEW");

      drawBentoCard(6, 32, 150, 96, getTftAccentGreen(), getTftCardBg());
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setTextSize(1);
      tft.setCursor(14, 40);
      tft.println("TODAY MEAL QUOTA");
      drawBentoPillBadge(112, 38, 38, 14, "35B", getTftAccentGreen(), isTftDarkMode ? 0x01E4 : 0xE7F8);

      drawBentoCard(164, 32, 150, 96, getTftCardBorder(), getTftCardBg());
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setTextSize(1);
      tft.setCursor(172, 40);
      tft.println("CORE TELEMETRY");

      drawBentoCard(6, 134, 150, 74, getTftAccentYellow(), getTftCardBg());
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setTextSize(1);
      tft.setCursor(14, 142);
      tft.println("TOTAL DISBURSED");

      drawBentoCard(164, 134, 150, 74, getTftCardBorder(), getTftCardBg());
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setTextSize(1);
      tft.setCursor(172, 142);
      tft.println(getDateFormattedStr());

      drawBentoBottomBar("[Click] Stations Matrix | [2x] Screensaver");
    }

    // ล้างเฉพาะพื้นที่ตัวเลขนิสิตก่อนพิมพ์ซ้ำ
    tft.fillRect(14, 58, 134, 26, getTftCardBg());
    tft.setTextColor(getTftAccentGreen(), getTftCardBg());
    tft.setTextSize(3);
    tft.setCursor(14, 58);
    tft.printf("%d", usedCount);
    tft.setTextSize(1);
    tft.setTextColor(getTftTextMain(), getTftCardBg());
    tft.printf(" / %u pax", (unsigned)db.size());

    int barW = 134;
    int progressW = (db.size() > 0) ? (int)((usedCount * barW) / db.size()) : 0;
    if (progressW > barW) progressW = barW;
    tft.drawRoundRect(14, 98, barW + 2, 8, 3, getTftCardBorder());
    tft.fillRoundRect(15, 99, barW, 6, 2, getTftCardBg());
    if (progressW > 0) tft.fillRoundRect(15, 99, progressW, 6, 2, getTftAccentGreen());

    // ล้างเฉพาะพื้นที่ Telemetry ก่อนพิมพ์ซ้ำ
    tft.fillRect(206, 56, 102, 64, getTftCardBg());
    float cTemp = getChipTemperature();
    float cCpu = calculateCpuLoad();
    tft.setTextSize(1);
    tft.setCursor(172, 58);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.print("TEMP: ");
    tft.setTextColor((cTemp < 65.0) ? getTftAccentGreen() : getTftAccentYellow(), getTftCardBg());
    tft.printf("%.1f C\n", cTemp);

    tft.setCursor(172, 74);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.print("CPU : ");
    tft.setTextColor(getTftAccentCyan(), getTftCardBg());
    tft.printf("%.0f %%\n", cCpu);

    tft.setCursor(172, 90);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.print("MEM : ");
    tft.setTextColor(getTftTextMain(), getTftCardBg());
    tft.printf("%uMB OPI\n", (unsigned)(ESP.getPsramSize() / 1024 / 1024));

    tft.setCursor(172, 106);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.printf("HEAP: %u KB\n", (unsigned)(ESP.getFreeHeap() / 1024));

    tft.fillRect(14, 160, 134, 24, getTftCardBg());
    tft.setTextColor(getTftAccentYellow(), getTftCardBg());
    tft.setTextSize(2);
    tft.setCursor(14, 160);
    tft.printf("%d B.", usedCount * 35);
    tft.setTextSize(1);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.setCursor(14, 186);
    tft.println("35 THB Allowance");

    // ล้างเฉพาะพื้นที่เวลาและสถานะเปิดบริการ
    tft.fillRect(172, 156, 138, 44, getTftCardBg());
    tft.setTextColor(getTftTextMain(), getTftCardBg());
    tft.setTextSize(2);
    tft.setCursor(172, 158);
    tft.print(getTimeOnlyStr());

    bool isOpen = isWithinServiceTime();
    tft.setCursor(172, 184);
    tft.setTextSize(1);
    if (isOpen) {
      tft.setTextColor(getTftAccentGreen(), getTftCardBg());
      tft.printf("OPEN (%02d:%02d-%02d:%02d)", serviceStartHour, serviceStartMin, serviceEndHour, serviceEndMin);
    } else {
      tft.setTextColor(getTftAccentRose(), getTftCardBg());
      tft.print("CLOSED (Out of Hours)");
    }
  }
  else if (currentHostPage == 1) {
    if (fullRedraw) {
      drawHostTopBar("[2/3] BENTO STATIONS 2x2 MATRIX");
      drawBentoBottomBar("[Click] Activity Log | [2x] Screensaver");
    }

    int coords[4][2] = { {6, 32}, {164, 32}, {6, 122}, {164, 122} };
    static int preloadStep = 0;
    if (!fullRedraw) preloadStep = (preloadStep + 1) % 4;

    for (int i = 0; i < 4; i++) {
      int x = coords[i][0];
      int y = coords[i][1];
      int w = 150;
      int h = 86;

      if (fullRedraw) {
        uint16_t cardBorderCol = stationNodes[i].isOnline ? getTftAccentGreen() : getTftAccentRose();
        drawBentoCard(x, y, w, h, cardBorderCol, getTftCardBg());

        tft.setTextColor(getTftAccentCyan(), getTftCardBg());
        tft.setTextSize(1);
        tft.setCursor(x + 8, y + 8);
        tft.printf("STATION 0%d", i + 1);

        if (stationNodes[i].isOnline) {
          drawBentoPillBadge(x + 90, y + 6, 52, 14, "ONLINE", 0x0000, getTftAccentGreen());
        } else {
          drawBentoPillBadge(x + 90, y + 6, 52, 14, "OFFLINE", 0xFFFF, getTftAccentRose());
        }

        tft.setTextColor(getTftTextMain(), getTftCardBg());
        tft.setTextSize(2);
        tft.setCursor(x + 8, y + 26);
        tft.printf("%d", shopCounts[i]);
        tft.setTextSize(1);
        tft.setTextColor(getTftTextMuted(), getTftCardBg());
        tft.print(" meals");

        tft.setTextColor(getTftAccentYellow(), getTftCardBg());
        tft.setTextSize(2);
        tft.setCursor(x + 8, y + 46);
        tft.printf("%d B.", shopCounts[i] * 35);

        tft.drawFastHLine(x + 8, y + 66, w - 16, getTftCardBorder());
      }

      tft.fillRect(x + 8, y + 70, w - 16, 14, getTftCardBg());
      tft.setTextSize(1);

      if (stationNodes[i].isOnline) {
        int sPct = getHostBatteryPercentage(stationNodes[i].systemVoltage);
        tft.setCursor(x + 8, y + 72);
        tft.setTextColor(getTftTextMuted(), getTftCardBg());
        tft.printf("%ddB", stationNodes[i].rssi);
        drawSignalBars(x + 46, y + 70, stationNodes[i].rssi, true, getTftCardBg());

        tft.setCursor(x + 76, y + 72);
        tft.setTextColor(getTftTextMuted(), getTftCardBg());
        tft.printf("%d%%", sPct);
        drawMiniBattery(x + 104, y + 71, sPct);
      } else {
        String waitStr = "WAITING";
        for (int d = 0; d <= preloadStep; d++) waitStr += ".";

        tft.setCursor(x + 8, y + 72);
        tft.setTextColor(getTftAccentRose(), getTftCardBg());
        tft.print(waitStr);
      }
    }
  }
  else if (currentHostPage == 2) {
    if (fullRedraw) {
      drawHostTopBar("[3/3] REAL-TIME ACTIVITY & NETWORK");

      drawBentoCard(6, 32, 308, 92, getTftCardBorder(), getTftCardBg());
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setTextSize(1);
      tft.setCursor(14, 40);
      tft.println("LAST SCANNED BENEFICIARY ACTIVITY");

      drawBentoCard(6, 130, 150, 78, getTftCardBorder(), getTftCardBg());
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setTextSize(1);
      tft.setCursor(14, 138);
      tft.println("NETWORK GATEWAY");

      tft.setTextColor(getTftTextMain(), getTftCardBg());
      tft.setCursor(14, 154); tft.println("SSID : MCU_CANTEEN");
      tft.setCursor(14, 168); tft.println("IP   : 192.168.4.1");
      tft.setTextColor(getTftAccentGreen(), getTftCardBg());
      tft.setCursor(14, 182); tft.println("ESP-NOW: LOCKED CH 1");

      drawBentoCard(164, 130, 150, 78, getTftCardBorder(), getTftCardBg());
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setTextSize(1);
      tft.setCursor(172, 138);
      tft.println("SYSTEM STATUS");

      tft.setTextColor(getTftTextMain(), getTftCardBg());
      tft.setCursor(172, 154); tft.printf("STUDENTS: %u pax\n", (unsigned)db.size());
      tft.setCursor(172, 168); tft.printf("OFFICERS: %u / 3\n", (unsigned)adminUsers.size());
      tft.setTextColor(getTftAccentCyan(), getTftCardBg());
      tft.setCursor(172, 182); tft.println("STORAGE : LITTLEFS OK");

      drawBentoBottomBar("[Click] Overview Bento | [2x] Screensaver");
    }

    tft.fillRect(14, 56, 290, 62, getTftCardBg());
    if (lastScannedUID != "-") {
      tft.setTextSize(2);
      tft.setCursor(14, 56);
      tft.setTextColor(getTftAccentCyan(), getTftCardBg());
      tft.printf("UID: %s (ST.0%d)", maskUID(lastScannedUID).c_str(), lastScannedStation);

      tft.setTextSize(1);
      tft.setCursor(14, 80);
      tft.setTextColor(getTftTextMain(), getTftCardBg());
      // เดิมเรียก generateRefNo() ตรงนี้ ทำให้สร้างเลขอ้างอิง "ใหม่" ทุกครั้งที่วาดจอ
      // เลขบนจอจึงไม่ตรงกับที่บันทึกไว้จริง และ transactionCounter ก็วิ่งขึ้นเรื่อย ๆ
      tft.printf("ID: %s | REF: %s\n", lastScannedStudentId.c_str(), lastScannedRefNo.c_str());

      if (lastScannedStatus == "APPROVED") {
        drawBentoPillBadge(14, 98, 120, 16, "[APPROVED 35B]", getTftAccentGreen(), 0x01E4);
      } else if (lastScannedStatus == "DUPLICATE") {
        drawBentoPillBadge(14, 98, 130, 16, "[ALREADY CLAIMED]", getTftAccentYellow(), 0x3A00);
      } else if (lastScannedStatus == "TIME_CLOSED") {
        drawBentoPillBadge(14, 98, 130, 16, "[OUT OF SERVICE]", getTftAccentRose(), 0x3000);
      } else {
        drawBentoPillBadge(14, 98, 120, 16, "[UNREGISTERED]", getTftAccentRose(), 0x3000);
      }
    } else {
      tft.setTextSize(2);
      tft.setCursor(14, 68);
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.println("WAITING FOR CARD TAP...");
    }
  }
}

// ============================================================================
// SCREENSAVER
// ============================================================================
void renderScreensaver(bool fullRedraw) {
  int usedCount = 0;
  for (const auto& s : db) if (s.claimed) usedCount++;

  static String lastHostClock = "";

  if (fullRedraw) {
    ledOff();
    tft.fillScreen(getTftBg());
    lastHostClock = "";
    drawHostTopBar("[SCREENSAVER] MCU PHRAE SMART CANTEEN");

    drawBentoCard(20, 40, 280, 156, getTftCardBorder(), getTftCardBg());

    String dateStr = getDateFormattedStr();
    int xDate = max(24, (320 - (int)dateStr.length() * 12) / 2);
    tft.setTextColor(getTftAccentCyan(), getTftCardBg());
    tft.setTextSize(2);
    tft.setCursor(xDate, 108);
    tft.print(dateStr);

    char statBuf[48];
    snprintf(statBuf, sizeof(statBuf), "CLAIMED: %3d / %3d STUDENTS (%5d B.)", usedCount, (int)db.size(), usedCount * 35);
    int statLen = strlen(statBuf) * 6;
    int posX = max(24, (320 - statLen) / 2);
    tft.setTextColor(getTftAccentGreen(), getTftCardBg());
    tft.setTextSize(1);
    tft.setCursor(posX, 140);
    tft.print(statBuf);

    float hVolt = readHostBatteryVoltage();
    char statBuf2[48];
    snprintf(statBuf2, sizeof(statBuf2), "BATT: %d%% | CORE: %.1fC | HEAP: %uKB",
             getHostBatteryPercentage(hVolt), getChipTemperature(), (unsigned)(ESP.getFreeHeap() / 1024));
    int statLen2 = strlen(statBuf2) * 6;
    int posX2 = max(24, (320 - statLen2) / 2);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.setCursor(posX2, 160);
    tft.print(statBuf2);

    drawBentoBottomBar("[ TAP CARD OR PRESS BUTTON TO WAKE UP ]");
  }

  String curTime = getTimeOnlyStr();
  if (fullRedraw || curTime != lastHostClock) {
    lastHostClock = curTime;
    tft.fillRect(60, 52, 200, 36, getTftCardBg());
    tft.setTextColor(getTftTextMain(), getTftCardBg());
    tft.setTextSize(4);
    tft.setCursor(64, 56);
    tft.print(curTime);
  }
}

// ============================================================================
// LIVE SCAN ALERT (FULL-SCREEN COLOR TAKEOVER & ADAPTIVE FEEDBACK)
// ============================================================================
void displayHostLiveScan(String uid, String studentId, String status, int stId) {
  isLiveScanDisplaying = true;
  liveScanHoldUntil = millis() + 4500;

  uint16_t screenBg;
  uint16_t cardBg;
  uint16_t bannerBg;
  uint16_t bannerFg;
  uint16_t accentColor;
  uint16_t textColor = BENTO_WHITE;
  uint16_t textMuted;
  const char* headerTitle;
  const char* footerDesc;

  if (status == "APPROVED") {
    ledApproved();
    screenBg    = 0x02E5;             // พื้นหลังจอเฉดเขียวเข้ม
    cardBg      = 0x01C3;             // พื้นหลังการ์ดโทนเขียวมรกตลึก
    bannerBg    = BENTO_NEON_GREEN;   // แถบแบนเนอร์สีเขียวนีออน
    bannerFg    = 0x0000;             // ตัวอักษรสีดำ
    accentColor = BENTO_NEON_GREEN;
    textMuted   = 0x87F0;             // ข้อความกำกับสีเขียวมิ้นต์
    headerTitle = ">>> APPROVED: 35B QUOTA <<<";
    footerDesc  = "TRANSACTION VERIFIED | DAILY QUOTA APPLIED";
  } 
  else if (status == "DUPLICATE") {
    ledDuplicate();
    screenBg    = 0x8200;             // พื้นหลังจอเฉดส้มอิฐเข้ม
    cardBg      = 0x4900;             // พื้นหลังการ์ดโทนส้มเข้ม
    bannerBg    = ST77XX_ORANGE;      // แถบแบนเนอร์สีส้มสด
    bannerFg    = 0x0000;             // ตัวอักษรสีดำ
    accentColor = ST77XX_ORANGE;
    textMuted   = 0xFDC0;             // ข้อความกำกับสีส้มอ่อน
    headerTitle = "! DUPLICATE: ALREADY CLAIMED !";
    footerDesc  = "QUOTA ALREADY CONSUMED FOR TODAY";
  } 
  else if (status == "TIME_CLOSED") {
    ledDuplicate();
    screenBg    = 0x6200;             // พื้นหลังโทนส้มอมน้ำตาล
    cardBg      = 0x3900;             // พื้นหลังการ์ดโทนน้ำตาลเข้ม
    bannerBg    = BENTO_NEON_YELLOW;  // แถบแบนเนอร์สีเหลืองเตือนภัย
    bannerFg    = 0x0000;
    accentColor = BENTO_NEON_YELLOW;
    textMuted   = 0xFEE0;
    headerTitle = "! SERVICE HOURS ARE CLOSED !";
    footerDesc  = "CARD SCANNED OUTSIDE SERVICE WINDOW";
  } 
  else {
    ledRejected();
    screenBg    = 0x8000;             // พื้นหลังจอเฉดแดงทึบเข้ม
    cardBg      = 0x4800;             // พื้นหลังการ์ดโทนแดงเข้ม
    bannerBg    = BENTO_NEON_ROSE;    // แถบแบนเนอร์สีแดงสด
    bannerFg    = BENTO_WHITE;        // ตัวอักษรสีขาว
    accentColor = BENTO_NEON_ROSE;
    textMuted   = 0xFCAE;             // ข้อความกำกับสีชมพูอ่อน
    headerTitle = "X REJECTED: UNREGISTERED X";
    footerDesc  = "CARD NOT FOUND IN STUDENT DIRECTORY";
  }

  tft.fillScreen(screenBg);

  tft.fillRect(0, 0, 320, 36, bannerBg);
  // ย่อฟอนต์อัตโนมัติ: หัวข้อยาว 30 ตัวอักษรที่ size 2 กว้าง 360px ล้นจอ 320px
  drawFitCenteredText(0, 0, 320, 36, headerTitle, 2, bannerFg, bannerBg);

  tft.fillRoundRect(8, 44, 304, 188, 8, cardBg);
  tft.drawRoundRect(8, 44, 304, 188, 8, accentColor);
  tft.drawRoundRect(9, 45, 302, 186, 7, accentColor);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 54);
  tft.print("SERVICE STATION:");
  
  tft.setTextSize(2);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 66);
  tft.printf("STATION 0%d", stId);

  tft.drawFastHLine(20, 88, 280, accentColor);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 96);
  tft.print("BENEFICIARY STUDENT ID:");

  String cleanId = (studentId != "-" && studentId.length() > 0) ? studentId : "UNKNOWN";
  tft.setTextSize(3);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 110);
  tft.print(cleanId);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 142);
  tft.print("CARD UID (PROTECTED):");

  tft.setTextSize(2);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 154);
  tft.print(maskUID(uid));

  tft.fillRoundRect(16, 184, 288, 36, 6, bannerBg);
  drawFitCenteredText(16, 184, 288, 36, footerDesc, 1, bannerFg, bannerBg);
}

void renderDeveloperCredit() {
  tft.fillScreen(getTftBg());
  drawHostTopBar("SYSTEM ARCHITECTURE & CREDITS");

  drawBentoCard(10, 36, 300, 166, getTftAccentCyan(), getTftCardBg());

  tft.setTextColor(getTftAccentCyan(), getTftCardBg());
  tft.setTextSize(1);
  tft.setCursor(22, 48);
  tft.println("SYSTEM DEVELOPER:");

  tft.setTextColor(getTftTextMain(), getTftCardBg());
  tft.setTextSize(2);
  tft.setCursor(22, 64);
  tft.println(DEV_NAME);

  tft.setTextColor(getTftAccentYellow(), getTftCardBg());
  tft.setTextSize(1);
  tft.setCursor(22, 92);
  tft.println(DEV_ROLE);

  tft.setTextColor(getTftAccentGreen(), getTftCardBg());
  tft.setCursor(22, 110);
  tft.println("Mahachulalongkornrajavidyalaya Phrae");

  tft.setTextColor(getTftTextMuted(), getTftCardBg());
  tft.setCursor(22, 134); tft.printf("Firmware: v%s (Bento Edition)\n", APP_VERSION);
  tft.setCursor(22, 150); tft.printf("Hardware: ESP32-S3 DevKitC (N16R8, PSRAM %uMB)\n", (unsigned)(ESP.getPsramSize()/1024/1024));
  tft.setCursor(22, 166); tft.println("Display : 2.8\" ST7789V 320x240 Modular Bento");

  drawBentoBottomBar("[ PRESS BUTTON TO RETURN TO DASHBOARD ]");
}

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
      isScreensaverActive = false; 
      isCreditActive = false;
      currentHostPage = 0; 
      soundHomeBeep(); 
      renderHostPage(true); 
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
    }
    else if (clickCount >= 3) {
      isTftDarkMode = !isTftDarkMode;
      preferences.begin("sys_cfg", false);
      preferences.putBool("tft_dark", isTftDarkMode);
      preferences.end();
      soundThemeSwitch();
      for (int i = 0; i < 4; i++) stationThemeSent[i] = true;
      broadcastStationTheme();
      renderHostPage(true);
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

    if (!stationNodes[stId - 1].isOnline) stationThemeSent[stId - 1] = false;
    stationNodes[stId - 1].isOnline = true;
    stationNodes[stId - 1].rssi = currentRssi;
    stationNodes[stId - 1].systemVoltage = pkt.systemVoltage;
    stationNodes[stId - 1].lastSeen = millis();
    bool macChanged = (memcmp(stationNodes[stId - 1].mac, mac, 6) != 0);
    if (macChanged) { stationPeerReady[stId - 1] = false; stationThemeSent[stId - 1] = false; }
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
  isCreditActive = false;

  String uid = String(pkt.uid);
  uid.trim();

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
        String currentTimestamp = getRealTimeStr();
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

// ============================================================================
// BENTO WEB PORTAL HTML
// ============================================================================
String getHTML() {
  int usedCount = 0;
  int shopCounts[4] = {0, 0, 0, 0};
  (void)shopCounts;   // ตัวเลขรายร้านถูกดึงสดผ่าน /api/dashboard แทนการฝังลง HTML
  int activeTempWaitingCount = 0;
  for (const auto& s : db) {
    if (s.claimed) {
      usedCount++;
      if (s.station >= 1 && s.station <= 4) shopCounts[s.station - 1]++;
    }
    if (s.isTempCard && !s.claimed) activeTempWaitingCount++;
  }

  float hostBattVolt = readHostBatteryVoltage();
  int hostBattPct = getHostBatteryPercentage(hostBattVolt);
  float initialTemp = getChipTemperature();
  float initialCpu = calculateCpuLoad();

  String archivesHtml = "";
  File root = LittleFS.open("/");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      String fn = String(file.name());
      if (fn.startsWith("arc_") || fn.startsWith("/arc_")) {
        String cleanName = fn.startsWith("/") ? fn.substring(1) : fn;
        String safeName = htmlEscape(cleanName);
        archivesHtml += "<tr><td><b>" + safeName + "</b></td><td>" + String(file.size() / 1024.0, 1) + " KB</td>";
        archivesHtml += "<td style='text-align:right;'>";
        archivesHtml += "<a href='/api/archive/download?file=" + safeName + "' class='btn btn-emerald' style='padding:0.35rem 0.75rem; font-size:0.8rem;'>📥 Download</a> ";
        // ใช้ data-attribute แทน onclick ที่ฝังชื่อไฟล์ลงในสตริง JavaScript
        archivesHtml += "<button type='button' class='btn btn-rose js-archive-delete' data-file='" + safeName + "' style='padding:0.35rem 0.75rem; font-size:0.8rem;'>🗑️ Delete</button>";
        archivesHtml += "</td></tr>";
      }
      file = root.openNextFile();
    }
    root.close();
  }
  if (archivesHtml.length() == 0) {
    archivesHtml = "<tr><td colspan='3' style='text-align:center; color:var(--text-muted); padding:1.5rem;' data-th='ไม่มีไฟล์ประวัติย้อนหลังในระบบ' data-en='No archives found in storage'>ไม่มีไฟล์ประวัติย้อนหลังในระบบ</td></tr>";
  }

  DateTime nowRTC = rtc.now();
  char dateInputBuf[12], timeInputBuf[10];
  snprintf(dateInputBuf, sizeof(dateInputBuf), "%04d-%02d-%02d", nowRTC.year(), nowRTC.month(), nowRTC.day());
  snprintf(timeInputBuf, sizeof(timeInputBuf), "%02d:%02d:%02d", nowRTC.hour(), nowRTC.minute(), nowRTC.second());

  int quotaPercent = (db.size() > 0) ? (usedCount * 100) / db.size() : 0;
  if (quotaPercent > 100) quotaPercent = 100;

  String html;
  // จองหน่วยความจำล่วงหน้าครั้งเดียว แทนการปล่อยให้ += ขยายบัฟเฟอร์นับสิบครั้ง
  // ซึ่งทำให้ heap แตกเป็นเสี่ยงบนอุปกรณ์ที่รันยาว ๆ (บอร์ดที่เปิด OPI PSRAM
  // จะได้บล็อกนี้จาก PSRAM ถ้าจองไม่สำเร็จก็ยังทำงานได้ตามปกติ)
  html.reserve(100 * 1024);
  html += R"rawliteral(<!DOCTYPE html>
<html lang="th" data-theme="dark">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>MCU Phrae - Smart Canteen Bento Portal</title>
  <link href="https://fonts.googleapis.com/css2?family=Plus+Jakarta+Sans:wght@400;500;600;700;800&family=Sarabun:wght@300;400;500;600;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg-base: #090d16;
      --bg-surface: rgba(26, 35, 50, 0.75);
      --bg-surface-elevated: #1e293b;
      --border-card: rgba(255, 255, 255, 0.08);
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --nav-bg: rgba(15, 23, 42, 0.85);
      --shadow-card: 0 16px 32px rgba(0, 0, 0, 0.35);
      
      --accent-green: #10b981;
      --accent-green-glow: rgba(16, 185, 129, 0.25);
      --accent-cyan: #06b6d4;
      --accent-yellow: #f59e0b;
      --accent-rose: #f43f5e;
      --accent-indigo: #6366f1;
    }

    [data-theme="light"] {
      --bg-base: #f1f5f9;
      --bg-surface: #ffffff;
      --bg-surface-elevated: #f8fafc;
      --border-card: rgba(0, 0, 0, 0.06);
      --text-main: #0f172a;
      --text-muted: #64748b;
      --nav-bg: rgba(255, 255, 255, 0.88);
      --shadow-card: 0 10px 25px rgba(0, 0, 0, 0.04);
      
      --accent-green: #059669;
      --accent-green-glow: rgba(5, 150, 105, 0.15);
      --accent-cyan: #0284c7;
      --accent-yellow: #d97706;
      --accent-rose: #e11d48;
      --accent-indigo: #4f46e5;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Plus Jakarta Sans', 'Sarabun', sans-serif; transition: background-color 0.25s, border-color 0.25s, color 0.25s; }
    body { background-color: var(--bg-base); color: var(--text-main); padding-top: 4.75rem; min-height: 100vh; display: flex; flex-direction: column; justify-content: space-between; }
    
    .navbar { position: fixed; top: 0; left: 0; right: 0; height: 4.25rem; background: var(--nav-bg); border-bottom: 1px solid var(--border-card); backdrop-filter: blur(16px); display: flex; align-items: center; justify-content: space-between; padding: 0 1.5rem; z-index: 100; }
    .nav-brand { display: flex; align-items: center; gap: 0.75rem; color: var(--text-main); text-decoration: none; }
    .brand-badge { background: var(--accent-green); color: #064e3b; font-size: 0.72rem; font-weight: 800; padding: 0.25rem 0.65rem; border-radius: 9999px; letter-spacing: 0.04em; }
    .brand-title { font-size: 1.15rem; font-weight: 800; white-space: nowrap; letter-spacing: -0.02em; }
    .nav-menu { display: flex; align-items: center; gap: 0.4rem; }
    .nav-item { padding: 0.55rem 0.95rem; border-radius: 0.85rem; font-weight: 600; font-size: 0.875rem; cursor: pointer; border: 1px solid transparent; background: transparent; color: var(--text-muted); text-decoration: none; display: inline-flex; align-items: center; gap: 0.4rem; }
    .nav-item:hover { background: var(--bg-surface-elevated); color: var(--text-main); }
    .nav-item.active { background: var(--accent-indigo); color: white; }

    .dropdown { position: relative; }
    .dropdown-btn { display: inline-flex; align-items: center; gap: 0.35rem; cursor: pointer; }
    .dropdown-arrow { font-size: 0.65rem; transition: transform 0.2s; }
    .dropdown:hover .dropdown-arrow { transform: rotate(180deg); }
    .dropdown-menu {
      display: none; position: absolute; top: calc(100% + 0.35rem); left: 0;
      background: var(--bg-surface); border: 1px solid var(--border-card); backdrop-filter: blur(16px);
      border-radius: 1.25rem; min-width: 14.5rem; box-shadow: var(--shadow-card);
      padding: 0.5rem; z-index: 200;
    }
    .dropdown:hover .dropdown-menu { display: block; }
    .dropdown-item {
      display: flex; align-items: center; gap: 0.5rem; width: 100%;
      padding: 0.65rem 0.95rem; color: var(--text-muted); font-size: 0.875rem;
      font-weight: 500; text-align: left; background: transparent;
      border: none; border-radius: 0.75rem; cursor: pointer;
    }
    .dropdown-item:hover { background: var(--bg-surface-elevated); color: var(--text-main); }
    .dropdown-item.active { background: var(--accent-indigo); color: #fff; font-weight: 700; }
    .badge-count { background: rgba(245, 158, 11, 0.2); color: var(--accent-yellow); font-size: 0.68rem; font-weight: 800; padding: 0.15rem 0.45rem; border-radius: 9999px; margin-left: auto; }

    .theme-toggle-btn {
      background: var(--bg-surface-elevated); border: 1px solid var(--border-card);
      color: var(--text-main); padding: 0.45rem 0.85rem; border-radius: 0.75rem;
      font-size: 0.95rem; cursor: pointer; display: inline-flex; align-items: center; gap: 0.35rem;
    }
    .lang-btn { background: var(--bg-surface-elevated); border: 1px solid var(--border-card); color: var(--text-main); padding: 0.45rem 0.85rem; border-radius: 0.75rem; font-weight: 700; font-size: 0.8rem; cursor: pointer; }
    .nav-toggle { display: none; background: transparent; border: 1px solid var(--border-card); color: var(--text-main); font-size: 1.4rem; padding: 0.35rem 0.75rem; border-radius: 0.5rem; cursor: pointer; }

    @media (max-width: 1024px) {
      .nav-toggle { display: block; }
      .nav-menu { display: none; position: absolute; top: 4.25rem; left: 0; right: 0; background: var(--nav-bg); border-bottom: 1px solid var(--border-card); flex-direction: column; align-items: stretch; padding: 1rem; gap: 0.5rem; }
      .nav-menu.open { display: flex; }
      .dropdown:hover .dropdown-menu { position: static; display: block; box-shadow: none; border: none; background: var(--bg-surface-elevated); padding-left: 0.5rem; margin-top: 0.2rem; }
      .dropdown-arrow { display: none; }
    }

    .content-area { max-width: 82rem; margin: 1.5rem auto; padding: 0 1.25rem; width: 100%; }
    
    .bento-grid {
      display: grid;
      grid-template-columns: repeat(12, 1fr);
      gap: 1.25rem;
      margin-bottom: 1.5rem;
    }
    .bento-card {
      background: var(--bg-surface);
      border: 1px solid var(--border-card);
      border-radius: 1.75rem;
      padding: 1.75rem;
      box-shadow: var(--shadow-card);
      backdrop-filter: blur(16px);
      position: relative;
      overflow: hidden;
    }
    .col-span-4 { grid-column: span 12; }
    .col-span-6 { grid-column: span 12; }
    .col-span-8 { grid-column: span 12; }
    .col-span-12 { grid-column: span 12; }
    
    @media (min-width: 768px) {
      .col-span-4 { grid-column: span 4; }
      .col-span-6 { grid-column: span 6; }
      .col-span-8 { grid-column: span 8; }
    }

    .gauge-wrapper { display: flex; flex-direction: column; align-items: center; justify-content: center; position: relative; }
    .gauge-svg { width: 220px; height: 130px; }
    .gauge-bg { fill: none; stroke: var(--border-card); stroke-width: 16; stroke-linecap: round; }
    .gauge-val { fill: none; stroke: var(--accent-green); stroke-width: 16; stroke-linecap: round; stroke-dasharray: 252; transition: stroke-dashoffset 1s ease-out; }
    .gauge-content { text-align: center; margin-top: -3.5rem; }
    .gauge-number { font-size: 2.2rem; font-weight: 800; color: var(--accent-green); letter-spacing: -0.03em; }
    .gauge-label { font-size: 0.8rem; color: var(--text-muted); font-weight: 600; text-transform: uppercase; }

    .pill-metric {
      background: var(--bg-surface-elevated);
      border: 1px solid var(--border-card);
      border-radius: 1.25rem;
      padding: 1rem 1.25rem;
      display: flex;
      align-items: center;
      justify-content: space-between;
    }

    .table-container { overflow-x: auto; border-radius: 1.25rem; border: 1px solid var(--border-card); margin-top: 1rem; }
    table { width: 100%; border-collapse: collapse; text-align: left; font-size: 0.92rem; }
    th { background: var(--bg-surface-elevated); padding: 1rem 1.25rem; font-weight: 600; color: var(--text-muted); border-bottom: 1px solid var(--border-card); }
    td { padding: 1rem 1.25rem; border-bottom: 1px solid var(--border-card); color: var(--text-main); }
    .btn { padding: 0.65rem 1.2rem; border-radius: 1rem; font-weight: 600; font-size: 0.88rem; cursor: pointer; border: none; text-decoration: none; display: inline-flex; align-items: center; justify-content: center; gap: 0.4rem; }
    .btn-emerald { background-color: var(--accent-green); color: #064e3b; font-weight: 700; }
    .btn-indigo { background-color: var(--accent-indigo); color: white; }
    .btn-rose { background-color: var(--accent-rose); color: white; }
    .btn-slate { background-color: var(--bg-surface-elevated); color: var(--text-main); border: 1px solid var(--border-card); }
    .form-input { width: 100%; padding: 0.8rem 1.1rem; border-radius: 1rem; border: 1px solid var(--border-card); background: var(--bg-surface-elevated); color: var(--text-main); font-size: 0.95rem; margin-bottom: 1rem; outline: none; }
    .modal { display: none; position: fixed; inset: 0; background: rgba(0, 0, 0, 0.7); backdrop-filter: blur(12px); z-index: 250; align-items: center; justify-content: center; padding: 1.5rem; }
    .modal.active { display: flex; }
    .dashboard-footer { margin-top: 3.5rem; padding: 2.5rem 1.25rem; border-top: 1px solid var(--border-card); text-align: center; font-size: 0.85rem; color: var(--text-muted); }

    /* --- Toast notifications (แทน alert() ที่ทำให้ต้องรีโหลดทั้งหน้า) --- */
    .toast-stack { position: fixed; right: 1.25rem; bottom: 1.25rem; z-index: 400; display: flex; flex-direction: column; gap: 0.6rem; max-width: min(26rem, calc(100vw - 2.5rem)); }
    .toast { display: flex; align-items: flex-start; gap: 0.65rem; background: var(--bg-surface-elevated); border: 1px solid var(--border-card); border-left: 4px solid var(--accent-indigo); border-radius: 1rem; padding: 0.9rem 1.1rem; box-shadow: var(--shadow-card); font-size: 0.9rem; font-weight: 600; color: var(--text-main); animation: toast-in 0.22s ease-out; }
    .toast.ok { border-left-color: var(--accent-green); }
    .toast.err { border-left-color: var(--accent-rose); }
    .toast.leaving { opacity: 0; transform: translateY(0.5rem); transition: opacity 0.25s, transform 0.25s; }
    @keyframes toast-in { from { opacity: 0; transform: translateY(0.75rem); } to { opacity: 1; transform: none; } }

    /* --- Skeleton loader ของตาราง --- */
    .skeleton { display: block; height: 0.85rem; border-radius: 0.5rem; background: linear-gradient(90deg, var(--bg-surface-elevated) 25%, var(--border-card) 50%, var(--bg-surface-elevated) 75%); background-size: 200% 100%; animation: skeleton-shift 1.2s infinite; }
    @keyframes skeleton-shift { from { background-position: 200% 0; } to { background-position: -200% 0; } }

    thead th { position: sticky; top: 0; z-index: 2; }
    tbody tr:hover td { background: var(--bg-surface-elevated); }
    .table-container { max-height: 68vh; overflow-y: auto; }
    .live-dot { display: inline-block; width: 0.5rem; height: 0.5rem; border-radius: 50%; background: var(--accent-green); margin-right: 0.35rem; animation: pulse-dot 1.6s infinite; }
    @keyframes pulse-dot { 0%,100% { opacity: 1; } 50% { opacity: 0.25; } }
    .station-card { background: var(--bg-surface-elevated); border: 1px solid var(--border-card); border-radius: 1.5rem; padding: 1.25rem; }
    .station-card.offline { border-color: rgba(244,63,94,0.35); }
    .sig-bars { display: inline-flex; align-items: flex-end; gap: 2px; height: 0.85rem; }
    .sig-bars i { width: 3px; background: var(--border-card); border-radius: 1px; display: block; }
    .sig-bars i.on { background: currentColor; }
    [title] { cursor: help; }
  </style>
</head>
<body>
  <header class="navbar">
    <div class="nav-brand">
      <span class="brand-badge">BENTO V4</span>
      <span class="brand-title" data-th="ระบบบริหารจัดการอาหารกลางวันนิสิต" data-en="Student Meal Subsidy Portal">ระบบบริหารจัดการอาหารกลางวันนิสิต</span>
    </div>
    <button class="nav-toggle" onclick="toggleNav()">☰</button>
    
    <nav class="nav-menu" id="navMenu">
      <button onclick="switchTab('dashboard')" id="btn-dashboard" class="nav-item active" data-th="📊 แดชบอร์ด" data-en="📊 Dashboard">📊 แดชบอร์ด</button>

      <div class="dropdown">
        <button class="nav-item dropdown-btn" id="group-beneficiaries">
          <span data-th="👥 ทะเบียน & สิทธิ์" data-en="👥 Beneficiaries">👥 ทะเบียน & สิทธิ์</span>
          <span class="dropdown-arrow">▼</span>
        </button>
        <div class="dropdown-menu">
          <button onclick="switchTab('students')" id="btn-students" class="dropdown-item" data-th="👥 รายชื่อผู้มีสิทธิ์" data-en="👥 Directory">👥 รายชื่อผู้มีสิทธิ์</button>
          <button onclick="switchTab('tempcard')" id="btn-tempcard" class="dropdown-item">
            <span data-th="💳 บัตรสำรอง" data-en="💳 Temp Cards">💳 บัตรสำรอง</span>
            <span class="badge-count" id="tempCount">)rawliteral" + String(activeTempWaitingCount) + R"rawliteral(</span>
          </button>
          <button onclick="switchTab('import')" id="btn-import" class="dropdown-item" data-th="📥 นำเข้ารายชื่อ CSV" data-en="📥 Import CSV">📥 นำเข้ารายชื่อ CSV</button>
        </div>
      </div>

      <div class="dropdown">
        <button class="nav-item dropdown-btn" id="group-admin">
          <span data-th="⚙️ จัดการระบบ" data-en="⚙️ Administration">⚙️ จัดการระบบ</span>
          <span class="dropdown-arrow">▼</span>
        </button>
        <div class="dropdown-menu">
          <button onclick="switchTab('shops')" id="btn-shops" class="dropdown-item" data-th="🏪 ร้านค้าพันธมิตร" data-en="🏪 Partner Vendors">🏪 ร้านค้าพันธมิตร</button>
          <button onclick="switchTab('archives')" id="btn-archives" class="dropdown-item" data-th="🗄️ คลังรายงานย้อนหลัง" data-en="🗄️ Audit Archives">🗄️ คลังรายงานย้อนหลัง</button>
          <button onclick="switchTab('admins')" id="btn-admins" class="dropdown-item" data-th="👤 เจ้าหน้าที่ดูแลระบบ" data-en="👤 System Officers">👤 เจ้าหน้าที่ดูแลระบบ ()rawliteral" + String(adminUsers.size()) + R"rawliteral(/3)</button>
          <button onclick="switchTab('settings')" id="btn-settings" class="dropdown-item" data-th="⚙️ กำหนดเวลา & ระบบ" data-en="⚙️ System Settings">⚙️ กำหนดเวลา & ระบบ</button>
        </div>
      </div>

      <div style="display: flex; align-items: center; gap: 0.5rem; margin-left: 0.5rem;">
        <button onclick="toggleTheme()" class="theme-toggle-btn" id="themeBtn" title="Toggle Light/Dark Mode">🌙</button>
        <button onclick="toggleLanguage()" class="lang-btn" id="langSwitch">EN</button>
        <a href="/logout" class="btn btn-rose" style="padding: 0.45rem 0.85rem; font-size: 0.8rem;" data-th="🚪 ออกจากระบบ" data-en="🚪 Sign Out">🚪 ออกจากระบบ</a>
      </div>
    </nav>
  </header>

  <main class="content-area">
    
    <!-- TAB 1: BENTO DASHBOARD -->
    <div id="tab-dashboard" class="tab-content" style="display: block;">
      <div class="bento-grid">
        
        <!-- Bento 1: Radial Quota Arc Gauge -->
        <div class="bento-card col-span-4" style="display: flex; flex-direction: column; justify-content: space-between;">
          <div style="display: flex; justify-content: space-between; align-items: center;">
            <span style="font-size: 0.85rem; font-weight: 700; color: var(--text-muted); text-transform: uppercase;" data-th="โควตาสวัสดิการวันนี้" data-en="Today's Meal Quota">โควตาสวัสดิการวันนี้</span>
            <span style="background: var(--accent-green-glow); color: var(--accent-green); font-size: 0.75rem; font-weight: 800; padding: 0.25rem 0.6rem; border-radius: 9999px;">35 THB / Meal</span>
          </div>

          <div class="gauge-wrapper" style="margin: 1.25rem 0;">
            <svg class="gauge-svg" viewBox="0 0 200 110">
              <path class="gauge-bg" d="M 20 100 A 80 80 0 0 1 180 100"></path>
              <path class="gauge-val" id="gaugeArc" d="M 20 100 A 80 80 0 0 1 180 100" style="stroke-dashoffset: )rawliteral" + String(252 - (quotaPercent * 252) / 100) + R"rawliteral(;"></path>
            </svg>
            <div class="gauge-content">
              <div class="gauge-number" id="statUsed">)rawliteral" + String(usedCount) + R"rawliteral(</div>
              <div class="gauge-label" data-th="ใช้สิทธิ์แล้ว (คน)" data-en="CLAIMED STUDENTS">ใช้สิทธิ์แล้ว (คน)</div>
            </div>
          </div>

          <div style="display: flex; justify-content: space-between; border-top: 1px solid var(--border-card); padding-top: 1rem; font-size: 0.85rem;">
            <div>
              <span style="color: var(--text-muted);" data-th="คงเหลือ:" data-en="Remaining:">คงเหลือ:</span>
              <b style="color: var(--text-main);" id="statRemaining">)rawliteral" + String((int)db.size() - usedCount) + R"rawliteral(</b>
            </div>
            <div>
              <span style="color: var(--text-muted);" data-th="ยอดจัดสรร:" data-en="Disbursed:">ยอดจัดสรร:</span>
              <b style="color: var(--accent-green);"><span id="statDisbursed">)rawliteral" + String(usedCount * 35) + R"rawliteral(</span> THB</b>
            </div>
          </div>
        </div>

        <!-- Bento 2: Host Hardware Telemetry -->
        <div class="bento-card col-span-4" style="display: flex; flex-direction: column; justify-content: space-between;">
          <div style="display: flex; justify-content: space-between; align-items: center;">
            <span style="font-size: 0.85rem; font-weight: 700; color: var(--text-muted); text-transform: uppercase;" data-th="สถานะฮาร์ดแวร์แม่ข่าย" data-en="Host Telemetry">สถานะฮาร์ดแวร์แม่ข่าย</span>
            <span style="color: var(--accent-cyan); font-size: 0.75rem; font-weight: 700;">ESP32-S3 N16R8</span>
          </div>

          <div style="display: flex; flex-direction: column; gap: 0.75rem; margin: 1rem 0;">
            <div class="pill-metric">
              <div>
                <div style="font-size: 0.75rem; color: var(--text-muted);">CORE TEMPERATURE</div>
                <div id="telemetryTemp" style="font-size: 1.4rem; font-weight: 800; color: var(--accent-green); margin-top: 0.2rem;">)rawliteral" + String(initialTemp, 1) + R"rawliteral( °C</div>
              </div>
              <div style="font-size: 1.6rem;">🌡️</div>
            </div>

            <div class="pill-metric">
              <div>
                <div style="font-size: 0.75rem; color: var(--text-muted);">PROCESSOR LOAD</div>
                <div id="telemetryCpu" style="font-size: 1.4rem; font-weight: 800; color: var(--accent-indigo); margin-top: 0.2rem;">)rawliteral" + String(initialCpu, 1) + R"rawliteral( %</div>
              </div>
              <div style="font-size: 1.6rem;">⚡</div>
            </div>
          </div>

          <div style="display: flex; justify-content: space-between; font-size: 0.8rem; color: var(--text-muted); border-top: 1px solid var(--border-card); padding-top: 0.85rem;">
            <span>🔋 Batt: <b style="color:var(--text-main);" id="statBatt">)rawliteral" + String(hostBattPct) + R"rawliteral(%</b></span>
            <span>💾 Heap: <b style="color:var(--text-main);" id="statHeap">)rawliteral" + String((unsigned)(ESP.getFreeHeap() / 1024)) + R"rawliteral( KB</b></span>
          </div>
        </div>

        <!-- Bento 3: Standard Clock & Quick Actions -->
        <div class="bento-card col-span-4" style="display: flex; flex-direction: column; justify-content: space-between; background: linear-gradient(135deg, var(--bg-surface), var(--bg-surface-elevated));">
          <div>
            <div style="font-size: 0.85rem; font-weight: 700; color: var(--accent-yellow); text-transform: uppercase;" data-th="เวลามาตรฐานระบบ" data-en="Standard RTC Time">เวลามาตรฐานระบบ</div>
            <div id="clockText" style="font-size: 1.8rem; font-weight: 800; margin: 0.5rem 0; letter-spacing: -0.02em;">)rawliteral" + getRealTimeStr() + R"rawliteral(</div>
            <div style="font-size: 0.82rem; color: var(--text-muted);" data-th="ปรับเทียบผ่านชิป DS3231 Precision I2C" data-en="Synced with DS3231 Precision I2C">ปรับเทียบผ่านชิป DS3231 Precision I2C</div>
            <div id="serviceBadge" style="margin-top: 0.75rem; display: inline-block; font-size: 0.75rem; font-weight: 800; padding: 0.25rem 0.7rem; border-radius: 9999px; background: var(--accent-green-glow); color: var(--accent-green);">—</div>
          </div>

          <div style="display: flex; flex-direction: column; gap: 0.5rem; margin-top: 1rem;">
            <a href="/export.csv" class="btn btn-emerald" style="width: 100%;" data-th="📥 ดาวน์โหลดรายงานสรุป (CSV)" data-en="📥 Export Summary (CSV)">📥 ดาวน์โหลดรายงานสรุป (CSV)</a>
            <button onclick="syncDeviceTime()" class="btn btn-slate" style="width: 100%;" data-th="⚡ ซิงค์เวลากับเครื่องนี้" data-en="⚡ Sync Device Time">⚡ ซิงค์เวลากับเครื่องนี้</button>
          </div>
        </div>

        <!-- Bento 4: 4 Stations Live Status -->
        <div class="bento-card col-span-12">
          <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 1.25rem;">
            <h3 style="font-size: 1.1rem; font-weight: 800;" data-th="📡 จุดให้บริการร้านค้าประจำโรงอาหาร (4 Stations Live Status)" data-en="📡 Canteen Vendor Stations (4 Stations Live Status)">📡 จุดให้บริการร้านค้าประจำโรงอาหาร (4 Stations Live Status)</h3>
            <span style="font-size: 0.8rem; color: var(--text-muted);">ESP-NOW Channel 1</span>
          </div>

          <div class="bento-grid" id="stationGrid" style="margin-bottom: 0;">
            <div class="col-span-6 station-card"><span class="skeleton" style="width: 60%;"></span></div>
            <div class="col-span-6 station-card"><span class="skeleton" style="width: 60%;"></span></div>
            <div class="col-span-6 station-card"><span class="skeleton" style="width: 60%;"></span></div>
            <div class="col-span-6 station-card"><span class="skeleton" style="width: 60%;"></span></div>
          </div>
        </div>

        <!-- Bento 5: Chart & Breakdown -->
        <div class="bento-card col-span-12">
          <h3 style="font-size: 1.1rem; font-weight: 800; margin-bottom: 1rem;" data-th="📈 สัดส่วนการใช้บริการจำแนกตามร้านค้า" data-en="📈 Service Distribution by Vendor">📈 สัดส่วนการใช้บริการจำแนกตามร้านค้า</h3>
          <div style="text-align: center; padding: 1rem 0;">
            <canvas id="analyticsChart" width="700" height="230" style="max-width: 100%; height: auto;"></canvas>
          </div>
        </div>

      </div>
    </div>

    <!-- TAB 2: STUDENTS -->
    <div id="tab-students" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12">
        <div style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 0.75rem; margin-bottom: 1.25rem;">
          <h2 style="font-size: 1.2rem; font-weight: 800;" data-th="👥 บัญชีรายชื่อผู้มีสิทธิ์รับสวัสดิการ" data-en="👥 Eligible Beneficiary Directory">👥 บัญชีรายชื่อผู้มีสิทธิ์รับสวัสดิการ</h2>
          <div style="display: flex; gap: 0.75rem;">
            <button onclick="openAddModal()" class="btn btn-indigo" data-th="➕ เพิ่มผู้มีสิทธิ์" data-en="➕ Add Student">➕ เพิ่มผู้มีสิทธิ์</button>
              <button type="button" onclick="dailyReset()" class="btn btn-rose" title="จัดเก็บบันทึกของวันนี้เข้าคลังประวัติ แล้วคืนสิทธิ์ให้นิสิตทุกคน" data-th="🔄 ปิดยอดประจำวัน & จัดเก็บประวัติ" data-en="🔄 Daily Reset &amp; Archive">🔄 ปิดยอดประจำวัน & จัดเก็บประวัติ</button>
          </div>
        </div>
        <div style="display: flex; gap: 0.75rem;">
          <input type="text" id="search" onkeyup="handleSearch(this.value)" placeholder="ค้นหารหัสนิสิต, ชื่อ-สกุล หรือเลขประจำตัว..." class="form-input" style="flex: 1; margin-bottom: 0;">
          <select id="pageSizeSelect" onchange="changePageSize(this.value)" class="form-input" style="width: 7.5rem; margin-bottom: 0;">
            <option value="20" selected>20 แถว</option>
            <option value="50">50 แถว</option>
            <option value="100">100 แถว</option>
          </select>
        </div>
        <div class="table-container">
          <table id="studentTable">
            <thead><tr><th data-th="รหัสนิสิต" data-en="Student ID">รหัสนิสิต</th><th data-th="ชื่อ-สกุล" data-en="Full Name">ชื่อ-สกุล</th><th data-th="เลขบัตรสมาร์ตการ์ด" data-en="Card UID">เลขบัตรสมาร์ตการ์ด</th><th data-th="เลขที่อ้างอิง" data-en="Ref No.">เลขที่อ้างอิง</th><th data-th="เวลาที่ใช้สิทธิ์" data-en="Timestamp">เวลาที่ใช้สิทธิ์</th><th data-th="จุดบริการ" data-en="Point">จุดบริการ</th><th style="text-align: right;" data-th="จัดการ" data-en="Action">จัดการ</th></tr></thead>
            <tbody id="studentTableBody">
              <tr><td colspan="7" style="padding: 2.5rem; text-align: center; color: var(--text-muted);">Loading data...</td></tr>
            </tbody>
          </table>
        </div>
        <div class="pagination-bar" style="display: flex; flex-direction: column; gap: 1rem; justify-content: space-between; align-items: center; padding-top: 1.25rem;">
          <div id="paginationInfo" style="color: var(--text-muted); font-size: 0.88rem;">Loading...</div>
          <div style="display: flex; gap: 0.4rem; align-items: center;">
            <button onclick="goToPage(1)" class="btn btn-slate" style="padding: 0.4rem 0.8rem;" data-th="⏮️ แรกสุด" data-en="⏮️ First">⏮️ แรกสุด</button>
            <button onclick="prevPage()" class="btn btn-slate" style="padding: 0.4rem 0.8rem;" data-th="◀ ก่อนหน้า" data-en="◀ Prev">◀ ก่อนหน้า</button>
            <span id="pageIndicator" style="padding: 0.4rem 0.85rem; font-weight: 800; color: var(--accent-indigo); background: rgba(99, 102, 241, 0.15); border-radius: 0.85rem;">1</span>
            <button onclick="nextPage()" class="btn btn-slate" style="padding: 0.4rem 0.8rem;" data-th="ถัดไป ▶" data-en="Next ▶">ถัดไป ▶</button>
            <button onclick="goToLastPage()" class="btn btn-slate" style="padding: 0.4rem 0.8rem;" data-th="ท้ายสุด ⏭️" data-en="Last ⏭️">ท้ายสุด ⏭️</button>
          </div>
        </div>
      </div>
    </div>

    <!-- TAB 3: TEMP CARD -->
    <div id="tab-tempcard" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12">
        <div style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 0.75rem; margin-bottom: 1.25rem;">
          <div>
            <h2 style="font-size: 1.2rem; font-weight: 800;" data-th="💳 บริหารจัดการบัตรสำรอง (One-Time Auto-Revoke)" data-en="💳 Temporary Cards (One-Time Auto-Revoke)">💳 บริหารจัดการบัตรสำรอง (One-Time Auto-Revoke)</h2>
            <p style="font-size: 0.85rem; color: var(--text-muted); margin-top: 0.2rem;" data-th="* บัตรจะถูกปลดออกจากระบบอัตโนมัติทันทีเมื่อนิสิตแตะรับสิทธิ์สำเร็จ เพื่อนำบัตรเดิมไปเวียนใช้งานต่อได้ทันที" data-en="* Cards are automatically unlinked upon successful tap, allowing immediate reuse.">* บัตรจะถูกปลดออกจากระบบอัตโนมัติทันทีเมื่อนิสิตแตะรับสิทธิ์สำเร็จ เพื่อนำบัตรเดิมไปเวียนใช้งานต่อได้ทันที</p>
          </div>
          <button onclick="openTempModal()" class="btn btn-emerald" data-th="➕ ผูกบัตรสำรองใหม่" data-en="➕ Assign Temporary Card">➕ ผูกบัตรสำรองใหม่</button>
        </div>
        <div class="table-container">
          <table>
            <thead><tr><th data-th="รหัสนิสิต" data-en="Student ID">รหัสนิสิต</th><th data-th="ชื่อ-สกุล" data-en="Full Name">ชื่อ-สกุล</th><th data-th="เลขบัตรที่ผูก" data-en="Assigned UID">เลขบัตรที่ผูก</th><th data-th="สถานะบัตรสำรอง" data-en="Temp Card Status">สถานะบัตรสำรอง</th><th style="text-align: right;" data-th="การจัดการ" data-en="Action">การจัดการ</th></tr></thead>
            <tbody>
  )rawliteral";

  bool hasTempRecords = false;
  for (size_t i = 0; i < db.size(); i++) {
    if (db[i].isTempCard) {
      hasTempRecords = true;
      String statusBadge = "";
      String actionBtn = "";

      if (db[i].claimed) {
        statusBadge = "<span style='background:rgba(16,185,129,0.2); color:var(--accent-green); padding:0.25rem 0.65rem; border-radius:9999px; font-size:0.75rem; font-weight:800;'>CLAIMED & REVOKED</span>";
        actionBtn = "<span style='color:var(--text-muted); font-size:0.8rem;' data-th='คืนสู่ส่วนกลางแล้ว' data-en='Card Returned'>คืนสู่ส่วนกลางแล้ว</span>";
      } else {
        statusBadge = "<span style='background:rgba(245,158,11,0.2); color:var(--accent-yellow); padding:0.25rem 0.65rem; border-radius:9999px; font-size:0.75rem; font-weight:800;'>WAITING TAP</span>";
        actionBtn = "<button type='button' class='btn btn-slate js-temp-revoke' data-id='" + htmlEscape(db[i].studentId) + "' style='color:var(--accent-rose); padding:0.35rem 0.65rem; font-size:0.75rem;'>Revoke</button>";
      }

      html += "<tr><td><b>" + htmlEscape(db[i].studentId) + "</b></td><td>" + htmlEscape(db[i].fullName) + "</td><td><code>" + htmlEscape(db[i].uid) + "</code></td>";
      html += "<td>" + statusBadge + "</td>";
      html += "<td style='text-align: right;'>" + actionBtn + "</td></tr>";
    }
  }
  if (!hasTempRecords) html += "<tr><td colspan='5' style='text-align: center; color: var(--text-muted); padding: 2rem;' data-th='ไม่มีรายการบัตรสำรองในวันนี้' data-en='No temporary card activity today'>ไม่มีรายการบัตรสำรองในวันนี้</td></tr>";

  html += R"rawliteral(
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <!-- TAB 4: ARCHIVES -->
    <div id="tab-archives" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12">
        <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 1rem;" data-th="🗄️ คลังรายงานประวัติย้อนหลัง (Audit Log Storage)" data-en="🗄️ Historical Audit Log Storage">🗄️ คลังรายงานประวัติย้อนหลัง (Audit Log Storage)</h2>
        <div class="table-container">
          <table>
            <thead><tr><th data-th="ชื่อไฟล์รายงาน" data-en="Archive Filename">ชื่อไฟล์รายงาน</th><th data-th="ขนาดไฟล์" data-en="File Size">ขนาดไฟล์</th><th style="text-align:right;" data-th="การจัดการ" data-en="Action">การจัดการ</th></tr></thead>
            <tbody>
  )rawliteral";

  html += archivesHtml;

  html += R"rawliteral(
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <!-- TAB 5: ADMINS -->
    <div id="tab-admins" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12">
        <div style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 0.75rem; margin-bottom: 1.25rem;">
          <div>
            <h2 style="font-size: 1.2rem; font-weight: 800;" data-th="👤 รายชื่อเจ้าหน้าที่ผู้มีสิทธิ์เข้าถึงระบบ (จำกัด 3 ท่าน)" data-en="👤 System Authorized Officers (Max 3 Users)">👤 รายชื่อเจ้าหน้าที่ผู้มีสิทธิ์เข้าถึงระบบ (จำกัด 3 ท่าน)</h2>
            <p style="font-size: 0.85rem; color: var(--text-muted); margin-top: 0.2rem;" data-th="เจ้าหน้าที่สามารถเข้าสู่ระบบเพื่อตรวจสอบแดชบอร์ด จัดการสิทธิ์ และส่งออกรายงานได้" data-en="Officers can log in to audit real-time subsidy claims, assign cards, and export logs.">เจ้าหน้าที่สามารถเข้าสู่ระบบเพื่อตรวจสอบแดชบอร์ด จัดการสิทธิ์ และส่งออกรายงานได้</p>
          </div>
  )rawliteral";

  if (adminUsers.size() < 3) {
    html += "<button onclick=\"openAddAdminModal()\" class=\"btn btn-indigo\" data-th=\"➕ เพิ่มเจ้าหน้าที่ใหม่\" data-en=\"➕ Add New Officer\">➕ เพิ่มเจ้าหน้าที่ใหม่</button>";
  } else {
    html += "<span style=\"color:var(--accent-yellow); background:rgba(245,158,11,0.2); padding:0.4rem 0.8rem; border-radius:0.75rem; font-size:0.8rem; font-weight:800;\">⚠️ Quota Reached (3/3)</span>";
  }

  html += R"rawliteral(
        </div>
        <div class="table-container">
          <table>
            <thead><tr><th data-th="ลำดับ" data-en="No.">ลำดับ</th><th data-th="ชื่อ-ตำแหน่งเจ้าหน้าที่" data-en="Officer Name / Role">ชื่อ-ตำแหน่งเจ้าหน้าที่</th><th data-th="ชื่อผู้ใช้ (Username)" data-en="Username">ชื่อผู้ใช้ (Username)</th><th data-th="รหัสผ่าน" data-en="Password">รหัสผ่าน</th><th style="text-align:right;" data-th="การจัดการ" data-en="Action">การจัดการ</th></tr></thead>
            <tbody>
  )rawliteral";

  for (size_t i = 0; i < adminUsers.size(); i++) {
    html += "<tr><td style='color:var(--text-muted); font-weight:600;'>" + String(i + 1) + "</td>";
    String safeUser = htmlEscape(adminUsers[i].username);
    String safeName = htmlEscape(adminUsers[i].displayName);
    html += "<td style='font-weight:700;'>" + safeName + "</td>";
    html += "<td><code>" + safeUser + "</code></td>";
    html += "<td style='color:var(--text-muted);'>••••••••</td>";
    html += "<td style='text-align:right;'>";
    html += "<button type=\"button\" class=\"btn btn-slate js-admin-edit\" data-user=\"" + safeUser + "\" data-name=\"" + safeName + "\" style=\"padding:0.35rem 0.75rem; font-size:0.8rem;\">✏️ Edit</button> ";
    if (adminUsers.size() > 1) {
      html += "<button type=\"button\" class=\"btn btn-rose js-admin-delete\" data-user=\"" + safeUser + "\" style=\"padding:0.35rem 0.75rem; font-size:0.8rem;\">🗑️ Delete</button>";
    }
    html += "</td></tr>";
  }

  html += R"rawliteral(
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <!-- TAB 6: SETTINGS -->
    <div id="tab-settings" class="tab-content" style="display: none;">
      <div class="bento-grid">
        <div class="col-span-6 bento-card">
          <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.5rem;" data-th="🕒 ตั้งค่าเวลามาตรฐานระบบ" data-en="🕒 System Time Synchronization">🕒 ตั้งค่าเวลามาตรฐานระบบ</h2>
          <form data-ajax="1" method="POST" action="/api/rtc/set" style="margin-top: 1rem;">
            <label style="font-size: 0.85rem; font-weight: 700;">กำหนดวันที่:</label>
            <input type="date" name="date" value=")rawliteral" + String(dateInputBuf) + R"rawliteral(" required class="form-input">
            <label style="font-size: 0.85rem; font-weight: 700;">กำหนดเวลา:</label>
            <input type="text" name="time" value=")rawliteral" + String(timeInputBuf) + R"rawliteral(" required class="form-input" placeholder="HH:MM:SS">
            <button type="submit" class="btn btn-indigo" style="width: 100%;">💾 บันทึกเวลามาตรฐาน</button>
          </form>
        </div>

        <div class="col-span-6 bento-card">
          <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.5rem;" data-th="⚙️ กำหนดช่วงเวลาเปิดให้บริการอาหาร" data-en="⚙️ Service Hours Window">⚙️ กำหนดช่วงเวลาเปิดให้บริการอาหาร</h2>
          <form data-ajax="1" method="POST" action="/api/settings/time" style="margin-top: 1rem;">
            <label style="font-size: 0.85rem; font-weight: 700;">สถานะการจำกัดเวลา:</label>
            <select name="enabled" class="form-input">
              <option value="1" )rawliteral" + String(timeWindowEnabled ? "selected" : "") + R"rawliteral(>เปิดใช้งาน (จำกัดเวลาตามกำหนด)</option>
              <option value="0" )rawliteral" + String(!timeWindowEnabled ? "selected" : "") + R"rawliteral(>ปิดใช้งาน (เปิดบริการตลอดเวลา)</option>
            </select>
            <div style="display:flex; gap:1rem;">
              <div style="flex:1;">
                <label style="font-size: 0.85rem; font-weight: 700;">เวลาเปิด:</label>
                <input type="text" name="start" value=")rawliteral" + String(serviceStartHour < 10 ? "0" : "") + String(serviceStartHour) + ":" + String(serviceStartMin < 10 ? "0" : "") + String(serviceStartMin) + R"rawliteral(" class="form-input">
              </div>
              <div style="flex:1;">
                <label style="font-size: 0.85rem; font-weight: 700;">เวลาปิด:</label>
                <input type="text" name="end" value=")rawliteral" + String(serviceEndHour < 10 ? "0" : "") + String(serviceEndHour) + ":" + String(serviceEndMin < 10 ? "0" : "") + String(serviceEndMin) + R"rawliteral(" class="form-input">
              </div>
            </div>
            <button type="submit" class="btn btn-indigo" style="width: 100%;">💾 บันทึกกำหนดเวลา</button>
          </form>
        </div>
      </div>
    </div>

    <!-- TAB 7: IMPORT -->
    <div id="tab-import" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12" style="max-width: 36rem; margin: auto; text-align: center;">
        <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.5rem;">📥 นำเข้าบัญชีรายชื่อนิสิต (Smart Upsert)</h2>
        <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1.5rem;">* ระบบจะไม่ลบรายชื่อเก่า: รหัสเดิมจะถูกอัปเดตข้อมูล และรหัสใหม่จะถูกเพิ่มเข้าสู่ฐานข้อมูลอัตโนมัติ</p>
        <form data-ajax="1" method="POST" action="/upload" enctype="multipart/form-data" style="border: 2px dashed var(--border-card); border-radius: 1.5rem; padding: 2rem;">
          <input type="file" name="csv" accept=".csv" required style="margin-bottom: 1.25rem;"><br>
          <button type="submit" class="btn btn-emerald">🚀 อัปโหลดและผสานข้อมูล</button>
        </form>
      </div>
    </div>

    <!-- TAB 8: SHOPS -->
    <div id="tab-shops" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12" style="max-width: 38rem; margin: auto;">
        <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 1rem;">🏪 จัดการข้อมูลร้านค้าและผู้ประกอบการ</h2>
        <form method="POST" action="/api/shops/save" data-ajax="1">
  )rawliteral";

  for (int i = 0; i < 4; i++) {
    html += "<div style='background: var(--bg-surface-elevated); border: 1px solid var(--border-card); border-radius: 1.25rem; padding: 1.25rem; margin-bottom: 1rem;'>";
    html += "<h4 style='color: var(--accent-indigo); margin-bottom: 0.5rem; font-weight: 800;'>Point " + String(i + 1) + "</h4>";
    html += "<input type='text' name='sname" + String(i) + "' value='" + htmlEscape(shops[i].name) + "' required maxlength='48' class='form-input'>";
    html += "<input type='text' name='vname" + String(i) + "' value='" + htmlEscape(shops[i].vendor) + "' required maxlength='64' class='form-input' style='margin-bottom:0;'></div>";
  }

  html += R"rawliteral(
          <button type="submit" class="btn btn-indigo" style="width: 100%;">💾 บันทึกข้อมูลร้านค้า</button>
        </form>
      </div>
    </div>

    <!-- Modals -->
    <div id="studentModal" class="modal">
      <div class="bento-card" style="max-width: 28rem; width: 100%;">
        <h3 id="modalTitle" style="font-size: 1.2rem; font-weight: 800; margin-bottom: 1rem;">Beneficiary Details</h3>
        <form data-ajax="1" method="POST" action="/api/student/save">
          <input type="hidden" id="modalOldId" name="oldStudentId">
          <label style="font-size: 0.85rem; font-weight: 700;">รหัสนิสิต:</label>
          <input type="text" id="modalId" name="studentId" required class="form-input">
          <label style="font-size: 0.85rem; font-weight: 700;">ชื่อ-สกุล:</label>
          <input type="text" id="modalName" name="fullName" required class="form-input">
          <label style="font-size: 0.85rem; font-weight: 700;">เลขสมาร์ตการ์ด:</label>
          <input type="text" id="modalUid" name="uid" class="form-input" placeholder="e.g. 0305419896">
          <div style="display: flex; justify-content: flex-end; gap: 0.5rem;">
            <button type="button" onclick="closeModal()" class="btn btn-slate">ยกเลิก</button>
            <button type="submit" class="btn btn-indigo">บันทึก</button>
          </div>
        </form>
      </div>
    </div>

    <div id="tempCardModal" class="modal">
      <div class="bento-card" style="max-width: 28rem; width: 100%;">
        <h3 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 1rem;">ผูกบัตรสำรองกรณีพิเศษ</h3>
        <form data-ajax="1" method="POST" action="/api/tempcard/save">
          <label style="font-size: 0.85rem; font-weight: 700;">รหัสนิสิต:</label>
          <input type="text" name="studentId" required class="form-input">
          <label style="font-size: 0.85rem; font-weight: 700;">เลขบัตรสำรอง (10 หลัก):</label>
          <input type="text" name="uid" required class="form-input" placeholder="e.g. 0305419896">
          <div style="display: flex; justify-content: flex-end; gap: 0.5rem;">
            <button type="button" onclick="closeTempModal()" class="btn btn-slate">ยกเลิก</button>
            <button type="submit" class="btn btn-emerald">ผูกบัตร</button>
          </div>
        </form>
      </div>
    </div>

    <div id="adminModal" class="modal">
      <div class="bento-card" style="max-width: 28rem; width: 100%;">
        <h3 id="adminModalTitle" style="font-size: 1.2rem; font-weight: 800; margin-bottom: 1rem;">Officer Details</h3>
        <form data-ajax="1" method="POST" action="/api/admin/save">
          <input type="hidden" id="adminOldUser" name="oldUsername">
          <label style="font-size: 0.85rem; font-weight: 700;">ชื่อ-ตำแหน่งเจ้าหน้าที่ (Display Name):</label>
          <input type="text" id="adminDisplayName" name="displayName" required class="form-input" placeholder="e.g. นายกิตติพันธ์ รัตนคร (IT Officer)">
          <label style="font-size: 0.85rem; font-weight: 700;">ชื่อผู้ใช้ (Username):</label>
          <input type="text" id="adminUsername" name="username" required class="form-input" placeholder="e.g. officer01">
          <label style="font-size: 0.85rem; font-weight: 700;">รหัสผ่าน (Password):</label>
          <input type="password" id="adminPassword" name="password" required class="form-input" placeholder="••••••••">
          <div style="display: flex; justify-content: flex-end; gap: 0.5rem;">
            <button type="button" onclick="closeAdminModal()" class="btn btn-slate">ยกเลิก</button>
            <button type="submit" class="btn btn-indigo">บันทึกเจ้าหน้าที่</button>
          </div>
        </form>
      </div>
    </div>
  </main>

  <div class="toast-stack" id="toastStack" aria-live="polite"></div>

  <!-- Modal: ตัดสิทธิ์ด้วยตนเอง (แทน prompt() เดิม) -->
  <div id="manualClaimModal" class="modal">
    <div class="bento-card" style="max-width: 26rem; width: 100%;">
      <h3 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.35rem;">ตัดสิทธิ์ด้วยตนเอง</h3>
      <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1rem;">ใช้กรณีนิสิตลืมบัตรหรือเครื่องอ่านขัดข้อง ระบบจะบันทึกลงประวัติเหมือนการแตะบัตรปกติ</p>
      <div style="font-size: 0.85rem; font-weight: 700; margin-bottom: 0.5rem;">รหัสนิสิต: <span id="manualClaimId" style="color: var(--accent-indigo);"></span></div>
      <label style="font-size: 0.85rem; font-weight: 700;">เลือกจุดบริการ:</label>
      <select id="manualClaimStation" class="form-input">
        <option value="1">จุดบริการ 1</option>
        <option value="2">จุดบริการ 2</option>
        <option value="3">จุดบริการ 3</option>
        <option value="4">จุดบริการ 4</option>
      </select>
      <div style="display: flex; justify-content: flex-end; gap: 0.5rem;">
        <button type="button" onclick="closeManualClaim()" class="btn btn-slate">ยกเลิก</button>
        <button type="button" onclick="confirmManualClaim()" class="btn btn-emerald">ยืนยันตัดสิทธิ์</button>
      </div>
    </div>
  </div>

  <!-- Modal: ผูกบัตรสำรองจากตารางรายชื่อ (แทน prompt() เดิม) -->
  <div id="assignTempModal" class="modal">
    <div class="bento-card" style="max-width: 26rem; width: 100%;">
      <h3 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.35rem;">ผูกบัตรสำรอง</h3>
      <div style="font-size: 0.85rem; font-weight: 700; margin-bottom: 0.75rem;">รหัสนิสิต: <span id="assignTempId" style="color: var(--accent-indigo);"></span></div>
      <label style="font-size: 0.85rem; font-weight: 700;">เลขบัตรสำรอง (10 หลัก):</label>
      <input type="text" id="assignTempUid" class="form-input" maxlength="15" placeholder="e.g. 0305419896">
      <div style="display: flex; justify-content: flex-end; gap: 0.5rem;">
        <button type="button" onclick="closeAssignTemp()" class="btn btn-slate">ยกเลิก</button>
        <button type="button" onclick="confirmAssignTemp()" class="btn btn-emerald">ผูกบัตร</button>
      </div>
    </div>
  </div>

  <footer class="dashboard-footer">
    <p style="font-weight: 800; font-size: 0.95rem; color: var(--text-main); margin-bottom: 0.35rem;">
      ระบบบริหารจัดการคูปองอาหารดิจิทัล (Smart Canteen Bento Suite)
    </p>
    <p style="margin-bottom: 0.25rem;">
      ออกแบบและพัฒนาโดย: <b style="color: var(--accent-indigo);">กิตติพันธ์ รัตนคร</b> | นักวิชาการคอมพิวเตอร์
    </p>
    <p style="color: var(--text-muted); font-size: 0.8rem;">
      มหาวิทยาลัยมหาจุฬาลงกรณราชวิทยาลัย วิทยาเขตแพร่ (MCU Phrae Campus)
    </p>
  </footer>

  <script>
    var curPage = 1, pageSize = 20, totalPages = 1, searchQuery = "", searchTimer = null;
    var currentLang = 'th';
    var dashboardData = null;
    var dashboardTimer = null;

    /* ---------------------------------------------------------------
       Toast: แทน alert() เดิมที่บังคับให้รีโหลดทั้งหน้าและทำให้เสียแท็บที่ค้างอยู่
       --------------------------------------------------------------- */
    function toast(message, ok) {
      var stack = document.getElementById('toastStack');
      if (!stack) return;
      var el = document.createElement('div');
      el.className = 'toast ' + (ok === false ? 'err' : 'ok');
      var icon = document.createElement('span');
      icon.textContent = (ok === false) ? '⚠️' : '✅';
      var text = document.createElement('span');
      text.textContent = message;          /* textContent = ปลอดภัยจาก HTML injection */
      el.appendChild(icon);
      el.appendChild(text);
      stack.appendChild(el);
      setTimeout(function () {
        el.classList.add('leaving');
        setTimeout(function () { if (el.parentNode) el.parentNode.removeChild(el); }, 300);
      }, 4200);
    }

    /* เรียก API แบบ POST พร้อมจัดการข้อผิดพลาดและ session หมดอายุให้ครบทุกทาง */
    function api(url, params) {
      var body = new URLSearchParams();
      if (params) { for (var k in params) { if (params.hasOwnProperty(k)) body.append(k, params[k]); } }
      return fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8' },
        body: body.toString()
      }).then(function (res) {
        if (res.status === 401) { window.location.href = '/login'; throw new Error('unauthorized'); }
        return res.json().catch(function () { return { ok: false, msg: 'เซิร์ฟเวอร์ตอบกลับไม่ถูกต้อง' }; });
      }).then(function (data) {
        toast(data.msg || (data.ok ? 'สำเร็จ' : 'ไม่สำเร็จ'), data.ok);
        return data;
      }).catch(function (err) {
        if (err && err.message === 'unauthorized') throw err;
        toast('ติดต่อเครื่องแม่ข่ายไม่สำเร็จ กรุณาตรวจสอบการเชื่อมต่อ', false);
        throw err;
      });
    }

    /* ส่งฟอร์มทุกใบแบบ AJAX แล้วรีเฟรชเฉพาะส่วนที่เปลี่ยน */
    function bindAjaxForms() {
      document.querySelectorAll('form[data-ajax]').forEach(function (form) {
        form.addEventListener('submit', function (ev) {
          ev.preventDefault();
          var btn = form.querySelector('button[type="submit"]');
          var original = btn ? btn.innerHTML : '';
          if (btn) { btn.disabled = true; btn.innerHTML = '⏳ กำลังบันทึก...'; }

          fetch(form.getAttribute('action'), { method: 'POST', body: new FormData(form) })
            .then(function (res) {
              if (res.status === 401) { window.location.href = '/login'; throw new Error('unauthorized'); }
              return res.json().catch(function () { return { ok: false, msg: 'เซิร์ฟเวอร์ตอบกลับไม่ถูกต้อง' }; });
            })
            .then(function (data) {
              toast(data.msg || (data.ok ? 'บันทึกสำเร็จ' : 'บันทึกไม่สำเร็จ'), data.ok);
              if (!data.ok) return;
              closeModal(); closeTempModal(); closeAdminModal();
              form.querySelectorAll('input[type="file"]').forEach(function (i) { i.value = ''; });
              refreshDashboard();
              if (document.getElementById('tab-students').style.display !== 'none') loadStudents(curPage);
              if (form.getAttribute('action').indexOf('/api/admin/') === 0 ||
                  form.getAttribute('action').indexOf('/api/tempcard/') === 0 ||
                  form.getAttribute('action') === '/upload') {
                setTimeout(function () { window.location.reload(); }, 900);
              }
            })
            .catch(function () { })
            .then(function () { if (btn) { btn.disabled = false; btn.innerHTML = original; } });
        });
      });
    }

    /* ---------------------------------------------------------------
       ธีมและภาษา
       --------------------------------------------------------------- */
    function initTheme() {
      var savedTheme = localStorage.getItem('canteen_theme') || 'dark';
      document.documentElement.setAttribute('data-theme', savedTheme);
      document.getElementById('themeBtn').innerText = (savedTheme === 'dark') ? '🌙' : '☀️';
    }

    function toggleTheme() {
      var curTheme = document.documentElement.getAttribute('data-theme');
      var newTheme = (curTheme === 'dark') ? 'light' : 'dark';
      document.documentElement.setAttribute('data-theme', newTheme);
      localStorage.setItem('canteen_theme', newTheme);
      document.getElementById('themeBtn').innerText = (newTheme === 'dark') ? '🌙' : '☀️';
      drawAnalyticsChart();
    }

    function toggleLanguage() {
      currentLang = (currentLang === 'th') ? 'en' : 'th';
      document.getElementById('langSwitch').innerText = (currentLang === 'th') ? 'EN' : 'TH';
      document.querySelectorAll('[data-th]').forEach(function (el) {
        var v = el.getAttribute('data-' + currentLang);
        if (v !== null) el.innerText = v;
      });
      var sInput = document.getElementById('search');
      if (sInput) {
        sInput.placeholder = (currentLang === 'th') ? 'ค้นหารหัสนิสิต, ชื่อ-สกุล หรือเลขประจำตัว...' : 'Search Beneficiary ID, Name, or Card UID...';
      }
      renderStations();
      drawAnalyticsChart();
      loadStudents(curPage);
    }

    function toggleNav() {
      var m = document.getElementById('navMenu');
      if (m) m.classList.toggle('open');
    }

    /* ---------------------------------------------------------------
       แดชบอร์ดสด: ดึงตัวเลขจริงจากเครื่องแม่ข่ายทุก 3 วินาที
       --------------------------------------------------------------- */
    function setText(id, value) {
      var el = document.getElementById(id);
      if (el) el.textContent = value;
    }

    function refreshDashboard() {
      return fetch('/api/dashboard')
        .then(function (res) {
          if (res.status === 401) { window.location.href = '/login'; throw new Error('unauthorized'); }
          return res.json();
        })
        .then(function (d) {
          dashboardData = d;

          setText('telemetryTemp', d.temp.toFixed(1) + ' °C');
          setText('telemetryCpu', d.cpu.toFixed(1) + ' %');
          setText('statBatt', d.battPct + '%');
          setText('statHeap', d.heap + ' KB');
          setText('statUsed', d.used);
          setText('statRemaining', d.remaining);
          setText('statDisbursed', d.disbursed);
          setText('clockText', d.clock);

          var tEl = document.getElementById('telemetryTemp');
          if (tEl) tEl.style.color = (d.temp < 65) ? 'var(--accent-green)' : 'var(--accent-rose)';

          var arc = document.getElementById('gaugeArc');
          if (arc) arc.style.strokeDashoffset = String(252 - Math.round(d.quotaPct * 252 / 100));

          var badge = document.getElementById('serviceBadge');
          if (badge) {
            badge.textContent = (d.serviceOpen ? '🟢 เปิดให้บริการ ' : '🔴 นอกเวลาให้บริการ ') + d.window;
            badge.style.background = d.serviceOpen ? 'var(--accent-green-glow)' : 'rgba(244,63,94,0.15)';
            badge.style.color = d.serviceOpen ? 'var(--accent-green)' : 'var(--accent-rose)';
          }

          var tempBadge = document.getElementById('tempCount');
          if (tempBadge) tempBadge.textContent = d.tempWaiting;

          renderStations();
          drawAnalyticsChart();
          return d;
        })
        .catch(function () { });
    }

    function signalBars(quality, online) {
      var wrap = document.createElement('span');
      wrap.className = 'sig-bars';
      var active = 0;
      if (online) { active = quality >= 85 ? 4 : quality >= 60 ? 3 : quality >= 35 ? 2 : 1; }
      wrap.style.color = !online ? 'var(--accent-rose)'
                       : (quality >= 60 ? 'var(--accent-green)' : quality >= 35 ? 'var(--accent-yellow)' : 'var(--accent-rose)');
      for (var i = 0; i < 4; i++) {
        var bar = document.createElement('i');
        bar.style.height = (4 + i * 3) + 'px';
        if (i < active) bar.className = 'on';
        wrap.appendChild(bar);
      }
      return wrap;
    }

    function metricBlock(label, value, color) {
      var box = document.createElement('div');
      var cap = document.createElement('div');
      cap.style.cssText = 'font-size:0.72rem; color:var(--text-muted); text-transform:uppercase;';
      cap.textContent = label;
      var val = document.createElement('div');
      val.style.cssText = 'font-size:1.2rem; font-weight:800; color:' + color + ';';
      val.textContent = value;
      box.appendChild(cap); box.appendChild(val);
      return box;
    }

    function renderStations() {
      var grid = document.getElementById('stationGrid');
      if (!grid || !dashboardData) return;
      grid.innerHTML = '';

      dashboardData.stations.forEach(function (st, idx) {
        var shop = dashboardData.shops[idx] || { name: 'Station', vendor: '', count: 0, amount: 0 };
        var card = document.createElement('div');
        card.className = 'col-span-6 station-card' + (st.online ? '' : ' offline');

        var head = document.createElement('div');
        head.style.cssText = 'display:flex; justify-content:space-between; align-items:flex-start; gap:0.75rem;';

        var titleWrap = document.createElement('div');
        var title = document.createElement('h4');
        title.style.cssText = 'font-size:1.05rem; font-weight:800;';
        title.textContent = shop.name;
        var vendor = document.createElement('p');
        vendor.style.cssText = 'font-size:0.82rem; color:var(--text-muted); margin-top:0.2rem;';
        vendor.textContent = (currentLang === 'th' ? 'ผู้ประกอบการ: ' : 'Vendor: ') + shop.vendor;
        titleWrap.appendChild(title); titleWrap.appendChild(vendor);

        var status = document.createElement('span');
        status.style.cssText = 'font-size:0.72rem; font-weight:800; padding:0.25rem 0.6rem; border-radius:9999px; white-space:nowrap; ' +
          (st.online ? 'background:var(--accent-green-glow); color:var(--accent-green);'
                     : 'background:rgba(244,63,94,0.15); color:var(--accent-rose);');
        if (st.online) {
          var dot = document.createElement('span');
          dot.className = 'live-dot';
          status.appendChild(dot);
          status.appendChild(document.createTextNode('ONLINE'));
          status.title = 'สัญญาณ ' + st.rssi + ' dBm · ได้ยินล่าสุดเมื่อ ' + st.ageSec + ' วินาทีที่แล้ว';
        } else {
          status.textContent = 'OFFLINE';
          status.title = 'ไม่ได้รับสัญญาณจากจุดบริการนี้เกิน 15 วินาที';
        }

        head.appendChild(titleWrap); head.appendChild(status);

        var body = document.createElement('div');
        body.style.cssText = 'display:flex; justify-content:space-between; align-items:center; gap:0.5rem; margin-top:1.1rem; border-top:1px dashed var(--border-card); padding-top:0.75rem; flex-wrap:wrap;';
        body.appendChild(metricBlock(currentLang === 'th' ? 'จำนวนที่จ่าย' : 'Orders', shop.count + (currentLang === 'th' ? ' จาน' : ' meals'), 'var(--accent-green)'));
        body.appendChild(metricBlock(currentLang === 'th' ? 'ยอดรวม' : 'Amount', shop.amount + ' B.', 'var(--accent-yellow)'));
        body.appendChild(metricBlock(currentLang === 'th' ? 'แบตเตอรี่' : 'Battery', st.online ? (st.battPct + '% / ' + st.volt.toFixed(2) + 'V') : '—', 'var(--text-main)'));

        var sig = document.createElement('div');
        sig.style.cssText = 'display:flex; align-items:center; gap:0.4rem;';
        sig.appendChild(signalBars(st.quality, st.online));
        var sigText = document.createElement('span');
        sigText.style.cssText = 'font-size:0.78rem; color:var(--text-muted);';
        sigText.textContent = st.online ? (st.rssi + ' dBm') : '—';
        sig.appendChild(sigText);
        body.appendChild(sig);

        card.appendChild(head); card.appendChild(body);
        grid.appendChild(card);
      });
    }

    function syncDeviceTime() {
      var now = new Date();
      var pad = function (n) { return n < 10 ? '0' + n : n; };
      var d = now.getFullYear() + '-' + pad(now.getMonth() + 1) + '-' + pad(now.getDate());
      var t = pad(now.getHours()) + ':' + pad(now.getMinutes()) + ':' + pad(now.getSeconds());
      api('/api/rtc/set', { date: d, time: t }).then(refreshDashboard).catch(function () { });
    }

    function dailyReset() {
      if (!confirm(currentLang === 'th'
            ? 'ต้องการปิดยอดประจำวัน จัดเก็บประวัติเข้าคลัง และคืนสิทธิ์ให้นิสิตทุกคนใช่หรือไม่?'
            : 'Archive today records and reset every claim?')) return;
      api('/api/system/reset').then(function (d) {
        if (d.ok) { refreshDashboard(); loadStudents(1); }
      }).catch(function () { });
    }

    function drawAnalyticsChart() {
      var canvas = document.getElementById('analyticsChart');
      if (!canvas) return;
      var ctx = canvas.getContext('2d');
      var isDark = document.documentElement.getAttribute('data-theme') === 'dark';
      var data = [0, 0, 0, 0];
      var labels = (currentLang === 'th') ? ['ร้านที่ 1', 'ร้านที่ 2', 'ร้านที่ 3', 'ร้านที่ 4'] : ['Shop 01', 'Shop 02', 'Shop 03', 'Shop 04'];
      if (dashboardData && dashboardData.shops) {
        data = dashboardData.shops.map(function (sh) { return sh.count; });
        labels = dashboardData.shops.map(function (sh) { return sh.name; });
      }
      var colors = ['#10b981', '#06b6d4', '#f59e0b', '#f43f5e'];
      var maxVal = Math.max.apply(null, data.concat([10]));

      ctx.clearRect(0, 0, canvas.width, canvas.height);
      var chartH = 150, startY = 180, barW = 75, gap = 65, startX = 100;
      ctx.strokeStyle = isDark ? 'rgba(255,255,255,0.08)' : 'rgba(0,0,0,0.08)';
      ctx.lineWidth = 1.5;
      ctx.beginPath(); ctx.moveTo(60, startY); ctx.lineTo(640, startY); ctx.stroke();

      for (var i = 0; i < data.length; i++) {
        var h = (data[i] / maxVal) * chartH;
        var x = startX + (i * (barW + gap));
        var y = startY - h;

        ctx.fillStyle = colors[i % colors.length];
        ctx.beginPath();
        if (typeof ctx.roundRect === 'function') ctx.roundRect(x, y, barW, h, [12, 12, 0, 0]);
        else ctx.rect(x, y, barW, h);
        ctx.fill();

        ctx.fillStyle = isDark ? '#f8fafc' : '#0f172a';
        ctx.font = 'bold 14px "Plus Jakarta Sans", Sarabun';
        ctx.textAlign = 'center';
        ctx.fillText(data[i] + (currentLang === 'th' ? ' จาน' : ''), x + (barW / 2), y - 10);

        ctx.fillStyle = isDark ? '#94a3b8' : '#64748b';
        ctx.font = '500 13px "Plus Jakarta Sans", Sarabun';
        ctx.fillText(labels[i], x + (barW / 2), startY + 24);
      }
    }

    function switchTab(tabId) {
      document.querySelectorAll('.tab-content').forEach(function (el) { el.style.display = 'none'; });
      document.querySelectorAll('.nav-item, .dropdown-item').forEach(function (el) { el.classList.remove('active'); });

      var targetTab = document.getElementById('tab-' + tabId);
      if (targetTab) targetTab.style.display = 'block';

      var activeBtn = document.getElementById('btn-' + tabId);
      if (activeBtn) {
        activeBtn.classList.add('active');
        var parentDropdown = activeBtn.closest('.dropdown');
        if (parentDropdown) {
          var trigger = parentDropdown.querySelector('.dropdown-btn');
          if (trigger) trigger.classList.add('active');
        }
      }
      var m = document.getElementById('navMenu');
      if (m) m.classList.remove('open');
      try { localStorage.setItem('canteen_tab', tabId); } catch (e) { }
      if (tabId === 'dashboard') { refreshDashboard(); }
      else if (tabId === 'students') loadStudents(curPage);
    }

    /* ---------------------------------------------------------------
       ตารางรายชื่อ: สร้างแถวด้วย DOM ทั้งหมด ชื่อที่มี < > " ' จึงปลอดภัย
       --------------------------------------------------------------- */
    function skeletonRows(count) {
      var tbody = document.getElementById('studentTableBody');
      tbody.innerHTML = '';
      for (var r = 0; r < count; r++) {
        var tr = document.createElement('tr');
        for (var c = 0; c < 7; c++) {
          var td = document.createElement('td');
          var sk = document.createElement('span');
          sk.className = 'skeleton';
          sk.style.width = (c === 1 ? '80%' : '60%');
          td.appendChild(sk);
          tr.appendChild(td);
        }
        tbody.appendChild(tr);
      }
    }

    function badge(text, color, bg) {
      var el = document.createElement('span');
      el.style.cssText = 'background:' + bg + '; color:' + color + '; font-size:0.78rem; padding:0.3rem 0.7rem; border-radius:0.75rem; font-weight:700; white-space:nowrap;';
      el.textContent = text;
      return el;
    }

    function smallButton(label, title, handler, danger) {
      var b = document.createElement('button');
      b.type = 'button';
      b.className = 'btn btn-slate';
      b.style.cssText = 'padding:0.3rem 0.6rem; font-size:0.75rem;' + (danger ? ' color:var(--accent-rose);' : '');
      b.textContent = label;
      b.title = title;
      b.addEventListener('click', handler);
      return b;
    }

    function loadStudents(page) {
      if (page < 1) page = 1;
      curPage = page;
      skeletonRows(Math.min(pageSize, 8));

      fetch('/api/students?page=' + curPage + '&limit=' + pageSize + '&search=' + encodeURIComponent(searchQuery))
        .then(function (res) {
          if (res.status === 401) { window.location.href = '/login'; throw new Error('unauthorized'); }
          return res.json();
        })
        .then(function (data) {
          totalPages = data.totalPages; curPage = data.currentPage;
          var pInd = document.getElementById('pageIndicator');
          if (pInd) pInd.textContent = curPage + ' / ' + totalPages;

          var pInfo = document.getElementById('paginationInfo');
          if (pInfo) {
            var start = (data.totalItems === 0) ? 0 : (curPage - 1) * pageSize + 1;
            var end = Math.min(curPage * pageSize, data.totalItems);
            pInfo.textContent = (currentLang === 'th')
              ? ('แสดง ' + start + ' - ' + end + ' จาก ' + data.totalItems + ' รายการ')
              : ('Showing ' + start + ' - ' + end + ' of ' + data.totalItems + ' records');
          }

          var tbody = document.getElementById('studentTableBody');
          tbody.innerHTML = '';

          if (data.students.length === 0) {
            var emptyRow = document.createElement('tr');
            var emptyCell = document.createElement('td');
            emptyCell.colSpan = 7;
            emptyCell.style.cssText = 'padding:2.5rem; text-align:center; color:var(--text-muted);';
            emptyCell.textContent = (currentLang === 'th' ? 'ไม่พบข้อมูลที่ตรงกับเงื่อนไขการค้นหา' : 'No records match your search');
            emptyRow.appendChild(emptyCell);
            tbody.appendChild(emptyRow);
            return;
          }

          data.students.forEach(function (st) {
            var tr = document.createElement('tr');

            var tdId = document.createElement('td');
            var bId = document.createElement('b');
            bId.textContent = st.id;
            tdId.appendChild(bId);

            var tdName = document.createElement('td');
            tdName.textContent = st.name;

            var tdUid = document.createElement('td');
            var codeUid = document.createElement('code');
            codeUid.textContent = st.uid || '—';
            tdUid.appendChild(codeUid);

            var tdRef = document.createElement('td');
            var codeRef = document.createElement('code');
            codeRef.textContent = st.ref;
            tdRef.appendChild(codeRef);
            if (st.isTemp) {
              tdRef.appendChild(document.createTextNode(' '));
              tdRef.appendChild(badge('Temp', 'var(--accent-yellow)', 'rgba(245,158,11,0.2)'));
            }

            var tdStatus = document.createElement('td');
            tdStatus.appendChild(st.claimed
              ? badge(st.time, 'var(--accent-green)', 'rgba(16,185,129,0.2)')
              : badge('READY', 'var(--accent-rose)', 'rgba(244,63,94,0.15)'));

            var tdShop = document.createElement('td');
            tdShop.textContent = st.shopName;

            var tdAct = document.createElement('td');
            tdAct.style.textAlign = 'right';
            tdAct.style.whiteSpace = 'nowrap';
            if (!st.claimed) {
              tdAct.appendChild(smallButton('💳 Temp', 'ผูกบัตรสำรองให้นิสิตรายนี้', (function (id) {
                return function () { openAssignTemp(id); };
              })(st.id), false));
              tdAct.appendChild(document.createTextNode(' '));
              tdAct.appendChild(smallButton('✔ ตัดสิทธิ์', 'บันทึกการรับสิทธิ์ด้วยตนเอง กรณีลืมบัตรหรือเครื่องอ่านขัดข้อง', (function (id) {
                return function () { openManualClaim(id); };
              })(st.id), false));
              tdAct.appendChild(document.createTextNode(' '));
            }
            tdAct.appendChild(smallButton('✏️', 'แก้ไขข้อมูลนิสิต', (function (a, b, c) {
              return function () { openEditModal(a, b, c); };
            })(st.id, st.name, st.uid), false));
            tdAct.appendChild(document.createTextNode(' '));
            tdAct.appendChild(smallButton('🗑️', 'ลบรายชื่อออกจากระบบ', (function (id) {
              return function () { deleteStudent(id); };
            })(st.id), true));

            tr.appendChild(tdId); tr.appendChild(tdName); tr.appendChild(tdUid);
            tr.appendChild(tdRef); tr.appendChild(tdStatus); tr.appendChild(tdShop); tr.appendChild(tdAct);
            tbody.appendChild(tr);
          });
        })
        .catch(function () {
          var tbody = document.getElementById('studentTableBody');
          if (tbody) tbody.innerHTML = '';
          toast('โหลดรายชื่อไม่สำเร็จ', false);
        });
    }

    function changePageSize(val) { pageSize = parseInt(val, 10); curPage = 1; loadStudents(1); }
    function prevPage() { if (curPage > 1) loadStudents(curPage - 1); }
    function nextPage() { if (curPage < totalPages) loadStudents(curPage + 1); }
    function goToPage(p) { loadStudents(p); }
    function goToLastPage() { loadStudents(totalPages); }
    function handleSearch(val) {
      clearTimeout(searchTimer);
      searchTimer = setTimeout(function () { searchQuery = val.trim(); curPage = 1; loadStudents(1); }, 350);
    }

    /* --------------------------- Modals --------------------------- */
    function openAddModal() {
      document.getElementById('modalOldId').value = ''; document.getElementById('modalId').value = '';
      document.getElementById('modalName').value = ''; document.getElementById('modalUid').value = '';
      document.getElementById('modalTitle').textContent = 'เพิ่มผู้มีสิทธิ์รายใหม่';
      document.getElementById('studentModal').classList.add('active');
    }
    function openEditModal(id, name, uid) {
      document.getElementById('modalOldId').value = id; document.getElementById('modalId').value = id;
      document.getElementById('modalName').value = name; document.getElementById('modalUid').value = uid;
      document.getElementById('modalTitle').textContent = 'แก้ไขข้อมูล: ' + id;
      document.getElementById('studentModal').classList.add('active');
    }
    function closeModal() { document.getElementById('studentModal').classList.remove('active'); }
    function openTempModal() { document.getElementById('tempCardModal').classList.add('active'); }
    function closeTempModal() { document.getElementById('tempCardModal').classList.remove('active'); }

    var manualClaimTarget = '', assignTempTarget = '';
    function openManualClaim(id) {
      manualClaimTarget = id;
      document.getElementById('manualClaimId').textContent = id;
      document.getElementById('manualClaimStation').value = '1';
      document.getElementById('manualClaimModal').classList.add('active');
    }
    function closeManualClaim() { document.getElementById('manualClaimModal').classList.remove('active'); }
    function confirmManualClaim() {
      var station = document.getElementById('manualClaimStation').value;
      api('/api/claim/manual', { id: manualClaimTarget, station: station }).then(function (d) {
        if (d.ok) { closeManualClaim(); loadStudents(curPage); refreshDashboard(); }
      }).catch(function () { });
    }

    function openAssignTemp(id) {
      assignTempTarget = id;
      document.getElementById('assignTempId').textContent = id;
      document.getElementById('assignTempUid').value = '';
      document.getElementById('assignTempModal').classList.add('active');
      setTimeout(function () { document.getElementById('assignTempUid').focus(); }, 60);
    }
    function closeAssignTemp() { document.getElementById('assignTempModal').classList.remove('active'); }
    function confirmAssignTemp() {
      var uid = document.getElementById('assignTempUid').value.trim();
      if (!uid) { toast('กรุณากรอกเลขบัตรสำรอง', false); return; }
      api('/api/tempcard/save', { studentId: assignTempTarget, uid: uid }).then(function (d) {
        if (d.ok) { closeAssignTemp(); loadStudents(curPage); refreshDashboard(); }
      }).catch(function () { });
    }

    function removeTempCard(id) {
      if (!confirm('ต้องการยกเลิกบัตรสำรองของนิสิต ' + id + ' ใช่หรือไม่?')) return;
      api('/api/tempcard/remove', { id: id }).then(function (d) {
        if (d.ok) setTimeout(function () { window.location.reload(); }, 700);
      }).catch(function () { });
    }
    function deleteStudent(id) {
      if (!confirm('ต้องการลบรายชื่อนิสิต ' + id + ' ออกจากระบบใช่หรือไม่?')) return;
      api('/api/student/delete', { id: id }).then(function (d) {
        if (d.ok) { loadStudents(curPage); refreshDashboard(); }
      }).catch(function () { });
    }

    function openAddAdminModal() {
      document.getElementById('adminOldUser').value = '';
      document.getElementById('adminUsername').value = '';
      document.getElementById('adminDisplayName').value = '';
      document.getElementById('adminPassword').value = '';
      document.getElementById('adminPassword').required = true;
      document.getElementById('adminModalTitle').textContent = 'เพิ่มเจ้าหน้าที่ใหม่ (สูงสุด 3 ท่าน)';
      document.getElementById('adminModal').classList.add('active');
    }
    function openEditAdminModal(user, name) {
      document.getElementById('adminOldUser').value = user;
      document.getElementById('adminUsername').value = user;
      document.getElementById('adminDisplayName').value = name;
      document.getElementById('adminPassword').value = '';
      document.getElementById('adminPassword').required = false;
      document.getElementById('adminModalTitle').textContent = 'แก้ไขเจ้าหน้าที่ (' + user + ')';
      document.getElementById('adminModal').classList.add('active');
    }
    function closeAdminModal() { document.getElementById('adminModal').classList.remove('active'); }

    /* ปุ่มที่ถูกสร้างจากฝั่งเซิร์ฟเวอร์ ใช้ data-attribute แทนการฝังค่าลงใน onclick */
    function bindDelegatedActions() {
      document.addEventListener('click', function (ev) {
        var el = ev.target.closest ? ev.target.closest('button') : null;
        if (!el) return;

        if (el.classList.contains('js-archive-delete')) {
          var file = el.getAttribute('data-file');
          if (!confirm('ต้องการลบไฟล์ประวัติ ' + file + ' ใช่หรือไม่?')) return;
          api('/api/archive/delete', { file: file }).then(function (d) {
            if (d.ok) setTimeout(function () { window.location.reload(); }, 700);
          }).catch(function () { });
        } else if (el.classList.contains('js-temp-revoke')) {
          removeTempCard(el.getAttribute('data-id'));
        } else if (el.classList.contains('js-admin-edit')) {
          openEditAdminModal(el.getAttribute('data-user'), el.getAttribute('data-name'));
        } else if (el.classList.contains('js-admin-delete')) {
          var user = el.getAttribute('data-user');
          if (!confirm('ยืนยันลบเจ้าหน้าที่ ' + user + ' ใช่หรือไม่?')) return;
          api('/api/admin/delete', { user: user }).then(function (d) {
            if (d.ok) setTimeout(function () { window.location.reload(); }, 700);
          }).catch(function () { });
        }
      });

      /* ปิด modal ด้วยปุ่ม Esc และการคลิกพื้นหลัง */
      document.addEventListener('keydown', function (ev) {
        if (ev.key === 'Escape') {
          document.querySelectorAll('.modal.active').forEach(function (m) { m.classList.remove('active'); });
        }
      });
      document.querySelectorAll('.modal').forEach(function (m) {
        m.addEventListener('mousedown', function (ev) { if (ev.target === m) m.classList.remove('active'); });
      });
      var uidInput = document.getElementById('assignTempUid');
      if (uidInput) {
        uidInput.addEventListener('keydown', function (ev) { if (ev.key === 'Enter') confirmAssignTemp(); });
      }
    }

    initTheme();
    bindAjaxForms();
    bindDelegatedActions();
    refreshDashboard();
    dashboardTimer = setInterval(refreshDashboard, 3000);

    try {
      var savedTab = localStorage.getItem('canteen_tab');
      if (savedTab && document.getElementById('tab-' + savedTab)) switchTab(savedTab);
    } catch (e) { }
  </script>
</body>
</html>)rawliteral";
  return html;
}

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

  WiFi.mode(WIFI_AP);
  WiFi.softAP(default_ap_ssid, default_ap_pass, ESPNOW_CHANNEL, 0, 4);
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

  server.on("/generate_204", HTTP_GET, []() { server.sendHeader("Location", "/login", true); server.send(302, "text/plain", ""); });
  server.on("/hotspot-detect.html", HTTP_GET, []() { server.sendHeader("Location", "/login", true); server.send(302, "text/plain", ""); });
  server.onNotFound([]() { server.sendHeader("Location", "/login", true); server.send(302, "text/plain", ""); });

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
      // ส่งธีมครั้งเดียวตอนสถานีเพิ่งออนไลน์ ถ้าส่งทุก heartbeat จะไปทับ
      // การสลับธีมที่ผู้ใช้กดเองที่หน้าเครื่องสถานีภายในไม่กี่วินาที
      if (!stationThemeSent[i]) {
        sendStationTheme(i + 1);
        stationThemeSent[i] = true;
      }
    }
  }

  if (isLiveScanDisplaying && (long)(millis() - liveScanHoldUntil) > 0) {
    isLiveScanDisplaying = false;
    renderHostPage(true);
  }

  if (!isLiveScanDisplaying && !isScreensaverActive && !isCreditActive && (millis() - lastActivity >= TIMEOUT_SCREENSAVER)) {
    isScreensaverActive = true; 
    renderScreensaver(true);
  }

  if (!isLiveScanDisplaying && (isScreensaverActive || currentHostPage == 0 || currentHostPage == 1) && !isCreditActive && (millis() - lastClockRefresh >= 1000)) {
    lastClockRefresh = millis();
    if (isScreensaverActive) renderScreensaver(false);
    else renderHostPage(false);
  }

  if (millis() - lastMonitorCheck > 2500) {
    lastMonitorCheck = millis();
    for (int i = 0; i < 4; i++) {
      if (stationNodes[i].isOnline && (millis() - stationNodes[i].lastSeen > STATION_OFFLINE_TIMEOUT)) {
        stationNodes[i].isOnline = false;
        stationThemeSent[i] = false;
      }
    }
    if (!isLiveScanDisplaying && !isScreensaverActive && !isCreditActive && currentHostPage == 1) {
      renderHostPage(false);
    }
  }

  delay(2);
}