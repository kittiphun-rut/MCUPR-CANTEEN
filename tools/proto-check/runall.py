# -*- coding: utf-8 -*-
"""รันตัวตรวจทุกตัว แล้วสรุปผลบรรทัดเดียวต่อหนึ่งตัว

รันแต่ละตัวเป็น **โปรเซสแยก** ไม่ใช่การ import เข้ามา
เพราะตัวตรวจแต่ละตัวเป็นสคริปต์ที่จบด้วย sys.exit() ถ้า import กันเองตรง ๆ
ตัวที่ถูก import จะรันตัวเองแล้วจบโปรแกรมทิ้ง ตัวที่เรียกจึงไม่ได้ทำงานเลย
แต่ยังพิมพ์ผลออกมาดูเหมือนผ่าน เกิดขึ้นจริงมาแล้วกับ packetcheck.py

ใช้:  python3 tools/proto-check/runall.py
"""
import os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))

CHECKS = [
    ('bracecheck',   'วงเล็บและเครื่องหมายคำพูดของทุกไฟล์'),
    ('splitcheck',   'ปัญหาจากการแยกโค้ดไปไว้ในไฟล์ .h'),
    ('ordercheck',   'ฟังก์ชันที่ถูกเรียกก่อนถูกประกาศ'),
    ('gfxcheck',     'ฟังก์ชัน GFX ที่ต้องมี startWrite() ก่อน'),
    ('packetcheck',  'แพ็กเก็ตคำสั่งที่ประกอบเองแล้วใส่ฟิลด์ไม่ครบ'),
    ('pincheck',     'ขาชนกันเองหรือไปโดนขาต้องห้ามของบอร์ด'),
    ('kicadcheck',   'ไฟล์วงจร KiCad ตรงกับผังขาในซอร์สหรือไม่'),
    ('headercheck',  'หัวไฟล์และป้ายเวอร์ชัน'),
    ('asciicheck',   'ข้อความที่ฟอนต์ของจอ TFT วาดไม่ได้'),
    ('privacycheck', 'ข้อมูลส่วนบุคคลบนหน้าสาธารณะ'),
]

bad, detail = 0, []
for name, what in CHECKS:
    r = subprocess.run([sys.executable, os.path.join(HERE, name + '.py')],
                       cwd=ROOT, capture_output=True, text=True)
    out = (r.stdout + r.stderr).rstrip()
    ok = (r.returncode == 0)
    # ตัวตรวจที่ล้มกลางคันจะไม่พิมพ์อะไรเลย ต้องถือว่าไม่ผ่าน ไม่ใช่ผ่านเงียบ ๆ
    if ok and not out:
        ok, out = False, 'ไม่พิมพ์ผลอะไรออกมาเลย น่าจะล้มกลางคัน'
    print('%-14s %-46s %s' % (name, what, 'ผ่าน' if ok else '*** ไม่ผ่าน ***'))
    if not ok:
        bad += 1
        detail.append((name, out))

print()
if not bad:
    print('ตัวตรวจทั้ง %d ตัวผ่านหมด — ยังไม่ได้แทนการคอมไพล์และการทดสอบบนเครื่องจริง' % len(CHECKS))
else:
    for name, out in detail:
        print('--- %s ---' % name)
        print(out)
    print('\n*** มี %d ตัวที่ไม่ผ่าน ***' % bad)
sys.exit(1 if bad else 0)
