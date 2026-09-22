// ชิมบางที่สุดเท่าที่โค้ดที่ดึงมาต้องใช้ แล้วจำลองโปรโตคอลบัญชีสิทธิ์ทั้งวงจร
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <cassert>
#include <ctime>

struct String : std::string {
  String() {}
  String(const char* s) : std::string(s) {}
  String(const std::string& s) : std::string(s) {}
  unsigned int length() const { return (unsigned int)std::string::size(); }
};

static unsigned long g_millis = 0;
static unsigned long millis() { return g_millis; }

#define ESPNOW_PROTO_MAGIC 0xCA
#define ESPNOW_PROTO_VER   2
enum MsgType : uint8_t { MSG_HEARTBEAT = 1, MSG_SCAN_REQ = 2, MSG_SCAN_RESP = 3,
                         MSG_CONFIG = 4, MSG_ROSTER = 5 };

// นาฬิกาจำลอง: ฝั่งแม่ข่ายใช้ DS3231 ฝั่งสถานีใช้เวลาที่ซิงค์มาจากแม่ข่าย
static int g_year = 2026, g_mon = 9, g_day = 22;
struct DateTime { int year() const { return g_year; } int month() const { return g_mon; } int day() const { return g_day; } };
struct RTCStub { DateTime now() const { return DateTime(); } } rtc;
static bool isTimeSynced = true;
static int g_stnYear = 2026, g_stnMon = 9, g_stnDay = 22;
static bool getLocalTime(struct tm* t) {
  t->tm_year = g_stnYear - 1900; t->tm_mon = g_stnMon - 1; t->tm_mday = g_stnDay; return true;
}

struct Student { String uid; bool claimed; };
static std::vector<Student> db;
struct StationNode { bool isOnline; };
static StationNode stationNodes[4] = {{true},{true},{true},{true}};

// สายอากาศจำลอง: นับแพ็กเก็ต และเลือกทิ้งได้ตามต้องการ
static int g_sent = 0, g_dropped = 0;
static int g_dropEveryNth = 0;
static bool sendToStation(uint8_t stationId, const uint8_t* data, size_t len);

#include "gen.inc"

static int g_deliverTo = 1;   // ส่งถึงสถานีจำลองหมายเลขนี้เท่านั้น
static bool sendToStation(uint8_t stationId, const uint8_t* data, size_t len) {
  g_sent++;
  if (g_dropEveryNth > 0 && (g_sent % g_dropEveryNth) == 0) { g_dropped++; return true; }
  if (stationId != g_deliverTo) return true;
  if (len != sizeof(HostRosterPacket)) return true;
  HostRosterPacket r;
  memcpy(&r, data, sizeof(r));
  if (r.magic != ESPNOW_PROTO_MAGIC || r.version != ESPNOW_PROTO_VER) return true;
  if (r.msgType != MSG_ROSTER) return true;
  stnsim::rosterApplyPacket(r);
  return true;
}

// เดินเวลาให้ serviceRosterPush() ได้ปล่อยทุกก้อนออกไป
static void runPush(int ticks = 400) {
  for (int i = 0; i < ticks; i++) { g_millis += 10; hostsim::serviceRosterPush(); }
}
static void heartbeat(uint8_t stId) {
  char stamp[24];
  snprintf(stamp, sizeof(stamp), "R:%u", (unsigned)stnsim::rosterVer);
  hostsim::noteStationRosterVer(stId, stamp);
  runPush();
}

static int fails = 0;
static void check(const char* what, bool ok) {
  printf("%-58s %s\n", what, ok ? "ผ่าน" : "*** ไม่ผ่าน ***");
  if (!ok) fails++;
}
static const char* verdictName(stnsim::RosterVerdict v) {
  switch (v) { case stnsim::ROSTER_UNKNOWN: return "UNKNOWN";
               case stnsim::ROSTER_ELIGIBLE: return "ELIGIBLE";
               case stnsim::ROSTER_CLAIMED: return "CLAIMED";
               default: return "NOT_FOUND"; }
}
static stnsim::RosterVerdict look(const char* uid) { return stnsim::rosterLookup(String(uid)); }

// เลียนแบบสิ่งที่ processScanRequest() ทำตอนตัดสิทธิ์สำเร็จด้วยบัตรปกติ
static void hostClaim(const char* uid) {
  for (auto& s : db) if (s.uid == String(uid)) s.claimed = true;
  hostsim::bumpRosterVer();
  hostsim::sendRosterDelta(String(uid), 1);
}
// เลียนแบบ saveDatabaseToFS(): ชุดผู้มีสิทธิ์เปลี่ยน ผลักชุดเต็มให้ทุกสถานีที่ออนไลน์
static void hostDbChanged() {
  hostsim::bumpRosterVer();
  for (int i = 0; i < 4; i++) if (stationNodes[i].isOnline) hostsim::startRosterPush(i + 1);
  runPush();
}

int main() {
  printf("sizeof(RosterEntry)=%zu  sizeof(HostRosterPacket)=%zu\n\n",
         sizeof(RosterEntry), sizeof(HostRosterPacket));

  // ---- นิสิต 412 คน เลขบัตรสไตล์เดียวกับของจริง ----
  char buf[32];
  for (int i = 0; i < 412; i++) {
    snprintf(buf, sizeof(buf), "03%08d", 5419896 + i * 37);
    db.push_back({String(buf), false});
  }
  db.push_back({String(""), false});   // นิสิตที่ยังไม่ผูกบัตร ต้องไม่ถูกส่งไปกินที่

  printf("== 1) ผลักบัญชีชุดเต็มครั้งแรก ==\n");
  hostDbChanged();
  check("สถานีได้จำนวนรายการครบ 412 (ตัดคนที่ไม่มีบัตรออก)", stnsim::rosterCount == 412);
  check("เลขรุ่นสองฝั่งตรงกัน", stnsim::rosterVer == hostsim::rosterVer);
  check("บัตรที่ลงทะเบียนแล้ว -> ELIGIBLE", look("0305419896") == stnsim::ROSTER_ELIGIBLE);
  check("บัตรแปลกปลอม -> NOT_FOUND", look("0000000001") == stnsim::ROSTER_NOT_FOUND);
  check("บัตรว่าง -> UNKNOWN (ปล่อยผ่าน)", look("") == stnsim::ROSTER_UNKNOWN);
  printf("   ใช้ %d แพ็กเก็ตสำหรับบัญชีชุดเต็ม\n\n", g_sent);

  printf("== 2) ตัดสิทธิ์แล้วส่ง delta ==\n");
  int before = g_sent;
  hostClaim("0305419896");
  check("บัตรที่เพิ่งใช้สิทธิ์ -> CLAIMED", look("0305419896") == stnsim::ROSTER_CLAIMED);
  check("เลขรุ่นยังตรงกัน ไม่ต้องผลักชุดเต็ม", stnsim::rosterVer == hostsim::rosterVer);
  check("delta ใช้แค่ 4 แพ็กเก็ต (สถานีละหนึ่ง)", g_sent - before == 4);
  check("บัตรอื่นยังใช้สิทธิ์ได้", look("0305419933") == stnsim::ROSTER_ELIGIBLE);
  printf("\n");

  printf("== 3) delta หายกลางทาง แล้วกู้คืนเอง ==\n");
  g_deliverTo = 0;                 // สถานีจำลองไม่ได้รับอะไรเลยชั่วคราว
  hostClaim("0305419933");
  g_deliverTo = 1;
  check("สถานียังเห็นบัตรนั้นเป็น ELIGIBLE (ข้อมูลเก่า)",
        look("0305419933") == stnsim::ROSTER_ELIGIBLE);
  hostClaim("0305419970");         // delta ถัดมา เลขรุ่นไม่ต่อกัน
  check("รับ delta ที่เลขรุ่นไม่ต่อกันไม่ได้ -> ทิ้งบัญชีทั้งชุด", stnsim::rosterVer == 0);
  check("ระหว่างไม่มีบัญชี ทุกใบได้ UNKNOWN (ปล่อยผ่าน)",
        look("0000000001") == stnsim::ROSTER_UNKNOWN);
  heartbeat(1);                    // heartbeat รายงานรุ่น 0 -> แม่ข่ายผลักชุดเต็ม
  check("heartbeat ดึงชุดเต็มกลับมาเอง", stnsim::rosterVer == hostsim::rosterVer);
  check("บัตรที่พลาด delta ไป ตอนนี้เป็น CLAIMED แล้ว",
        look("0305419933") == stnsim::ROSTER_CLAIMED);
  check("บัตรที่ใช้สิทธิ์รอบหลังก็เป็น CLAIMED", look("0305419970") == stnsim::ROSTER_CLAIMED);
  printf("\n");

  printf("== 4) สถานีบันทึกเองตอนออฟไลน์ ==\n");
  stnsim::rosterMarkClaimedLocal(String("0305420007"));
  check("แตะครั้งแรกแล้วทำเครื่องหมายไว้ -> ครั้งที่สองเป็น CLAIMED",
        look("0305420007") == stnsim::ROSTER_CLAIMED);
  printf("\n");

  printf("== 5) ปิดยอดประจำวัน คืนสิทธิ์ทุกคน ==\n");
  for (auto& s : db) s.claimed = false;
  hostDbChanged();
  check("ทุกใบกลับมา ELIGIBLE", look("0305419896") == stnsim::ROSTER_ELIGIBLE &&
                                 look("0305419933") == stnsim::ROSTER_ELIGIBLE);
  printf("\n");

  printf("== 6) บัญชีใหญ่เกินที่สถานีเก็บไหว (ROSTER_MAX=%d) ==\n", ROSTER_MAX);
  for (int i = 0; i < 300; i++) {
    snprintf(buf, sizeof(buf), "07%08d", 1000000 + i);
    db.push_back({String(buf), false});
  }
  hostDbChanged();
  check("เก็บไม่ครบ -> ปล่อยผ่านทุกใบ", look("0000000001") == stnsim::ROSTER_UNKNOWN);
  check("แต่ยังรับเลขรุ่นไว้ ไม่ให้แม่ข่ายผลักชุดเต็มซ้ำไม่จบ",
        stnsim::rosterVer == hostsim::rosterVer && stnsim::rosterTruncated == true);
  {
    // heartbeat หลังเก็บไม่ครบ ต้องไม่สั่งผลักชุดเต็มซ้ำ
    int before6 = g_sent;
    heartbeat(1);
    check("heartbeat รอบถัดไปไม่ผลักชุดเต็มซ้ำ", g_sent == before6);
    char stamp[24];
    snprintf(stamp, sizeof(stamp), "R:%u%s", (unsigned)stnsim::rosterVer,
             stnsim::rosterTruncated ? "!" : "");
    hostsim::noteStationRosterVer(1, stamp);
    check("แม่ข่ายรู้ว่าสถานีเก็บบัญชีไม่ไหว", hostsim::stationRosterTooBig[0] == true);
  }
  check("แม่ข่ายนับผู้มีสิทธิ์ที่มีบัตรได้ถูกต้อง", hostsim::countRosterEligible() == 712);
  db.resize(413);
  hostDbChanged();
  check("ลดจำนวนกลับมาแล้วใช้งานได้ตามเดิม",
        stnsim::rosterVer == hostsim::rosterVer && stnsim::rosterCount == 412);
  printf("\n");

  printf("== 7) บัญชีค้างข้ามวัน (เปิดเครื่องตอนเช้าก่อนแม่ข่ายขึ้น) ==\n");
  hostClaim("0305419896");
  check("วันนี้ บัตรที่ใช้สิทธิ์แล้ว -> CLAIMED", look("0305419896") == stnsim::ROSTER_CLAIMED);
  g_stnDay = 23;   // ข้ามไปวันรุ่งขึ้น แต่สถานียังถือบัญชีของเมื่อวาน
  check("ข้ามวันแล้ว สถานะใช้สิทธิ์ของเมื่อวานใช้ไม่ได้ -> ELIGIBLE",
        look("0305419896") == stnsim::ROSTER_ELIGIBLE);
  check("แต่บัตรแปลกปลอมยังถูกปัดตกอยู่", look("0000000001") == stnsim::ROSTER_NOT_FOUND);
  isTimeSynced = false;
  check("นาฬิกายังไม่ซิงค์ ก็ไม่เชื่อสถานะใช้สิทธิ์เช่นกัน",
        look("0305419896") == stnsim::ROSTER_ELIGIBLE);
  check("บัตรแปลกปลอมยังถูกปัดตกแม้นาฬิกาไม่ตรง",
        look("0000000001") == stnsim::ROSTER_NOT_FOUND);
  isTimeSynced = true; g_stnDay = 22;
  check("กลับมาวันเดิม สถานะใช้สิทธิ์ใช้ได้ตามปกติ",
        look("0305419896") == stnsim::ROSTER_CLAIMED);
  printf("\n");

  printf("== 8) ค่าแฮชชนกันไหมในชุดจริง ==\n");
  std::vector<uint32_t> hs;
  for (auto& s : db) if (s.uid.length()) hs.push_back(hostsim::uidHash32(s.uid));
  int dup = 0;
  for (size_t i = 0; i < hs.size(); i++)
    for (size_t j = i + 1; j < hs.size(); j++) if (hs[i] == hs[j]) dup++;
  check("ไม่มีค่าแฮชชนกันในชุดนิสิต 412 คน", dup == 0);
  check("แฮชสองฝั่งให้ผลเท่ากัน",
        hostsim::uidHash32(String("0305419896")) == stnsim::uidHash32(String("0305419896")));
  printf("\n");

  printf("== 9) เลขรุ่นวนรอบที่ 65535 ==\n");
  hostsim::rosterVer = 65535;
  hostsim::bumpRosterVer();
  check("ข้ามค่า 0 ไปเป็น 1", hostsim::rosterVer == 1);
  printf("\n");

  printf("== 10) heartbeat จากเฟิร์มแวร์สถานีรุ่นเก่า (ไม่ฝากเลขรุ่นมา) ==\n");
  hostsim::rosterPushActive[1] = false;
  hostsim::noteStationRosterVer(2, "");
  check("ถูกมองว่าเป็นรุ่น 0 แล้วสั่งผลักชุดเต็มให้", hostsim::rosterPushActive[1] == true);
  printf("\n");

  printf(fails ? "*** มี %d ข้อไม่ผ่าน ***\n" : "ผ่านทั้งหมด (%d ข้อที่ไม่ผ่าน)\n", fails);
  return fails ? 1 : 0;
}
