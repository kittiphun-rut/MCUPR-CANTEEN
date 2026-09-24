/* ===== ค่าตัวอย่างที่ใช้จำลอง (ตรงกับชุดเดียวกับภาพเว็บพอร์ทัล) ===== */
const DB_TOTAL = 412, USED = 268, SHOP = [84, 72, 63, 49];
const HOST_V = 4.04, HOST_PCT = 87;
const TEMP_C = 47.3, CPU = 11.6, HEAP = 214;
const CLOCK = '11:47:05', DATE_H = 'Tue, 22 Sep 2026', DATE_S = 'TUE, 22 SEP 2026';
const NODES = [
  { on: true,  rssi: -46, v: 4.08 },
  { on: true,  rssi: -63, v: 3.90 },
  { on: true,  rssi: -81, v: 3.59 },
  { on: false, rssi: -100, v: 0 },
];
const ST_ID = 2, ST_RSSI = -63, ST_V = 3.90, ST_SERVED = 72;
const SCAN_UID = '030541****', SCAN_SID = '6501234501';
const ST_REJECT = 3, LAST_PAYOUT = '11:47:05';
const ST_FEED = [  // ใหม่สุดอยู่บน
  { time: '11:47:05', id: '65012*****' },
  { time: '11:44:21', id: '65027*****' },
  { time: '11:41:08', id: '65006*****' },
];
const OFFLINE_COUNT = 7, OFFLINE_MAX = 48;
const SYS_USED = 268, SYS_TOTAL = 412, SYS_OPEN = true, SYS_WINDOW = '10:00-13:30';

/* ===== Host: แถบบน/ล่าง และการ์ด ===== */
function drawHostBatteryHUD(x, y) {
  const pct = HOST_PCT, volt = HOST_V;
  fillRect(x, y, 54, 18, BG());
  drawRect(x, y + 2, 44, 14, TEXT());
  fillRect(x + 44, y + 6, 3, 6, TEXT());
  let fw = Math.floor(pct * 40 / 100); if (fw < 1 && pct > 0) fw = 1;
  let col = pct > 20 ? GREEN() : ROSE(); if (volt > 4.0) col = CYAN();
  fillRect(x + 2, y + 4, fw, 10, col);
  const s = pct + '%', tw = s.length * 6, tx = x + 2 + Math.floor((40 - tw) / 2), ty = y + 6;
  setTextSize(1);
  const tbg = fw > 22 ? col : BG();
  setTextColor(fw > 22 ? 0x0000 : TEXT(), tbg); setCursor(tx, ty); print(s);
}
function drawHostTopBar(t, pageNo) {
  fillRect(0, 0, 320, 26, BG()); drawFastHLine(0, 26, 320, BORDER());
  setTextColor(TEXT(), BG()); setTextSize(2); setCursor(8, 6); print(t);
  if (pageNo > 0) {
    const tag = pageNo + '/3';
    setTextSize(1); setTextColor(MUTED(), BG()); setCursor(222, 9); print(tag);
  }
  drawHostBatteryHUD(252, 4);
}
function fitLabel(raw, maxChars) {
  if (raw.length <= maxChars) return raw;
  if (maxChars <= 1) return raw.slice(0, maxChars);
  return raw.slice(0, maxChars - 1) + '.';
}
function drawCardLabel(x, y, t) { setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(x, y); print(t); }
function drawStatusDot(cx, cy, good) { fillCircle(cx, cy, 4, good ? GREEN() : ROSE()); }
function drawProgressBar(x, y, w, h, pct) {
  pct = Math.max(0, Math.min(100, pct));
  fillRect(x, y, w, h, CARD());
  fillRoundRect(x, y, w, h, h / 2, BG());
  let fw = Math.floor(w * pct / 100); if (fw < 2 && pct > 0) fw = 2;
  if (fw > 0) fillRoundRect(x, y, fw, h, h / 2, GREEN());
}
function drawBentoBottomBar(t) {
  fillRect(0, 214, 320, 26, BG()); drawFastHLine(0, 214, 320, BORDER());
  setTextColor(MUTED(), BG()); setTextSize(1); setCursor(12, 222); print(t);
}
function drawBentoCard(x, y, w, h, b, c) { fillRoundRect(x, y, w, h, 8, c); drawRoundRect(x, y, w, h, 8, b); }
function drawBentoPillBadge(x, y, w, h, t, f, b) {
  fillRoundRect(x, y, w, h, h / 2, b); setTextColor(f, b); setTextSize(1);
  const px = x + Math.floor((w - t.length * 6) / 2);
  setCursor(Math.max(x + 2, px), y + Math.floor((h - 8) / 2)); print(t);
}

/* ===== Station: แถบบน/ล่าง และการ์ด ===== */
function drawStationBatteryHUD(x, y) {
  const volt = ST_V, pct = battPct(volt);
  fillRect(x, y, 52, 18, BG());
  drawRect(x, y + 2, 44, 14, TEXT()); fillRect(x + 44, y + 6, 3, 6, TEXT());
  let fw = Math.floor(pct * 40 / 100); if (fw < 1 && pct > 0) fw = 1;
  let col = pct > 20 ? GREEN() : ROSE(); if (volt > 4.05) col = CYAN();
  fillRect(x + 2, y + 4, fw, 10, col);
  const s = pct + '%', tw = s.length * 6, tx = x + 2 + Math.floor((40 - tw) / 2), ty = y + 5;
  setTextSize(1);
  const tbg = fw > 22 ? col : BG();
  setTextColor(fw > 22 ? 0x0000 : TEXT(), tbg); setCursor(tx, ty); print(s);
}
/* [122.9.0] ตัดตัวเลข dBm และคำว่า OFFLINE ออก เหลือแต่สัญลักษณ์ขีดสัญญาณ
   ต้องตรงกับ updateTopRightHeaderSmooth() ใน StationScreen.h */
function updateStationHeaderStatus(online) {
  drawSignalBars(228, 6, online ? ST_RSSI : -100, online, BG());
  drawStationBatteryHUD(260, 3);
}
function drawStationTopBar(t, online = true) {
  fillRect(0, 0, 320, 24, BG()); drawFastHLine(0, 24, 320, BORDER());
  setTextColor(CYAN(), BG()); setTextSize(1); setCursor(8, 8);
  print(t.length > 30 ? t.slice(0, 30) : t);
  updateStationHeaderStatus(online);
}
function drawStationBottomBar(t) {
  fillRect(0, 218, 320, 22, BG()); drawFastHLine(0, 218, 320, BORDER());
  setTextColor(MUTED(), BG()); setTextSize(1); setCursor(12, 224); print(t);
}
function drawStationCard(x, y, w, h, b, c) { fillRoundRect(x, y, w, h, 6, c); drawRoundRect(x, y, w, h, 6, b); }
function drawStationPillBadge(x, y, w, h, t, f, b) {
  fillRoundRect(x, y, w, h, 3, b); setTextColor(f, b); setTextSize(1);
  const px = x + Math.floor((w - t.length * 6) / 2);
  setCursor(Math.max(x + 2, px), y + Math.floor((h - 8) / 2)); print(t);
}

/* ===== หน้าจอทั้งหมด ===== */
function hostPage0() {
  fillScreen(BG());
  drawHostTopBar('TODAY', 1);
  drawBentoCard(6, 32, 150, 88, BORDER(), CARD());   drawCardLabel(14, 40, 'MEALS SERVED');
  drawBentoCard(164, 32, 150, 88, BORDER(), CARD()); drawCardLabel(172, 40, 'BAHT PAID');
  drawBentoCard(6, 126, 308, 82, BORDER(), CARD());  drawCardLabel(14, 134, 'SERVICE');
  drawBentoBottomBar('Click: next page    Double click: sleep');

  fillRect(12, 54, 140, 60, CARD());
  setTextColor(TEXT(), CARD()); setTextSize(4); setCursor(14, 56); print(String(USED));
  setTextSize(1); setTextColor(MUTED(), CARD()); setCursor(14, 92); print('of ' + DB_TOTAL + ' students');
  drawProgressBar(14, 106, 134, 6, Math.floor(USED * 100 / DB_TOTAL));

  fillRect(170, 54, 140, 60, CARD());
  setTextColor(GREEN(), CARD()); setTextSize(4); setCursor(172, 56); print(String(USED * 35));
  setTextSize(1); setTextColor(MUTED(), CARD()); setCursor(172, 92); print('35 baht per student');

  fillRect(12, 148, 296, 56, CARD());
  setTextColor(TEXT(), CARD()); setTextSize(4); setCursor(14, 150); print(CLOCK);
  setTextSize(1); setTextColor(MUTED(), CARD()); setCursor(14, 186); print(DATE_H);

  const hours = SYS_WINDOW;
  drawBentoPillBadge(214, 152, 94, 22, SYS_OPEN ? 'OPEN' : 'CLOSED',
                     dark ? 0x0000 : 0xFFFF, SYS_OPEN ? GREEN() : ROSE());
  setTextColor(MUTED(), CARD()); setTextSize(1);
  setCursor(214 + Math.floor((94 - hours.length * 6) / 2), 186); print(hours);
}

function hostPage1() {
  fillScreen(BG());
  drawHostTopBar('SHOPS', 2);
  drawBentoBottomBar('Click: next page    Double click: sleep');
  const coords = [[6, 32], [164, 32], [6, 122], [164, 122]];
  const names = ['Chicken Rice & Drinks', 'Noodle House', 'Vegetarian Corner', 'Coffee & Bakery'];
  const waitDots = 2;
  for (let i = 0; i < 4; i++) {
    const x = coords[i][0], y = coords[i][1], w = 150, h = 86;
    drawBentoCard(x, y, w, h, BORDER(), CARD());

    drawFixedText(x + 8, y + 7, 1, TEXT(), CARD(), 22, names[i]);

    drawFixedText(x + 8, y + 36, 3, TEXT(), CARD(), 3, String(SHOP[i]));
    drawFixedText(x + 68, y + 38, 1, MUTED(), CARD(), 6, 'meals');
    drawFixedText(x + 68, y + 52, 1, GREEN(), CARD(), 10, SHOP[i] * 35 + ' baht');

    fillRect(x + 8, y + 20, 74, 12, CARD());
    drawStatusDot(x + 12, y + 25, NODES[i].on);
    setTextSize(1); setTextColor(MUTED(), CARD());
    setCursor(x + 22, y + 22); print('Station ' + (i + 1));

    fillRect(x + 8, y + 66, w - 16, 14, CARD());
    if (NODES[i].on) {
      const sPct = battPct(NODES[i].v);
      drawSignalBars(x + 8, y + 68, NODES[i].rssi, true, CARD());
      setTextColor(MUTED(), CARD()); setCursor(x + 40, y + 70); print('signal');
      setCursor(x + 92, y + 70); print(sPct + '%');
      drawMiniBattery(x + 120, y + 68, sPct);
    } else {
      setTextColor(ROSE(), CARD()); setCursor(x + 8, y + 70);
      print('Waiting' + '.'.repeat(waitDots + 1));
    }
  }
}

function hostPage2() {
  fillScreen(BG());
  drawHostTopBar('SYSTEM', 3);
  drawBentoCard(6, 32, 308, 84, BORDER(), CARD());  drawCardLabel(14, 40, 'LAST CARD TAP');
  drawBentoCard(6, 122, 150, 86, BORDER(), CARD()); drawCardLabel(14, 130, 'NETWORK');
  drawBentoCard(164, 122, 150, 86, BORDER(), CARD()); drawCardLabel(172, 130, 'DEVICE');

  setTextColor(TEXT(), CARD()); setTextSize(1);
  setCursor(14, 148); print('Wi-Fi  MCU_CANTEEN');
  setCursor(14, 164); print('Page   192.168.4.1');
  setCursor(14, 180); print('Radio  channel 1');
  setTextColor(MUTED(), CARD()); setCursor(14, 192); print('4 stations linked');
  drawBentoBottomBar('Click: next page    Double click: sleep');

  fillRect(12, 54, 296, 58, CARD());
  setTextColor(TEXT(), CARD()); setTextSize(3); setCursor(14, 56); print(SCAN_SID);
  setTextSize(1); setTextColor(MUTED(), CARD()); setCursor(14, 86);
  print('Station ' + ST_ID + '   card ' + SCAN_UID);
  drawBentoPillBadge(14, 98, 130, 14, 'SERVED', dark ? 0x0000 : 0xFFFF, GREEN());
  let ref = 'TXN-20260922-114705-ST2-0007';
  if (ref.length > 24) ref = '.' + ref.slice(ref.length - 23);
  setTextColor(MUTED(), CARD()); setCursor(152, 101); print(ref);

  fillRect(170, 142, 138, 62, CARD());
  setTextSize(1); setTextColor(TEXT(), CARD());
  setCursor(172, 148); print('Students ' + DB_TOTAL);
  setCursor(172, 164); print('Staff    1 of 3');
  setTextColor(TEMP_C < 65 ? TEXT() : YELLOW(), CARD());
  setCursor(172, 180); print('Chip     ' + Math.round(TEMP_C) + ' C');
  setTextColor(MUTED(), CARD()); setCursor(172, 192); print('Free memory ' + HEAP + ' KB');
}

function hostBootNet(apOk) {
  fillScreen(BG());
  drawHostTopBar('NETWORK');
  const tone = apOk ? GREEN() : ROSE();
  fillRect(0, 30, 320, 34, tone);
  const head = apOk ? 'WI-FI IS ON' : 'WI-FI FAILED TO START';
  setTextSize(2); setTextColor(dark ? 0x0000 : 0xFFFF, tone);
  setCursor(Math.max(4, Math.floor((320 - head.length * 12) / 2)), 39); print(head);

  drawBentoCard(6, 74, 308, 134, BORDER(), CARD());
  setTextSize(1);
  if (apOk) {
    setTextColor(MUTED(), CARD());
    setCursor(20, 92);  print('Wi-Fi name');
    setCursor(20, 122); print('Password');
    setCursor(20, 152); print('Web page');
    setTextColor(TEXT(), CARD()); setTextSize(2);
    setCursor(20, 102); print('MCU_CANTEEN_HOST');
    setCursor(20, 132); print('12345678');
    setCursor(20, 162); print('192.168.4.1');
    setTextSize(1); setTextColor(MUTED(), CARD());
    setCursor(20, 186); print('or http://canteen.local');
  } else {
    setTextColor(TEXT(), CARD()); setTextSize(2);
    setCursor(20, 100); print('Turn the power off');
    setCursor(20, 124); print('and on again');
    setTextSize(1); setTextColor(MUTED(), CARD());
    setCursor(20, 162); print('If it keeps happening, flash the board again.');
    setCursor(20, 176); print('Card reading still works without Wi-Fi.');
  }
  drawBentoBottomBar(apOk ? 'Starting up...' : 'Wi-Fi is off, the rest still works');
}

function hostScreensaver() {
  fillScreen(BG());
  drawHostTopBar('MCU CANTEEN');
  drawBentoCard(20, 40, 280, 156, BORDER(), CARD());
  drawBentoBottomBar('Tap a card or press the button to wake');
  drawCenteredText(160, 68, 5, TEXT(), CARD(), 8, CLOCK);
  drawCenteredText(160, 124, 1, MUTED(), CARD(), 20, DATE_H);
  drawCenteredText(160, 146, 1, GREEN(), CARD(), 34,
                   USED + ' of ' + DB_TOTAL + ' served   ' + (USED * 35) + ' baht');
  drawProgressBar(60, 170, 200, 6, Math.floor(USED * 100 / DB_TOTAL));
}

function hostLiveScan(status) {
  let screenBg, cardBg, bannerBg, bannerFg, accent, muted, title, foot, sid = SCAN_SID;
  if (status === 'APPROVED') {
    screenBg = 0x02E5; cardBg = 0x01C3; bannerBg = 0x07E0; bannerFg = 0x0000; accent = 0x07E0; muted = 0x87F0;
    title = 'APPROVED'; foot = 'Meal paid, 35 baht';
  } else if (status === 'DUPLICATE') {
    screenBg = 0x8200; cardBg = 0x4900; bannerBg = 0xFD20; bannerFg = 0x0000; accent = 0xFD20; muted = 0xFDC0;
    title = 'ALREADY SERVED'; foot = 'This student already ate today';
  } else if (status === 'TIME_CLOSED') {
    screenBg = 0x6200; cardBg = 0x3900; bannerBg = 0xFFE0; bannerFg = 0x0000; accent = 0xFFE0; muted = 0xFEE0;
    title = 'CLOSED NOW'; foot = 'Outside the service hours'; sid = 'Unknown';
  } else {
    screenBg = 0x8000; cardBg = 0x4800; bannerBg = 0xF800; bannerFg = 0xFFFF; accent = 0xF800; muted = 0xFCAE;
    title = 'NOT ON THE LIST'; foot = 'This card is not registered'; sid = 'Unknown';
  }
  fillScreen(screenBg);
  fillRect(0, 0, 320, 36, bannerBg);
  setTextSize(3); setTextColor(bannerFg, bannerBg);
  setCursor(Math.max(4, Math.floor((320 - title.length * 18) / 2)), 7); print(title);
  fillRoundRect(8, 44, 304, 188, 8, cardBg);
  drawRoundRect(8, 44, 304, 188, 8, accent); drawRoundRect(9, 45, 302, 186, 7, accent);
  setTextSize(1); setTextColor(muted, cardBg); setCursor(20, 54); print('STATION');
  setTextSize(2); setTextColor(0xFFFF, cardBg); setCursor(20, 66); printf('Station %d', 2);
  drawFastHLine(20, 88, 280, accent);
  setTextSize(1); setTextColor(muted, cardBg); setCursor(20, 96); print('STUDENT ID');
  setTextSize(3); setTextColor(0xFFFF, cardBg); setCursor(20, 110); print(sid);
  setTextSize(1); setTextColor(muted, cardBg); setCursor(20, 142); print('CARD');
  setTextSize(2); setTextColor(0xFFFF, cardBg); setCursor(20, 154); print(SCAN_UID);
  fillRoundRect(16, 184, 288, 36, 6, bannerBg);
  drawFitCenteredText(16, 184, 288, 36, foot, 1, bannerFg, bannerBg);
}

function hostCredit() {
  fillScreen(BG());
  drawHostTopBar('ABOUT');
  drawBentoCard(10, 36, 300, 166, BORDER(), CARD());
  drawCardLabel(22, 48, 'BUILT BY');
  setTextColor(TEXT(), CARD()); setTextSize(2); setCursor(22, 64); print('Kittiphan Rattanakorn');
  setTextColor(MUTED(), CARD()); setTextSize(1);
  setCursor(22, 92); print('Computer Technical Officer');
  setCursor(22, 108); print('Mahachulalongkornrajavidyalaya Phrae');
  drawFastHLine(22, 126, 276, BORDER());
  setTextColor(MUTED(), CARD());
  setCursor(22, 140); print('Firmware  v108.0.0');
  setCursor(22, 156); print('Board     ESP32-S3 N16R8, 8 MB PSRAM');
  setCursor(22, 172); print('Screen    2.8 inch 320x240');
  drawBentoBottomBar('Press the button to go back');
}


function drawRfidTapIcon(cx, cy, cardColor, waveColor, bgColor) {
  drawRoundRect(cx - 33, cy - 15, 42, 30, 5, cardColor);
  drawRoundRect(cx - 32, cy - 14, 40, 28, 4, cardColor);
  fillRoundRect(cx - 27, cy - 8, 12, 10, 2, cardColor);
  drawFastHLine(cx - 27, cy - 4, 12, bgColor);
  drawFastVLine(cx - 21, cy - 8, 10, bgColor);
  for (let i = 0; i < 3; i++) {
    const r = 9 + i * 6;
    drawRightArc(cx + 14, cy, r, waveColor);
    drawRightArc(cx + 14, cy, r + 1, waveColor);
  }
}

// [122.12.0] ต้องตรงกับ drawStandbyService() ใน StationScreen.h ทุกพิกัด
//   0 = พร้อมรับบัตร   1 = แม่ข่ายไม่ตอบ   2 = เครื่องอ่านบัตรไม่ตอบ
function drawStandbyService(svc) {
  const bg = CARD();
  let wave, noteColor, head, note;
  if (svc === 1) {
    wave = YELLOW(); noteColor = YELLOW();
    head = 'SERVER OFFLINE'; note = 'Cannot serve now - please tell the staff';
  } else if (svc === 2) {
    wave = ROSE(); noteColor = ROSE();
    head = 'OUT OF SERVICE'; note = 'Card reader not responding - call staff';
  } else {
    wave = GREEN(); noteColor = MUTED();
    head = 'TAP YOUR CARD'; note = 'Free meal  35 baht  once a day';
  }
  fillRect(160 - 34, 72 - 23, 72, 47, bg);
  drawRfidTapIcon(160, 72, TEXT(), wave, bg);
  drawCenteredText(160, 104, 3, TEXT(), bg, 15, head);
  drawCenteredText(160, 139, 1, noteColor, bg, 44, note);
}

function stStandby(svc) {
  if (svc === undefined) svc = 0;
  fillScreen(BG());
  drawStationTopBar('STATION ' + ST_ID, svc !== 1);
  drawStationCard(16, 34, 288, 130, BORDER(), CARD());
  drawStandbyService(svc);
  drawStationCard(16, 172, 288, 40, BORDER(), CARD());
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(28, 178); print('SERVED TODAY');
  drawFixedText(28, 191, 2, TEXT(), CARD(), 4, String(ST_SERVED));
  drawFixedText(196, 191, 2, TEXT(), CARD(), 8, CLOCK);
  drawStationBottomBar('Page 1/3    Press the button for the next page');
}

function stStats() {
  fillScreen(BG());
  drawStationTopBar('STATION ' + ST_ID);

  drawStationCard(6, 30, 150, 172, BORDER(), CARD());
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(14, 40); print('SERVED TODAY');
  drawFixedText(14, 62, 4, TEXT(), CARD(), 3, String(ST_SERVED));
  drawFixedText(14, 112, 3, GREEN(), CARD(), 5, String(ST_SERVED * 35));
  drawFixedText(14, 140, 1, MUTED(), CARD(), 5, 'baht');
  setTextColor(MUTED(), CARD()); setCursor(14, 174); print('35 baht per student');

  drawStationCard(164, 30, 150, 172, BORDER(), CARD());
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(172, 40); print('THIS STATION');
  setTextColor(TEXT(), CARD()); setTextSize(4); setCursor(172, 62); print(String(ST_ID));
  drawStationPillBadge(172, 118, 96, 20, 'ONLINE', dark ? 0x0000 : 0xFFFF, GREEN());
  drawFixedText(172, 166, 2, TEXT(), CARD(), 8, CLOCK);

  drawStationBottomBar('Page 2/3    Press the button for the next page');
}

function stDiagnostics() {
  fillScreen(BG());
  drawStationTopBar('SYSTEM');
  drawStationCard(6, 30, 308, 172, BORDER(), CARD());
  setTextColor(MUTED(), CARD()); setTextSize(1);
  setCursor(16, 46);  print('Host link');
  setCursor(16, 72);  print('Station');
  setCursor(16, 98);  print('Chip');
  setCursor(16, 124); print('Battery');
  setCursor(16, 150); print('Served today');
  setCursor(16, 176); print('Address');
  setTextColor(TEXT(), CARD());
  setCursor(140, 72);  print('No. ' + ST_ID);
  setCursor(140, 176); print('A0:B7:65:2C:1E:44');
  setTextColor(GREEN(), CARD()); setCursor(140, 46); print('Connected  ' + ST_RSSI + ' dB');
  setTextColor(TEXT(), CARD()); setCursor(140, 98); print('45 C   cpu 12%');
  setCursor(140, 124); print(battPct(ST_V) + '%   ' + ST_V.toFixed(2) + ' V');
  setCursor(140, 150); print(ST_SERVED + ' meals   ' + (ST_SERVED * 35) + ' baht');
  drawStationBottomBar('Page 3/3    Press the button for the next page');
}

function stScreensaver() {
  fillScreen(BG());
  drawStationTopBar('STATION ' + ST_ID);
  drawStationCard(20, 36, 280, 162, BORDER(), CARD());
  drawStationBottomBar('Tap your card or press the button to wake');
  drawCenteredText(160, 62, 5, TEXT(), CARD(), 8, CLOCK);
  drawCenteredText(160, 118, 1, MUTED(), CARD(), 20, DATE_S);
  drawCenteredText(160, 146, 1, GREEN(), CARD(), 32,
                   ST_SERVED + ' served today   ' + (ST_SERVED * 35) + ' baht');
  drawCenteredText(160, 168, 1, MUTED(), CARD(), 16, 'battery ' + battPct(ST_V) + '%');
}

function stScanning() {
  fillScreen(BG());
  drawStationTopBar('PROCESSING CARD TAP');
  drawStationCard(10, 36, 300, 168, CYAN(), CARD());
  drawStationPillBadge(24, 48, 272, 22, 'RFID CARD DETECTED', dark ? 0x0000 : 0xFFFF, CYAN());
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(24, 84); print('ENCRYPTED CARD UID:');
  setTextColor(CYAN(), CARD()); setTextSize(2); setCursor(24, 104); print(SCAN_UID);
  setTextColor(TEXT(), CARD()); setTextSize(1); setCursor(24, 148); print('Transmitting payload to Central Host...');
  drawStationBottomBar('PLEASE WAIT FOR VERIFICATION');
}

function stResult(status) {
  let screenBg, cardBg, bannerBg, bannerFg, accent, muted, title, foot;
  let sid = SCAN_SID, name = 'Somchai Wongsa';
  if (status === 'SUCCESS') {
    screenBg = 0x02E5; cardBg = 0x01C3; bannerBg = 0x07E0; bannerFg = 0x0000; accent = 0x07E0; muted = 0x87F0;
    title = 'APPROVED'; foot = 'Meal paid, 35 baht. Enjoy your meal.';
  } else if (status === 'ALREADY_USED') {
    screenBg = 0x8200; cardBg = 0x4900; bannerBg = 0xFD20; bannerFg = 0x0000; accent = 0xFD20; muted = 0xFDC0;
    title = 'ALREADY SERVED'; foot = 'You already had your meal today.';
  } else {
    screenBg = 0x8000; cardBg = 0x4800; bannerBg = 0xF800; bannerFg = 0xFFFF; accent = 0xF800; muted = 0xFCAE;
    title = 'NOT ON THE LIST'; foot = 'Ask the staff to register this card.';
    sid = 'Unknown'; name = 'Unknown card';
  }
  fillScreen(screenBg);
  fillRect(0, 0, 320, 40, bannerBg);
  drawFitCenteredText(4, 0, 312, 40, title, 3, bannerFg, bannerBg);
  fillRoundRect(8, 46, 304, 140, 8, cardBg);
  drawRoundRect(8, 46, 304, 140, 8, accent);
  setTextSize(1); setTextColor(muted, cardBg); setCursor(20, 56); print('NAME');
  setTextSize(2); setTextColor(0xFFFF, cardBg); setCursor(20, 68); print(name.slice(0, 22));
  drawFastHLine(20, 94, 280, accent);
  setTextSize(1); setTextColor(muted, cardBg); setCursor(20, 102); print('STUDENT ID');
  setTextSize(3); setTextColor(0xFFFF, cardBg); setCursor(20, 114); print(sid);
  setTextSize(1); setTextColor(muted, cardBg);
  setCursor(20, 150); print('Station ' + ST_ID);
  setCursor(20, 164); print('Card ' + SCAN_UID);
  const stamp = '11:47:05';
  setTextColor(muted, cardBg); setCursor(296 - stamp.length * 6, 164); print(stamp);
  fillRoundRect(8, 192, 304, 40, 8, bannerBg);
  drawFitCenteredText(14, 192, 292, 40, foot, 1, bannerFg, bannerBg);
}



function stSyncProgress() {
  fillScreen(BG());
  drawStationTopBar('SYNCING OFFLINE RECORDS');
  drawStationCard(16, 46, 288, 140, CYAN(), CARD());
  drawFitCenteredText(28, 60, 264, 16, 'SENDING SAVED RECORDS TO THE HOST', 1, MUTED(), CARD());
  drawFitCenteredText(28, 86, 264, 32, '3 / 7', 4, CYAN(), CARD());
  const pct = Math.floor(3 * 100 / 7);
  drawRoundRect(40, 132, 240, 12, 5, BORDER());
  const w = Math.floor(pct * 236 / 100);
  if (w > 0) fillRoundRect(42, 134, w, 8, 4, CYAN());
  drawFitCenteredText(28, 156, 264, 16, 'PLEASE DO NOT TURN OFF THE DEVICE', 1, MUTED(), CARD());
  drawStationBottomBar('SYNCING | PLEASE WAIT');
}

function stSyncDone() {
  fillScreen(BG());
  drawStationTopBar('OFFLINE SYNC COMPLETE');
  drawStationCard(16, 50, 288, 132, GREEN(), CARD());
  drawFitCenteredText(28, 64, 264, 20, 'SAVED RECORDS HAVE BEEN SENT', 2, TEXT(), CARD());
  drawFitCenteredText(28, 100, 264, 24, 'ACCEPTED 6', 3, GREEN(), CARD());
  drawFitCenteredText(28, 140, 264, 16, 'REJECTED AS DUPLICATE: 1', 1, ROSE(), CARD());
  drawStationBottomBar('RETURNING TO THE MAIN PAGE...');
}

/* [122.12.0] สองกรณีที่ต่างกันโดยสิ้นเชิง ต้องตรงกับ displayOfflineAlert() ในเฟิร์มแวร์
   resultUnknown = false  ยังไม่ได้ส่งอะไรออกไป การันตีได้ว่าไม่มีการตัดสิทธิ์
   resultUnknown = true   ส่งไปแล้วไม่มีคำตอบ ไม่มีทางรู้ว่าตัดสิทธิ์ไปแล้วหรือยัง */
function stOffline(resultUnknown) {
  fillScreen(0x8000);
  drawStationCard(10, 16, 300, 208, 0xF800, 0x4800);
  fillRoundRect(24, 28, 272, 24, 3, 0xF800);
  drawFitCenteredText(24, 28, 272, 24,
    resultUnknown ? 'RESULT UNKNOWN' : 'SERVER OFFLINE', 1, 0xFFFF, 0xF800);
  setTextColor(0xFCAE, 0x4800); setTextSize(1); setCursor(24, 68); print('WHAT HAPPENED');
  setTextColor(0xF800, 0x4800); setTextSize(2);
  setCursor(24, 84);  print(resultUnknown ? 'No answer from' : 'Main computer');
  setCursor(24, 104); print(resultUnknown ? 'the main computer' : 'is not answering');
  setTextColor(0xFCAE, 0x4800); setTextSize(1); setCursor(24, 124); print('WHAT TO DO');
  setTextColor(0xFFFF, 0x4800);
  setCursor(24, 142); print(resultUnknown ? 'Your meal may or may not be recorded'
                                          : 'Nothing was taken from your quota');
  setCursor(24, 158); print(resultUnknown ? 'Ask the staff to check for you'
                                          : 'Please tell the staff');
  drawStationBottomBar(resultUnknown ? 'Tap your card again to see the real result'
                                     : 'Press the button to go back');
}

function stIdSetup() {
  fillScreen(BG());
  drawStationTopBar('STATION ID SETUP');
  drawStationCard(16, 36, 288, 166, YELLOW(), CARD());
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(32, 48); print('SELECT STATION ID');
  fillRect(130, 70, 64, 40, CARD());
  setTextColor(GREEN(), CARD()); setTextSize(4); setCursor(140, 72); printf('0%d', ST_ID);
  drawStationPillBadge(34, 124, 252, 22, 'TAP = NEXT ID (01-04)', dark ? 0x0000 : 0xFFFF, CYAN());
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(44, 151); print('RELEASE / WAIT 3 SEC TO SAVE');
  drawStationBottomBar('ID SETUP | AUTO SAVE');
  fillRect(70, 178, 180, 16, CARD());
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(82, 182); printf('AUTO SAVE IN %d SEC', 2);
}

function stThemeLocked() {
  fillScreen(BG());
  drawStationTopBar('DISPLAY MODE');
  drawStationCard(16, 52, 288, 128, CYAN(), CARD());
  drawFitCenteredText(28, 70, 264, 24, 'THEME IS SET BY THE HOST', 2, TEXT(), CARD());
  drawFitCenteredText(28, 104, 264, 16, 'ALL STATIONS SHARE ONE DISPLAY MODE', 1, MUTED(), CARD());
  drawFitCenteredText(28, 132, 264, 16, 'PRESS 3x AT THE HOST TERMINAL TO SWITCH', 1, MUTED(), CARD());
  drawStationBottomBar('RETURNING TO THE PREVIOUS PAGE...');
}

/* ===== ทะเบียนหน้าจอ ===== */
const SCREENS = [
  ['host-01-overview',      'Host · หน้า 1/3 — ภาพรวมโควตาและฮาร์ดแวร์', true,  hostPage0],
  ['host-02-stations',      'Host · หน้า 2/3 — สถานะจุดบริการ 4 จุด',     true,  hostPage1],
  ['host-03-activity',      'Host · หน้า 3/3 — รายการล่าสุดและเครือข่าย',  true,  hostPage2],
  ['host-04-screensaver',   'Host · โหมดพักหน้าจอ',                      true,  hostScreensaver],
  ['host-05-approved',      'Host · แจ้งผล APPROVED',                     true,  () => hostLiveScan('APPROVED')],
  ['host-06-duplicate',     'Host · แจ้งผล DUPLICATE',                    true,  () => hostLiveScan('DUPLICATE')],
  ['host-07-timeclosed',    'Host · แจ้งผล นอกเวลาให้บริการ',             true,  () => hostLiveScan('TIME_CLOSED')],
  ['host-08-rejected',      'Host · แจ้งผล บัตรไม่อยู่ในทะเบียน',          true,  () => hostLiveScan('REJECTED')],
  ['host-09-bootnet-ok',    'Host · แจ้งสถานะ Wi-Fi ตอนบูต (สำเร็จ)',      true,  () => hostBootNet(true)],
  ['host-10-bootnet-fail',  'Host · แจ้งสถานะ Wi-Fi ตอนบูต (ล้มเหลว)',     true,  () => hostBootNet(false)],
  ['host-11-credit',        'Host · หน้าเครดิตผู้พัฒนา',                  true,  hostCredit],
  ['host-12-overview-light','Host · หน้า 1/3 โหมดสว่าง',                  false, hostPage0],
  ['host-13-stations-light','Host · หน้า 2/3 โหมดสว่าง',                  false, hostPage1],
  ['stn-01-standby',        'Station · หน้า 1/3 — รอแตะบัตร',            true,  stStandby],
  ['stn-02-stats',          'Station · หน้า 2/3 — ยอดวันนี้',             true,  stStats],
  ['stn-03-diagnostics',    'Station · หน้า 3/3 — ตรวจสอบระบบ',          true,  stDiagnostics],
  ['stn-04-screensaver',    'Station · โหมดพักหน้าจอ',                    true,  stScreensaver],
  ['stn-05-scanning',       'Station · กำลังส่งข้อมูลบัตร',                true,  stScanning],
  ['stn-06-approved',       'Station · แจ้งผล APPROVED',                  true,  () => stResult('SUCCESS')],
  ['stn-07-duplicate',      'Station · แจ้งผล DUPLICATE',                 true,  () => stResult('ALREADY_USED')],
  ['stn-08-rejected',       'Station · แจ้งผล บัตรไม่อยู่ในทะเบียน',       true,  () => stResult('REJECTED')],
  ['stn-09-offline',        'Station · แตะบัตรตอนแม่ข่ายไม่ตอบ',            true,  () => stOffline(false)],
  ['stn-15-result-unknown', 'Station · ส่งแล้วไม่มีคำตอบ ไม่ทราบผล',        true,  () => stOffline(true)],
  ['stn-10-idsetup',        'Station · ตั้งหมายเลขสถานี',                  true,  stIdSetup],
  ['stn-11-standby-light',  'Station · หน้า 1/3 โหมดสว่าง',               false, () => stStandby(0)],
  ['stn-13-reader-down',    'Station · หน้า 1/3 — เครื่องอ่านบัตรไม่ตอบ',   true,  () => stStandby(2)],
  ['stn-14-server-offline','Station · หน้า 1/3 — แม่ข่ายไม่ตอบ',            true,  () => stStandby(1)],
  ['stn-12-themelocked',    'Station · ธีมถูกกำหนดจากแม่ข่าย',            true,  stThemeLocked],
];
window.SCREEN_LIST = SCREENS.map(([id, label]) => ({ id, label }));
window.renderScreen = function (i) {
  dark = SCREENS[i][2];
  g.setTransform(1, 0, 0, 1, 0, 0);
  SCREENS[i][3]();
};
