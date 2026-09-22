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
  fillRect(12, 54, 140, 60, CARD());
  setTextColor(TEXT(), CARD()); setTextSize(4); setCursor(14, 56); print(String(used));
  setTextSize(1); setTextColor(MUTED(), CARD()); setCursor(14, 92); print('of ' + total + ' students');
  drawProgressBar(14, 106, 134, 6, Math.floor(used * 100 / total));
  fillRect(170, 54, 140, 60, CARD());
  setTextColor(GREEN(), CARD()); setTextSize(4); setCursor(172, 56); print(String(used * 35));
  setTextSize(1); setTextColor(MUTED(), CARD()); setCursor(172, 92); print('35 baht per student');
}
function hostTodayClock(clock) {
  fillRect(12, 148, 196, 34, CARD());
  setTextColor(TEXT(), CARD()); setTextSize(4); setCursor(14, 150); print(clock);
}
function hostTodayDate(date) {
  fillRect(12, 184, 190, 12, CARD());
  setTextSize(1); setTextColor(MUTED(), CARD()); setCursor(14, 186); print(date);
}
function hostTodayOpen(open) {
  const hours = '10:00-13:30';
  fillRect(210, 148, 100, 48, CARD());
  drawBentoPillBadge(214, 152, 94, 22, open ? 'OPEN' : 'CLOSED', dark ? 0x0000 : 0xFFFF,
                     open ? GREEN() : ROSE());
  setTextColor(MUTED(), CARD()); setTextSize(1);
  setCursor(214 + Math.floor((94 - hours.length * 6) / 2), 182); print(hours);
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
  const x = coords[i][0], y = coords[i][1], w = 150;
  fillRect(x + 6, y + 34, w - 12, 30, CARD());
  setTextColor(TEXT(), CARD()); setTextSize(3); setCursor(x + 8, y + 36); print(String(n));
  const numW = String(n).length * 18;
  setTextSize(1); setTextColor(MUTED(), CARD());
  setCursor(x + 14 + numW, y + 38); print('meals');
  setTextColor(GREEN(), CARD()); setCursor(x + 14 + numW, y + 52); print(n * 35 + ' baht');
}

/* ---------- station หน้าแรก ---------- */
function stnStandbyFull(served, clock) {
  fillScreen(BG());
  drawStationTopBar('POINT ' + ST_ID);
  drawStationCard(16, 34, 288, 130, BORDER(), CARD());
  drawFitCenteredText(24, 62, 272, 40, 'TAP YOUR CARD', 4, TEXT(), CARD());
  drawFitCenteredText(24, 112, 272, 16, 'Free meal  35 baht  once a day', 1, MUTED(), CARD());
  drawStationCard(16, 172, 288, 40, BORDER(), CARD());
  setTextColor(MUTED(), CARD()); setTextSize(1); setCursor(28, 178); print('SERVED TODAY');
  stnStandbyServed(served);
  stnStandbyClock(clock);
  drawStationBottomBar('Page 1/3    Press the button for the next page');
}
function stnStandbyServed(served) {
  fillRect(26, 188, 120, 18, CARD());
  setTextColor(TEXT(), CARD()); setTextSize(2); setCursor(28, 191); print(String(served));
}
function stnStandbyClock(clock) {
  fillRect(180, 184, 124, 18, CARD());
  setTextColor(TEXT(), CARD()); setTextSize(2);
  setCursor(292 - clock.length * 12, 186); print(clock);
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
  setTextColor(TEXT(), CARD()); setTextSize(4); setCursor(172, 62); print(String(ST_ID));
  stnStatsPill(online);
  stnStatsClock(clock);
  drawStationBottomBar('Page 2/3    Press the button for the next page');
}
function stnStatsServed(served) {
  fillRect(12, 60, 140, 36, CARD());
  setTextColor(TEXT(), CARD()); setTextSize(4); setCursor(14, 62); print(String(served));
  fillRect(12, 116, 140, 26, CARD());
  setTextColor(GREEN(), CARD()); setTextSize(3); setCursor(14, 118); print(String(served * 35));
  setTextSize(1); setTextColor(MUTED(), CARD());
  setCursor(14 + String(served * 35).length * 18 + 6, 134); print('baht');
}
function stnStatsPill(online) {
  fillRect(172, 118, 96, 20, CARD());
  drawStationPillBadge(172, 118, 96, 20, online ? 'ONLINE' : 'OFFLINE',
                       online ? (dark ? 0x0000 : 0xFFFF) : 0xFFFF, online ? GREEN() : ROSE());
}
function stnStatsClock(clock) {
  fillRect(170, 164, 138, 18, CARD());
  setTextColor(TEXT(), CARD()); setTextSize(2); setCursor(172, 166); print(clock);
}

window.CASES = [
  { id: 'host-today-served', full: () => hostTodayFull(9, 420, '11:47:05', 'Tue, 22 Sep 2026', true),
    start: () => hostTodayFull(187, 420, '11:47:05', 'Tue, 22 Sep 2026', true),
    patch: () => hostTodayServed(9, 420) },
  { id: 'host-today-clock', full: () => hostTodayFull(187, 420, '9:05:01', 'Tue, 22 Sep 2026', true),
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
  { id: 'stn-standby-clock', full: () => stnStandbyFull(72, '9:05:01'),
    start: () => stnStandbyFull(72, '11:47:05'),
    patch: () => stnStandbyClock('9:05:01') },
  { id: 'stn-stats-served', full: () => stnStatsFull(5, '11:47:05', true),
    start: () => stnStatsFull(72, '11:47:05', true),
    patch: () => stnStatsServed(5) },
  { id: 'stn-stats-offline', full: () => stnStatsFull(72, '11:47:05', false),
    start: () => stnStatsFull(72, '11:47:05', true),
    patch: () => stnStatsPill(false) },
  { id: 'stn-stats-clock', full: () => stnStatsFull(72, '9:05:01', true),
    start: () => stnStatsFull(72, '11:47:05', true),
    patch: () => stnStatsClock('9:05:01') },
];
