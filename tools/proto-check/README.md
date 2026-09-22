# ตัวตรวจ forward declaration

Arduino IDE จะ **สร้างต้นแบบฟังก์ชัน (prototype) ให้เองอัตโนมัติ** แล้ววางไว้
ใกล้ ๆ หัวไฟล์ ซึ่งอยู่ **เหนือจุดที่นิยาม struct ของสเก็ตช์** ผลคือฟังก์ชันไหน
ที่รับ struct ของเราเป็นพารามิเตอร์แล้วไม่มีการประกาศล่วงหน้าไว้เอง จะคอมไพล์ไม่ผ่าน
ด้วยข้อความแบบนี้

```
error: variable or field 'fillStationConfig' declared void
error: 'HostConfigPacket' was not declared in this scope
```

วิธีแก้คือประกาศฟังก์ชันนั้นไว้ในบล็อก *Forward Declarations* (ซึ่งอยู่หลังนิยาม
โครงสร้างทั้งหมด) พอมีการประกาศไว้แล้ว Arduino จะไม่สร้างต้นแบบซ้อนให้อีก

สคริปต์นี้ไล่หาฟังก์ชันที่ยังขาดการประกาศ ใช้แทนการคอมไพล์จริงได้ในกรณีที่
เครื่องที่ใช้พัฒนาไม่มี toolchain ของ ESP32

## วิธีรัน

```sh
python3 tools/proto-check/protoscan.py \
  Canteen_Host_Server/Canteen_Host_Server.ino \
  Canteen_Station_Client/Canteen_Station_Client.ino
```

ได้ `ไม่พบปัญหา` ทั้งสองไฟล์แปลว่าผ่าน ถ้ามีรายการขึ้นมา ให้เอาชื่อฟังก์ชันนั้น
ไปเพิ่มในบล็อก Forward Declarations ของไฟล์นั้น (ฟังก์ชันที่เป็น `static`
ต้องประกาศเป็น `static` ด้วย)

## ข้อจำกัด

ตรวจเฉพาะฟังก์ชันที่เขียนนิยามโดยวงเล็บปีกกาเปิดอยู่บรรทัดเดียวกัน
ซึ่งเป็นรูปแบบที่ใช้ทั้งโครงการอยู่แล้ว
