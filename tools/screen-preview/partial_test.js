/* จำลองการวาดบางส่วน แล้วเทียบกับการวาดใหม่ทั้งหน้า
   ถ้าช่องล้างเล็กเกินไป จะเหลือเศษพิกเซลค้าง แล้วสองภาพจะไม่ตรงกัน */

/* ---------- host หน้า TODAY ---------- */
function hostTodayFull(used, total, clock, date, open) {
  fillScreen(BG());
  drawHostTopBar('TODAY', 1);
  drawBentoCard(6, 32, 150, 88, BORDER(), CARD());   drawCardLabel(14, 40, 'MEALS SERVED');
  drawBentoCard(164, 32, 150, 88, BORDER(), CARD()); drawCardLabel(172, 40, 'BAHT PAID');
  drawBentoCard(6, 126, 308, 82, BORDER(), CARD());  drawCardLabel(14, 134, 'SERVICE');
  drawBentoBottomBar('Click: next page    Double click: sleep');
  hostTodayServed(used, total);
  hostTodayClock(clock);
  hostTodayDate(date);
  hostTodayOpen(open);
}
function hostTodayServed(used, total) {
  drawFixedText(14, 56, 4, TEXT(), CARD(), 4, String(used));
  drawFixedText(14, 92, 1, MUTED(), CARD(), 20, 'of ' + total + ' students');
  drawProgressBar(14, 106, 134, 6, Math.floor(used * 100 / total));
  drawFixedText(172, 56, 4, GREEN(), CARD(), 5, String(used * 35));
  drawFixedText(172, 92, 1, MUTED(), CARD(), 20, '35 baht per student');
}
function hostTodayClock(clock) {
  drawFixedText(14, 150, 4, TEXT(), CARD(), 8, clock);
}
function hostTodayDate(date) {
  drawFixedText(14, 186, 1, MUTED(), CARD(), 24, date);
}
function hostTodayOpen(open) {
  const hours = '10:00-13:30';
  fillRect(210, 148, 100, 48, CARD());
  drawBentoPillBadge(214, 152, 94, 22, open ? 'OPEN' : 'CLOSED', dark ? 0x0000 : 0xFFFF,
                     open ? GREEN() : ROSE());
  setTextColor(MUTED(), CARD()); setTextSize(1);
  setCursor(214 + Math.floor((94 - hours.length * 6) / 2), 186); print(hours);
}

/* ---------- host หน้า SHOPS (การ์ดใบเดียวพอ) ---------- */
const SHOP_NAMES = ['Chicken Rice & Drinks','Noodle House','Vegetarian Corner','Coffee & Bakery'];
function hostShopsFull(counts) {
  fillScreen(BG());
  drawHostTopBar('SHOPS', 2);
  drawBentoBottomBar('Click: next page    Double click: sleep');
  const coords = [[6,32],[164,32],[6,122],[164,122]];
  for (let i = 0; i < 4; i++) {
    const x = coords[i][0], y = coords[i][1], w = 150, h = 86;
    drawBentoCard(x, y, w, h, BORDER(), CARD());
    setTextColor(TEXT(), CARD()); setTextSize(1); setCursor(x + 8, y + 7);
    print(fitLabel(SHOP_NAMES[i], 22));
    hostShopValue(i, counts[i]);
    fillRect(x + 8, y + 20, 74, 12, CARD());
    drawStatusDot(x + 12, y + 25, NODES[i].on);
    setTextSize(1); setTextColor(MUTED(), CARD());
    setCursor(x + 22, y + 22); print('Point ' + (i + 1));
    fillRect(x + 8, y + 66, w - 16, 14, CARD());
    if (NODES[i].on) {
      const sPct = battPct(NODES[i].v);
      drawSignalBars(x + 8, y + 68, NODES[i].rssi, true, CARD());
      setTextColor(MUTED(), CARD()); setCursor(x + 40, y + 70); print('signal');
      setCursor(x + 92, y + 70); print(sPct + '%');
      drawMiniBattery(x + 120, y + 68, sPct);
    } else {
      setTextColor(ROSE(), CARD()); setCursor(x + 8, y + 70); print('Waiting...');
    }
  }
}
function hostShopValue(i, n) {
  const coords = [[6,32],[164,32],[6,122],[164,122]];
  const x = coords[i][0], y = coords[i][1];
  drawFixedText(x + 8, y + 36, 3, TEXT(), CARD(), 3, String(n));
  drawFixedText(x + 68, y + 38, 1, MUTED(), CARD(), 6, 'meals');
  drawFixedText(x + 68, y + 52, 1, GREEN(), CARD(), 10, n * 35 + ' baht');
}

/* ---------- station หน้าแรก ---------- */
function stnStandbyFull(served, clock, ready) {
  if (ready === undefined) ready = true;
  fillScreen(BG());
  drawStationTopBar('POINT ' + ST_ID);
  drawStationCard(16, 34, 288, 130, BORDER(), CARD());
  stnStandbyReady(ready);
  drawFitCenteredText(24, 100, 272, 32, 'TAP YOUR CARD', 4, TEXT(), CARD());
  drawStationCard(16, 172, 288, 40, BORDER(), CARD());
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(28, 178); print('SERVED TODAY');
  stnStandbyServed(served);
  stnStandbyClock(clock);
  drawStationBottomBar('Page 1/3    Press the button for the next page');
}
// [122.6.0] ต้องตรงกับ drawStandbyReadyState() ใน StationScreen.h ทุกพิกัด
function stnStandbyReady(ready) {
  const bg = CARD();
  fillRect(160 - 34, 72 - 23, 72, 47, bg);
  drawRfidTapIcon(160, 72, TEXT(), ready ? GREEN() : ROSE(), bg);
  drawCenteredText(160, 139, 1, ready ? MUTED() : ROSE(), bg, 44,
    ready ? 'Free meal  35 baht  once a day' : 'Card reader not responding - call staff');
}
function stnStandbyServed(served) {
  drawFixedText(28, 191, 2, TEXT(), CARD(), 4, String(served));
}
function stnStandbyClock(clock) {
  drawFixedText(196, 191, 2, TEXT(), CARD(), 8, clock);
}

/* ---------- station หน้าสอง ---------- */
function stnStatsFull(served, clock, online) {
  fillScreen(BG());
  drawStationTopBar('POINT ' + ST_ID);
  drawStationCard(6, 30, 150, 172, BORDER(), CARD());
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(14, 40); print('SERVED TODAY');
  stnStatsServed(served);
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(14, 174); print('35 baht per student');
  drawStationCard(164, 30, 150, 172, BORDER(), CARD());
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(172, 40); print('THIS POINT');
  drawFixedText(172, 62, 4, TEXT(), CARD(), 3, String(ST_ID));
  stnStatsPill(online);
  stnStatsClock(clock);
  drawStationBottomBar('Page 2/3    Press the button for the next page');
}
function stnStatsServed(served) {
  drawFixedText(14, 62, 4, TEXT(), CARD(), 3, String(served));
  drawFixedText(14, 112, 3, GREEN(), CARD(), 5, String(served * 35));
  drawFixedText(14, 140, 1, MUTED(), CARD(), 5, 'baht');
}
function stnStatsPill(online) {
  fillRect(172, 118, 96, 20, CARD());
  drawStationPillBadge(172, 118, 96, 20, online ? 'ONLINE' : 'OFFLINE',
                       online ? (dark ? 0x0000 : 0xFFFF) : 0xFFFF, online ? GREEN() : ROSE());
}
function stnStatsClock(clock) {
  drawFixedText(172, 166, 2, TEXT(), CARD(), 8, clock);
}

/* ชื่อร้านบนจอ ตัดที่ 22 ตัวอักษรเสมอ และต้องไม่ล้นออกนอกการ์ด */
function hostShopName(i, name) {
  const coords = [[6,32],[164,32],[6,122],[164,122]];
  const x = coords[i][0], y = coords[i][1];
  drawFixedText(x + 8, y + 7, 1, TEXT(), CARD(), 22, name);
}

window.CASES = [
  { id: 'host-today-served', full: () => hostTodayFull(9, 420, '11:47:05', 'Tue, 22 Sep 2026', true),
    start: () => hostTodayFull(187, 420, '11:47:05', 'Tue, 22 Sep 2026', true),
    patch: () => hostTodayServed(9, 420) },
  { id: 'host-today-clock', perSecond: true, full: () => hostTodayFull(187, 420, '9:05:01', 'Tue, 22 Sep 2026', true),
    start: () => hostTodayFull(187, 420, '11:47:05', 'Tue, 22 Sep 2026', true),
    patch: () => hostTodayClock('9:05:01') },
  { id: 'host-today-closed', full: () => hostTodayFull(187, 420, '11:47:05', 'Tue, 22 Sep 2026', false),
    start: () => hostTodayFull(187, 420, '11:47:05', 'Tue, 22 Sep 2026', true),
    patch: () => hostTodayOpen(false) },
  { id: 'host-shops-count', full: () => hostShopsFull([7, 72, 63, 49]),
    start: () => hostShopsFull([84, 72, 63, 49]),
    patch: () => hostShopValue(0, 7) },
  { id: 'stn-standby-served', full: () => stnStandbyFull(5, '11:47:05'),
    start: () => stnStandbyFull(72, '11:47:05'),
    patch: () => stnStandbyServed(5) },
  { id: 'stn-standby-clock', perSecond: true, full: () => stnStandbyFull(72, '9:05:01'),
    start: () => stnStandbyFull(72, '11:47:05'),
    patch: () => stnStandbyClock('9:05:01') },
  { id: 'stn-reader-down', full: () => stnStandbyFull(72, '11:47:05', false),
    start: () => stnStandbyFull(72, '11:47:05', true),
    patch: () => stnStandbyReady(false) },
  { id: 'stn-reader-back', full: () => stnStandbyFull(72, '11:47:05', true),
    start: () => stnStandbyFull(72, '11:47:05', false),
    patch: () => stnStandbyReady(true) },
  { id: 'stn-stats-served', full: () => stnStatsFull(5, '11:47:05', true),
    start: () => stnStatsFull(72, '11:47:05', true),
    patch: () => stnStatsServed(5) },
  { id: 'stn-stats-offline', full: () => stnStatsFull(72, '11:47:05', false),
    start: () => stnStatsFull(72, '11:47:05', true),
    patch: () => stnStatsPill(false) },
  { id: 'host-shop-name', full: () => { hostShopsFull([84, 72, 63, 49]); hostShopName(0, 'Noodle'); },
    start: () => { hostShopsFull([84, 72, 63, 49]);
                   hostShopName(0, 'Chicken Rice and Drinks Corner'); },
    patch: () => hostShopName(0, 'Noodle') },
  { id: 'stn-stats-clock', perSecond: true, full: () => stnStatsFull(72, '9:05:01', true),
    start: () => stnStatsFull(72, '11:47:05', true),
    patch: () => stnStatsClock('9:05:01') },
];
