# -*- coding: utf-8 -*-
"""ตรวจว่าไม่มีการเรียกฟังก์ชันของ Adafruit GFX ที่ต้องมีทรานแซกชัน SPI เปิดอยู่ก่อน

Adafruit GFX แบ่งฟังก์ชันวาดเป็นสองพวก

  พวกที่เรียกเดี่ยว ๆ ได้   drawPixel, drawLine, drawRect, fillRect,
                          drawRoundRect, fillRoundRect, drawFastHLine ฯลฯ
                          ข้างในมี startWrite() กับ endWrite() ครบในตัว

  พวกที่เรียกเดี่ยว ๆ ไม่ได้  writePixel, writeLine, writeFastHLine, writeFastVLine,
                          writeFillRect, drawCircleHelper, fillCircleHelper
                          ข้างในเรียก writePixel() ตรง ๆ ไม่เปิดทรานแซกชันเอง
                          ถูกออกแบบมาให้ถูกเรียกจากข้างในฟังก์ชันพวกแรกเท่านั้น

ถ้าเรียกพวกที่สองเดี่ยว ๆ ขา CS จะไม่ถูกดึงลง จอไม่ได้รับข้อมูล
**โค้ดคอมไพล์ผ่าน ไม่มีคำเตือน แต่ภาพไม่ขึ้นบนจอเงียบ ๆ**
และโปรแกรมจำลองบน canvas ก็วาดสำเร็จเสมอ จึงมองไม่เห็นเช่นกัน
เกิดขึ้นจริงมาแล้วกับคลื่นของสัญลักษณ์แตะบัตรในรุ่น 122.6.0 และ 122.6.1

ใช้:  python3 tools/proto-check/gfxcheck.py
"""
import io, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bracecheck import strip_line

FILES = ['Canteen_Host_Server/Canteen_Host_Server.ino',
         'Canteen_Host_Server/HostScreen.h',
         'Canteen_Station_Client/Canteen_Station_Client.ino',
         'Canteen_Station_Client/StationScreen.h']

NEEDS_TRANSACTION = ['writePixel', 'writeLine', 'writeFastHLine', 'writeFastVLine',
                     'writeFillRect', 'drawCircleHelper', 'fillCircleHelper',
                     'writeFillRectPreclipped']

CALL = re.compile(r'\b(?:tft|display|canvas)\s*\.\s*(%s)\s*\(' % '|'.join(NEEDS_TRANSACTION))

bad = 0
for path in FILES:
    if not os.path.exists(path):
        continue
    raw = io.open(path, encoding='utf-8').read().split('\n')
    code, ib, ir = [], False, None
    for line in raw:
        c, ib, ir = strip_line(line, ib, ir)
        code.append(c)

    problems, open_tx = [], 0
    for i, line in enumerate(code):
        # นับทรานแซกชันที่เปิดค้างอยู่ ณ บรรทัดนี้
        open_tx += len(re.findall(r'\.\s*startWrite\s*\(', line))
        for m in CALL.finditer(line):
            if open_tx <= 0:
                problems.append('บรรทัด %d เรียก %s() โดยไม่มี startWrite() เปิดไว้ '
                                'ภาพจะไม่ขึ้นบนจอจริงถึงแม้จะคอมไพล์ผ่าน'
                                % (i + 1, m.group(1)))
        open_tx -= len(re.findall(r'\.\s*endWrite\s*\(', line))
        if open_tx < 0:
            open_tx = 0

    print('%-48s %s' % (path, 'OK' if not problems else '*** พบ %d ปัญหา ***' % len(problems)))
    for line in problems:
        print('      ' + line)
    bad += len(problems)

sys.exit(1 if bad else 0)
