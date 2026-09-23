# -*- coding: utf-8 -*-
"""ตรวจว่าเส้นทางที่เปิดให้ดูโดยไม่ต้องเข้าสู่ระบบ ไม่ปล่อยข้อมูลส่วนบุคคลออกไป

จอสาธารณะบนทีวีในโรงอาหารเปิดดูได้โดยไม่ต้องล็อกอิน ใครเดินผ่านก็เห็น
ข้อกำหนดที่ตกลงกันไว้คือ **ห้ามมีชื่อนิสิต หมายเลขบัตร เลขอ้างอิง
และข้อมูลฮาร์ดแวร์ของเครื่องแม่ข่าย** ส่วนรหัสนิสิตต้องปิดบังเหลือห้าหลักแรก

ตรวจสองอย่าง
  1. ฟังก์ชันที่ตอบข้อมูลสาธารณะ ต้องไม่อ้างถึงฟิลด์ต้องห้ามเลย
  2. เส้นทางที่ประกาศว่าเป็นสาธารณะ ต้องมีเท่าที่ตั้งใจไว้ ไม่มีใครมาเพิ่มทีหลังเงียบ ๆ
     (เส้นทางอื่นทั้งหมดต้องเรียก isAuthenticated() ในตัวจัดการของมัน)

ใช้:  python3 tools/proto-check/privacycheck.py
"""
import io, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bracecheck import strip_line

INO = 'Canteen_Host_Server/Canteen_Host_Server.ino'

# ฟังก์ชันที่ตอบข้อมูลให้คนที่ยังไม่ได้เข้าสู่ระบบ
PUBLIC_HANDLERS = ['handlePublicBoard']

# ฟิลด์ที่ห้ามโผล่ในคำตอบสาธารณะเด็ดขาด
FORBIDDEN = ['fullName', 'uid', 'refNo', 'originalUid', 'lastScannedUID',
             'lastScannedStudentId', 'lastScannedRef', 'adminUsers',
             'getChipTemperature', 'calculateCpuLoad', 'getFreeHeap',
             'readHostBatteryVoltage', 'WiFi.macAddress', 'studentId']

# เส้นทางที่ตั้งใจให้เปิดสาธารณะ นอกจากนี้ต้องตรวจการเข้าสู่ระบบทั้งหมด
ALLOWED_PUBLIC = {'/login', '/logout', '/display', '/api/board',
                  '/generate_204', '/hotspot-detect.html', '/s.css', '/a.js'}

def strip_all(text):
    out, ib, ir = [], False, None
    for line in text.split('\n'):
        code, ib, ir = strip_line(line, ib, ir)
        out.append(code)
    return '\n'.join(out)

def body_of(text, name):
    """คืนเนื้อในของฟังก์ชัน โดยนับปีกกาจากโค้ดที่ลอกสตริงออกแล้ว"""
    m = re.search(r'^[A-Za-z_][\w \*]*\b%s\s*\([^;{]*\)\s*\{' % re.escape(name), text, re.M)
    if not m:
        return None
    i = m.end() - 1
    depth, j = 0, i
    while j < len(text):
        if text[j] == '{': depth += 1
        elif text[j] == '}':
            depth -= 1
            if depth == 0: return text[i:j + 1]
        j += 1
    return None

# [113.7.0] แก้: ห่อเนื้อการตรวจไว้ใน main() แล้วเรียกผ่าน if __name__ == '__main__'
#           ของเดิมโค้ดตรวจอยู่ระดับโมดูล พอไฟล์อื่น import body_of() ไปใช้
#           ไฟล์นี้จะรันตัวเองแล้ว sys.exit() ทันที ตัวตรวจที่ import มันจึงไม่ได้ทำงานเลย
#           แต่ยังพิมพ์ผลออกมาดูเหมือนผ่าน เกิดขึ้นจริงมาแล้วกับ packetcheck.py
def main():
    raw = io.open(INO, encoding='utf-8').read()
    code = strip_all(raw)
    problems = []

    for fn in PUBLIC_HANDLERS:
        body = body_of(code, fn)
        if body is None:
            problems.append('ไม่พบฟังก์ชัน %s()' % fn)
            continue
        for bad in FORBIDDEN:
            if re.search(r'\b%s\b' % re.escape(bad).replace(r'\.', r'\.'), body):
                problems.append('%s() อ้างถึง %s ซึ่งห้ามหลุดออกทางหน้าสาธารณะ' % (fn, bad))

    # เส้นทางไหนบ้างที่ไม่ได้ตรวจการเข้าสู่ระบบ
    for m in re.finditer(r'server\.on\(\s*"([^"]+)"\s*,\s*HTTP_(GET|POST)\s*,\s*(.*)', raw):
        path, method, rest = m.group(1), m.group(2), m.group(3)
        if path in ALLOWED_PUBLIC:
            continue
        tail = raw[m.start():m.start() + 1400]

        # [113.6.0] แก้: รูปแบบสี่อาร์กิวเมนต์ server.on(path, method, handler, upload)
        #           มีตัวจัดการสองตัว ต้องตรวจการเข้าสู่ระบบทุกตัว
        #           เดิมจับเฉพาะตัวเดียวที่ตามด้วยวงเล็บปิด จึงมองข้าม handleFileUpload()
        args = rest.split(')')[0]
        named = [a.strip() for a in args.split(',') if re.fullmatch(r'[A-Za-z_]\w*', a.strip())]
        if named:
            guarded = True
            for fn in named:
                body = body_of(code, fn)
                if not body or 'isAuthenticated' not in body:
                    guarded = False
                    problems.append('เส้นทาง %s (%s) ตัวจัดการ %s() ไม่ได้ตรวจ isAuthenticated()'
                                    % (path, method, fn))
        else:
            guarded = 'isAuthenticated' in tail
            if not guarded:
                problems.append('เส้นทาง %s (%s) ไม่ได้ตรวจ isAuthenticated() และไม่ได้อยู่ในรายการสาธารณะ'
                                % (path, method))

    print('%-48s %s' % (INO, 'OK' if not problems else '*** พบ %d ปัญหา ***' % len(problems)))
    for line in problems:
        print('      ' + line)
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
