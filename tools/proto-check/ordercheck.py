# -*- coding: utf-8 -*-
"""ตรวจว่าในไฟล์เดียวกัน ไม่มีฟังก์ชันไหนถูกเรียกก่อนที่จะถูกประกาศ

C++ อ่านไฟล์จากบนลงล่าง ฟังก์ชันที่ถูกเรียกต้องถูก "รู้จัก" ก่อนแล้ว
คือมีนิยามอยู่ข้างบน หรือมีบรรทัดประกาศล่วงหน้าอยู่ข้างบน
ถ้าไม่มีทั้งสองอย่าง คอมไพเลอร์จะฟ้อง 'xxx' was not declared in this scope

ตัวตรวจเดิม splitcheck.py ดูแลเฉพาะรอยต่อระหว่าง .ino กับ .h ที่ถูก #include
จึงมองไม่เห็นกรณีที่ทั้งผู้เรียกและผู้ถูกเรียกอยู่ในไฟล์เดียวกัน
ซึ่งเกิดขึ้นจริงมาแล้วกับ drawStandbyReadyState() ในรุ่น 122.6.0
ที่ถูกวางไว้บรรทัด 503 แต่ถูกเรียกจาก refreshStationLiveValues() บรรทัด 393

ใช้:  python3 tools/proto-check/ordercheck.py
"""
import io, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bracecheck import strip_line

# คอมไพเลอร์เห็น .ino กับ .h ที่ถูก #include เป็นไฟล์เดียวกัน
# บรรทัดประกาศล่วงหน้าที่อยู่ใน .ino เหนือ #include จึงมีผลกับโค้ดในไฟล์ .h ด้วย
# ตัวตรวจต้องมองแบบเดียวกัน ไม่งั้นจะฟ้องผิดทุกฟังก์ชันที่ประกาศไว้ใน .ino
UNITS = [('Canteen_Host_Server/Canteen_Host_Server.ino',
          ['Canteen_Host_Server/HostScreen.h',
           'Canteen_Host_Server/WebDashboard.h']),
         ('Canteen_Station_Client/Canteen_Station_Client.ino',
          ['Canteen_Station_Client/StationScreen.h'])]

# คำที่ตามด้วยวงเล็บแต่ไม่ใช่การเรียกฟังก์ชน
KEYWORDS = {'if', 'for', 'while', 'switch', 'return', 'sizeof', 'else',
            'do', 'catch', 'defined', 'static_cast', 'reinterpret_cast'}

# ชื่อ + วงเล็บเปิด โดยไม่มีจุดหรือลูกศรนำหน้า (กันไม่ให้จับ tft.print เป็นการเรียกฟังก์ชันอิสระ)
CALL = re.compile(r'(?<![\w.>:])([A-Za-z_]\w*)\s*\(')

# หัวฟังก์ชันที่ขึ้นต้นบรรทัด เช่น  void ชื่อ(...)   หรือ  static String ชื่อ(...)
HEAD = re.compile(r'^[A-Za-z_][\w:]*(?:\s*[\*&]|\s+[\w:\*&]+)*\s+([A-Za-z_]\w*)\s*\(')

def read_code(path):
    """คืนโค้ดทีละบรรทัดโดยลอกสตริงและคอมเมนต์ออกแล้ว"""
    raw = io.open(path, encoding='utf-8').read().split('\n')
    out, ib, ir = [], False, None
    for line in raw:
        c, ib, ir = strip_line(line, ib, ir)
        out.append(c)
    return out


def declared_in_sketch(ino_path, header_path):
    """ชื่อฟังก์ชันที่ .ino ประกาศล่วงหน้าไว้เหนือบรรทัด #include ของไฟล์ .h นี้"""
    code = read_code(ino_path)
    base = os.path.basename(header_path)
    limit = len(code)
    for i, line in enumerate(code):
        if '#include' in line and base in line:
            limit = i
            break
    names = set()
    for line in code[:limit]:
        m = HEAD.match(line)
        if m and line.rstrip().endswith(';'):
            names.add(m.group(1))
    return names


def scan(path, outer=frozenset()):
    code = read_code(path)
    defs, decls, heads = {}, {}, set()
    for i, line in enumerate(code):
        m = HEAD.match(line)
        if not m:
            continue
        name = m.group(1)
        if name in KEYWORDS:
            continue
        # ไล่หาวงเล็บปิดของรายการพารามิเตอร์ แล้วดูว่าตามด้วย { (นิยาม) หรือ ; (ประกาศ)
        depth, j, k, tail = 0, i, line.index('('), ''
        while j < len(code):
            row = code[j]
            while k < len(row):
                if row[k] == '(': depth += 1
                elif row[k] == ')':
                    depth -= 1
                    if depth == 0:
                        tail = row[k + 1:]
                        break
                k += 1
            if depth == 0: break
            j, k = j + 1, 0
        if depth != 0:
            continue
        # ข้ามรายการเริ่มต้นของ constructor และ = 0 ฯลฯ
        tail = tail.strip()
        if tail.startswith('{'):
            heads.add((i, j))
            defs.setdefault(name, i)
        elif tail.startswith(';'):
            heads.add((i, j))
            decls.setdefault(name, i)

    problems = []
    for i, line in enumerate(code):
        # ไม่นับบรรทัดที่เป็นหัวฟังก์ชันเอง
        if any(a <= i <= b for a, b in heads):
            continue
        for m in CALL.finditer(line):
            name = m.group(1)
            if name in KEYWORDS or name not in defs or name in outer:
                continue
            if i < defs[name] and (name not in decls or decls[name] > i):
                problems.append('บรรทัด %d เรียก %s() ซึ่งนิยามอยู่บรรทัด %d '
                                'คือต่ำกว่าจุดที่เรียก และไม่มีบรรทัดประกาศล่วงหน้าอยู่ข้างบน'
                                % (i + 1, name, defs[name] + 1))
    return problems

bad = 0
for ino, headers in UNITS:
    for f in [ino] + headers:
        if not os.path.exists(f):
            continue
        outer = frozenset() if f == ino else declared_in_sketch(ino, f)
        probs = scan(f, outer)
        print('%-48s %s' % (f, 'OK' if not probs else '*** พบ %d ปัญหา ***' % len(probs)))
        for line in sorted(set(probs)):
            print('      ' + line)
        bad += len(probs)
sys.exit(1 if bad else 0)
