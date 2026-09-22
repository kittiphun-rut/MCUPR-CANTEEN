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
      g.setTransform(1,0,0,1,0,0); cs.start(); cs.patch();
      const got = px();
      let diff = 0, first = null;
      for (let i = 0; i < want.length; i += 4) {
        if (want[i]!==got[i] || want[i+1]!==got[i+1] || want[i+2]!==got[i+2]) {
          diff++;
          if (!first) { const q=i/4; const c=document.getElementById('c');
            first = {x: q % c.width, y: Math.floor(q / c.width)}; }
        }
      }
      out.push({id: cs.id, theme: theme?'มืด':'สว่าง', diff, first});
    }
  }
  return out;
});
await b.close();
let bad = 0;
for (const r of results) {
  const ok = r.diff === 0;
  if (!ok) bad++;
  console.log((ok?'✓ ':'✗ ') + r.id.padEnd(22) + r.theme.padEnd(7) +
    (ok ? 'ตรงกันทุกพิกเซล' : ('ต่างกัน '+r.diff+' พิกเซล จุดแรกที่ '+JSON.stringify(r.first))));
}
if (errs.length) console.log('PAGE ERRORS:', errs.slice(0,3));
console.log(bad ? ('\n*** มี '+bad+' กรณีที่วาดบางส่วนแล้วไม่เหมือนวาดใหม่ทั้งหน้า ***')
                : '\nทุกกรณี: วาดเฉพาะส่วนที่เปลี่ยน ได้ผลเหมือนวาดใหม่ทั้งหน้าเป๊ะ ไม่มีเศษพิกเซลค้าง');
