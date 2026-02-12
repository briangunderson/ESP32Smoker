#ifndef WEB_CONTENT_H
#define WEB_CONTENT_H

#include <Arduino.h>

const char web_index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>GunderGrill</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <div class="container">
        <header class="header">
            <div class="header-left">
                <h1>GunderGrill</h1>
                <div class="connection-bar">
                    <span class="conn-dot" id="conn-wifi" title="WiFi"></span>
                    <span class="conn-label" id="conn-wifi-label">WiFi</span>
                    <span class="conn-dot" id="conn-api" title="API"></span>
                    <span class="conn-label" id="conn-api-label">API</span>
                </div>
            </div>
            <div class="state-badge" id="state-badge">Idle</div>
        </header>

        <main>
            <!-- Temperature Hero -->
            <section class="temp-hero">
                <div class="temp-block temp-current">
                    <div class="temp-ring" id="temp-ring">
                        <svg viewBox="0 0 120 120">
                            <circle class="ring-bg" cx="60" cy="60" r="52" />
                            <circle class="ring-fill" id="ring-fill" cx="60" cy="60" r="52" />
                        </svg>
                        <div class="temp-value">
                            <span id="current-temp">--</span>
                            <span class="temp-unit">&deg;F</span>
                        </div>
                    </div>
                    <div class="temp-label">Current</div>
                </div>
                <div class="temp-block temp-target">
                    <div class="temp-ring target">
                        <svg viewBox="0 0 120 120">
                            <circle class="ring-bg" cx="60" cy="60" r="52" />
                        </svg>
                        <div class="temp-value">
                            <span id="setpoint-temp">225</span>
                            <span class="temp-unit">&deg;F</span>
                        </div>
                    </div>
                    <div class="temp-label">Target</div>
                </div>
            </section>

            <!-- Temperature Graph -->
            <section class="card graph-card">
                <h3>Temperature History</h3>
                <canvas id="temp-graph"></canvas>
                <div class="graph-legend">
                    <span class="legend-item"><span class="legend-swatch temp"></span>Temp</span>
                    <span class="legend-item"><span class="legend-swatch setpoint"></span>Setpoint</span>
                    <span class="legend-item"><span class="legend-swatch event"></span>State Change</span>
                </div>
            </section>

            <!-- Controls -->
            <section class="card controls-card">
                <div class="control-row">
                    <button id="btn-start" class="btn btn-start" onclick="startSmoking()">Start</button>
                    <button id="btn-stop" class="btn btn-stop" onclick="stopSmoking()" disabled>Stop</button>
                    <button id="btn-shutdown" class="btn btn-shutdown" onclick="doShutdown()">Shutdown</button>
                </div>
                <div class="setpoint-row">
                    <button class="btn-adj" onclick="adjSetpoint(-5)">-</button>
                    <input type="number" id="setpoint-input" min="150" max="500" value="225" step="5">
                    <span class="sp-unit">&deg;F</span>
                    <button class="btn-adj" onclick="adjSetpoint(5)">+</button>
                    <button class="btn btn-apply" onclick="applySetpoint()">Set</button>
                </div>
            </section>

            <!-- Relays & Info Row -->
            <div class="info-row">
                <section class="card relay-card">
                    <h3>Relays</h3>
                    <div class="relay-grid">
                        <div class="relay" id="relay-auger">
                            <div class="relay-led"></div>
                            <span>Auger</span>
                        </div>
                        <div class="relay" id="relay-fan">
                            <div class="relay-led"></div>
                            <span>Fan</span>
                        </div>
                        <div class="relay" id="relay-igniter">
                            <div class="relay-led"></div>
                            <span>Igniter</span>
                        </div>
                    </div>
                </section>
                <section class="card info-card">
                    <h3>Info</h3>
                    <div class="info-list">
                        <div class="info-row-item">
                            <span>Runtime</span><span id="runtime">0:00</span>
                        </div>
                        <div class="info-row-item">
                            <span>Errors</span><span id="error-count">0</span>
                        </div>
                        <div class="info-row-item">
                            <span>Heap</span><span id="heap-free">--</span>
                        </div>
                        <div class="info-row-item">
                            <span>Firmware</span><span id="fw-version">--</span>
                        </div>
                        <div class="info-row-item hidden" id="update-row">
                            <span>Update</span>
                            <span><span id="update-version"></span> <button class="btn-sm btn-on" onclick="applyUpdate()">Install</button></span>
                        </div>
                    </div>
                </section>
            </div>

            <!-- Debug Panel -->
            <section class="card debug-card">
                <h3 class="debug-header" onclick="toggleDebug()">
                    Debug / Testing <span id="debug-arrow">&#9662;</span>
                </h3>
                <div id="debug-panel" class="debug-panel hidden">
                    <div class="debug-warn">Debug mode disables automatic temperature control.</div>
                    <div class="debug-btn-row">
                        <button id="btn-debug" class="btn btn-debug" onclick="toggleDebugMode()">Enable Debug Mode</button>
                        <button id="btn-reset-error" class="btn btn-reset hidden" onclick="resetError()">Reset Error</button>
                        <button class="btn btn-debug" onclick="checkForUpdate()">Check for Updates</button>
                        <button id="btn-fast-ota" class="btn btn-debug" onclick="toggleFastOta()">Fast OTA: Off</button>
                    </div>
                    <div id="debug-controls" class="hidden">
                        <h4>Manual Relay Control</h4>
                        <div class="relay-ctrl-grid">
                            <div class="relay-ctrl">
                                <span>Auger</span>
                                <button class="btn-sm btn-on" onclick="setRelay('auger',true)">ON</button>
                                <button class="btn-sm btn-off" onclick="setRelay('auger',false)">OFF</button>
                            </div>
                            <div class="relay-ctrl">
                                <span>Fan</span>
                                <button class="btn-sm btn-on" onclick="setRelay('fan',true)">ON</button>
                                <button class="btn-sm btn-off" onclick="setRelay('fan',false)">OFF</button>
                            </div>
                            <div class="relay-ctrl">
                                <span>Igniter</span>
                                <button class="btn-sm btn-on" onclick="setRelay('igniter',true)">ON</button>
                                <button class="btn-sm btn-off" onclick="setRelay('igniter',false)">OFF</button>
                            </div>
                        </div>
                        <h4>Temperature Override</h4>
                        <div class="override-row">
                            <input type="number" id="temp-override" min="0" max="600" value="225" placeholder="&deg;F">
                            <button class="btn btn-apply" onclick="setTempOverride()">Set</button>
                            <button class="btn btn-stop" onclick="clearTempOverride()">Clear</button>
                        </div>
                    </div>
                </div>
            </section>
        </main>

        <footer>GunderGrill <span id="fw-footer-version">v1.1.0</span></footer>
    </div>

    <!-- Toast container -->
    <div id="toasts"></div>

    <script src="script.js"></script>
</body>
</html>
)rawliteral";

const char web_style_css[] PROGMEM = R"rawliteral(
/* GunderGrill Controller */
:root {
  --fire: #ff6b35;
  --fire-dim: #cc5529;
  --green: #2ecc71;
  --green-dim: #27ae60;
  --red: #e74c3c;
  --red-dim: #c0392b;
  --blue: #3498db;
  --yellow: #f1c40f;
  --bg: #111;
  --surface: #1e1e1e;
  --surface2: #292929;
  --border: #333;
  --text: #eee;
  --text2: #999;
  --radius: 10px;
}
*, *::before, *::after { margin:0; padding:0; box-sizing:border-box; }
body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', system-ui, sans-serif;
  background: var(--bg);
  color: var(--text);
  line-height: 1.5;
  -webkit-font-smoothing: antialiased;
}
.container { max-width: 600px; margin: 0 auto; padding: 0 16px 24px; }

/* Header */
.header {
  display: flex; justify-content: space-between; align-items: center;
  padding: 16px 0; border-bottom: 1px solid var(--border); margin-bottom: 20px;
}
.header h1 { font-size: 20px; font-weight: 700; letter-spacing: -0.5px; }
.connection-bar { display: flex; align-items: center; gap: 6px; margin-top: 4px; }
.conn-dot {
  width: 8px; height: 8px; border-radius: 50%;
  background: var(--red); display: inline-block;
}
.conn-dot.ok { background: var(--green); }
.conn-label { font-size: 11px; color: var(--text2); margin-right: 8px; }

/* State Badge */
.state-badge {
  padding: 6px 16px; border-radius: 20px; font-size: 13px; font-weight: 700;
  text-transform: uppercase; letter-spacing: 1px;
  background: var(--surface2); color: var(--text2); border: 1px solid var(--border);
}
.state-badge.running { background: rgba(46,204,113,.15); color: var(--green); border-color: var(--green); }
.state-badge.startup { background: rgba(255,107,53,.15); color: var(--fire); border-color: var(--fire); }
.state-badge.cooldown { background: rgba(52,152,219,.15); color: var(--blue); border-color: var(--blue); }
.state-badge.error { background: rgba(231,76,60,.15); color: var(--red); border-color: var(--red); animation: pulse-badge 1.5s infinite; }
.state-badge.shutdown { background: rgba(241,196,15,.15); color: var(--yellow); border-color: var(--yellow); }
@keyframes pulse-badge { 0%,100%{opacity:1} 50%{opacity:.6} }

/* Temperature Hero */
.temp-hero {
  display: flex; gap: 20px; justify-content: center;
  margin-bottom: 20px; padding: 24px 0;
}
.temp-block { display: flex; flex-direction: column; align-items: center; flex: 1; }
.temp-ring {
  position: relative; width: 160px; height: 160px;
  display: flex; align-items: center; justify-content: center;
}
.temp-ring svg { position: absolute; inset: 0; width: 100%; height: 100%; transform: rotate(-90deg); }
.ring-bg { fill: none; stroke: var(--surface2); stroke-width: 6; }
.ring-fill {
  fill: none; stroke: var(--fire); stroke-width: 6;
  stroke-linecap: round; stroke-dasharray: 326.7; stroke-dashoffset: 326.7;
  transition: stroke-dashoffset 1s ease, stroke .5s;
}
.temp-value { position: relative; text-align: center; z-index: 1; }
.temp-value span:first-child { font-size: 42px; font-weight: 700; color: var(--fire); }
.temp-target .temp-value span:first-child { color: var(--text); }
.temp-unit { font-size: 18px; color: var(--text2); }
.temp-label {
  margin-top: 8px; font-size: 12px; text-transform: uppercase;
  letter-spacing: 2px; color: var(--text2);
}

/* Temperature Graph */
.graph-card { padding-bottom: 12px; }
#temp-graph {
  width: 100%; height: 200px;
  display: block; border-radius: 6px;
  background: var(--bg);
}
.graph-legend {
  display: flex; gap: 16px; justify-content: center;
  margin-top: 8px; font-size: 11px; color: var(--text2);
}
.legend-item { display: flex; align-items: center; gap: 4px; }
.legend-swatch {
  width: 14px; height: 3px; border-radius: 2px; display: inline-block;
}
.legend-swatch.temp { background: var(--fire); }
.legend-swatch.setpoint { background: var(--red); opacity: .7; }
.legend-swatch.event { background: var(--text2); opacity: .5; width: 1px; height: 10px; border-left: 1px dashed var(--text2); }

/* Cards */
.card {
  background: var(--surface); border: 1px solid var(--border);
  border-radius: var(--radius); padding: 16px; margin-bottom: 16px;
}
.card h3 {
  font-size: 12px; text-transform: uppercase; letter-spacing: 1.5px;
  color: var(--text2); margin-bottom: 12px;
}

/* Controls */
.control-row { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; margin-bottom: 16px; }
.btn {
  padding: 12px; border: none; border-radius: 8px; font-size: 14px;
  font-weight: 700; cursor: pointer; transition: all .15s;
  text-transform: uppercase; letter-spacing: .5px;
}
.btn:active:not(:disabled) { transform: scale(.97); }
.btn:disabled { opacity: .35; cursor: not-allowed; }
.btn-start { background: var(--green); color: #fff; }
.btn-start:hover:not(:disabled) { background: var(--green-dim); }
.btn-stop { background: var(--blue); color: #fff; }
.btn-stop:hover:not(:disabled) { background: #2980b9; }
.btn-shutdown { background: var(--red); color: #fff; }
.btn-shutdown:hover:not(:disabled) { background: var(--red-dim); }
.btn-apply { background: var(--fire); color: #fff; min-width: 60px; }
.btn-apply:hover:not(:disabled) { background: var(--fire-dim); }
.debug-btn-row { display: flex; flex-wrap: wrap; gap: 10px; margin-bottom: 16px; }
.btn-debug { background: var(--surface2); color: var(--yellow); border: 1px solid var(--yellow); flex: 1; }
.btn-debug.active { background: rgba(231,76,60,.15); color: var(--red); border-color: var(--red); }
.btn-reset { background: var(--red); color: #fff; flex: 1; animation: pulse-badge 1.5s infinite; }
.btn-reset:hover:not(:disabled) { background: var(--red-dim); }

/* Setpoint Row */
.setpoint-row { display: flex; align-items: center; gap: 8px; }
.btn-adj {
  width: 40px; height: 40px; border: 1px solid var(--border); border-radius: 8px;
  background: var(--surface2); color: var(--text); font-size: 20px; font-weight: 700;
  cursor: pointer; display: flex; align-items: center; justify-content: center;
  transition: all .15s;
}
.btn-adj:hover { border-color: var(--fire); color: var(--fire); }
#setpoint-input {
  width: 80px; padding: 8px; text-align: center; font-size: 18px; font-weight: 700;
  border: 1px solid var(--border); border-radius: 8px;
  background: var(--bg); color: var(--text); outline: none;
}
#setpoint-input:focus { border-color: var(--fire); }
.sp-unit { font-size: 14px; color: var(--text2); }

/* Relay & Info Row */
.info-row { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
.relay-grid { display: flex; gap: 8px; }
.relay {
  flex: 1; display: flex; flex-direction: column; align-items: center; gap: 6px;
  padding: 10px 0; background: var(--bg); border-radius: 8px;
}
.relay span { font-size: 11px; text-transform: uppercase; color: var(--text2); letter-spacing: .5px; }
.relay-led {
  width: 20px; height: 20px; border-radius: 50%;
  background: #333; border: 2px solid #444; transition: all .3s;
}
.relay.on .relay-led { background: var(--green); border-color: var(--green); box-shadow: 0 0 12px rgba(46,204,113,.5); }
.info-list { display: flex; flex-direction: column; gap: 6px; }
.info-row-item {
  display: flex; justify-content: space-between; padding: 6px 10px;
  background: var(--bg); border-radius: 6px; font-size: 13px;
}
.info-row-item span:first-child { color: var(--text2); }
.info-row-item span:last-child { font-weight: 600; }
#update-version { color: var(--green); font-weight: 700; }
#update-row .btn-sm { margin-left: 6px; padding: 3px 10px; font-size: 11px; }

/* Debug */
.debug-card { border-color: #444; }
.debug-card h3 { color: var(--text2); }
.debug-header { cursor: pointer; user-select: none; display: flex; justify-content: space-between; }
.debug-panel { margin-top: 12px; }
.debug-panel h4 {
  font-size: 11px; text-transform: uppercase; letter-spacing: 1px;
  color: var(--text2); margin: 16px 0 8px;
}
.debug-warn {
  background: rgba(241,196,15,.08); border-left: 3px solid var(--yellow);
  padding: 10px 12px; margin-bottom: 12px; font-size: 13px; color: var(--yellow);
  border-radius: 0 6px 6px 0;
}
.relay-ctrl-grid { display: flex; flex-direction: column; gap: 6px; }
.relay-ctrl {
  display: flex; align-items: center; gap: 8px; padding: 8px 12px;
  background: var(--bg); border-radius: 6px;
}
.relay-ctrl span { flex: 1; font-size: 13px; font-weight: 600; }
.btn-sm {
  padding: 6px 14px; border: none; border-radius: 6px; font-size: 12px;
  font-weight: 700; cursor: pointer; transition: all .15s;
}
.btn-on { background: var(--green); color: #fff; }
.btn-off { background: #555; color: #ccc; }
.override-row { display: flex; gap: 8px; align-items: center; }
.override-row input {
  flex: 1; padding: 8px; border: 1px solid var(--border); border-radius: 6px;
  background: var(--bg); color: var(--text); font-size: 14px; outline: none;
}

/* Toast */
#toasts { position: fixed; bottom: 20px; right: 20px; display: flex; flex-direction: column-reverse; gap: 8px; z-index: 100; }
.toast {
  padding: 10px 16px; border-radius: 8px; font-size: 13px; font-weight: 600;
  color: #fff; animation: toast-in .3s ease; min-width: 200px;
  box-shadow: 0 4px 12px rgba(0,0,0,.4);
}
.toast.ok { background: var(--green-dim); }
.toast.err { background: var(--red-dim); }
.toast.info { background: #2c3e50; }
.toast.out { animation: toast-out .3s ease forwards; }
@keyframes toast-in { from { opacity:0; transform:translateY(10px); } to { opacity:1; transform:translateY(0); } }
@keyframes toast-out { to { opacity:0; transform:translateY(10px); } }

.hidden { display: none !important; }

/* Footer */
footer { text-align: center; padding: 16px 0; font-size: 12px; color: #555; border-top: 1px solid var(--border); }

/* Mobile */
@media (max-width: 480px) {
  .temp-ring { width: 130px; height: 130px; }
  .temp-value span:first-child { font-size: 34px; }
  .info-row { grid-template-columns: 1fr; }
  .control-row { grid-template-columns: 1fr 1fr; }
  .control-row .btn-shutdown { grid-column: 1 / -1; }
}
)rawliteral";

const char web_script_js[] PROGMEM = R"rawliteral(
/* GunderGrill Controller */
const API = '/api';
const POLL_MS = 2000;
let apiOk = false;
let debugActive = false;

// --- Graph State ---
let graphSamples = [];   // {t, c, s, st} from backend
let graphEvents = [];    // {t, st} from backend
let deviceNow = 0;       // device uptime (seconds) at last history fetch
let localAtFetch = 0;    // Date.now() when history was fetched
let graphInited = false;

// --- Init ---
document.addEventListener('DOMContentLoaded', () => {
  fetchHistory();
  updateStatus();
  setInterval(updateStatus, POLL_MS);
  window.addEventListener('resize', drawGraph);
  checkVersionStatus();
});

// --- Toast Notifications ---
function toast(msg, type) {
  const el = document.createElement('div');
  el.className = 'toast ' + (type || 'info');
  el.textContent = msg;
  document.getElementById('toasts').appendChild(el);
  setTimeout(() => { el.classList.add('out'); setTimeout(() => el.remove(), 300); }, 3000);
}

// --- API Helper (uses FormData - matches ESPAsyncWebServer) ---
async function post(endpoint, params) {
  try {
    const fd = new FormData();
    if (params) Object.entries(params).forEach(([k, v]) => fd.append(k, String(v)));
    const r = await fetch(API + endpoint, { method: 'POST', body: fd });
    if (!r.ok) throw new Error(r.status);
    return await r.json();
  } catch (e) {
    console.error('API POST ' + endpoint, e);
    toast('Request failed', 'err');
    return null;
  }
}

// --- Status Polling ---
async function updateStatus() {
  try {
    const r = await fetch(API + '/status');
    if (!r.ok) throw new Error(r.status);
    const s = await r.json();
    apiOk = true;
    updateUI(s);
  } catch (e) {
    apiOk = false;
    setConn('conn-api', false);
  }
}

function updateUI(s) {
  // Connection indicators
  setConn('conn-wifi', true);
  setConn('conn-api', true);

  // Temperature
  const temp = s.temp;
  const sp = s.setpoint;
  const tempEl = document.getElementById('current-temp');
  const spEl = document.getElementById('setpoint-temp');

  if (temp < -100 || temp > 1000) {
    tempEl.textContent = '--';
  } else {
    tempEl.textContent = temp.toFixed(0);
  }
  spEl.textContent = sp.toFixed(0);

  // Temperature ring (0-500degF range mapped to SVG circle)
  const ring = document.getElementById('ring-fill');
  const pct = Math.max(0, Math.min(1, temp / 500));
  const circumference = 326.7;
  ring.style.strokeDashoffset = circumference * (1 - pct);

  // Ring color: blue < 150, orange 150-350, red > 350
  if (temp < 0 || temp > 1000) {
    ring.style.stroke = '#555';
  } else if (temp < 150) {
    ring.style.stroke = '#3498db';
  } else if (temp > 400) {
    ring.style.stroke = '#e74c3c';
  } else {
    ring.style.stroke = '#ff6b35';
  }

  // State badge
  const badge = document.getElementById('state-badge');
  const state = s.state || 'Idle';
  badge.textContent = state;
  badge.className = 'state-badge ' + state.toLowerCase();

  // Relays
  setRelayStat('relay-auger', s.auger);
  setRelayStat('relay-fan', s.fan);
  setRelayStat('relay-igniter', s.igniter);

  // Info
  if (s.runtime !== undefined) {
    const totalSec = Math.floor(s.runtime / 1000);
    const m = Math.floor(totalSec / 60);
    const sec = totalSec % 60;
    document.getElementById('runtime').textContent = m + ':' + String(sec).padStart(2, '0');
  }
  document.getElementById('error-count').textContent = s.errors || 0;

  // Firmware version
  if (s.version) {
    document.getElementById('fw-version').textContent = 'v' + s.version;
    document.getElementById('fw-footer-version').textContent = 'v' + s.version;
  }

  // Buttons
  const running = state !== 'Idle' && state !== 'Shutdown' && state !== 'Error';
  document.getElementById('btn-start').disabled = running;
  document.getElementById('btn-stop').disabled = !running;

  // Show/hide reset error button
  const resetBtn = document.getElementById('btn-reset-error');
  if (state === 'Error') {
    resetBtn.classList.remove('hidden');
  } else {
    resetBtn.classList.add('hidden');
  }

  // Sync setpoint input (only if not focused)
  const spInput = document.getElementById('setpoint-input');
  if (document.activeElement !== spInput) {
    spInput.value = sp.toFixed(0);
  }

  // Append to graph
  appendGraphPoint(s);
}

// --- Temperature Graph ---
const STATE_NAMES = ['Idle','Startup','Running','Cooldown','Shutdown','Error'];
const STATE_COLORS = ['#555','#ff6b35','#2ecc71','#3498db','#f1c40f','#e74c3c'];

async function fetchHistory() {
  try {
    const r = await fetch(API + '/history');
    if (!r.ok) return;
    const d = await r.json();
    graphSamples = d.samples || [];
    graphEvents = d.events || [];
    deviceNow = d.now || 0;
    localAtFetch = Date.now();
    graphInited = true;
    drawGraph();
  } catch (e) { console.error('fetchHistory', e); }
}

function appendGraphPoint(s) {
  if (!graphInited) return;
  // Estimate current device uptime from drift since fetch
  var estNow = deviceNow + (Date.now() - localAtFetch) / 1000;
  var stIdx = STATE_NAMES.indexOf(s.state);
  if (stIdx < 0) stIdx = 0;
  graphSamples.push({t: Math.round(estNow), c: s.temp, s: s.setpoint, st: stIdx});
  // Detect state changes vs last sample
  if (graphSamples.length >= 2) {
    var prev = graphSamples[graphSamples.length - 2];
    if (prev.st !== stIdx) {
      graphEvents.push({t: Math.round(estNow), st: stIdx});
    }
  }
  // Trim to ~1 hour
  var cutoff = estNow - 3660;
  while (graphSamples.length > 1 && graphSamples[0].t < cutoff) graphSamples.shift();
  while (graphEvents.length > 0 && graphEvents[0].t < cutoff) graphEvents.shift();
  drawGraph();
}

function drawGraph() {
  var canvas = document.getElementById('temp-graph');
  if (!canvas) return;
  var dpr = window.devicePixelRatio || 1;
  var rect = canvas.getBoundingClientRect();
  var W = rect.width;
  var H = rect.height;
  canvas.width = W * dpr;
  canvas.height = H * dpr;
  var ctx = canvas.getContext('2d');
  ctx.scale(dpr, dpr);

  // Clear
  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  if (graphSamples.length < 2) {
    ctx.fillStyle = '#555';
    ctx.font = '13px -apple-system, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('Collecting data\u2026', W / 2, H / 2);
    return;
  }

  // Layout: padding for axis labels
  var padL = 42, padR = 12, padT = 14, padB = 24;
  var gW = W - padL - padR;
  var gH = H - padT - padB;

  // Time range
  var tMin = graphSamples[0].t;
  var tMax = graphSamples[graphSamples.length - 1].t;
  if (tMax - tMin < 30) tMax = tMin + 30; // at least 30s window

  // Temp range (auto-scale with padding)
  var cMin = Infinity, cMax = -Infinity;
  for (var i = 0; i < graphSamples.length; i++) {
    var p = graphSamples[i];
    if (p.c > -100 && p.c < 1000) {
      if (p.c < cMin) cMin = p.c;
      if (p.c > cMax) cMax = p.c;
    }
    if (p.s < cMin) cMin = p.s;
    if (p.s > cMax) cMax = p.s;
  }
  var tempPad = Math.max(10, (cMax - cMin) * 0.15);
  cMin = Math.floor((cMin - tempPad) / 10) * 10;
  cMax = Math.ceil((cMax + tempPad) / 10) * 10;
  if (cMax - cMin < 20) { cMin -= 10; cMax += 10; }

  function tx(t) { return padL + (t - tMin) / (tMax - tMin) * gW; }
  function ty(temp) { return padT + (1 - (temp - cMin) / (cMax - cMin)) * gH; }

  // State background bands
  if (graphSamples.length > 1) {
    for (var i = 0; i < graphSamples.length - 1; i++) {
      var p = graphSamples[i];
      var n = graphSamples[i + 1];
      var x0 = tx(p.t), x1 = tx(n.t);
      var col = STATE_COLORS[p.st] || '#555';
      ctx.fillStyle = col.replace(')', ',0.06)').replace('rgb', 'rgba').replace('#', '');
      // Use hex alpha approach
      ctx.fillStyle = hexAlpha(col, 0.06);
      ctx.fillRect(x0, padT, x1 - x0, gH);
    }
  }

  // Grid lines
  ctx.strokeStyle = '#222';
  ctx.lineWidth = 0.5;
  // Y grid
  var yStep = niceStep(cMax - cMin, 5);
  for (var v = Math.ceil(cMin / yStep) * yStep; v <= cMax; v += yStep) {
    var y = ty(v);
    ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(W - padR, y); ctx.stroke();
    ctx.fillStyle = '#666';
    ctx.font = '10px -apple-system, sans-serif';
    ctx.textAlign = 'right';
    ctx.fillText(v + '\u00B0', padL - 4, y + 3);
  }
  // X grid
  var tRange = tMax - tMin;
  var xStep = niceTimeStep(tRange);
  ctx.textAlign = 'center';
  for (var t = Math.ceil(tMin / xStep) * xStep; t <= tMax; t += xStep) {
    var x = tx(t);
    ctx.beginPath(); ctx.moveTo(x, padT); ctx.lineTo(x, padT + gH); ctx.stroke();
    ctx.fillStyle = '#666';
    ctx.font = '10px -apple-system, sans-serif';
    var ago = tMax - t;
    ctx.fillText(ago < 5 ? 'now' : fmtAgo(ago), x, H - 4);
  }

  // Event lines (state changes)
  for (var i = 0; i < graphEvents.length; i++) {
    var e = graphEvents[i];
    if (e.t < tMin || e.t > tMax) continue;
    var x = tx(e.t);
    ctx.strokeStyle = STATE_COLORS[e.st] || '#666';
    ctx.globalAlpha = 0.4;
    ctx.lineWidth = 1;
    ctx.setLineDash([3, 3]);
    ctx.beginPath(); ctx.moveTo(x, padT); ctx.lineTo(x, padT + gH); ctx.stroke();
    ctx.setLineDash([]);
    ctx.globalAlpha = 0.7;
    ctx.fillStyle = STATE_COLORS[e.st] || '#666';
    ctx.font = '9px -apple-system, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText(STATE_NAMES[e.st] || '', x, padT - 2);
    ctx.globalAlpha = 1;
  }

  // Setpoint line (dashed)
  ctx.strokeStyle = '#e74c3c';
  ctx.globalAlpha = 0.6;
  ctx.lineWidth = 1.5;
  ctx.setLineDash([6, 4]);
  ctx.beginPath();
  for (var i = 0; i < graphSamples.length; i++) {
    var p = graphSamples[i];
    var x = tx(p.t), y = ty(p.s);
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.stroke();
  ctx.setLineDash([]);
  ctx.globalAlpha = 1;

  // Temperature line
  ctx.strokeStyle = '#ff6b35';
  ctx.lineWidth = 2;
  ctx.lineJoin = 'round';
  ctx.beginPath();
  var started = false;
  for (var i = 0; i < graphSamples.length; i++) {
    var p = graphSamples[i];
    if (p.c < -100 || p.c > 1000) { started = false; continue; }
    var x = tx(p.t), y = ty(p.c);
    if (!started) { ctx.moveTo(x, y); started = true; } else ctx.lineTo(x, y);
  }
  ctx.stroke();

  // Current value label at end of temperature line
  if (graphSamples.length > 0) {
    var last = graphSamples[graphSamples.length - 1];
    if (last.c > -100 && last.c < 1000) {
      var lx = tx(last.t), ly = ty(last.c);
      ctx.fillStyle = '#ff6b35';
      ctx.font = 'bold 11px -apple-system, sans-serif';
      ctx.textAlign = 'left';
      ctx.fillText(last.c.toFixed(0) + '\u00B0', lx + 4, ly + 4);
    }
  }
}

function hexAlpha(hex, a) {
  if (hex[0] === '#') {
    var r = parseInt(hex.slice(1,3),16);
    var g = parseInt(hex.slice(3,5),16);
    var b = parseInt(hex.slice(5,7),16);
    return 'rgba('+r+','+g+','+b+','+a+')';
  }
  return hex;
}

function niceStep(range, maxTicks) {
  var rough = range / maxTicks;
  var mag = Math.pow(10, Math.floor(Math.log10(rough)));
  var norm = rough / mag;
  var step;
  if (norm <= 1.5) step = 1;
  else if (norm <= 3) step = 2;
  else if (norm <= 7) step = 5;
  else step = 10;
  return step * mag;
}

function niceTimeStep(rangeSeconds) {
  if (rangeSeconds < 120) return 30;
  if (rangeSeconds < 600) return 60;
  if (rangeSeconds < 1800) return 300;
  if (rangeSeconds < 3600) return 600;
  return 900;
}

function fmtAgo(sec) {
  if (sec < 60) return sec.toFixed(0) + 's';
  var m = Math.round(sec / 60);
  return m + 'm';
}

function setConn(id, ok) {
  const el = document.getElementById(id);
  if (ok) el.classList.add('ok'); else el.classList.remove('ok');
}

function setRelayStat(id, on) {
  const el = document.getElementById(id);
  if (on) el.classList.add('on'); else el.classList.remove('on');
}

// --- Controls ---
async function startSmoking() {
  const temp = parseFloat(document.getElementById('setpoint-input').value);
  const r = await post('/start', { temp });
  if (r) toast('Smoker started at ' + temp + '\u00B0F', 'ok');
}

async function stopSmoking() {
  const r = await post('/stop');
  if (r) toast('Stopping...', 'ok');
}

async function doShutdown() {
  if (!confirm('Shutdown the smoker?')) return;
  const r = await post('/shutdown');
  if (r) toast('Shutting down', 'info');
}

function adjSetpoint(delta) {
  const el = document.getElementById('setpoint-input');
  const v = Math.max(150, Math.min(500, parseInt(el.value) + delta));
  el.value = v;
  el.focus();
}

async function applySetpoint() {
  const v = parseInt(document.getElementById('setpoint-input').value);
  if (v < 150 || v > 500) { toast('150-500\u00B0F range', 'err'); return; }
  const r = await post('/setpoint', { temp: v });
  if (r) toast('Setpoint: ' + v + '\u00B0F', 'ok');
}

// --- Debug ---
function toggleDebug() {
  const panel = document.getElementById('debug-panel');
  const arrow = document.getElementById('debug-arrow');
  panel.classList.toggle('hidden');
  arrow.innerHTML = panel.classList.contains('hidden') ? '&#9662;' : '&#9652;';
}

async function toggleDebugMode() {
  debugActive = !debugActive;
  const r = await post('/debug/mode', { enabled: debugActive });
  if (!r) { debugActive = !debugActive; return; }
  const btn = document.getElementById('btn-debug');
  const ctrl = document.getElementById('debug-controls');
  if (debugActive) {
    btn.textContent = 'Disable Debug Mode';
    btn.classList.add('active');
    ctrl.classList.remove('hidden');
    toast('Debug mode ON', 'info');
  } else {
    btn.textContent = 'Enable Debug Mode';
    btn.classList.remove('active');
    ctrl.classList.add('hidden');
    toast('Debug mode OFF', 'ok');
  }
}

async function setRelay(relay, state) {
  await post('/debug/relay', { relay, state });
}

async function setTempOverride() {
  const v = document.getElementById('temp-override').value;
  const r = await post('/debug/temp', { temp: v });
  if (r) toast('Override: ' + v + '\u00B0F', 'info');
}

async function clearTempOverride() {
  try {
    await fetch(API + '/debug/temp', { method: 'DELETE' });
    toast('Override cleared', 'ok');
  } catch (e) { toast('Failed', 'err'); }
}

async function resetError() {
  const r = await post('/debug/reset');
  if (r) toast('Error state cleared', 'ok');
}

// --- Firmware Updates ---
async function checkVersionStatus() {
  try {
    const r = await fetch(API + '/version');
    if (!r.ok) return;
    const v = await r.json();
    if (v.updateAvailable) showUpdateBanner(v.latest);
    updateFastOtaBtn(v.fastCheck);
  } catch (e) { /* ignore on page load */ }
}

async function checkForUpdate() {
  toast('Checking for updates...', 'info');
  const r = await post('/update/check');
  if (!r) return;
  // Check is deferred to main loop — poll /api/version for result
  for (let i = 0; i < 10; i++) {
    await new Promise(ok => setTimeout(ok, 2000));
    try {
      const v = await (await fetch(API + '/version')).json();
      if (!v.checkComplete) continue;
      if (v.checkResult === 'update_available') {
        toast('Update available: v' + v.latest, 'info');
        showUpdateBanner(v.latest);
      } else if (v.checkResult === 'no_update') {
        toast('Firmware is up to date (v' + v.current + ')', 'ok');
      } else {
        toast('Update check failed: ' + (v.lastError || 'unknown'), 'err');
      }
      return;
    } catch (e) { /* retry */ }
  }
  toast('Update check timed out', 'err');
}

function showUpdateBanner(version) {
  document.getElementById('update-version').textContent = 'v' + version;
  document.getElementById('update-row').classList.remove('hidden');
}

async function applyUpdate() {
  if (!confirm('Install firmware update? The device will reboot.')) return;
  const r = await post('/update/apply');
  if (r && r.ok) toast('Installing update, device will reboot...', 'info');
}

let fastOtaActive = false;
async function toggleFastOta() {
  fastOtaActive = !fastOtaActive;
  const r = await post('/update/fast', { enabled: fastOtaActive });
  if (!r) { fastOtaActive = !fastOtaActive; return; }
  updateFastOtaBtn(fastOtaActive);
  toast('OTA check interval: ' + (fastOtaActive ? '60s' : '6hrs'), 'info');
}

function updateFastOtaBtn(active) {
  fastOtaActive = !!active;
  const btn = document.getElementById('btn-fast-ota');
  if (!btn) return;
  btn.textContent = 'Fast OTA: ' + (active ? 'On' : 'Off');
  if (active) btn.classList.add('active'); else btn.classList.remove('active');
}
)rawliteral";

#endif // WEB_CONTENT_H
