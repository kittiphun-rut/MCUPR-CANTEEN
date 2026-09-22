# สารบัญเวอร์ชัน (Release Index)

ทุกจุดที่เลขเวอร์ชันของฝั่งใดฝั่งหนึ่งเปลี่ยน ถูกชี้ตำแหน่งไว้ที่นี่
เพื่อให้ **ย้อนกลับไปใช้ชุดที่สมบูรณ์ชุดไหนก็ได้** โดยไม่ต้องไล่หาคอมมิตเอง

หนึ่งรุ่นคือ **เฟิร์มแวร์ครบทั้งคู่** ทั้งฝั่งแม่ข่ายและฝั่งสถานี
ที่ทดสอบและจัดทำเอกสารมาด้วยกัน — ไม่ควรจับคู่ข้ามรุ่น เพราะโปรโตคอล
ESP-NOW ระหว่างสองฝั่งเปลี่ยนไปในบางรุ่น

ทุกรุ่นถูก push ขึ้น GitHub ไว้เป็น **branch** ชื่อ `release-NN-hX-sY`
กดโหลดเป็นไฟล์ zip จากตารางข้างล่างได้เลย ไม่ต้องใช้คำสั่ง git

> หมายเหตุ: ปกติงานลักษณะนี้ควรเป็น **tag** ไม่ใช่ branch แต่บัญชีที่เซสชันนี้ใช้
> push `refs/tags/*` ไม่ได้ เซิร์ฟเวอร์ตอบ 403 ทุกครั้ง จึงใช้ branch แทน
> ได้ผลเหมือนกันทุกอย่างสำหรับการย้อนกลับ
> ถ้าอยากได้ tag จริงด้วย รันคำสั่งในหัวข้อ **สร้างแท็กเอง** ครั้งเดียวจบ
>
> **branch เหล่านี้ห้ามแก้และห้าม push ทับ** ให้ถือเป็นหมุดเวลาอย่างเดียว
> งานที่พัฒนาต่ออยู่ที่ `claude/fervent-wozniak-66knt2`

| ชื่อรุ่น | Host | Station | โหลด zip | คอมมิต | สาระสำคัญ |
|---|---|---|---|---|---|
| `release-00-h107.0.1-s117.0.7` | 107.0.1 | 117.0.7 | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release-00-h107.0.1-s117.0.7.zip) | [`b351769`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/b3517697c239e72ce180c0759843973b8903d7bf) | **ต้นฉบับตามที่ส่งมา** ยังไม่แก้อะไรเลย เก็บสำเนาไว้ที่ [`original/`](original/) ด้วย<br><sub>21 ก.ย. 2026</sub> |
| `release-01-h107.0.1-s118.0.0` | 107.0.1 | 118.0.0 | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release-01-h107.0.1-s118.0.0.zip) | [`8a20d11`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/8a20d11c8268e7102bd789f66c212bea5e83f9fc) | ฝั่งสถานี: แยกบัส SPI ของจอกับเครื่องอ่านบัตร (จอเพี้ยนหลัง `PCD_Init()`), ซ่อมสถานะลิงก์สด, รับคำสั่งสลับธีมจากแม่ข่าย<br><sub>21 ก.ย. 2026</sub> |
| `release-02-h108.0.0-s118.0.0` | 108.0.0 | 118.0.0 | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release-02-h108.0.0-s118.0.0.zip) | [`eec7139`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/eec713955d9c3fd56498f801d28f0bdd6d0f4662) | ฝั่งแม่ข่าย: ปิดช่องโหว่ความปลอดภัย, ซ่อม CSV และเลขอ้างอิง, ยกเครื่องเว็บพอร์ทัลให้อัปเดตสด, แยกหน้าเว็บออกเป็น `WebPortal.h`, เพิ่มจอสาธารณะ `/display`<br><sub>21 ก.ย. 2026</sub> |
| `release-03-h108.0.0-s118.1.0` | 108.0.0 | 118.1.0 | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release-03-h108.0.0-s118.1.0.zip) | [`a883805`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/a883805c9283a80f4240d1d90b7a971217f1a5b5) | ซิงค์โหมดการแสดงผลจากแม่ข่ายไปทุกสถานี, แก้พอร์ทัลใช้งานไม่ได้ในหน้าต่าง captive portal, เพิ่มสัญลักษณ์ RFID บนหน้าแรกของสถานี<br><sub>21 ก.ย. 2026</sub> |
| `release-04-h108.0.0-s118.2.0` | 108.0.0 | 118.2.0 | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release-04-h108.0.0-s118.2.0.zip) | [`1ca1c8a`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/1ca1c8ac3f8d21df85a88f87a076334e787bf659) | เติมข้อมูลหน้า 2 ของสถานีให้เต็มจอและเป็นข้อมูลสด (ยอดทั้งโรงอาหาร + รายการที่จ่ายล่าสุด)<br><sub>21 ก.ย. 2026</sub> |
| `release-05-h108.0.0-s119.0.0` | 108.0.0 | 119.0.0 | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release-05-h108.0.0-s119.0.0.zip) | [`6620ab5`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/6620ab5382a80fa3a9519bbb75f90561896788f5) | เสริมความทนทานของลิงก์สำหรับระยะ 20 เมตร และเพิ่ม **คิวออฟไลน์** ฝั่งสถานี<br><sub>21 ก.ย. 2026</sub> |
| `release-06-h109.0.0-s119.0.0` | 109.0.0 | 119.0.0 | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release-06-h109.0.0-s119.0.0.zip) | [`c3584ff`](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/c3584ffe8522ec5314dd960944e6ae16a2df569a) | เพิ่ม **เครื่องพิมพ์สลิปความร้อน 58 มม.** (`ThermalPrinter.h`) ผ่าน USB OTG หรือ UART<br><sub>21 ก.ย. 2026</sub> |
| `release-07-h110.0.1-s120.0.0` | 110.0.1 | 120.0.0 | [zip](https://github.com/kittiphun-rut/MCUPR-CANTEEN/archive/refs/heads/release-07-h110.0.1-s120.0.0.zip) | [ล่าสุดของ branch นี้](https://github.com/kittiphun-rut/MCUPR-CANTEEN/tree/release-07-h110.0.1-s120.0.0) | **ตรวจสิทธิ์ได้เองตอนลิงก์ขาด** ด้วยบัญชีสิทธิ์ย่อที่แม่ข่ายผลักไปเก็บที่สถานี<br><sub>22 ก.ย. 2026</sub> |

รายละเอียดของแต่ละรุ่นอยู่ใน [`CHANGELOG.md`](CHANGELOG.md)

## หมายเหตุเรื่องเลขเวอร์ชันฝั่งแม่ข่ายที่ค้าง

ระหว่าง `release-02` ถึง `release-05` ฝั่งแม่ข่ายมีการแก้ไขจริงหลายรอบ
(จอสาธารณะ, ซิงค์โหมดการแสดงผล, แก้ captive portal) แต่ `APP_VERSION`
ในโค้ดถูกลืมเลื่อน ค้างอยู่ที่ `108.0.0` ตลอด ขณะที่ CHANGELOG ระบุเป็น
108.1.0 / 108.2.0 / 108.3.0 ไปแล้ว เรื่องนี้ถูกแก้ตอน `release-06`
ด้วยการกระโดดไป `109.0.0` ทีเดียว
**ชื่อรุ่นในตารางข้างบนใช้เลขที่อยู่ในโค้ดจริง** ไม่ใช่เลขใน CHANGELOG
ตรวจแล้วทีละรุ่นด้วยการอ่าน `APP_VERSION` ออกมาจากโค้ดของคอมมิตนั้นจริง ๆ

## วิธีย้อนกลับไปใช้เวอร์ชันเก่า

### ไม่ใช้ git เลย

กด **zip** ในตารางข้างบน ได้ไฟล์ที่มีทั้งสองสเก็ตช์ครบชุดพร้อมอัปโหลดทันที

### ดูว่ารุ่นนั้นมีอะไรบ้าง

```sh
git fetch origin
git show origin/release-05-h108.0.0-s119.0.0 --stat
```

### ดึงโค้ดของรุ่นนั้นมาดูโดยไม่ยุ่งกับงานปัจจุบัน

```sh
git switch --detach origin/release-05-h108.0.0-s119.0.0
```

กลับมางานล่าสุดด้วย `git switch claude/fervent-wozniak-66knt2`

### ดึงเฉพาะไฟล์เดียวจากรุ่นเก่ามาทับของปัจจุบัน

```sh
git checkout origin/release-05-h108.0.0-s119.0.0 -- Canteen_Station_Client/
```

### ล้าง branch ที่เหลือค้าง

ระหว่างทางมี branch ชุดที่ขึ้นต้นด้วย `claude/release-...` ถูกสร้างไว้ก่อนจะเปลี่ยนมา
ใช้ชื่อสั้น และมี `fervent-wozniak-66knt2` (ไม่มีคำนำหน้า) หลงเหลืออยู่หนึ่งอัน
ทั้งหมดชี้คอมมิตเดียวกับของที่ใช้จริง จึงลบทิ้งได้โดยไม่เสียอะไรเลย
บัญชีที่เซสชันนี้ใช้ **ลบ branch ไม่ได้** (เซิร์ฟเวอร์ตอบ 403 เช่นเดียวกับ tag)
คัดลอกคำสั่งนี้ไปรันในเครื่องที่มีสิทธิ์เต็ม

```sh
git push origin --delete \
  claude/release-00-h107.0.1-s117.0.7 \
  claude/release-01-h107.0.1-s118.0.0 \
  claude/release-02-h108.0.0-s118.0.0 \
  claude/release-03-h108.0.0-s118.1.0 \
  claude/release-04-h108.0.0-s118.2.0 \
  claude/release-05-h108.0.0-s119.0.0 \
  claude/release-06-h109.0.0-s119.0.0 \
  claude/release-07-h110.0.1-s120.0.0 \
  fervent-wozniak-66knt2
```

> อย่าลบ `claude/fervent-wozniak-66knt2` — นั่นคือ branch ที่งานพัฒนาเดินอยู่

### สร้างแท็กเอง

คัดลอกทั้งก้อนนี้ไปวางในเครื่องที่มีสิทธิ์ push เต็ม แล้วรันครั้งเดียวจบ

```sh
git tag -a release-00-h107.0.1-s117.0.7 b351769 -m "Host 107.0.1 / Station 117.0.7"
git tag -a release-01-h107.0.1-s118.0.0 8a20d11 -m "Host 107.0.1 / Station 118.0.0"
git tag -a release-02-h108.0.0-s118.0.0 eec7139 -m "Host 108.0.0 / Station 118.0.0"
git tag -a release-03-h108.0.0-s118.1.0 a883805 -m "Host 108.0.0 / Station 118.1.0"
git tag -a release-04-h108.0.0-s118.2.0 1ca1c8a -m "Host 108.0.0 / Station 118.2.0"
git tag -a release-05-h108.0.0-s119.0.0 6620ab5 -m "Host 108.0.0 / Station 119.0.0"
git tag -a release-06-h109.0.0-s119.0.0 c3584ff -m "Host 109.0.0 / Station 119.0.0"
git tag -a release-07-h110.0.1-s120.0.0 origin/release-07-h110.0.1-s120.0.0 -m "Host 110.0.1 / Station 120.0.0"
git push origin --tags
```

หลังจากนั้นหน้า <https://github.com/kittiphun-rut/MCUPR-CANTEEN/tags> จะมีทุกรุ่นให้กดดาวน์โหลด zip ได้เลย

## เวลาจะแจ้งให้ปรับปรุง

บอกชื่อรุ่นมาได้เลย เช่น *"ย้อนไป `release-05` แล้วแก้เรื่อง ..."*
จะได้ตรงกันว่าหมายถึงโค้ดชุดไหน และไม่ต้องเดาว่าเป็นเวอร์ชันก่อนหรือหลัง
การเปลี่ยนโปรโตคอล

## ความเข้ากันได้ของโปรโตคอล ESP-NOW

ขนาดโครงสร้างด้านล่างอ่านจาก `static_assert` ในโค้ดของแต่ละแท็กจริง ไม่ได้อ้างจากความจำ

| ช่วงรุ่น | โครงสร้างแพ็กเก็ต | ผสมรุ่นข้ามฝั่งได้ไหม |
|---|---|---|
| `release-00` ถึง `release-02` | Station 50 / Host 200 / Config **5** ไบต์ | ได้ ขนาดเท่าต้นฉบับทั้งหมด (ต้นฉบับมี `HostConfigPacket` อยู่แล้ว แต่ฝั่งสถานีประกาศไว้เฉย ๆ ไม่เคยรับคำสั่งจริง) |
| `release-03` ถึง `release-06` | Config ขยาย **5 → 8** ไบต์ | **ไม่ได้** ต้องแฟลชทั้งสองฝั่ง ไม่งั้นคำสั่งโหมดการแสดงผลจะถูกทิ้งทั้งหมด |
| `release-07` | เพิ่ม `HostRosterPacket` **206** ไบต์ | ได้ แม่ข่ายรุ่นใหม่ตรวจจาก heartbeat ว่าสถานีรองรับบัญชีหรือไม่ ถ้าเป็นรุ่นเก่าจะไม่ส่งบัญชีไปให้เลย และแดชบอร์ดขึ้นว่า *เฟิร์มแวร์เก่า ไม่รองรับ* (จุดบริการนั้นยังทำงานครบทุกอย่าง แค่ไม่มีการตรวจสิทธิ์ตอนลิงก์ขาด) |
