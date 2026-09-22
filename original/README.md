# ต้นฉบับของเฟิร์มแวร์ก่อนแก้ไข (Baseline)

โฟลเดอร์นี้เก็บ **โค้ดต้นฉบับที่ส่งมาให้ตอนเริ่มโครงการ** ไว้ตามเดิมทุกไบต์
เพื่อให้ย้อนดูได้เสมอว่าจุดเริ่มต้นของระบบเขียนมาแบบไหน และการแก้ไขแต่ละครั้ง
เปลี่ยนอะไรไปบ้างเมื่อเทียบกับของเดิม

| ไฟล์ | เวอร์ชัน | ขนาด |
|---|---|---|
| `Canteen_Host_Server/Canteen_Host_Server.ino` | 107.0.1 | 3,207 บรรทัด |
| `Canteen_Station_Client/Canteen_Station_Client.ino` | 117.0.7 | 1,629 บรรทัด |

## ข้อควรระวัง

> **ห้ามแก้ไฟล์ในโฟลเดอร์นี้** ถ้าแก้แล้วจะหมดความหมายของการเก็บต้นฉบับ
> โค้ดที่ใช้งานจริงและพัฒนาต่ออยู่ที่ `Canteen_Host_Server/` และ
> `Canteen_Station_Client/` ที่โฟลเดอร์หลักของโครงการ

ทั้งสองไฟล์วางไว้ในโฟลเดอร์ที่ชื่อตรงกับชื่อไฟล์ จึงเปิดด้วย Arduino IDE
และคอมไพล์ได้ทันทีถ้าต้องการย้อนกลับไปใช้ของเดิมจริง ๆ

## เทียบกับของเดิมอย่างไร

ดูความต่างทั้งหมดตั้งแต่ต้นจนถึงตอนนี้:

```sh
diff -u original/Canteen_Host_Server/Canteen_Host_Server.ino \
        Canteen_Host_Server/Canteen_Host_Server.ino | less

diff -u original/Canteen_Station_Client/Canteen_Station_Client.ino \
        Canteen_Station_Client/Canteen_Station_Client.ino | less
```

หรือดูผ่าน git ซึ่งเก็บต้นฉบับไว้เป็นคอมมิตแรกของโครงการด้วย
(แท็ก `release-00-h107.0.1-s117.0.7`)

```sh
git diff release-00-h107.0.1-s117.0.7 HEAD -- Canteen_Host_Server/
```

## สิ่งที่พบในต้นฉบับ

รายการข้อผิดพลาดและช่องโหว่ที่ตรวจพบในโค้ดชุดนี้ พร้อมวิธีแก้ทีละข้อ
อยู่ในหัวข้อท้าย ๆ ของ [`../CHANGELOG.md`](../CHANGELOG.md)
(หัวข้อ *Host Server 108.0.0 / Station Client 118.0.0*)
