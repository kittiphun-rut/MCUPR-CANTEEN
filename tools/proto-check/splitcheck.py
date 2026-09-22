# -*- coding: utf-8 -*-
"""ตรวจปัญหาที่เกิดจากการแยกโค้ดไปไว้ในไฟล์ .h ของสเก็ตช์เดียวกัน

ไฟล์ .h ที่ถูก include ไว้ "ท้ายไฟล์ก่อน setup()" จะถูกแทรกเข้ามาตรงนั้นจริง ๆ
ฟังก์ชันที่นิยามอยู่ในนั้นจึงมาทีหลังโค้ดทุกบรรทัดที่อยู่เหนือบรรทัด include
ถ้าโค้ดข้างบน "เรียกใช้" โดยไม่มีการประกาศล่วงหน้า จะคอมไพล์ไม่ผ่าน

ตรวจสองข้อ
  1. ฟังก์ชันในไฟล์ .h ที่ถูกเรียกจากโค้ดเหนือบรรทัด include ต้องมีการประกาศล่วงหน้า
  2. ค่าปริยายของพารามิเตอร์ต้องระบุที่เดียว ถ้าใส่ทั้งที่ประกาศและที่นิยาม C++ ถือว่าผิด

ฟังก์ชันที่ใช้อยู่ภายในไฟล์ .h เองเท่านั้น ไม่ต้องประกาศล่วงหน้า จึงไม่นับเป็นปัญหา
และสตริงกับคอมเมนต์ถูกลอกออกก่อนเสมอ โค้ด JavaScript ที่ฝังอยู่ใน raw string
จึงไม่ถูกเข้าใจผิดว่าเป็นฟังก์ชันของ C++

ใช้:  python3 tools/proto-check/splitcheck.py
"""
import io, re, os, sys, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bracecheck import strip_line

def strip_all(text):
    """ลอกคอมเมนต์ สตริง และ raw string ออกทั้งไฟล์ เหลือแต่โค้ด"""
    out, ib, ir = [], False, None
    for line in text.split('\n'):
        code, ib, ir = strip_line(line, ib, ir)
        out.append(code)
    return '\n'.join(out)

KEYWORDS = {'if', 'for', 'while', 'switch', 'else', 'return', 'do', 'sizeof', 'catch'}

def funcs_defined(path):
    """ชื่อฟังก์ชันที่นิยามระดับบนสุดในไฟล์ พร้อมข้อความพารามิเตอร์"""
    text = strip_all(io.open(path, encoding='utf-8').read())
    out = {}
    pat = re.compile(r'^([A-Za-z_][\w:<>,\* ]*?)\b(\w+)\s*\(([^;{]*?)\)\s*\{', re.M | re.S)
    for m in pat.finditer(text):
        if m.group(1).strip() in KEYWORDS:
            continue
        out[m.group(2)] = m.group(3)
    return out

def check(ino):
    problems = []
    raw = io.open(ino, encoding='utf-8').read()
    folder = os.path.dirname(ino)
    for m in re.finditer(r'^\s*#include\s+"([^"]+\.h)"', raw, re.M):
        h = os.path.join(folder, m.group(1))
        if not os.path.exists(h):
            continue
        above = strip_all(raw[:m.start()])
        for name, args in funcs_defined(h).items():
            decl = re.search(r'^[A-Za-z_][\w:<>,\* ]*\b%s\s*\(([^;{]*)\);'
                             % re.escape(name), above, re.M | re.S)
            called = re.search(r'(?<![\w.>])%s\s*\(' % re.escape(name), above)
            if called and not decl:
                problems.append('%s() ถูกเรียกในไฟล์หลักก่อนบรรทัด #include "%s" '
                                'แต่ไม่มีการประกาศล่วงหน้า' % (name, m.group(1)))
            if decl and '=' in args and '=' in decl.group(1):
                problems.append('%s() ใส่ค่าปริยายทั้งที่ประกาศและที่นิยาม '
                                'C++ อนุญาตแค่ที่เดียว' % name)
    return problems

targets = sys.argv[1:] or sorted(glob.glob('*/*.ino'))
bad = 0
for ino in targets:
    p = check(ino)
    print('%-48s %s' % (ino, 'OK' if not p else '*** พบ %d ปัญหา ***' % len(p)))
    for line in p:
        print('      ' + line)
    bad += len(p)
sys.exit(1 if bad else 0)
