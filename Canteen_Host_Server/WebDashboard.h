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
 * ============================================================================
 */

#pragma once

// --- หน้าเข้าสู่ระบบ ---
String getLoginHTML(bool hasError) {
  String html = R"rawliteral(<!DOCTYPE html>
<html lang="th">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>MCU Phrae - Staff Authentication</title>
  <link href="https://fonts.googleapis.com/css2?family=Plus+Jakarta+Sans:wght@400;600;700;800&family=Sarabun:wght@300;400;500;600;700&display=swap" rel="stylesheet">
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Plus Jakarta Sans', 'Sarabun', sans-serif; }
    body { background: radial-gradient(circle at top, #1e293b, #090d16); color: #f8fafc; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 1.5rem; }
    .login-card { background: rgba(30, 41, 59, 0.7); border: 1px solid rgba(255,255,255,0.08); backdrop-filter: blur(16px); border-radius: 2rem; max-width: 26rem; width: 100%; padding: 2.5rem; box-shadow: 0 25px 50px -12px rgba(0,0,0,0.5); }
    .brand-badge { background: #10b981; color: #064e3b; font-size: 0.72rem; font-weight: 800; padding: 0.3rem 0.75rem; border-radius: 9999px; display: inline-block; margin-bottom: 0.9rem; letter-spacing: 0.05em; }
    .title { font-size: 1.45rem; font-weight: 800; color: white; letter-spacing: -0.02em; margin-bottom: 0.3rem; }
    .subtitle { font-size: 0.88rem; color: #94a3b8; margin-bottom: 2rem; }
    .form-group { margin-bottom: 1.25rem; text-align: left; }
    .form-label { display: block; font-size: 0.85rem; font-weight: 600; margin-bottom: 0.45rem; color: #cbd5e1; }
    .form-input { width: 100%; padding: 0.85rem 1.1rem; border-radius: 1rem; border: 1px solid rgba(255,255,255,0.1); background: rgba(15, 23, 42, 0.6); color: white; font-size: 0.95rem; outline: none; transition: all 0.2s; }
    .form-input:focus { border-color: #10b981; box-shadow: 0 0 0 3px rgba(16,185,129,0.2); }
    .btn-login { width: 100%; padding: 0.9rem; border-radius: 1rem; font-size: 0.98rem; font-weight: 700; background: #10b981; color: #064e3b; border: none; cursor: pointer; transition: all 0.2s; margin-top: 0.6rem; }
    .btn-login:hover { background: #059669; color: white; transform: translateY(-1px); }
    .error-box { background: rgba(239, 68, 68, 0.15); border: 1px solid #ef4444; color: #fca5a5; border-radius: 1rem; padding: 0.8rem 1rem; font-size: 0.85rem; font-weight: 600; margin-bottom: 1.25rem; }
    .footer { text-align: center; margin-top: 2rem; font-size: 0.8rem; color: #64748b; line-height: 1.5; }
  </style>
</head>
<body>
  <div class="login-card">
    <div style="text-align: center;">
      <span class="brand-badge">EXECUTIVE BENTO PORTAL</span>
      <h1 class="title">Smart Canteen System</h1>
      <p class="subtitle">ระบบบริหารจัดการอาหารกลางวันนิสิต มจร. แพร่</p>
    </div>
)rawliteral";

  if (hasError) {
    html += "<div class='error-box'>⚠️ ชื่อผู้ใช้หรือรหัสผ่านไม่ถูกต้อง กรุณาลองใหม่อีกครั้ง</div>";
  }

  html += R"rawliteral(
    <form method="POST" action="/login">
      <div class="form-group">
        <label class="form-label">ชื่อผู้ใช้ (Username):</label>
        <input type="text" name="username" required class="form-input" placeholder="e.g. admin" autofocus>
      </div>
      <div class="form-group">
        <label class="form-label">รหัสผ่าน (Password):</label>
        <input type="password" name="password" required class="form-input" placeholder="••••••••">
      </div>
      <button type="submit" class="btn-login">เข้าสู่ระบบ (Sign In)</button>
    </form>
    <div class="footer">
      มจร. วิทยาเขตแพร่ (MCU Phrae Campus)<br>
      Authorized Officers Only
    </div>
  </div>
</body>
</html>)rawliteral";
  return html;
}

// --- แดชบอร์ดเจ้าหน้าที่ ---
String getHTML() {
  int usedCount = 0;
  int shopCounts[4] = {0, 0, 0, 0};
  int activeTempWaitingCount = 0;
  for (const auto& s : db) {
    if (s.claimed) {
      usedCount++;
      if (s.station >= 1 && s.station <= 4) shopCounts[s.station - 1]++;
    }
    if (s.isTempCard && !s.claimed) activeTempWaitingCount++;
  }

  float hostBattVolt = readHostBatteryVoltage();
  int hostBattPct = getHostBatteryPercentage(hostBattVolt);
  float initialTemp = getChipTemperature();
  float initialCpu = calculateCpuLoad();

  String archivesHtml = "";
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String fn = String(file.name());
    if (fn.startsWith("arc_") || fn.startsWith("/arc_")) {
      String cleanName = fn.startsWith("/") ? fn.substring(1) : fn;
      archivesHtml += "<tr><td><b>" + cleanName + "</b></td><td>" + String(file.size() / 1024.0, 1) + " KB</td>";
      archivesHtml += "<td style='text-align:right;'>";
      archivesHtml += "<a href='/api/archive/download?file=" + cleanName + "' class='btn btn-emerald' style='padding:0.35rem 0.75rem; font-size:0.8rem;'>📥 Download</a> ";
      archivesHtml += "<a href='/api/archive/delete?file=" + cleanName + "' onclick=\"return confirm('Delete archive " + cleanName + "?');\" class='btn btn-rose' style='padding:0.35rem 0.75rem; font-size:0.8rem;'>🗑️ Delete</a>";
      archivesHtml += "</td></tr>";
    }
    file = root.openNextFile();
  }
  if (archivesHtml.length() == 0) {
    archivesHtml = "<tr><td colspan='3' style='text-align:center; color:var(--text-muted); padding:1.5rem;' data-th='ไม่มีไฟล์ประวัติย้อนหลังในระบบ' data-en='No archives found in storage'>ไม่มีไฟล์ประวัติย้อนหลังในระบบ</td></tr>";
  }

  DateTime nowRTC = rtc.now();
  char dateInputBuf[12], timeInputBuf[10];
  snprintf(dateInputBuf, sizeof(dateInputBuf), "%04d-%02d-%02d", nowRTC.year(), nowRTC.month(), nowRTC.day());
  snprintf(timeInputBuf, sizeof(timeInputBuf), "%02d:%02d:%02d", nowRTC.hour(), nowRTC.minute(), nowRTC.second());

  int quotaPercent = (db.size() > 0) ? (usedCount * 100) / db.size() : 0;
  if (quotaPercent > 100) quotaPercent = 100;

  String html = R"rawliteral(<!DOCTYPE html>
<html lang="th" data-theme="dark">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>MCU Phrae - Smart Canteen Bento Portal</title>
  <link href="https://fonts.googleapis.com/css2?family=Plus+Jakarta+Sans:wght@400;500;600;700;800&family=Sarabun:wght@300;400;500;600;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg-base: #090d16;
      --bg-surface: rgba(26, 35, 50, 0.75);
      --bg-surface-elevated: #1e293b;
      --border-card: rgba(255, 255, 255, 0.08);
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --nav-bg: rgba(15, 23, 42, 0.85);
      --shadow-card: 0 16px 32px rgba(0, 0, 0, 0.35);
      
      --accent-green: #10b981;
      --accent-green-glow: rgba(16, 185, 129, 0.25);
      --accent-cyan: #06b6d4;
      --accent-yellow: #f59e0b;
      --accent-rose: #f43f5e;
      --accent-indigo: #6366f1;
    }

    [data-theme="light"] {
      --bg-base: #f1f5f9;
      --bg-surface: #ffffff;
      --bg-surface-elevated: #f8fafc;
      --border-card: rgba(0, 0, 0, 0.06);
      --text-main: #0f172a;
      --text-muted: #64748b;
      --nav-bg: rgba(255, 255, 255, 0.88);
      --shadow-card: 0 10px 25px rgba(0, 0, 0, 0.04);
      
      --accent-green: #059669;
      --accent-green-glow: rgba(5, 150, 105, 0.15);
      --accent-cyan: #0284c7;
      --accent-yellow: #d97706;
      --accent-rose: #e11d48;
      --accent-indigo: #4f46e5;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Plus Jakarta Sans', 'Sarabun', sans-serif; transition: background-color 0.25s, border-color 0.25s, color 0.25s; }
    body { background-color: var(--bg-base); color: var(--text-main); padding-top: 4.75rem; min-height: 100vh; display: flex; flex-direction: column; justify-content: space-between; }
    
    .navbar { position: fixed; top: 0; left: 0; right: 0; height: 4.25rem; background: var(--nav-bg); border-bottom: 1px solid var(--border-card); backdrop-filter: blur(16px); display: flex; align-items: center; justify-content: space-between; padding: 0 1.5rem; z-index: 100; }
    .nav-brand { display: flex; align-items: center; gap: 0.75rem; color: var(--text-main); text-decoration: none; }
    .brand-badge { background: var(--accent-green); color: #064e3b; font-size: 0.72rem; font-weight: 800; padding: 0.25rem 0.65rem; border-radius: 9999px; letter-spacing: 0.04em; }
    .brand-title { font-size: 1.15rem; font-weight: 800; white-space: nowrap; letter-spacing: -0.02em; }
    .nav-menu { display: flex; align-items: center; gap: 0.4rem; }
    .nav-item { padding: 0.55rem 0.95rem; border-radius: 0.85rem; font-weight: 600; font-size: 0.875rem; cursor: pointer; border: 1px solid transparent; background: transparent; color: var(--text-muted); text-decoration: none; display: inline-flex; align-items: center; gap: 0.4rem; }
    .nav-item:hover { background: var(--bg-surface-elevated); color: var(--text-main); }
    .nav-item.active { background: var(--accent-indigo); color: white; }

    .dropdown { position: relative; }
    .dropdown-btn { display: inline-flex; align-items: center; gap: 0.35rem; cursor: pointer; }
    .dropdown-arrow { font-size: 0.65rem; transition: transform 0.2s; }
    .dropdown:hover .dropdown-arrow { transform: rotate(180deg); }
    .dropdown-menu {
      display: none; position: absolute; top: calc(100% + 0.35rem); left: 0;
      background: var(--bg-surface); border: 1px solid var(--border-card); backdrop-filter: blur(16px);
      border-radius: 1.25rem; min-width: 14.5rem; box-shadow: var(--shadow-card);
      padding: 0.5rem; z-index: 200;
    }
    .dropdown:hover .dropdown-menu { display: block; }
    .dropdown-item {
      display: flex; align-items: center; gap: 0.5rem; width: 100%;
      padding: 0.65rem 0.95rem; color: var(--text-muted); font-size: 0.875rem;
      font-weight: 500; text-align: left; background: transparent;
      border: none; border-radius: 0.75rem; cursor: pointer;
    }
    .dropdown-item:hover { background: var(--bg-surface-elevated); color: var(--text-main); }
    .dropdown-item.active { background: var(--accent-indigo); color: #fff; font-weight: 700; }
    .badge-count { background: rgba(245, 158, 11, 0.2); color: var(--accent-yellow); font-size: 0.68rem; font-weight: 800; padding: 0.15rem 0.45rem; border-radius: 9999px; margin-left: auto; }

    .theme-toggle-btn {
      background: var(--bg-surface-elevated); border: 1px solid var(--border-card);
      color: var(--text-main); padding: 0.45rem 0.85rem; border-radius: 0.75rem;
      font-size: 0.95rem; cursor: pointer; display: inline-flex; align-items: center; gap: 0.35rem;
    }
    .lang-btn { background: var(--bg-surface-elevated); border: 1px solid var(--border-card); color: var(--text-main); padding: 0.45rem 0.85rem; border-radius: 0.75rem; font-weight: 700; font-size: 0.8rem; cursor: pointer; }
    .nav-toggle { display: none; background: transparent; border: 1px solid var(--border-card); color: var(--text-main); font-size: 1.4rem; padding: 0.35rem 0.75rem; border-radius: 0.5rem; cursor: pointer; }

    @media (max-width: 1024px) {
      .nav-toggle { display: block; }
      .nav-menu { display: none; position: absolute; top: 4.25rem; left: 0; right: 0; background: var(--nav-bg); border-bottom: 1px solid var(--border-card); flex-direction: column; align-items: stretch; padding: 1rem; gap: 0.5rem; }
      .nav-menu.open { display: flex; }
      .dropdown:hover .dropdown-menu { position: static; display: block; box-shadow: none; border: none; background: var(--bg-surface-elevated); padding-left: 0.5rem; margin-top: 0.2rem; }
      .dropdown-arrow { display: none; }
    }

    .content-area { max-width: 82rem; margin: 1.5rem auto; padding: 0 1.25rem; width: 100%; }
    
    .bento-grid {
      display: grid;
      grid-template-columns: repeat(12, 1fr);
      gap: 1.25rem;
      margin-bottom: 1.5rem;
    }
    .bento-card {
      background: var(--bg-surface);
      border: 1px solid var(--border-card);
      border-radius: 1.75rem;
      padding: 1.75rem;
      box-shadow: var(--shadow-card);
      backdrop-filter: blur(16px);
      position: relative;
      overflow: hidden;
    }
    .col-span-4 { grid-column: span 12; }
    .col-span-6 { grid-column: span 12; }
    .col-span-8 { grid-column: span 12; }
    .col-span-12 { grid-column: span 12; }
    
    @media (min-width: 768px) {
      .col-span-4 { grid-column: span 4; }
      .col-span-6 { grid-column: span 6; }
      .col-span-8 { grid-column: span 8; }
    }

    .gauge-wrapper { display: flex; flex-direction: column; align-items: center; justify-content: center; position: relative; }
    .gauge-svg { width: 220px; height: 130px; }
    .gauge-bg { fill: none; stroke: var(--border-card); stroke-width: 16; stroke-linecap: round; }
    .gauge-val { fill: none; stroke: var(--accent-green); stroke-width: 16; stroke-linecap: round; stroke-dasharray: 252; transition: stroke-dashoffset 1s ease-out; }
    .gauge-content { text-align: center; margin-top: -3.5rem; }
    .gauge-number { font-size: 2.2rem; font-weight: 800; color: var(--accent-green); letter-spacing: -0.03em; }
    .gauge-label { font-size: 0.8rem; color: var(--text-muted); font-weight: 600; text-transform: uppercase; }

    .pill-metric {
      background: var(--bg-surface-elevated);
      border: 1px solid var(--border-card);
      border-radius: 1.25rem;
      padding: 1rem 1.25rem;
      display: flex;
      align-items: center;
      justify-content: space-between;
    }

    .table-container { overflow-x: auto; border-radius: 1.25rem; border: 1px solid var(--border-card); margin-top: 1rem; }
    table { width: 100%; border-collapse: collapse; text-align: left; font-size: 0.92rem; }
    th { background: var(--bg-surface-elevated); padding: 1rem 1.25rem; font-weight: 600; color: var(--text-muted); border-bottom: 1px solid var(--border-card); }
    td { padding: 1rem 1.25rem; border-bottom: 1px solid var(--border-card); color: var(--text-main); }
    .btn { padding: 0.65rem 1.2rem; border-radius: 1rem; font-weight: 600; font-size: 0.88rem; cursor: pointer; border: none; text-decoration: none; display: inline-flex; align-items: center; justify-content: center; gap: 0.4rem; }
    .btn-emerald { background-color: var(--accent-green); color: #064e3b; font-weight: 700; }
    .btn-indigo { background-color: var(--accent-indigo); color: white; }
    .btn-rose { background-color: var(--accent-rose); color: white; }
    .btn-slate { background-color: var(--bg-surface-elevated); color: var(--text-main); border: 1px solid var(--border-card); }
    .form-input { width: 100%; padding: 0.8rem 1.1rem; border-radius: 1rem; border: 1px solid var(--border-card); background: var(--bg-surface-elevated); color: var(--text-main); font-size: 0.95rem; margin-bottom: 1rem; outline: none; }
    .modal { display: none; position: fixed; inset: 0; background: rgba(0, 0, 0, 0.7); backdrop-filter: blur(12px); z-index: 250; align-items: center; justify-content: center; padding: 1.5rem; }
    .modal.active { display: flex; }
    .dashboard-footer { margin-top: 3.5rem; padding: 2.5rem 1.25rem; border-top: 1px solid var(--border-card); text-align: center; font-size: 0.85rem; color: var(--text-muted); }
  </style>
</head>
<body>
  <header class="navbar">
    <div class="nav-brand">
      <span class="brand-badge">BENTO V4</span>
      <span class="brand-title" data-th="ระบบบริหารจัดการอาหารกลางวันนิสิต" data-en="Student Meal Subsidy Portal">ระบบบริหารจัดการอาหารกลางวันนิสิต</span>
    </div>
    <button class="nav-toggle" onclick="toggleNav()">☰</button>
    
    <nav class="nav-menu" id="navMenu">
      <button onclick="switchTab('dashboard')" id="btn-dashboard" class="nav-item active" data-th="📊 แดชบอร์ด" data-en="📊 Dashboard">📊 แดชบอร์ด</button>

      <div class="dropdown">
        <button class="nav-item dropdown-btn" id="group-beneficiaries">
          <span data-th="👥 ทะเบียน & สิทธิ์" data-en="👥 Beneficiaries">👥 ทะเบียน & สิทธิ์</span>
          <span class="dropdown-arrow">▼</span>
        </button>
        <div class="dropdown-menu">
          <button onclick="switchTab('students')" id="btn-students" class="dropdown-item" data-th="👥 รายชื่อผู้มีสิทธิ์" data-en="👥 Directory">👥 รายชื่อผู้มีสิทธิ์</button>
          <button onclick="switchTab('tempcard')" id="btn-tempcard" class="dropdown-item">
            <span data-th="💳 บัตรสำรอง" data-en="💳 Temp Cards">💳 บัตรสำรอง</span>
            <span class="badge-count" id="tempCount">)rawliteral" + String(activeTempWaitingCount) + R"rawliteral(</span>
          </button>
          <button onclick="switchTab('import')" id="btn-import" class="dropdown-item" data-th="📥 นำเข้ารายชื่อ CSV" data-en="📥 Import CSV">📥 นำเข้ารายชื่อ CSV</button>
        </div>
      </div>

      <div class="dropdown">
        <button class="nav-item dropdown-btn" id="group-admin">
          <span data-th="⚙️ จัดการระบบ" data-en="⚙️ Administration">⚙️ จัดการระบบ</span>
          <span class="dropdown-arrow">▼</span>
        </button>
        <div class="dropdown-menu">
          <button onclick="switchTab('shops')" id="btn-shops" class="dropdown-item" data-th="🏪 ร้านค้าพันธมิตร" data-en="🏪 Partner Vendors">🏪 ร้านค้าพันธมิตร</button>
          <button onclick="switchTab('archives')" id="btn-archives" class="dropdown-item" data-th="🗄️ คลังรายงานย้อนหลัง" data-en="🗄️ Audit Archives">🗄️ คลังรายงานย้อนหลัง</button>
          <button onclick="switchTab('admins')" id="btn-admins" class="dropdown-item" data-th="👤 เจ้าหน้าที่ดูแลระบบ" data-en="👤 System Officers">👤 เจ้าหน้าที่ดูแลระบบ ()rawliteral" + String(adminUsers.size()) + R"rawliteral(/3)</button>
          <button onclick="switchTab('settings')" id="btn-settings" class="dropdown-item" data-th="⚙️ กำหนดเวลา & ระบบ" data-en="⚙️ System Settings">⚙️ กำหนดเวลา & ระบบ</button>
        </div>
      </div>

      <div style="display: flex; align-items: center; gap: 0.5rem; margin-left: 0.5rem;">
        <button onclick="toggleTheme()" class="theme-toggle-btn" id="themeBtn" title="Toggle Light/Dark Mode">🌙</button>
        <button onclick="toggleLanguage()" class="lang-btn" id="langSwitch">EN</button>
        <a href="/logout" class="btn btn-rose" style="padding: 0.45rem 0.85rem; font-size: 0.8rem;" data-th="🚪 ออกจากระบบ" data-en="🚪 Sign Out">🚪 ออกจากระบบ</a>
      </div>
    </nav>
  </header>

  <main class="content-area">
    
    <!-- TAB 1: BENTO DASHBOARD -->
    <div id="tab-dashboard" class="tab-content" style="display: block;">
      <div class="bento-grid">
        
        <!-- Bento 1: Radial Quota Arc Gauge -->
        <div class="bento-card col-span-4" style="display: flex; flex-direction: column; justify-content: space-between;">
          <div style="display: flex; justify-content: space-between; align-items: center;">
            <span style="font-size: 0.85rem; font-weight: 700; color: var(--text-muted); text-transform: uppercase;" data-th="โควตาสวัสดิการวันนี้" data-en="Today's Meal Quota">โควตาสวัสดิการวันนี้</span>
            <span style="background: var(--accent-green-glow); color: var(--accent-green); font-size: 0.75rem; font-weight: 800; padding: 0.25rem 0.6rem; border-radius: 9999px;">35 THB / Meal</span>
          </div>

          <div class="gauge-wrapper" style="margin: 1.25rem 0;">
            <svg class="gauge-svg" viewBox="0 0 200 110">
              <path class="gauge-bg" d="M 20 100 A 80 80 0 0 1 180 100"></path>
              <path class="gauge-val" id="gaugeArc" d="M 20 100 A 80 80 0 0 1 180 100" style="stroke-dashoffset: )rawliteral" + String(252 - (quotaPercent * 252) / 100) + R"rawliteral(;"></path>
            </svg>
            <div class="gauge-content">
              <div class="gauge-number">)rawliteral" + String(usedCount) + R"rawliteral(</div>
              <div class="gauge-label" data-th="ใช้สิทธิ์แล้ว (คน)" data-en="CLAIMED STUDENTS">ใช้สิทธิ์แล้ว (คน)</div>
            </div>
          </div>

          <div style="display: flex; justify-content: space-between; border-top: 1px solid var(--border-card); padding-top: 1rem; font-size: 0.85rem;">
            <div>
              <span style="color: var(--text-muted);" data-th="คงเหลือ:" data-en="Remaining:">คงเหลือ:</span>
              <b style="color: var(--text-main);">)rawliteral" + String(db.size() - usedCount) + R"rawliteral(</b>
            </div>
            <div>
              <span style="color: var(--text-muted);" data-th="ยอดจัดสรร:" data-en="Disbursed:">ยอดจัดสรร:</span>
              <b style="color: var(--accent-green);">)rawliteral" + String(usedCount * 35) + R"rawliteral( THB</b>
            </div>
          </div>
        </div>

        <!-- Bento 2: Host Hardware Telemetry -->
        <div class="bento-card col-span-4" style="display: flex; flex-direction: column; justify-content: space-between;">
          <div style="display: flex; justify-content: space-between; align-items: center;">
            <span style="font-size: 0.85rem; font-weight: 700; color: var(--text-muted); text-transform: uppercase;" data-th="สถานะฮาร์ดแวร์แม่ข่าย" data-en="Host Telemetry">สถานะฮาร์ดแวร์แม่ข่าย</span>
            <span style="color: var(--accent-cyan); font-size: 0.75rem; font-weight: 700;">ESP32-S3 N16R8</span>
          </div>

          <div style="display: flex; flex-direction: column; gap: 0.75rem; margin: 1rem 0;">
            <div class="pill-metric">
              <div>
                <div style="font-size: 0.75rem; color: var(--text-muted);">CORE TEMPERATURE</div>
                <div id="telemetryTemp" style="font-size: 1.4rem; font-weight: 800; color: var(--accent-green); margin-top: 0.2rem;">)rawliteral" + String(initialTemp, 1) + R"rawliteral( °C</div>
              </div>
              <div style="font-size: 1.6rem;">🌡️</div>
            </div>

            <div class="pill-metric">
              <div>
                <div style="font-size: 0.75rem; color: var(--text-muted);">PROCESSOR LOAD</div>
                <div id="telemetryCpu" style="font-size: 1.4rem; font-weight: 800; color: var(--accent-indigo); margin-top: 0.2rem;">)rawliteral" + String(initialCpu, 1) + R"rawliteral( %</div>
              </div>
              <div style="font-size: 1.6rem;">⚡</div>
            </div>
          </div>

          <div style="display: flex; justify-content: space-between; font-size: 0.8rem; color: var(--text-muted); border-top: 1px solid var(--border-card); padding-top: 0.85rem;">
            <span>🔋 Batt: <b style="color:var(--text-main);">)rawliteral" + String(hostBattPct) + R"rawliteral(%</b></span>
            <span>💾 PSRAM: <b style="color:var(--text-main);">8 MB (OPI)</b></span>
          </div>
        </div>

        <!-- Bento 3: Standard Clock & Quick Actions -->
        <div class="bento-card col-span-4" style="display: flex; flex-direction: column; justify-content: space-between; background: linear-gradient(135deg, var(--bg-surface), var(--bg-surface-elevated));">
          <div>
            <div style="font-size: 0.85rem; font-weight: 700; color: var(--accent-yellow); text-transform: uppercase;" data-th="เวลามาตรฐานระบบ" data-en="Standard RTC Time">เวลามาตรฐานระบบ</div>
            <div id="clockText" style="font-size: 1.8rem; font-weight: 800; margin: 0.5rem 0; letter-spacing: -0.02em;">)rawliteral" + getRealTimeStr() + R"rawliteral(</div>
            <div style="font-size: 0.82rem; color: var(--text-muted);" data-th="ปรับเทียบผ่านชิป DS3231 Precision I2C" data-en="Synced with DS3231 Precision I2C">ปรับเทียบผ่านชิป DS3231 Precision I2C</div>
          </div>

          <div style="display: flex; flex-direction: column; gap: 0.5rem; margin-top: 1rem;">
            <a href="/export.csv" class="btn btn-emerald" style="width: 100%;" data-th="📥 ดาวน์โหลดรายงานสรุป (CSV)" data-en="📥 Export Summary (CSV)">📥 ดาวน์โหลดรายงานสรุป (CSV)</a>
            <button onclick="syncDeviceTime()" class="btn btn-slate" style="width: 100%;" data-th="⚡ ซิงค์เวลากับเครื่องนี้" data-en="⚡ Sync Device Time">⚡ ซิงค์เวลากับเครื่องนี้</button>
          </div>
        </div>

        <!-- Bento 4: 4 Stations Live Status -->
        <div class="bento-card col-span-12">
          <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 1.25rem;">
            <h3 style="font-size: 1.1rem; font-weight: 800;" data-th="📡 จุดให้บริการร้านค้าประจำโรงอาหาร (4 Stations Live Status)" data-en="📡 Canteen Vendor Stations (4 Stations Live Status)">📡 จุดให้บริการร้านค้าประจำโรงอาหาร (4 Stations Live Status)</h3>
            <span style="font-size: 0.8rem; color: var(--text-muted);">ESP-NOW Channel 1</span>
          </div>

          <div class="bento-grid" style="margin-bottom: 0;">
  )rawliteral";

  for (int i = 0; i < 4; i++) {
    int stBattPct = getHostBatteryPercentage(stationNodes[i].systemVoltage);
    html += "<div class='col-span-6 bento-card' style='padding: 1.25rem; background: var(--bg-surface-elevated); margin-bottom: 0;'>";
    html += "<div style='display: flex; justify-content: space-between; align-items: flex-start;'>";
    html += "<div><h4 style='font-size: 1.05rem; font-weight: 800;'>" + shops[i].name + "</h4>";
    html += "<p style='font-size: 0.82rem; color: var(--text-muted); margin-top: 0.2rem;'>" + shops[i].vendor + "</p></div>";
    
    if (stationNodes[i].isOnline) {
      html += "<span style='background: var(--accent-green-glow); color: var(--accent-green); font-size: 0.72rem; font-weight: 800; padding: 0.25rem 0.6rem; border-radius: 9999px;'>ONLINE</span>";
    } else {
      html += "<span style='background: rgba(244,63,94,0.15); color: var(--accent-rose); font-size: 0.72rem; font-weight: 800; padding: 0.25rem 0.6rem; border-radius: 9999px;'>OFFLINE</span>";
    }
    html += "</div>";

    html += "<div style='display: flex; justify-content: space-between; align-items: center; margin-top: 1.25rem; border-top: 1px dashed var(--border-card); padding-top: 0.75rem;'>";
    html += "<div><span style='font-size: 0.75rem; color: var(--text-muted);'>ORDERS</span><div style='font-size: 1.25rem; font-weight: 800; color: var(--accent-green);'>" + String(shopCounts[i]) + " <span style='font-size:0.75rem; font-weight:400; color:var(--text-muted);'>จาน</span></div></div>";
    html += "<div><span style='font-size: 0.75rem; color: var(--text-muted);'>TOTAL AMOUNT</span><div style='font-size: 1.25rem; font-weight: 800; color: var(--accent-yellow);'>" + String(shopCounts[i] * 35) + " <span style='font-size:0.75rem; font-weight:400; color:var(--text-muted);'>B.</span></div></div>";
    html += "<div><span style='font-size: 0.75rem; color: var(--text-muted);'>BATTERY</span><div style='font-size: 1.25rem; font-weight: 800; color: var(--text-main);'>" + (stationNodes[i].isOnline ? String(stBattPct) + "%" : "-") + "</div></div>";
    html += "</div></div>";
  }

  html += R"rawliteral(
          </div>
        </div>

        <!-- Bento 5: Chart & Breakdown -->
        <div class="bento-card col-span-12">
          <h3 style="font-size: 1.1rem; font-weight: 800; margin-bottom: 1rem;" data-th="📈 สัดส่วนการใช้บริการจำแนกตามร้านค้า" data-en="📈 Service Distribution by Vendor">📈 สัดส่วนการใช้บริการจำแนกตามร้านค้า</h3>
          <div style="text-align: center; padding: 1rem 0;">
            <canvas id="analyticsChart" width="700" height="230" style="max-width: 100%; height: auto;"></canvas>
          </div>
        </div>

      </div>
    </div>

    <!-- TAB 2: STUDENTS -->
    <div id="tab-students" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12">
        <div style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 0.75rem; margin-bottom: 1.25rem;">
          <h2 style="font-size: 1.2rem; font-weight: 800;" data-th="👥 บัญชีรายชื่อผู้มีสิทธิ์รับสวัสดิการ" data-en="👥 Eligible Beneficiary Directory">👥 บัญชีรายชื่อผู้มีสิทธิ์รับสวัสดิการ</h2>
          <div style="display: flex; gap: 0.75rem;">
            <button onclick="openAddModal()" class="btn btn-indigo" data-th="➕ เพิ่มผู้มีสิทธิ์" data-en="➕ Add Student">➕ เพิ่มผู้มีสิทธิ์</button>
            <a href="/reset" onclick="return confirm(currentLang==='th'?'ต้องการรีเซ็ตสิทธิ์และจัดเก็บข้อมูลวันนี้เข้าคลังใช่หรือไม่?':'Perform daily reset and archive today records?')" class="btn btn-rose" data-th="🔄 ปิดยอดประจำวัน & จัดเก็บประวัติ" data-en="🔄 Daily Reset & Archive">🔄 ปิดยอดประจำวัน & จัดเก็บประวัติ</a>
          </div>
        </div>
        <div style="display: flex; gap: 0.75rem;">
          <input type="text" id="search" onkeyup="handleSearch(this.value)" placeholder="ค้นหารหัสนิสิต, ชื่อ-สกุล หรือเลขประจำตัว..." class="form-input" style="flex: 1; margin-bottom: 0;">
          <select id="pageSizeSelect" onchange="changePageSize(this.value)" class="form-input" style="width: 7.5rem; margin-bottom: 0;">
            <option value="20" selected>20 แถว</option>
            <option value="50">50 แถว</option>
            <option value="100">100 แถว</option>
          </select>
        </div>
        <div class="table-container">
          <table id="studentTable">
            <thead><tr><th data-th="รหัสนิสิต" data-en="Student ID">รหัสนิสิต</th><th data-th="ชื่อ-สกุล" data-en="Full Name">ชื่อ-สกุล</th><th data-th="เลขบัตรสมาร์ตการ์ด" data-en="Card UID">เลขบัตรสมาร์ตการ์ด</th><th data-th="เลขที่อ้างอิง" data-en="Ref No.">เลขที่อ้างอิง</th><th data-th="เวลาที่ใช้สิทธิ์" data-en="Timestamp">เวลาที่ใช้สิทธิ์</th><th data-th="จุดบริการ" data-en="Point">จุดบริการ</th><th style="text-align: right;" data-th="จัดการ" data-en="Action">จัดการ</th></tr></thead>
            <tbody id="studentTableBody">
              <tr><td colspan="7" style="padding: 2.5rem; text-align: center; color: var(--text-muted);">Loading data...</td></tr>
            </tbody>
          </table>
        </div>
        <div class="pagination-bar" style="display: flex; flex-direction: column; gap: 1rem; justify-content: space-between; align-items: center; padding-top: 1.25rem;">
          <div id="paginationInfo" style="color: var(--text-muted); font-size: 0.88rem;">Loading...</div>
          <div style="display: flex; gap: 0.4rem; align-items: center;">
            <button onclick="goToPage(1)" class="btn btn-slate" style="padding: 0.4rem 0.8rem;" data-th="⏮️ แรกสุด" data-en="⏮️ First">⏮️ แรกสุด</button>
            <button onclick="prevPage()" class="btn btn-slate" style="padding: 0.4rem 0.8rem;" data-th="◀ ก่อนหน้า" data-en="◀ Prev">◀ ก่อนหน้า</button>
            <span id="pageIndicator" style="padding: 0.4rem 0.85rem; font-weight: 800; color: var(--accent-indigo); background: rgba(99, 102, 241, 0.15); border-radius: 0.85rem;">1</span>
            <button onclick="nextPage()" class="btn btn-slate" style="padding: 0.4rem 0.8rem;" data-th="ถัดไป ▶" data-en="Next ▶">ถัดไป ▶</button>
            <button onclick="goToLastPage()" class="btn btn-slate" style="padding: 0.4rem 0.8rem;" data-th="ท้ายสุด ⏭️" data-en="Last ⏭️">ท้ายสุด ⏭️</button>
          </div>
        </div>
      </div>
    </div>

    <!-- TAB 3: TEMP CARD -->
    <div id="tab-tempcard" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12">
        <div style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 0.75rem; margin-bottom: 1.25rem;">
          <div>
            <h2 style="font-size: 1.2rem; font-weight: 800;" data-th="💳 บริหารจัดการบัตรสำรอง (One-Time Auto-Revoke)" data-en="💳 Temporary Cards (One-Time Auto-Revoke)">💳 บริหารจัดการบัตรสำรอง (One-Time Auto-Revoke)</h2>
            <p style="font-size: 0.85rem; color: var(--text-muted); margin-top: 0.2rem;" data-th="* บัตรจะถูกปลดออกจากระบบอัตโนมัติทันทีเมื่อนิสิตแตะรับสิทธิ์สำเร็จ เพื่อนำบัตรเดิมไปเวียนใช้งานต่อได้ทันที" data-en="* Cards are automatically unlinked upon successful tap, allowing immediate reuse.">* บัตรจะถูกปลดออกจากระบบอัตโนมัติทันทีเมื่อนิสิตแตะรับสิทธิ์สำเร็จ เพื่อนำบัตรเดิมไปเวียนใช้งานต่อได้ทันที</p>
          </div>
          <button onclick="openTempModal()" class="btn btn-emerald" data-th="➕ ผูกบัตรสำรองใหม่" data-en="➕ Assign Temporary Card">➕ ผูกบัตรสำรองใหม่</button>
        </div>
        <div class="table-container">
          <table>
            <thead><tr><th data-th="รหัสนิสิต" data-en="Student ID">รหัสนิสิต</th><th data-th="ชื่อ-สกุล" data-en="Full Name">ชื่อ-สกุล</th><th data-th="เลขบัตรที่ผูก" data-en="Assigned UID">เลขบัตรที่ผูก</th><th data-th="สถานะบัตรสำรอง" data-en="Temp Card Status">สถานะบัตรสำรอง</th><th style="text-align: right;" data-th="การจัดการ" data-en="Action">การจัดการ</th></tr></thead>
            <tbody>
  )rawliteral";

  bool hasTempRecords = false;
  for (size_t i = 0; i < db.size(); i++) {
    if (db[i].isTempCard) {
      hasTempRecords = true;
      String statusBadge = "";
      String actionBtn = "";

      if (db[i].claimed) {
        statusBadge = "<span style='background:rgba(16,185,129,0.2); color:var(--accent-green); padding:0.25rem 0.65rem; border-radius:9999px; font-size:0.75rem; font-weight:800;'>CLAIMED & REVOKED</span>";
        actionBtn = "<span style='color:var(--text-muted); font-size:0.8rem;' data-th='คืนสู่ส่วนกลางแล้ว' data-en='Card Returned'>คืนสู่ส่วนกลางแล้ว</span>";
      } else {
        statusBadge = "<span style='background:rgba(245,158,11,0.2); color:var(--accent-yellow); padding:0.25rem 0.65rem; border-radius:9999px; font-size:0.75rem; font-weight:800;'>WAITING TAP</span>";
        actionBtn = "<button onclick='removeTempCard(\"" + db[i].studentId + "\")' class='btn btn-slate' style='color:var(--accent-rose); padding:0.35rem 0.65rem; font-size:0.75rem;'>Revoke</button>";
      }

      html += "<tr><td><b>" + db[i].studentId + "</b></td><td>" + db[i].fullName + "</td><td><code>" + db[i].uid + "</code></td>";
      html += "<td>" + statusBadge + "</td>";
      html += "<td style='text-align: right;'>" + actionBtn + "</td></tr>";
    }
  }
  if (!hasTempRecords) html += "<tr><td colspan='5' style='text-align: center; color: var(--text-muted); padding: 2rem;' data-th='ไม่มีรายการบัตรสำรองในวันนี้' data-en='No temporary card activity today'>ไม่มีรายการบัตรสำรองในวันนี้</td></tr>";

  html += R"rawliteral(
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <!-- TAB 4: ARCHIVES -->
    <div id="tab-archives" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12">
        <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 1rem;" data-th="🗄️ คลังรายงานประวัติย้อนหลัง (Audit Log Storage)" data-en="🗄️ Historical Audit Log Storage">🗄️ คลังรายงานประวัติย้อนหลัง (Audit Log Storage)</h2>
        <div class="table-container">
          <table>
            <thead><tr><th data-th="ชื่อไฟล์รายงาน" data-en="Archive Filename">ชื่อไฟล์รายงาน</th><th data-th="ขนาดไฟล์" data-en="File Size">ขนาดไฟล์</th><th style="text-align:right;" data-th="การจัดการ" data-en="Action">การจัดการ</th></tr></thead>
            <tbody>
  )rawliteral";

  html += archivesHtml;

  html += R"rawliteral(
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <!-- TAB 5: ADMINS -->
    <div id="tab-admins" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12">
        <div style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 0.75rem; margin-bottom: 1.25rem;">
          <div>
            <h2 style="font-size: 1.2rem; font-weight: 800;" data-th="👤 รายชื่อเจ้าหน้าที่ผู้มีสิทธิ์เข้าถึงระบบ (จำกัด 3 ท่าน)" data-en="👤 System Authorized Officers (Max 3 Users)">👤 รายชื่อเจ้าหน้าที่ผู้มีสิทธิ์เข้าถึงระบบ (จำกัด 3 ท่าน)</h2>
            <p style="font-size: 0.85rem; color: var(--text-muted); margin-top: 0.2rem;" data-th="เจ้าหน้าที่สามารถเข้าสู่ระบบเพื่อตรวจสอบแดชบอร์ด จัดการสิทธิ์ และส่งออกรายงานได้" data-en="Officers can log in to audit real-time subsidy claims, assign cards, and export logs.">เจ้าหน้าที่สามารถเข้าสู่ระบบเพื่อตรวจสอบแดชบอร์ด จัดการสิทธิ์ และส่งออกรายงานได้</p>
          </div>
  )rawliteral";

  if (adminUsers.size() < 3) {
    html += "<button onclick=\"openAddAdminModal()\" class=\"btn btn-indigo\" data-th=\"➕ เพิ่มเจ้าหน้าที่ใหม่\" data-en=\"➕ Add New Officer\">➕ เพิ่มเจ้าหน้าที่ใหม่</button>";
  } else {
    html += "<span style=\"color:var(--accent-yellow); background:rgba(245,158,11,0.2); padding:0.4rem 0.8rem; border-radius:0.75rem; font-size:0.8rem; font-weight:800;\">⚠️ Quota Reached (3/3)</span>";
  }

  html += R"rawliteral(
        </div>
        <div class="table-container">
          <table>
            <thead><tr><th data-th="ลำดับ" data-en="No.">ลำดับ</th><th data-th="ชื่อ-ตำแหน่งเจ้าหน้าที่" data-en="Officer Name / Role">ชื่อ-ตำแหน่งเจ้าหน้าที่</th><th data-th="ชื่อผู้ใช้ (Username)" data-en="Username">ชื่อผู้ใช้ (Username)</th><th data-th="รหัสผ่าน" data-en="Password">รหัสผ่าน</th><th style="text-align:right;" data-th="การจัดการ" data-en="Action">การจัดการ</th></tr></thead>
            <tbody>
  )rawliteral";

  for (size_t i = 0; i < adminUsers.size(); i++) {
    html += "<tr><td style='color:var(--text-muted); font-weight:600;'>" + String(i + 1) + "</td>";
    html += "<td style='font-weight:700;'>" + adminUsers[i].displayName + "</td>";
    html += "<td><code>" + adminUsers[i].username + "</code></td>";
    html += "<td style='color:var(--text-muted);'>••••••••</td>";
    html += "<td style='text-align:right;'>";
    html += "<button onclick=\"openEditAdminModal('" + adminUsers[i].username + "','" + adminUsers[i].displayName + "')\" class=\"btn btn-slate\" style=\"padding:0.35rem 0.75rem; font-size:0.8rem;\">✏️ Edit</button> ";
    if (adminUsers.size() > 1) {
      html += "<a href=\"/api/admin/delete?user=" + adminUsers[i].username + "\" onclick=\"return confirm('ยืนยันลบเจ้าหน้าที่ " + adminUsers[i].username + " หรือไม่?');\" class=\"btn btn-rose\" style=\"padding:0.35rem 0.75rem; font-size:0.8rem;\">🗑️ Delete</a>";
    }
    html += "</td></tr>";
  }

  html += R"rawliteral(
            </tbody>
          </table>
        </div>
      </div>
    </div>

    <!-- TAB 6: SETTINGS -->
    <div id="tab-settings" class="tab-content" style="display: none;">
      <div class="bento-grid">
        <div class="col-span-6 bento-card">
          <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.5rem;" data-th="🕒 ตั้งค่าเวลามาตรฐานระบบ" data-en="🕒 System Time Synchronization">🕒 ตั้งค่าเวลามาตรฐานระบบ</h2>
          <form method="POST" action="/api/rtc/set" style="margin-top: 1rem;">
            <label style="font-size: 0.85rem; font-weight: 700;">กำหนดวันที่:</label>
            <input type="date" name="date" value=")rawliteral" + String(dateInputBuf) + R"rawliteral(" required class="form-input">
            <label style="font-size: 0.85rem; font-weight: 700;">กำหนดเวลา:</label>
            <input type="text" name="time" value=")rawliteral" + String(timeInputBuf) + R"rawliteral(" required class="form-input" placeholder="HH:MM:SS">
            <button type="submit" class="btn btn-indigo" style="width: 100%;">💾 บันทึกเวลามาตรฐาน</button>
          </form>
        </div>

        <div class="col-span-6 bento-card">
          <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.5rem;" data-th="⚙️ กำหนดช่วงเวลาเปิดให้บริการอาหาร" data-en="⚙️ Service Hours Window">⚙️ กำหนดช่วงเวลาเปิดให้บริการอาหาร</h2>
          <form method="POST" action="/api/settings/time" style="margin-top: 1rem;">
            <label style="font-size: 0.85rem; font-weight: 700;">สถานะการจำกัดเวลา:</label>
            <select name="enabled" class="form-input">
              <option value="1" )rawliteral" + String(timeWindowEnabled ? "selected" : "") + R"rawliteral(>เปิดใช้งาน (จำกัดเวลาตามกำหนด)</option>
              <option value="0" )rawliteral" + String(!timeWindowEnabled ? "selected" : "") + R"rawliteral(>ปิดใช้งาน (เปิดบริการตลอดเวลา)</option>
            </select>
            <div style="display:flex; gap:1rem;">
              <div style="flex:1;">
                <label style="font-size: 0.85rem; font-weight: 700;">เวลาเปิด:</label>
                <input type="text" name="start" value=")rawliteral" + String(serviceStartHour < 10 ? "0" : "") + String(serviceStartHour) + ":" + String(serviceStartMin < 10 ? "0" : "") + String(serviceStartMin) + R"rawliteral(" class="form-input">
              </div>
              <div style="flex:1;">
                <label style="font-size: 0.85rem; font-weight: 700;">เวลาปิด:</label>
                <input type="text" name="end" value=")rawliteral" + String(serviceEndHour < 10 ? "0" : "") + String(serviceEndHour) + ":" + String(serviceEndMin < 10 ? "0" : "") + String(serviceEndMin) + R"rawliteral(" class="form-input">
              </div>
            </div>
            <button type="submit" class="btn btn-indigo" style="width: 100%;">💾 บันทึกกำหนดเวลา</button>
          </form>
        </div>
      </div>
    </div>

    <!-- TAB 7: IMPORT -->
    <div id="tab-import" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12" style="max-width: 36rem; margin: auto; text-align: center;">
        <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.5rem;">📥 นำเข้าบัญชีรายชื่อนิสิต (Smart Upsert)</h2>
        <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1.5rem;">* ระบบจะไม่ลบรายชื่อเก่า: รหัสเดิมจะถูกอัปเดตข้อมูล และรหัสใหม่จะถูกเพิ่มเข้าสู่ฐานข้อมูลอัตโนมัติ</p>
        <form method="POST" action="/upload" enctype="multipart/form-data" style="border: 2px dashed var(--border-card); border-radius: 1.5rem; padding: 2rem;">
          <input type="file" name="csv" accept=".csv" required style="margin-bottom: 1.25rem;"><br>
          <button type="submit" class="btn btn-emerald">🚀 อัปโหลดและผสานข้อมูล</button>
        </form>
      </div>
    </div>

    <!-- TAB 8: SHOPS -->
    <div id="tab-shops" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12" style="max-width: 38rem; margin: auto;">
        <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 1rem;">🏪 จัดการข้อมูลร้านค้าและผู้ประกอบการ</h2>
        <form method="POST" action="/save-shops">
  )rawliteral";

  for (int i = 0; i < 4; i++) {
    html += "<div style='background: var(--bg-surface-elevated); border: 1px solid var(--border-card); border-radius: 1.25rem; padding: 1.25rem; margin-bottom: 1rem;'>";
    html += "<h4 style='color: var(--accent-indigo); margin-bottom: 0.5rem; font-weight: 800;'>Point " + String(i + 1) + "</h4>";
    html += "<input type='text' name='sname" + String(i) + "' value='" + shops[i].name + "' required class='form-input'>";
    html += "<input type='text' name='vname" + String(i) + "' value='" + shops[i].vendor + "' required class='form-input' style='margin-bottom:0;'></div>";
  }

  html += R"rawliteral(
          <button type="submit" class="btn btn-indigo" style="width: 100%;">💾 บันทึกข้อมูลร้านค้า</button>
        </form>
      </div>
    </div>

    <!-- Modals -->
    <div id="studentModal" class="modal">
      <div class="bento-card" style="max-width: 28rem; width: 100%;">
        <h3 id="modalTitle" style="font-size: 1.2rem; font-weight: 800; margin-bottom: 1rem;">Beneficiary Details</h3>
        <form method="POST" action="/api/student/save">
          <input type="hidden" id="modalOldId" name="oldStudentId">
          <label style="font-size: 0.85rem; font-weight: 700;">รหัสนิสิต:</label>
          <input type="text" id="modalId" name="studentId" required class="form-input">
          <label style="font-size: 0.85rem; font-weight: 700;">ชื่อ-สกุล:</label>
          <input type="text" id="modalName" name="fullName" required class="form-input">
          <label style="font-size: 0.85rem; font-weight: 700;">เลขสมาร์ตการ์ด:</label>
          <input type="text" id="modalUid" name="uid" class="form-input" placeholder="e.g. 0305419896">
          <div style="display: flex; justify-content: flex-end; gap: 0.5rem;">
            <button type="button" onclick="closeModal()" class="btn btn-slate">ยกเลิก</button>
            <button type="submit" class="btn btn-indigo">บันทึก</button>
          </div>
        </form>
      </div>
    </div>

    <div id="tempCardModal" class="modal">
      <div class="bento-card" style="max-width: 28rem; width: 100%;">
        <h3 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 1rem;">ผูกบัตรสำรองกรณีพิเศษ</h3>
        <form method="POST" action="/api/tempcard/save">
          <label style="font-size: 0.85rem; font-weight: 700;">รหัสนิสิต:</label>
          <input type="text" name="studentId" required class="form-input">
          <label style="font-size: 0.85rem; font-weight: 700;">เลขบัตรสำรอง (10 หลัก):</label>
          <input type="text" name="uid" required class="form-input" placeholder="e.g. 0305419896">
          <div style="display: flex; justify-content: flex-end; gap: 0.5rem;">
            <button type="button" onclick="closeTempModal()" class="btn btn-slate">ยกเลิก</button>
            <button type="submit" class="btn btn-emerald">ผูกบัตร</button>
          </div>
        </form>
      </div>
    </div>

    <div id="adminModal" class="modal">
      <div class="bento-card" style="max-width: 28rem; width: 100%;">
        <h3 id="adminModalTitle" style="font-size: 1.2rem; font-weight: 800; margin-bottom: 1rem;">Officer Details</h3>
        <form method="POST" action="/api/admin/save">
          <input type="hidden" id="adminOldUser" name="oldUsername">
          <label style="font-size: 0.85rem; font-weight: 700;">ชื่อ-ตำแหน่งเจ้าหน้าที่ (Display Name):</label>
          <input type="text" id="adminDisplayName" name="displayName" required class="form-input" placeholder="e.g. นายกิตติพันธ์ รัตนคร (IT Officer)">
          <label style="font-size: 0.85rem; font-weight: 700;">ชื่อผู้ใช้ (Username):</label>
          <input type="text" id="adminUsername" name="username" required class="form-input" placeholder="e.g. officer01">
          <label style="font-size: 0.85rem; font-weight: 700;">รหัสผ่าน (Password):</label>
          <input type="password" id="adminPassword" name="password" required class="form-input" placeholder="••••••••">
          <div style="display: flex; justify-content: flex-end; gap: 0.5rem;">
            <button type="button" onclick="closeAdminModal()" class="btn btn-slate">ยกเลิก</button>
            <button type="submit" class="btn btn-indigo">บันทึกเจ้าหน้าที่</button>
          </div>
        </form>
      </div>
    </div>
  </main>

  <footer class="dashboard-footer">
    <p style="font-weight: 800; font-size: 0.95rem; color: var(--text-main); margin-bottom: 0.35rem;">
      ระบบบริหารจัดการคูปองอาหารดิจิทัล (Smart Canteen Bento Suite)
    </p>
    <p style="margin-bottom: 0.25rem;">
      ออกแบบและพัฒนาโดย: <b style="color: var(--accent-indigo);">กิตติพันธ์ รัตนคร</b> | นักวิชาการคอมพิวเตอร์
    </p>
    <p style="color: var(--text-muted); font-size: 0.8rem;">
      มหาวิทยาลัยมหาจุฬาลงกรณราชวิทยาลัย วิทยาเขตแพร่ (MCU Phrae Campus)
    </p>
  </footer>

  <script>
    var curPage = 1, pageSize = 20, totalPages = 1, searchQuery = "", searchTimer = null;
    var currentLang = 'th';

    function initTheme() {
      var savedTheme = localStorage.getItem('canteen_theme') || 'dark';
      document.documentElement.setAttribute('data-theme', savedTheme);
      document.getElementById('themeBtn').innerText = (savedTheme === 'dark') ? '🌙' : '☀️';
    }

    function toggleTheme() {
      var curTheme = document.documentElement.getAttribute('data-theme');
      var newTheme = (curTheme === 'dark') ? 'light' : 'dark';
      document.documentElement.setAttribute('data-theme', newTheme);
      localStorage.setItem('canteen_theme', newTheme);
      document.getElementById('themeBtn').innerText = (newTheme === 'dark') ? '🌙' : '☀️';
      drawAnalyticsChart();
    }

    function toggleLanguage() {
      currentLang = (currentLang === 'th') ? 'en' : 'th';
      document.getElementById('langSwitch').innerText = (currentLang === 'th') ? 'EN' : 'TH';
      document.querySelectorAll('[data-th]').forEach(el => {
        el.innerText = el.getAttribute('data-' + currentLang);
      });
      var sInput = document.getElementById('search');
      if (sInput) {
        sInput.placeholder = (currentLang === 'th') ? 'ค้นหารหัสนิสิต, ชื่อ-สกุล หรือเลขประจำตัว...' : 'Search Beneficiary ID, Name, or Card UID...';
      }
      drawAnalyticsChart();
      loadStudents(curPage);
    }

    function toggleNav() {
      var m = document.getElementById('navMenu');
      if (m) m.classList.toggle('open');
    }

    setInterval(() => {
      fetch('/api/system/health')
        .then(res => res.json())
        .then(data => {
          var tEl = document.getElementById('telemetryTemp');
          var cEl = document.getElementById('telemetryCpu');
          if (tEl) tEl.innerText = data.temp + ' °C';
          if (cEl) cEl.innerText = data.cpu + ' %';
        })
        .catch(err => {});
    }, 3000);

    function syncDeviceTime() {
      var now = new Date();
      var pad = function(n) { return n < 10 ? '0' + n : n; };
      var d = now.getFullYear() + '-' + pad(now.getMonth() + 1) + '-' + pad(now.getDate());
      var t = pad(now.getHours()) + ':' + pad(now.getMinutes()) + ':' + pad(now.getSeconds());

      var form = document.createElement('form');
      form.method = 'POST';
      form.action = '/api/rtc/set';

      var inputDate = document.createElement('input');
      inputDate.type = 'hidden';
      inputDate.name = 'date';
      inputDate.value = d;
      form.appendChild(inputDate);

      var inputTime = document.createElement('input');
      inputTime.type = 'hidden';
      inputTime.name = 'time';
      inputTime.value = t;
      form.appendChild(inputTime);

      document.body.appendChild(form);
      form.submit();
    }

    function drawAnalyticsChart() {
      var canvas = document.getElementById('analyticsChart');
      if (!canvas) return;
      var ctx = canvas.getContext('2d');
      var isDark = document.documentElement.getAttribute('data-theme') === 'dark';
      var data = [)rawliteral" + String(shopCounts[0]) + "," + String(shopCounts[1]) + "," + String(shopCounts[2]) + "," + String(shopCounts[3]) + R"rawliteral(];
      var labels = (currentLang === 'th') ? ['ร้านที่ 1', 'ร้านที่ 2', 'ร้านที่ 3', 'ร้านที่ 4'] : ['Shop 01', 'Shop 02', 'Shop 03', 'Shop 04'];
      var colors = ['#10b981', '#06b6d4', '#f59e0b', '#f43f5e'];
      var maxVal = Math.max(...data, 10);

      ctx.clearRect(0, 0, canvas.width, canvas.height);
      var chartH = 150, startY = 180, barW = 75, gap = 65, startX = 100;
      ctx.strokeStyle = isDark ? 'rgba(255,255,255,0.08)' : 'rgba(0,0,0,0.08)';
      ctx.lineWidth = 1.5;
      ctx.beginPath(); ctx.moveTo(60, startY); ctx.lineTo(640, startY); ctx.stroke();

      for (var i = 0; i < data.length; i++) {
        var h = (data[i] / maxVal) * chartH;
        var x = startX + (i * (barW + gap));
        var y = startY - h;
        
        ctx.fillStyle = colors[i];
        ctx.beginPath();
        ctx.roundRect(x, y, barW, h, [12, 12, 0, 0]);
        ctx.fill();

        ctx.fillStyle = isDark ? '#f8fafc' : '#0f172a';
        ctx.font = 'bold 14px "Plus Jakarta Sans", Sarabun';
        ctx.textAlign = 'center';
        ctx.fillText(data[i] + ' จาน', x + (barW / 2), y - 10);

        ctx.fillStyle = isDark ? '#94a3b8' : '#64748b';
        ctx.font = '500 13px "Plus Jakarta Sans", Sarabun';
        ctx.fillText(labels[i], x + (barW / 2), startY + 24);
      }
    }

    function switchTab(tabId) {
      document.querySelectorAll('.tab-content').forEach(el => el.style.display = 'none');
      document.querySelectorAll('.nav-item, .dropdown-item').forEach(el => el.classList.remove('active'));
      
      var targetTab = document.getElementById('tab-' + tabId);
      if (targetTab) targetTab.style.display = 'block';

      var activeBtn = document.getElementById('btn-' + tabId);
      if (activeBtn) {
        activeBtn.classList.add('active');
        var parentDropdown = activeBtn.closest('.dropdown');
        if (parentDropdown) {
          var trigger = parentDropdown.querySelector('.dropdown-btn');
          if (trigger) trigger.classList.add('active');
        }
      }
      var m = document.getElementById('navMenu');
      if (m) m.classList.remove('open');
      if (tabId === 'dashboard') drawAnalyticsChart();
      else if (tabId === 'students') loadStudents(curPage);
    }

    function loadStudents(page) {
      if (page < 1) page = 1;
      curPage = page;
      var tbody = document.getElementById('studentTableBody');
      tbody.innerHTML = '<tr><td colspan="7" style="padding: 2rem; text-align: center; color: var(--text-muted);">Loading...</td></tr>';
      fetch('/api/students?page=' + curPage + '&limit=' + pageSize + '&search=' + encodeURIComponent(searchQuery))
        .then(res => res.json())
        .then(data => {
          totalPages = data.totalPages; curPage = data.currentPage;
          var pInd = document.getElementById('pageIndicator');
          if (pInd) pInd.innerText = curPage;

          var pInfo = document.getElementById('paginationInfo');
          if (pInfo) {
            var start = (data.totalItems === 0) ? 0 : (curPage - 1) * pageSize + 1;
            var end = Math.min(curPage * pageSize, data.totalItems);
            pInfo.innerText = (currentLang === 'th') 
              ? ('แสดง ' + start + ' - ' + end + ' จาก ' + data.totalItems + ' รายการ') 
              : ('Showing ' + start + ' - ' + end + ' of ' + data.totalItems + ' records');
          }

          tbody.innerHTML = '';
          if (data.students.length === 0) {
            tbody.innerHTML = '<tr><td colspan="7" style="padding: 2rem; text-align: center; color: var(--text-muted);">' + (currentLang==='th'?'ไม่พบข้อมูล':'No records found') + '</td></tr>';
            return;
          }
          data.students.forEach(s => {
            var tr = document.createElement('tr');
            var refHtml = s.ref + (s.isTemp ? " <span style='background:rgba(245,158,11,0.2); color:var(--accent-yellow); font-size:0.68rem; padding:0.15rem 0.4rem; border-radius:9999px; font-weight:700;'>Temp</span>" : "");
            var statusBadge = s.claimed 
              ? "<span style='background:rgba(16,185,129,0.2); color:var(--accent-green); font-size:0.8rem; padding:0.35rem 0.75rem; border-radius:0.75rem; font-weight:700;'>" + s.time + "</span>"
              : "<span style='background:rgba(244,63,94,0.15); color:var(--accent-rose); font-size:0.8rem; padding:0.35rem 0.75rem; border-radius:0.75rem; font-weight:700;'>READY</span>";
            var actions = '';
            if (!s.claimed) {
              actions += "<button onclick='assignTemp(\"" + s.id + "\")' class='btn btn-slate' style='padding:0.3rem 0.6rem; font-size:0.75rem;'>Temp</button> ";
              actions += "<button onclick='manualClaim(\"" + s.id + "\")' class='btn btn-slate' style='padding:0.3rem 0.6rem; font-size:0.75rem;'>ตัดสิทธิ์</button> ";
            }
            actions += "<button onclick='openEditModal(\"" + s.id + "\",\"" + s.name + "\",\"" + s.uid + "\")' class='btn btn-slate' style='padding:0.3rem 0.6rem; font-size:0.75rem;'>✏️</button> ";
            actions += "<button onclick='deleteStudent(\"" + s.id + "\")' class='btn btn-slate' style='padding:0.3rem 0.6rem; font-size:0.75rem; color:var(--accent-rose);'>🗑️</button>";
            tr.innerHTML = "<td><b>" + s.id + "</b></td><td>" + s.name + "</td><td><code>" + s.uid + "</code></td><td><code>" + refHtml + "</code></td><td>" + statusBadge + "</td><td>" + s.shopName + "</td><td style='text-align: right;'>" + actions + "</td>";
            tbody.appendChild(tr);
          });
        });
    }

    function changePageSize(val) { pageSize = parseInt(val); curPage = 1; loadStudents(1); }
    function prevPage() { if (curPage > 1) loadStudents(curPage - 1); }
    function nextPage() { if (curPage < totalPages) loadStudents(curPage + 1); }
    function goToPage(p) { loadStudents(p); }
    function goToLastPage() { loadStudents(totalPages); }
    function handleSearch(val) { clearTimeout(searchTimer); searchTimer = setTimeout(() => { searchQuery = val.trim(); curPage = 1; loadStudents(1); }, 350); }

    function openAddModal() {
      document.getElementById('modalOldId').value = ''; document.getElementById('modalId').value = '';
      document.getElementById('modalName').value = ''; document.getElementById('modalUid').value = '';
      document.getElementById('studentModal').classList.add('active');
    }
    function openEditModal(id, name, uid) {
      document.getElementById('modalOldId').value = id; document.getElementById('modalId').value = id;
      document.getElementById('modalName').value = name; document.getElementById('modalUid').value = uid;
      document.getElementById('studentModal').classList.add('active');
    }
    function closeModal() { document.getElementById('studentModal').classList.remove('active'); }
    function openTempModal() { document.getElementById('tempCardModal').classList.add('active'); }
    function closeTempModal() { document.getElementById('tempCardModal').classList.remove('active'); }
    function assignTemp(id) { var uid = prompt('กรอก UID บัตรสำรองให้นิสิต ' + id + ':'); if (uid) window.location.href = '/bind-temp?id=' + id + '&uid=' + encodeURIComponent(uid); }
    function removeTempCard(id) { if (confirm('ต้องการยกเลิกบัตรสำรองของนิสิต ' + id + '?')) window.location.href = '/api/tempcard/remove?id=' + id; }
    function manualClaim(id) { var st = prompt('ระบุหมายเลขจุดบริการ (1-4):', '1'); if (st) window.location.href = '/manual-claim?id=' + id + '&station=' + st; }
    function deleteStudent(id) { if (confirm('ต้องการลบรายชื่อนิสิต ' + id + '?')) window.location.href = '/api/student/delete?id=' + id; }

    function openAddAdminModal() {
      document.getElementById('adminOldUser').value = '';
      document.getElementById('adminUsername').value = '';
      document.getElementById('adminDisplayName').value = '';
      document.getElementById('adminPassword').value = '';
      document.getElementById('adminModalTitle').innerText = 'Add New Officer (Max 3)';
      document.getElementById('adminModal').classList.add('active');
    }
    function openEditAdminModal(user, name) {
      document.getElementById('adminOldUser').value = user;
      document.getElementById('adminUsername').value = user;
      document.getElementById('adminDisplayName').value = name;
      document.getElementById('adminPassword').value = '';
      document.getElementById('adminModalTitle').innerText = 'Edit Officer (' + user + ')';
      document.getElementById('adminModal').classList.add('active');
    }
    function closeAdminModal() { document.getElementById('adminModal').classList.remove('active'); }

    initTheme();
    drawAnalyticsChart();
  </script>
</body>
</html>)rawliteral";
  return html;
}
