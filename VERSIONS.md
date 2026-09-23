# สารบัญเวอร์ชัน (Release Index)

ทุกจุดที่เลขเวอร์ชันของฝั่งใดฝั่งหนึ่งเปลี่ยน ถูกชี้ตำแหน่งไว้ที่นี่
เพื่อให้ **ย้อนกลับไปใช้ชุดที่สมบูรณ์ชุดไหนก็ได้** โดยไม่ต้องไล่หาคอมมิตเอง

หนึ่งรุ่นคือ **เฟิร์มแวร์ครบทั้งคู่** ทั้งฝั่งแม่ข่ายและฝั่งสถานี
ที่ทดสอบและจัดทำเอกสารมาด้วยกัน — ไม่ควรจับคู่ข้ามรุ่น เพราะโปรโตคอล
ESP-NOW ระหว่างสองฝั่งเปลี่ยนไปในบางรุ่น

ทุกรุ่นถูก push ขึ้น GitHub ไว้เป็น **branch** ใต้โฟลเดอร์ `release/`
กดโหลดเป็นไฟล์ zip จากตารางข้างล่างได้เลย ไม่ต้องใช้คำสั่ง git

### โครงสร้าง branch ของ repository

```
main                              branch หลัก งานล่าสุดทั้งหมดอยู่ที่นี่
claude/fervent-wozniak-66knt2     branch ที่ Claude ใช้พัฒนา ชี้คอมมิตเดียวกับ main
release/00-h107.0.1-s117.0.7      หมุดเวลาของแต่ละรุ่น ห้ามแก้ ห้าม push ทับ
release/01-h107.0.1-s118.0.0
release/02-h108.0.0-s118.0.0
release/03-h108.0.0-s118.1.0
release/04-h108.0.0-s118.2.0
release/05-h108.0.0-s119.0.0
release/06-h109.0.0-s119.0.0
release/07-h110.0.1-s120.0.0
release/08-h110.0.2-s120.0.1
release/09-h111.0.0-s120.0.1
release/10-h111.1.0-s120.0.1
release/11-h112.0.0-s121.0.0
release/12-h113.0.0-s122.0.0
release/13-h113.1.0-s122.0.0
release/14-h113.1.0-s122.1.0
release/15-h113.1.0-s122.1.0-docs
release/16-h113.2.0-s122.2.0
release/17-h113.3.0-s122.3.0
release/18-h113.4.0-s122.4.0
release/19-h113.5.0-s122.5.0
release/20-h113.6.0-s122.5.0
release/21-h113.6.0-s122.6.0
release/22-h113.6.0-s122.6.1
release/23-h113.6.0-s122.6.2
release/24-h113.7.0-s122.7.0
```

> หมายเหตุ: ปกติงานลักษณะนี้ควรเป็น **tag** ไม่ใช่ branch แต่บัญชีที่เซสชันนี้ใช้
> push `refs/tags/*` ไม่ได้ เซิร์ฟเวอร์ตอบ 403 ทุกครั้ง จึงใช้ branch แทน
> ได้ผลเหมือนกันทุกอย่างสำหรับการย้อนกลับ
> ถ้าอยากได้ tag จริงด้วย รันคำสั่งในหัวข้อ **สร้างแท็กเอง** ครั้งเดียวจบ
>
> **branch เหล่านี้ห้ามแก้และห้าม push ทับ** ให้ถือเป็นหมุดเวลาอย่างเดียว
> งานที่พัฒนาต่ออยู่ที่ `main`

| ชื่อรุ่น | Host | Station | คอมไพล์ | โหลด zip | คอมมิต | สาระสำคัญ |
|---|---|---|---|---|---|---|
| `release/00-h107.0.1-s117.0.7` | 107.0.1 | 117.0.7 | ✅ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/00-h107.0.1-s117.0.7.zip) | [`b351769`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/b3517697c239e72ce180c0759843973b8903d7bf) | **ต้นฉบับตามที่ส่งมา** ยังไม่แก้อะไรเลย เก็บสำเนาไว้ที่ [`original/`](original/) ด้วย<br><sub>21 ก.ย. 2026</sub> |
| `release/01-h107.0.1-s118.0.0` | 107.0.1 | 118.0.0 | ✅ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/01-h107.0.1-s118.0.0.zip) | [`8a20d11`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/8a20d11c8268e7102bd789f66c212bea5e83f9fc) | ฝั่งสถานี: แยกบัส SPI ของจอกับเครื่องอ่านบัตร (จอเพี้ยนหลัง `PCD_Init()`), ซ่อมสถานะลิงก์สด, รับคำสั่งสลับธีมจากแม่ข่าย<br><sub>21 ก.ย. 2026</sub> |
| `release/02-h108.0.0-s118.0.0` | 108.0.0 | 118.0.0 | ✅ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/02-h108.0.0-s118.0.0.zip) | [`eec7139`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/eec713955d9c3fd56498f801d28f0bdd6d0f4662) | ฝั่งแม่ข่าย: ปิดช่องโหว่ความปลอดภัย, ซ่อม CSV และเลขอ้างอิง, ยกเครื่องเว็บพอร์ทัลให้อัปเดตสด, แยกหน้าเว็บออกเป็น `WebPortal.h`, เพิ่มจอสาธารณะ `/display`<br><sub>21 ก.ย. 2026</sub> |
| `release/03-h108.0.0-s118.1.0` | 108.0.0 | 118.1.0 | ❌ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/03-h108.0.0-s118.1.0.zip) | [`a883805`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/a883805c9283a80f4240d1d90b7a971217f1a5b5) | ซิงค์โหมดการแสดงผลจากแม่ข่ายไปทุกสถานี, แก้พอร์ทัลใช้งานไม่ได้ในหน้าต่าง captive portal, เพิ่มสัญลักษณ์ RFID บนหน้าแรกของสถานี<br><sub>21 ก.ย. 2026</sub> |
| `release/04-h108.0.0-s118.2.0` | 108.0.0 | 118.2.0 | ❌ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/04-h108.0.0-s118.2.0.zip) | [`1ca1c8a`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/1ca1c8ac3f8d21df85a88f87a076334e787bf659) | เติมข้อมูลหน้า 2 ของสถานีให้เต็มจอและเป็นข้อมูลสด (ยอดทั้งโรงอาหาร + รายการที่จ่ายล่าสุด)<br><sub>21 ก.ย. 2026</sub> |
| `release/05-h108.0.0-s119.0.0` | 108.0.0 | 119.0.0 | ❌ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/05-h108.0.0-s119.0.0.zip) | [`6620ab5`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/6620ab5382a80fa3a9519bbb75f90561896788f5) | เสริมความทนทานของลิงก์สำหรับระยะ 20 เมตร และเพิ่ม **คิวออฟไลน์** ฝั่งสถานี<br><sub>21 ก.ย. 2026</sub> |
| `release/06-h109.0.0-s119.0.0` | 109.0.0 | 119.0.0 | ❌ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/06-h109.0.0-s119.0.0.zip) | [`c3584ff`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/c3584ffe8522ec5314dd960944e6ae16a2df569a) | เพิ่ม **เครื่องพิมพ์สลิปความร้อน 58 มม.** (`ThermalPrinter.h`) ผ่าน USB OTG หรือ UART<br><sub>21 ก.ย. 2026</sub> |
| `release/07-h110.0.1-s120.0.0` | 110.0.1 | 120.0.0 | ❌ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/07-h110.0.1-s120.0.0.zip) | [`7fd0e20`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/7fd0e20) | **ตรวจสิทธิ์ได้เองตอนลิงก์ขาด** ด้วยบัญชีสิทธิ์ย่อที่แม่ข่ายผลักไปเก็บที่สถานี<br><sub>22 ก.ย. 2026</sub> |
| `release/08-h110.0.2-s120.0.1` | 110.0.2 | 120.0.1 | ✅ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/08-h110.0.2-s120.0.1.zip) | [`b7dfc73`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/b7dfc734119d9352e1c4e2de58a9e684670a343c) | **แก้ให้คอมไพล์ผ่าน Arduino IDE** — เพิ่ม forward declaration ที่ขาดสองจุด ฟีเจอร์เท่ากับ `release/07` ทุกอย่าง<br><sub>22 ก.ย. 2026</sub> |
| `release/09-h111.0.0-s120.0.1` | 111.0.0 | 120.0.1 | ✅ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/09-h111.0.0-s120.0.1.zip) | [`4ade160`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/4ade160790e7f5dd9c22b044dd1ec9f31f60fb70) | **ปิดเครื่องพิมพ์สลิปเป็นค่าเริ่มต้น** เพื่อลดขนาดเฟิร์มแวร์ ฟีเจอร์เดิมคงไว้ครบทุกอย่าง เปิดกลับได้ด้วยการแก้ `#define` บรรทัดเดียว<br><sub>22 ก.ย. 2026</sub> |
| `release/10-h111.1.0-s120.0.1` | 111.1.0 | 120.0.1 | ✅ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/10-h111.1.0-s120.0.1.zip) | [ล่าสุดของ branch นี้](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/release/10-h111.1.0-s120.0.1) | **แยกไฟล์หลักให้เหลือ 2,674 บรรทัด** ย้ายส่วนวาดจอไป `HostDisplay.h` และเนื้อหาสลิปไป `HostSlips.h` เป็นการย้ายที่อยู่ล้วน ๆ ตรรกะไม่เปลี่ยน<br><sub>22 ก.ย. 2026</sub> |

| `release/11-h112.0.0-s121.0.0` | 112.0.0 | 121.0.0 | ✅ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/11-h112.0.0-s121.0.0.zip) | [`427e5d4`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/427e5d499f3dde2921b1fac6c9f8b9882c37a870) | **แก้ปัญหาชื่อ Wi-Fi ไม่โผล่มา** ปิด `WIFI_PROTOCOL_LR` เป็นค่าเริ่มต้น ย้าย `printerBegin()` ไปหลังเว็บเซิร์ฟเวอร์ และเพิ่มจอแจ้งสถานะเครือข่ายตอนบูต<br><sub>22 ก.ย. 2026</sub> |
| `release/12-h113.0.0-s122.0.0` | 113.0.0 | 122.0.0 | ❔ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/12-h113.0.0-s122.0.0.zip) | [`7f4fed9`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/7f4fed947d667942dbf3e8ceccf1079165cfda96) | **เริ่มใหม่จากต้นฉบับ 107.0.1/117.0.7** แล้วรื้ออินเตอร์เฟสทั้งหมดเป็นภาษาอังกฤษสองโหมด แยกไฟล์แดชบอร์ดและไฟล์วาดจอออกมา แม่ข่ายคุมการแสดงผลของทุกจุดบริการ **ยังไม่มีเครื่องพิมพ์สลิป คิวออฟไลน์ และจอสาธารณะ**<br><sub>22 ก.ย. 2026</sub> |

| `release/13-h113.1.0-s122.0.0` | 113.1.0 | 122.0.0 | ❔ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/13-h113.1.0-s122.0.0.zip) | [`b18a20c`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/b18a20cb0929869e58c711e713fab3f6268c5a3f) | **หน้าเว็บกลับมาเป็นสองภาษา ไทย/อังกฤษ** คำไทยอยู่ในพจนานุกรมชุดเดียว ไม่ได้เขียนซ้ำสองชุดทุกบรรทัดแบบรุ่นเก่า · ฝั่งจอ TFT ยังเป็นอังกฤษ · เพิ่มตัวตรวจคำพูดไม่ครบคู่ใน bracecheck<br><sub>22 ก.ย. 2026</sub> |

| `release/14-h113.1.0-s122.1.0` | 113.1.0 | 122.1.0 | ❔ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/14-h113.1.0-s122.1.0.zip) | [`0a4bd73`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/0a4bd7397e8102e8c929b02df0e71965e987ec8a) | **แยกส่วนวาดจอของฝั่งจุดบริการออกเป็น `StationScreen.h`** ไฟล์หลักเหลือ 1,141 บรรทัด ย้ายแบบยกก้อน ไม่แก้เนื้อใน · เพิ่มเครื่องมือ `splitcheck.py` ตรวจปัญหาจากการแยกไฟล์<br><sub>22 ก.ย. 2026</sub> |

| `release/15-h113.1.0-s122.1.0-docs` | 113.1.0 | 122.1.0 | ❔ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/15-h113.1.0-s122.1.0-docs.zip) | [`3d8a123`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/3d8a12324dd3261527af9bf94af9fc76b7b5620e) | **มาตรฐานคอมเมนต์แบบ Doxygen** หัวไฟล์มีตารางประวัติการแก้ไข และป้าย `// [เวอร์ชัน] เพิ่ม/แก้/ย้าย` ในเนื้อโค้ด · เพิ่ม `headercheck.py` บังคับใช้ · **แตะแต่คอมเมนต์ ตรรกะเหมือน `release/14` ทุกบรรทัด**<br><sub>22 ก.ย. 2026</sub> |

| `release/16-h113.2.0-s122.2.0` | 113.2.0 | 122.2.0 | ❔ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/16-h113.2.0-s122.2.0.zip) | [`beda1b4`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/beda1b4c5d6f682d200c53eec77cb17fd8cb0119) | **ซ่อมการรีเฟรชตัวเลขบนจอทั้งสองฝั่ง** ยอดรายร้าน หน้า SYSTEM นาฬิกาของจุดบริการ และแถบสัญญาณที่ไม่เคยขึ้นมาเลย · วาดซ้ำเฉพาะช่องที่เปลี่ยน จอไม่กะพริบ · เพิ่ม `tools/screen-preview/`<br><sub>22 ก.ย. 2026</sub> |

| `release/17-h113.3.0-s122.3.0` | 113.3.0 | 122.3.0 | ❔ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/17-h113.3.0-s122.3.0.zip) | [`38eb7ee`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/38eb7eefd48ba438e35ab521dffe0f8c704c6bac) | **แก้จอกะพริบทุกวินาที** ต้นเหตุคือการล้างพื้นก่อนเขียนตัวอักษร เปลี่ยนมาเขียนทับที่เดิมด้วย `drawFixedText()` · แก้สีค้างที่มุมโค้งของแถบความคืบหน้า<br><sub>22 ก.ย. 2026</sub> |

| `release/18-h113.4.0-s122.4.0` | 113.4.0 | 122.4.0 | ❔ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/18-h113.4.0-s122.4.0.zip) | [`712f5c4`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/712f5c4da8ebe1fc0d6ab977c637ae73547c529e) | **แก้อักษรซ้อนและภาษาไทยอ่านไม่ออกบนจอ** ต้นตอคือค่าเริ่มต้นของชื่อร้านเป็นภาษาไทยแล้วถูกวาดลงจอตรง ๆ · เพิ่มช่องตั้งชื่อร้านสำหรับจอ และตัวกรอง `asciiOnly()`<br><sub>23 ก.ย. 2026</sub> |

| `release/19-h113.5.0-s122.5.0` | 113.5.0 | 122.5.0 | ❔ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/19-h113.5.0-s122.5.0.zip) | [`24e4b9c`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/24e4b9cdd50e07890d8c8f54417139125dafc6a5) | **จัดตำแหน่งตัวอักษรบนจอ** หน้าพักจอจัดกึ่งกลางทุกบรรทัด · แก้หน่วยเงินที่เหลือช่องว่างกลางอากาศ · กระจายแถวหน้า SYSTEM ของจุดบริการให้เต็มการ์ด<br><sub>23 ก.ย. 2026</sub> |

| `release/20-h113.6.0-s122.5.0` | 113.6.0 | 122.5.0 | ❔ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/20-h113.6.0-s122.5.0.zip) | [`9e19814`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/9e198141987be7143dbb6ff61b74772c98e93683) | **จอสาธารณะบนทีวีในโรงอาหาร** หน้า `/display` เปิดดูได้โดยไม่ต้องเข้าสู่ระบบ · มี `/api/board` เป็นของตัวเองที่ปิดบังรหัสนิสิตเหลือห้าหลักแรก ไม่มีชื่อ ไม่มีหมายเลขบัตร ไม่มีเลขอ้างอิง และไม่มีข้อมูลฮาร์ดแวร์ · เพิ่มตัวตรวจ `privacycheck.py` บังคับข้อกำหนดนี้<br><sub>23 ก.ย. 2026</sub> |

| `release/21-h113.6.0-s122.6.0` | 113.6.0 | 122.6.0 | ❌ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/21-h113.6.0-s122.6.0.zip) | [`9d86481`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/9d8648129de47011b51e201491fceee9b1dbcdd3) | **คืนสัญลักษณ์แตะบัตร RFID บนหน้าแรกของจุดบริการ** ที่หายไปตอนเริ่มใหม่ในรุ่น 122.0.0 · ต่อเข้ากับตัวตรวจสุขภาพ RC522 คลื่นเขียวคือเครื่องอ่านพร้อม แดงคือไม่ตอบพร้อมบรรทัดให้ตามเจ้าหน้าที่<br><sub>23 ก.ย. 2026</sub> |

| `release/22-h113.6.0-s122.6.1` | 113.6.0 | 122.6.1 | ✅ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/22-h113.6.0-s122.6.1.zip) | [`f35e8b0`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/f35e8b0ff8f17d550c5f81b5c99a4fe589ac47bd) | **แก้ `release/21` ที่คอมไพล์ไม่ผ่าน** `drawStandbyReadyState()` ถูกวางไว้ต่ำกว่าจุดที่เรียก · เพิ่มตัวตรวจ `ordercheck.py` จับฟังก์ชันที่ถูกเรียกก่อนถูกประกาศ ซึ่งตัวตรวจเดิมทั้งหกตัวมองไม่เห็น<br><sub>23 ก.ย. 2026</sub> |

| `release/23-h113.6.0-s122.6.2` | 113.6.0 | 122.6.2 | ✅ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/23-h113.6.0-s122.6.2.zip) | [`51bb316`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/51bb31635c3761aab7b672d4b4348804ca87b7c9) | **แก้คลื่นของสัญลักษณ์แตะบัตรที่ไม่ขึ้นบนจอจริง** `drawCircleHelper()` ไม่เปิดทรานแซกชัน SPI เอง · เปลี่ยนมาวาดครึ่งวงกลมเองด้วย `drawPixel()` · เพิ่มตัวตรวจ `gfxcheck.py` และแก้โปรแกรมจำลองที่ปิดบังบั๊กนี้<br><sub>23 ก.ย. 2026</sub> |

| `release/24-h113.7.0-s122.7.0` | 113.7.0 | 122.7.0 | ❔ | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release/24-h113.7.0-s122.7.0.zip) | [`7fc1ebf`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/7fc1ebf19da60b6e5d1401faa4aa56c8d14b3b85) | **คำสั่งเปลี่ยนโหมดการแสดงผลไปถึงจุดบริการทันที** เดิมต้องรอรอบ heartbeat ราวห้าวินาที · ส่งยูนิแคสต์ก่อนแล้วค่อยกระจายเสียง · แก้แพ็กเก็ตกระจายเสียงที่ใส่ฟิลด์ไม่ครบ · สั่งเปลี่ยนธีมตอนพักหน้าจอไม่เตะออกจากหน้าพักจออีก<br><sub>23 ก.ย. 2026</sub> |

> ❔ **`release/12` ถึง `release/24` ยังไม่ผ่านการคอมไพล์จริง**
> (`release/18` ท่านแฟลชแล้วและยืนยันว่าอักษรซ้อนหาย การกะพริบดีขึ้น แต่ยังไม่ได้แจ้งผลการคอมไพล์) เพราะเครื่องที่ใช้พัฒนาเข้า
> `downloads.arduino.cc` ไม่ได้ ตรวจมาแล้วเท่าที่ตรวจได้: วงเล็บครบทุกไฟล์
> การประกาศฟังก์ชันล่วงหน้าครบ ไวยากรณ์ JavaScript ที่ฝังอยู่ผ่าน `node --check`
> และทุกหน้าจอถูกเรนเดอร์ดูแล้วว่าตัวอักษรไม่ล้นกรอบ
> **รบกวนท่านคอมไพล์แล้วแจ้งผลกลับมา** ถ้าผ่านจะได้แก้ช่องนี้เป็น ✅

> ⚠️ **`release/22` คอมไพล์ผ่านแล้ว แต่คลื่นของสัญลักษณ์แตะบัตรไม่ขึ้นบนจอ**
> `drawCircleHelper()` ของไลบรารีไม่เปิดทรานแซกชัน SPI เอง จอจึงไม่ได้รับข้อมูล
> ใช้ **`release/23`** แทน ซึ่งมีฟีเจอร์เท่ากันทุกอย่าง

> ⚠️ **`release/21` คอมไพล์ไม่ผ่าน** `drawStandbyReadyState()` ถูกวางไว้ต่ำกว่า
> จุดที่เรียกใน `StationScreen.h` ขึ้นข้อความว่า `was not declared in this scope`
> ใช้ **`release/22`** แทน ซึ่งมีฟีเจอร์เท่ากันทุกอย่าง

> ⚠️ **`release/03` ถึง `release/07` คอมไพล์ไม่ผ่าน** ด้วย Arduino IDE
> ติดปัญหา forward declaration ที่หายไป (`fillStationConfig` และ `rosterApplyPacket`)
> ขึ้นข้อความว่า `variable or field 'fillStationConfig' declared void`
> ถ้าต้องการรุ่นล่าสุดที่ใช้งานได้จริง ให้ใช้ **`release/08`**
> ถ้าจำเป็นต้องย้อนไปรุ่นใดรุ่นหนึ่งในช่วงนั้นจริง ๆ แก้ได้เองด้วยการเพิ่มบรรทัดเดียว
> ในบล็อก *Forward Declarations* ของไฟล์นั้น รายละเอียดอยู่ใน
> [`tools/proto-check/`](tools/proto-check/)

รายละเอียดของแต่ละรุ่นอยู่ใน [`CHANGELOG.md`](CHANGELOG.md)

## หมายเหตุเรื่องเลขเวอร์ชันฝั่งแม่ข่ายที่ค้าง

ระหว่าง `release/02` ถึง `release/05` ฝั่งแม่ข่ายมีการแก้ไขจริงหลายรอบ
(จอสาธารณะ, ซิงค์โหมดการแสดงผล, แก้ captive portal) แต่ `APP_VERSION`
ในโค้ดถูกลืมเลื่อน ค้างอยู่ที่ `108.0.0` ตลอด ขณะที่ CHANGELOG ระบุเป็น
108.1.0 / 108.2.0 / 108.3.0 ไปแล้ว เรื่องนี้ถูกแก้ตอน `release/06`
ด้วยการกระโดดไป `109.0.0` ทีเดียว
**ชื่อรุ่นในตารางข้างบนใช้เลขที่อยู่ในโค้ดจริง** ไม่ใช่เลขใน CHANGELOG
ตรวจแล้วทีละรุ่นด้วยการอ่าน `APP_VERSION` ออกมาจากโค้ดของคอมมิตนั้นจริง ๆ

## วิธีย้อนกลับไปใช้เวอร์ชันเก่า

### ไม่ใช้ git เลย

กด **zip** ในตารางข้างบน ได้ไฟล์ที่มีทั้งสองสเก็ตช์ครบชุดพร้อมอัปโหลดทันที

### ดูว่ารุ่นนั้นมีอะไรบ้าง

```sh
git fetch origin
git show origin/release/05-h108.0.0-s119.0.0 --stat
```

### ดึงโค้ดของรุ่นนั้นมาดูโดยไม่ยุ่งกับงานปัจจุบัน

```sh
git switch --detach origin/release/05-h108.0.0-s119.0.0
```

กลับมางานล่าสุดด้วย `git switch main`

### ดึงเฉพาะไฟล์เดียวจากรุ่นเก่ามาทับของปัจจุบัน

```sh
git checkout origin/release/05-h108.0.0-s119.0.0 -- Canteen_Station_Client/
```

### ล้าง branch ชุดเก่า

รอบก่อนใช้ชื่อแบน ๆ `release-00-...` ตอนนี้ย้ายมาอยู่ใต้โฟลเดอร์ `release/` แล้ว
และ branch หลักเดิมชื่อ `fervent-wozniak-66knt2` (ชื่อสุ่มจากเซสชัน) ถูกแทนด้วย `main`
ของเก่าทั้งหมดชี้คอมมิตเดียวกับของใหม่ จึงลบทิ้งได้โดยไม่เสียอะไรเลย

บัญชีที่เซสชันนี้ใช้ **ลบ branch ไม่ได้** (เซิร์ฟเวอร์ตอบ 403 เช่นเดียวกับ tag)
ทำสองขั้นนี้เองครับ

**ขั้นที่ 1** เปลี่ยน branch หลักเป็น `main` ที่หน้า
<https://github.com/kittiphun-rut/MCUPR-CANTEEN/settings/branches>
กดปุ่มสลับตรง *Default branch* แล้วเลือก `main`

**ขั้นที่ 2** รันคำสั่งนี้เพื่อลบของเก่าทิ้ง

```sh
git push origin --delete \
  release-00-h107.0.1-s117.0.7 \
  release-01-h107.0.1-s118.0.0 \
  release-02-h108.0.0-s118.0.0 \
  release-03-h108.0.0-s118.1.0 \
  release-04-h108.0.0-s118.2.0 \
  release-05-h108.0.0-s119.0.0 \
  release-06-h109.0.0-s119.0.0 \
  release-07-h110.0.1-s120.0.0 \
  fervent-wozniak-66knt2
```

> ต้องทำขั้นที่ 1 ก่อนเสมอ เพราะ GitHub ไม่ยอมให้ลบ branch ที่เป็น default อยู่
> และอย่าลบ `main` กับ `claude/fervent-wozniak-66knt2`

### สร้างแท็กเอง

คัดลอกทั้งก้อนนี้ไปวางในเครื่องที่มีสิทธิ์ push เต็ม แล้วรันครั้งเดียวจบ

```sh
git tag -a v00-h107.0.1-s117.0.7 b351769 -m "Host 107.0.1 / Station 117.0.7"
git tag -a v01-h107.0.1-s118.0.0 8a20d11 -m "Host 107.0.1 / Station 118.0.0"
git tag -a v02-h108.0.0-s118.0.0 eec7139 -m "Host 108.0.0 / Station 118.0.0"
git tag -a v03-h108.0.0-s118.1.0 a883805 -m "Host 108.0.0 / Station 118.1.0"
git tag -a v04-h108.0.0-s118.2.0 1ca1c8a -m "Host 108.0.0 / Station 118.2.0"
git tag -a v05-h108.0.0-s119.0.0 6620ab5 -m "Host 108.0.0 / Station 119.0.0"
git tag -a v06-h109.0.0-s119.0.0 c3584ff -m "Host 109.0.0 / Station 119.0.0"
git tag -a v07-h110.0.1-s120.0.0 origin/release-07-h110.0.1-s120.0.0 -m "Host 110.0.1 / Station 120.0.0"
git tag -a v08-h110.0.2-s120.0.1 origin/release/08-h110.0.2-s120.0.1 -m "Host 110.0.2 / Station 120.0.1"
git tag -a v09-h111.0.0-s120.0.1 origin/release/09-h111.0.0-s120.0.1 -m "Host 111.0.0 / Station 120.0.1"
git tag -a v10-h111.1.0-s120.0.1 origin/release/10-h111.1.0-s120.0.1 -m "Host 111.1.0 / Station 120.0.1"
git push origin --tags
```

หลังจากนั้นหน้า <https://github.com/kittiphun-rut/MCUPR-CANTEEN/tags> จะมีทุกรุ่นให้กดดาวน์โหลด zip ได้เลย

## เวลาจะแจ้งให้ปรับปรุง

บอกชื่อรุ่นมาได้เลย เช่น *"ย้อนไป `release/05` แล้วแก้เรื่อง ..."*
จะได้ตรงกันว่าหมายถึงโค้ดชุดไหน และไม่ต้องเดาว่าเป็นเวอร์ชันก่อนหรือหลัง
การเปลี่ยนโปรโตคอล

## ความเข้ากันได้ของโปรโตคอล ESP-NOW

ขนาดโครงสร้างด้านล่างอ่านจาก `static_assert` ในโค้ดของแต่ละแท็กจริง ไม่ได้อ้างจากความจำ

| ช่วงรุ่น | โครงสร้างแพ็กเก็ต | ผสมรุ่นข้ามฝั่งได้ไหม |
|---|---|---|
| `release/00` ถึง `release/02` | Station 50 / Host 200 / Config **5** ไบต์ | ได้ ขนาดเท่าต้นฉบับทั้งหมด (ต้นฉบับมี `HostConfigPacket` อยู่แล้ว แต่ฝั่งสถานีประกาศไว้เฉย ๆ ไม่เคยรับคำสั่งจริง) |
| `release/03` ถึง `release/06` | Config ขยาย **5 → 8** ไบต์ | **ไม่ได้** ต้องแฟลชทั้งสองฝั่ง ไม่งั้นคำสั่งโหมดการแสดงผลจะถูกทิ้งทั้งหมด |
| `release/07` | เพิ่ม `HostRosterPacket` **206** ไบต์ | ได้ แม่ข่ายรุ่นใหม่ตรวจจาก heartbeat ว่าสถานีรองรับบัญชีหรือไม่ ถ้าเป็นรุ่นเก่าจะไม่ส่งบัญชีไปให้เลย และแดชบอร์ดขึ้นว่า *เฟิร์มแวร์เก่า ไม่รองรับ* (จุดบริการนั้นยังทำงานครบทุกอย่าง แค่ไม่มีการตรวจสิทธิ์ตอนลิงก์ขาด) |
