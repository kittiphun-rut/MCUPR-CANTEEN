# -*- coding: utf-8 -*-
"""ดึงฟังก์ชันจริงจากไฟล์ .ino ออกมาคอมไพล์ด้วย g++ บนเครื่อง
   เป็นการตรวจทั้งไวยากรณ์และพฤติกรรมของโปรโตคอลบัญชีสิทธิ์โดยไม่ต้องมี toolchain ของ ESP32"""
import io, re, sys

HOST = "../../Canteen_Host_Server/Canteen_Host_Server.ino"
STN  = "../../Canteen_Station_Client/Canteen_Station_Client.ino"

def grab_func(path, header):
    """ดึงตั้งแต่บรรทัดที่ขึ้นต้นด้วย header จนถึง '}' ที่คอลัมน์ 0"""
    src = io.open(path, encoding='utf-8').read()
    i = src.index(header)
    j = src.index('\n}\n', i) + 3
    return src[i:j]

def grab_between(path, start, end):
    src = io.open(path, encoding='utf-8').read()
    i = src.index(start)
    j = src.index(end, i)
    return src[i:j]

host_funcs = [
    'uint32_t uidHash32(const String &uid) {',
    'void bumpRosterVer() {',
    'uint32_t todayYmd() {',
    'uint16_t countRosterEligible() {',
    'void rebuildRosterSnapshot() {',
    'void startRosterPush(uint8_t stationId) {',
    'void serviceRosterPush() {',
    'void sendRosterDelta(const String &uid, uint8_t state) {',
    'void noteStationRosterVer(uint8_t stationId, const char *stamp) {',
]
stn_funcs = [
    'uint32_t uidHash32(const String &uid) {',
    'uint32_t stationTodayYmd() {',
    'RosterVerdict rosterLookup(const String &uid) {',
    'void rosterMarkClaimedLocal(const String &uid) {',
    'void rosterApplyPacket(const HostRosterPacket &r) {',
]

structs = grab_between(HOST, '#define ROSTER_ENTRIES_PER_PKT 38',
                       'static_assert(sizeof(StationPacket) == 50')
# ตรวจว่าฝั่งสถานีนิยามโครงสร้างเหมือนกันเป๊ะ
stn_structs = grab_between(STN, '#define ROSTER_ENTRIES_PER_PKT 38',
                           'static_assert(sizeof(StationPacket) == 50')
def norm(t):
    t = re.sub(r'//[^\n]*', '', t)
    return re.sub(r'\s+', ' ', t).strip()
if norm(structs) != norm(stn_structs):
    print('*** โครงสร้างแพ็กเก็ตสองฝั่งไม่ตรงกัน ***')
    a, b = norm(structs), norm(stn_structs)
    for k in range(min(len(a), len(b))):
        if a[k] != b[k]:
            print('ต่างกันที่ตำแหน่ง', k)
            print('HOST:', a[max(0,k-60):k+60])
            print('STN :', b[max(0,k-60):k+60])
            break
    sys.exit(1)
print('// โครงสร้างแพ็กเก็ตสองฝั่งตรงกันทุกไบต์', file=sys.stderr)

host_globals = grab_between(HOST, 'uint16_t rosterVer = 1;', 'const unsigned long ROSTER_CHUNK_GAP_MS = 30;') + 'const unsigned long ROSTER_CHUNK_GAP_MS = 30;\n'
stn_globals  = grab_between(STN, '#define ROSTER_MAX 600', 'const unsigned long ROSTER_SAVE_GAP_MS = 300000;')

out = io.open('gen.inc', 'w', encoding='utf-8')
out.write('// ==== โครงสร้างร่วม (ดึงจาก .ino ตรง ๆ) ====\n')
out.write(structs)
out.write('\nnamespace hostsim {\n')
out.write(host_globals)
for f in host_funcs: out.write('\n' + grab_func(HOST, f))
out.write('\n}  // namespace hostsim\n')
out.write('\nnamespace stnsim {\n')
out.write(stn_globals)
for f in stn_funcs: out.write('\n' + grab_func(STN, f))
out.write('\n}  // namespace stnsim\n')
out.close()
print('gen.inc written', file=sys.stderr)
