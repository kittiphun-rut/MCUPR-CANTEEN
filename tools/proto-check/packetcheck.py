# -*- coding: utf-8 -*-
"""ตรวจว่าแพ็กเก็ตคำสั่งการแสดงผล ถูกประกอบจากที่เดียวเสมอ

HostConfigPacket มีเจ็ดฟิลด์ ถ้าใครประกอบเองทีละฟิลด์แล้วใส่ไม่ครบ
ฟิลด์ที่เหลือจะเป็นศูนย์เงียบ ๆ เพราะประกาศด้วย `= {}`
**คอมไพล์ผ่าน ไม่มีคำเตือน แต่คำสั่งที่ส่งออกไปผิด**

เกิดขึ้นจริงมาแล้วกับ broadcastStationTheme() ในรุ่น 113.0.0 ถึง 113.6.0
ซึ่งตั้งแค่ magic, version, msgType, stationId, darkMode แล้วหยุด
screenOn, screensaver และ modeSeq จึงเป็นศูนย์ติดไปทุกครั้ง
ผลคือคำสั่งพักหน้าจอไม่เคยเดินทางมากับ broadcast เลย
จุดบริการต้องรอ heartbeat รอบถัดไปจึงได้ค่าจริง ช้าไปหลายวินาที

กติกา: ทุกที่ที่ประกอบ HostConfigPacket ต้องเรียก fillStationConfig()
ห้ามตั้ง msgType = MSG_CONFIG เองนอกฟังก์ชันนั้น

ใช้:  python3 tools/proto-check/packetcheck.py
"""
import io, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bracecheck import strip_line
from privacycheck import body_of

INO = 'Canteen_Host_Server/Canteen_Host_Server.ino'

BUILDER = 'fillStationConfig'          # ฟังก์ชันเดียวที่มีสิทธิ์ประกอบแพ็กเก็ตนี้
FIELDS = ['magic', 'version', 'msgType', 'stationId',
          'darkMode', 'screenOn', 'screensaver', 'modeSeq']

raw = io.open(INO, encoding='utf-8').read()
code, ib, ir = [], False, None
for line in raw.split('\n'):
    c, ib, ir = strip_line(line, ib, ir)
    code.append(c)
text = '\n'.join(code)

problems = []

# 1) ตัวประกอบกลางต้องตั้งครบทุกฟิลด์ ไม่งั้นที่อื่นก็พังตามหมด
body = body_of(text, BUILDER)
if body is None:
    problems.append('ไม่พบฟังก์ชัน %s()' % BUILDER)
else:
    for f in FIELDS:
        if not re.search(r'\.\s*%s\s*=' % re.escape(f), body):
            problems.append('%s() ไม่ได้ตั้งฟิลด์ %s ซึ่งจะกลายเป็นศูนย์เงียบ ๆ' % (BUILDER, f))

# ช่วงบรรทัดของตัวประกอบกลาง นับปีกกาเอาเอง
# เทียบด้วยเลขบรรทัด ไม่ใช่การหาสตริงย่อย เพราะบรรทัด `cfg.msgType = MSG_CONFIG;`
# ในสองฟังก์ชันเขียนเหมือนกันเป๊ะ การหาสตริงย่อยจึงตอบว่า "อยู่ข้างใน" ทั้งคู่
# แล้วปล่อยบั๊กผ่านไป ซึ่งเกิดขึ้นจริงกับตัวตรวจรุ่นแรกของไฟล์นี้เอง
builder_from = builder_to = -1
for i, line in enumerate(code):
    if re.match(r'^[A-Za-z_][\w \*]*\b%s\s*\(' % BUILDER, line) and '{' in line:
        depth = 0
        for j in range(i, len(code)):
            depth += code[j].count('{') - code[j].count('}')
            if depth == 0:
                builder_from, builder_to = i, j
                break
        break

# 2) ฟังก์ชันอื่นห้ามประกอบแพ็กเก็ตนี้เอง
for i, line in enumerate(code):
    if not re.search(r'\.\s*msgType\s*=\s*MSG_CONFIG', line):
        continue
    if not (builder_from <= i <= builder_to):
        problems.append('บรรทัด %d ตั้ง msgType = MSG_CONFIG เอง '
                        'ต้องเรียก %s() แทน ไม่งั้นฟิลด์ที่ลืมจะเป็นศูนย์เงียบ ๆ'
                        % (i + 1, BUILDER))

print('%-48s %s' % (INO, 'OK' if not problems else '*** พบ %d ปัญหา ***' % len(problems)))
for line in problems:
    print('      ' + line)
sys.exit(1 if problems else 0)
