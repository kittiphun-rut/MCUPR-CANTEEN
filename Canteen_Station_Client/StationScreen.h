/**
 * @file      StationScreen.h
 * @brief     ทุกอย่างที่วาดลงจอ TFT ของเครื่องประจำร้านค้า
 * @version   122.8.0
 * @date      2026-09-23
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
 * | 122.8.0 | 2026-09-23 | เปลี่ยนคำว่า Point บนหน้าจอเป็น Station ให้ตรงกับชื่อที่ใช้ทั้งระบบ ส่วนชื่อร้านยังใช้คำว่า Shop เหมือนเดิม |
 * | 122.6.2 | 2026-09-23 | คลื่นของสัญลักษณ์แตะบัตรไม่ขึ้นบนจอจริง เพราะ drawCircleHelper() ของไลบรารีไม่เปิดทรานแซกชัน SPI เอง เปลี่ยนมาวาดครึ่งวงกลมเองด้วย drawPixel() |
 * | 122.6.1 | 2026-09-23 | ย้าย drawStandbyReadyState() ขึ้นไปไว้ใต้ drawRfidTapIcon() เพราะเดิมถูกวางไว้ต่ำกว่า refreshStationLiveValues() ที่เรียกใช้ ทำให้คอมไพล์ไม่ผ่าน |
 * | 122.6.0 | 2026-09-23 | คืนสัญลักษณ์แตะบัตร RFID กลับมาบนหน้าแรก คลื่นเป็นสีเขียวเมื่อเครื่องอ่านพร้อม และเป็นสีแดงพร้อมข้อความเตือนเมื่อไม่ตอบ |
 * | 122.5.0 | 2026-09-23 | จัดกึ่งกลางหน้าพักจอ ย้ายหน่วยเงินลงใต้ตัวเลข และกระจายแถวหน้า SYSTEM ให้เต็มการ์ด |
 * | 122.4.0 | 2026-09-23 | กรองชื่อและรหัสนิสิตให้เหลือเฉพาะ ASCII ก่อนวาด และตัดความยาวทุกช่อง |
 * | 122.3.0 | 2026-09-22 | เลิกล้างพื้นก่อนเขียนตัวอักษร และแถบสัญญาณเทียบด้วยจำนวนขีดแทนค่า dBm ดิบ |
 * | 122.2.0 | 2026-09-22 | เพิ่ม refreshStationLiveValues() วาดซ้ำเฉพาะตัวเลขที่เปลี่ยน นาฬิกาเดินแล้ว |
 * | 122.1.0 | 2026-09-22 | แยกออกมาจากไฟล์หลัก ย้ายแบบยกก้อน ไม่แก้เนื้อใน |
 * | 122.0.0 | 2026-09-22 | ออกแบบหน้าจอใหม่ให้เรียบง่าย ตัวอักษรน้อย แบ่งช่องชัดเจน และเติมค่าที่หายไปในหน้าตรวจสอบระบบ |
 * | 117.0.7 | 2026-09-21 | ต้นฉบับที่ใช้เป็นจุดเริ่ม เก็บสำเนาไว้ที่ original/ |
 *
 * @warning  ถูก #include ท้ายไฟล์หลักก่อน setup() ห้ามย้ายขึ้นไปบนสุด
 * @warning  ค่าปริยายของพารามิเตอร์ต้องอยู่ที่การประกาศล่วงหน้าในไฟล์หลักเท่านั้น
 *           ใส่ซ้ำที่นิยามในไฟล์นี้ไม่ได้ ตรวจด้วย tools/proto-check/splitcheck.py
 */

#pragma once

#include <stdarg.h>   // drawFixedText รับอาร์กิวเมนต์แบบ printf

// [122.4.0] เพิ่ม: บีบข้อความให้เหลือเฉพาะอักขระที่ฟอนต์ในตัวของจอวาดได้จริง
//
// ฟอนต์ของ Adafruit GFX มีแต่ ASCII ไบต์ตั้งแต่ 0x80 ขึ้นไปจะถูกวาดเป็นสัญลักษณ์มั่ว
// ภาษาไทยหนึ่งตัวกินสามไบต์ จึงกลายเป็นตัวประหลาดสามตัว และ **กินที่กว้างกว่าที่นับไว้
// สามเท่า** ข้อความจึงล้นกรอบไปทับของข้างเคียง เห็นเป็นอักษรซ้อนกัน
//
// ตัดทิ้งไปเลยแทนการแทนด้วยเครื่องหมายคำถาม เพราะชื่อไทยล้วนจะได้เหลือสตริงว่าง
// แล้วให้ผู้เรียกถอยไปใช้ชื่อภาษาอังกฤษสำรองได้ถูกต้อง
// [122.5.0] เพิ่ม: วาดข้อความให้อยู่กึ่งกลางแกน x โดยไม่ต้องล้างพื้นก่อน
//
// drawFixedText() เติมช่องว่างท้ายอย่างเดียว ข้อความจึงชิดซ้ายของช่องเสมอ
// พอเอาไปใช้แทนบรรทัดที่เคยจัดกึ่งกลาง ข้อความเลยเยื้องไปทางซ้าย
//
// ตัวนี้เติมช่องว่างทั้งสองข้างให้ช่องกว้างคงที่ ข้อความจึงอยู่กึ่งกลางจริง
// และช่องว่างที่เติมก็ลบของเดิมที่ยาวกว่าไปพร้อมกัน ไม่ต้องล้างพื้น ไม่กะพริบ
//
// cx คือจุดกึ่งกลางที่ต้องการ ส่วน width คือความกว้างของช่องเป็นจำนวนตัวอักษร
// ต้องกว้างพอสำหรับค่าที่ยาวที่สุดที่เป็นไปได้ ไม่งั้นจะถูกตัด
void drawCenteredText(int cx, int y, uint8_t size, uint16_t fg, uint16_t bg,
                      int width, const char* fmt, ...) {
  if (width < 1) width = 1;
  if (width > 70) width = 70;

  char raw[80];
  va_list args;
  va_start(args, fmt);
  vsnprintf(raw, sizeof(raw), fmt, args);
  va_end(args);

  int len = (int)strlen(raw);
  if (len > width) { len = width; raw[width] = '\0'; }
  int left = (width - len) / 2;

  char padded[80];
  memset(padded, ' ', sizeof(padded));
  memcpy(padded + left, raw, len);
  padded[width] = '\0';

  tft.setTextSize(size);
  tft.setTextColor(fg, bg);
  tft.setCursor(cx - (width * 6 * (int)size) / 2, y);
  tft.print(padded);
}


String asciiOnly(const String &raw) {
  String out;
  out.reserve(raw.length());
  for (unsigned int i = 0; i < raw.length(); i++) {
    uint8_t c = (uint8_t)raw[i];
    if (c >= 0x20 && c < 0x7F) out += (char)c;
  }
  out.trim();
  return out;
}


// [122.3.0] เพิ่ม: วาดข้อความทับที่เดิมโดยไม่ต้องล้างพื้นก่อน
//
// ต้นเหตุของการกะพริบคือลำดับ "ล้างพื้น แล้วค่อยเขียนตัวอักษร"
// ระหว่างสองจังหวะนั้นจอว่างเปล่าจริง ๆ ตาคนจึงเห็นเป็นการกะพริบทุกวินาที
//
// Adafruit GFX ลงสีพื้นให้ทุกตัวอักษรอยู่แล้วเมื่อกำหนดสีพื้นไว้ด้วย
// (setTextColor สองอาร์กิวเมนต์) ข้อความใหม่จึงเขียนทับของเดิมได้ในจังหวะเดียว
// ไม่มีช่วงที่จอว่าง ไม่ต้องเรียก fillRect เลย
//
// เงื่อนไขเดียวคือความยาวต้องคงที่ จึงเติมช่องว่างท้ายข้อความให้ครบ width เสมอ
// ไม่งั้นตัวอักษรเก่าที่ยาวกว่าจะค้างอยู่ เช่น 187 เปลี่ยนเป็น 9 แล้วเหลือ 87
// ใช้ %-*s ซึ่งเติมให้ครบแต่ไม่ตัดทิ้ง ค่าที่ยาวเกินคาดจึงยังแสดงครบ ไม่โกหกตัวเลข
void drawFixedText(int x, int y, uint8_t size, uint16_t fg, uint16_t bg,
                   int width, const char* fmt, ...) {
  char raw[64];
  va_list args;
  va_start(args, fmt);
  vsnprintf(raw, sizeof(raw), fmt, args);
  va_end(args);

  // ตัดให้ยาวเท่า width พอดีเสมอ ไม่ใช่แค่เติมให้ครบ
  // ถ้าปล่อยให้ยาวเกินได้ ข้อความจะล้นไปทับช่องข้างเคียง และพอรอบหน้าค่าสั้นลง
  // ช่องว่างที่เติมก็ลบของเก่าไม่หมด เหลือเป็นอักษรซ้อนกันค้างอยู่
  // ทุกช่องจึงถูกกำหนดความกว้างเผื่อไว้แล้วให้ค่าจริงยาวไม่ถึง
  char padded[72];
  snprintf(padded, sizeof(padded), "%-*.*s", width, width, raw);

  tft.setTextSize(size);
  tft.setTextColor(fg, bg);
  tft.setCursor(x, y);
  tft.print(padded);
}



// ============================================================================
// ชิ้นส่วนพื้นฐาน: การ์ด ป้าย แถบบน แถบล่าง และการย่อตัวอักษรให้พอดีกรอบ
// ============================================================================

void drawStationCard(int x, int y, int w, int h, uint16_t borderColor, uint16_t bgColor) {
  tft.fillRoundRect(x, y, w, h, 6, bgColor);
  tft.drawRoundRect(x, y, w, h, 6, borderColor);
}

// ============================================================================
// สัญลักษณ์แตะบัตร RFID
// ============================================================================
// [122.6.0] เพิ่ม: เอาสัญลักษณ์กลับมา เคยมีตั้งแต่รุ่น 118.1.0 แล้วหายไปตอนเริ่มใหม่
//           จากต้นฉบับในรุ่น 122.0.0 นิสิตที่เดินมาถึงจึงไม่มีอะไรบอกว่าเครื่องพร้อม
//           วาดด้วยพรีมิทีฟของ Adafruit GFX ล้วน ๆ จึงไม่กินแฟลชเพิ่มเหมือนการฝังบิตแมป
//
// [122.6.2] แก้: เดิมวาดคลื่นด้วย tft.drawCircleHelper() แล้วเส้นไม่ขึ้นบนจอจริง
//           ทั้งที่ตัวบัตรขึ้นครบ ต้นเหตุคือฟังก์ชันนั้นของไลบรารีเรียก writePixel()
//           ตรง ๆ ซึ่ง "ไม่เปิดทรานแซกชัน SPI เอง" มันถูกออกแบบมาให้ถูกเรียก
//           จากข้างใน drawRoundRect() ที่ startWrite() ไว้แล้วเท่านั้น
//           เรียกเดี่ยว ๆ แบบเรา ขา CS ไม่ถูกดึงลง จอจึงไม่ได้รับข้อมูลเลย
//           จึงเขียนวงกลมครึ่งขวาเองด้วย tft.drawPixel() ซึ่งเปิดปิดทรานแซกชันให้ในตัว
//
// กรอบที่สัญลักษณ์นี้กิน เทียบกับจุดกึ่งกลางที่ส่งเข้ามา
//   แนวนอน  cx-33 ถึง cx+36   (70 พิกเซล)
//   แนวตั้ง  cy-22 ถึง cy+22   (45 พิกเซล)
// ตัวเลขชุดนี้ต้องตรงกับ RFID_ICON_* ข้างล่าง ซึ่งใช้ตอนล้างพื้นก่อนวาดทับ
#define RFID_ICON_X(cx)  ((cx) - 34)
#define RFID_ICON_Y(cy)  ((cy) - 23)
#define RFID_ICON_W      72
#define RFID_ICON_H      47

// วาดครึ่งขวาของวงกลมด้วยอัลกอริทึมของ Bresenham ชุดเดียวกับที่ไลบรารีใช้
// แต่ลงจุดด้วย tft.drawPixel() ซึ่งเปิดและปิดทรานแซกชัน SPI ให้เองทุกจุด
// จึงวาดเดี่ยว ๆ ได้โดยไม่ต้องมี startWrite() ครอบไว้ข้างนอก
// เพิ่มจุดขวาสุดให้ด้วย เพราะลูปของไลบรารีข้ามจุดนั้นไป ทำให้ปลายโค้งขาดหนึ่งพิกเซล
void drawRightArc(int cx, int cy, int r, uint16_t color) {
  int f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
  tft.drawPixel(cx + r, cy, color);
  while (x < y) {
    if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
    x++; ddF_x += 2; f += ddF_x;
    tft.drawPixel(cx + x, cy + y, color);
    tft.drawPixel(cx + y, cy + x, color);
    tft.drawPixel(cx + x, cy - y, color);
    tft.drawPixel(cx + y, cy - x, color);
  }
}

void drawRfidTapIcon(int cx, int cy, uint16_t cardColor, uint16_t waveColor, uint16_t bgColor) {
  // ตัวบัตร วาดสองชั้นให้เส้นหนาขึ้น เพื่อให้เห็นชัดจากอีกฝั่งของเคาน์เตอร์
  tft.drawRoundRect(cx - 33, cy - 15, 42, 30, 5, cardColor);
  tft.drawRoundRect(cx - 32, cy - 14, 40, 28, 4, cardColor);

  // ชิปสัมผัสบนหน้าบัตร เส้นสีพื้นสองเส้นตัดให้เป็นลายชิป
  tft.fillRoundRect(cx - 27, cy - 8, 12, 10, 2, cardColor);
  tft.drawFastHLine(cx - 27, cy - 4, 12, bgColor);
  tft.drawFastVLine(cx - 21, cy - 8, 10, bgColor);

  // คลื่นสัญญาณสามชั้น ไล่รัศมีออกไปทางขวา
  for (int i = 0; i < 3; i++) {
    int r = 9 + i * 6;
    drawRightArc(cx + 14, cy, r, waveColor);
    drawRightArc(cx + 14, cy, r + 1, waveColor);
  }
}

// [122.6.0] เพิ่ม: สัญลักษณ์แตะบัตรพร้อมบรรทัดคำอธิบาย บนหน้าแรกของจุดบริการ
//           แยกออกมาเป็นฟังก์ชันของตัวเองเพราะถูกเรียกจากสองที่
//           คือตอนวาดหน้าใหม่ทั้งหน้า และตอนที่เครื่องอ่านบัตรเปลี่ยนสถานะ
//           ทั้งสองที่จึงใช้พิกัดชุดเดียวกันเสมอ ไม่มีทางเลื่อนออกจากกันได้
//
// สีคลื่นบอกความพร้อม เขียวคือเครื่องอ่านตอบอยู่ แดงคือไม่ตอบ
// ล้างพื้นก่อนวาดเพราะเส้นโค้งวาดทับเส้นโค้งเดิมไม่มิด สีเก่าจะค้างตามขอบ
// จุดนี้วาดเฉพาะตอนสถานะเปลี่ยนซึ่งนาน ๆ ครั้ง จึงไม่ทำให้กลับไปกะพริบทุกวินาที
void drawStandbyReadyState(bool ready) {
  uint16_t bg = getStCardBg();
  tft.fillRect(RFID_ICON_X(160), RFID_ICON_Y(72), RFID_ICON_W, RFID_ICON_H, bg);
  drawRfidTapIcon(160, 72, getStTextMain(), ready ? getStGreen() : getStRose(), bg);

  // ความกว้าง 44 ตัวอักษรที่ขนาด 1 คือ 264 พิกเซล พอดีกับช่องในการ์ด
  // ข้อความยาวไม่เท่ากัน จึงต้องเป็น drawCenteredText ที่เติมช่องว่างทั้งสองข้าง
  // ไม่ใช่ drawFitCenteredText ที่ไม่ลบของเดิม
  if (ready) {
    drawCenteredText(160, 139, 1, getStTextMuted(), bg, 44,
                     "Free meal  35 baht  once a day");
  } else {
    drawCenteredText(160, 139, 1, getStRose(), bg, 44,
                     "Card reader not responding - call staff");
  }
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

  // [122.3.0] แก้: เดิมเทียบด้วยค่า dBm ดิบ ๆ ซึ่งแกว่งเกินสามหน่วยแทบทุกวินาที
  //           แถบบนจึงถูกล้างแล้ววาดใหม่ตลอดเวลา เห็นเป็นการกะพริบมุมขวาบน
  //           ตอนนี้เทียบด้วยจำนวนขีดกับสถานะออนไลน์ ซึ่งนาน ๆ เปลี่ยนที
  if (!forceRedraw && online == lastDrawnOnline && currentBars == lastBars) {
    // ตัวเลข dBm ยังอัปเดตได้ เพราะเขียนทับที่เดิมโดยไม่ล้างพื้น ไม่ทำให้กะพริบ
    if (online && rssi != lastDrawnRssi) {
      drawFixedText(194, 8, 1, getStTextMuted(), getStBg(), 6, "%ddB", rssi);
      lastDrawnRssi = rssi;
    }
    return;
  }

  lastDrawnRssi = rssi;
  lastDrawnOnline = online;
  lastBars = currentBars;

  if (online) {
    drawFixedText(194, 8, 1, getStTextMuted(), getStBg(), 6, "%ddB", rssi);
    drawSignalBars(228, 6, rssi, true, getStBg());
  } else {
    drawFixedText(192, 8, 1, getStRose(), getStBg(), 7, "OFFLINE");
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
// การวาดซ้ำเฉพาะตัวเลขที่เปลี่ยน
// ============================================================================

// [122.2.0] เพิ่ม: อัปเดตตัวเลขบนหน้าจอโดยไม่ต้องวาดใหม่ทั้งหน้า
//           ของเดิมโค้ดอัปเดตยอดอยู่ใน loop() ของไฟล์หลัก ซึ่งมีพิกัดของตัวเอง
//           พอหน้าจอถูกออกแบบใหม่ พิกัดสองชุดจึงไม่ตรงกันและวาดผิดที่
//           ย้ายมาอยู่ข้างเดียวกับโค้ดที่วาดหน้านั้นจริง จะได้แก้พร้อมกันเสมอ
// [122.3.0] แก้: เลิกล้างพื้นก่อนเขียน ใช้การเขียนทับที่เดิมแทน จอจะได้ไม่กะพริบ
void refreshStationLiveValues(bool force) {
  if (!isScreenOn) return;
  // หน้าแจ้งผล หน้าขาดการเชื่อมต่อ และหน้าเครดิต ยึดพื้นที่ทั้งจอไว้
  // ห้ามไปวาดทับ ไม่งั้นแถบสัญญาณจะไปโผล่กลางหน้าแจ้งผลการแตะบัตร
  if (currentState != STATE_STANDBY &&
      currentState != STATE_STATUS &&
      currentState != STATE_SCREENSAVER) return;

  static int  lastServed = -1;
  static String lastClock = "";
  static int  lastOnline = -1;
  static int  lastReady  = -1;

  int served = (int)totalSuccessToday;
  String clock = getTimeOnlyStr();

  // ---- หน้าแรก: ยอดวันนี้มุมซ้ายล่าง และนาฬิกามุมขวาล่าง ----
  if (currentState == STATE_STANDBY && currentStationPage == 1) {
    // [122.6.0] เพิ่ม: สัญลักษณ์แตะบัตรเปลี่ยนสีตามความพร้อมของเครื่องอ่านบัตร
    //           วาดเฉพาะตอนสถานะเปลี่ยน ไม่ได้วาดทุกวินาที
    if (force || (int)isReaderReady != lastReady) {
      drawStandbyReadyState(isReaderReady);
    }
    if (force || served != lastServed) {
      drawFixedText(28, 191, 2, getStTextMain(), getStCardBg(), 4, "%d", served);
    }
    if (force || clock != lastClock) {
      // HH:MM:SS ยาวคงที่แปดตัว ขนาด 2 กว้างตัวละ 12 จึงเริ่มที่ 292-96 = 196
      drawFixedText(196, 191, 2, getStTextMain(), getStCardBg(), 8, "%s", clock.c_str());
    }
  }
  // ---- หน้าสอง: ยอดจาน ยอดเงิน สถานะการเชื่อมต่อ และนาฬิกา ----
  else if (currentState == STATE_STANDBY && currentStationPage == 2) {
    if (force || served != lastServed) {
      // ต้องตรงกับที่ displayStatsDashboard() วาดเป๊ะ
      drawFixedText(14, 62, 4, getStTextMain(), getStCardBg(), 3, "%d", served);
      drawFixedText(14, 112, 3, getStGreen(), getStCardBg(), 5, "%d", served * 35);
      drawFixedText(14, 140, 1, getStTextMuted(), getStCardBg(), 5, "baht");
    }
    if (force || (int)isHostOnline != lastOnline) {
      // ป้ายเป็นสี่เหลี่ยมมุมโค้ง การวาดทับป้ายเดิมจึงไม่ลบสีที่มุมทั้งสี่
      // ต้องล้างกรอบสี่เหลี่ยมเต็ม ๆ ก่อน ไม่งั้นเปลี่ยนจากเขียวเป็นแดงแล้วมุมยังเขียวค้าง
      tft.fillRect(172, 118, 96, 20, getStCardBg());
      drawStationPillBadge(172, 118, 96, 20, isHostOnline ? "ONLINE" : "OFFLINE",
                           isHostOnline ? (isStationDarkMode ? 0x0000 : 0xFFFF) : 0xFFFF,
                           isHostOnline ? getStGreen() : getStRose());
    }
    if (force || clock != lastClock) {
      drawFixedText(172, 166, 2, getStTextMain(), getStCardBg(), 8, "%s", clock.c_str());
    }
  }
  // ---- หน้าพักจอ: ยอดวันนี้บรรทัดเดียว นาฬิกามีคนดูแลอยู่แล้วในฟังก์ชันของมันเอง ----
  else if (currentState == STATE_SCREENSAVER) {
    if (force || served != lastServed) {
      // ต้องตรงกับที่ renderScreensaver() วาดเป๊ะ ไม่งั้นจะได้สองบรรทัดคนละตำแหน่ง
      drawCenteredText(160, 146, 1, getStGreen(), getStCardBg(), 32,
                       "%d served today   %d baht", served, served * 35);
    }
  }

  lastServed = served;
  lastClock  = clock;
  lastOnline = (int)isHostOnline;
  lastReady  = (int)isReaderReady;

  // แถบสัญญาณและแบตเตอรี่มุมขวาบน วาดเองเมื่อค่าเปลี่ยนพอสมควร
  updateTopRightHeaderSmooth(lastHostRssi, isHostOnline, force);
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
  tft.println("Canteen station");

  drawStationCard(50, 95, 220, 48, getStGreen(), getStCardBg());
  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(2);
  // [122.8.0] แก้: "Station N" กว้างกว่า "Point N" จึงเลื่อนจุดเริ่มให้กลับมาอยู่กลางการ์ด
  tft.setCursor(106, 110);
  tft.printf("Station %d", currentStationId);

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

  // ช่องใหญ่ช่องเดียว มีสัญลักษณ์แตะบัตรกับประโยคเดียวที่นิสิตต้องอ่าน
  drawStationCard(16, 34, 288, 130, getStCardBorder(), getStCardBg());
  drawStandbyReadyState(isReaderReady);
  drawFitCenteredText(24, 100, 272, 32, "TAP YOUR CARD", 4, getStTextMain(), getStCardBg());

  // ช่องล่าง: ยอดของจุดบริการนี้วันนี้ และเวลา
  drawStationCard(16, 172, 288, 40, getStCardBorder(), getStCardBg());
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(28, 178);
  tft.print("SERVED TODAY");
  // ตำแหน่งและความกว้างต้องตรงกับ refreshStationLiveValues() เป๊ะ
  // ไม่งั้นพอวาดซ้ำครั้งแรกตัวเลขจะขยับที่ แล้วของเดิมค้างอยู่
  drawFixedText(28, 191, 2, getStTextMain(), getStCardBg(), 4, "%d", (int)totalSuccessToday);
  drawFixedText(196, 191, 2, getStTextMain(), getStCardBg(), 8,
                "%s", getTimeOnlyStr().c_str());

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

  // [122.5.0] แก้: เดิมคำว่า baht อยู่ที่ x คงที่ทางขวาของตัวเลข
  //           พอตัวเลขสั้นกว่าช่องก็เหลือช่องว่างกลางอากาศ ดูไม่เข้าชุดกัน
  //           ย้ายมาอยู่ใต้ตัวเลข ชิดซ้ายเหมือนบรรทัดอื่นในการ์ดนี้
  drawFixedText(14, 62, 4, getStTextMain(), getStCardBg(), 3, "%d", (int)totalSuccessToday);
  drawFixedText(14, 112, 3, getStGreen(), getStCardBg(), 5, "%d", (int)totalSuccessToday * 35);
  drawFixedText(14, 140, 1, getStTextMuted(), getStCardBg(), 5, "baht");

  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(14, 174);
  tft.print("35 baht per student");

  // ขวา: จุดบริการนี้คือจุดไหน ต่อกับแม่ข่ายอยู่ไหม และเวลาเท่าไร
  drawStationCard(164, 30, 150, 172, getStCardBorder(), getStCardBg());
  tft.setTextColor(getStTextMuted(), getStCardBg());
  tft.setTextSize(1);
  tft.setCursor(172, 40);
  tft.print("THIS STATION");

  tft.setTextColor(getStTextMain(), getStCardBg());
  tft.setTextSize(4);
  tft.setCursor(172, 62);
  tft.printf("%d", currentStationId);

  drawStationPillBadge(172, 118, 96, 20, isHostOnline ? "ONLINE" : "OFFLINE",
                       isHostOnline ? (isStationDarkMode ? 0x0000 : 0xFFFF) : 0xFFFF,
                       isHostOnline ? getStGreen() : getStRose());

  drawFixedText(172, 166, 2, getStTextMain(), getStCardBg(), 8,
                "%s", getTimeOnlyStr().c_str());

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
    // [122.5.0] แก้: กระจายหกแถวให้เต็มการ์ด ของเดิมกองอยู่ครึ่งบน เหลือที่ว่างข้างล่างเยอะ
    tft.setCursor(16, 46);  tft.print("Host link");
    tft.setCursor(16, 72);  tft.print("Station");
    tft.setCursor(16, 98);  tft.print("Chip");
    tft.setCursor(16, 124); tft.print("Battery");
    tft.setCursor(16, 150); tft.print("Served today");
    tft.setCursor(16, 176); tft.print("Address");

    tft.setTextColor(getStTextMain(), getStCardBg());
    tft.setCursor(140, 72);  tft.printf("No. %d", currentStationId);
    tft.setCursor(140, 176); tft.print(WiFi.macAddress());

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

    // [122.3.0] แก้: หน้านี้อัปเดตทุกครึ่งวินาที การล้างพื้นก่อนเขียนจึงกะพริบถี่ที่สุด
    if (isHostOnline) {
      drawFixedText(140, 46, 1, getStGreen(), getStCardBg(), 24,
                    "Connected  %d dB", lastHostRssi);
    } else {
      drawFixedText(140, 46, 1, getStRose(), getStCardBg(), 24, "Not connected");
    }

    drawFixedText(140, 98, 1, (chipT < 65.0f) ? getStTextMain() : getStYellow(),
                  getStCardBg(), 24, "%.0f C   cpu %.0f%%", chipT, cpuL);

    // สองแถวนี้เดิมมีแต่หัวข้อ ไม่เคยมีค่าโผล่มาเลย เติมให้ครบ
    float volt = readBatteryVoltage();
    drawFixedText(140, 124, 1, getStTextMain(), getStCardBg(), 24,
                  "%d%%   %.2f V", getBatteryPercentage(volt), volt);
    drawFixedText(140, 150, 1, getStTextMain(), getStCardBg(), 24,
                  "%d meals   %d baht", (int)totalSuccessToday, (int)totalSuccessToday * 35);
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

    drawStationBottomBar("Tap your card or press the button to wake");
  }

  // [122.5.0] แก้: ทุกบรรทัดบนหน้านี้จัดกึ่งกลางที่ x = 160 เหมือนกันหมด
  //           ของเดิมบรรทัดสีเขียวชิดซ้ายที่ x = 62 จึงเยื้องไม่ตรงกับนาฬิกาและวันที่
  static String lastSaverDate = "";
  static int lastSaverServed = -1;
  static int lastSaverBatt = -1;

  // นาฬิกาตัวโต ยาวคงที่แปดตัว
  String curTime = getTimeOnlyStr();
  if (fullRedraw || curTime != lastStationClock) {
    lastStationClock = curTime;
    drawCenteredText(160, 62, 5, getStTextMain(), getStCardBg(), 8, "%s", curTime.c_str());
  }

  String dateStr = getDateFormattedStr();
  if (fullRedraw || dateStr != lastSaverDate) {
    lastSaverDate = dateStr;
    drawCenteredText(160, 118, 1, getStTextMuted(), getStCardBg(), 20, "%s", dateStr.c_str());
  }

  if (fullRedraw || usedCount != lastSaverServed) {
    lastSaverServed = usedCount;
    drawCenteredText(160, 146, 1, getStGreen(), getStCardBg(), 32,
                     "%d served today   %d baht", usedCount, usedCount * 35);
  }

  int battPct = getBatteryPercentage(readBatteryVoltage());
  if (fullRedraw || battPct != lastSaverBatt) {
    lastSaverBatt = battPct;
    drawCenteredText(160, 168, 1, getStTextMuted(), getStCardBg(), 16, "battery %d%%", battPct);
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

  // [122.4.0] แก้: กรองให้เหลือเฉพาะ ASCII อีกชั้นก่อนวาด
  //           ปกติแม่ข่ายแทนชื่อไทยด้วย "Student <รหัส>" ให้อยู่แล้ว
  //           แต่ถ้าวันหนึ่งมีชื่อไทยหลุดมาได้ จอจะขึ้นสัญลักษณ์มั่วเต็มการ์ด
  String displayName = asciiOnly(name);
  if (displayName.length() == 0 || displayName == "-") displayName = "Unknown card";
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

  String cleanId = asciiOnly(id);
  if (cleanId.length() == 0 || cleanId == "-") cleanId = "Unknown";
  tft.setTextSize(3);
  tft.setTextColor(textColor, cardBg);
  tft.setCursor(20, 114);
  tft.print(cleanId);

  tft.setTextSize(1);
  tft.setTextColor(textMuted, cardBg);
  tft.setCursor(20, 150);
  tft.printf("Station %d", currentStationId);
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
    else tft.print("Release to change the station number");
  } else {
    if (remain > 0) tft.printf("AUTO SAVE IN %lu SEC", sec);
    else tft.print("SAVING...");
  }
}
