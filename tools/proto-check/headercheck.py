# -*- coding: utf-8 -*-
"""ตรวจว่าหัวไฟล์และป้ายเวอร์ชันเป็นไปตาม docs/CODE-STANDARD.md

ตรวจห้าข้อ
  1. ทุกไฟล์ .ino และ .h มีบล็อกหัวไฟล์ พร้อมแท็กบังคับครบ
  2. @file ตรงกับชื่อไฟล์จริง
  3. @version ของไฟล์ .ino ตรงกับ APP_VERSION เป๊ะ
     ส่วนไฟล์ .h คือ "เวอร์ชันที่ไฟล์นี้ถูกแก้ครั้งล่าสุด" ต้องเป็นแถวบนสุดของตาราง
     และต้องไม่ใหม่กว่า APP_VERSION (ไฟล์ที่ไม่ได้แก้ในรุ่นล่าสุดจึงค้างเลขเดิมไว้ได้)
  4. @date เป็นรูปแบบ YYYY-MM-DD และตาราง Revision History มีแถวของเวอร์ชันปัจจุบัน
  5. ป้าย // [x.y.z] ในเนื้อโค้ด อ้างเลขที่มีอยู่จริงในตารางของไฟล์นั้น

ข้อที่ห้ามีไว้กันไม่ให้ปักป้ายด้วยเลขที่ไม่มีใครตามได้
ใช้:  python3 tools/proto-check/headercheck.py
"""
import io, os, re, sys, glob

REQUIRED = ['@file', '@brief', '@version', '@date', '@author']
MARK = re.compile(r'^\s*//\s*\[(\d+\.\d+\.\d+)\]\s*(\S+):', re.M)
VERBS = {'เพิ่ม', 'แก้', 'ย้าย'}

def vtuple(v):
    """แปลง 113.1.0 เป็น (113, 1, 0) เพื่อเทียบว่าใหม่กว่ากันไหม"""
    return tuple(int(x) for x in v.split('.'))

def sketch_version(path):
    """อ่าน APP_VERSION จากไฟล์ .ino ของโฟลเดอร์เดียวกัน"""
    folder = os.path.dirname(path)
    for ino in glob.glob(os.path.join(folder, '*.ino')):
        m = re.search(r'#define\s+APP_VERSION\s+"([^"]+)"',
                      io.open(ino, encoding='utf-8').read())
        if m:
            return m.group(1)
    return None

def check(path):
    problems = []
    text = io.open(path, encoding='utf-8').read()
    if not text.startswith('/**'):
        return ['ไม่มีบล็อกหัวไฟล์ ต้องขึ้นต้นด้วย /**']
    head = text[:text.index(' */\n') + 4]

    tags = {}
    for tag in REQUIRED:
        m = re.search(r'^\s*\*\s*%s\s+(.+?)\s*$' % re.escape(tag), head, re.M)
        if not m:
            problems.append('ไม่มีแท็ก %s' % tag)
        else:
            tags[tag] = m.group(1)

    if '@file' in tags and tags['@file'] != os.path.basename(path):
        problems.append('@file เขียนว่า %s แต่ชื่อไฟล์จริงคือ %s'
                        % (tags['@file'], os.path.basename(path)))

    want = sketch_version(path)
    if '@version' in tags and want:
        if path.endswith('.ino'):
            if tags['@version'] != want:
                problems.append('@version คือ %s แต่ APP_VERSION คือ %s'
                                % (tags['@version'], want))
        elif vtuple(tags['@version']) > vtuple(want):
            problems.append('@version คือ %s ซึ่งใหม่กว่า APP_VERSION ของสเก็ตช์ (%s)'
                            % (tags['@version'], want))

    if '@date' in tags and not re.fullmatch(r'\d{4}-\d{2}-\d{2}', tags['@date']):
        problems.append('@date ต้องเป็นรูปแบบ YYYY-MM-DD ไม่ใช่ %s' % tags['@date'])

    if 'Revision History' not in head:
        problems.append('ไม่มีตาราง @par Revision History')
        return problems

    listed = re.findall(r'^\s*\*\s*\|\s*(\d+\.\d+\.\d+)\s*\|', head, re.M)
    if not listed:
        problems.append('ตาราง Revision History ไม่มีแถวไหนเลย')
    elif '@version' in tags and tags['@version'] != listed[0]:
        problems.append('@version คือ %s แต่แถวบนสุดของตารางคือ %s '
                        'ตารางต้องเรียงจากใหม่ไปเก่า และแถวบนสุดต้องเป็นเวอร์ชันของไฟล์'
                        % (tags['@version'], listed[0]))

    body = text[len(head):]
    for ver, verb in MARK.findall(body):
        if ver not in listed:
            problems.append('ป้าย [%s] อ้างเวอร์ชันที่ไม่มีในตาราง Revision History' % ver)
        if verb not in VERBS:
            problems.append('ป้าย [%s] ใช้คำว่า "%s" ต้องเป็น เพิ่ม/แก้/ย้าย เท่านั้น'
                            % (ver, verb))
    return problems

targets = sys.argv[1:] or sorted(glob.glob('*/*.ino') + glob.glob('*/*.h'))
bad = 0
for f in targets:
    p = check(f)
    marks = len(MARK.findall(io.open(f, encoding='utf-8').read()))
    print('%-48s %-28s %s' % (f, 'ป้ายเวอร์ชัน %d จุด' % marks,
                              'OK' if not p else '*** พบ %d ปัญหา ***' % len(p)))
    for line in p:
        print('      ' + line)
    bad += len(p)
sys.exit(1 if bad else 0)
