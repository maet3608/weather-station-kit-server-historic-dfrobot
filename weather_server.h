#pragma once
#include <WebServer.h>
#include "weather_history.h"   // brings in weather_data.h

extern WeatherData wxData;     // defined in .ino
extern WebServer   server;     // defined in .ino

// ── HTML ─────────────────────────────────────────────────────────────────────
static const char HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Weather Station</title>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.4.0/chart.umd.min.js"></script>
  <style>
    * { box-sizing:border-box; margin:0; padding:0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background: #e8f4f8; min-height:100vh;
      display:flex; align-items:flex-start; justify-content:center; padding:20px;
    }
    .card {
      background:white; border-radius:16px;
      box-shadow:0 4px 20px rgba(0,0,0,.1);
      width:100%; max-width:620px; overflow:hidden;
    }
    .header {
      background:linear-gradient(135deg,#1a6b9a,#2196a6);
      color:white; padding:24px; text-align:center;
    }
    .header h1 { font-size:1.5rem; font-weight:600; }
    .header p  { opacity:.8; font-size:.85rem; margin-top:4px; }
    .grid { display:grid; grid-template-columns:1fr 1fr; gap:1px; background:#e2e8f0; }
    .cell {
      background:white; padding:16px;
      display:flex; flex-direction:column; gap:3px;
    }
    .cell .icon  { font-size:1.3rem; }
    .cell .label { font-size:.72rem; color:#718096; text-transform:uppercase; letter-spacing:.05em; }
    .cell .value { font-size:1.3rem; font-weight:700; color:#2d3748; }
    .cell .unit  { font-size:.78rem; color:#a0aec0; }
    .history     { padding:20px; border-top:1px solid #e2e8f0; }
    .hist-title  {
      font-size:.8rem; font-weight:600; color:#718096;
      text-transform:uppercase; letter-spacing:.08em; margin-bottom:14px;
    }
    .tabs { display:flex; gap:6px; flex-wrap:wrap; margin-bottom:16px; }
    .tab {
      padding:5px 14px; border-radius:20px; border:none; cursor:pointer;
      font-size:.8rem; font-weight:500; background:#edf2f7; color:#4a5568;
      transition:all .2s;
    }
    .tab.active { background:#1a6b9a; color:white; }
    .chart-wrap { position:relative; height:210px; }
    .no-hist {
      display:flex; flex-direction:column;
      align-items:center; justify-content:center;
      height:210px; color:#a0aec0; font-size:.85rem; gap:8px;
    }
    .footer {
      padding:12px 20px; text-align:center;
      font-size:.78rem; color:#a0aec0; background:#f7fafc;
    }
    .dot {
      display:inline-block; width:8px; height:8px; border-radius:50%;
      background:#48bb78; margin-right:5px; animation:pulse 2s infinite;
    }
    @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:.3} }
  </style>
</head>
<body>
<div class="card">
  <div class="header">
    <h1>&#127780; Weather Station</h1>
    <p id="status">Loading...</p>
  </div>

  <div class="grid">
    <div class="cell"><span class="icon">&#x1F9ED;</span><span class="label">Wind Direction</span><span class="value" id="windDir">--</span><span class="unit">degrees</span></div>
    <div class="cell"><span class="icon">&#x1F4A8;</span><span class="label">Wind Speed (avg)</span><span class="value" id="windAvg">--</span><span class="unit">m/s</span></div>
    <div class="cell"><span class="icon">&#x26A1;</span><span class="label">Wind Speed (max)</span><span class="value" id="windMax">--</span><span class="unit">m/s</span></div>
    <div class="cell"><span class="icon">&#x1F321;&#xFE0F;</span><span class="label">Temperature</span><span class="value" id="temp">--</span><span class="unit">&deg;C</span></div>
    <div class="cell"><span class="icon">&#x1F4A7;</span><span class="label">Humidity</span><span class="value" id="hum">--</span><span class="unit">%</span></div>
    <div class="cell"><span class="icon">&#x1F30A;</span><span class="label">Pressure</span><span class="value" id="pres">--</span><span class="unit">hPa</span></div>
    <div class="cell"><span class="icon">&#x1F327;&#xFE0F;</span><span class="label">Rainfall (1 hr)</span><span class="value" id="rain1">--</span><span class="unit">mm</span></div>
    <div class="cell"><span class="icon">&#x1F327;&#xFE0F;</span><span class="label">Rainfall (24 hr)</span><span class="value" id="rain24">--</span><span class="unit">mm</span></div>
  </div>

  <div class="history">
    <div class="hist-title">&#x1F4C8; History &mdash; last 5 days (30-min samples)</div>
    <div class="tabs">
      <button class="tab active" onclick="switchTab(this,'temp')">Temperature</button>
      <button class="tab" onclick="switchTab(this,'hum')">Humidity</button>
      <button class="tab" onclick="switchTab(this,'pres')">Pressure</button>
      <button class="tab" onclick="switchTab(this,'wind')">Wind Speed</button>
      <button class="tab" onclick="switchTab(this,'wdir')">Wind Dir</button>
      <button class="tab" onclick="switchTab(this,'rain')">Rainfall</button>
    </div>
    <div class="chart-wrap">
      <canvas id="wxChart"></canvas>
      <div id="noHist" class="no-hist" style="display:none">
        <span>&#x23F3;</span><span>No history yet &mdash; first point records in 30 min</span>
      </div>
    </div>
  </div>

  <div class="footer"><span class="dot"></span><span id="updated">Waiting for first reading...</span></div>
</div>

<script>
let wxChart   = null;
let hist      = null;
let activeTab = 'temp';

// Metric definitions: single object = one line, array = multi-line
const CFG = {
  temp: { label:'Temperature (°C)',  get:r=>r.temp, color:'#e53e3e', fill:'rgba(229,62,62,0.08)' },
  hum:  { label:'Humidity (%)',       get:r=>r.hum,  color:'#3182ce', fill:'rgba(49,130,206,0.08)' },
  pres: { label:'Pressure (hPa)',     get:r=>r.pres, color:'#805ad5', fill:'rgba(128,90,213,0.08)' },
  wdir: { label:'Wind Direction (°)', get:r=>r.wdir, color:'#2f855a', fill:'rgba(47,133,90,0.08)' },
  rain: { label:'Rainfall 1h (mm)',   get:r=>r.r1h,  color:'#2b6cb0', fill:'rgba(43,108,176,0.12)', bar:true },
  wind: [
    { label:'Avg Speed (m/s)', get:r=>r.wavg, color:'#319795', fill:false },
    { label:'Max Speed (m/s)', get:r=>r.wmax, color:'#dd6b20', fill:false }
  ]
};

function timeLabel(t, latest) {
  const s = latest - t;
  if (s < 300)  return 'now';
  if (s < 3600) return '-' + Math.round(s/60) + 'm';
  return '-' + (s/3600).toFixed(1) + 'h';
}

function renderChart(tab) {
  const canvas = document.getElementById('wxChart');
  const noHist = document.getElementById('noHist');
  if (!hist || hist.count === 0) {
    canvas.style.display = 'none'; noHist.style.display = 'flex'; return;
  }
  canvas.style.display = 'block'; noHist.style.display = 'none';

  const rec    = hist.records;
  const latest = rec[rec.length - 1].t;
  const labels = rec.map(r => timeLabel(r.t, latest));
  const cfgs   = Array.isArray(CFG[tab]) ? CFG[tab] : [CFG[tab]];
  const sparse = rec.length > 100;

  const datasets = cfgs.map(c => ({
    label: c.label,
    data: rec.map(r => c.get(r)),
    borderColor: c.color,
    backgroundColor: c.fill || c.color + '99',
    fill: !!c.fill,
    tension: 0.3,
    pointRadius: sparse ? 0 : 3,
    borderWidth: 2
  }));

  if (wxChart) wxChart.destroy();
  wxChart = new Chart(canvas, {
    type: CFG[tab].bar ? 'bar' : 'line',
    data: { labels, datasets },
    options: {
      responsive: true, maintainAspectRatio: false, animation: false,
      plugins: { legend: { display: cfgs.length > 1, labels: { font:{ size:11 } } } },
      scales: {
        x: { ticks:{ maxTicksLimit:8, font:{size:10} }, grid:{ color:'rgba(0,0,0,0.04)' } },
        y: { ticks:{ font:{size:10} },                  grid:{ color:'rgba(0,0,0,0.04)' } }
      }
    }
  });
}

function switchTab(btn, tab) {
  document.querySelectorAll('.tab').forEach(b => b.classList.remove('active'));
  btn.classList.add('active');
  activeTab = tab;
  renderChart(tab);
}

function updateCurrent() {
  fetch('/data').then(r=>r.json()).then(d => {
    if (!d.valid) { document.getElementById('status').textContent='Waiting for sensor...'; return; }
    document.getElementById('status').textContent  = 'Makerguides';
    document.getElementById('windDir').textContent = d.windDir;
    document.getElementById('windAvg').textContent = d.windAvg.toFixed(2);
    document.getElementById('windMax').textContent = d.windMax.toFixed(2);
    document.getElementById('temp').textContent    = d.tempC.toFixed(1);
    document.getElementById('hum').textContent     = d.humidity;
    document.getElementById('pres').textContent    = d.pressure.toFixed(1);
    document.getElementById('rain1').textContent   = d.rain1h.toFixed(2);
    document.getElementById('rain24').textContent  = d.rain24h.toFixed(2);
    document.getElementById('updated').textContent = 'Updated: ' + new Date().toLocaleTimeString();
  }).catch(() => { document.getElementById('status').textContent = 'Connection error'; });
}

function updateHistory() {
  fetch('/history').then(r=>r.json()).then(d => { hist = d; renderChart(activeTab); }).catch(()=>{});
}

updateCurrent(); updateHistory();
setInterval(updateCurrent,  10000);   // current data every 10 s
setInterval(updateHistory,  60000);   // history every 60 s
</script>
</body>
</html>
)rawhtml";

// ── ROUTE HANDLERS ────────────────────────────────────────────────────────────

void handleRoot() {
  server.send_P(200, "text/html", HTML);
}

void handleData() {
  if (!wxData.valid) { server.send(200, "application/json", "{\"valid\":false}"); return; }
  char json[256];
  snprintf(json, sizeof(json),
    "{\"valid\":true,\"windDir\":%d,\"windAvg\":%.2f,\"windMax\":%.2f,"
    "\"tempC\":%.1f,\"humidity\":%d,\"pressure\":%.1f,\"rain1h\":%.2f,\"rain24h\":%.2f}",
    wxData.windDir, wxData.windAvg, wxData.windMax,
    wxData.tempC, wxData.humidity, wxData.pressure, wxData.rain1h, wxData.rain24h);
  server.send(200, "application/json", json);
}

void handleHistory() {
  // Stream JSON in small chunks – avoids a large heap allocation for 240 records
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  char buf[128];
  snprintf(buf, sizeof(buf), "{\"count\":%d,\"records\":[", histCount);
  server.sendContent(buf);
  for (int i = 0; i < histCount; i++) {
    int idx = (histHead - histCount + i + HISTORY_SIZE) % HISTORY_SIZE;
    const HistoryRecord& r = histBuf[idx];
    snprintf(buf, sizeof(buf),
      "%s{\"t\":%u,\"temp\":%.1f,\"hum\":%u,\"pres\":%.1f,"
      "\"wdir\":%u,\"wavg\":%.2f,\"wmax\":%.2f,\"r1h\":%.2f,\"r24h\":%.2f}",
      i ? "," : "",
      (unsigned)r.ts, r.tempC, (unsigned)r.humidity, r.pressure,
      (unsigned)r.windDir, r.windAvg, r.windMax, r.rain1h, r.rain24h);
    server.sendContent(buf);
  }
  server.sendContent("]}");
  server.sendContent("");    // end chunked transfer
}

// ── PUBLIC API ────────────────────────────────────────────────────────────────

void serverSetup() {
  server.on("/",        handleRoot);
  server.on("/data",    handleData);
  server.on("/history", handleHistory);
  server.begin();
  Serial.println("[HTTP] Server started on port 80");
}

void serverHandle() { server.handleClient(); }