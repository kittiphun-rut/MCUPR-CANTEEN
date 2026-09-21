/**
 * ============================================================================
 * ThermalPrinter.h — ไดรเวอร์เครื่องพิมพ์ความร้อน 58 มม. (ESC/POS)
 *
 * ไฟล์นี้เป็นไดรเวอร์ล้วน ๆ ไม่รู้จักข้อมูลของระบบโรงอาหารเลย หน้าที่มีแค่
 * รับสตริงไบต์ ESC/POS ใส่คิว แล้วทยอยส่งออกไปให้เครื่องพิมพ์แบบไม่บล็อกลูปหลัก
 * ส่วนเนื้อหาสลิปถูกประกอบในไฟล์สเก็ตช์หลักที่มีข้อมูลนิสิตและร้านค้าอยู่
 *
 * รองรับสองช่องทาง เลือกด้วย PRINTER_TRANSPORT_UART
 *   0 = USB OTG host  (ค่าเริ่มต้น) เขียนไดรเวอร์คลาสปริ้นเตอร์เองบน usb_host_lib
 *   1 = UART TTL      ทางสำรองที่แน่นอนกว่า ต่อสาย TX/RX/GND ที่ GPIO 17/18
 *
 * ---------------------------------------------------------------------------
 * ข้อกำหนดเมื่อใช้ USB OTG (อ่านก่อนต่อสาย)
 *
 *  1. บอร์ด DevKitC-1 ออกแบบพอร์ต USB ไว้เป็น device ขา VBUS เป็นไฟ "เข้า"
 *     การทำ host ต้องป้อน 5V เข้าขา VBUS ของพอร์ตนั้นเองจากแหล่งจ่ายภายนอก
 *  2. เมื่อสแต็ก USB host ยึดขา GPIO 19/20 แล้ว พอร์ต USB เนทีฟจะใช้อัปโหลด
 *     และดู Serial Monitor ไม่ได้ ให้ใช้พอร์ต UART/COM แทน
 *  3. Tools > USB Mode ต้องเป็น "Hardware CDC and JTAG" ห้ามเป็น USB-OTG (TinyUSB)
 *  4. หัวพิมพ์ความร้อนกินกระแสพีค 1.5-2 A เครื่องพิมพ์ต้องมีไฟเลี้ยงของตัวเอง
 *     ห้ามดึงจากบอร์ด ESP32 เด็ดขาด ต่อร่วมกันได้แค่สาย GND
 *
 * ถ้า USB ไม่ยอมทำงาน ให้ตั้ง PRINTER_TRANSPORT_UART เป็น 1 แล้วต่อสายแทน
 * เนื้อหาสลิปและปุ่มบนหน้าเว็บทำงานเหมือนกันทั้งสองช่องทาง
 * ============================================================================
 */

#pragma once

#ifndef ENABLE_THERMAL_PRINTER
  #define ENABLE_THERMAL_PRINTER 1
#endif
#ifndef PRINTER_TRANSPORT_UART
  #define PRINTER_TRANSPORT_UART 0      // 0 = USB OTG host, 1 = UART TTL
#endif

#define PRINTER_COLS        32          // 58 มม. = 384 จุด ฟอนต์ A 12x24 จึงได้ 32 ตัวอักษร
#define PRINTER_QUEUE_MAX   8           // จำนวนสลิปที่พักคิวไว้ได้
#define PRINTER_CHUNK       64          // ขนาดก้อนที่ส่งต่อรอบ (เท่ากับ MPS ของ bulk endpoint)
#define PRINTER_STALE_MS    120000UL    // สลิปที่ค้างคิวนานกว่านี้โดยไม่มีเครื่องพิมพ์ ให้ทิ้ง

#define PRINTER_UART_NUM    1
#define PRINTER_TX_PIN      17
#define PRINTER_RX_PIN      18
#define PRINTER_BAUD        9600

// ---------------------------------------------------------------------------
// ตัวช่วยประกอบคำสั่ง ESC/POS
// หมายเหตุ: หลายคำสั่งมีไบต์ 0x00 อยู่ข้างใน String ของ Arduino เก็บความยาวแยก
// จากบัฟเฟอร์จึงพก 0x00 ได้ ทุกจุดที่ส่งออกต้องใช้ length() ห้ามใช้ strlen
// ---------------------------------------------------------------------------
inline String escInit()              { return String("\x1B\x40"); }
inline String escAlign(uint8_t n)    { String s = "\x1B\x61"; s += (char)n; return s; }   // 0 ซ้าย 1 กลาง 2 ขวา
inline String escBold(bool on)       { String s = "\x1B\x45"; s += (char)(on ? 1 : 0); return s; }
inline String escSize(uint8_t n)     { String s = "\x1D\x21"; s += (char)n; return s; }   // 0x11 = สูงและกว้างสองเท่า
inline String escFeed(uint8_t n)     { String s = "\x1B\x64"; s += (char)n; return s; }
inline String escCut()               { return String("\x1D\x56\x01"); }
inline String escRule(char c = '-') {
  String s;
  for (int i = 0; i < PRINTER_COLS; i++) s += c;
  s += "\n";
  return s;
}
// จัดข้อความคู่ ซ้าย-ขวา ให้เต็มความกว้างกระดาษ
inline String escPair(const String &left, const String &right) {
  int pad = PRINTER_COLS - (int)left.length() - (int)right.length();
  String s = left;
  if (pad < 1) { s += " "; }
  else { for (int i = 0; i < pad; i++) s += ' '; }
  s += right;
  s += "\n";
  return s;
}
inline String escCenter(const String &text) {
  int pad = (PRINTER_COLS - (int)text.length()) / 2;
  String s;
  for (int i = 0; i < pad; i++) s += ' ';
  s += text;
  s += "\n";
  return s;
}

bool   printerEnqueue(const String &payload);
void   printerBegin();
void   printerLoop();
bool   printerIsConnected();
String printerStatusText();
uint8_t printerQueueDepth();

#if ENABLE_THERMAL_PRINTER

// ---------------------------------------------------------------------------
// คิวสลิป: เก็บเป็นสตริงไบต์พร้อมส่ง ทยอยปล่อยออกทีละก้อนในลูปหลัก
// ---------------------------------------------------------------------------
static String        pqSlots[PRINTER_QUEUE_MAX];
static unsigned long pqStamp[PRINTER_QUEUE_MAX] = {0};
static uint8_t pqCount  = 0;
static uint8_t pqHead   = 0;
static size_t  pqCursor = 0;

bool printerEnqueue(const String &payload) {
  if (payload.length() == 0) return false;
  if (pqCount >= PRINTER_QUEUE_MAX) return false;     // คิวเต็ม ทิ้งดีกว่าบล็อกการตัดสิทธิ์
  uint8_t slot = (pqHead + pqCount) % PRINTER_QUEUE_MAX;
  pqSlots[slot] = payload;
  pqStamp[slot] = millis();
  pqCount++;
  return true;
}
uint8_t printerQueueDepth() { return pqCount; }

static String *pqFront() { return pqCount ? &pqSlots[pqHead] : nullptr; }
static void pqPop() {
  if (!pqCount) return;
  pqSlots[pqHead] = "";
  pqHead = (pqHead + 1) % PRINTER_QUEUE_MAX;
  pqCount--;
  pqCursor = 0;
}

// ถ้าไม่มีเครื่องพิมพ์ต่ออยู่ คิวจะตันอยู่อย่างนั้นและเต็มถาวร พอเสียบเครื่องทีหลัง
// ก็จะพ่นสลิปเก่าที่หมดความหมายไปแล้วออกมารวดเดียว จึงทิ้งใบที่ค้างนานเกินกำหนดทิ้งไป
// เรียกจาก printerLoop() ทุกช่องทาง หัวคิวที่กำลังส่งอยู่ (pqCursor > 0) จะไม่ถูกแตะ
static void pqDropStale() {
  while (pqCount && pqCursor == 0 && (millis() - pqStamp[pqHead]) > PRINTER_STALE_MS) {
    pqPop();
  }
}

#if PRINTER_TRANSPORT_UART
// ===========================================================================
// ช่องทาง UART TTL
// ===========================================================================
static HardwareSerial PrinterSerial(PRINTER_UART_NUM);
static bool sUartReady = false;

void printerBegin() {
  PrinterSerial.begin(PRINTER_BAUD, SERIAL_8N1, PRINTER_RX_PIN, PRINTER_TX_PIN);
  sUartReady = true;
  printerEnqueue(escInit());
}

void printerLoop() {
  if (!sUartReady) return;
  pqDropStale();
  String *cur = pqFront();
  if (!cur) return;
  size_t remain = cur->length() - pqCursor;
  if (remain == 0) { pqPop(); return; }
  int room = PrinterSerial.availableForWrite();
  if (room <= 0) return;                               // บัฟเฟอร์เต็ม รอรอบหน้า
  size_t n = remain;
  if (n > (size_t)room) n = (size_t)room;
  if (n > PRINTER_CHUNK) n = PRINTER_CHUNK;
  PrinterSerial.write((const uint8_t *)cur->c_str() + pqCursor, n);
  pqCursor += n;
}

bool   printerIsConnected() { return sUartReady; }     // UART ไม่มีทางรู้ว่าปลายทางมีจริงไหม
String printerStatusText()  { return sUartReady ? "UART READY" : "UART NOT STARTED"; }

#else
// ===========================================================================
// ช่องทาง USB OTG host
// ===========================================================================
#if defined(ARDUINO_USB_MODE) && (ARDUINO_USB_MODE == 0)
  #error "USB OTG ถูก TinyUSB ยึดไว้ในโหมด device — ตั้ง Tools > USB Mode เป็น 'Hardware CDC and JTAG' หรือใช้ PRINTER_TRANSPORT_UART 1"
#endif
#if defined(__has_include)
  #if !__has_include(<usb/usb_host.h>)
    #error "core ที่ใช้อยู่ไม่มี USB Host stack — ใช้ Arduino ESP32 core 3.x ขึ้นไป หรือตั้ง PRINTER_TRANSPORT_UART เป็น 1"
  #endif
#endif
#include <usb/usb_host.h>

static usb_host_client_handle_t sClient = NULL;
static usb_device_handle_t      sDev    = NULL;
static usb_transfer_t          *sXfer   = NULL;
static volatile bool            sXferBusy  = false;
static bool                     sIfClaimed = false;
static uint8_t                  sIfNum  = 0;
static uint8_t                  sEpOut  = 0;
static uint16_t                 sEpMps  = PRINTER_CHUNK;
static const char              *sClassName = "-";
static bool                     sHostUp = false;

static void printerXferCb(usb_transfer_t *t) { (void)t; sXferBusy = false; }

static void printerCloseDevice() {
  if (sXfer)      { usb_host_transfer_free(sXfer); sXfer = NULL; }
  if (sIfClaimed && sDev) { usb_host_interface_release(sClient, sDev, sIfNum); sIfClaimed = false; }
  if (sDev)       { usb_host_device_close(sClient, sDev); sDev = NULL; }
  sEpOut = 0; sXferBusy = false; sClassName = "-";
  pqCursor = 0;                                        // เริ่มสลิปที่ค้างอยู่ใหม่ทั้งใบ
}

// เดินไล่ descriptor ของ configuration ตามสเปก USB โดยตรง ไม่พึ่งฟังก์ชันช่วยของ IDF
// เพื่อให้โค้ดไม่ผูกกับลายเซ็นของ usb_helpers ที่ต่างกันไปตามเวอร์ชัน
// เลือกอินเทอร์เฟซที่มี bulk OUT โดยให้คลาส 0x07 (Printer) มาก่อน 0x0A (CDC Data)
static bool printerPickInterface(const usb_config_desc_t *cfg) {
  const uint8_t *p = (const uint8_t *)cfg;
  int total = (int)cfg->wTotalLength;
  int i = 0;
  int curScore = 0, bestScore = 0;
  uint8_t curIf = 0;

  while (i + 2 <= total) {
    uint8_t len  = p[i];
    uint8_t type = p[i + 1];
    if (len == 0) break;

    if (type == 0x04 && i + 9 <= total) {              // INTERFACE descriptor
      curIf = p[i + 2];
      uint8_t cls = p[i + 5];
      curScore = (cls == 0x07) ? 2 : (cls == 0x0A ? 1 : 0);
    } else if (type == 0x05 && i + 7 <= total) {       // ENDPOINT descriptor
      uint8_t addr = p[i + 2];
      uint8_t attr = p[i + 3];
      uint16_t mps = (uint16_t)p[i + 4] | ((uint16_t)p[i + 5] << 8);
      bool bulkOut = ((attr & 0x03) == 0x02) && ((addr & 0x80) == 0);
      if (bulkOut && curScore > bestScore) {
        bestScore  = curScore;
        sIfNum     = curIf;
        sEpOut     = addr;
        sEpMps     = mps ? mps : PRINTER_CHUNK;
        sClassName = (curScore == 2) ? "PRINTER CLASS" : "CDC DATA";
      }
    }
    i += len;
  }
  return bestScore > 0;
}

static void printerClientCb(const usb_host_client_event_msg_t *msg, void *arg) {
  (void)arg;
  if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
    if (sDev) return;                                  // รองรับเครื่องพิมพ์ตัวเดียว
    if (usb_host_device_open(sClient, msg->new_dev.address, &sDev) != ESP_OK) { sDev = NULL; return; }

    const usb_config_desc_t *cfg = NULL;
    if (usb_host_get_active_config_descriptor(sDev, &cfg) != ESP_OK || cfg == NULL) { printerCloseDevice(); return; }
    if (!printerPickInterface(cfg))                                                { printerCloseDevice(); return; }
    if (usb_host_interface_claim(sClient, sDev, sIfNum, 0) != ESP_OK)              { printerCloseDevice(); return; }
    sIfClaimed = true;

    size_t bufSize = sEpMps > PRINTER_CHUNK ? sEpMps : PRINTER_CHUNK;
    if (usb_host_transfer_alloc(bufSize, 0, &sXfer) != ESP_OK) { printerCloseDevice(); return; }
    sXfer->device_handle    = sDev;
    sXfer->bEndpointAddress = sEpOut;
    sXfer->callback         = printerXferCb;
    sXfer->context          = NULL;
    sXferBusy               = false;

    printerEnqueue(escInit());                         // ตั้งค่าเริ่มต้นทุกครั้งที่เสียบใหม่
  } else if (msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
    printerCloseDevice();
  }
}

static void printerUsbLibTask(void *arg) {
  (void)arg;
  while (true) {
    uint32_t flags = 0;
    usb_host_lib_handle_events(portMAX_DELAY, &flags);
    if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) usb_host_device_free_all();
  }
}

static void printerUsbClientTask(void *arg) {
  (void)arg;
  while (true) usb_host_client_handle_events(sClient, portMAX_DELAY);
}

void printerBegin() {
  usb_host_config_t hostCfg = {};
  hostCfg.skip_phy_setup = false;
  hostCfg.intr_flags     = ESP_INTR_FLAG_LEVEL1;
  if (usb_host_install(&hostCfg) != ESP_OK) return;

  xTaskCreatePinnedToCore(printerUsbLibTask, "prn_usb_lib", 4096, NULL, 2, NULL, 0);

  usb_host_client_config_t clientCfg = {};
  clientCfg.is_synchronous              = false;
  clientCfg.max_num_event_msg           = 5;
  clientCfg.async.client_event_callback = printerClientCb;
  clientCfg.async.callback_arg          = NULL;
  if (usb_host_client_register(&clientCfg, &sClient) != ESP_OK) return;

  xTaskCreatePinnedToCore(printerUsbClientTask, "prn_usb_cli", 4096, NULL, 2, NULL, 0);
  sHostUp = true;
}

void printerLoop() {
  if (sEpOut == 0) pqDropStale();
  if (!sHostUp || sEpOut == 0 || sXfer == NULL || sXferBusy) return;
  String *cur = pqFront();
  if (!cur) return;

  size_t remain = cur->length() - pqCursor;
  if (remain == 0) { pqPop(); return; }

  size_t n = remain;
  if (n > sEpMps) n = sEpMps;
  memcpy(sXfer->data_buffer, cur->c_str() + pqCursor, n);
  sXfer->num_bytes = n;
  sXferBusy = true;
  if (usb_host_transfer_submit(sXfer) != ESP_OK) { sXferBusy = false; return; }
  pqCursor += n;
}

bool printerIsConnected() { return sEpOut != 0; }
String printerStatusText() {
  if (!sHostUp) return "USB HOST NOT STARTED";
  if (sEpOut == 0) return "NO PRINTER DETECTED";
  return String("CONNECTED (") + sClassName + ")";
}

#endif  // PRINTER_TRANSPORT_UART

#else
// ปิดฟีเจอร์: ทุกอย่างกลายเป็นฟังก์ชันเปล่าเพื่อให้ไฟล์หลักคอมไพล์ได้เหมือนเดิม
bool    printerEnqueue(const String &payload) { (void)payload; return false; }
void    printerBegin() {}
void    printerLoop()  {}
bool    printerIsConnected() { return false; }
String  printerStatusText()  { return "DISABLED"; }
uint8_t printerQueueDepth()  { return 0; }
#endif  // ENABLE_THERMAL_PRINTER
