import { chromium } from '/opt/node22/lib/node_modules/playwright/index.mjs';
const b = await chromium.launch({ executablePath: '/opt/pw-browsers/chromium-1194/chrome-linux/chrome' });
const p = await b.newPage({ viewport: { width: 1000, height: 760 } });
const errs = [];
p.on('pageerror', e => errs.push(String(e)));
await p.goto('http://127.0.0.1:8099/tft.html', { waitUntil: 'load' });
await p.waitForTimeout(400);
const list = await p.evaluate(() => window.SCREEN_LIST);
for (let i = 0; i < list.length; i++) {
  await p.evaluate(n => window.renderScreen(n), i);
  await p.waitForTimeout(60);
  await p.locator('#c').screenshot({ path: './shots/tft-' + list[i].id + '.png' });
  console.log('✓', list[i].id);
}
if (errs.length) console.log('PAGE ERRORS:', errs.slice(0, 5));
console.log(JSON.stringify(list));
await b.close();
