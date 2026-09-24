# -*- coding: utf-8 -*-
"""สร้างไฟล์วงจรสำหรับ KiCad 7 ขึ้นมาจากผังขาชุดเดียวกับที่เฟิร์มแวร์ใช้จริง

ทำไมต้องสร้างด้วยสคริปต์แทนที่จะวาดมือ
  ผังขาอยู่ในซอร์สของเฟิร์มแวร์ ถ้าวาดมือแล้ววันหนึ่งย้ายขา
  ไฟล์วงจรจะเพี้ยนจากของจริงโดยไม่มีใครรู้ สคริปต์นี้ **อ่านค่าขาจากซอร์สจริง**
  แล้วสร้างใหม่ทั้งหมด สั่งรันซ้ำเมื่อไรก็ตรงกับเฟิร์มแวร์เสมอ

ผลลัพธ์ต่อหนึ่งบอร์ด
  <ชื่อ>.kicad_pro   ไฟล์โปรเจกต์
  <ชื่อ>.kicad_sch   ไฟล์วงจร ฝังสัญลักษณ์ไว้ในตัวครบ เปิดได้โดยไม่ต้องมีไลบรารีอื่น
  MCUPR_Canteen.kicad_sym  ไลบรารีสัญลักษณ์ เผื่ออยากแก้หรือเอาไปใช้ต่อ

ใช้:  python3 hardware/kicad/generate_kicad.py
ตรวจ: kicad-cli sch erc  แล้ว  kicad-cli sch export svg
"""
import io, os, re, sys, uuid as _uuid

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
MM = 2.54                                  # ระยะกริดมาตรฐานของ KiCad

def uid():
    return str(_uuid.uuid4())

# ---------------------------------------------------------------------------
# อ่านค่าขาจากซอร์สของเฟิร์มแวร์ ไม่ได้พิมพ์ซ้ำไว้ที่นี่
# ---------------------------------------------------------------------------
def read_pins(ino):
    src = io.open(os.path.join(ROOT, ino), encoding='utf-8').read()
    out = {}
    for m in re.finditer(r'^#define\s+([A-Z0-9_]+)\s+(\d+)', src, re.M):
        out[m.group(1)] = int(m.group(2))
    return out

HOST_PINS = read_pins('Canteen_Host_Server/Canteen_Host_Server.ino')
STN_PINS  = read_pins('Canteen_Station_Client/Canteen_Station_Client.ino')

# ---------------------------------------------------------------------------
# ขาของบอร์ด ESP32-S3 DevKitC-1 เรียงตามตำแหน่งจริงบนบอร์ด
# หมายเลขขาในสัญลักษณ์: 1-22 คือแถวซ้ายไล่จากบนลงล่าง, 23-44 คือแถวขวา
# ---------------------------------------------------------------------------
LEFT = ['3V3', '3V3', 'RST', 'GPIO4', 'GPIO5', 'GPIO6', 'GPIO7', 'GPIO15',
        'GPIO16', 'GPIO17', 'GPIO18', 'GPIO8', 'GPIO3', 'GPIO46', 'GPIO9',
        'GPIO10', 'GPIO11', 'GPIO12', 'GPIO13', 'GPIO14', '5V0', 'GND']
RIGHT = ['GND', 'GPIO43', 'GPIO44', 'GPIO1', 'GPIO2', 'GPIO42', 'GPIO41',
         'GPIO40', 'GPIO39', 'GPIO38', 'GPIO37', 'GPIO36', 'GPIO35', 'GPIO0',
         'GPIO45', 'GPIO48', 'GPIO47', 'GPIO21', 'GPIO20', 'GPIO19', 'GND', 'GND']

POWER_OUT = {1, 2}                 # ขา 3V3 สองขาบนสุด เป็นแหล่งจ่ายของบอร์ดเอง
POWER_IN  = {22, 23, 43, 44, 21}   # GND ทั้งหมด กับ 5V0

# ---------------------------------------------------------------------------
# โมดูลภายนอก: (ชื่อสัญลักษณ์, รายชื่อขา, ชนิดขา)
# ---------------------------------------------------------------------------
MODULES = {
    'ST7789_LCD':  ['GND', 'VCC', 'SCL', 'SDA', 'RES', 'DC', 'CS', 'BLK'],
    'RC522_RFID':  ['SDA', 'SCK', 'MOSI', 'MISO', 'IRQ', 'GND', 'RST', '3V3'],
    'DS3231_RTC':  ['32K', 'SQW', 'SCL', 'SDA', 'VCC', 'GND'],
    'BUZZER':      ['+', '-'],
    'SW_PUSH':     ['1', '2'],
    'CONN_BATT':   ['+', '-'],
    # หัวต่อการ์ด SD ที่อยู่ด้านหลังโมดูลจอ เป็นหัวต่อคนละชุดกับแถว SPI ของจอ
    'SD_SLOT':     ['SD_SCK', 'SD_MISO', 'SD_MOSI', 'SD_CS'],
}
MODULE_PWR = {
    'ST7789_LCD': {'GND': 'power_in', 'VCC': 'power_in'},
    'RC522_RFID': {'GND': 'power_in', '3V3': 'power_in'},
    'DS3231_RTC': {'GND': 'power_in', 'VCC': 'power_in'},
}

# ---------------------------------------------------------------------------
# ตัวช่วยเขียน S-expression ของ KiCad
# ---------------------------------------------------------------------------
FONT = '(effects (font (size 1.27 1.27)))'

def sym_pin(etype, x, y, angle, name, number, length=5.08):
    return ('      (pin %s line (at %.2f %.2f %d) (length %.2f)\n'
            '        (name "%s" %s)\n'
            '        (number "%s" %s)\n'
            '      )\n' % (etype, x, y, angle, length, name, FONT, number, FONT))

SYMBOL_BOX = {}   # ชื่อสัญลักษณ์ -> (ขอบบน, ขอบล่าง) ในระบบพิกัดของสัญลักษณ์

def make_symbol(name, left, right, body_w, y_top, pin_types=None, value=None):
    """สร้างสัญลักษณ์สี่เหลี่ยมหนึ่งตัว ขาซ้ายกับขาขวาเป็นรายการ (ชื่อ, เลขขา)"""
    pin_types = pin_types or {}
    n = max(len(left), len(right))
    half_w = body_w / 2.0
    y_bot = y_top - (n - 1) * MM
    top = y_top + MM
    bot = y_bot - MM
    SYMBOL_BOX[name] = (top, bot)
    s = ['  (symbol "%s" (pin_names (offset 0.508)) (in_bom yes) (on_board yes)\n' % name]
    s.append('    (property "Reference" "U" (at 0 %.2f 0) %s)\n' % (top + 2.54, FONT))
    s.append('    (property "Value" "%s" (at 0 %.2f 0) %s)\n' % (value or name, bot - 2.54, FONT))
    s.append('    (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))\n')
    s.append('    (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))\n')
    s.append('    (symbol "%s_0_1"\n' % name)
    s.append('      (rectangle (start %.2f %.2f) (end %.2f %.2f)\n'
             '        (stroke (width 0.254) (type default)) (fill (type background)))\n'
             % (-half_w, top, half_w, bot))
    s.append('    )\n')
    s.append('    (symbol "%s_1_1"\n' % name)
    for i, (pname, pnum) in enumerate(left):
        y = y_top - i * MM
        s.append(sym_pin(pin_types.get(pnum, 'bidirectional'), -half_w - 5.08, y, 0, pname, pnum))
    for i, (pname, pnum) in enumerate(right):
        y = y_top - i * MM
        s.append(sym_pin(pin_types.get(pnum, 'bidirectional'), half_w + 5.08, y, 180, pname, pnum))
    s.append('    )\n  )\n')
    return ''.join(s)

def esp32_symbol():
    left  = [(LEFT[i], str(i + 1)) for i in range(22)]
    right = [(RIGHT[i], str(i + 23)) for i in range(22)]
    types = {}
    for p in POWER_OUT: types[str(p)] = 'power_out'
    for p in POWER_IN:  types[str(p)] = 'power_in'
    types['3'] = 'input'                       # RST
    return make_symbol('ESP32-S3-DevKitC-1', left, right, 25.4, 26.67, types,
                       value='ESP32-S3-DevKitC-1 N16R8')

def module_symbol(name):
    pins = MODULES[name]
    left = [(p, str(i + 1)) for i, p in enumerate(pins)]
    types = {}
    for p, t in MODULE_PWR.get(name, {}).items():
        types[str(pins.index(p) + 1)] = t
    if name in ('BUZZER', 'SW_PUSH', 'CONN_BATT'):
        types = {str(i + 1): 'passive' for i in range(len(pins))}
    return make_symbol(name, left, [], 17.78, 0.0, types)

def passive_symbol(name, letter):
    SYMBOL_BOX[name] = (2.54, -2.54)
    return (
        '  (symbol "%s" (pin_names (offset 0.508) hide) (in_bom yes) (on_board yes)\n'
        '    (property "Reference" "%s" (at 2.54 1.27 0) %s)\n'
        '    (property "Value" "%s" (at 2.54 -1.27 0) %s)\n'
        '    (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))\n'
        '    (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))\n'
        '    (symbol "%s_0_1"\n'
        '      (rectangle (start -1.016 2.54) (end 1.016 -2.54)\n'
        '        (stroke (width 0.254) (type default)) (fill (type none)))\n'
        '    )\n'
        '    (symbol "%s_1_1"\n'
        '%s%s    )\n  )\n' % (
            name, letter, FONT, name, FONT, name, name,
            sym_pin('passive', 0, 5.08, 270, '1', '1', 2.54),
            sym_pin('passive', 0, -5.08, 90, '2', '2', 2.54)))

def pwr_flag_symbol():
    SYMBOL_BOX['PWR_FLAG'] = (2.54, 0.0)
    return (
        '  (symbol "PWR_FLAG" (power) (pin_numbers hide) (pin_names (offset 0) hide)\n'
        '    (in_bom no) (on_board yes)\n'
        '    (property "Reference" "#FLG" (at 0 1.905 0) (effects (font (size 1.27 1.27)) hide))\n'
        '    (property "Value" "PWR_FLAG" (at 0 3.81 0) %s)\n'
        '    (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))\n'
        '    (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))\n'
        '    (symbol "PWR_FLAG_0_0"\n'
        '      (pin power_out line (at 0 0 90) (length 0)\n'
        '        (name "pwr" %s)\n        (number "1" %s)\n      )\n'
        '    )\n'
        '    (symbol "PWR_FLAG_0_1"\n'
        '      (polyline (pts (xy 0 0) (xy 0 1.27) (xy -1.016 1.905) (xy 0 2.54)'
        ' (xy 1.016 1.905) (xy 0 1.27))\n'
        '        (stroke (width 0) (type default)) (fill (type none)))\n'
        '    )\n  )\n' % (FONT, FONT, FONT))

def build_library():
    parts = ['(kicad_symbol_lib (version 20220914) (generator mcupr_canteen)\n',
             esp32_symbol()]
    for m in MODULES:
        parts.append(module_symbol(m))
    parts.append(passive_symbol('R', 'R'))
    parts.append(passive_symbol('C', 'C'))
    parts.append(pwr_flag_symbol())
    parts.append(')\n')
    return ''.join(parts)


# ---------------------------------------------------------------------------
# สร้างไฟล์วงจร
# ทุกการเชื่อมต่อใช้ global label ไม่ได้ลากเส้นยาวข้ามหน้า
# อ่านง่ายกว่าและตรงกับงานเดินสายจริง คือดูว่าขาไหนอยู่เน็ตชื่ออะไร
# ---------------------------------------------------------------------------
class Sheet:
    def __init__(self, title, rev):
        self.items = []
        self.title = title
        self.rev = rev
        self.uuid = uid()
        self.refs = {}

    def next_ref(self, letter):
        self.refs[letter] = self.refs.get(letter, 0) + 1
        return '%s%d' % (letter, self.refs[letter])

    def wire(self, x1, y1, x2, y2):
        self.items.append('  (wire (pts (xy %.2f %.2f) (xy %.2f %.2f))\n'
                          '    (stroke (width 0) (type default)) (uuid %s)\n  )\n'
                          % (x1, y1, x2, y2, uid()))

    def label(self, net, x, y, angle):
        just = 'left' if angle == 0 else 'right'
        self.items.append(
            '  (global_label "%s" (shape bidirectional) (at %.2f %.2f %d) (fields_autoplaced)\n'
            '    (effects (font (size 1.27 1.27)) (justify %s)) (uuid %s)\n'
            '    (property "Intersheetrefs" "${INTERSHEET_REFS}" (at 0 0 0)\n'
            '      (effects (font (size 1.27 1.27)) hide))\n  )\n'
            % (net, x, y, angle, just, uid()))

    def no_connect(self, x, y):
        self.items.append('  (no_connect (at %.2f %.2f) (uuid %s))\n' % (x, y, uid()))

    def place(self, libname, ref, value, x, y, npins, mirror=False):
        u = uid()
        # ขอบของสัญลักษณ์ในระบบพิกัดของตัวมันเอง แกน Y ชี้ขึ้น
        # แต่ในไฟล์วงจร แกน Y ชี้ลง จึงต้องกลับเครื่องหมายตอนคำนวณตำแหน่งข้อความ
        top, bot = SYMBOL_BOX.get(libname, (2.54, -2.54))
        if libname in ('R', 'C', 'PWR_FLAG'):
            # ตัวเล็กและมีสายต่อทั้งบนและล่าง วางข้อความไว้ทางขวาแทน จะได้ไม่ทับป้ายเน็ต
            rx, ry = x + 3.81, y - 1.27
            vx, vy = x + 3.81, y + 1.27
        else:
            half_w = 12.7
            rx, ry = x - half_w, y - top - 2.54
            vx, vy = x - half_w, y - bot + 3.81
        pins = ''.join('    (pin "%d" (uuid %s))\n' % (i + 1, uid()) for i in range(npins))
        self.items.append(
            '  (symbol (lib_id "MCUPR_Canteen:%s") (at %.2f %.2f 0) (unit 1)\n'
            '    (in_bom yes) (on_board yes) (dnp no) (fields_autoplaced)\n'
            '    (uuid %s)\n'
            '    (property "Reference" "%s" (at %.2f %.2f 0) (effects (font (size 1.27 1.27)) (justify left)))\n'
            '    (property "Value" "%s" (at %.2f %.2f 0) (effects (font (size 1.27 1.27)) (justify left)))\n'
            '    (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))\n'
            '    (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))\n'
            '%s'
            '    (instances (project "%s" (path "/%s" (reference "%s") (unit 1))))\n'
            '  )\n' % (libname, x, y, u, ref, rx, ry, value, vx, vy,
                       pins, self.title, self.uuid, ref))
        return u

    def render(self):
        head = ('(kicad_sch (version 20230121) (generator mcupr_canteen)\n'
                '  (uuid %s)\n  (paper "A3")\n'
                '  (title_block (title "%s") (date "2026-09-24") (rev "%s")\n'
                '    (company "MCU Phrae Campus")\n'
                '    (comment 1 "Generated by hardware/kicad/generate_kicad.py")\n'
                '    (comment 2 "Pin map is read from the firmware source. Do not edit by hand.")\n'
                '    (comment 3 "Verify with: python3 tools/proto-check/kicadcheck.py")\n'
                '  )\n' % (self.uuid, self.title, self.rev))
        return head + lib_symbols_block() + ''.join(self.items) + \
               '  (sheet_instances (path "/" (page "1")))\n)\n'


def lib_symbols_block():
    """เอาเนื้อของไลบรารีมาใส่ในไฟล์วงจร โดยเติมชื่อไลบรารีนำหน้าทุกสัญลักษณ์"""
    body = build_library()
    body = body.split('\n', 1)[1].rsplit(')\n', 1)[0]
    body = re.sub(r'^  \(symbol "', '  (symbol "MCUPR_Canteen:', body, flags=re.M)
    return '  (lib_symbols\n' + body + '  )\n'


# ---------------------------------------------------------------------------
# ผังเน็ตของแต่ละบอร์ด — คีย์คือหมายเลขขาในสัญลักษณ์ของบอร์ด ESP32
# ---------------------------------------------------------------------------
def gpio_pin_no(name):
    """หาว่า GPIOn อยู่ขาที่เท่าไรของสัญลักษณ์"""
    tag = 'GPIO%d' % name
    if tag in LEFT:  return LEFT.index(tag) + 1
    if tag in RIGHT: return RIGHT.index(tag) + 23
    raise SystemExit('ไม่พบ %s บนบอร์ด' % tag)

def board_nets(pins, is_station):
    n = {
        gpio_pin_no(pins['TFT_BLK']):  'TFT_BLK',
        gpio_pin_no(pins['TFT_CS']):   'TFT_CS',
        gpio_pin_no(pins['TFT_DC']):   'TFT_DC',
        gpio_pin_no(pins['TFT_RST']):  'TFT_RES',
        gpio_pin_no(pins['TFT_MOSI']): 'TFT_SDA',
        gpio_pin_no(pins['TFT_SCLK']): 'TFT_SCL',
        gpio_pin_no(pins['BUZZER_PIN']):      'BUZZER',
        gpio_pin_no(pins['BTN_PIN']):         'BTN',
        gpio_pin_no(pins['BATTERY_ADC_PIN']): 'VBAT_SENSE',
        1: '+3V3', 2: '+3V3', 22: 'GND', 23: 'GND', 43: 'GND', 44: 'GND',
    }
    if is_station:
        n[gpio_pin_no(pins['RC522_SS'])]   = 'RC522_SS'
        n[gpio_pin_no(pins['RC522_SCK'])]  = 'RC522_SCK'
        n[gpio_pin_no(pins['RC522_MOSI'])] = 'RC522_MOSI'
        n[gpio_pin_no(pins['RC522_MISO'])] = 'RC522_MISO'
        n[gpio_pin_no(pins['RC522_RST'])]  = 'RC522_RST'
    else:
        n[gpio_pin_no(pins['I2C_SDA_PIN'])] = 'RTC_SDA'
        n[gpio_pin_no(pins['I2C_SCL_PIN'])] = 'RTC_SCL'
        # การ์ด SD ใช้บัส SPI ร่วมกับจอ เหลือสองเส้นที่มีขาของตัวเอง
        n[gpio_pin_no(pins['SD_MISO_PIN'])] = 'SD_MISO'
        n[gpio_pin_no(pins['SD_CS_PIN'])]   = 'SD_CS'
    return n


def build_sheet(title, pins, is_station):
    sh = Sheet(title, '1.0')
    nets = board_nets(pins, is_station)

    # --- บอร์ด ESP32 วางไว้ซ้ายมือ ---
    ux, uy = 76.2, 109.22
    sh.place('ESP32-S3-DevKitC-1', 'U1', 'ESP32-S3-DevKitC-1 N16R8', ux, uy, 44)
    half = 12.7 + 5.08
    for i in range(22):
        y = uy - 26.67 + i * MM
        num = i + 1
        px = ux - half
        if num in nets:
            sh.wire(px, y, px - MM, y)
            sh.label(nets[num], px - MM, y, 180)
        else:
            sh.no_connect(px, y)
    for i in range(22):
        y = uy - 26.67 + i * MM
        num = i + 23
        px = ux + half
        if num in nets:
            sh.wire(px, y, px + MM, y)
            sh.label(nets[num], px + MM, y, 0)
        else:
            sh.no_connect(px, y)

    # --- โมดูลทางขวา ---
    def module(libname, ref, value, x, y, wiring):
        pinnames = MODULES[libname]
        sh.place(libname, ref, value, x, y, len(pinnames))
        for i, pn in enumerate(pinnames):
            py = y - 0.0 + i * MM
            px = x - (17.78 / 2 + 5.08)
            net = wiring.get(pn)
            if net:
                sh.wire(px, py, px - MM, py)
                sh.label(net, px - MM, py, 180)
            else:
                sh.no_connect(px, py)

    module('ST7789_LCD', 'J2', 'ST7789 2.8in 320x240', 190.5, 55.88, {
        'GND': 'GND', 'VCC': '+3V3', 'SCL': 'TFT_SCL', 'SDA': 'TFT_SDA',
        'RES': 'TFT_RES', 'DC': 'TFT_DC', 'CS': 'TFT_CS', 'BLK': 'TFT_BLK'})

    if is_station:
        module('RC522_RFID', 'J3', 'MFRC522 13.56MHz', 190.5, 104.14, {
            'SDA': 'RC522_SS', 'SCK': 'RC522_SCK', 'MOSI': 'RC522_MOSI',
            'MISO': 'RC522_MISO', 'GND': 'GND', 'RST': 'RC522_RST', '3V3': '+3V3'})
    else:
        module('DS3231_RTC', 'J3', 'DS3231 ZS-042', 190.5, 104.14, {
            'SCL': 'RTC_SCL', 'SDA': 'RTC_SDA', 'VCC': '+3V3', 'GND': 'GND'})

    if not is_station:
        # การ์ด SD อยู่บนโมดูลจอเดียวกัน แต่เป็นหัวต่ออีกชุดหนึ่ง
        # SCK กับ MOSI ใช้เส้นเดียวกับจอ ส่วน MISO กับ CS มีขาของตัวเอง
        module('SD_SLOT', 'J5', 'microSD on LCD module', 284.48, 175.26, {
            'SD_SCK': 'TFT_SCL', 'SD_MISO': 'SD_MISO',
            'SD_MOSI': 'TFT_SDA', 'SD_CS': 'SD_CS'})

    module('BUZZER', 'BZ1', 'Passive buzzer', 190.5, 147.32,
           {'+': 'BUZZER', '-': 'GND'})
    module('SW_PUSH', 'SW1', 'Tactile switch', 190.5, 165.1,
           {'1': 'BTN', '2': 'GND'})
    module('CONN_BATT', 'J4', 'Battery 1S Li-ion', 190.5, 182.88,
           {'+': 'VBAT', '-': 'GND'})

    # --- วงจรแบ่งแรงดันวัดแบตเตอรี่ 1:2 ---
    def passive(lib, letter, value, x, y, top_net, bot_net):
        # หักเส้นออกทางซ้ายก่อนติดป้าย ป้ายจะได้เป็นแนวนอนทั้งหมด อ่านง่ายกว่าป้ายตั้ง
        ref = sh.next_ref(letter)
        sh.place(lib, ref, value, x, y, 2)
        sh.wire(x, y - 5.08, x, y - 7.62)
        sh.wire(x, y - 7.62, x - 10.16, y - 7.62)
        sh.label(top_net, x - 10.16, y - 7.62, 180)
        sh.wire(x, y + 5.08, x, y + 7.62)
        sh.wire(x, y + 7.62, x - 10.16, y + 7.62)
        sh.label(bot_net, x - 10.16, y + 7.62, 180)

    passive('R', 'R', '100k 1%', 284.48, 66.04,  'VBAT', 'VBAT_SENSE')
    passive('R', 'R', '100k 1%', 284.48, 91.44, 'VBAT_SENSE', 'GND')
    passive('C', 'C', '100nF',   330.2, 91.44, 'VBAT_SENSE', 'GND')

    # --- ตัวเก็บประจุกรองไฟของแต่ละโมดูล ---
    passive('C', 'C', '100nF', 284.48, 129.54, '+3V3', 'GND')
    passive('C', 'C', '100nF', 330.2, 129.54, '+3V3', 'GND')

    # --- ธงบอกว่าเน็ต GND มีแหล่งจ่ายจริง ---
    ref = sh.next_ref('#FLG')
    sh.place('PWR_FLAG', ref, 'PWR_FLAG', 53.34, 172.72, 1)
    sh.wire(53.34, 172.72, 53.34, 177.8)
    sh.wire(53.34, 177.8, 43.18, 177.8)
    sh.label('GND', 43.18, 177.8, 180)
    return sh

def supply_x():
    return 63.5


PROJECT = '''{
  "board": {"design_settings": {}},
  "boards": [],
  "cvpcb": {"equivalence_files": []},
  "libraries": {"pinned_footprint_libs": [], "pinned_symbol_libs": []},
  "meta": {"filename": "%s.kicad_pro", "version": 1},
  "net_settings": {"classes": [{"name": "Default", "clearance": 0.2,
    "track_width": 0.25, "via_diameter": 0.8, "via_drill": 0.4}]},
  "pcbnew": {"page_layout_descr_file": ""},
  "schematic": {"legacy_lib_dir": "", "legacy_lib_list": []},
  "sheets": [["%s", ""]],
  "text_variables": {}
}
'''

def write(path, text):
    io.open(path, 'w', encoding='utf-8', newline='\n').write(text)
    print('เขียน %-46s %6d ไบต์' % (os.path.relpath(path, ROOT), len(text)))

def main():
    write(os.path.join(HERE, 'MCUPR_Canteen.kicad_sym'), build_library())
    for title, pins, station in [('Canteen_Host', HOST_PINS, False),
                                 ('Canteen_Station', STN_PINS, True)]:
        sh = build_sheet(title, pins, station)
        write(os.path.join(HERE, title + '.kicad_sch'), sh.render())
        write(os.path.join(HERE, title + '.kicad_pro'), PROJECT % (title, sh.uuid))

if __name__ == '__main__':
    main()
