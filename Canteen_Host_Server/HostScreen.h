/**
 * @file      HostScreen.h
 * @brief     ทุกอย่างที่วาดลงจอ TFT ของเครื่องแม่ข่าย
 * @version   113.0.0
 * @date      2026-09-22
 * @author    Kittiphan Rattanakorn <varitwuttikul@gmail.com>
 *
 * @par Organization
 * มหาวิทยาลัยมหาจุฬาลงกรณราชวิทยาลัย วิทยาเขตแพร่
 *
 * @par Description
 * แยกออกมาจาก Canteen_Host_Server.ino เพื่อให้ไฟล์หลักสั้นและอ่านง่าย
 * นิสิตที่มารับช่วงดูแลต่อจะได้หาของเจอเร็วขึ้น
 *
 * มีสามหน้าหลักสลับด้วยปุ่มกด TODAY / SHOPS / SYSTEM
 * พร้อมหน้าพักจอ หน้าแจ้งผลการแตะบัตร หน้าแจ้งสถานะ Wi-Fi ตอนบูต และหน้าเครดิต
 *
 * ข้อความบนจอเป็นภาษาอังกฤษอย่างเดียว เพราะฟอนต์ในตัวไลบรารี Adafruit GFX
 * ไม่มีตัวอักษรไทย ส่วนหน้าเว็บเป็นสองภาษา
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 113.0.0 | 2026-09-22 | แยกออกมาจากไฟล์หลัก แล้วออกแบบหน้าจอใหม่ให้เรียบง่าย ตัวอักษรน้อย แบ่งช่องชัดเจน และรองรับสองโหมดสี |
 * | 107.0.1 | 2026-09-21 | ต้นฉบับที่ใช้เป็นจุดเริ่ม เก็บสำเนาไว้ที่ original/ |
 *
 * @warning  ถูก #include ท้ายไฟล์หลักก่อน setup() ห้ามย้ายขึ้นไปบนสุด
 * @note     Arduino IDE แสดงไฟล์นี้เป็นแท็บของสเก็ตช์เดียวกัน เปิดคู่กันได้เลย
 */

#pragma once

// --- ชิ้นส่วนพื้นฐาน: การ์ด ป้าย แถบบน แถบล่าง ไอคอนแบตเตอรี่ ---
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

void drawHostTopBar(String title, int pageNo) {
  tft.fillRect(0, 0, 320, 26, getTftBg());
  tft.drawFastHLine(0, 26, 320, getTftCardBorder());
  tft.setTextColor(getTftTextMain(), getTftBg());
  tft.setTextSize(2);
  tft.setCursor(8, 6);
  tft.print(title);
  if (pageNo > 0) {
    char tag[8];
    snprintf(tag, sizeof(tag), "%d/%d", pageNo, TOTAL_PAGES);
    tft.setTextSize(1);
    tft.setTextColor(getTftTextMuted(), getTftBg());
    tft.setCursor(222, 9);
    tft.print(tag);
  }
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

// --- แอนิเมชันตอนบูต ---
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
  tft.println("MCU CANTEEN");
  
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(44, 60);
  tft.println("Meal subsidy system");

  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setCursor(68, 76);
  tft.printf("ESP32-S3 N16R8 | PSRAM: %d MB\n", ESP.getPsramSize() / 1024 / 1024);

  int barX = 40; int barY = 120; int barW = 240; int barH = 16;
  tft.drawRoundRect(barX - 2, barY - 2, barW + 4, barH + 4, 4, ST77XX_WHITE);
  const char* loadSteps[] = {
    "Opening storage...",
    "Loading student list...",
    "Checking the clock...",
    "Starting Wi-Fi...",
    "Ready"
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

// หน้าบอกสถานะเครือข่ายตอนบูต ค้างไว้สามวินาที
// มีไว้เพราะเคยเจอปัญหาชื่อ Wi-Fi ไม่โผล่มา แล้วไม่มีอะไรบนเครื่องบอกเลยว่าเกิดอะไรขึ้น
// [113.0.0] เพิ่ม: บอกสถานะ Wi-Fi ตอนบูต ค้างไว้สามวินาที
//           เคยเจอปัญหาชื่อ Wi-Fi ไม่โผล่มา แล้วไม่มีอะไรบนเครื่องบอกว่าเกิดอะไรขึ้น
void showBootNetworkStatus() {
  tft.fillScreen(getTftBg());
  drawHostTopBar("NETWORK");

  uint16_t tone = hostApReady ? getTftAccentGreen() : getTftAccentRose();
  tft.fillRect(0, 30, 320, 34, tone);
  const char* head = hostApReady ? "WI-FI IS ON" : "WI-FI FAILED TO START";
  tft.setTextSize(2);
  tft.setTextColor(isTftDarkMode ? 0x0000 : BENTO_WHITE, tone);
  tft.setCursor(max(4, (320 - (int)strlen(head) * 12) / 2), 39);
  tft.print(head);

  drawBentoCard(6, 74, 308, 134, getTftCardBorder(), getTftCardBg());
  tft.setTextSize(1);
  if (hostApReady) {
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.setCursor(20, 92);  tft.print("Wi-Fi name");
    tft.setCursor(20, 122); tft.print("Password");
    tft.setCursor(20, 152); tft.print("Web page");
    tft.setTextColor(getTftTextMain(), getTftCardBg());
    tft.setTextSize(2);
    tft.setCursor(20, 102); tft.print(default_ap_ssid);
    tft.setCursor(20, 132); tft.print(default_ap_pass);
    tft.setCursor(20, 162); tft.print(WiFi.softAPIP().toString());
    tft.setTextSize(1);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.setCursor(20, 186); tft.printf("or http://%s.local", mdns_hostname);
  } else {
    tft.setTextColor(getTftTextMain(), getTftCardBg());
    tft.setTextSize(2);
    tft.setCursor(20, 100); tft.print("Turn the power off");
    tft.setCursor(20, 124); tft.print("and on again");
    tft.setTextSize(1);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.setCursor(20, 162); tft.print("If it keeps happening, flash the board again.");
    tft.setCursor(20, 176); tft.print("Card reading still works without Wi-Fi.");
    soundBeep();
  }

  drawBentoBottomBar(hostApReady ? "Starting up..." : "Wi-Fi is off, the rest still works");
  delay(3000);
}

// --- ชิ้นส่วนเพิ่มเติมสำหรับหน้าจอแบบใหม่ ---

// [113.0.0] เพิ่ม: ชุดชิ้นส่วนสำหรับหน้าจอแบบใหม่
// ตัดข้อความให้พอดีความกว้างที่ให้มา (ฟอนต์ในตัวกว้างตัวละ 6 พิกเซลต่อขนาด 1)
String fitLabel(const String &raw, int maxChars) {
  if ((int)raw.length() <= maxChars) return raw;
  if (maxChars <= 1) return raw.substring(0, maxChars);
  return raw.substring(0, maxChars - 1) + ".";
}

// หัวข้อเล็ก ๆ มุมบนซ้ายของการ์ด บอกว่าการ์ดนี้คืออะไร
void drawCardLabel(int x, int y, const char* label) {
  tft.setTextColor(getTftTextMuted(), getTftCardBg());
  tft.setTextSize(1);
  tft.setCursor(x, y);
  tft.print(label);
}

// จุดกลมบอกสถานะ ใช้แทนคำว่า ONLINE/OFFLINE ที่ยาวเกินไป
void drawStatusDot(int cx, int cy, bool good) {
  tft.fillCircle(cx, cy, 4, good ? getTftAccentGreen() : getTftAccentRose());
}

// แถบความคืบหน้า ใช้บอกว่าวันนี้จ่ายไปแล้วกี่ส่วนของทั้งหมด
void drawProgressBar(int x, int y, int w, int h, int pct) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  tft.fillRoundRect(x, y, w, h, h / 2, getTftBg());
  int fillW = (w * pct) / 100;
  if (fillW < 2 && pct > 0) fillW = 2;
  if (fillW > 0) tft.fillRoundRect(x, y, fillW, h, h / 2, getTftAccentGreen());
}

// --- หน้าจอเต็ม: สามหน้าหลัก โหมดพักจอ ผลการสแกน และหน้าเครดิต ---
// ออกแบบใหม่ให้คนที่ไม่ใช่ช่างอ่านออกในสายตาเดียว
// หน้า 1 ยอดวันนี้ | หน้า 2 ร้านค้าและจุดบริการ | หน้า 3 ข้อมูลเครื่อง
// [113.0.0] แก้: ออกแบบสามหน้าหลักใหม่ให้คนที่ไม่ใช่ช่างอ่านออกในสายตาเดียว
//           หน้า 1 ยอดวันนี้ | หน้า 2 ร้านค้าและจุดบริการ | หน้า 3 ข้อมูลเครื่อง
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

  // ==========================================================================
  // หน้า 1 — ยอดวันนี้ สามช่องใหญ่ ตัวเลขโต ๆ อ่านจากไกลได้
  // ==========================================================================
  if (currentHostPage == 0) {
    if (fullRedraw) {
      drawHostTopBar("TODAY", 1);
      drawBentoCard(6, 32, 150, 88, getTftCardBorder(), getTftCardBg());
      drawCardLabel(14, 40, "MEALS SERVED");
      drawBentoCard(164, 32, 150, 88, getTftCardBorder(), getTftCardBg());
      drawCardLabel(172, 40, "BAHT PAID");
      drawBentoCard(6, 126, 308, 82, getTftCardBorder(), getTftCardBg());
      drawCardLabel(14, 134, "SERVICE");
      drawBentoBottomBar("Click: next page    Double click: sleep");
    }

    // ช่องซ้าย: จำนวนจานที่จ่ายไปแล้ว พร้อมแถบความคืบหน้า
    tft.fillRect(12, 54, 140, 60, getTftCardBg());
    tft.setTextColor(getTftTextMain(), getTftCardBg());
    tft.setTextSize(4);
    tft.setCursor(14, 56);
    tft.printf("%d", usedCount);
    tft.setTextSize(1);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.setCursor(14, 92);
    tft.printf("of %d students", (int)db.size());
    int pct = (db.size() > 0) ? (usedCount * 100) / (int)db.size() : 0;
    drawProgressBar(14, 106, 134, 6, pct);

    // ช่องขวา: เป็นเงินเท่าไร
    tft.fillRect(170, 54, 140, 60, getTftCardBg());
    tft.setTextColor(getTftAccentGreen(), getTftCardBg());
    tft.setTextSize(4);
    tft.setCursor(172, 56);
    tft.printf("%d", usedCount * 35);
    tft.setTextSize(1);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.setCursor(172, 92);
    tft.print("35 baht per student");

    // ช่องล่าง: นาฬิกา วันที่ และเปิด/ปิดบริการ
    tft.fillRect(12, 148, 296, 56, getTftCardBg());
    tft.setTextColor(getTftTextMain(), getTftCardBg());
    tft.setTextSize(4);
    tft.setCursor(14, 150);
    tft.print(getTimeOnlyStr());
    tft.setTextSize(1);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.setCursor(14, 186);
    tft.print(getDateFormattedStr());

    char hours[16];
    snprintf(hours, sizeof(hours), "%02d:%02d-%02d:%02d",
             serviceStartHour, serviceStartMin, serviceEndHour, serviceEndMin);
    bool isOpen = isWithinServiceTime();
    drawBentoPillBadge(214, 152, 94, 22, isOpen ? "OPEN" : "CLOSED",
                       isTftDarkMode ? 0x0000 : BENTO_WHITE,
                       isOpen ? getTftAccentGreen() : getTftAccentRose());
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.setTextSize(1);
    tft.setCursor(214 + (94 - (int)strlen(hours) * 6) / 2, 182);
    tft.print(hours);
  }
  // ==========================================================================
  // หน้า 2 — ร้านค้าและจุดบริการ สี่ช่อง เจ้าของร้านดูยอดของตัวเองได้ทันที
  // ==========================================================================
  else if (currentHostPage == 1) {
    if (fullRedraw) {
      drawHostTopBar("SHOPS", 2);
      drawBentoBottomBar("Click: next page    Double click: sleep");
    }

    int coords[4][2] = { {6, 32}, {164, 32}, {6, 122}, {164, 122} };
    static int waitDots = 0;
    if (!fullRedraw) waitDots = (waitDots + 1) % 4;

    for (int i = 0; i < 4; i++) {
      int x = coords[i][0], y = coords[i][1], w = 150, h = 86;

      if (fullRedraw) {
        drawBentoCard(x, y, w, h, getTftCardBorder(), getTftCardBg());

        // ชื่อร้านมาก่อน เพราะเจ้าของร้านมองหาชื่อตัวเอง ไม่ได้มองหาเลขจุดบริการ
        // เต็มความกว้างการ์ด ไม่มีอะไรมาวาดทับภายหลัง
        tft.setTextColor(getTftTextMain(), getTftCardBg());
        tft.setTextSize(1);
        tft.setCursor(x + 8, y + 7);
        tft.print(fitLabel(shops[i].name, 22));

        // ยอดของร้าน ตัวเลขจานใหญ่สุดในการ์ด อ่านได้จากอีกฝั่งของโรงอาหาร
        tft.setTextColor(getTftTextMain(), getTftCardBg());
        tft.setTextSize(3);
        tft.setCursor(x + 8, y + 36);
        tft.printf("%d", shopCounts[i]);

        int numW = String(shopCounts[i]).length() * 18;
        tft.setTextSize(1);
        tft.setTextColor(getTftTextMuted(), getTftCardBg());
        tft.setCursor(x + 14 + numW, y + 38);
        tft.print("meals");
        tft.setTextColor(getTftAccentGreen(), getTftCardBg());
        tft.setCursor(x + 14 + numW, y + 52);
        tft.printf("%d baht", shopCounts[i] * 35);
      }

      // สองแถวนี้วาดซ้ำทุกรอบ จึงล้างพื้นที่ของตัวเองก่อน และต้องไม่ทับชื่อร้าน
      tft.fillRect(x + 8, y + 20, 74, 12, getTftCardBg());
      drawStatusDot(x + 12, y + 25, stationNodes[i].isOnline);
      tft.setTextSize(1);
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setCursor(x + 22, y + 22);
      tft.printf("Point %d", i + 1);

      tft.fillRect(x + 8, y + 66, w - 16, 14, getTftCardBg());
      if (stationNodes[i].isOnline) {
        int sPct = getHostBatteryPercentage(stationNodes[i].systemVoltage);
        drawSignalBars(x + 8, y + 68, stationNodes[i].rssi, true, getTftCardBg());
        tft.setTextColor(getTftTextMuted(), getTftCardBg());
        tft.setCursor(x + 40, y + 70);
        tft.print("signal");
        tft.setCursor(x + 92, y + 70);
        tft.printf("%d%%", sPct);
        drawMiniBattery(x + 120, y + 68, sPct);
      } else {
        String wait = "Waiting";
        for (int d = 0; d <= waitDots; d++) wait += ".";
        tft.setTextColor(getTftAccentRose(), getTftCardBg());
        tft.setCursor(x + 8, y + 70);
        tft.print(wait);
      }
    }
  }
  // ==========================================================================
  // หน้า 3 — ข้อมูลเครื่อง สำหรับคนดูแลระบบ ไม่ใช่สำหรับเจ้าของร้าน
  // ==========================================================================
  else {
    if (fullRedraw) {
      drawHostTopBar("SYSTEM", 3);
      drawBentoCard(6, 32, 308, 84, getTftCardBorder(), getTftCardBg());
      drawCardLabel(14, 40, "LAST CARD TAP");
      drawBentoCard(6, 122, 150, 86, getTftCardBorder(), getTftCardBg());
      drawCardLabel(14, 130, "NETWORK");
      drawBentoCard(164, 122, 150, 86, getTftCardBorder(), getTftCardBg());
      drawCardLabel(172, 130, "DEVICE");

      tft.setTextColor(getTftTextMain(), getTftCardBg());
      tft.setTextSize(1);
      tft.setCursor(14, 148); tft.print("Wi-Fi  MCU_CANTEEN");
      tft.setCursor(14, 164); tft.print("Page   192.168.4.1");
      tft.setCursor(14, 180); tft.print("Radio  channel 1");
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setCursor(14, 192); tft.print("4 service points linked");

      drawBentoBottomBar("Click: next page    Double click: sleep");
    }

    // ผลการแตะบัตรครั้งล่าสุด
    tft.fillRect(12, 54, 296, 58, getTftCardBg());
    if (lastScannedUID != "-" && lastScannedUID.length() > 0) {
      tft.setTextColor(getTftTextMain(), getTftCardBg());
      tft.setTextSize(3);
      tft.setCursor(14, 56);
      tft.print(lastScannedStudentId != "-" ? lastScannedStudentId : String("Unknown card"));

      tft.setTextSize(1);
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setCursor(14, 86);
      tft.printf("Point %d   card %s", lastScannedStation, maskUID(lastScannedUID).c_str());

      const char* word; uint16_t tone;
      if (lastScannedStatus == "APPROVED")         { word = "SERVED";         tone = getTftAccentGreen(); }
      else if (lastScannedStatus == "DUPLICATE")   { word = "ALREADY SERVED"; tone = getTftAccentYellow(); }
      else if (lastScannedStatus == "TIME_CLOSED") { word = "CLOSED NOW";     tone = getTftAccentYellow(); }
      else                                         { word = "NOT ON LIST";    tone = getTftAccentRose(); }
      drawBentoPillBadge(14, 98, 130, 14, word,
                         isTftDarkMode ? 0x0000 : BENTO_WHITE, tone);

      // เลขอ้างอิงยาวกว่าที่การ์ดรับไหว จึงโชว์ท้ายเลข ซึ่งเป็นส่วนที่ไม่ซ้ำกัน
      String ref = lastScannedRef;
      if (ref.length() > 24) ref = "." + ref.substring(ref.length() - 23);
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setCursor(152, 101);
      tft.print(ref);
    } else {
      tft.setTextColor(getTftTextMuted(), getTftCardBg());
      tft.setTextSize(2);
      tft.setCursor(14, 72);
      tft.print("No card tapped yet");
    }

    // ตัวเลขของเครื่อง อัปเดตทุกรอบ
    tft.fillRect(170, 142, 138, 62, getTftCardBg());
    float cTemp = getChipTemperature();
    tft.setTextSize(1);
    tft.setTextColor(getTftTextMain(), getTftCardBg());
    tft.setCursor(172, 148); tft.printf("Students %d", (int)db.size());
    tft.setCursor(172, 164); tft.printf("Staff    %d of 3", (int)adminUsers.size());
    tft.setTextColor((cTemp < 65.0f) ? getTftTextMain() : getTftAccentYellow(), getTftCardBg());
    tft.setCursor(172, 180); tft.printf("Chip     %.0f C", cTemp);
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.setCursor(172, 192); tft.printf("Free memory %d KB", ESP.getFreeHeap() / 1024);
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
    drawHostTopBar("MCU CANTEEN");

    drawBentoCard(20, 40, 280, 156, getTftCardBorder(), getTftCardBg());

    // วันที่ อยู่ใต้นาฬิกา จัดกึ่งกลางการ์ด
    String dateStr = getDateFormattedStr();
    tft.setTextColor(getTftTextMuted(), getTftCardBg());
    tft.setTextSize(1);
    tft.setCursor(160 - (int)dateStr.length() * 3, 124);
    tft.print(dateStr);

    // สรุปยอดวันนี้บรรทัดเดียว พอให้รู้ว่าถึงไหนแล้ว
    char line[48];
    snprintf(line, sizeof(line), "%d of %d served   %d baht",
             usedCount, (int)db.size(), usedCount * 35);
    tft.setTextColor(getTftAccentGreen(), getTftCardBg());
    tft.setTextSize(1);
    tft.setCursor(160 - (int)strlen(line) * 3, 152);
    tft.print(line);

    int pct = (db.size() > 0) ? (usedCount * 100) / (int)db.size() : 0;
    drawProgressBar(60, 170, 200, 6, pct);

    drawBentoBottomBar("Tap a card or press the button to wake");
  }

  // นาฬิกาตัวโต วาดซ้ำเฉพาะตอนเวลาเปลี่ยน จอจะได้ไม่กระพริบ
  String curTime = getTimeOnlyStr();
  if (fullRedraw || curTime != lastHostClock) {
    lastHostClock = curTime;
    tft.fillRect(30, 60, 260, 52, getTftCardBg());
    tft.setTextColor(getTftTextMain(), getTftCardBg());
    tft.setTextSize(5);
    tft.setCursor(160 - (int)curTime.length() * 15, 68);
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
    headerTitle = "APPROVED";
    footerDesc  = "Meal paid, 35 baht";
  } 
  else if (status == "DUPLICATE") {
    ledDuplicate();
    screenBg    = 0x8200;             // พื้นหลังจอเฉดส้มอิฐเข้ม
    cardBg      = 0x4900;             // พื้นหลังการ์ดโทนส้มเข้ม
    bannerBg    = ST77XX_ORANGE;      // แถบแบนเนอร์สีส้มสด
    bannerFg    = 0x0000;             // ตัวอักษรสีดำ
    accentColor = ST77XX_ORANGE;
    textMuted   = 0xFDC0;             // ข้อความกำกับสีส้มอ่อน
    headerTitle = "ALREADY SERVED";
    footerDesc  = "This student already ate today";
  } 
  else if (status == "TIME_CLOSED") {
    ledDuplicate();
    screenBg    = 0x6200;             // พื้นหลังโทนส้มอมน้ำตาล
    cardBg      = 0x3900;             // พื้นหลังการ์ดโทนน้ำตาลเข้ม
    bannerBg    = BENTO_NEON_YELLOW;  // แถบแบนเนอร์สีเหลืองเตือนภัย
    bannerFg    = 0x0000;
    accentColor = BENTO_NEON_YELLOW;
    textMuted   = 0xFEE0;
    headerTitle = "CLOSED NOW";
    footerDesc  = "Outside the service hours";
  } 
  else {
    ledRejected();
    screenBg    = 0x8000;             // พื้นหลังจอเฉดแดงทึบเข้ม
    cardBg      = 0x4800;             // พื้นหลังการ์ดโทนแดงเข้ม
    bannerBg    = BENTO_NEON_ROSE;    // แถบแบนเนอร์สีแดงสด
    bannerFg    = BENTO_WHITE;        // ตัวอักษรสีขาว
    accentColor = BENTO_NEON_ROSE;
    textMuted   = 0xFCAE;             // ข้อความกำกับสีชมพูอ่อน
    headerTitle = "NOT ON THE LIST";
    footerDesc  = "This card is not registered";
  }

  tft.fillScreen(screenBg);

  tft.fillRect(0, 0, 320, 36, bannerBg);
  tft.setTextSize(3);
  tft.setTextColor(bannerFg, bannerBg);
  int titleLen = strlen(headerTitle) * 18;
  int titleX = max(4, (320 - titleLen) / 2);
  tft.setCursor(titleX, 7);
  tft.print(headerTitle);

  tft.fillRoundRect(8, 44, 304, 188, 8, cardBg);
  tft.drawRoundRect(8, 44, 304, 188, 8, accentColor);
  tft.drawRoundRect(9, 45, 302, 186, 7, accentColor);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 54);
  tft.print("SERVICE POINT");
  
  tft.setTextSize(2);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 66);
  tft.printf("Point %d", stId);

  tft.drawFastHLine(20, 88, 280, accentColor);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 96);
  tft.print("STUDENT ID");

  String cleanId = (studentId != "-" && studentId.length() > 0) ? studentId : "Unknown";
  tft.setTextSize(3);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 110);
  tft.print(cleanId);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 142);
  tft.print("CARD");

  tft.setTextSize(2);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 154);
  tft.print(maskUID(uid));

  tft.fillRoundRect(16, 184, 288, 36, 6, bannerBg);
  tft.setTextSize(1);
  tft.setTextColor(bannerFg, bannerBg);
  int descLen = strlen(footerDesc) * 6;
  int descX = max(20, (320 - descLen) / 2);
  tft.setCursor(descX, 198);
  tft.print(footerDesc);
}

void renderDeveloperCredit() {
  tft.fillScreen(getTftBg());
  drawHostTopBar("ABOUT");

  drawBentoCard(10, 36, 300, 166, getTftCardBorder(), getTftCardBg());

  drawCardLabel(22, 48, "BUILT BY");
  tft.setTextColor(getTftTextMain(), getTftCardBg());
  tft.setTextSize(2);
  tft.setCursor(22, 64);
  tft.println(DEV_NAME);

  tft.setTextColor(getTftTextMuted(), getTftCardBg());
  tft.setTextSize(1);
  tft.setCursor(22, 92);
  tft.println(DEV_ROLE);
  tft.setCursor(22, 108);
  tft.println("Mahachulalongkornrajavidyalaya Phrae");

  tft.drawFastHLine(22, 126, 276, getTftCardBorder());

  tft.setTextColor(getTftTextMuted(), getTftCardBg());
  tft.setCursor(22, 140); tft.printf("Firmware  v%s\n", APP_VERSION);
  tft.setCursor(22, 156); tft.printf("Board     ESP32-S3 N16R8, %d MB PSRAM\n", ESP.getPsramSize() / 1024 / 1024);
  tft.setCursor(22, 172); tft.println("Screen    2.8 inch 320x240");

  drawBentoBottomBar("Press the button to go back");
}
