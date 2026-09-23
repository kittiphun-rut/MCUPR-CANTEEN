/**
 * @file      WebDashboard.h
 * @brief     หน้าเว็บทั้งหมดของเครื่องแม่ข่าย: หน้าเข้าสู่ระบบ แดชบอร์ดเจ้าหน้าที่ และจอสาธารณะบนทีวี
 * @version   113.8.0
 * @date      2026-09-23
 * @author    Kittiphan Rattanakorn <kittiphun.rut@mcu.ac.th>
 *
 * @par Organization
 * มหาวิทยาลัยมหาจุฬาลงกรณราชวิทยาลัย วิทยาเขตแพร่
 *
 * @par Description
 * CSS กับ JavaScript เก็บไว้ใน PROGMEM แล้วเสิร์ฟแยกที่ /s.css และ /a.js
 * เบราว์เซอร์จึงแคชไว้ได้ และไม่ต้องต่อสตริงยาวสองหมื่นตัวอักษรในแรมทุกครั้งที่เปิดหน้า
 * หน้า / จึงเหลือแค่โครงเล็ก ๆ ส่วนตัวเลขสดมาจาก /api/dashboard ทุกสองวินาที
 *
 * @par Bilingual
 * ข้อความอังกฤษเขียนไว้ใน HTML ตามปกติแล้วติดป้าย data-i="คีย์"
 * ส่วนคำไทยอยู่ในพจนานุกรม TH ชุดเดียวที่หัวของ DASH_JS
 * ไม่ได้เขียนซ้ำสองชุดในทุกบรรทัดแบบเฟิร์มแวร์รุ่นเก่า
 * เพิ่มข้อความใหม่ทีหลังจึงแก้แค่สองที่ และถ้าลืมเติมคำแปล
 * หน้าเว็บจะแสดงภาษาอังกฤษไว้ก่อน ไม่พังและไม่เป็นช่องว่าง
 *
 * @par Public Display
 * DISPLAY_HTML คือหน้าจอสาธารณะที่ /display เปิดค้างไว้บนทีวีในโรงอาหาร
 * ไม่ต้องเข้าสู่ระบบ ใครเดินผ่านก็เห็น จึงแสดงได้เฉพาะยอดรวม ชื่อร้าน
 * และรหัสนิสิตที่ปิดบังเหลือห้าหลักแรกเท่านั้น
 * ห้ามเพิ่มชื่อนิสิต หมายเลขบัตร เลขอ้างอิง หรือข้อมูลฮาร์ดแวร์ลงในหน้านี้เด็ดขาด
 * ตรวจก่อน commit ด้วย tools/proto-check/privacycheck.py
 *
 * @par Revision History
 * | Version | Date | Change |
 * |---|---|---|
 * | 113.8.0 | 2026-09-23 | เปลี่ยนคำอังกฤษ Service points และ Point เป็น Stations และ Station คำไทยคงเดิม |
 * | 113.6.0 | 2026-09-23 | เพิ่ม DISPLAY_HTML จอสาธารณะสำหรับทีวี ใช้ชุดสีและรูปแบบเดียวกับแดชบอร์ด แสดงไทยคู่อังกฤษพร้อมกันโดยไม่ต้องสลับภาษา |
 * | 113.4.0 | 2026-09-23 | เพิ่มช่องตั้งชื่อร้านที่จะขึ้นบนจอ พร้อมคำอธิบายว่าต้องเป็นภาษาอังกฤษ |
 * | 113.1.0 | 2026-09-22 | เพิ่มระบบสองภาษา ไทย/อังกฤษ ทั้งแดชบอร์ดและหน้าเข้าสู่ระบบ |
 * | 113.0.0 | 2026-09-22 | แยกออกมาจากไฟล์หลัก แล้วเขียนหน้าเว็บใหม่ทั้งหมด สี่แท็บ สองโหมดสี และย้าย CSS/JS ไปอยู่ใน PROGMEM |
 * | 107.0.1 | 2026-09-21 | ต้นฉบับที่ใช้เป็นจุดเริ่ม เก็บสำเนาไว้ที่ original/ |
 *
 * @warning  ถูก #include ท้ายไฟล์หลักก่อน setup() ห้ามย้ายขึ้นไปบนสุด
 * @warning  เวลาแก้ HTML ที่อยู่ในสตริง C++ ระวังลบเครื่องหมายคำพูดเปิดทิ้ง
 *           เคยพลาดมาแล้ว ตรวจด้วย tools/proto-check/bracecheck.py ก่อน commit
 */

#pragma once

// ============================================================================
// สไตล์ชีต — สองโหมด สว่าง/มืด สลับด้วย data-theme บน <html>
// สีทั้งหมดเป็นตัวแปร CSS ชุดเดียว แก้ที่เดียวเปลี่ยนทั้งหน้า
// ============================================================================
// [113.0.0] เพิ่ม: เก็บสไตล์ชีตไว้ในแฟลช แล้วเสิร์ฟแยกที่ /s.css เบราว์เซอร์แคชได้
static const char DASH_CSS[] PROGMEM = R"css(*{box-sizing:border-box;margin:0;padding:0}
:root{
  color-scheme:light;
  --bg:#f2f1ee; --panel:#fcfcfb; --line:#dedcd5;
  --ink:#0b0b0b; --ink2:#52514e; --ink3:#78766f;
  --bar:#2a78d6; --track:#e6e4dd;
  --ok:#0ca30c; --off:#d03b3b; --warn:#fab219;
  --r:14px;
}
:root[data-theme="dark"]{
  color-scheme:dark;
  --bg:#111110; --panel:#1a1a19; --line:#33332f;
  --ink:#ffffff; --ink2:#c3c2b7; --ink3:#94938a;
  --bar:#3987e5; --track:#2b2b28;
  --ok:#0ca30c; --off:#d03b3b; --warn:#fab219;
}
body{background:var(--bg);color:var(--ink);
  font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
  font-size:16px;line-height:1.45;-webkit-font-smoothing:antialiased}
.wrap{max-width:1080px;margin:0 auto;padding:16px}
.top{display:flex;align-items:center;gap:12px;padding:14px 0 18px}
.brand{font-size:1.05rem;font-weight:700;letter-spacing:-.01em}
.brand span{color:var(--ink3);font-weight:500}
.top .sp{flex:1}
.btn{border:1px solid var(--line);background:var(--panel);color:var(--ink);
  border-radius:10px;padding:9px 14px;font-size:.9rem;font-weight:600;
  cursor:pointer;font-family:inherit;text-decoration:none;display:inline-block}
.btn:hover{border-color:var(--ink3)}
.btn.main{background:var(--bar);border-color:var(--bar);color:#fff}
.btn.danger{color:var(--off)}
.btn.sm{padding:5px 10px;font-size:.8rem;border-radius:8px}
.panel{background:var(--panel);border:1px solid var(--line);
  border-radius:var(--r);padding:18px;margin-bottom:16px}
.ptitle{font-size:.72rem;font-weight:700;letter-spacing:.12em;
  text-transform:uppercase;color:var(--ink3);margin-bottom:14px}
.note{color:var(--ink2);font-size:.9rem;margin-bottom:12px}
.hint{color:var(--ink3);font-size:.82rem;margin-top:8px}
.tiles{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}
.tile{background:var(--panel);border:1px solid var(--line);
  border-radius:var(--r);padding:16px 18px}
.tile .n{font-size:2.3rem;font-weight:700;letter-spacing:-.03em;
  line-height:1.1;font-variant-numeric:tabular-nums}
.tile .k{font-size:.72rem;font-weight:700;letter-spacing:.12em;
  text-transform:uppercase;color:var(--ink3);margin-top:6px}
.shop{display:grid;grid-template-columns:200px 1fr 96px 112px;
  gap:14px;align-items:center;padding:11px 0}
.shop+.shop{border-top:1px solid var(--line)}
.shop .nm{font-weight:600;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.shop .nm small{display:block;font-weight:400;font-size:.76rem;color:var(--ink3)}
.track{background:var(--track);border-radius:4px;height:14px;overflow:hidden}
.fill{background:var(--bar);height:100%;border-radius:4px;min-width:2px;
  transition:width .4s ease}
.num{text-align:right;font-variant-numeric:tabular-nums;font-weight:600}
.num small{color:var(--ink3);font-weight:400}
.chips{display:flex;gap:10px;flex-wrap:wrap}
.chip{display:flex;align-items:center;gap:7px;border:1px solid var(--line);
  border-radius:999px;padding:6px 13px;font-size:.85rem;font-weight:600}
.dot{width:9px;height:9px;border-radius:50%;flex:none}
.chip.on .dot{background:var(--ok)} .chip.off .dot{background:var(--off)}
.chip.off{color:var(--ink3)}
label{display:block;font-size:.78rem;font-weight:600;color:var(--ink2);margin:10px 0 5px}
input,select{width:100%;padding:9px 11px;border:1px solid var(--line);
  border-radius:9px;background:var(--bg);color:var(--ink);
  font-family:inherit;font-size:.92rem}
input:focus,select:focus{outline:2px solid var(--bar);outline-offset:-1px}
table{width:100%;border-collapse:collapse;font-size:.9rem}
th{text-align:left;font-size:.7rem;letter-spacing:.1em;text-transform:uppercase;
  color:var(--ink3);padding:8px 10px;border-bottom:1px solid var(--line)}
td{padding:9px 10px;border-bottom:1px solid var(--line)}
tr:last-child td{border-bottom:0}
.row{display:flex;gap:10px;flex-wrap:wrap;align-items:flex-end}
.row>*{flex:1;min-width:150px}
.row .fit{flex:0 0 auto;min-width:0}
.row .gap{flex:1;min-width:0}
.tag{display:inline-block;padding:2px 9px;border-radius:999px;
  font-size:.74rem;font-weight:700}
.tag.y{background:var(--ok);color:#fff}
.tag.n{border:1px solid var(--line);color:var(--ink3)}
.tag.t{border:1px solid var(--warn);color:var(--warn)}
.tabs{display:flex;gap:6px;margin-bottom:16px;flex-wrap:wrap}
.tab{border:1px solid var(--line);background:transparent;color:var(--ink2);
  border-radius:10px;padding:8px 15px;font-size:.88rem;font-weight:600;
  cursor:pointer;font-family:inherit}
.tab[aria-selected="true"]{background:var(--ink);color:var(--bg);border-color:var(--ink)}
.view{display:none} .view.show{display:block}
#toast{position:fixed;left:50%;transform:translateX(-50%);bottom:24px;z-index:9;
  background:var(--ink);color:var(--bg);padding:11px 20px;border-radius:10px;
  font-size:.9rem;font-weight:600;opacity:0;transition:opacity .2s;pointer-events:none}
#toast.show{opacity:1}
@media(max-width:760px){
  .tiles{grid-template-columns:repeat(2,1fr)}
  .shop{grid-template-columns:1fr 84px;gap:8px}
  .shop .track{grid-column:1/-1}
  .tile .n{font-size:1.9rem}
  .hide-s{display:none}
}
)css";

// ============================================================================
// สคริปต์หน้าเว็บ — ดึง /api/dashboard ทุกสองวินาทีแล้ววาดเฉพาะตัวเลขที่เปลี่ยน
// ไม่มีไลบรารีภายนอก เพราะเครื่องนี้ไม่ได้ต่ออินเทอร์เน็ต
// ============================================================================
// [113.0.0] เพิ่ม: เก็บสคริปต์ไว้ในแฟลช แล้วเสิร์ฟแยกที่ /a.js
// [113.1.0] เพิ่ม: พจนานุกรมภาษาไทยและกลไกสลับภาษาอยู่ที่หัวของสคริปต์นี้
static const char DASH_JS[] PROGMEM = R"js(/* ---- พจนานุกรมภาษาไทย คีย์ตรงกับ data-i ใน HTML ----
   ไม่มีคีย์ไหน = ใช้ข้อความอังกฤษเดิมในหน้า ไม่พังและไม่เป็นช่องว่าง */
var TH = {
  sub:'สวัสดิการอาหารกลางวัน', signout:'ออกจากระบบ',
  lang:'EN', themeL:'โหมดสว่าง', themeD:'โหมดมืด',
  today:'วันนี้', students:'นิสิต', shops:'ร้านค้า', settings:'ตั้งค่า',
  kServed:'จ่ายแล้ว', kBaht:'เป็นเงิน', kLeft:'คงเหลือ',
  open:'เปิดบริการ', closed:'ปิดบริการ',
  salesby:'ยอดขายรายร้าน', points:'จุดบริการ', point:'จุดที่',
  online:'ออนไลน์', offline:'ออฟไลน์',
  meals:'จาน', baht:'บาท', ofN:'จากทั้งหมด', studentsN:'คน',
  sSearch:'ค้นหาชื่อหรือรหัสนิสิต', sAdd:'เพิ่มนิสิต', sExport:'ดาวน์โหลด CSV',
  hId:'รหัสนิสิต', hName:'ชื่อ-สกุล', hCard:'บัตร', hStatus:'สถานะ', hShop:'ร้าน',
  stServed:'รับแล้ว', stWait:'ยังไม่รับ', stTemp:'บัตรชั่วคราว',
  aServe:'ตัดสิทธิ์', aTemp:'บัตรชั่วคราว', aRevoke:'คืนบัตร', aRemove:'ลบ',
  sNone:'ไม่พบนิสิตที่ค้นหา', sLoad:'กำลังโหลด...', page:'หน้า',
  prev:'ก่อนหน้า', next:'ถัดไป',
  shTitle:'ชื่อร้านและผู้ประกอบการ', shName:'ชื่อร้านที่', shOwner:'ผู้ประกอบการ',
  shScreen:'ชื่อที่ขึ้นบนจอ', shSave:'บันทึกร้านค้า',
  shHint:'ช่องขวาสุดคือชื่อที่ขึ้นบนจอของเครื่องแม่ข่าย ต้องเป็นภาษาอังกฤษหรือตัวเลขเท่านั้น '
    + 'เพราะฟอนต์ในตัวของจอไม่มีตัวอักษรไทย ถ้าเว้นว่างไว้จอจะขึ้นว่า Shop 1 ถึง Shop 4',
  dTitle:'การแสดงผลของทุกจุดบริการ',
  dNote:'เครื่องแม่ข่ายเป็นผู้กำหนดธีมและการพักหน้าจอให้ทุกจุดบริการ',
  dTheme:'ธีม', dDark:'มืด', dLight:'สว่าง',
  dScreens:'หน้าจอ', dAwake:'เปิดอยู่', dSleep:'พักหน้าจอ',
  hrsTitle:'เวลาให้บริการ', hrsLimit:'การจำกัดเวลา',
  hrsOn:'ให้บริการเฉพาะช่วงเวลานี้', hrsOff:'ให้บริการตลอดวัน',
  hrsOpen:'เริ่ม', hrsClose:'สิ้นสุด', hrsSave:'บันทึกเวลา',
  clkTitle:'นาฬิกาของเครื่อง', clkDate:'วันที่', clkTime:'เวลา', clkSet:'ตั้งนาฬิกา',
  clkHint:'นาฬิกาเป็นตัวกำหนดว่าเปิดบริการเมื่อไร และข้อมูลนับเป็นของวันไหน',
  impTitle:'นำเข้ารายชื่อนิสิต', impBtn:'อัปโหลด CSV',
  impHint:'คอลัมน์: รหัสนิสิต, ชื่อ-สกุล, หมายเลขบัตร — รายชื่อเดิมจะถูกอัปเดต รายชื่อใหม่จะถูกเพิ่ม',
  stfTitle:'เจ้าหน้าที่ที่เข้าระบบได้', stfUser:'ชื่อผู้ใช้', stfName:'ชื่อที่แสดง',
  stfPass:'รหัสผ่าน', stfAdd:'เพิ่มเจ้าหน้าที่', stfRemove:'ลบ',
  stfHint:'ได้สูงสุดสามบัญชี และควรเปลี่ยนรหัสผ่านเริ่มต้นก่อนเปิดใช้งานจริง',
  arcTitle:'ไฟล์ประวัติย้อนหลัง', arcFile:'ไฟล์', arcSize:'ขนาด',
  arcDl:'ดาวน์โหลด', arcDel:'ลบ', arcNone:'ยังไม่มีไฟล์ประวัติ',
  eodTitle:'ปิดยอดประจำวัน',
  eodNote:'เก็บข้อมูลของวันนี้เข้าแฟ้ม แล้วคืนสิทธิ์ให้นิสิตทุกคน',
  eodBtn:'ปิดยอดวันนี้',
  tDisp:'สั่งไปยังทุกจุดบริการแล้ว', tRec:'บันทึกแล้ว', tRm:'ลบแล้ว',
  tTmp:'ผูกบัตรชั่วคราวแล้ว', tRev:'คืนบัตรแล้ว', tAdd:'เพิ่มนิสิตแล้ว',
  tDay:'ปิดยอดวันนี้แล้ว', tArc:'ลบไฟล์แล้ว', tSaved:'บันทึกแล้ว',
  tFail:'บันทึกไม่สำเร็จ',
  pShop:'นิสิตคนนี้รับอาหารจากร้านไหน (1-4)', pId:'รหัสนิสิต', pName:'ชื่อ-สกุล',
  pCard:'หมายเลขบัตร (ไม่มีให้เว้นว่าง)', pRmStu:'ลบนิสิตรหัส ',
  pClose:'ปิดยอดวันนี้และคืนสิทธิ์ให้นิสิตทุกคนใช่หรือไม่',
  pTmp:'แตะบัตรชั่วคราว แล้วพิมพ์หมายเลขบัตร', pArc:'ลบไฟล์ ',
  pStf:'ลบเจ้าหน้าที่ '
};
var lang = 'en';
/* แปลข้อความหนึ่งคำ ถ้าไม่มีคำแปลให้คืนค่าที่ส่งมา */
function t(k, en){ return (lang === 'th' && TH[k]) ? TH[k] : en; }
/* วาดข้อความทั้งหน้าใหม่ตามภาษาที่เลือก
   ครั้งแรกจะจำข้อความอังกฤษเดิมไว้ที่ตัว element เอง จะได้สลับกลับได้ตรง ๆ */
function paintLang(l){
  lang = (l === 'th') ? 'th' : 'en';
  document.documentElement.setAttribute('lang', lang);
  lsSet('lang', lang);
  document.querySelectorAll('[data-i]').forEach(function(e){
    if (e._en === undefined) e._en = e.textContent;
    e.textContent = t(e.getAttribute('data-i'), e._en);
  });
  document.querySelectorAll('[data-ip]').forEach(function(e){
    if (e._ep === undefined) e._ep = e.placeholder;
    e.placeholder = t(e.getAttribute('data-ip'), e._ep);
  });
  var b = el('langBtn'); if (b) b.textContent = (lang === 'th') ? 'EN' : 'ไทย';
  paintTheme(document.documentElement.getAttribute('data-theme') || 'dark');
  var sf = el('shopForm'); if (sf) sf.dataset.filled = '';
  refresh();
  if (el('v-students') && el('v-students').classList.contains('show')) loadStudents();
}
function toggleLang(){ paintLang(lang === 'th' ? 'en' : 'th'); }

function lsGet(k){try{return localStorage.getItem(k)}catch(e){return null}}
function lsSet(k,v){try{localStorage.setItem(k,v)}catch(e){}}
function el(id){return document.getElementById(id)}
function txt(id,v){var e=el(id); if(e) e.textContent=v}
function esc(s){return String(s==null?'':s).replace(/[&<>"']/g,function(c){
  return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]})}
function toast(m){var t=el('toast'); if(!t)return; t.textContent=m; t.classList.add('show');
  clearTimeout(t._h); t._h=setTimeout(function(){t.classList.remove('show')},2200)}
function post(url,data){
  var b=new URLSearchParams(); for(var k in data) b.append(k,data[k]);
  return fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:b.toString()}).then(function(r){
      if(r.status===401){location.href='/login';throw 0}
      return r.text().then(function(t){try{return JSON.parse(t)}catch(e){return{}}})});
}

/* ---- theme: remembered in this browser, and pushed to every station ---- */
function paintTheme(th){
  document.documentElement.setAttribute('data-theme',th);
  var b=el('themeBtn');
  if(b) b.textContent = (th==='dark') ? t('themeL','Light mode') : t('themeD','Dark mode');
}
function toggleTheme(){
  var th=document.documentElement.getAttribute('data-theme')==='dark'?'light':'dark';
  paintTheme(th); lsSet('theme',th);
  var sel=el('dispDark'); if(sel) sel.value=(th==='dark')?'1':'0';
  post('/api/display',{dark:(th==='dark')?'1':'0'});
}
function show(name,btn){
  ['today','students','shops','settings'].forEach(function(v){
    var s=el('v-'+v); if(s) s.classList.toggle('show', v===name)});
  document.querySelectorAll('.tab').forEach(function(t){t.setAttribute('aria-selected','false')});
  if(btn) btn.setAttribute('aria-selected','true');
  lsSet('tab',name);
  if(name==='students') loadStudents();
}

/* ---- live numbers ---- */
var peak=1;
function refresh(){
  return fetch('/api/dashboard').then(function(r){
    if(r.status===401){location.href='/login';throw 0}
    return r.json();
  }).then(function(d){
    txt('tServed', d.served);
    txt('tAmount', d.amount.toLocaleString());
    txt('tLeft',   d.left);
    txt('tClock',  d.clock);
    txt('tState',  (d.open ? t('open','Open') : t('closed','Closed'))+' · '+d.window);
    txt('tDate',   d.date);

    peak=Math.max(1, d.shops.reduce(function(a,s){return Math.max(a,s.meals)},0));
    var h='';
    d.shops.forEach(function(s){
      var pct=Math.round(s.meals*100/peak);
      h+='<div class="shop">'
       + '<div class="nm">'+esc(s.name)+'<small>'+esc(s.owner)+'</small></div>'
       + '<div class="track"><div class="fill" style="width:'+pct+'%"></div></div>'
       + '<div class="num">'+s.meals+' <small>'+t('meals','meals')+'</small></div>'
       + '<div class="num">'+s.amount.toLocaleString()+' <small>'+t('baht','THB')+'</small></div>'
       + '</div>';
    });
    el('shopList').innerHTML=h;

    el('stationList').innerHTML=d.stations.map(function(s){
      return '<span class="chip '+(s.online?'on':'off')+'"><i class="dot"></i>'
           + t('point','Station')+' '+s.id+' · '
           + (s.online ? t('online','Online') : t('offline','Offline'))+'</span>';
    }).join('');

    var sd=el('dispDark'), ss=el('dispSaver');
    if(sd && document.activeElement!==sd) sd.value=d.dark?'1':'0';
    if(ss && document.activeElement!==ss) ss.value=d.saver?'1':'0';

    var sf=el('shopForm');
    if(sf && !sf.dataset.filled){
      sf.dataset.filled='1';
      sf.innerHTML=d.shops.map(function(s,i){
        return '<div class="row" style="margin-bottom:10px">'
         + '<div><label>'+t('shName','Shop')+' '+(i+1)+(lang==='th'?'':' name')+'</label>'
         + '<input name="sname'+i+'" value="'+esc(s.name)+'" required></div>'
         + '<div><label>'+t('shOwner','Owner')+'</label>'
         + '<input name="vname'+i+'" value="'+esc(s.owner)+'"></div>'
         + '<div><label>'+t('shScreen','Name on the screen')+'</label>'
         + '<input name="dname'+i+'" value="'+esc(s.screen||'')+'" maxlength="22" '
         + 'placeholder="Shop '+(i+1)+'"></div></div>';
      }).join('');
    }
  }).catch(function(){});
}

/* ---- display mode for every station (host decides, stations obey) ---- */
function setDisplay(){
  post('/api/display',{dark:el('dispDark').value, saver:el('dispSaver').value})
    .then(function(){
      paintTheme(el('dispDark').value==='1'?'dark':'light');
      lsSet('theme',el('dispDark').value==='1'?'dark':'light');
      toast(t('tDisp','Stations updated')); refresh();
    });
}

/* ---- students ---- */
var page=1, pages=1, term='';
function loadStudents(){
  fetch('/api/students?page='+page+'&limit=20&search='+encodeURIComponent(term))
    .then(function(r){if(r.status===401){location.href='/login';throw 0}return r.json()})
    .then(function(d){
      pages=d.totalPages||1; page=d.currentPage||1;
      txt('pageInfo', t('page','Page')+' '+page+' '+t('ofN','of')+' '+pages
                     +' · '+(d.totalItems||0)+' '+t('studentsN','students'));
      var b=el('stuBody');
      if(!d.students || !d.students.length){
        b.innerHTML='<tr><td colspan="6" style="color:var(--ink3)">'
                   + t('sNone','No students found')+'</td></tr>'; return;
      }
      b.innerHTML=d.students.map(function(s){
        var id=esc(s.id);
        var status=s.claimed?'<span class="tag y">'+t('stServed','Served')+'</span>'
                            :(s.isTemp?'<span class="tag t">'+t('stTemp','Temp card')+'</span>'
                                      :'<span class="tag n">'+t('stWait','Waiting')+'</span>');
        var act='';
        if(!s.claimed) act+='<button class="btn sm" onclick="serve(\''+id+'\')">'+t('aServe','Serve')+'</button> ';
        act+= s.isTemp ? '<button class="btn sm" onclick="dropCard(\''+id+'\')">'+t('aRevoke','Revoke card')+'</button> '
                       : '<button class="btn sm" onclick="tempCard(\''+id+'\')">'+t('aTemp','Temp card')+'</button> ';
        act+='<button class="btn sm danger" onclick="delStudent(\''+id+'\')">'+t('aRemove','Remove')+'</button>';
        return '<tr><td><b>'+id+'</b></td><td>'+esc(s.name)+'</td>'
         + '<td class="hide-s" style="color:var(--ink3)">'+esc(s.uid||'—')+'</td>'
         + '<td>'+status+'</td>'
         + '<td class="hide-s">'+esc(s.claimed?(s.shopName||'—'):'—')+'</td>'
         + '<td style="text-align:right;white-space:nowrap">'+act+'</td></tr>';
      }).join('');
    }).catch(function(){});
}
var findTimer;
function findStudents(v){clearTimeout(findTimer);
  findTimer=setTimeout(function(){term=v.trim();page=1;loadStudents()},300)}
function turnPage(d){var n=page+d; if(n<1||n>pages)return; page=n; loadStudents()}
function serve(id){
  var shop=prompt(t('pShop','Which shop served this student? (1-4)'),'1'); if(!shop)return;
  post('/manual-claim',{id:id,station:shop})
    .then(function(){toast(t('tRec','Recorded'));loadStudents();refresh()});
}
function delStudent(id){
  if(!confirm(t('pRmStu','Remove student ')+id+'?'))return;
  post('/api/student/delete',{id:id}).then(function(){toast(t('tRm','Removed'));loadStudents()});
}
function tempCard(id){
  var uid=prompt(t('pTmp','Tap the temporary card, then type its number')); if(!uid)return;
  post('/bind-temp',{id:id,uid:uid})
    .then(function(){toast(t('tTmp','Temporary card assigned'));loadStudents()});
}
function dropCard(id){
  post('/api/tempcard/remove',{id:id})
    .then(function(){toast(t('tRev','Card revoked'));loadStudents()});
}
function openAdd(){
  var id=prompt(t('pId','Student ID')); if(!id)return;
  var nm=prompt(t('pName','Full name')); if(!nm)return;
  var uid=prompt(t('pCard','Card number (leave empty if none)'))||'';
  post('/api/student/save',{oldStudentId:'',studentId:id,fullName:nm,uid:uid})
    .then(function(){toast(t('tAdd','Student added'));loadStudents();refresh()});
}
function closeDay(){
  if(!confirm(t('pClose','Close today and give every student their allowance back?')))return;
  post('/reset',{}).then(function(){toast(t('tDay','Day closed'));refresh();loadStudents()});
}
function delArchive(f){
  if(!confirm(t('pArc','Delete ')+f+'?'))return;
  post('/api/archive/delete',{file:f}).then(function(){
    var r=el('arc-'+f); if(r) r.remove(); toast(t('tArc','Archive deleted'))});
}
function delAdmin(u){
  if(!confirm(t('pStf','Remove officer ')+u+'?'))return;
  post('/api/admin/delete',{username:u}).then(function(){location.reload()});
}

/* ---- forms submit without leaving the page ---- */
function bindForms(){
  document.querySelectorAll('form[data-ajax]').forEach(function(f){
    f.addEventListener('submit',function(ev){
      ev.preventDefault();
      fetch(f.getAttribute('action'),{method:'POST',body:new FormData(f)})
        .then(function(r){if(r.status===401){location.href='/login';throw 0}return r.text()})
        .then(function(){toast(t('tSaved','Saved')); refresh(); if(f.dataset.reload) location.reload()})
        .catch(function(){toast(t('tFail','Could not save'))});
    });
  });
}

/* ---- start ---- */
paintTheme(lsGet('theme')||'dark');
try{bindForms()}catch(e){}
paintLang(lsGet('lang')||'en');   /* paintLang เรียก refresh() ให้แล้วในตัว */
setInterval(refresh,2000);
(function(){
  var saved=lsGet('tab');
  if(saved && el('v-'+saved)){
    var idx=['today','students','shops','settings'].indexOf(saved);
    var tabs=document.querySelectorAll('.tab');
    if(idx>=0 && tabs[idx]) show(saved,tabs[idx]);
  }
})();
)js";

// ============================================================================
// [113.6.0] เพิ่ม: จอสาธารณะสำหรับทีวีในโรงอาหาร เปิดดูได้โดยไม่ต้องเข้าสู่ระบบ
//
// หน้านี้ไม่มีค่าอะไรที่ต้องแทนตอนสร้าง ข้อมูลทั้งหมดมาจาก /api/board
// จึงเก็บเป็นไฟล์นิ่งในแฟลชแล้วส่งตรงด้วย send_P ไม่ต้องสร้างสตริงในแรมเลย
//
// ขนาดตัวอักษรอิงหน่วย vmin ทั้งหน้า จึงปรับตามขนาดจอเองโดยไม่ต้องตั้งค่า
// ใช้ได้ตั้งแต่จอ 1366x768 ไปจนถึง 4K
//
// โหมดมืด/สว่างตามที่เครื่องแม่ข่ายกำหนด เหมือนจอของจุดบริการ
// ============================================================================
static const char DISPLAY_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MCU Canteen</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
:root{
  color-scheme:light;
  --bg:#f2f1ee; --panel:#fcfcfb; --line:#dedcd5;
  --ink:#0b0b0b; --ink2:#52514e; --ink3:#78766f;
  --bar:#2a78d6; --track:#e6e4dd;
  --ok:#0ca30c; --off:#d03b3b;
  --u:1vmin;
}
:root[data-theme="dark"]{
  color-scheme:dark;
  --bg:#111110; --panel:#1a1a19; --line:#33332f;
  --ink:#ffffff; --ink2:#c3c2b7; --ink3:#94938a;
  --bar:#3987e5; --track:#2b2b28;
}
html,body{height:100%}
body{background:var(--bg);color:var(--ink);overflow:hidden;
  font-family:"IBM Plex Sans Thai",system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
  font-size:calc(var(--u)*1.6);line-height:1.35;-webkit-font-smoothing:antialiased}
.wrap{height:100%;display:flex;flex-direction:column;gap:calc(var(--u)*1.4);
  padding:calc(var(--u)*2.2) calc(var(--u)*2.6)}

/* ---- top bar ---- */
.top{display:flex;align-items:flex-end;gap:calc(var(--u)*2)}
.brand{font-size:calc(var(--u)*3);font-weight:700;letter-spacing:-.01em;line-height:1.1}
.brand small{display:block;font-size:calc(var(--u)*1.7);font-weight:400;color:var(--ink3);
  margin-top:calc(var(--u)*.4)}
.top .sp{flex:1}
.now{text-align:right}
.clock{font-size:calc(var(--u)*5.2);font-weight:700;letter-spacing:-.02em;line-height:1;
  font-variant-numeric:tabular-nums}
.when{font-size:calc(var(--u)*1.7);color:var(--ink3);margin-top:calc(var(--u)*.6);
  display:flex;gap:calc(var(--u)*1.2);justify-content:flex-end;align-items:center}
.pill{border-radius:999px;padding:calc(var(--u)*.35) calc(var(--u)*1.3);
  font-weight:700;font-size:calc(var(--u)*1.5);color:#fff;background:var(--ok)}
.pill.shut{background:var(--off)}

/* ---- stat tiles ---- */
.tiles{display:grid;grid-template-columns:repeat(3,1fr);gap:calc(var(--u)*1.4)}
.tile{background:var(--panel);border:1px solid var(--line);border-radius:calc(var(--u)*1.6);
  padding:calc(var(--u)*1.8) calc(var(--u)*2.2)}
.tile .n{font-size:calc(var(--u)*9);font-weight:700;letter-spacing:-.04em;line-height:1;
  font-variant-numeric:tabular-nums}
.tile .k{font-size:calc(var(--u)*1.6);font-weight:700;letter-spacing:.1em;text-transform:uppercase;
  color:var(--ink3);margin-top:calc(var(--u)*.9)}
.tile .th{font-size:calc(var(--u)*1.7);color:var(--ink2);margin-top:calc(var(--u)*.2)}

/* ---- two panels ---- */
.cols{flex:1;display:grid;grid-template-columns:1.45fr 1fr;gap:calc(var(--u)*1.4);min-height:0}
.panel{background:var(--panel);border:1px solid var(--line);border-radius:calc(var(--u)*1.6);
  padding:calc(var(--u)*1.8) calc(var(--u)*2.2);display:flex;flex-direction:column;min-height:0}
.ptitle{font-size:calc(var(--u)*1.5);font-weight:700;letter-spacing:.12em;text-transform:uppercase;
  color:var(--ink3)}
.ptitle b{display:block;font-size:calc(var(--u)*1.7);font-weight:400;letter-spacing:0;
  text-transform:none;color:var(--ink2);margin-top:calc(var(--u)*.2)}
.body{flex:1;display:flex;flex-direction:column;justify-content:space-evenly;min-height:0;
  margin-top:calc(var(--u)*1.2)}

/* ---- shop rows ---- */
.shop{display:grid;grid-template-columns:1fr calc(var(--u)*11) calc(var(--u)*13);
  gap:calc(var(--u)*1.4);align-items:center}
.shop .nm{font-size:calc(var(--u)*2.1);font-weight:600;overflow:hidden;
  text-overflow:ellipsis;white-space:nowrap}
.track{grid-column:1/-1;background:var(--track);border-radius:calc(var(--u)*.5);
  height:calc(var(--u)*1.5);overflow:hidden;margin-top:calc(var(--u)*.5)}
.fill{background:var(--bar);height:100%;border-radius:calc(var(--u)*.5);
  transition:width .5s ease}
.num{text-align:right;font-variant-numeric:tabular-nums;font-weight:700;
  font-size:calc(var(--u)*2.1)}
.num small{color:var(--ink3);font-weight:400;font-size:calc(var(--u)*1.4)}

/* ---- recent feed ---- */
.row{display:grid;grid-template-columns:1fr auto auto;gap:calc(var(--u)*1.4);
  align-items:baseline;padding:calc(var(--u)*.6) 0}
.row+.row{border-top:1px solid var(--line)}
.row .id{font-size:calc(var(--u)*2.2);font-weight:700;font-variant-numeric:tabular-nums;
  letter-spacing:.02em}
.row .sh{font-size:calc(var(--u)*1.6);color:var(--ink2)}
.row .at{font-size:calc(var(--u)*1.6);color:var(--ink3);font-variant-numeric:tabular-nums}
.empty{color:var(--ink3);font-size:calc(var(--u)*2)}

/* ---- offline banner ---- */
#down{position:fixed;left:0;right:0;bottom:0;background:var(--off);color:#fff;
  text-align:center;padding:calc(var(--u)*.9);font-size:calc(var(--u)*1.7);
  font-weight:700;display:none}
#down.show{display:block}
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <div class="brand">MCU Canteen
      <small>Meal subsidy &middot; &#3626;&#3623;&#3633;&#3626;&#3604;&#3636;&#3585;&#3634;&#3619;&#3629;&#3634;&#3627;&#3634;&#3619;&#3585;&#3621;&#3634;&#3591;&#3623;&#3633;&#3609;</small>
    </div>
    <div class="sp"></div>
    <div class="now">
      <div class="clock" id="clock">--:--:--</div>
      <div class="when"><span id="date">&nbsp;</span>
        <span class="pill" id="state">&nbsp;</span></div>
    </div>
  </div>

  <div class="tiles">
    <div class="tile"><div class="n" id="tServed">0</div>
      <div class="k">Meals served</div>
      <div class="th">&#3592;&#3656;&#3634;&#3618;&#3649;&#3621;&#3657;&#3623;&#3623;&#3633;&#3609;&#3609;&#3637;&#3657;</div></div>
    <div class="tile"><div class="n" id="tAmount">0</div>
      <div class="k">Baht paid</div>
      <div class="th">&#3648;&#3611;&#3655;&#3609;&#3648;&#3591;&#3636;&#3609;</div></div>
    <div class="tile"><div class="n" id="tLeft">0</div>
      <div class="k">Students left</div>
      <div class="th">&#3618;&#3633;&#3591;&#3652;&#3617;&#3656;&#3617;&#3634;&#3619;&#3633;&#3610;</div></div>
  </div>

  <div class="cols">
    <div class="panel">
      <div class="ptitle">Sales by shop
        <b>&#3618;&#3629;&#3604;&#3586;&#3634;&#3618;&#3619;&#3634;&#3618;&#3619;&#3657;&#3634;&#3609;</b></div>
      <div class="body" id="shops"></div>
    </div>
    <div class="panel">
      <div class="ptitle">Recent
        <b>&#3619;&#3634;&#3618;&#3585;&#3634;&#3619;&#3621;&#3656;&#3634;&#3626;&#3640;&#3604;</b></div>
      <div class="body" id="feed"></div>
    </div>
  </div>
</div>
<div id="down">Cannot reach the canteen server &middot;
  &#3605;&#3636;&#3604;&#3605;&#3656;&#3629;&#3648;&#3588;&#3619;&#3639;&#3656;&#3629;&#3591;&#3649;&#3617;&#3656;&#3586;&#3656;&#3634;&#3618;&#3652;&#3617;&#3656;&#3652;&#3604;&#3657;</div>

<script>
var fails = 0;
function el(id){return document.getElementById(id)}
function esc(s){return String(s==null?'':s).replace(/[&<>"']/g,function(c){
  return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]})}

function paint(d){
  document.documentElement.setAttribute('data-theme', d.dark ? 'dark' : 'light');
  el('clock').textContent  = d.clock;
  el('date').textContent   = d.date;
  var st = el('state');
  st.textContent = (d.open ? 'OPEN ' : 'CLOSED ') + d.window;
  st.className = 'pill' + (d.open ? '' : ' shut');
  el('tServed').textContent = d.served.toLocaleString();
  el('tAmount').textContent = d.amount.toLocaleString();
  el('tLeft').textContent   = d.left.toLocaleString();

  var peak = Math.max(1, d.shops.reduce(function(a,s){return Math.max(a,s.meals)},0));
  el('shops').innerHTML = d.shops.map(function(s,i){
    var pct = s.meals ? Math.max(2, Math.round(s.meals*100/peak)) : 0;
    return '<div class="shop"><div class="nm">'+esc(s.name)+'</div>'
      + '<div class="num">'+s.meals+' <small>meals</small></div>'
      + '<div class="num">'+s.amount.toLocaleString()+' <small>THB</small></div>'
      + '<div class="track"><div class="fill" style="width:'+pct+'%"></div></div></div>';
  }).join('');

  el('feed').innerHTML = d.feed.length ? d.feed.map(function(f){
    var shop = (f.shop >= 1 && f.shop <= 4) ? esc(d.shops[f.shop-1].name) : '—';
    return '<div class="row"><div class="id">'+esc(f.id)+'</div>'
      + '<div class="sh">'+shop+'</div>'
      + '<div class="at">'+esc(f.at)+'</div></div>';
  }).join('') : '<div class="empty">No meals served yet &middot; '
      + 'ยังไม่มีรายการ</div>';
}

function tick(){
  fetch('/api/board').then(function(r){return r.json()}).then(function(d){
    fails = 0; el('down').classList.remove('show'); paint(d);
  }).catch(function(){
    // พลาดครั้งเดียวไม่เตือน เพราะ Wi-Fi สะดุดชั่วครู่เป็นเรื่องปกติ
    if (++fails >= 2) el('down').classList.add('show');
  });
}
tick();
setInterval(tick, 2000);
</script>
</body>
</html>
)html";

// --- หน้าเข้าสู่ระบบ ---
String getLoginHTML(bool hasError) {
  String html = R"rawliteral(<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MCU Canteen — Sign in</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{min-height:100vh;display:flex;align-items:center;justify-content:center;
  background:#111110;color:#fff;padding:20px;
  font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
.card{width:100%;max-width:360px;background:#1a1a19;border:1px solid #33332f;
  border-radius:16px;padding:28px}
h1{font-size:1.25rem;letter-spacing:-.01em;margin-bottom:4px}
p.sub{color:#94938a;font-size:.88rem;margin-bottom:22px}
label{display:block;font-size:.78rem;font-weight:600;color:#c3c2b7;margin:14px 0 5px}
input{width:100%;padding:11px 13px;border:1px solid #33332f;border-radius:10px;
  background:#111110;color:#fff;font-family:inherit;font-size:.95rem}
input:focus{outline:2px solid #3987e5;outline-offset:-1px}
button{width:100%;margin-top:22px;padding:12px;border:0;border-radius:10px;
  background:#3987e5;color:#fff;font-family:inherit;font-size:.95rem;
  font-weight:700;cursor:pointer}
.err{margin-top:16px;padding:10px 13px;border-radius:10px;
  border:1px solid #d03b3b;color:#d03b3b;font-size:.86rem}
.foot{margin-top:20px;text-align:center;color:#78766f;font-size:.76rem}
.lang{position:absolute;top:16px;right:16px;background:none;border:1px solid #33332f;
  color:#c3c2b7;border-radius:8px;padding:6px 12px;width:auto;margin:0;
  font-size:.8rem;font-weight:600;cursor:pointer;font-family:inherit}
</style>
</head>
<body>
<button class="lang" id="lang" onclick="flip()" type="button">&#3652;&#3607;&#3618;</button>
<form class="card" method="POST" action="/login">
  <h1>MCU Canteen</h1>
  <p class="sub" data-i="sub">Meal subsidy &middot; staff sign in</p>
  <label for="u" data-i="user">Username</label>
  <input id="u" name="username" autocomplete="username" required autofocus>
  <label for="p" data-i="pass">Password</label>
  <input id="p" name="password" type="password" autocomplete="current-password" required>
  <button type="submit" data-i="in">Sign in</button>
)rawliteral";

  if (hasError) {
    html += R"rawliteral(  <div class="err" data-i="err">Wrong username or password.</div>
)rawliteral";
  }

  // หน้านี้แยกจากแดชบอร์ด จึงมีคำแปลชุดย่อของตัวเอง ห้าคำ ไม่ต้องโหลด /a.js
  html += R"rawliteral(  <div class="foot" data-i="foot">Mahachulalongkornrajavidyalaya University &middot; Phrae Campus</div>
</form>
<script>
var TH={sub:'\u0e2a\u0e27\u0e31\u0e2a\u0e14\u0e34\u0e01\u0e32\u0e23\u0e2d\u0e32\u0e2b\u0e32\u0e23 \u00b7 \u0e2a\u0e33\u0e2b\u0e23\u0e31\u0e1a\u0e40\u0e08\u0e49\u0e32\u0e2b\u0e19\u0e49\u0e32\u0e17\u0e35\u0e48',
  user:'\u0e0a\u0e37\u0e48\u0e2d\u0e1c\u0e39\u0e49\u0e43\u0e0a\u0e49', pass:'\u0e23\u0e2b\u0e31\u0e2a\u0e1c\u0e48\u0e32\u0e19',
  in:'\u0e40\u0e02\u0e49\u0e32\u0e2a\u0e39\u0e48\u0e23\u0e30\u0e1a\u0e1a',
  err:'\u0e0a\u0e37\u0e48\u0e2d\u0e1c\u0e39\u0e49\u0e43\u0e0a\u0e49\u0e2b\u0e23\u0e37\u0e2d\u0e23\u0e2b\u0e31\u0e2a\u0e1c\u0e48\u0e32\u0e19\u0e44\u0e21\u0e48\u0e16\u0e39\u0e01\u0e15\u0e49\u0e2d\u0e07',
  foot:'\u0e21\u0e2b\u0e32\u0e27\u0e34\u0e17\u0e22\u0e32\u0e25\u0e31\u0e22\u0e21\u0e2b\u0e32\u0e08\u0e38\u0e2c\u0e32\u0e25\u0e07\u0e01\u0e23\u0e13\u0e23\u0e32\u0e0a\u0e27\u0e34\u0e17\u0e22\u0e32\u0e25\u0e31\u0e22 \u00b7 \u0e27\u0e34\u0e17\u0e22\u0e32\u0e40\u0e02\u0e15\u0e41\u0e1e\u0e23\u0e48'};
function paint(l){
  document.documentElement.setAttribute('lang',l);
  document.querySelectorAll('[data-i]').forEach(function(e){
    if(e._en===undefined) e._en=e.textContent;
    e.textContent = (l==='th' && TH[e.getAttribute('data-i')]) ? TH[e.getAttribute('data-i')] : e._en;
  });
  document.getElementById('lang').textContent = (l==='th') ? 'EN' : '\u0e44\u0e17\u0e22';
  try{localStorage.setItem('lang',l)}catch(e){}
}
function flip(){
  paint(document.documentElement.getAttribute('lang')==='th' ? 'en' : 'th');
}
try{paint(localStorage.getItem('lang')||'en')}catch(e){paint('en')}
</script>
</body>
</html>)rawliteral";
  return html;
}

// --- แดชบอร์ดเจ้าหน้าที่ ---
// ส่งเฉพาะโครงหน้า ส่วนตัวเลขสด ๆ ให้ /api/dashboard เป็นคนป้อน
String getHTML() {
  // รายการไฟล์ประวัติย้อนหลังที่เก็บไว้ในหน่วยความจำ
  String archives;
  File root = LittleFS.open("/");
  File f = root.openNextFile();
  while (f) {
    String fn = String(f.name());
    if (fn.startsWith("/")) fn = fn.substring(1);
    if (fn.startsWith("arc_")) {
      archives += "<tr id=\"arc-" + fn + "\"><td><b>" + fn + "</b></td>";
      archives += "<td class=\"hide-s\">" + String(f.size() / 1024.0, 1) + " KB</td>";
      archives += "<td style=\"text-align:right;white-space:nowrap\">";
      archives += "<a class=\"btn sm\" href=\"/api/archive/download?file=" + fn + "\" data-i=\"arcDl\">Download</a> ";
      archives += "<button class=\"btn sm danger\" data-i=\"arcDel\" onclick=\"delArchive('" + fn + "')\">Delete</button>";
      archives += "</td></tr>";
    }
    f = root.openNextFile();
  }
  if (archives.length() == 0) {
    archives = "<tr><td colspan=\"3\" style=\"color:var(--ink3)\" data-i=\"arcNone\">No archived days yet</td></tr>";
  }

  // รายชื่อเจ้าหน้าที่ที่เข้าระบบได้ (สูงสุดสามคน)
  String admins;
  for (const auto &u : adminUsers) {
    admins += "<tr><td><b>" + u.username + "</b></td><td>" + u.displayName + "</td>";
    admins += "<td style=\"text-align:right\">";
    if (adminUsers.size() > 1) {
      admins += "<button class=\"btn sm danger\" data-i=\"stfRemove\" onclick=\"delAdmin('" + u.username + "')\">Remove</button>";
    }
    admins += "</td></tr>";
  }

  DateTime nowRTC = rtc.now();
  char dateBuf[12], timeBuf[10], startBuf[6], endBuf[6];
  snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", nowRTC.year(), nowRTC.month(), nowRTC.day());
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", nowRTC.hour(), nowRTC.minute(), nowRTC.second());
  snprintf(startBuf, sizeof(startBuf), "%02d:%02d", serviceStartHour, serviceStartMin);
  snprintf(endBuf, sizeof(endBuf), "%02d:%02d", serviceEndHour, serviceEndMin);

  String html = R"rawliteral(<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MCU Canteen</title>
<link rel="stylesheet" href="/s.css">
</head>
<body>
<div class="wrap">
  <div class="top">
    <div class="brand">MCU Canteen <span>&middot; <span data-i="sub">Meal Subsidy</span></span></div>
    <div class="sp"></div>
    <button class="btn" id="langBtn" onclick="toggleLang()">&#3652;&#3607;&#3618;</button>
    <button class="btn" id="themeBtn" onclick="toggleTheme()">Light mode</button>
    <a class="btn" href="/logout" data-i="signout">Sign out</a>
  </div>

  <div class="tabs" role="tablist">
    <button class="tab" role="tab" aria-selected="true"  onclick="show('today',this)" data-i="today">Today</button>
    <button class="tab" role="tab" aria-selected="false" onclick="show('students',this)" data-i="students">Students</button>
    <button class="tab" role="tab" aria-selected="false" onclick="show('shops',this)" data-i="shops">Shops</button>
    <button class="tab" role="tab" aria-selected="false" onclick="show('settings',this)" data-i="settings">Settings</button>
  </div>

  <!-- ===================== TODAY ===================== -->
  <section id="v-today" class="view show">
    <div class="tiles">
      <div class="tile"><div class="n" id="tServed">0</div><div class="k" data-i="kServed">Meals served</div></div>
      <div class="tile"><div class="n" id="tAmount">0</div><div class="k" data-i="kBaht">Baht paid</div></div>
      <div class="tile"><div class="n" id="tLeft">0</div><div class="k" data-i="kLeft">Students left</div></div>
      <div class="tile"><div class="n" id="tClock">--:--</div><div class="k" id="tState">Closed</div></div>
    </div>

    <div class="panel" style="margin-top:16px">
      <div class="ptitle"><span data-i="salesby">Sales by shop</span> &middot; <span id="tDate">today</span></div>
      <div id="shopList"></div>
    </div>

    <div class="panel">
      <div class="ptitle" data-i="points">Stations</div>
      <div class="chips" id="stationList"></div>
    </div>
  </section>

  <!-- ===================== STUDENTS ===================== -->
  <section id="v-students" class="view">
    <div class="panel">
      <div class="row" style="margin-bottom:12px">
        <input id="q" data-ip="sSearch" placeholder="Search name or student ID" onkeyup="findStudents(this.value)">
        <button class="btn fit" onclick="openAdd()" data-i="sAdd">Add student</button>
        <a class="btn fit" href="/export.csv" data-i="sExport">Export CSV</a>
      </div>
      <table>
        <thead><tr><th data-i="hId">Student ID</th><th data-i="hName">Name</th>
          <th class="hide-s" data-i="hCard">Card</th><th data-i="hStatus">Status</th>
          <th class="hide-s" data-i="hShop">Shop</th><th></th></tr></thead>
        <tbody id="stuBody"><tr><td colspan="6" style="color:var(--ink3)" data-i="sLoad">Loading&hellip;</td></tr></tbody>
      </table>
      <div class="row" style="margin-top:12px">
        <div class="fit" style="color:var(--ink3);font-size:.85rem" id="pageInfo"></div>
        <div class="gap"></div>
        <button class="btn sm fit" onclick="turnPage(-1)" data-i="prev">Previous</button>
        <button class="btn sm fit" onclick="turnPage(1)" data-i="next">Next</button>
      </div>
    </div>
  </section>

  <!-- ===================== SHOPS ===================== -->
  <section id="v-shops" class="view">
    <form class="panel" method="POST" action="/save-shops" data-ajax="1">
      <div class="ptitle" data-i="shTitle">Shop names and owners</div>
      <div id="shopForm"></div>
      <p class="hint" data-i="shHint">The last box is the name shown on the host screen. Use English
        letters and numbers only &mdash; the screen font has no Thai characters. Leave it empty and
        the screen falls back to Shop 1 to Shop 4.</p>
      <button class="btn main" type="submit" style="margin-top:14px" data-i="shSave">Save shops</button>
    </form>
  </section>

  <!-- ===================== SETTINGS ===================== -->
  <section id="v-settings" class="view">
    <div class="panel">
      <div class="ptitle" data-i="dTitle">Display on all stations</div>
      <p class="note" data-i="dNote">The host decides the theme and the sleep mode for every station.</p>
      <div class="row">
        <div><label for="dispDark" data-i="dTheme">Theme</label>
          <select id="dispDark" onchange="setDisplay()">
            <option value="1" data-i="dDark">Dark</option>
            <option value="0" data-i="dLight">Light</option>
          </select></div>
        <div><label for="dispSaver" data-i="dScreens">Screens</label>
          <select id="dispSaver" onchange="setDisplay()">
            <option value="0" data-i="dAwake">Awake</option>
            <option value="1" data-i="dSleep">Sleeping</option>
          </select></div>
      </div>
    </div>

    <form class="panel" method="POST" action="/api/settings/time" data-ajax="1">
      <div class="ptitle" data-i="hrsTitle">Service hours</div>
      <div class="row">
        <div><label for="sWin" data-i="hrsLimit">Limit</label>
          <select id="sWin" name="enabled">
)rawliteral";

  html += String("            <option value=\"1\" data-i=\"hrsOn\"") + (timeWindowEnabled ? " selected" : "") + ">Serve only in these hours</option>\n";
  html += String("            <option value=\"0\" data-i=\"hrsOff\"") + (!timeWindowEnabled ? " selected" : "") + ">Serve all day</option>\n";

  html += R"rawliteral(          </select></div>
        <div><label for="sStart" data-i="hrsOpen">Opens</label>
)rawliteral";
  html += String("          <input id=\"sStart\" name=\"start\" value=\"") + startBuf + "\"></div>\n";
  html += R"rawliteral(        <div><label for="sEnd" data-i="hrsClose">Closes</label>
)rawliteral";
  html += String("          <input id=\"sEnd\" name=\"end\" value=\"") + endBuf + "\"></div>\n";
  html += R"rawliteral(        <button class="btn main fit" type="submit" data-i="hrsSave">Save hours</button>
      </div>
    </form>

    <form class="panel" method="POST" action="/api/rtc/set" data-ajax="1">
      <div class="ptitle" data-i="clkTitle">Host clock</div>
      <div class="row">
)rawliteral";
  html += String("        <div><label for=\"cDate\" data-i=\"clkDate\">Date</label><input id=\"cDate\" type=\"date\" name=\"date\" value=\"") + dateBuf + "\" required></div>\n";
  html += String("        <div><label for=\"cTime\" data-i=\"clkTime\">Time</label><input id=\"cTime\" name=\"time\" value=\"") + timeBuf + "\" placeholder=\"HH:MM:SS\" required></div>\n";
  html += R"rawliteral(        <button class="btn main fit" type="submit" data-i="clkSet">Set clock</button>
      </div>
      <p class="hint" data-i="clkHint">The clock decides when service opens and which day the records belong to.</p>
    </form>

    <form class="panel" method="POST" action="/upload" enctype="multipart/form-data" data-ajax="1">
      <div class="ptitle" data-i="impTitle">Import student list</div>
      <div class="row">
        <div><input type="file" name="csv" accept=".csv" required></div>
        <button class="btn fit" type="submit" data-i="impBtn">Upload CSV</button>
      </div>
      <p class="hint" data-i="impHint">Columns: student ID, name, card number. Existing students are updated, new ones added.</p>
    </form>

    <div class="panel">
      <div class="ptitle" data-i="stfTitle">Staff who can sign in</div>
      <table>
        <thead><tr><th data-i="stfUser">Username</th><th data-i="stfName">Name</th><th></th></tr></thead>
        <tbody>
)rawliteral";
  html += admins;
  html += R"rawliteral(        </tbody>
      </table>
      <form method="POST" action="/api/admin/save" data-ajax="1" data-reload="1" style="margin-top:8px">
        <input type="hidden" name="oldUsername" value="">
        <div class="row">
          <div><label for="aU" data-i="stfUser">Username</label><input id="aU" name="username" required></div>
          <div><label for="aP" data-i="stfPass">Password</label><input id="aP" name="password" type="password" required></div>
          <div><label for="aN" data-i="stfName">Name</label><input id="aN" name="displayName"></div>
          <button class="btn fit" type="submit" data-i="stfAdd">Add staff</button>
        </div>
      </form>
      <p class="hint" data-i="stfHint">Up to three accounts. Change the factory password before the canteen opens.</p>
    </div>

    <div class="panel">
      <div class="ptitle" data-i="arcTitle">Archived days</div>
      <table>
        <thead><tr><th data-i="arcFile">File</th><th class="hide-s" data-i="arcSize">Size</th><th></th></tr></thead>
        <tbody>
)rawliteral";
  html += archives;
  html += R"rawliteral(        </tbody>
      </table>
    </div>

    <div class="panel">
      <div class="ptitle" data-i="eodTitle">End of day</div>
      <p class="note" data-i="eodNote">Files today&rsquo;s records away and gives every student their allowance back.</p>
      <button class="btn danger" onclick="closeDay()" data-i="eodBtn">Close today</button>
    </div>
  </section>
</div>
<div id="toast"></div>
<script src="/a.js"></script>
</body>
</html>)rawliteral";

  return html;
}
