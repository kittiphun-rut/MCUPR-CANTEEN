# -*- coding: utf-8 -*-
"""ตรวจว่าไม่มีข้อความที่ฟอนต์ของจอวาดไม่ได้หลุดเข้าไปในโค้ดวาดจอ

ฟอนต์ในตัวของ Adafruit GFX มีแต่ ASCII ไบต์ตั้งแต่ 0x80 ขึ้นไปจะถูกวาดเป็น
สัญลักษณ์มั่ว ภาษาไทยหนึ่งตัวกินสามไบต์ จึงกลายเป็นตัวประหลาดสามตัว
และกินที่กว้างกว่าที่โค้ดนับไว้สามเท่า ข้อความจึงล้นไปทับของข้างเคียง

เคยเกิดขึ้นจริงมาแล้ว: ค่าเริ่มต้นของชื่อร้านเป็นภาษาไทย พอขึ้นจอ TFT
จึงอ่านไม่ออกและซ้อนกันตั้งแต่ยังไม่ได้ตั้งค่าอะไรเลย

ตรวจสองอย่างในไฟล์ที่วาดจอ
  1. สตริงในโค้ด (ไม่นับคอมเมนต์) ต้องเป็น ASCII ล้วน
  2. ตัวแปรที่มาจากผู้ใช้หรือฐานข้อมูล ต้องผ่าน asciiOnly() ก่อนถึงจอ
     ข้อนี้ตรวจแบบหยาบ ๆ ด้วยรายชื่อที่ประกาศไว้ว่าต้องกรอง

ใช้:  python3 tools/proto-check/asciicheck.py
"""
import io, os, re, sys, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bracecheck import strip_line

# ไฟล์ที่เนื้อหาทุกตัวอักษรจะไปโผล่บนจอ จึงต้องเป็น ASCII ล้วน
SCREEN_FILES = ['Canteen_Host_Server/HostScreen.h',
                'Canteen_Station_Client/StationScreen.h']

# ตัวแปรที่เนื้อหามาจากคนกรอกหรือจากไฟล์ ต้องผ่าน asciiOnly() ก่อนวาด
MUST_FILTER = ['shops[', 'fullName', 'studentId', 'lastScannedStudentId',
               '.name', 'displayName']

def check_literals(path):
    """สตริงในโค้ดต้องไม่มีไบต์ตั้งแต่ 0x80 ขึ้นไป (คอมเมนต์ยกเว้น)"""
    bad = []
    in_block, in_raw = False, None
    for ln, line in enumerate(io.open(path, encoding='utf-8'), 1):
        line = line.rstrip('\n')
        before = in_raw
        code, in_block, in_raw = strip_line(line, in_block, in_raw)
        if before is not None or in_raw is not None or in_block:
            continue
        # หาสตริงในบรรทัดนี้แล้วดูว่ามีอักขระนอก ASCII ไหม
        for m in re.finditer(r'"((?:[^"\\]|\\.)*)"', line):
            # ข้ามถ้าบรรทัดนี้เป็นคอมเมนต์ล้วน
            if line.lstrip().startswith(('//', '*', '/*')):
                continue
            if any(ord(c) > 126 for c in m.group(1)):
                bad.append((ln, m.group(1)[:40]))
    return bad

def check_filtered(path):
    """ตัวแปรจากผู้ใช้ที่ถูกส่งเข้า tft.print/printf โดยไม่ผ่าน asciiOnly()"""
    bad = []
    # ตัวแปรที่รับค่ามาจาก asciiOnly() แล้ว ถือว่าปลอดภัย ไม่ต้องเตือนซ้ำ
    text = io.open(path, encoding='utf-8').read()
    cleaned = set(re.findall(r'(\w+)\s*=\s*asciiOnly\(', text))
    in_block, in_raw = False, None
    for ln, line in enumerate(io.open(path, encoding='utf-8'), 1):
        line = line.rstrip('\n')
        before = in_raw
        code, in_block, in_raw = strip_line(line, in_block, in_raw)
        if before is not None or in_raw is not None or in_block:
            continue
        if not re.search(r'tft\.print|drawFixedText|drawFitCenteredText|PillBadge', code):
            continue
        if 'asciiOnly' in code or 'ScreenName' in code:
            continue
        if any(re.search(r'\b%s\b' % re.escape(v), code) for v in cleaned):
            continue
        for name in MUST_FILTER:
            if name in code:
                bad.append((ln, line.strip()[:64], name))
                break
    return bad

bad_total = 0
for f in SCREEN_FILES:
    lits = check_literals(f)
    filt = check_filtered(f)
    n = len(lits) + len(filt)
    bad_total += n
    print('%-48s %s' % (f, 'OK' if not n else '*** พบ %d ปัญหา ***' % n))
    for ln, txt in lits:
        print('      บรรทัด %d: สตริงมีอักขระนอก ASCII -> %s' % (ln, txt))
    for ln, txt, name in filt:
        print('      บรรทัด %d: %s ยังไม่ผ่าน asciiOnly() -> %s' % (ln, name, txt))
sys.exit(1 if bad_total else 0)
