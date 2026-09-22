/**
 * ============================================================================
 * WebDashboard.h — หน้าเว็บทั้งหมด: หน้าเข้าสู่ระบบ และแดชบอร์ดเจ้าหน้าที่
 *
 * แยกออกมาจาก Canteen_Host_Server.ino เพื่อให้ไฟล์หลักสั้นและอ่านง่าย
 * นิสิตที่มารับช่วงดูแลต่อจะได้หาของเจอเร็วขึ้น ไม่ต้องไล่อ่านไฟล์เดียวสามพันบรรทัด
 *
 * ไฟล์นี้ถูก #include ไว้ท้ายไฟล์หลักก่อน setup() เพราะโค้ดข้างในอ้างถึง
 * ตัวแปรส่วนกลางและฟังก์ชันช่วยเหลือที่ประกาศไว้ข้างบน
 * **อย่าย้าย #include ขึ้นไปไว้บนสุด**
 *
 * ข้อความบนหน้าเว็บเป็นภาษาอังกฤษทั้งหมด เพราะผู้ใช้งานจริงคือเจ้าของร้านค้า
 * และนิสิตต่างชาติ ส่วนคำอธิบายโค้ดยังเป็นภาษาไทยไว้ให้คนดูแลระบบอ่าน
 *
 * ประสิทธิภาพ: CSS และ JavaScript เก็บไว้ใน PROGMEM (แฟลช ไม่กินแรม)
 * แล้วเสิร์ฟแยกที่ /s.css และ /a.js เบราว์เซอร์จึงแคชไว้ได้
 * หน้า / จึงเหลือแค่โครงเล็ก ๆ ไม่ต้องต่อสตริงยาวสองหมื่นตัวอักษรในแรมทุกครั้ง
 * ============================================================================
 */

#pragma once

// ============================================================================
// สไตล์ชีต — สองโหมด สว่าง/มืด สลับด้วย data-theme บน <html>
// สีทั้งหมดเป็นตัวแปร CSS ชุดเดียว แก้ที่เดียวเปลี่ยนทั้งหน้า
// ============================================================================
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
static const char DASH_JS[] PROGMEM = R"js(function lsGet(k){try{return localStorage.getItem(k)}catch(e){return null}}
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

/* ---- theme: remembered in this browser, and pushed to every service point ---- */
function paintTheme(t){
  document.documentElement.setAttribute('data-theme',t);
  var b=el('themeBtn'); if(b) b.textContent=(t==='dark')?'Light mode':'Dark mode';
}
function toggleTheme(){
  var t=document.documentElement.getAttribute('data-theme')==='dark'?'light':'dark';
  paintTheme(t); lsSet('theme',t);
  var sel=el('dispDark'); if(sel) sel.value=(t==='dark')?'1':'0';
  post('/api/display',{dark:(t==='dark')?'1':'0'});
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
    txt('tState',  (d.open?'Open':'Closed')+' · '+d.window);
    txt('tDate',   d.date);

    peak=Math.max(1, d.shops.reduce(function(a,s){return Math.max(a,s.meals)},0));
    var h='';
    d.shops.forEach(function(s){
      var pct=Math.round(s.meals*100/peak);
      h+='<div class="shop">'
       + '<div class="nm">'+esc(s.name)+'<small>'+esc(s.owner)+'</small></div>'
       + '<div class="track"><div class="fill" style="width:'+pct+'%"></div></div>'
       + '<div class="num">'+s.meals+' <small>meals</small></div>'
       + '<div class="num">'+s.amount.toLocaleString()+' <small>THB</small></div>'
       + '</div>';
    });
    el('shopList').innerHTML=h;

    el('stationList').innerHTML=d.stations.map(function(s){
      return '<span class="chip '+(s.online?'on':'off')+'"><i class="dot"></i>Point '
           + s.id+' · '+(s.online?'Online':'Offline')+'</span>';
    }).join('');

    var sd=el('dispDark'), ss=el('dispSaver');
    if(sd && document.activeElement!==sd) sd.value=d.dark?'1':'0';
    if(ss && document.activeElement!==ss) ss.value=d.saver?'1':'0';

    var sf=el('shopForm');
    if(sf && !sf.dataset.filled){
      sf.dataset.filled='1';
      sf.innerHTML=d.shops.map(function(s,i){
        return '<div class="row" style="margin-bottom:10px">'
         + '<div><label>Shop '+(i+1)+' name</label><input name="sname'+i+'" value="'+esc(s.name)+'" required></div>'
         + '<div><label>Owner</label><input name="vname'+i+'" value="'+esc(s.owner)+'"></div></div>';
      }).join('');
    }
  }).catch(function(){});
}

/* ---- display mode for every service point (host decides, stations obey) ---- */
function setDisplay(){
  post('/api/display',{dark:el('dispDark').value, saver:el('dispSaver').value})
    .then(function(){
      paintTheme(el('dispDark').value==='1'?'dark':'light');
      lsSet('theme',el('dispDark').value==='1'?'dark':'light');
      toast('Service points updated'); refresh();
    });
}

/* ---- students ---- */
var page=1, pages=1, term='';
function loadStudents(){
  fetch('/api/students?page='+page+'&limit=20&search='+encodeURIComponent(term))
    .then(function(r){if(r.status===401){location.href='/login';throw 0}return r.json()})
    .then(function(d){
      pages=d.totalPages||1; page=d.currentPage||1;
      txt('pageInfo','Page '+page+' of '+pages+' · '+(d.totalItems||0)+' students');
      var b=el('stuBody');
      if(!d.students || !d.students.length){
        b.innerHTML='<tr><td colspan="6" style="color:var(--ink3)">No students found</td></tr>'; return;
      }
      b.innerHTML=d.students.map(function(s){
        var id=esc(s.id);
        var status=s.claimed?'<span class="tag y">Served</span>'
                            :(s.isTemp?'<span class="tag t">Temp card</span>'
                                      :'<span class="tag n">Waiting</span>');
        var act='';
        if(!s.claimed) act+='<button class="btn sm" onclick="serve(\''+id+'\')">Serve</button> ';
        act+= s.isTemp ? '<button class="btn sm" onclick="dropCard(\''+id+'\')">Revoke card</button> '
                       : '<button class="btn sm" onclick="tempCard(\''+id+'\')">Temp card</button> ';
        act+='<button class="btn sm danger" onclick="delStudent(\''+id+'\')">Remove</button>';
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
  var shop=prompt('Which shop served this student? (1-4)','1'); if(!shop)return;
  post('/manual-claim',{id:id,station:shop}).then(function(){toast('Recorded');loadStudents();refresh()});
}
function delStudent(id){
  if(!confirm('Remove student '+id+'?'))return;
  post('/api/student/delete',{id:id}).then(function(){toast('Removed');loadStudents()});
}
function tempCard(id){
  var uid=prompt('Tap the temporary card, then type its number'); if(!uid)return;
  post('/bind-temp',{id:id,uid:uid}).then(function(){toast('Temporary card assigned');loadStudents()});
}
function dropCard(id){
  post('/api/tempcard/remove',{id:id}).then(function(){toast('Card revoked');loadStudents()});
}
function openAdd(){
  var id=prompt('Student ID'); if(!id)return;
  var nm=prompt('Full name'); if(!nm)return;
  var uid=prompt('Card number (leave empty if none)')||'';
  post('/api/student/save',{oldStudentId:'',studentId:id,fullName:nm,uid:uid})
    .then(function(){toast('Student added');loadStudents();refresh()});
}
function closeDay(){
  if(!confirm('Close today and give every student their allowance back?'))return;
  post('/reset',{}).then(function(){toast('Day closed');refresh();loadStudents()});
}
function delArchive(f){
  if(!confirm('Delete '+f+'?'))return;
  post('/api/archive/delete',{file:f}).then(function(){
    var r=el('arc-'+f); if(r) r.remove(); toast('Archive deleted')});
}
function delAdmin(u){
  if(!confirm('Remove officer '+u+'?'))return;
  post('/api/admin/delete',{username:u}).then(function(){location.reload()});
}

/* ---- forms submit without leaving the page ---- */
function bindForms(){
  document.querySelectorAll('form[data-ajax]').forEach(function(f){
    f.addEventListener('submit',function(ev){
      ev.preventDefault();
      fetch(f.getAttribute('action'),{method:'POST',body:new FormData(f)})
        .then(function(r){if(r.status===401){location.href='/login';throw 0}return r.text()})
        .then(function(){toast('Saved'); refresh(); if(f.dataset.reload) location.reload()})
        .catch(function(){toast('Could not save')});
    });
  });
}

/* ---- start ---- */
paintTheme(lsGet('theme')||'dark');
try{bindForms()}catch(e){}
refresh();
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
</style>
</head>
<body>
<form class="card" method="POST" action="/login">
  <h1>MCU Canteen</h1>
  <p class="sub">Meal subsidy &middot; staff sign in</p>
  <label for="u">Username</label>
  <input id="u" name="username" autocomplete="username" required autofocus>
  <label for="p">Password</label>
  <input id="p" name="password" type="password" autocomplete="current-password" required>
  <button type="submit">Sign in</button>
)rawliteral";

  if (hasError) {
    html += R"rawliteral(  <div class="err">Wrong username or password.</div>
)rawliteral";
  }

  html += R"rawliteral(  <div class="foot">Mahachulalongkornrajavidyalaya University &middot; Phrae Campus</div>
</form>
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
      archives += "<a class=\"btn sm\" href=\"/api/archive/download?file=" + fn + "\">Download</a> ";
      archives += "<button class=\"btn sm danger\" onclick=\"delArchive('" + fn + "')\">Delete</button>";
      archives += "</td></tr>";
    }
    f = root.openNextFile();
  }
  if (archives.length() == 0) {
    archives = "<tr><td colspan=\"3\" style=\"color:var(--ink3)\">No archived days yet</td></tr>";
  }

  // รายชื่อเจ้าหน้าที่ที่เข้าระบบได้ (สูงสุดสามคน)
  String admins;
  for (const auto &u : adminUsers) {
    admins += "<tr><td><b>" + u.username + "</b></td><td>" + u.displayName + "</td>";
    admins += "<td style=\"text-align:right\">";
    if (adminUsers.size() > 1) {
      admins += "<button class=\"btn sm danger\" onclick=\"delAdmin('" + u.username + "')\">Remove</button>";
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
    <div class="brand">MCU Canteen <span>&middot; Meal Subsidy</span></div>
    <div class="sp"></div>
    <button class="btn" id="themeBtn" onclick="toggleTheme()">Light mode</button>
    <a class="btn" href="/logout">Sign out</a>
  </div>

  <div class="tabs" role="tablist">
    <button class="tab" role="tab" aria-selected="true"  onclick="show('today',this)">Today</button>
    <button class="tab" role="tab" aria-selected="false" onclick="show('students',this)">Students</button>
    <button class="tab" role="tab" aria-selected="false" onclick="show('shops',this)">Shops</button>
    <button class="tab" role="tab" aria-selected="false" onclick="show('settings',this)">Settings</button>
  </div>

  <!-- ===================== TODAY ===================== -->
  <section id="v-today" class="view show">
    <div class="tiles">
      <div class="tile"><div class="n" id="tServed">0</div><div class="k">Meals served</div></div>
      <div class="tile"><div class="n" id="tAmount">0</div><div class="k">Baht paid</div></div>
      <div class="tile"><div class="n" id="tLeft">0</div><div class="k">Students left</div></div>
      <div class="tile"><div class="n" id="tClock">--:--</div><div class="k" id="tState">Closed</div></div>
    </div>

    <div class="panel" style="margin-top:16px">
      <div class="ptitle">Sales by shop &middot; <span id="tDate">today</span></div>
      <div id="shopList"></div>
    </div>

    <div class="panel">
      <div class="ptitle">Service points</div>
      <div class="chips" id="stationList"></div>
    </div>
  </section>

  <!-- ===================== STUDENTS ===================== -->
  <section id="v-students" class="view">
    <div class="panel">
      <div class="row" style="margin-bottom:12px">
        <input id="q" placeholder="Search name or student ID" onkeyup="findStudents(this.value)">
        <button class="btn fit" onclick="openAdd()">Add student</button>
        <a class="btn fit" href="/export.csv">Export CSV</a>
      </div>
      <table>
        <thead><tr><th>Student ID</th><th>Name</th><th class="hide-s">Card</th>
          <th>Status</th><th class="hide-s">Shop</th><th></th></tr></thead>
        <tbody id="stuBody"><tr><td colspan="6" style="color:var(--ink3)">Loading&hellip;</td></tr></tbody>
      </table>
      <div class="row" style="margin-top:12px">
        <div class="fit" style="color:var(--ink3);font-size:.85rem" id="pageInfo"></div>
        <div class="gap"></div>
        <button class="btn sm fit" onclick="turnPage(-1)">Previous</button>
        <button class="btn sm fit" onclick="turnPage(1)">Next</button>
      </div>
    </div>
  </section>

  <!-- ===================== SHOPS ===================== -->
  <section id="v-shops" class="view">
    <form class="panel" method="POST" action="/save-shops" data-ajax="1">
      <div class="ptitle">Shop names and owners</div>
      <div id="shopForm"></div>
      <button class="btn main" type="submit" style="margin-top:14px">Save shops</button>
    </form>
  </section>

  <!-- ===================== SETTINGS ===================== -->
  <section id="v-settings" class="view">
    <div class="panel">
      <div class="ptitle">Display on all service points</div>
      <p class="note">The host decides the theme and the sleep mode for every service point.</p>
      <div class="row">
        <div><label for="dispDark">Theme</label>
          <select id="dispDark" onchange="setDisplay()">
            <option value="1">Dark</option><option value="0">Light</option>
          </select></div>
        <div><label for="dispSaver">Screens</label>
          <select id="dispSaver" onchange="setDisplay()">
            <option value="0">Awake</option><option value="1">Sleeping</option>
          </select></div>
      </div>
    </div>

    <form class="panel" method="POST" action="/api/settings/time" data-ajax="1">
      <div class="ptitle">Service hours</div>
      <div class="row">
        <div><label for="sWin">Limit</label>
          <select id="sWin" name="enabled">
)rawliteral";

  html += String("            <option value=\"1\"") + (timeWindowEnabled ? " selected" : "") + ">Serve only in these hours</option>\n";
  html += String("            <option value=\"0\"") + (!timeWindowEnabled ? " selected" : "") + ">Serve all day</option>\n";

  html += R"rawliteral(          </select></div>
        <div><label for="sStart">Opens</label>
)rawliteral";
  html += String("          <input id=\"sStart\" name=\"start\" value=\"") + startBuf + "\"></div>\n";
  html += R"rawliteral(        <div><label for="sEnd">Closes</label>
)rawliteral";
  html += String("          <input id=\"sEnd\" name=\"end\" value=\"") + endBuf + "\"></div>\n";
  html += R"rawliteral(        <button class="btn main fit" type="submit">Save hours</button>
      </div>
    </form>

    <form class="panel" method="POST" action="/api/rtc/set" data-ajax="1">
      <div class="ptitle">Host clock</div>
      <div class="row">
)rawliteral";
  html += String("        <div><label for=\"cDate\">Date</label><input id=\"cDate\" type=\"date\" name=\"date\" value=\"") + dateBuf + "\" required></div>\n";
  html += String("        <div><label for=\"cTime\">Time</label><input id=\"cTime\" name=\"time\" value=\"") + timeBuf + "\" placeholder=\"HH:MM:SS\" required></div>\n";
  html += R"rawliteral(        <button class="btn main fit" type="submit">Set clock</button>
      </div>
      <p class="hint">The clock decides when service opens and which day the records belong to.</p>
    </form>

    <form class="panel" method="POST" action="/upload" enctype="multipart/form-data" data-ajax="1">
      <div class="ptitle">Import student list</div>
      <div class="row">
        <div><input type="file" name="csv" accept=".csv" required></div>
        <button class="btn fit" type="submit">Upload CSV</button>
      </div>
      <p class="hint">Columns: student ID, name, card number. Existing students are updated, new ones added.</p>
    </form>

    <div class="panel">
      <div class="ptitle">Staff who can sign in</div>
      <table>
        <thead><tr><th>Username</th><th>Name</th><th></th></tr></thead>
        <tbody>
)rawliteral";
  html += admins;
  html += R"rawliteral(        </tbody>
      </table>
      <form method="POST" action="/api/admin/save" data-ajax="1" data-reload="1" style="margin-top:8px">
        <input type="hidden" name="oldUsername" value="">
        <div class="row">
          <div><label for="aU">Username</label><input id="aU" name="username" required></div>
          <div><label for="aP">Password</label><input id="aP" name="password" type="password" required></div>
          <div><label for="aN">Name</label><input id="aN" name="displayName"></div>
          <button class="btn fit" type="submit">Add staff</button>
        </div>
      </form>
      <p class="hint">Up to three accounts. Change the factory password before the canteen opens.</p>
    </div>

    <div class="panel">
      <div class="ptitle">Archived days</div>
      <table>
        <thead><tr><th>File</th><th class="hide-s">Size</th><th></th></tr></thead>
        <tbody>
)rawliteral";
  html += archives;
  html += R"rawliteral(        </tbody>
      </table>
    </div>

    <div class="panel">
      <div class="ptitle">End of day</div>
      <p class="note">Files today&rsquo;s records away and gives every student their allowance back.</p>
      <button class="btn danger" onclick="closeDay()">Close today</button>
    </div>
  </section>
</div>
<div id="toast"></div>
<script src="/a.js"></script>
</body>
</html>)rawliteral";

  return html;
}
