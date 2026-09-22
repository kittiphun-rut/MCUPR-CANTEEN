/**
 * ============================================================================
 * WebPortal.h — หน้าเว็บทั้งหมดของเครื่องแม่ข่าย (Bento Portal)
 *
 * ไฟล์นี้เก็บเฉพาะ "ตัวสร้างหน้าเว็บ" คือ HTML, CSS และ JavaScript ที่ส่งให้
 * เบราว์เซอร์ของเจ้าหน้าที่ แยกออกมาจาก Canteen_Host_Server.ino เพื่อให้ไฟล์
 * หลักเหลือเฉพาะตรรกะของระบบ อ่านและแก้ไขได้ง่ายขึ้น
 *
 * วิธีใช้: Arduino IDE จะแสดงไฟล์นี้เป็นอีกแท็บของสเก็ตช์เดียวกันโดยอัตโนมัติ
 * ขอเพียงวางไว้ในโฟลเดอร์ Canteen_Host_Server/ ข้าง ๆ ไฟล์ .ino
 * การคอมไพล์และการอัปโหลดยังทำเหมือนเดิมทุกอย่าง
 *
 * ข้อควรระวัง: ไฟล์นี้ถูก #include ไว้ "ท้าย" ไฟล์หลัก เพราะโค้ดข้างในอ้างถึง
 * ตัวแปรส่วนกลาง (db, shops, adminUsers, rtc, ช่วงเวลาให้บริการ) และฟังก์ชัน
 * ช่วยเหลือ (htmlEscape, getRealTimeStr, readHostBatteryVoltage และอื่น ๆ)
 * ที่ประกาศไว้ในไฟล์หลัก — อย่าย้าย #include ขึ้นไปไว้บนสุด
 *
 * สิ่งที่อยู่ในไฟล์นี้
 *   getLoginHTML()  หน้าเข้าสู่ระบบสำหรับเจ้าหน้าที่
 *   getHTML()       หน้าพอร์ทัลหลัก (แดชบอร์ด ทะเบียนนิสิต บัตรสำรอง
 *                   คลังรายงาน เจ้าหน้าที่ ตั้งค่า นำเข้า CSV และร้านค้า)
 *
 * ส่วนที่ "ไม่ได้" อยู่ในไฟล์นี้: เส้นทาง API และตัวจัดการคำขอทั้งหมด
 * ยังอยู่ในไฟล์หลัก เพราะทำงานกับข้อมูลโดยตรงไม่ใช่การสร้างหน้าเว็บ
 * ============================================================================
 */

#pragma once

// ============================================================================
// หน้าเข้าสู่ระบบ
// ============================================================================
String getLoginHTML(const String &errorMsg) {
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

  if (errorMsg.length() > 0) {
    html += "<div class='error-box'>⚠️ " + htmlEscape(errorMsg) + "</div>";
  }

  if (publicDisplayEnabled) {
    html += R"rawliteral(<div style="background: rgba(6,182,212,0.12); border: 1px solid rgba(6,182,212,0.4); border-radius: 1rem; padding: 0.85rem 1rem; margin-bottom: 1.25rem; font-size: 0.85rem; color: #a5f3fc; text-align: left;">
      ไม่ใช่เจ้าหน้าที่ใช่ไหม? ดูยอดการให้บริการแบบเรียลไทม์ได้ที่
      <a href="/display" style="color:#67e8f9; font-weight:700;">หน้าสถานะโรงอาหาร</a>
    </div>)rawliteral";
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

// ============================================================================
// BENTO WEB PORTAL HTML — หน้าพอร์ทัลหลัก
// ============================================================================
String getHTML() {
  int usedCount = 0;
  int shopCounts[4] = {0, 0, 0, 0};
  (void)shopCounts;   // ตัวเลขรายร้านถูกดึงสดผ่าน /api/dashboard แทนการฝังลง HTML
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
  if (root) {
    File file = root.openNextFile();
    while (file) {
      String fn = String(file.name());
      if (fn.startsWith("arc_") || fn.startsWith("/arc_")) {
        String cleanName = fn.startsWith("/") ? fn.substring(1) : fn;
        String safeName = htmlEscape(cleanName);
        archivesHtml += "<tr><td><b>" + safeName + "</b></td><td>" + String(file.size() / 1024.0, 1) + " KB</td>";
        archivesHtml += "<td style='text-align:right;'>";
        archivesHtml += "<a href='/api/archive/download?file=" + safeName + "' class='btn btn-emerald' style='padding:0.35rem 0.75rem; font-size:0.8rem;'>📥 Download</a> ";
        // ใช้ data-attribute แทน onclick ที่ฝังชื่อไฟล์ลงในสตริง JavaScript
        archivesHtml += "<button type='button' class='btn btn-rose js-archive-delete' data-file='" + safeName + "' style='padding:0.35rem 0.75rem; font-size:0.8rem;'>🗑️ Delete</button>";
        archivesHtml += "</td></tr>";
      }
      file = root.openNextFile();
    }
    root.close();
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

  String html;
  // จองหน่วยความจำล่วงหน้าครั้งเดียว แทนการปล่อยให้ += ขยายบัฟเฟอร์นับสิบครั้ง
  // ซึ่งทำให้ heap แตกเป็นเสี่ยงบนอุปกรณ์ที่รันยาว ๆ (บอร์ดที่เปิด OPI PSRAM
  // จะได้บล็อกนี้จาก PSRAM ถ้าจองไม่สำเร็จก็ยังทำงานได้ตามปกติ)
  html.reserve(100 * 1024);
  html += R"rawliteral(<!DOCTYPE html>
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

    /* --- Toast notifications (แทน alert() ที่ทำให้ต้องรีโหลดทั้งหน้า) --- */
    .toast-stack { position: fixed; right: 1.25rem; bottom: 1.25rem; z-index: 400; display: flex; flex-direction: column; gap: 0.6rem; max-width: min(26rem, calc(100vw - 2.5rem)); }
    .toast { display: flex; align-items: flex-start; gap: 0.65rem; background: var(--bg-surface-elevated); border: 1px solid var(--border-card); border-left: 4px solid var(--accent-indigo); border-radius: 1rem; padding: 0.9rem 1.1rem; box-shadow: var(--shadow-card); font-size: 0.9rem; font-weight: 600; color: var(--text-main); animation: toast-in 0.22s ease-out; }
    .toast.ok { border-left-color: var(--accent-green); }
    .toast.err { border-left-color: var(--accent-rose); }
    .toast.leaving { opacity: 0; transform: translateY(0.5rem); transition: opacity 0.25s, transform 0.25s; }
    @keyframes toast-in { from { opacity: 0; transform: translateY(0.75rem); } to { opacity: 1; transform: none; } }

    /* --- Skeleton loader ของตาราง --- */
    .skeleton { display: block; height: 0.85rem; border-radius: 0.5rem; background: linear-gradient(90deg, var(--bg-surface-elevated) 25%, var(--border-card) 50%, var(--bg-surface-elevated) 75%); background-size: 200% 100%; animation: skeleton-shift 1.2s infinite; }
    @keyframes skeleton-shift { from { background-position: 200% 0; } to { background-position: -200% 0; } }

    thead th { position: sticky; top: 0; z-index: 2; }
    tbody tr:hover td { background: var(--bg-surface-elevated); }
    .table-container { max-height: 68vh; overflow-y: auto; }
    .live-dot { display: inline-block; width: 0.5rem; height: 0.5rem; border-radius: 50%; background: var(--accent-green); margin-right: 0.35rem; animation: pulse-dot 1.6s infinite; }
    @keyframes pulse-dot { 0%,100% { opacity: 1; } 50% { opacity: 0.25; } }
    .station-card { background: var(--bg-surface-elevated); border: 1px solid var(--border-card); border-radius: 1.5rem; padding: 1.25rem; }
    .station-card.offline { border-color: rgba(244,63,94,0.35); }
    .sig-bars { display: inline-flex; align-items: flex-end; gap: 2px; height: 0.85rem; }
    .sig-bars i { width: 3px; background: var(--border-card); border-radius: 1px; display: block; }
    .sig-bars i.on { background: currentColor; }
    [title] { cursor: help; }
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
              <div class="gauge-number" id="statUsed">)rawliteral" + String(usedCount) + R"rawliteral(</div>
              <div class="gauge-label" data-th="ใช้สิทธิ์แล้ว (คน)" data-en="CLAIMED STUDENTS">ใช้สิทธิ์แล้ว (คน)</div>
            </div>
          </div>

          <div style="display: flex; justify-content: space-between; border-top: 1px solid var(--border-card); padding-top: 1rem; font-size: 0.85rem;">
            <div>
              <span style="color: var(--text-muted);" data-th="คงเหลือ:" data-en="Remaining:">คงเหลือ:</span>
              <b style="color: var(--text-main);" id="statRemaining">)rawliteral" + String((int)db.size() - usedCount) + R"rawliteral(</b>
            </div>
            <div>
              <span style="color: var(--text-muted);" data-th="ยอดจัดสรร:" data-en="Disbursed:">ยอดจัดสรร:</span>
              <b style="color: var(--accent-green);"><span id="statDisbursed">)rawliteral" + String(usedCount * 35) + R"rawliteral(</span> THB</b>
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
            <span>🔋 Batt: <b style="color:var(--text-main);" id="statBatt">)rawliteral" + String(hostBattPct) + R"rawliteral(%</b></span>
            <span>💾 Heap: <b style="color:var(--text-main);" id="statHeap">)rawliteral" + String((unsigned)(ESP.getFreeHeap() / 1024)) + R"rawliteral( KB</b></span>
          </div>
        </div>

        <!-- Bento 3: Standard Clock & Quick Actions -->
        <div class="bento-card col-span-4" style="display: flex; flex-direction: column; justify-content: space-between; background: linear-gradient(135deg, var(--bg-surface), var(--bg-surface-elevated));">
          <div>
            <div style="font-size: 0.85rem; font-weight: 700; color: var(--accent-yellow); text-transform: uppercase;" data-th="เวลามาตรฐานระบบ" data-en="Standard RTC Time">เวลามาตรฐานระบบ</div>
            <div id="clockText" style="font-size: 1.8rem; font-weight: 800; margin: 0.5rem 0; letter-spacing: -0.02em;">)rawliteral" + getRealTimeStr() + R"rawliteral(</div>
            <div style="font-size: 0.82rem; color: var(--text-muted);" data-th="ปรับเทียบผ่านชิป DS3231 Precision I2C" data-en="Synced with DS3231 Precision I2C">ปรับเทียบผ่านชิป DS3231 Precision I2C</div>
            <div id="serviceBadge" style="margin-top: 0.75rem; display: inline-block; font-size: 0.75rem; font-weight: 800; padding: 0.25rem 0.7rem; border-radius: 9999px; background: var(--accent-green-glow); color: var(--accent-green);">—</div>
          </div>

          <div style="display: flex; flex-direction: column; gap: 0.5rem; margin-top: 1rem;">
            <a href="/export.csv" class="btn btn-emerald" style="width: 100%;" data-th="📥 ดาวน์โหลดรายงานสรุป (CSV)" data-en="📥 Export Summary (CSV)">📥 ดาวน์โหลดรายงานสรุป (CSV)</a>
            <button onclick="printDaily()" class="btn btn-indigo" style="width: 100%;" title="พิมพ์ใบสรุปยอดประจำวันออกเครื่องพิมพ์ความร้อน 58 มม." data-th="🖨️ พิมพ์ใบสรุปประจำวัน" data-en="🖨️ Print Daily Summary">🖨️ พิมพ์ใบสรุปประจำวัน</button>
            <button onclick="syncDeviceTime()" class="btn btn-slate" style="width: 100%;" data-th="⚡ ซิงค์เวลากับเครื่องนี้" data-en="⚡ Sync Device Time">⚡ ซิงค์เวลากับเครื่องนี้</button>
            <div id="printerBadge" style="font-size: 0.72rem; font-weight: 700; text-align: center; padding: 0.2rem 0; color: var(--text-muted);">🖨️ —</div>
          </div>
        </div>

        <!-- Bento 4: 4 Stations Live Status -->
        <div class="bento-card col-span-12">
          <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 1.25rem;">
            <h3 style="font-size: 1.1rem; font-weight: 800;" data-th="📡 จุดให้บริการร้านค้าประจำโรงอาหาร (4 Stations Live Status)" data-en="📡 Canteen Vendor Stations (4 Stations Live Status)">📡 จุดให้บริการร้านค้าประจำโรงอาหาร (4 Stations Live Status)</h3>
            <span style="font-size: 0.8rem; color: var(--text-muted);">ESP-NOW Channel 1</span>
          </div>

          <div class="bento-grid" id="stationGrid" style="margin-bottom: 0;">
            <div class="col-span-6 station-card"><span class="skeleton" style="width: 60%;"></span></div>
            <div class="col-span-6 station-card"><span class="skeleton" style="width: 60%;"></span></div>
            <div class="col-span-6 station-card"><span class="skeleton" style="width: 60%;"></span></div>
            <div class="col-span-6 station-card"><span class="skeleton" style="width: 60%;"></span></div>
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
              <button type="button" onclick="dailyReset()" class="btn btn-rose" title="จัดเก็บบันทึกของวันนี้เข้าคลังประวัติ แล้วคืนสิทธิ์ให้นิสิตทุกคน" data-th="🔄 ปิดยอดประจำวัน & จัดเก็บประวัติ" data-en="🔄 Daily Reset &amp; Archive">🔄 ปิดยอดประจำวัน & จัดเก็บประวัติ</button>
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
        actionBtn = "<button type='button' class='btn btn-slate js-temp-revoke' data-id='" + htmlEscape(db[i].studentId) + "' style='color:var(--accent-rose); padding:0.35rem 0.65rem; font-size:0.75rem;'>Revoke</button>";
      }

      html += "<tr><td><b>" + htmlEscape(db[i].studentId) + "</b></td><td>" + htmlEscape(db[i].fullName) + "</td><td><code>" + htmlEscape(db[i].uid) + "</code></td>";
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
    String safeUser = htmlEscape(adminUsers[i].username);
    String safeName = htmlEscape(adminUsers[i].displayName);
    html += "<td style='font-weight:700;'>" + safeName + "</td>";
    html += "<td><code>" + safeUser + "</code></td>";
    html += "<td style='color:var(--text-muted);'>••••••••</td>";
    html += "<td style='text-align:right;'>";
    html += "<button type=\"button\" class=\"btn btn-slate js-admin-edit\" data-user=\"" + safeUser + "\" data-name=\"" + safeName + "\" style=\"padding:0.35rem 0.75rem; font-size:0.8rem;\">✏️ Edit</button> ";
    if (adminUsers.size() > 1) {
      html += "<button type=\"button\" class=\"btn btn-rose js-admin-delete\" data-user=\"" + safeUser + "\" style=\"padding:0.35rem 0.75rem; font-size:0.8rem;\">🗑️ Delete</button>";
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
          <form data-ajax="1" method="POST" action="/api/rtc/set" style="margin-top: 1rem;">
            <label style="font-size: 0.85rem; font-weight: 700;">กำหนดวันที่:</label>
            <input type="date" name="date" value=")rawliteral" + String(dateInputBuf) + R"rawliteral(" required class="form-input">
            <label style="font-size: 0.85rem; font-weight: 700;">กำหนดเวลา:</label>
            <input type="text" name="time" value=")rawliteral" + String(timeInputBuf) + R"rawliteral(" required class="form-input" placeholder="HH:MM:SS">
            <button type="submit" class="btn btn-indigo" style="width: 100%;">💾 บันทึกเวลามาตรฐาน</button>
          </form>
        </div>

        <div class="col-span-6 bento-card">
          <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.5rem;" data-th="⚙️ กำหนดช่วงเวลาเปิดให้บริการอาหาร" data-en="⚙️ Service Hours Window">⚙️ กำหนดช่วงเวลาเปิดให้บริการอาหาร</h2>
          <form data-ajax="1" method="POST" action="/api/settings/time" style="margin-top: 1rem;">
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

        <div class="col-span-12 bento-card">
          <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.5rem;" data-th="📺 หน้าจอสาธารณะสำหรับมอนิเตอร์จอใหญ่" data-en="📺 Public Display Screen">📺 หน้าจอสาธารณะสำหรับมอนิเตอร์จอใหญ่</h2>
          <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1rem;">
            หน้า <code>/display</code> ออกแบบไว้ต่อกับมอนิเตอร์จอใหญ่ให้นิสิตและร้านค้าดูยอดการใช้บริการแบบเรียลไทม์
            เปิดดูได้โดยไม่ต้องเข้าสู่ระบบ จึงไม่กินช่องผู้ใช้งานของเจ้าหน้าที่
            หน้านี้ <b>ไม่แสดงชื่อนิสิต ไม่แสดงเลขบัตร และไม่มีปุ่มสั่งงานใด ๆ</b>
            รหัสนิสิตในรายการล่าสุดถูกปิดบังเหลือห้าหลักแรก และแสดงเฉพาะรายการที่ตัดสิทธิ์สำเร็จเท่านั้น
          </p>
          <form method="POST" action="/api/settings/display" data-ajax="1" style="display:flex; gap:1rem; align-items:flex-end; flex-wrap:wrap;">
            <div style="flex:1; min-width:16rem;">
              <label style="font-size: 0.85rem; font-weight: 700;">สถานะหน้าจอสาธารณะ:</label>
              <select name="enabled" class="form-input" style="margin-bottom:0;">
                <option value="1" )rawliteral" + String(publicDisplayEnabled ? "selected" : "") + R"rawliteral(>เปิดใช้งาน (ทุกคนในเครือข่ายเปิดดูได้)</option>
                <option value="0" )rawliteral" + String(!publicDisplayEnabled ? "selected" : "") + R"rawliteral(>ปิดใช้งาน (ต้องเข้าสู่ระบบก่อนจึงจะเปิดได้)</option>
              </select>
            </div>
            <button type="submit" class="btn btn-indigo">💾 บันทึก</button>
            <a href="/display" target="_blank" rel="noopener" class="btn btn-emerald">📺 เปิดหน้าจอสาธารณะ</a>
          </form>
        </div>

        <div class="col-span-12 bento-card">
          <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.5rem;" data-th="🖨️ เครื่องพิมพ์สลิปความร้อน 58 มม." data-en="🖨️ 58 mm Thermal Slip Printer">🖨️ เครื่องพิมพ์สลิปความร้อน 58 มม.</h2>
          <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1rem;">
            เมื่อเปิดใช้งาน ระบบจะพิมพ์สลิปให้อัตโนมัติ <b>ทุกครั้ง</b> ที่ตัดสิทธิ์สำเร็จ ทั้งจากการแตะบัตรที่จุดบริการ
            และจากการกดตัดสิทธิ์ด้วยตนเองบนหน้าเว็บ หรือจะสั่งพิมพ์ย้อนหลังทีละรายการจากปุ่ม 🖨️ ในตารางรายชื่อก็ได้
            <br>
            เนื้อหาบนสลิปเป็น <b>ภาษาอังกฤษและตัวเลขล้วน</b> เพราะหัวพิมพ์ราคาประหยัดไม่มีฟอนต์ไทยในตัว
            ชื่อนิสิตที่เป็นภาษาไทยจะถูกแทนด้วย <code>STUDENT &lt;รหัสนิสิต&gt;</code> โดยอัตโนมัติ
            ส่วนชื่อร้านที่เป็นภาษาไทยจะเหลือแค่หมายเลขร้าน — ถ้าต้องการให้ชื่อร้านขึ้นบนสลิปด้วย
            ให้ตั้งชื่อร้านเป็นภาษาอังกฤษในแท็บ 🏪 ร้านค้า
            <br>
            รายการที่ซิงค์ย้อนหลังหลังลิงก์ขาด <b>จะไม่พิมพ์สลิปอัตโนมัติ</b> เพราะนิสิตรับอาหารและกลับไปแล้ว
            ยอดยังอยู่ครบในใบสรุปประจำวันและไฟล์ CSV
          </p>
          <div style="display:flex; gap:1rem; align-items:flex-end; flex-wrap:wrap;">
            <form method="POST" action="/api/settings/printer" data-ajax="1" style="display:flex; gap:1rem; align-items:flex-end; flex-wrap:wrap; flex:1; min-width:20rem; margin:0;">
              <div style="flex:1; min-width:16rem;">
                <label style="font-size: 0.85rem; font-weight: 700;">การพิมพ์สลิปอัตโนมัติ:</label>
                <select name="auto" class="form-input" style="margin-bottom:0;">
                  <option value="1" )rawliteral" + String(printerAutoSlip ? "selected" : "") + R"rawliteral(>เปิด (พิมพ์ทุกครั้งที่ตัดสิทธิ์สำเร็จ)</option>
                  <option value="0" )rawliteral" + String(!printerAutoSlip ? "selected" : "") + R"rawliteral(>ปิด (สั่งพิมพ์เองจากปุ่มเท่านั้น)</option>
                </select>
              </div>
              <button type="submit" class="btn btn-indigo">💾 บันทึก</button>
            </form>
            <button type="button" onclick="printTest()" class="btn btn-emerald">🧾 พิมพ์สลิปทดสอบ</button>
          </div>
          <div style="margin-top:1rem; padding:0.75rem 1rem; border:1px solid var(--border-card); border-radius:1rem; background: var(--bg-surface-elevated); font-size:0.82rem;">
            <span style="color: var(--text-muted);">สถานะการเชื่อมต่อ:</span>
            <b id="printerStatusText">)rawliteral" + htmlEscape(printerStatusText()) + R"rawliteral(</b>
            <span style="color: var(--text-muted);"> · งานค้างในคิว </span><b id="printerQueueText">0</b>
            <br>
            <span style="color: var(--text-muted);">
              ช่องทางที่คอมไพล์ไว้: )rawliteral" + String(PRINTER_TRANSPORT_UART ? "UART TTL (GPIO 17 = TX, GPIO 18 = RX, ร่วม GND)" : "USB OTG host (ต้องจ่ายไฟ 5V เข้าขา VBUS เอง และตั้ง USB Mode เป็น Hardware CDC and JTAG)") + R"rawliteral(
              — สลับช่องทางได้ด้วยการแก้ <code>#define PRINTER_TRANSPORT_UART</code> ที่หัวสเก็ตช์แล้วอัปโหลดใหม่
            </span>
          </div>
        </div>
      </div>
    </div>

    <!-- TAB 7: IMPORT -->
    <div id="tab-import" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12" style="max-width: 36rem; margin: auto; text-align: center;">
        <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.5rem;">📥 นำเข้าบัญชีรายชื่อนิสิต (Smart Upsert)</h2>
        <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1.5rem;">* ระบบจะไม่ลบรายชื่อเก่า: รหัสเดิมจะถูกอัปเดตข้อมูล และรหัสใหม่จะถูกเพิ่มเข้าสู่ฐานข้อมูลอัตโนมัติ</p>
        <form data-ajax="1" method="POST" action="/upload" enctype="multipart/form-data" style="border: 2px dashed var(--border-card); border-radius: 1.5rem; padding: 2rem;">
          <input type="file" name="csv" accept=".csv" required style="margin-bottom: 1.25rem;"><br>
          <button type="submit" class="btn btn-emerald">🚀 อัปโหลดและผสานข้อมูล</button>
        </form>
      </div>
    </div>

    <!-- TAB 8: SHOPS -->
    <div id="tab-shops" class="tab-content" style="display: none;">
      <div class="bento-card col-span-12" style="max-width: 38rem; margin: auto;">
        <h2 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 1rem;">🏪 จัดการข้อมูลร้านค้าและผู้ประกอบการ</h2>
        <form method="POST" action="/api/shops/save" data-ajax="1">
  )rawliteral";

  for (int i = 0; i < 4; i++) {
    html += "<div style='background: var(--bg-surface-elevated); border: 1px solid var(--border-card); border-radius: 1.25rem; padding: 1.25rem; margin-bottom: 1rem;'>";
    html += "<h4 style='color: var(--accent-indigo); margin-bottom: 0.5rem; font-weight: 800;'>Point " + String(i + 1) + "</h4>";
    html += "<input type='text' name='sname" + String(i) + "' value='" + htmlEscape(shops[i].name) + "' required maxlength='48' class='form-input'>";
    html += "<input type='text' name='vname" + String(i) + "' value='" + htmlEscape(shops[i].vendor) + "' required maxlength='64' class='form-input' style='margin-bottom:0;'></div>";
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
        <form data-ajax="1" method="POST" action="/api/student/save">
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
        <form data-ajax="1" method="POST" action="/api/tempcard/save">
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
        <form data-ajax="1" method="POST" action="/api/admin/save">
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

  <div class="toast-stack" id="toastStack" aria-live="polite"></div>

  <!-- Modal: ตัดสิทธิ์ด้วยตนเอง (แทน prompt() เดิม) -->
  <div id="manualClaimModal" class="modal">
    <div class="bento-card" style="max-width: 26rem; width: 100%;">
      <h3 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.35rem;">ตัดสิทธิ์ด้วยตนเอง</h3>
      <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1rem;">ใช้กรณีนิสิตลืมบัตรหรือเครื่องอ่านขัดข้อง ระบบจะบันทึกลงประวัติเหมือนการแตะบัตรปกติ</p>
      <div style="font-size: 0.85rem; font-weight: 700; margin-bottom: 0.5rem;">รหัสนิสิต: <span id="manualClaimId" style="color: var(--accent-indigo);"></span></div>
      <label style="font-size: 0.85rem; font-weight: 700;">เลือกจุดบริการ:</label>
      <select id="manualClaimStation" class="form-input">
        <option value="1">จุดบริการ 1</option>
        <option value="2">จุดบริการ 2</option>
        <option value="3">จุดบริการ 3</option>
        <option value="4">จุดบริการ 4</option>
      </select>
      <div style="display: flex; justify-content: flex-end; gap: 0.5rem;">
        <button type="button" onclick="closeManualClaim()" class="btn btn-slate">ยกเลิก</button>
        <button type="button" onclick="confirmManualClaim()" class="btn btn-emerald">ยืนยันตัดสิทธิ์</button>
      </div>
    </div>
  </div>

  <!-- Modal: ผูกบัตรสำรองจากตารางรายชื่อ (แทน prompt() เดิม) -->
  <div id="assignTempModal" class="modal">
    <div class="bento-card" style="max-width: 26rem; width: 100%;">
      <h3 style="font-size: 1.2rem; font-weight: 800; margin-bottom: 0.35rem;">ผูกบัตรสำรอง</h3>
      <div style="font-size: 0.85rem; font-weight: 700; margin-bottom: 0.75rem;">รหัสนิสิต: <span id="assignTempId" style="color: var(--accent-indigo);"></span></div>
      <label style="font-size: 0.85rem; font-weight: 700;">เลขบัตรสำรอง (10 หลัก):</label>
      <input type="text" id="assignTempUid" class="form-input" maxlength="15" placeholder="e.g. 0305419896">
      <div style="display: flex; justify-content: flex-end; gap: 0.5rem;">
        <button type="button" onclick="closeAssignTemp()" class="btn btn-slate">ยกเลิก</button>
        <button type="button" onclick="confirmAssignTemp()" class="btn btn-emerald">ผูกบัตร</button>
      </div>
    </div>
  </div>

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
    var dashboardData = null;
    var dashboardTimer = null;

    /* ---------------------------------------------------------------
       Toast: แทน alert() เดิมที่บังคับให้รีโหลดทั้งหน้าและทำให้เสียแท็บที่ค้างอยู่
       --------------------------------------------------------------- */
    function toast(message, ok) {
      var stack = document.getElementById('toastStack');
      if (!stack) return;
      var el = document.createElement('div');
      el.className = 'toast ' + (ok === false ? 'err' : 'ok');
      var icon = document.createElement('span');
      icon.textContent = (ok === false) ? '⚠️' : '✅';
      var text = document.createElement('span');
      text.textContent = message;          /* textContent = ปลอดภัยจาก HTML injection */
      el.appendChild(icon);
      el.appendChild(text);
      stack.appendChild(el);
      setTimeout(function () {
        el.classList.add('leaving');
        setTimeout(function () { if (el.parentNode) el.parentNode.removeChild(el); }, 300);
      }, 4200);
    }

    /* เรียก API แบบ POST พร้อมจัดการข้อผิดพลาดและ session หมดอายุให้ครบทุกทาง */
    function api(url, params) {
      var body = new URLSearchParams();
      if (params) { for (var k in params) { if (params.hasOwnProperty(k)) body.append(k, params[k]); } }
      return fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8' },
        body: body.toString()
      }).then(function (res) {
        if (res.status === 401) { window.location.href = '/login'; throw new Error('unauthorized'); }
        return res.json().catch(function () { return { ok: false, msg: 'เซิร์ฟเวอร์ตอบกลับไม่ถูกต้อง' }; });
      }).then(function (data) {
        toast(data.msg || (data.ok ? 'สำเร็จ' : 'ไม่สำเร็จ'), data.ok);
        return data;
      }).catch(function (err) {
        if (err && err.message === 'unauthorized') throw err;
        toast('ติดต่อเครื่องแม่ข่ายไม่สำเร็จ กรุณาตรวจสอบการเชื่อมต่อ', false);
        throw err;
      });
    }

    /* ส่งฟอร์มทุกใบแบบ AJAX แล้วรีเฟรชเฉพาะส่วนที่เปลี่ยน */
    function bindAjaxForms() {
      document.querySelectorAll('form[data-ajax]').forEach(function (form) {
        form.addEventListener('submit', function (ev) {
          ev.preventDefault();
          var btn = form.querySelector('button[type="submit"]');
          var original = btn ? btn.innerHTML : '';
          if (btn) { btn.disabled = true; btn.innerHTML = '⏳ กำลังบันทึก...'; }

          fetch(form.getAttribute('action'), { method: 'POST', body: new FormData(form) })
            .then(function (res) {
              if (res.status === 401) { window.location.href = '/login'; throw new Error('unauthorized'); }
              return res.json().catch(function () { return { ok: false, msg: 'เซิร์ฟเวอร์ตอบกลับไม่ถูกต้อง' }; });
            })
            .then(function (data) {
              toast(data.msg || (data.ok ? 'บันทึกสำเร็จ' : 'บันทึกไม่สำเร็จ'), data.ok);
              if (!data.ok) return;
              closeModal(); closeTempModal(); closeAdminModal();
              form.querySelectorAll('input[type="file"]').forEach(function (i) { i.value = ''; });
              refreshDashboard();
              if (document.getElementById('tab-students').style.display !== 'none') loadStudents(curPage);
              if (form.getAttribute('action').indexOf('/api/admin/') === 0 ||
                  form.getAttribute('action').indexOf('/api/tempcard/') === 0 ||
                  form.getAttribute('action') === '/upload') {
                setTimeout(function () { window.location.reload(); }, 900);
              }
            })
            .catch(function () { })
            .then(function () { if (btn) { btn.disabled = false; btn.innerHTML = original; } });
        });
      });
    }

    /* ---------------------------------------------------------------
       ธีมและภาษา
       --------------------------------------------------------------- */
    /* หน้าต่าง captive portal ของ iOS/Android และโหมดไม่ระบุตัวตนของเบราว์เซอร์
       จะโยน SecurityError ทันทีที่แตะ localStorage การอ่าน-เขียนทุกจุดจึงต้องหุ้ม
       try/catch ไว้ ไม่เช่นนั้นสคริปต์จะตายตั้งแต่บรรทัดแรกและทั้งหน้าใช้งานไม่ได้ */
    function lsGet(key) { try { return localStorage.getItem(key); } catch (e) { return null; } }
    function lsSet(key, value) { try { localStorage.setItem(key, value); } catch (e) { } }

    function initTheme() {
      var savedTheme = lsGet('canteen_theme') || 'dark';
      document.documentElement.setAttribute('data-theme', savedTheme);
      var btn = document.getElementById('themeBtn');
      if (btn) btn.innerText = (savedTheme === 'dark') ? '🌙' : '☀️';
    }

    function toggleTheme() {
      var curTheme = document.documentElement.getAttribute('data-theme');
      var newTheme = (curTheme === 'dark') ? 'light' : 'dark';
      document.documentElement.setAttribute('data-theme', newTheme);
      lsSet('canteen_theme', newTheme);
      document.getElementById('themeBtn').innerText = (newTheme === 'dark') ? '🌙' : '☀️';
      drawAnalyticsChart();
    }

    function toggleLanguage() {
      currentLang = (currentLang === 'th') ? 'en' : 'th';
      document.getElementById('langSwitch').innerText = (currentLang === 'th') ? 'EN' : 'TH';
      document.querySelectorAll('[data-th]').forEach(function (el) {
        var v = el.getAttribute('data-' + currentLang);
        if (v !== null) el.innerText = v;
      });
      var sInput = document.getElementById('search');
      if (sInput) {
        sInput.placeholder = (currentLang === 'th') ? 'ค้นหารหัสนิสิต, ชื่อ-สกุล หรือเลขประจำตัว...' : 'Search Beneficiary ID, Name, or Card UID...';
      }
      renderStations();
      drawAnalyticsChart();
      loadStudents(curPage);
    }

    function toggleNav() {
      var m = document.getElementById('navMenu');
      if (m) m.classList.toggle('open');
    }

    /* ---------------------------------------------------------------
       แดชบอร์ดสด: ดึงตัวเลขจริงจากเครื่องแม่ข่ายทุก 3 วินาที
       --------------------------------------------------------------- */
    function setText(id, value) {
      var el = document.getElementById(id);
      if (el) el.textContent = value;
    }

    function refreshDashboard() {
      return fetch('/api/dashboard')
        .then(function (res) {
          if (res.status === 401) { window.location.href = '/login'; throw new Error('unauthorized'); }
          return res.json();
        })
        .then(function (d) {
          dashboardData = d;

          setText('telemetryTemp', d.temp.toFixed(1) + ' °C');
          setText('telemetryCpu', d.cpu.toFixed(1) + ' %');
          setText('statBatt', d.battPct + '%');
          setText('statHeap', d.heap + ' KB');
          setText('statUsed', d.used);
          setText('statRemaining', d.remaining);
          setText('statDisbursed', d.disbursed);
          setText('clockText', d.clock);

          var tEl = document.getElementById('telemetryTemp');
          if (tEl) tEl.style.color = (d.temp < 65) ? 'var(--accent-green)' : 'var(--accent-rose)';

          var arc = document.getElementById('gaugeArc');
          if (arc) arc.style.strokeDashoffset = String(252 - Math.round(d.quotaPct * 252 / 100));

          var badge = document.getElementById('serviceBadge');
          if (badge) {
            badge.textContent = (d.serviceOpen ? '🟢 เปิดให้บริการ ' : '🔴 นอกเวลาให้บริการ ') + d.window;
            badge.style.background = d.serviceOpen ? 'var(--accent-green-glow)' : 'rgba(244,63,94,0.15)';
            badge.style.color = d.serviceOpen ? 'var(--accent-green)' : 'var(--accent-rose)';
          }

          var tempBadge = document.getElementById('tempCount');
          if (tempBadge) tempBadge.textContent = d.tempWaiting;

          var prn = d.printer || {};
          var prnBadge = document.getElementById('printerBadge');
          if (prnBadge) {
            if (!prn.enabled) {
              prnBadge.textContent = '🖨️ ปิดการใช้งานในเฟิร์มแวร์';
              prnBadge.style.color = 'var(--text-muted)';
            } else {
              prnBadge.textContent = '🖨️ ' + (prn.ready ? 'พร้อมพิมพ์' : 'ยังไม่พบเครื่องพิมพ์')
                                   + (prn.auto ? ' · อัตโนมัติ' : ' · พิมพ์เอง')
                                   + (prn.queue ? ' · คิว ' + prn.queue : '');
              prnBadge.style.color = prn.ready ? 'var(--accent-green)' : 'var(--accent-yellow)';
            }
          }
          setText('printerStatusText', prn.status || '—');
          setText('printerQueueText', (prn.queue === undefined) ? '0' : prn.queue);

          renderStations();
          drawAnalyticsChart();
          return d;
        })
        .catch(function () { });
    }

    function signalBars(quality, online) {
      var wrap = document.createElement('span');
      wrap.className = 'sig-bars';
      var active = 0;
      if (online) { active = quality >= 85 ? 4 : quality >= 60 ? 3 : quality >= 35 ? 2 : 1; }
      wrap.style.color = !online ? 'var(--accent-rose)'
                       : (quality >= 60 ? 'var(--accent-green)' : quality >= 35 ? 'var(--accent-yellow)' : 'var(--accent-rose)');
      for (var i = 0; i < 4; i++) {
        var bar = document.createElement('i');
        bar.style.height = (4 + i * 3) + 'px';
        if (i < active) bar.className = 'on';
        wrap.appendChild(bar);
      }
      return wrap;
    }

    function metricBlock(label, value, color) {
      var box = document.createElement('div');
      var cap = document.createElement('div');
      cap.style.cssText = 'font-size:0.72rem; color:var(--text-muted); text-transform:uppercase;';
      cap.textContent = label;
      var val = document.createElement('div');
      val.style.cssText = 'font-size:1.2rem; font-weight:800; color:' + color + ';';
      val.textContent = value;
      box.appendChild(cap); box.appendChild(val);
      return box;
    }

    function renderStations() {
      var grid = document.getElementById('stationGrid');
      if (!grid || !dashboardData) return;
      grid.innerHTML = '';

      dashboardData.stations.forEach(function (st, idx) {
        var shop = dashboardData.shops[idx] || { name: 'Station', vendor: '', count: 0, amount: 0 };
        var card = document.createElement('div');
        card.className = 'col-span-6 station-card' + (st.online ? '' : ' offline');

        var head = document.createElement('div');
        head.style.cssText = 'display:flex; justify-content:space-between; align-items:flex-start; gap:0.75rem;';

        var titleWrap = document.createElement('div');
        var title = document.createElement('h4');
        title.style.cssText = 'font-size:1.05rem; font-weight:800;';
        title.textContent = shop.name;
        var vendor = document.createElement('p');
        vendor.style.cssText = 'font-size:0.82rem; color:var(--text-muted); margin-top:0.2rem;';
        vendor.textContent = (currentLang === 'th' ? 'ผู้ประกอบการ: ' : 'Vendor: ') + shop.vendor;
        titleWrap.appendChild(title); titleWrap.appendChild(vendor);

        var status = document.createElement('span');
        status.style.cssText = 'font-size:0.72rem; font-weight:800; padding:0.25rem 0.6rem; border-radius:9999px; white-space:nowrap; ' +
          (st.online ? 'background:var(--accent-green-glow); color:var(--accent-green);'
                     : 'background:rgba(244,63,94,0.15); color:var(--accent-rose);');
        if (st.online) {
          var dot = document.createElement('span');
          dot.className = 'live-dot';
          status.appendChild(dot);
          status.appendChild(document.createTextNode('ONLINE'));
          status.title = 'สัญญาณ ' + st.rssi + ' dBm · ได้ยินล่าสุดเมื่อ ' + st.ageSec + ' วินาทีที่แล้ว';
        } else {
          status.textContent = 'OFFLINE';
          status.title = 'ไม่ได้รับสัญญาณจากจุดบริการนี้เกิน 15 วินาที';
        }

        head.appendChild(titleWrap); head.appendChild(status);

        var body = document.createElement('div');
        body.style.cssText = 'display:flex; justify-content:space-between; align-items:center; gap:0.5rem; margin-top:1.1rem; border-top:1px dashed var(--border-card); padding-top:0.75rem; flex-wrap:wrap;';
        body.appendChild(metricBlock(currentLang === 'th' ? 'จำนวนที่จ่าย' : 'Orders', shop.count + (currentLang === 'th' ? ' จาน' : ' meals'), 'var(--accent-green)'));
        body.appendChild(metricBlock(currentLang === 'th' ? 'ยอดรวม' : 'Amount', shop.amount + ' B.', 'var(--accent-yellow)'));
        body.appendChild(metricBlock(currentLang === 'th' ? 'แบตเตอรี่' : 'Battery', st.online ? (st.battPct + '% / ' + st.volt.toFixed(2) + 'V') : '—', 'var(--text-main)'));

        var sig = document.createElement('div');
        sig.style.cssText = 'display:flex; align-items:center; gap:0.4rem;';
        sig.appendChild(signalBars(st.quality, st.online));
        var sigText = document.createElement('span');
        sigText.style.cssText = 'font-size:0.78rem; color:var(--text-muted);';
        sigText.textContent = st.online ? (st.rssi + ' dBm') : '—';
        sig.appendChild(sigText);
        body.appendChild(sig);

        /* สถานะบัญชีสิทธิ์ที่ผลักไปเก็บไว้ที่จุดบริการ ใช้ตรวจบัตรเองตอนลิงก์ขาด */
        var ros = document.createElement('div');
        ros.style.cssText = 'margin-top:0.7rem; font-size:0.74rem; font-weight:700; display:flex; align-items:center; gap:0.35rem;';
        if (!st.online) {
          ros.textContent = '📋 รายชื่อออฟไลน์: —';
          ros.style.color = 'var(--text-muted)';
          ros.title = 'จุดบริการนี้ออฟไลน์อยู่ จึงยังตรวจสอบไม่ได้';
        } else if (st.rosterTooBig) {
          ros.textContent = '📋 รายชื่อออฟไลน์: เกินที่เครื่องเก็บไหว';
          ros.style.color = 'var(--accent-rose)';
          ros.title = 'จำนวนผู้มีสิทธิ์มากกว่าที่เฟิร์มแวร์จุดบริการเก็บได้ (ROSTER_MAX) '
                    + 'ตอนลิงก์ขาดจุดบริการนี้จะรับบัตรทุกใบเหมือนเดิม';
        } else if (st.rosterOk) {
          ros.textContent = '📋 รายชื่อออฟไลน์: พร้อมใช้ (รุ่น ' + st.rosterVer + ')';
          ros.style.color = 'var(--accent-green)';
          ros.title = 'จุดบริการนี้ตรวจบัตรที่ไม่ได้ลงทะเบียนและบัตรที่ใช้สิทธิ์แล้วได้เองตอนลิงก์ขาด';
        } else {
          ros.textContent = '📋 รายชื่อออฟไลน์: กำลังส่งให้...';
          ros.style.color = 'var(--accent-yellow)';
          ros.title = 'กำลังผลักรายชื่อรุ่นล่าสุดไปให้ ใช้เวลาไม่กี่วินาที '
                    + 'ระหว่างนี้ถ้าลิงก์ขาดจุดบริการจะรับบัตรทุกใบไว้ก่อน';
        }
        card.appendChild(head); card.appendChild(body); card.appendChild(ros);
        grid.appendChild(card);
      });
    }

    function syncDeviceTime() {
      var now = new Date();
      var pad = function (n) { return n < 10 ? '0' + n : n; };
      var d = now.getFullYear() + '-' + pad(now.getMonth() + 1) + '-' + pad(now.getDate());
      var t = pad(now.getHours()) + ':' + pad(now.getMinutes()) + ':' + pad(now.getSeconds());
      api('/api/rtc/set', { date: d, time: t }).then(refreshDashboard).catch(function () { });
    }

    function dailyReset() {
      if (!confirm(currentLang === 'th'
            ? 'ต้องการปิดยอดประจำวัน จัดเก็บประวัติเข้าคลัง และคืนสิทธิ์ให้นิสิตทุกคนใช่หรือไม่?'
            : 'Archive today records and reset every claim?')) return;
      api('/api/system/reset').then(function (d) {
        if (d.ok) { refreshDashboard(); loadStudents(1); }
      }).catch(function () { });
    }

    function drawAnalyticsChart() {
      var canvas = document.getElementById('analyticsChart');
      if (!canvas) return;
      var ctx = canvas.getContext('2d');
      var isDark = document.documentElement.getAttribute('data-theme') === 'dark';
      var data = [0, 0, 0, 0];
      var labels = (currentLang === 'th') ? ['ร้านที่ 1', 'ร้านที่ 2', 'ร้านที่ 3', 'ร้านที่ 4'] : ['Shop 01', 'Shop 02', 'Shop 03', 'Shop 04'];
      if (dashboardData && dashboardData.shops) {
        data = dashboardData.shops.map(function (sh) { return sh.count; });
        labels = dashboardData.shops.map(function (sh) { return sh.name; });
      }
      var colors = ['#10b981', '#06b6d4', '#f59e0b', '#f43f5e'];
      var maxVal = Math.max.apply(null, data.concat([10]));

      ctx.clearRect(0, 0, canvas.width, canvas.height);
      var chartH = 150, startY = 180, barW = 75, gap = 65, startX = 100;
      ctx.strokeStyle = isDark ? 'rgba(255,255,255,0.08)' : 'rgba(0,0,0,0.08)';
      ctx.lineWidth = 1.5;
      ctx.beginPath(); ctx.moveTo(60, startY); ctx.lineTo(640, startY); ctx.stroke();

      for (var i = 0; i < data.length; i++) {
        var h = (data[i] / maxVal) * chartH;
        var x = startX + (i * (barW + gap));
        var y = startY - h;

        ctx.fillStyle = colors[i % colors.length];
        ctx.beginPath();
        if (typeof ctx.roundRect === 'function') ctx.roundRect(x, y, barW, h, [12, 12, 0, 0]);
        else ctx.rect(x, y, barW, h);
        ctx.fill();

        ctx.fillStyle = isDark ? '#f8fafc' : '#0f172a';
        ctx.font = 'bold 14px "Plus Jakarta Sans", Sarabun';
        ctx.textAlign = 'center';
        ctx.fillText(data[i] + (currentLang === 'th' ? ' จาน' : ''), x + (barW / 2), y - 10);

        ctx.fillStyle = isDark ? '#94a3b8' : '#64748b';
        ctx.font = '500 13px "Plus Jakarta Sans", Sarabun';
        ctx.fillText(labels[i], x + (barW / 2), startY + 24);
      }
    }

    function switchTab(tabId) {
      document.querySelectorAll('.tab-content').forEach(function (el) { el.style.display = 'none'; });
      document.querySelectorAll('.nav-item, .dropdown-item').forEach(function (el) { el.classList.remove('active'); });

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
      lsSet('canteen_tab', tabId);
      if (tabId === 'dashboard') { refreshDashboard(); }
      else if (tabId === 'students') loadStudents(curPage);
    }

    /* ---------------------------------------------------------------
       ตารางรายชื่อ: สร้างแถวด้วย DOM ทั้งหมด ชื่อที่มี < > " ' จึงปลอดภัย
       --------------------------------------------------------------- */
    function skeletonRows(count) {
      var tbody = document.getElementById('studentTableBody');
      tbody.innerHTML = '';
      for (var r = 0; r < count; r++) {
        var tr = document.createElement('tr');
        for (var c = 0; c < 7; c++) {
          var td = document.createElement('td');
          var sk = document.createElement('span');
          sk.className = 'skeleton';
          sk.style.width = (c === 1 ? '80%' : '60%');
          td.appendChild(sk);
          tr.appendChild(td);
        }
        tbody.appendChild(tr);
      }
    }

    function badge(text, color, bg) {
      var el = document.createElement('span');
      el.style.cssText = 'background:' + bg + '; color:' + color + '; font-size:0.78rem; padding:0.3rem 0.7rem; border-radius:0.75rem; font-weight:700; white-space:nowrap;';
      el.textContent = text;
      return el;
    }

    function smallButton(label, title, handler, danger) {
      var b = document.createElement('button');
      b.type = 'button';
      b.className = 'btn btn-slate';
      b.style.cssText = 'padding:0.3rem 0.6rem; font-size:0.75rem;' + (danger ? ' color:var(--accent-rose);' : '');
      b.textContent = label;
      b.title = title;
      b.addEventListener('click', handler);
      return b;
    }

    function loadStudents(page) {
      if (page < 1) page = 1;
      curPage = page;
      skeletonRows(Math.min(pageSize, 8));

      fetch('/api/students?page=' + curPage + '&limit=' + pageSize + '&search=' + encodeURIComponent(searchQuery))
        .then(function (res) {
          if (res.status === 401) { window.location.href = '/login'; throw new Error('unauthorized'); }
          return res.json();
        })
        .then(function (data) {
          totalPages = data.totalPages; curPage = data.currentPage;
          var pInd = document.getElementById('pageIndicator');
          if (pInd) pInd.textContent = curPage + ' / ' + totalPages;

          var pInfo = document.getElementById('paginationInfo');
          if (pInfo) {
            var start = (data.totalItems === 0) ? 0 : (curPage - 1) * pageSize + 1;
            var end = Math.min(curPage * pageSize, data.totalItems);
            pInfo.textContent = (currentLang === 'th')
              ? ('แสดง ' + start + ' - ' + end + ' จาก ' + data.totalItems + ' รายการ')
              : ('Showing ' + start + ' - ' + end + ' of ' + data.totalItems + ' records');
          }

          var tbody = document.getElementById('studentTableBody');
          tbody.innerHTML = '';

          if (data.students.length === 0) {
            var emptyRow = document.createElement('tr');
            var emptyCell = document.createElement('td');
            emptyCell.colSpan = 7;
            emptyCell.style.cssText = 'padding:2.5rem; text-align:center; color:var(--text-muted);';
            emptyCell.textContent = (currentLang === 'th' ? 'ไม่พบข้อมูลที่ตรงกับเงื่อนไขการค้นหา' : 'No records match your search');
            emptyRow.appendChild(emptyCell);
            tbody.appendChild(emptyRow);
            return;
          }

          data.students.forEach(function (st) {
            var tr = document.createElement('tr');

            var tdId = document.createElement('td');
            var bId = document.createElement('b');
            bId.textContent = st.id;
            tdId.appendChild(bId);

            var tdName = document.createElement('td');
            tdName.textContent = st.name;

            var tdUid = document.createElement('td');
            var codeUid = document.createElement('code');
            codeUid.textContent = st.uid || '—';
            tdUid.appendChild(codeUid);

            var tdRef = document.createElement('td');
            var codeRef = document.createElement('code');
            codeRef.textContent = st.ref;
            tdRef.appendChild(codeRef);
            if (st.isTemp) {
              tdRef.appendChild(document.createTextNode(' '));
              tdRef.appendChild(badge('Temp', 'var(--accent-yellow)', 'rgba(245,158,11,0.2)'));
            }

            var tdStatus = document.createElement('td');
            tdStatus.appendChild(st.claimed
              ? badge(st.time, 'var(--accent-green)', 'rgba(16,185,129,0.2)')
              : badge('READY', 'var(--accent-rose)', 'rgba(244,63,94,0.15)'));

            var tdShop = document.createElement('td');
            tdShop.textContent = st.shopName;

            var tdAct = document.createElement('td');
            tdAct.style.textAlign = 'right';
            tdAct.style.whiteSpace = 'nowrap';
            if (!st.claimed) {
              tdAct.appendChild(smallButton('💳 Temp', 'ผูกบัตรสำรองให้นิสิตรายนี้', (function (id) {
                return function () { openAssignTemp(id); };
              })(st.id), false));
              tdAct.appendChild(document.createTextNode(' '));
              tdAct.appendChild(smallButton('✔ ตัดสิทธิ์', 'บันทึกการรับสิทธิ์ด้วยตนเอง กรณีลืมบัตรหรือเครื่องอ่านขัดข้อง', (function (id) {
                return function () { openManualClaim(id); };
              })(st.id), false));
              tdAct.appendChild(document.createTextNode(' '));
            }
            if (st.claimed) {
              tdAct.appendChild(smallButton('🖨️', 'พิมพ์สลิปของรายการนี้ซ้ำ', (function (id) {
                return function () { printSlip(id); };
              })(st.id), false));
              tdAct.appendChild(document.createTextNode(' '));
            }
            tdAct.appendChild(smallButton('✏️', 'แก้ไขข้อมูลนิสิต', (function (a, b, c) {
              return function () { openEditModal(a, b, c); };
            })(st.id, st.name, st.uid), false));
            tdAct.appendChild(document.createTextNode(' '));
            tdAct.appendChild(smallButton('🗑️', 'ลบรายชื่อออกจากระบบ', (function (id) {
              return function () { deleteStudent(id); };
            })(st.id), true));

            tr.appendChild(tdId); tr.appendChild(tdName); tr.appendChild(tdUid);
            tr.appendChild(tdRef); tr.appendChild(tdStatus); tr.appendChild(tdShop); tr.appendChild(tdAct);
            tbody.appendChild(tr);
          });
        })
        .catch(function () {
          var tbody = document.getElementById('studentTableBody');
          if (tbody) tbody.innerHTML = '';
          toast('โหลดรายชื่อไม่สำเร็จ', false);
        });
    }

    /* --------------------------- เครื่องพิมพ์สลิป --------------------------- */
    function printSlip(id) { api('/api/print/slip', { id: id }).catch(function () { }); }
    function printDaily()  { api('/api/print/daily').catch(function () { }); }
    function printTest()   { api('/api/print/test').catch(function () { }); }

    function changePageSize(val) { pageSize = parseInt(val, 10); curPage = 1; loadStudents(1); }
    function prevPage() { if (curPage > 1) loadStudents(curPage - 1); }
    function nextPage() { if (curPage < totalPages) loadStudents(curPage + 1); }
    function goToPage(p) { loadStudents(p); }
    function goToLastPage() { loadStudents(totalPages); }
    function handleSearch(val) {
      clearTimeout(searchTimer);
      searchTimer = setTimeout(function () { searchQuery = val.trim(); curPage = 1; loadStudents(1); }, 350);
    }

    /* --------------------------- Modals --------------------------- */
    function openAddModal() {
      document.getElementById('modalOldId').value = ''; document.getElementById('modalId').value = '';
      document.getElementById('modalName').value = ''; document.getElementById('modalUid').value = '';
      document.getElementById('modalTitle').textContent = 'เพิ่มผู้มีสิทธิ์รายใหม่';
      document.getElementById('studentModal').classList.add('active');
    }
    function openEditModal(id, name, uid) {
      document.getElementById('modalOldId').value = id; document.getElementById('modalId').value = id;
      document.getElementById('modalName').value = name; document.getElementById('modalUid').value = uid;
      document.getElementById('modalTitle').textContent = 'แก้ไขข้อมูล: ' + id;
      document.getElementById('studentModal').classList.add('active');
    }
    function closeModal() { document.getElementById('studentModal').classList.remove('active'); }
    function openTempModal() { document.getElementById('tempCardModal').classList.add('active'); }
    function closeTempModal() { document.getElementById('tempCardModal').classList.remove('active'); }

    var manualClaimTarget = '', assignTempTarget = '';
    function openManualClaim(id) {
      manualClaimTarget = id;
      document.getElementById('manualClaimId').textContent = id;
      document.getElementById('manualClaimStation').value = '1';
      document.getElementById('manualClaimModal').classList.add('active');
    }
    function closeManualClaim() { document.getElementById('manualClaimModal').classList.remove('active'); }
    function confirmManualClaim() {
      var station = document.getElementById('manualClaimStation').value;
      api('/api/claim/manual', { id: manualClaimTarget, station: station }).then(function (d) {
        if (d.ok) { closeManualClaim(); loadStudents(curPage); refreshDashboard(); }
      }).catch(function () { });
    }

    function openAssignTemp(id) {
      assignTempTarget = id;
      document.getElementById('assignTempId').textContent = id;
      document.getElementById('assignTempUid').value = '';
      document.getElementById('assignTempModal').classList.add('active');
      setTimeout(function () { document.getElementById('assignTempUid').focus(); }, 60);
    }
    function closeAssignTemp() { document.getElementById('assignTempModal').classList.remove('active'); }
    function confirmAssignTemp() {
      var uid = document.getElementById('assignTempUid').value.trim();
      if (!uid) { toast('กรุณากรอกเลขบัตรสำรอง', false); return; }
      api('/api/tempcard/save', { studentId: assignTempTarget, uid: uid }).then(function (d) {
        if (d.ok) { closeAssignTemp(); loadStudents(curPage); refreshDashboard(); }
      }).catch(function () { });
    }

    function removeTempCard(id) {
      if (!confirm('ต้องการยกเลิกบัตรสำรองของนิสิต ' + id + ' ใช่หรือไม่?')) return;
      api('/api/tempcard/remove', { id: id }).then(function (d) {
        if (d.ok) setTimeout(function () { window.location.reload(); }, 700);
      }).catch(function () { });
    }
    function deleteStudent(id) {
      if (!confirm('ต้องการลบรายชื่อนิสิต ' + id + ' ออกจากระบบใช่หรือไม่?')) return;
      api('/api/student/delete', { id: id }).then(function (d) {
        if (d.ok) { loadStudents(curPage); refreshDashboard(); }
      }).catch(function () { });
    }

    function openAddAdminModal() {
      document.getElementById('adminOldUser').value = '';
      document.getElementById('adminUsername').value = '';
      document.getElementById('adminDisplayName').value = '';
      document.getElementById('adminPassword').value = '';
      document.getElementById('adminPassword').required = true;
      document.getElementById('adminModalTitle').textContent = 'เพิ่มเจ้าหน้าที่ใหม่ (สูงสุด 3 ท่าน)';
      document.getElementById('adminModal').classList.add('active');
    }
    function openEditAdminModal(user, name) {
      document.getElementById('adminOldUser').value = user;
      document.getElementById('adminUsername').value = user;
      document.getElementById('adminDisplayName').value = name;
      document.getElementById('adminPassword').value = '';
      document.getElementById('adminPassword').required = false;
      document.getElementById('adminModalTitle').textContent = 'แก้ไขเจ้าหน้าที่ (' + user + ')';
      document.getElementById('adminModal').classList.add('active');
    }
    function closeAdminModal() { document.getElementById('adminModal').classList.remove('active'); }

    /* ปุ่มที่ถูกสร้างจากฝั่งเซิร์ฟเวอร์ ใช้ data-attribute แทนการฝังค่าลงใน onclick */
    function bindDelegatedActions() {
      document.addEventListener('click', function (ev) {
        var el = ev.target.closest ? ev.target.closest('button') : null;
        if (!el) return;

        if (el.classList.contains('js-archive-delete')) {
          var file = el.getAttribute('data-file');
          if (!confirm('ต้องการลบไฟล์ประวัติ ' + file + ' ใช่หรือไม่?')) return;
          api('/api/archive/delete', { file: file }).then(function (d) {
            if (d.ok) setTimeout(function () { window.location.reload(); }, 700);
          }).catch(function () { });
        } else if (el.classList.contains('js-temp-revoke')) {
          removeTempCard(el.getAttribute('data-id'));
        } else if (el.classList.contains('js-admin-edit')) {
          openEditAdminModal(el.getAttribute('data-user'), el.getAttribute('data-name'));
        } else if (el.classList.contains('js-admin-delete')) {
          var user = el.getAttribute('data-user');
          if (!confirm('ยืนยันลบเจ้าหน้าที่ ' + user + ' ใช่หรือไม่?')) return;
          api('/api/admin/delete', { user: user }).then(function (d) {
            if (d.ok) setTimeout(function () { window.location.reload(); }, 700);
          }).catch(function () { });
        }
      });

      /* ปิด modal ด้วยปุ่ม Esc และการคลิกพื้นหลัง */
      document.addEventListener('keydown', function (ev) {
        if (ev.key === 'Escape') {
          document.querySelectorAll('.modal.active').forEach(function (m) { m.classList.remove('active'); });
        }
      });
      document.querySelectorAll('.modal').forEach(function (m) {
        m.addEventListener('mousedown', function (ev) { if (ev.target === m) m.classList.remove('active'); });
      });
      var uidInput = document.getElementById('assignTempUid');
      if (uidInput) {
        uidInput.addEventListener('keydown', function (ev) { if (ev.key === 'Enter') confirmAssignTemp(); });
      }
    }

    /* เรียกทีละขั้นแบบหุ้ม try/catch เพื่อไม่ให้ขั้นใดขั้นหนึ่งล้มแล้วลากส่วนที่เหลือตายไปด้วย */
    [initTheme, bindAjaxForms, bindDelegatedActions].forEach(function (fn) {
      try { fn(); } catch (e) { console.warn('init skipped:', e && e.message); }
    });
    refreshDashboard();
    dashboardTimer = setInterval(refreshDashboard, 3000);

    var savedTab = lsGet('canteen_tab');
    if (savedTab && document.getElementById('tab-' + savedTab)) switchTab(savedTab);
  </script>
</body>
</html>)rawliteral";
  return html;
}

// ============================================================================
// PUBLIC DISPLAY — หน้าจอสาธารณะสำหรับต่อออกมอนิเตอร์จอใหญ่
// เปิดได้โดยไม่ต้องเข้าสู่ระบบ จึงต้องไม่มีข้อมูลส่วนบุคคลและไม่มีปุ่มสั่งงานใด ๆ
// ใช้ฟอนต์ของระบบล้วน ๆ เพราะเครือข่ายของเครื่องแม่ข่ายไม่มีทางออกอินเทอร์เน็ต
// ที่จะโหลดเว็บฟอนต์ได้
// ============================================================================
String getDisplayHTML() {
  return String(R"rawliteral(<!DOCTYPE html>
<html lang="th">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>โรงอาหาร มจร. แพร่ - สถานะการให้บริการ</title>
<style>
  :root{
    --bg:#070b12; --panel:#111a28; --panel-2:#16202f; --line:#243147;
    --ink:#f4f8ff; --dim:#8fa3c0;
    --green:#3ddc97; --cyan:#38d6f0; --amber:#ffc53d; --rose:#ff6b81;
  }
  *{box-sizing:border-box;margin:0;padding:0}
  html,body{height:100%}
  body{
    background:var(--bg); color:var(--ink); overflow:hidden;
    font-family:"Noto Sans Thai","IBM Plex Sans Thai",system-ui,-apple-system,"Segoe UI",Tahoma,sans-serif;
    display:flex; flex-direction:column; height:100dvh;
  }
  body.idle{cursor:none}
  .mono{font-family:ui-monospace,"SF Mono",Menlo,Consolas,monospace;font-variant-numeric:tabular-nums}

  /* ---------- header ---------- */
  header{
    display:flex; align-items:center; justify-content:space-between; gap:2vmin;
    padding:1.6vmin 2.4vmin; border-bottom:1px solid var(--line);
    background:linear-gradient(180deg,#0d1522,#070b12);
  }
  .brand{display:flex;align-items:center;gap:1.4vmin;min-width:0}
  .brand-mark{
    width:clamp(34px,4.6vmin,64px); height:clamp(34px,4.6vmin,64px); border-radius:28%;
    background:linear-gradient(135deg,var(--green),var(--cyan)); color:#062018;
    display:grid; place-items:center; font-weight:800; font-size:clamp(14px,2.1vmin,28px); flex:0 0 auto;
  }
  .brand-txt h1{font-size:clamp(15px,2.3vmin,32px);font-weight:700;letter-spacing:-.01em;white-space:nowrap}
  .brand-txt p{font-size:clamp(11px,1.5vmin,19px);color:var(--dim);white-space:nowrap}
  .head-right{display:flex;align-items:center;gap:1.8vmin;flex:0 0 auto}
  .svc{
    font-size:clamp(11px,1.6vmin,21px); font-weight:700; padding:.7vmin 1.6vmin; border-radius:999px;
    border:1px solid transparent; white-space:nowrap;
  }
  .svc.open{color:var(--green);background:rgba(61,220,151,.14);border-color:rgba(61,220,151,.4)}
  .svc.shut{color:var(--rose);background:rgba(255,107,129,.14);border-color:rgba(255,107,129,.4)}
  #clock{font-size:clamp(26px,5.4vmin,76px);font-weight:700;letter-spacing:-.02em;line-height:1}
  #today{font-size:clamp(10px,1.4vmin,18px);color:var(--dim);text-align:right}

  /* ---------- main ---------- */
  main{flex:1;display:grid;grid-template-columns:1fr;gap:1.6vmin;padding:1.6vmin 2.4vmin;min-height:0}
  @media (min-width:900px){ main{grid-template-columns:5fr 4fr} }
  .panel{background:var(--panel);border:1px solid var(--line);border-radius:2vmin;padding:2vmin 2.4vmin;min-height:0;display:flex;flex-direction:column}
  .panel h2{font-size:clamp(12px,1.7vmin,22px);font-weight:700;color:var(--dim);letter-spacing:.04em;text-transform:uppercase;margin-bottom:1vmin}

  .big{display:flex;align-items:baseline;gap:1.2vmin;flex-wrap:wrap}
  #used{font-size:clamp(56px,15vmin,220px);font-weight:800;color:var(--green);line-height:.92;letter-spacing:-.03em}
  .of{font-size:clamp(16px,2.6vmin,38px);color:var(--dim);font-weight:600}
  .bar{height:clamp(10px,1.6vmin,22px);background:var(--panel-2);border-radius:999px;overflow:hidden;margin:1.8vmin 0 1.2vmin}
  #barFill{height:100%;width:0;border-radius:999px;background:linear-gradient(90deg,var(--green),var(--cyan));transition:width .6s ease}
  .mid{margin:auto 0;text-align:center;padding:1vmin 0}
  .mid-k{font-size:clamp(11px,1.6vmin,21px);color:var(--dim);letter-spacing:.04em}
  .mid-v{font-size:clamp(30px,7.4vmin,116px);font-weight:800;color:var(--amber);line-height:1.05;letter-spacing:-.02em}
  .row2{display:flex;justify-content:space-between;gap:2vmin;flex-wrap:wrap;padding-top:1.4vmin;border-top:1px solid var(--line)}
  .stat .k{font-size:clamp(10px,1.4vmin,18px);color:var(--dim)}
  .stat .v{font-size:clamp(20px,3.4vmin,48px);font-weight:800;line-height:1.1}
  .v-amber{color:var(--amber)}

  /* ---------- feed ---------- */
  .feed-head{display:flex;align-items:center;justify-content:space-between;gap:1vmin}
  .live{display:inline-flex;align-items:center;gap:.7vmin;font-size:clamp(10px,1.4vmin,18px);color:var(--green);font-weight:700}
  .live i{width:.9vmin;height:.9vmin;min-width:6px;min-height:6px;border-radius:50%;background:var(--green);animation:blip 1.6s infinite}
  @keyframes blip{0%,100%{opacity:1}50%{opacity:.2}}
  #feed{list-style:none;flex:1;overflow:hidden;display:flex;flex-direction:column;gap:.9vmin;margin-top:1vmin}
  #feed li{
    display:grid;grid-template-columns:auto 1fr auto;align-items:center;gap:1.4vmin;
    background:var(--panel-2);border:1px solid var(--line);border-left:.5vmin solid var(--green);
    border-radius:1.2vmin;padding:1vmin 1.6vmin;
  }
  #feed li.fresh{animation:pop .9s ease-out}
  @keyframes pop{0%{background:rgba(61,220,151,.3);transform:translateY(-.8vmin)}100%{background:var(--panel-2);transform:none}}
  .f-time{font-size:clamp(12px,1.9vmin,26px);color:var(--dim)}
  .f-id{font-size:clamp(18px,3.1vmin,44px);font-weight:700;letter-spacing:.06em}
  .f-shop{font-size:clamp(11px,1.7vmin,23px);color:var(--cyan);font-weight:600;text-align:right;max-width:34vw;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
  .empty{color:var(--dim);font-size:clamp(13px,2vmin,26px);text-align:center;margin:auto;padding:2vmin}

  /* ---------- shops ---------- */
  .shops{display:grid;grid-template-columns:repeat(2,1fr);gap:1.4vmin;padding:0 2.4vmin 2vmin}
  @media (min-width:900px){ .shops{grid-template-columns:repeat(4,1fr)} }
  .shop{background:var(--panel);border:1px solid var(--line);border-radius:1.6vmin;padding:1.4vmin 1.8vmin;min-width:0}
  .shop.off{border-color:rgba(255,107,129,.35)}
  .shop-top{display:flex;align-items:center;gap:.8vmin;min-width:0}
  .dot{width:1vmin;height:1vmin;min-width:7px;min-height:7px;border-radius:50%;background:var(--green);flex:0 0 auto}
  .shop.off .dot{background:var(--rose)}
  .shop-name{font-size:clamp(11px,1.7vmin,23px);font-weight:700;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
  .shop-nums{display:flex;align-items:baseline;gap:1vmin;margin-top:.6vmin;flex-wrap:wrap}
  .shop-cnt{font-size:clamp(22px,4vmin,58px);font-weight:800;color:var(--green);line-height:1}
  .shop-unit{font-size:clamp(10px,1.4vmin,18px);color:var(--dim)}
  .shop-amt{font-size:clamp(12px,1.8vmin,24px);color:var(--amber);font-weight:700;margin-left:auto}

  /* ---------- connection banner ---------- */
  #warn{
    flex:0 0 auto; display:none; text-align:center;
    background:#4a121f; border-top:2px solid var(--rose); color:#ffd7dd;
    padding:1.1vmin 2.4vmin; font-size:clamp(12px,1.8vmin,24px); font-weight:700;
  }
  #warn.on{display:block}
</style>
</head>
<body>

<header>
  <div class="brand">
    <div class="brand-mark">มจร</div>
    <div class="brand-txt">
      <h1>โรงอาหารนิสิต มจร. วิทยาเขตแพร่</h1>
      <p>สวัสดิการอาหารกลางวัน 35 บาท / คน / วัน</p>
    </div>
  </div>
  <div class="head-right">
    <span class="svc" id="svc">—</span>
    <div>
      <div id="clock" class="mono">--:--:--</div>
      <div id="today">—</div>
    </div>
  </div>
</header>

<main>
  <section class="panel">
    <h2>ใช้สิทธิ์แล้ววันนี้</h2>
    <div class="big">
      <span id="used" class="mono">0</span>
      <span class="of">/ <span id="total" class="mono">0</span> คน</span>
    </div>
    <div class="bar"><div id="barFill"></div></div>
    <div class="mid">
      <div class="mid-k">ยอดจัดสรรสวัสดิการวันนี้</div>
      <div class="mid-v mono" id="amount">0 บาท</div>
    </div>
    <div class="row2">
      <div class="stat"><div class="k">คงเหลือสิทธิ์</div><div class="v mono" id="remain">0</div></div>
      <div class="stat" style="text-align:right"><div class="k">คิดเป็นสัดส่วนผู้มีสิทธิ์</div><div class="v mono" id="pct">0%</div></div>
    </div>
  </section>

  <section class="panel">
    <div class="feed-head">
      <h2 style="margin:0">รายการล่าสุด</h2>
      <span class="live"><i></i> อัปเดตอัตโนมัติ</span>
    </div>
    <ul id="feed"><li class="empty">กำลังเชื่อมต่อกับเครื่องแม่ข่าย...</li></ul>
  </section>
</main>

<div class="shops" id="shops"></div>
<div id="warn">⚠️ ขาดการเชื่อมต่อกับเครื่องแม่ข่าย — ตัวเลขที่เห็นอาจไม่ใช่ข้อมูลล่าสุด กำลังลองเชื่อมต่อใหม่...</div>

<script>
var seen = {};
var first = true;
var fails = 0;

function el(tag, cls, txt){
  var e = document.createElement(tag);
  if (cls) e.className = cls;
  if (txt !== undefined) e.textContent = txt;
  return e;
}
function nf(n){ return Number(n).toLocaleString('th-TH'); }

function renderShops(shops){
  var wrap = document.getElementById('shops');
  wrap.innerHTML = '';
  shops.forEach(function(s){
    var card = el('div', 'shop' + (s.online ? '' : ' off'));
    var top = el('div', 'shop-top');
    top.appendChild(el('span', 'dot'));
    top.appendChild(el('span', 'shop-name', s.name));
    var nums = el('div', 'shop-nums');
    nums.appendChild(el('span', 'shop-cnt mono', nf(s.count)));
    nums.appendChild(el('span', 'shop-unit', 'จาน'));
    nums.appendChild(el('span', 'shop-amt mono', nf(s.amount) + ' บาท'));
    card.appendChild(top);
    card.appendChild(nums);
    wrap.appendChild(card);
  });
}

function renderFeed(events, shops){
  var list = document.getElementById('feed');
  list.innerHTML = '';
  if (!events.length){
    var e = el('li', 'empty', 'ยังไม่มีการใช้สิทธิ์ในวันนี้');
    list.appendChild(e);
    return;
  }
  var fresh = {};
  events.forEach(function(ev){
    var key = ev.time + '|' + ev.id + '|' + ev.station;
    var li = el('li');
    li.appendChild(el('span', 'f-time mono', ev.time));
    li.appendChild(el('span', 'f-id mono', ev.id));
    var shop = shops[ev.station - 1];
    li.appendChild(el('span', 'f-shop', shop ? shop.name : ('จุดบริการ ' + ev.station)));
    if (!first && !seen[key]) li.classList.add('fresh');
    fresh[key] = true;
    list.appendChild(li);
  });
  seen = fresh;
  first = false;
}

function tick(){
  fetch('/api/display', { cache: 'no-store' })
    .then(function(r){ return r.json(); })
    .then(function(d){
      if (!d.ok) throw new Error('disabled');
      fails = 0;
      document.getElementById('warn').classList.remove('on');

      document.getElementById('clock').textContent  = d.clock;
      document.getElementById('today').textContent  = d.date;
      document.getElementById('used').textContent   = nf(d.used);
      document.getElementById('total').textContent  = nf(d.total);
      document.getElementById('remain').textContent = nf(d.remaining);
      document.getElementById('amount').textContent = nf(d.disbursed) + ' บาท';
      document.getElementById('pct').textContent     = d.quotaPct + '%';
      document.getElementById('barFill').style.width = d.quotaPct + '%';

      var svc = document.getElementById('svc');
      svc.textContent = (d.serviceOpen ? '● เปิดให้บริการ ' : '● นอกเวลาให้บริการ ') + d.window;
      svc.className = 'svc ' + (d.serviceOpen ? 'open' : 'shut');

      renderShops(d.shops);
      renderFeed(d.events, d.shops);
    })
    .catch(function(){
      if (++fails >= 2) document.getElementById('warn').classList.add('on');
    });
}

tick();
setInterval(tick, 2000);

/* ซ่อนเคอร์เซอร์เมื่อไม่มีการขยับเมาส์ เพื่อให้จอดูสะอาดตา */
var idleTimer = null;
function wake(){
  document.body.classList.remove('idle');
  clearTimeout(idleTimer);
  idleTimer = setTimeout(function(){ document.body.classList.add('idle'); }, 4000);
}
['mousemove','touchstart','keydown'].forEach(function(evt){ document.addEventListener(evt, wake); });
wake();
</script>
</body>
</html>)rawliteral");
}
