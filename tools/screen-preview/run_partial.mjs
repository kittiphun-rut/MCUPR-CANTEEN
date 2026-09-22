import { chromium } from '/opt/node22/lib/node_modules/playwright/index.mjs';
const b = await chromium.launch({executablePath:'/opt/pw-browsers/chromium-1194/chrome-linux/chrome'});
const p = await b.newPage({viewport:{width:1000,height:800}});
const errs=[]; p.on('pageerror',e=>errs.push(String(e)));
await p.goto('http://127.0.0.1:8099/tft.html',{waitUntil:'load'});
await p.waitForTimeout(300);

const results = await p.evaluate(() => {
  const out = [];
  const px = () => {
    const c = document.getElementById('c');
    return c.getContext('2d').getImageData(0,0,c.width,c.height).data;
  };
  for (const cs of window.CASES) {
    for (const theme of [true, false]) {
      dark = theme;
      g.setTransform(1,0,0,1,0,0); cs.full();
      const want = Uint8ClampedArray.from(px());
      g.setTransform(1,0,0,1,0,0); cs.start();
      // นับการล้างพื้นระหว่างวาดซ้ำ นาฬิกาเดินทุกวินาที ถ้ามีการล้างพื้นคือกะพริบแน่นอน
      let fills = 0;
      const realRect = window.fillRect, realRound = window.fillRoundRect,
            realScreen = window.fillScreen;
      window.fillRect = function(){ fills++; return realRect.apply(null, arguments) };
      window.fillRoundRect = function(){ fills++; return realRound.apply(null, arguments) };
      window.fillScreen = function(){ fills++; return realScreen.apply(null, arguments) };
      cs.patch();
      window.fillRect = realRect; window.fillRoundRect = realRound;
      window.fillScreen = realScreen;
      const got = px();
      let diff = 0, box = null;
      const c = document.getElementById('c'), S = c.width / 320;
      for (let i = 0; i < want.length; i += 4) {
        if (want[i]!==got[i] || want[i+1]!==got[i+1] || want[i+2]!==got[i+2]) {
          diff++;
          const q = i/4, x = Math.floor((q % c.width)/S), y = Math.floor(Math.floor(q / c.width)/S);
          if (!box) box = {x1:x, y1:y, x2:x, y2:y};
          else { box.x1=Math.min(box.x1,x); box.y1=Math.min(box.y1,y);
                 box.x2=Math.max(box.x2,x); box.y2=Math.max(box.y2,y); }
        }
      }
      out.push({id: cs.id, theme: theme?'มืด':'สว่าง', diff, box, fills, perSecond: !!cs.perSecond});
    }
  }
  return out;
});
await b.close();
let bad = 0;
for (const r of results) {
  const ok = r.diff === 0 && !(r.perSecond && r.fills > 0);
  if (!ok) bad++;
  console.log((ok?'✓ ':'✗ ') + r.id.padEnd(22) + r.theme.padEnd(7) +
    (ok ? ('ตรงกันทุกพิกเซล' + (r.perSecond ? ' · ล้างพื้น 0 ครั้ง' : ''))
        : (r.diff ? ('ต่างกัน '+r.diff+' พิกเซล ในกรอบ TFT x '+r.box.x1+'-'+r.box.x2+' y '+r.box.y1+'-'+r.box.y2)
                  : ('วาดทุกวินาทีแต่ยังล้างพื้น '+r.fills+' ครั้ง จะกะพริบ'))));
}
if (errs.length) console.log('PAGE ERRORS:', errs.slice(0,3));
console.log(bad ? ('\n*** มี '+bad+' กรณีที่วาดบางส่วนแล้วไม่เหมือนวาดใหม่ทั้งหน้า ***')
                : '\nทุกกรณี: วาดเฉพาะส่วนที่เปลี่ยน ได้ผลเหมือนวาดใหม่ทั้งหน้าเป๊ะ ไม่มีเศษพิกเซลค้าง');
