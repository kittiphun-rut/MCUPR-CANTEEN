# -*- coding: utf-8 -*-
"""ตรวจการจัดขาของทั้งสองบอร์ด ไม่ให้ชนกันเองและไม่ไปโดนขาที่ห้ามใช้

ESP32-S3 DevKitC-1 รุ่น N16R8 มีขาที่ห้ามเอาไปต่ออุปกรณ์ เพราะถูกจองไว้แล้ว
ถ้าเผลอใช้ **โค้ดคอมไพล์ผ่านและบูตได้ แต่จะพังแบบหาสาเหตุยาก** เช่น
บูตไม่ขึ้นเป็นบางครั้ง แฟลชไม่ได้ หรือหน่วยความจำ PSRAM เสียหาย

ตรวจสามอย่าง
  1. ไม่มีขาไหนถูกใช้ซ้ำสองหน้าที่ในบอร์ดเดียวกัน
  2. ไม่มีขาไหนไปโดนรายการต้องห้าม
  3. ขาที่ใช้วัดแรงดันแบตเตอรี่ต้องอยู่ใน ADC1 เท่านั้น
     ADC2 อ่านไม่ได้ขณะเปิด Wi-Fi ซึ่งเครื่องนี้เปิดตลอดเวลา

ใช้:  python3 tools/proto-check/pincheck.py
"""
import io, os, re, sys

BOARDS = [
    ('Canteen_Host_Server/Canteen_Host_Server.ino', 'เครื่องแม่ข่าย'),
    ('Canteen_Station_Client/Canteen_Station_Client.ino', 'จุดบริการ'),
]

# ขาที่ห้ามต่ออุปกรณ์ พร้อมเหตุผล
FORBIDDEN = {}
for n in range(26, 33): FORBIDDEN[n] = 'ขาของ SPI flash บนโมดูล'
for n in range(33, 38): FORBIDDEN[n] = 'ขาของ PSRAM แบบ OPI บนรุ่น N16R8'
FORBIDDEN[19] = 'USB D-  ใช้แล้วพอร์ต USB ของบอร์ดใช้ไม่ได้'
FORBIDDEN[20] = 'USB D+  ใช้แล้วพอร์ต USB ของบอร์ดใช้ไม่ได้'
FORBIDDEN[43] = 'U0TXD ของ Serial Monitor'
FORBIDDEN[44] = 'U0RXD ของ Serial Monitor'
FORBIDDEN[0]  = 'ขา strapping และปุ่ม BOOT'
FORBIDDEN[3]  = 'ขา strapping (เลือกแหล่ง JTAG)'
FORBIDDEN[45] = 'ขา strapping (แรงดัน VDD_SPI)'
FORBIDDEN[46] = 'ขา strapping (เปิดปิดข้อความจาก ROM)'

# ขาที่อ่านค่าแอนะล็อกได้ขณะเปิด Wi-Fi
ADC1 = set(range(1, 11))

# หน้าที่ประจำของแต่ละขาบน ESP32-S3 และสิ่งที่ต้องรู้ก่อนเอาไปใช้
# ใช้พิมพ์เป็นตารางตรวจสอบ จะได้ไม่ต้องเชื่อคำยืนยันลอย ๆ ว่า "ปลอดภัยแล้ว"
ROLE = {
    1:  'ADC1_CH0 · TOUCH1 · RTC',
    2:  'ADC1_CH1 · TOUCH2 · RTC',
    4:  'ADC1_CH3 · TOUCH4 · RTC',
    5:  'ADC1_CH4 · TOUCH5 · RTC',
    6:  'ADC1_CH5 · TOUCH6 · RTC',
    7:  'ADC1_CH6 · TOUCH7 · RTC',
    8:  'ADC1_CH7 · TOUCH8 · RTC',
    9:  'ADC1_CH8 · TOUCH9 · FSPIHD',
    10: 'ADC1_CH9 · TOUCH10 · FSPICS0',
    11: 'ADC2_CH0 · TOUCH11 · FSPID',
    12: 'ADC2_CH1 · TOUCH12 · FSPICLK',
    13: 'ADC2_CH2 · TOUCH13 · FSPIQ',
    14: 'ADC2_CH3 · TOUCH14 · FSPIWP',
    15: 'ADC2_CH4 · U0RTS · XTAL_32K_P',
    16: 'ADC2_CH5 · U0CTS · XTAL_32K_N',
    17: 'ADC2_CH6 · U1TXD',
    18: 'ADC2_CH7 · U1RXD',
    21: 'RTC',
    47: 'SPICLK_P',
    48: 'SPICLK_N · ไฟ WS2812 บนบอร์ด (รุ่น v1.1)',
}

# ขาที่ใช้ได้ แต่มีเงื่อนไขที่ต้องรู้ ไม่ถึงกับห้าม
CAUTION = {
    15: 'เป็นขาคริสตัล 32.768 kHz ถ้าบอร์ดรุ่นที่ใช้ติดคริสตัลมาจะใช้ไม่ได้ '
        'DevKitC-1 มาตรฐานไม่ติดมา',
    16: 'เป็นขาคริสตัล 32.768 kHz เงื่อนไขเดียวกับ GPIO15',
    38: 'บางรุ่นใช้เป็นไฟ WS2812 บนบอร์ด (v1.0)',
    39: 'ขา JTAG ภายนอก (MTCK) ใช้เป็น GPIO ได้ แต่ดีบักด้วย JTAG ภายนอกไม่ได้',
    40: 'ขา JTAG ภายนอก (MTDO) เงื่อนไขเดียวกับ GPIO39',
    41: 'ขา JTAG ภายนอก (MTDI) เงื่อนไขเดียวกับ GPIO39',
    42: 'ขา JTAG ภายนอก (MTMS) เงื่อนไขเดียวกับ GPIO39',
    48: 'ไฟ WS2812 บนบอร์ด รุ่น v1.0 อยู่ที่ GPIO38 แทน',
}

# ขาบนบอร์ดที่มีอยู่จริง นอกจากนี้ถือว่าพิมพ์ผิด
EXISTS = set(range(0, 22)) | set(range(35, 49))

NAME = re.compile(r'^#define\s+((?:TFT_|RC522_|SD_|BUZZER|BTN|BATTERY|RGB_LED|I2C_)\w*)\s+(\d+)', re.M)

bad = 0
for path, who in BOARDS:
    if not os.path.exists(path):
        continue
    src = io.open(path, encoding='utf-8').read()
    pins, problems = {}, []
    for m in NAME.finditer(src):
        name, pin = m.group(1), int(m.group(2))
        if pin in pins:
            problems.append('ขา GPIO%d ถูกใช้ซ้ำ ทั้ง %s และ %s' % (pin, pins[pin], name))
        else:
            pins[pin] = name
        if pin in FORBIDDEN:
            problems.append('%s ใช้ GPIO%d ซึ่งห้ามใช้ เพราะเป็น%s' % (name, pin, FORBIDDEN[pin]))
        if pin not in EXISTS:
            problems.append('%s ใช้ GPIO%d ซึ่งไม่มีอยู่บนบอร์ดรุ่นนี้' % (name, pin))
        if 'BATTERY' in name and pin not in ADC1:
            problems.append('%s ใช้ GPIO%d ซึ่งไม่ได้อยู่ใน ADC1 '
                            'อ่านค่าไม่ได้ขณะเปิด Wi-Fi' % (name, pin))

    print('%-34s %-14s %d ขา  %s' % (os.path.basename(path), who, len(pins),
                              'OK' if not problems else '*** พบ %d ปัญหา ***' % len(problems)))
    for pin in sorted(pins):
        note = CAUTION.get(pin)
        print('      GPIO%-3d %-20s %-34s%s' % (
            pin, pins[pin], ROLE.get(pin, '-'), ('  << ' + note) if note else ''))
    for line in problems:
        print('      ' + line)
    bad += len(problems)

sys.exit(1 if bad else 0)
