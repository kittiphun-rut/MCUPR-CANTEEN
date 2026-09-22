/**
 * ============================================================================
 * HostSlips.h — เนื้อหาสลิปของเครื่องพิมพ์ความร้อน 58 มม. และปลายทาง API
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

#if ENABLE_THERMAL_PRINTER

// ============================================================================
// สลิปเครื่องพิมพ์ความร้อน 58 มม.
// เนื้อหาเป็น "อังกฤษ + ตัวเลข" ล้วนตามที่ตกลงไว้ เพราะหัวพิมพ์ราคาประหยัด
// ไม่มีฟอนต์ไทยในตัว ถ้าส่งไบต์ UTF-8 ภาษาไทยออกไปจะได้อักขระขยะเต็มม้วน
// ทุกข้อความที่มาจากฐานข้อมูล (ชื่อนิสิต ชื่อร้าน) จึงถูกกรองเหลือ ASCII ก่อนเสมอ
// และถ้ากรองแล้วไม่เหลืออะไร จะใช้ข้อความสำรองที่อ่านออกแทน
// ============================================================================

// ตัดให้สั้นพอดีความกว้างกระดาษ ป้องกันบรรทัดล้นไปขึ้นบรรทัดใหม่เอง
static String slipClip(const String &v, int maxLen) {
  if ((int)v.length() <= maxLen) return v;
  return v.substring(0, maxLen);
}

String asciiSafe(const String &raw, const String &fallback) {
  String out;
  out.reserve(raw.length());
  for (unsigned int i = 0; i < raw.length(); i++) {
    uint8_t c = (uint8_t)raw[i];
    if (c >= 32 && c < 127) out += (char)c;
  }
  out.trim();
  if (out.length() == 0) return fallback;
  return out;
}

// สำหรับ "ชื่อคน" และ "ชื่อร้าน" โดยเฉพาะ
// ชื่อไทยอย่าง "ร้านที่ 1" พอกรอง ASCII แล้วจะเหลือแค่ "1" ซึ่งอ่านไม่รู้เรื่อง
// จึงบังคับว่าผลลัพธ์ต้องมีตัวอักษรอังกฤษอย่างน้อยหนึ่งตัว ไม่งั้นใช้ข้อความสำรองแทน
String asciiName(const String &raw, const String &fallback) {
  String out = asciiSafe(raw, "");
  bool hasLetter = false;
  for (unsigned int i = 0; i < out.length(); i++) {
    char c = out[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) { hasLetter = true; break; }
  }
  if (!hasLetter) return fallback;
  return out;
}

static String slipHeader(const String &subtitle) {
  String s;
  s += escInit();
  s += escAlign(1);
  s += escSize(0x11);
  s += escBold(true);
  s += "MCU CANTEEN\n";
  s += escSize(0);
  s += escBold(false);
  s += slipClip(subtitle, PRINTER_COLS) + "\n";
  s += "MCU PHRAE CAMPUS\n";
  s += escAlign(0);
  s += escRule('=');
  return s;
}

static String slipFooter() {
  String s;
  s += escRule('=');
  s += escAlign(1);
  s += "KEEP THIS SLIP AS PROOF\n";
  s += "CANTEEN HOST v" APP_VERSION "\n";
  s += escAlign(0);
  s += escFeed(3);
  s += escCut();
  return s;
}

// สลิปประจำตัวนิสิต 1 ใบต่อการใช้สิทธิ์ 1 ครั้ง
String buildClaimSlip(const Student &s) {
  String sid   = asciiSafe(s.studentId, "-");
  // "NAME  " กินไป 6 ช่อง เหลือให้ชื่อ 26 ช่องพอดีหนึ่งบรรทัด ไม่ล้นไปขึ้นบรรทัดใหม่
  String name  = slipClip(asciiName(s.fullName, "STUDENT " + sid), PRINTER_COLS - 6);
  // ชื่อร้านที่ตั้งเป็นภาษาไทยพิมพ์ออกหัวพิมพ์ไม่ได้ จะเหลือแค่หมายเลขร้าน
  // ถ้าต้องการให้ชื่อร้านขึ้นบนสลิปด้วย ให้ตั้งชื่อร้านเป็นภาษาอังกฤษในแท็บร้านค้า
  String shopN = "-";
  if (s.station >= 1 && s.station <= 4) {
    shopN = slipClip(asciiName(shops[s.station - 1].name, "-"), 21);
  }

  String out = slipHeader("MEAL SUBSIDY RECEIPT");
  out += escPair("DATE", getDateFormattedStr());
  out += escPair("TIME", asciiSafe(s.claimTime, getRealTimeStr()));
  out += escRule('-');

  out += escAlign(1);
  out += "STUDENT ID\n";
  out += escSize(0x11);
  out += sid + "\n";
  out += escSize(0);
  out += escAlign(0);
  out += "NAME  " + name + "\n";
  out += escRule('-');

  out += escPair("SHOP", String(s.station) + " " + shopN);
  // เลขอ้างอิงเต็มรูปแบบยาว 28 ตัว ถ้าใส่คู่กับป้ายชื่อจะเกิน 32 ช่องและถูกตัดขึ้นบรรทัดใหม่เอง
  // จึงขึ้นบรรทัดใหม่ให้ตั้งแต่ต้น เพื่อให้เลขอ้างอิงอยู่ครบในบรรทัดเดียวเสมอ
  out += "REF NO\n";
  out += slipClip(asciiSafe(s.refNo, "-"), PRINTER_COLS) + "\n";
  out += escRule('-');

  out += escAlign(1);
  out += escBold(true);
  out += escSize(0x11);
  out += "35.00 THB\n";
  out += escSize(0);
  out += escBold(false);
  out += "ONE MEAL PER STUDENT PER DAY\n";
  out += escAlign(0);

  out += slipFooter();
  return out;
}

// สลิปสรุปยอดประจำวัน สำหรับแนบเอกสารเบิกจ่าย
String buildDailySlip() {
  int usedCount = 0;
  int shopCounts[4] = {0, 0, 0, 0};
  for (const auto &st : db) {
    if (st.claimed) {
      usedCount++;
      if (st.station >= 1 && st.station <= 4) shopCounts[st.station - 1]++;
    }
  }
  int total = (int)db.size();

  char win[20];
  snprintf(win, sizeof(win), "%02d:%02d-%02d:%02d",
           serviceStartHour, serviceStartMin, serviceEndHour, serviceEndMin);

  String out = slipHeader("DAILY SUMMARY REPORT");
  out += escPair("DATE", getDateFormattedStr());
  out += escPair("PRINTED", getTimeOnlyStr());
  out += escPair("WINDOW", String(win));
  out += escRule('-');
  out += escPair("ELIGIBLE", String(total));
  out += escPair("CLAIMED", String(usedCount));
  out += escPair("REMAINING", String(total - usedCount));
  out += escRule('-');

  for (int i = 0; i < 4; i++) {
    String label = "SHOP " + String(i + 1);
    String value = String(shopCounts[i]) + " x 35 = " + String(shopCounts[i] * 35);
    out += escPair(label, value);
  }
  out += escRule('-');

  out += escAlign(1);
  out += escBold(true);
  out += escSize(0x11);
  out += String(usedCount * 35) + " THB\n";
  out += escSize(0);
  out += escBold(false);
  out += "TOTAL DISBURSED\n";
  out += escAlign(0);
  out += escFeed(2);
  out += "CHECKED BY\n";
  out += escRule('.');
  out += escFeed(1);
  out += "APPROVED BY\n";
  out += escRule('.');
  out += slipFooter();
  return out;
}

// สลิปทดสอบ ใช้ยืนยันว่าสายและไฟเลี้ยงเครื่องพิมพ์พร้อมใช้งาน
String buildTestSlip() {
  String out = slipHeader("PRINTER SELF TEST");
  out += escPair("DATE", getDateFormattedStr());
  out += escPair("TIME", getTimeOnlyStr());
  out += escPair("LINK", printerStatusText());
  out += escRule('-');
  out += "0123456789012345678901234567890\n";
  out += "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n";
  out += escBold(true);
  out += "BOLD SAMPLE\n";
  out += escBold(false);
  out += escSize(0x11);
  out += "BIG SAMPLE\n";
  out += escSize(0);
  out += escRule('-');
  out += escAlign(1);
  out += "IF THIS LOOKS CORRECT\n";
  out += "THE PRINTER IS READY\n";
  out += escAlign(0);
  out += slipFooter();
  return out;
}

bool printClaimSlip(const Student &s) {
  return printerEnqueue(buildClaimSlip(s));
}

// ---------------------------------------------------------------------------
// ปลายทาง API ของเครื่องพิมพ์
// ---------------------------------------------------------------------------
void handlePrintTest() {
  if (!requireAuth()) return;
  if (!printerEnqueue(buildTestSlip())) { sendJson(false, "คิวงานพิมพ์เต็ม กรุณารอสักครู่แล้วลองใหม่"); return; }
  sendJson(true, "ส่งสลิปทดสอบเข้าคิวแล้ว (สถานะ: " + printerStatusText() + ")");
}

void handlePrintSlip() {
  if (!requireAuth()) return;
  String id = server.arg("id"); id.trim();
  for (const auto &st : db) {
    if (st.studentId == id) {
      if (!st.claimed) { sendJson(false, "นิสิต " + id + " ยังไม่ได้ใช้สิทธิ์ของวันนี้ จึงยังไม่มีสลิปให้พิมพ์"); return; }
      if (!printClaimSlip(st)) { sendJson(false, "คิวงานพิมพ์เต็ม กรุณารอสักครู่แล้วลองใหม่"); return; }
      sendJson(true, "ส่งสลิปของรหัส " + id + " เข้าคิวพิมพ์แล้ว");
      return;
    }
  }
  sendJson(false, "ไม่พบรหัสนิสิต " + id + " ในระบบ");
}

void handlePrintDaily() {
  if (!requireAuth()) return;
  if (!printerEnqueue(buildDailySlip())) { sendJson(false, "คิวงานพิมพ์เต็ม กรุณารอสักครู่แล้วลองใหม่"); return; }
  sendJson(true, "ส่งใบสรุปยอดประจำวันเข้าคิวพิมพ์แล้ว");
}

void handleSavePrinterSettings() {
  if (!requireAuth()) return;
  printerAutoSlip = (server.arg("auto") == "1");
  preferences.begin("sys_cfg", false);
  preferences.putBool("prn_auto", printerAutoSlip);
  preferences.end();
  sendJson(true, printerAutoSlip ? "เปิดการพิมพ์สลิปอัตโนมัติแล้ว" : "ปิดการพิมพ์สลิปอัตโนมัติแล้ว");
}

#else   // ENABLE_THERMAL_PRINTER == 0
// ---------------------------------------------------------------------------
// ปิดเครื่องพิมพ์ไว้: เนื้อหาสลิปทั้งหมดไม่ถูกคอมไพล์เข้าไปเลย เหลือแค่โครงเปล่า
// ให้ส่วนอื่นเรียกได้โดยไม่ต้องใส่ #if กระจายเต็มไฟล์ ประหยัดแฟลชได้ทั้งก้อน
// ---------------------------------------------------------------------------
String asciiSafe(const String &raw, const String &fallback) { (void)raw; return fallback; }
String asciiName(const String &raw, const String &fallback) { (void)raw; return fallback; }
String buildClaimSlip(const Student &s) { (void)s; return String(); }
String buildDailySlip() { return String(); }
String buildTestSlip()  { return String(); }
bool   printClaimSlip(const Student &s) { (void)s; return false; }

static const char PRINTER_OFF_MSG[] =
  "เฟิร์มแวร์นี้ปิดการใช้งานเครื่องพิมพ์ไว้ (ENABLE_THERMAL_PRINTER 0) "
  "ถ้าต่อเครื่องพิมพ์แล้วให้เปลี่ยนเป็น 1 ที่หัวสเก็ตช์แล้วอัปโหลดใหม่";

void handlePrintTest()  { if (!requireAuth()) return; sendJson(false, PRINTER_OFF_MSG); }
void handlePrintSlip()  { if (!requireAuth()) return; sendJson(false, PRINTER_OFF_MSG); }
void handlePrintDaily() { if (!requireAuth()) return; sendJson(false, PRINTER_OFF_MSG); }
void handleSavePrinterSettings() { if (!requireAuth()) return; sendJson(false, PRINTER_OFF_MSG); }

#endif  // ENABLE_THERMAL_PRINTER
