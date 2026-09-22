/**
 * @file      StationScreen.h
 * @brief     ทุกอย่างที่วาดลงจอ TFT ของเครื่องประจำร้านค้า
 * @version   122.1.0
 * @date      2026-09-22
 * @author    Kittiphan Rattanakorn <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * มหาวิทยาลัยมหาจุฬาลงกรณราชวิทยาลัย วิทยาเขตแพร่
 *
 * @par Description
 * แยกออกมาจาก Canteen_Station_Client.ino ด้วยเหตุผลเดียวกับฝั่งแม่ข่าย
 * คือให้ไฟล์หลักสั้นและอ่านง่าย
 *
 * มีสามหน้าหลักสลับด้วยปุ่มกด หน้าแรกรอแตะบัตร หน้าสองยอดวันนี้ หน้าสามข้อมูลเครื่อง
 * พร้อมหน้าพักจอ หน้าแจ้งผลการแตะบัตร หน้าเตือนเมื่อขาดการเชื่อมต่อ
 * และหน้าตั้งหมายเลขจุดบริการ
 *
 * ข้อความบนจอเป็นภาษาอังกฤษอย่างเดียว เพราะฟอนต์ในตัวไลบรารี Adafruit GFX
 * ไม่มีตัวอักษรไทย
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 122.1.0 | 2026-09-22 | แยกออกมาจากไฟล์หลัก ย้ายแบบยกก้อน ไม่แก้เนื้อใน |
 * | 122.0.0 | 2026-09-22 | ออกแบบหน้าจอใหม่ให้เรียบง่าย ตัวอักษรน้อย แบ่งช่องชัดเจน และเติมค่าที่หายไปในหน้าตรวจสอบระบบ |
 * | 117.0.7 | 2026-09-21 | ต้นฉบับที่ใช้เป็นจุดเริ่ม เก็บสำเนาไว้ที่ original/ |
 *
 * @warning  ถูก #include ท้ายไฟล์หลักก่อน setup() ห้ามย้ายขึ้นไปบนสุด
 * @warning  ค่าปริยายของพารามิเตอร์ต้องอยู่ที่การประกาศล่วงหน้าในไฟล์หลักเท่านั้น
 *           ใส่ซ้ำที่นิยามในไฟล์นี้ไม่ได้ ตรวจด้วย tools/proto-check/splitcheck.py
 */

#pragma once


// ============================================================================
// ชิ้นส่วนพื้นฐาน: การ์ด ป้าย แถบบน แถบล่าง และการย่อตัวอักษรให้พอดีกรอบ
// ============================================================================

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

void drawStationBatteryHUD(int x, int y) {
  float volt = readBatteryVoltage();
  int pct = getBatteryPercentage(volt);

  tft.fillRect(x, y, 52, 18, getStBg());
  tft.drawRect(x, y + 2, 44, 14, getStTextMain());
  tft.fillRect(x + 44, y + 6, 3, 6, getStTextMain());

  int fillWidth = (pct * 40) / 100;
  if (fillWidth < 1 && pct > 0) fillWidth = 1;

  uint16_t fillColor = (pct > 20) ? getStGreen() : getStRose();
  if (volt > 4.05f) fillColor = getStCyan();

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
  tft.println(title);
  drawStationBatteryHUD(260, 3);
}

void drawStationBottomBar(String instruction) {
  tft.fillRect(0, 218, 320, 22, getStBg());
  tft.drawFastHLine(0, 218, 320, getStCardBorder());
  tft.setTextColor(getStTextMuted(), getStBg());
  tft.setTextSize(1);
  tft.setCursor(12, 224);
  tft.println(instruction);
}

// [122.0.0] เพิ่ม: ย่อขนาดตัวอักษรอัตโนมัติให้พอดีกรอบ แทนการปล่อยให้ล้นขอบจอ
uint8_t fitTextSize(const char* text, int maxWidth, uint8_t maxSize) {
  int len = strlen(text);
  if (len == 0) return maxSize;
  for (uint8_t sz = maxSize; sz >= 1; sz--) {
    if (len * 6 * sz <= maxWidth) return sz;
    if (sz == 1) break;
  }
  return 1;
}

void drawFitCenteredText(int x, int y, int w, int h, const char* text,
                         uint8_t maxSize, uint16_t fg, uint16_t bg) {
  uint8_t sz = fitTextSize(text, w - 8, maxSize);
  int tw = strlen(text) * 6 * sz;
  int th = 8 * sz;
  tft.setTextSize(sz);
  tft.setTextColor(fg, bg);
  tft.setCursor(x + (w - tw) / 2, y + (h - th) / 2);
  tft.print(text);
}


// ============================================================================
// ความแรงสัญญาณที่มุมขวาบน
// ============================================================================

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

// ค่าปริยายของ forceRedraw อยู่ที่การประกาศล่วงหน้าในไฟล์หลัก
// เขียนซ้ำตรงนี้อีกครั้งไม่ได้ ภาษา C++ อนุญาตให้ระบุได้ครั้งเดียว
void updateTopRightHeaderSmooth(int rssi, bool online, bool forceRedraw) {
  static int lastDrawnRssi = -999;
  static bool lastDrawnOnline = false;
  static int lastBars = -1;

  int currentBars = 0;
  int q = calculateSignalQuality(rssi);
  if (online) {
    if (q >= 85) currentBars = 4;
    else if (q >= 60) currentBars = 3;
    else if (q >= 35) currentBars = 2;
    else currentBars = 1; // แก้ไขจุด activeBars ให้เป็น currentBars เรียบร้อยแล้ว
  }

  if (!forceRedraw && (online == lastDrawnOnline) && (currentBars == lastBars) && (abs(rssi - lastDrawnRssi) < 3)) {
    return;
  }

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

  drawStationBatteryHUD(260, 3);
}


// ============================================================================
// ไฟส่องหลังจอ
// ============================================================================

void setScreenPower(bool powerOn) {
  isScreenOn = powerOn;
  digitalWrite(TFT_BLK, isScreenOn ? HIGH : LOW);
}

void wakeScreenIfNeeded() {
  if (!isScreenOn) setScreenPower(true);
}


// ============================================================================
// หน้าจอเต็มทั้งหมด
// ============================================================================

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
  tft.println("MCU CANTEEN");
  
  tft.setTextColor(getStTextMain(), getStBg());
  tft.setTextSize(1);
  tft.setCursor(44, 60);
  tft.println("Service point");

  drawStationCard(50, 95, 220, 48, getStGreen(), getStCardBg());
  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(2);
  tft.setCursor(85, 110);
  tft.printf("Point %d", currentStationId);

  tft.setTextColor(getStGreen(), getStBg());
  tft.setTextSize(1);
  tft.setCursor(68, 175);
  tft.println("ESP-NOW Radio: Locked CH 1 (Stable)");

  soundWelcome();
  ledStandby();
  delay(300);
}

// [122.0.0] แก้: รวมสามบล็อกที่เขียนซ้ำกันไว้ที่เดียว และตั้ง currentState ให้ถูก
void showStationPage(int page, bool fullRedraw) {
  if (page < 1 || page > 3) page = 1;
  currentStationPage = page;
  currentState = (page == 3) ? STATE_STATUS : STATE_STANDBY;
  if (page == 1)      displayTapCardStandby();
  else if (page == 2) displayStatsDashboard();
  else                displayStatusScreen(fullRedraw);
}

// [122.0.0] แก้: หน้าที่นิสิตเห็นมากที่สุด จึงเหลือประโยคเดียวที่ต้องอ่าน
void displayTapCardStandby() {
  ledStandby();
  tft.fillScreen(getStBg());

  drawStationTopBar(String(dynamicShopLabel));

  // ช่องใหญ่ช่องเดียว มีประโยคเดียวที่นิสิตต้องอ่าน
  drawStationCard(16, 34, 288, 130, getStCardBorder(), getStCardBg());
  drawFitCenteredText(24, 62, 272, 40, "TAP YOUR CARD", 4, getStTextMain(), getStCardBg());
  drawFitCenteredText(24, 112, 272, 16, "Free meal  35 baht  once a day", 1,
                      getStTextMuted(), getStCardBg());

  // ช่องล่าง: ยอดของจุดบริการนี้วันนี้ และเวลา
  drawStationCard(16, 172, 288, 40, getStCardBorder(), getStCardBg());
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(28, 178);
  tft.print("SERVED TODAY");
  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(2);
  tft.setCursor(28, 191);
  tft.printf("%d", totalSuccessToday);

  String clock = getTimeOnlyStr();
  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(2);
  tft.setCursor(292 - (int)clock.length() * 12, 186);
  tft.print(clock);

  drawStationBottomBar("Page 1/3    Press the button for the next page");
}

void displayStatsDashboard() {
  ledStandby();
  tft.fillScreen(getStBg());

  drawStationTopBar(String(dynamicShopLabel));

  // ซ้าย: จ่ายไปแล้วกี่จาน เป็นเงินเท่าไร
  drawStationCard(6, 30, 150, 172, getStCardBorder(), getStCardBg());
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(14, 40);
  tft.print("SERVED TODAY");

  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(4);
  tft.setCursor(14, 62);
  tft.printf("%d", totalSuccessToday);

  tft.setTextColor(getStGreen(), getStCardBg());
  tft.setTextSize(3);
  tft.setCursor(14, 118);
  tft.printf("%d", totalSuccessToday * 35);
  tft.setTextSize(1);
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setCursor(14 + (int)String(totalSuccessToday * 35).length() * 18 + 6, 134);
  tft.print("baht");

  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(14, 174);
  tft.print("35 baht per student");

  // ขวา: จุดบริการนี้คือจุดไหน ต่อกับแม่ข่ายอยู่ไหม และเวลาเท่าไร
  drawStationCard(164, 30, 150, 172, getStCardBorder(), getStCardBg());
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(172, 40);
  tft.print("THIS POINT");

  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(4);
  tft.setCursor(172, 62);
  tft.printf("%d", currentStationId);

  drawStationPillBadge(172, 118, 96, 20, isHostOnline ? "ONLINE" : "OFFLINE",
                       isHostOnline ? (isStationDarkMode ? 0x0000 : 0xFFFF) : 0xFFFF,
                       isHostOnline ? getStGreen() : getStRose());

  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(2);
  tft.setCursor(172, 166);
  tft.print(getTimeOnlyStr());

  drawStationBottomBar("Page 2/3    Press the button for the next page");
}

void displayStatusScreen(bool fullRedraw) {
  if (fullRedraw) {
    ledStandby();
    tft.fillScreen(getStBg());
    drawStationTopBar("SYSTEM");

    drawStationCard(6, 30, 308, 172, getStCardBorder(), getStCardBg());

    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.setTextSize(1);
    tft.setCursor(16, 42);  tft.print("Host link");
    tft.setCursor(16, 64);  tft.print("Service point");
    tft.setCursor(16, 86);  tft.print("Chip");
    tft.setCursor(16, 108); tft.print("Battery");
    tft.setCursor(16, 130); tft.print("Served today");
    tft.setCursor(16, 152); tft.print("Address");

    tft.setTextColor(getStTextMain(), getStCardBg());
    tft.setCursor(140, 64);  tft.printf("Point %d", currentStationId);
    tft.setCursor(140, 152); tft.print(WiFi.macAddress());

    drawStationBottomBar("Page 3/3    Press the button for the next page");
  }

  float chipT = getChipTemperature();
  float cpuL = calculateCpuLoad();

  bool updateDynamic = (millis() - lastCpuDisplayUpdate >= 500) ||
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
      tft.printf("Connected  %d dB", lastHostRssi);
    } else {
      tft.setTextColor(getStRose(), getStCardBg());
      tft.print("Not connected");
    }

    tft.fillRect(140, 84, 166, 14, getStCardBg());
    tft.setCursor(140, 86);
    tft.setTextColor((chipT < 65.0f) ? getStTextMain() : getStYellow(), getStCardBg());
    tft.printf("%.0f C   cpu %.0f%%", chipT, cpuL);

    // สองแถวนี้เดิมมีแต่หัวข้อ ไม่เคยมีค่าโผล่มาเลย เติมให้ครบ
    float volt = readBatteryVoltage();
    tft.fillRect(140, 106, 166, 14, getStCardBg());
    tft.setCursor(140, 108);
    tft.setTextColor(getStTextMain(), getStCardBg());
    tft.printf("%d%%   %.2f V", getBatteryPercentage(volt), volt);

    tft.fillRect(140, 128, 166, 14, getStCardBg());
    tft.setCursor(140, 130);
    tft.printf("%d meals   %d baht", totalSuccessToday, totalSuccessToday * 35);
  }
}

void renderScreensaver(bool fullRedraw) {
  int usedCount = totalSuccessToday;
  static String lastStationClock = "";

  if (fullRedraw) {
    ledOff();
    tft.fillScreen(getStBg());
    lastStationClock = "";
    drawStationTopBar(String(dynamicShopLabel));

    drawStationCard(20, 36, 280, 162, getStCardBorder(), getStCardBg());

    String dateStr = getDateFormattedStr();
    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.setTextSize(1);
    tft.setCursor(160 - (int)dateStr.length() * 3, 118);
    tft.print(dateStr);

    char line[48];
    snprintf(line, sizeof(line), "%d served today   %d baht", usedCount, usedCount * 35);
    tft.setTextColor(getStGreen(), getStCardBg());
    tft.setTextSize(1);
    tft.setCursor(160 - (int)strlen(line) * 3, 146);
    tft.print(line);

    float volt = readBatteryVoltage();
    char line2[48];
    snprintf(line2, sizeof(line2), "battery %d%%", getBatteryPercentage(volt));
    tft.setTextColor(getStTextMuted(), getStCardBg());
    tft.setCursor(160 - (int)strlen(line2) * 3, 168);
    tft.print(line2);

    drawStationBottomBar("Tap your card or press the button to wake");
  }

  String curTime = getTimeOnlyStr();
  if (fullRedraw || curTime != lastStationClock) {
    lastStationClock = curTime;
    tft.fillRect(30, 54, 260, 52, getStCardBg());
    tft.setTextColor(getStTextMain(), getStCardBg());
    tft.setTextSize(5);
    tft.setCursor(160 - (int)curTime.length() * 15, 62);
    tft.print(curTime);
  }
}

void displayScanningUID(String uid) {
  wakeScreenIfNeeded();
  soundClick();
  setLedColor(30, 30, 30);
  tft.fillScreen(getStBg());

  drawStationTopBar("CHECKING");

  drawStationCard(10, 36, 300, 168, getStCyan(), getStCardBg());
  drawStationPillBadge(24, 48, 272, 22, "CARD READ", isStationDarkMode ? 0x0000 : 0xFFFF, getStCyan());

  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(24, 84);
  tft.println("CARD");

  tft.setTextColor(getStCyan(), getStCardBg());
  tft.setTextSize(2);
  tft.setCursor(24, 104);
  tft.println(maskUID(uid));

  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(24, 148);
  tft.println("Asking the main computer...");

  drawStationBottomBar("Please wait a moment");
}

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

  if (status == "SUCCESS") {
    totalSuccessToday++;
    ledApproved();
    soundSuccess();
    screenBg    = 0x02E5;             // สีพื้นหลังเฉดเขียวเข้ม
    cardBg      = 0x01C3;             // สีพื้นการ์ดโทนเขียวมรกตลึก
    bannerBg    = DARK_ACCENT_GREEN;  // แบนเนอร์สีเขียวนีออน (0x07E0)
    bannerFg    = 0x0000;             // ตัวอักษรสีดำ
    accentColor = DARK_ACCENT_GREEN;  
    textMuted   = 0x87F0;             // ข้อความกำกับสีเขียวมิ้นต์
    headerTitle = "APPROVED";
    footerDesc  = "Meal paid, 35 baht. Enjoy your meal.";
  } 
  else if (status == "ALREADY_USED") {
    ledDuplicate();
    soundAlarm();
    screenBg    = 0x8200;             // สีพื้นหลังเฉดส้มอิฐเข้ม
    cardBg      = 0x4900;             // สีพื้นการ์ดโทนส้มเข้ม
    bannerBg    = ST77XX_ORANGE;      // แบนเนอร์สีส้มสด (0xFD20)
    bannerFg    = 0x0000;             // ตัวอักษรสีดำ
    accentColor = ST77XX_ORANGE;
    textMuted   = 0xFDC0;             // ข้อความกำกับสีส้มอ่อน
    headerTitle = "ALREADY SERVED";
    footerDesc  = "You already had your meal today.";
  } 
  else if (status == "TIME_CLOSED") {
    ledDuplicate();
    soundError();
    screenBg    = 0x6200;             // พื้นหลังโทนส้มอมน้ำตาล
    cardBg      = 0x3900;             // พื้นหลังการ์ดโทนน้ำตาลเข้ม
    bannerBg    = DARK_ACCENT_YELLOW; // แถบแบนเนอร์สีเหลืองเตือนภัย (0xFFE0 ตามไฟล์คาลิเบท)
    bannerFg    = 0x0000;             
    accentColor = DARK_ACCENT_YELLOW; // ขอบการ์ดและเส้นคั่นสีเหลืองเตือนภัย (0xFFE0)
    textMuted   = 0xFEE0;             // ข้อความกำกับสีเหลืองอ่อน
    headerTitle = "CLOSED NOW";
    footerDesc  = "Please come back during service hours.";
  } 
  else {
    ledRejected();
    soundError();
    screenBg    = 0x8000;             // สีพื้นหลังเฉดแดงทึบเข้ม
    cardBg      = 0x4800;             // สีพื้นการ์ดโทนแดงเข้ม
    bannerBg    = DARK_ACCENT_ROSE;   // แบนเนอร์สีแดงสด (0xF800 ตามไฟล์คาลิเบท)
    bannerFg    = 0xFFFF;             // ตัวอักษรสีขาว
    accentColor = DARK_ACCENT_ROSE;   
    textMuted   = 0xFCAE;             // ข้อความกำกับสีชมพูอ่อน
    headerTitle = "NOT ON THE LIST";
    footerDesc  = "Ask the staff to register this card.";
  }

  tft.fillScreen(screenBg);

  // แบนเนอร์คำเดียว ตัวโต นิสิตอ่านออกตั้งแต่ยังไม่เก็บบัตร
  tft.fillRect(0, 0, 320, 40, bannerBg);
  drawFitCenteredText(4, 0, 312, 40, headerTitle, 3, bannerFg, bannerBg);

  tft.fillRoundRect(8, 46, 304, 140, 8, cardBg);
  tft.drawRoundRect(8, 46, 304, 140, 8, accentColor);

  // ชื่อมาก่อน เพราะนิสิตจำชื่อตัวเองได้เร็วกว่ารหัส
  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 56);
  tft.print("NAME");

  String displayName = (name != "-" && name.length() > 0) ? name : "Unknown card";
  if (displayName.length() > 22) displayName = displayName.substring(0, 22);
  tft.setTextSize(2);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 68);
  tft.print(displayName);

  tft.drawFastHLine(20, 94, 280, accentColor);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 102);
  tft.print("STUDENT ID");

  String cleanId = (id != "-" && id.length() > 0) ? id : "Unknown";
  tft.setTextSize(3);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 114);
  tft.print(cleanId);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 150);
  tft.printf("Point %d", currentStationId);
  tft.setCursor(20, 164);
  tft.printf("Card %s", maskUID(lastProcessedUID).c_str());

  // เวลาที่ได้รับสิทธิ์ อยู่มุมขวาของการ์ด เผื่อเจ้าหน้าที่ต้องตรวจย้อนหลัง
  if (claimTime.length() > 0 && claimTime != "-") {
    String stamp = claimTime;
    if (stamp.length() > 16) stamp = stamp.substring(stamp.length() - 8);
    tft.setTextColor(textMuted, cardBg);
    tft.setCursor(296 - (int)stamp.length() * 6, 164);
    tft.print(stamp);
  }

  tft.fillRoundRect(8, 192, 304, 40, 8, bannerBg);
  drawFitCenteredText(14, 192, 292, 40, footerDesc, 1, bannerFg, bannerBg);
}

void displayOfflineAlert() {
  ledOffline();
  tft.fillScreen(0x8000);

  drawStationCard(10, 16, 300, 208, 0xF800, 0x4800);
  drawStationPillBadge(24, 28, 272, 24, "NO CONNECTION", 0xFFFF, 0xF800);

  tft.setTextColor(0xFCAE, 0x4800);
  tft.setTextSize(1);
  tft.setCursor(24, 68);
  tft.println("WHAT HAPPENED");
  tft.setTextColor(0xF800, 0x4800);
  tft.setTextSize(2);
  tft.setCursor(24, 84);
  tft.println("Main computer");
  tft.setCursor(24, 104);
  tft.println("is not answering");

  tft.setTextColor(0xFCAE, 0x4800);
  tft.setTextSize(1);
  tft.setCursor(24, 124);
  tft.println("WHAT TO DO");
  tft.setTextColor(0xFFFF, 0x4800);
  tft.setTextSize(1);
  tft.setCursor(24, 142);
  tft.println("Tell the staff, then press the button");

  drawStationBottomBar("Press the button to try again");
  soundError();
}

// [122.0.0] เพิ่ม: แจ้งว่าการแสดงผลถูกกำหนดจากแม่ข่าย แทนการสลับเองที่เครื่อง
void showDisplayLockedNotice() {
  tft.fillRect(0, 100, 320, 44, getStCyan());
  drawFitCenteredText(0, 100, 320, 20, "Display is set by the main computer", 1,
                      isStationDarkMode ? 0x0000 : 0xFFFF, getStCyan());
  drawFitCenteredText(0, 120, 320, 20, "Change it on the web page", 1,
                      isStationDarkMode ? 0x0000 : 0xFFFF, getStCyan());
  soundClick();
  delay(1400);
  showStationPage(currentStationPage, true);
}

void renderDeveloperCredit() {
  tft.fillScreen(getStBg());
  drawStationTopBar("ABOUT");

  drawStationCard(10, 36, 300, 166, getStCyan(), getStCardBg());

  tft.setTextColor(getStCyan(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(22, 48);
  tft.println("BUILT BY");

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
  tft.setCursor(22, 134); tft.printf("Firmware  v%s\n", APP_VERSION);
  tft.setCursor(22, 150); tft.printf("Hardware: ESP32-S3 + ST7789V + RC522\n");
  tft.setCursor(22, 166); tft.println("Display : 2.8\" ST7789V 320x240 Modular Bento");

  drawStationBottomBar("Press the button to go back");
}

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
    else tft.print("Release to change the point number");
  } else {
    if (remain > 0) tft.printf("AUTO SAVE IN %lu SEC", sec);
    else tft.print("SAVING...");
  }
}
