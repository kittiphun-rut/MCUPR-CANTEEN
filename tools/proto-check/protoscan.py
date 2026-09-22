# -*- coding: utf-8 -*-
"""หาฟังก์ชันในสเก็ตช์ที่ใช้ชนิดข้อมูลซึ่งนิยามไว้ในไฟล์เดียวกัน
   แต่ไม่มี forward declaration — Arduino builder จะสร้างต้นแบบให้เองแล้ววางไว้
   เหนือจุดที่นิยามชนิดข้อมูลนั้น ทำให้คอมไพล์ไม่ผ่าน"""
import io, re, sys

def scan(path):
    src = io.open(path, encoding='utf-8').read()
    lines = src.split('\n')

    # 1) ชนิดข้อมูลที่นิยามในไฟล์นี้
    types = set()
    types |= set(re.findall(r'^\}\s*(\w+);', src, re.M))            # typedef struct {...} Name;
    types |= set(re.findall(r'^struct\s+(\w+)\s*\{', src, re.M))     # struct Name {
    types |= set(re.findall(r'^enum\s+(?:class\s+)?(\w+)', src, re.M))
    types |= set(re.findall(r'^enum\s+\w+\s*:\s*\w+\s*\{', src, re.M))
    for m in re.finditer(r'^enum\s+(\w+)', src, re.M): types.add(m.group(1))
    types.discard('')

    # 2) นิยามฟังก์ชันที่คอลัมน์ 0 (ลงท้ายด้วย { ในบรรทัดเดียวกัน)
    fdef = re.compile(r'^(?:static\s+|inline\s+)*'
                      r'(?:const\s+)?[A-Za-z_]\w*(?:\s*[*&])?\s+'
                      r'\**(\w+)\s*\(([^;{]*)\)\s*(?:const\s*)?\{\s*$')
    defs = []
    for i, ln in enumerate(lines, 1):
        m = fdef.match(ln)
        if m: defs.append((i, m.group(1), m.group(2), ln.strip()))

    # 3) บรรทัดที่เป็น declaration (ลงท้ายด้วย ; )
    fdecl = re.compile(r'^(?:static\s+|inline\s+)*'
                       r'(?:const\s+)?[A-Za-z_]\w*(?:\s*[*&])?\s+'
                       r'\**(\w+)\s*\([^;{]*\)\s*;\s*$')
    declared = {}
    for i, ln in enumerate(lines, 1):
        m = fdecl.match(ln)
        if m: declared.setdefault(m.group(1), i)

    # 4) บรรทัดที่ชนิดข้อมูลแต่ละตัวถูกนิยามเสร็จ
    type_line = {}
    for t in types:
        for pat in (r'^\}\s*%s;' % re.escape(t), r'^struct\s+%s\s*\{' % re.escape(t),
                    r'^enum\s+(?:class\s+)?%s\b' % re.escape(t)):
            m = re.search(pat, src, re.M)
            if m:
                type_line[t] = src[:m.start()].count('\n') + 1
                break

    bad = []
    for line, name, params, text in defs:
        used = [t for t in types if re.search(r'\b%s\b' % re.escape(t), params)]
        if not used: continue
        dl = declared.get(name)
        # ต้องมี declaration และต้องอยู่หลังจุดที่นิยามชนิดข้อมูลครบแล้ว
        need_after = max((type_line.get(t, 0) for t in used), default=0)
        if dl is None:
            bad.append((line, name, used, 'ไม่มี forward declaration'))
        elif dl < need_after:
            bad.append((line, name, used, 'declaration อยู่บรรทัด %d ก่อนนิยามชนิดข้อมูล (บรรทัด %d)' % (dl, need_after)))
    return types, defs, bad

for path in sys.argv[1:]:
    types, defs, bad = scan(path)
    print('=== %s ===' % path)
    print('ชนิดข้อมูลที่นิยามในไฟล์: %d | ฟังก์ชันระดับบนสุด: %d' % (len(types), len(defs)))
    if not bad:
        print('ไม่พบปัญหา\n')
    else:
        for line, name, used, why in bad:
            print('  บรรทัด %-5d %-28s ใช้ %-22s -> %s' % (line, name, ','.join(used), why))
        print()
