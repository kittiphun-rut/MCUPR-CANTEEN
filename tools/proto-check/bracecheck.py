import io, sys, re

def strip_line(line, in_block, in_raw, report=None):
    """ลอกคอมเมนต์และสตริงออกทีละบรรทัด คืนโค้ดล้วนกับสถานะที่ค้างอยู่

    ถ้าส่ง list มาทาง report จะเติมคำว่า 'unterminated' ลงไปเมื่อเจอสตริง
    ที่เปิดแล้วไม่ปิดภายในบรรทัดเดียวกัน ซึ่งในภาษา C++ คือความผิดพลาดเสมอ
    (ยกเว้นบรรทัดที่จงใจต่อด้วยเครื่องหมาย \\ ท้ายบรรทัด)
    เคยพลาดมาแล้วตอนแก้ข้อความ HTML ที่ฝังอยู่ในสตริงแล้วลบคำพูดเปิดทิ้ง
    บรรทัดแบบนั้นคอมไพล์ไม่ผ่าน แต่ปีกกายังครบทุกตัว ตัวนับปีกกาจึงมองไม่เห็น"""
    out = []
    i = 0
    n = len(line)
    while i < n:
        if in_raw is not None:
            end = ')' + in_raw + '"'
            k = line.find(end, i)
            if k < 0: return ''.join(out), in_block, in_raw
            i = k + len(end); in_raw = None; continue
        if in_block:
            k = line.find('*/', i)
            if k < 0: return ''.join(out), True, None
            i = k + 2; in_block = False; continue
        c = line[i]
        if c == '/' and i + 1 < n and line[i+1] == '/':
            break
        if c == '/' and i + 1 < n and line[i+1] == '*':
            in_block = True; i += 2; continue
        m = re.match(r'R"([^(]*)\(', line[i:])
        if m:
            in_raw = m.group(1); i += m.end(); continue
        if c == '"' or c == "'":
            q = c; i += 1; closed = False
            while i < n:
                if line[i] == '\\': i += 2; continue
                if line[i] == q: i += 1; closed = True; break
                i += 1
            if not closed and report is not None and not line.rstrip().endswith('\\'):
                report.append(q)
            continue
        out.append(c); i += 1
    return ''.join(out), in_block, in_raw

import glob, os
targets = []
for f in sorted(glob.glob('Canteen_Host_Server/*.ino') + glob.glob('Canteen_Host_Server/*.h')
                + glob.glob('Canteen_Station_Client/*.ino') + glob.glob('Canteen_Station_Client/*.h')):
    # ตัวตรวจนี้นับปีกกาของ "ทุกสาขา" ของ #if/#else ทั้งที่ตอนคอมไพล์จริง
    # จะถูกเลือกมาแค่สาขาเดียว ส่วนเกินจึงเท่ากับผลรวมของ delta ในสาขา #else
    # คำนวณจากไฟล์จริงแทนการตั้งค่าตายตัว จะได้ไม่ต้องแก้ทุกครั้งที่โค้ดเปลี่ยน
    txt = io.open(f, encoding='utf-8').read().split('\n')
    exp = 0
    ib, ir, depth, in_else = False, None, 0, []
    for ln in txt:
        st = ln.strip()
        code, ib, ir = strip_line(ln, ib, ir)
        if st.startswith('#if'):
            in_else.append(False)
        elif st == '#else' and in_else:
            in_else[-1] = True
        elif st.startswith('#endif') and in_else:
            in_else.pop()
        elif in_else and in_else[-1]:
            exp += code.count('{') - code.count('}')
    targets.append((f, exp, 0))
def unbalanced_quotes(path):
    """หาบรรทัดที่เปิดสตริงแล้วไม่ปิดภายในบรรทัดเดียวกัน"""
    bad = []
    in_block = False; in_raw = None
    for ln, line in enumerate(io.open(path, encoding='utf-8'), 1):
        rep = []
        code, in_block, in_raw = strip_line(line.rstrip('\n'), in_block, in_raw, rep)
        if rep:
            bad.append((ln, line.strip()[:70]))
    return bad

for path, exp_b, exp_p in targets:
    in_block = False; in_raw = None
    b = p = k = 0
    for ln, line in enumerate(io.open(path, encoding='utf-8'), 1):
        code, in_block, in_raw = strip_line(line.rstrip('\n'), in_block, in_raw)
        b += code.count('{') - code.count('}')
        p += code.count('(') - code.count(')')
        k += code.count('[') - code.count(']')
    q = unbalanced_quotes(path)
    ok = (b == exp_b and p == exp_p and k == 0 and not in_block
          and in_raw is None and not q)
    print('%-46s braces=%+d(exp %+d) parens=%+d brackets=%+d quotes=%d raw=%s %s'
          % (path, b, exp_b, p, k, len(q), in_raw, 'OK' if ok else '*** MISMATCH ***'))
    for ln, txt in q[:5]:
        print('      คำพูดไม่ครบคู่ บรรทัด %d: %s' % (ln, txt))
