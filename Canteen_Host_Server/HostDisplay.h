/**
 * ============================================================================
 * HostDisplay.h — ทุกอย่างที่วาดลงจอ TFT ของเครื่องแม่ข่าย
 *
 * แยกออกมาจาก Canteen_Host_Server.ino เพื่อให้ไฟล์หลักไม่ยาวเกิน 3000 บรรทัด
 * เป็นการ "ย้ายที่อยู่" ล้วน ๆ ไม่ได้แก้ตรรกะแม้แต่บรรทัดเดียว
 *
 * ไฟล์นี้ถูก #include ไว้ท้ายไฟล์หลักก่อน setup() เพราะโค้ดข้างในอ้างถึง
 * ตัวแปรส่วนกลางและฟังก์ชันช่วยเหลือที่ประกาศไว้ข้างบน
 * **อย่าย้าย #include ขึ้นไปไว้บนสุด**
 * ============================================================================
 */

#pragma once

// ---------------------------------------------------------------------------
// ชิ้นส่วนพื้นฐาน: การ์ด ป้าย แถบบน แถบล่าง และไอคอนแบตเตอรี่
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// หน้าจอเต็ม: แอนิเมชันตอนบูต สามหน้าหลัก โหมดพักจอ ผลการสแกน และหน้าเครดิต
// ---------------------------------------------------------------------------
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
