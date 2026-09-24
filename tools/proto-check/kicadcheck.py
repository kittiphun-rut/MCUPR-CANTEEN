# -*- coding: utf-8 -*-
"""ตรวจว่าไฟล์วงจร KiCad ตรงกับผังขาในซอร์สเฟิร์มแวร์จริง

ไฟล์วงจรกับเฟิร์มแวร์แยกกันอยู่คนละที่ ถ้าวันหนึ่งย้ายขาในโค้ดแล้วลืมสร้างไฟล์วงจรใหม่
**ช่างจะเดินสายตามแบบที่ผิด และไม่มีอะไรฟ้องเลย** จนกว่าจะประกอบเสร็จแล้วใช้ไม่ได้

ตัวตรวจนี้ใช้ netlist ที่ KiCad เอ็กซ์พอร์ตออกมาเอง ไม่ได้อ่านไฟล์วงจรตรง ๆ
จึงเท่ากับให้ KiCad เป็นคนบอกว่าเน็ตเชื่อมถึงกันจริงหรือไม่ แล้วเราค่อยเทียบกับซอร์ส

สร้าง netlist ก่อนด้วย
  cd hardware/kicad
  kicad-cli sch export netlist --output Canteen_Host.net    Canteen_Host.kicad_sch
  kicad-cli sch export netlist --output Canteen_Station.net Canteen_Station.kicad_sch

ใช้:  python3 tools/proto-check/kicadcheck.py
"""
import io, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
KI   = os.path.join(ROOT, 'hardware', 'kicad')
sys.path.insert(0, KI)

NODE = re.compile(r'\(node \(ref "([^"]+)"\)\s*\(pin "([^"]+)"\)')

def parse_nets(path):
    txt = io.open(path, encoding='utf-8').read()
    nets = {}
    for m in re.finditer(r'\(net \(code "\d+"\) \(name "([^"]*)"\)((?:\s*\(node[^\n]*\n?)+)', txt):
        nets[m.group(1).split('/')[-1]] = sorted(NODE.findall(m.group(2)))
    return nets

def main():
    try:
        import generate_kicad as g
    except Exception as e:
        print('เปิด generate_kicad.py ไม่ได้:', e)
        return 1

    boards = [('Canteen_Host', g.HOST_PINS, False), ('Canteen_Station', g.STN_PINS, True)]
    bad = 0
    for title, pins, station in boards:
        net_file = os.path.join(KI, title + '.net')
        if not os.path.exists(net_file):
            print('%-18s ยังไม่ได้เอ็กซ์พอร์ต netlist ข้ามไป' % title)
            continue
        nets = parse_nets(net_file)
        want = g.board_nets(pins, station)
        problems = []

        # 1) ทุกขาที่เฟิร์มแวร์ใช้ ต้องอยู่ในเน็ตชื่อเดียวกันในไฟล์วงจร
        for pin_no, net in sorted(want.items()):
            if net not in nets:
                problems.append('ไม่มีเน็ต %s ในไฟล์วงจร' % net)
            elif ('U1', str(pin_no)) not in nets[net]:
                problems.append('U1 ขา %s ควรอยู่เน็ต %s แต่ไม่อยู่' % (pin_no, net))

        # 2) เน็ตสัญญาณต้องมีอย่างน้อยสองปลาย ปลายเดียวคือลืมต่อ
        #    ที่มีเกินสองได้คือบัสที่ตั้งใจให้ใช้ร่วมกัน ต้องอยู่ในรายการข้างล่างเท่านั้น
        #    ถ้าเน็ตอื่นมีสามปลายแปลว่าขาชนกันโดยไม่ตั้งใจ
        # ตั้งแต่รุ่น 113.12.0 การ์ด SD ย้ายไปบัสของตัวเอง จึงไม่มีบัสที่ใช้ร่วมกันแล้ว
        # เก็บกลไกไว้เผื่ออนาคต รายการว่างแปลว่าเน็ตสัญญาณทุกเส้นต้องมีสองปลายพอดี
        SHARED_BUS = ()
        POWER      = ('GND', '+3V3', 'VBAT', 'VBAT_SENSE')
        for name, nodes in sorted(nets.items()):
            if name in POWER or name.startswith('unconnected'):
                continue
            if len(nodes) < 2:
                problems.append('เน็ต %s มีปลายเดียว น่าจะลืมต่อ: %s' % (name, nodes))
            elif len(nodes) > 2 and name not in SHARED_BUS:
                problems.append('เน็ต %s มี %d ปลาย ทั้งที่ไม่ใช่บัสที่ใช้ร่วมกัน: %s'
                                % (name, len(nodes), nodes))

        # 3) ขา GPIO ของ U1 ต้องไม่ถูกใช้ซ้ำสองเน็ต
        seen = {}
        for name, nodes in nets.items():
            for ref, pin in nodes:
                if ref != 'U1':
                    continue
                if pin in seen and seen[pin] != name:
                    problems.append('U1 ขา %s อยู่ทั้งเน็ต %s และ %s' % (pin, seen[pin], name))
                seen[pin] = name

        print('%-18s %d เน็ต · %s' % (title, len(nets),
              'ตรงกับซอร์สเฟิร์มแวร์' if not problems else '*** พบ %d ปัญหา ***' % len(problems)))
        for line in problems:
            print('      ' + line)
        bad += len(problems)
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main())
