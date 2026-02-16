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

        <nav class="tab-bar">
            <button class="tab-btn active" onclick="switchTab('dashboard')">Dashboard</button>
            <button class="tab-btn" onclick="switchTab('pid')">PID Visualizer</button>
        </nav>

        <main id="tab-dashboard" class="tab-panel active">
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
                            <span class="temp-unit">Â°F</span>
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
                            <span class="temp-unit">Â°F</span>
                        </div>
                    </div>
                    <div class="temp-label">Target</div>
                </div>
            </section>

            <!-- Temperature Graph -->
            <section class="card graph-card">
                <div class="graph-header">
                    <h3>Temperature History</h3>
                    <div class="graph-range-btns" id="graph-range-btns">
                        <button onclick="setGraphRange(3600)">1h</button>
                        <button class="active" onclick="setGraphRange(14400)">4h</button>
                        <button onclick="setGraphRange(28800)">8h</button>
                        <button onclick="setGraphRange(43200)">12h</button>
                        <button onclick="setGraphRange(0)">All</button>
                    </div>
                </div>
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
                    <span class="sp-unit">Â°F</span>
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
                            <input type="number" id="temp-override" min="0" max="600" value="225" placeholder="Â°F">
                            <button class="btn btn-apply" onclick="setTempOverride()">Set</button>
                            <button class="btn btn-stop" onclick="clearTempOverride()">Clear</button>
                        </div>
                    </div>
                </div>
            </section>
        </main>

        <div id="tab-pid" class="tab-panel" style="display:none;">
            <!-- Animated Smoker Cross-Section -->
            <section class="card pid-scene-card">
                <h3>Smoker Cross-Section</h3>
                <canvas id="smoker-scene"></canvas>
            </section>

            <!-- PID Brain: Three Visual Metaphors -->
            <div class="pid-brain-row">
                <section class="card pid-brain-card">
                    <canvas id="pid-spring"></canvas>
                </section>
                <section class="card pid-brain-card">
                    <canvas id="pid-bucket"></canvas>
                </section>
                <section class="card pid-brain-card">
                    <canvas id="pid-speedo"></canvas>
                </section>
            </div>

            <!-- Feedback Loop Band -->
            <section class="card pid-feedback-card">
                <canvas id="feedback-loop"></canvas>
            </section>

            <!-- PID Time Series Chart -->
            <section class="card pid-chart-card">
                <div class="graph-header">
                    <h3>PID History</h3>
                    <div class="graph-range-btns" id="pid-range-btns">
                        <button class="active" onclick="setPidRange(120)">2m</button>
                        <button onclick="setPidRange(300)">5m</button>
                        <button onclick="setPidRange(600)">10m</button>
                    </div>
                </div>
                <canvas id="pid-history-chart"></canvas>
                <div class="graph-legend">
                    <span class="legend-item"><span class="legend-swatch" style="background:#3498db"></span>P</span>
                    <span class="legend-item"><span class="legend-swatch" style="background:#2ecc71"></span>I</span>
                    <span class="legend-item"><span class="legend-swatch" style="background:#ff6b35"></span>D</span>
                    <span class="legend-item"><span class="legend-swatch" style="background:#fff"></span>Output</span>
                    <span class="legend-item"><span class="legend-swatch" style="background:#e74c3c;opacity:.6"></span>Temp</span>
                </div>
            </section>
        </div>

        <footer>GunderGrill <span id="fw-footer-version">v1.2.0</span></footer>
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
.graph-header {
  display: flex; justify-content: space-between; align-items: center;
  margin-bottom: 12px; flex-wrap: wrap; gap: 8px;
}
.graph-header h3 { margin-bottom: 0; min-width: 0; }
.graph-range-btns {
  display: flex; gap: 4px; flex-shrink: 0;
}
.graph-range-btns button {
  padding: 4px 10px; border: 1px solid var(--border); border-radius: 6px;
  background: var(--bg); color: var(--text2); font-size: 11px; font-weight: 600;
  cursor: pointer; transition: all .15s; white-space: nowrap;
}
.graph-range-btns button:hover { border-color: var(--fire); color: var(--fire); }
.graph-range-btns button.active {
  background: var(--fire); color: #fff; border-color: var(--fire);
}
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
.info-row { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; min-width: 0; }
.info-row > .card { min-width: 0; overflow: hidden; }
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

/* Tab Navigation */
.tab-bar {
  display: flex; gap: 0; margin-bottom: 20px;
  border-bottom: 2px solid var(--border);
}
.tab-btn {
  flex: 1; padding: 10px 0; border: none; background: none;
  color: var(--text2); font-size: 13px; font-weight: 700;
  text-transform: uppercase; letter-spacing: 1.5px;
  cursor: pointer; transition: all .2s;
  border-bottom: 2px solid transparent; margin-bottom: -2px;
}
.tab-btn:hover { color: var(--text); }
.tab-btn.active { color: var(--fire); border-bottom-color: var(--fire); }
.tab-panel { display: block; }
.tab-panel:not(.active) { display: none; }

/* PID Visualizer â€” Animated Scene */
.pid-scene-card { padding-bottom: 12px; }
#smoker-scene {
  width: 100%; height: 280px; display: block;
  border-radius: 6px; background: var(--bg);
}

/* PID Brain â€” Three Metaphor Canvases */
.pid-brain-row {
  display: grid; grid-template-columns: 1fr 1fr 1fr;
  gap: 12px; margin-bottom: 16px;
}
.pid-brain-card {
  min-width: 0; overflow: hidden;
  padding: 8px !important;
}
#pid-spring, #pid-bucket, #pid-speedo {
  width: 100%; height: 140px; display: block;
  border-radius: 6px; background: var(--bg);
}

/* Feedback Loop Band */
.pid-feedback-card { padding: 10px 16px; }
#feedback-loop {
  width: 100%; height: 90px; display: block;
  border-radius: 6px; background: var(--bg);
}

/* PID Time-Series Chart */
.pid-chart-card { padding-bottom: 12px; }
#pid-history-chart {
  width: 100%; height: 200px; display: block;
  border-radius: 6px; background: var(--bg);
}
#pid-range-btns button {
  padding: 4px 10px; border: 1px solid var(--border); border-radius: 6px;
  background: var(--bg); color: var(--text2); font-size: 11px; font-weight: 600;
  cursor: pointer; transition: all .15s; white-space: nowrap;
}
#pid-range-btns button:hover { border-color: var(--fire); color: var(--fire); }
#pid-range-btns button.active {
  background: var(--fire); color: #fff; border-color: var(--fire);
}

/* Mobile */
@media (max-width: 480px) {
  .temp-ring { width: 130px; height: 130px; }
  .temp-value span:first-child { font-size: 34px; }
  .info-row { grid-template-columns: 1fr; }
  .control-row { grid-template-columns: 1fr 1fr; }
  .control-row .btn-shutdown { grid-column: 1 / -1; }
  .pid-brain-row { grid-template-columns: 1fr; }
  #pid-spring, #pid-bucket, #pid-speedo { height: 120px; }
  #smoker-scene { height: 220px; }
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
let graphRangeSec = 14400; // 0 = all data

// --- Init ---
document.addEventListener('DOMContentLoaded', function() {
  fetchHistory();
  updateStatus();
  setInterval(updateStatus, POLL_MS);
  window.addEventListener('resize', function() {
    drawGraph();
    if (document.getElementById('tab-pid') && document.getElementById('tab-pid').classList.contains('active')) {
      drawPidChart();
    }
  });
  checkVersionStatus();
});

// --- Toast Notifications ---
function toast(msg, type) {
  var el = document.createElement('div');
  el.className = 'toast ' + (type || 'info');
  el.textContent = msg;
  document.getElementById('toasts').appendChild(el);
  setTimeout(function() { el.classList.add('out'); setTimeout(function() { el.remove(); }, 300); }, 3000);
}

// --- API Helper (uses FormData - matches ESPAsyncWebServer) ---
async function post(endpoint, params) {
  try {
    var fd = new FormData();
    if (params) Object.entries(params).forEach(function(kv) { fd.append(kv[0], String(kv[1])); });
    var r = await fetch(API + endpoint, { method: 'POST', body: fd });
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
    var r = await fetch(API + '/status');
    if (!r.ok) throw new Error(r.status);
    var s = await r.json();
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
  var temp = s.temp;
  var sp = s.setpoint;
  var tempEl = document.getElementById('current-temp');
  var spEl = document.getElementById('setpoint-temp');

  if (temp < -100 || temp > 1000) {
    tempEl.textContent = '--';
  } else {
    tempEl.textContent = temp.toFixed(0);
  }
  spEl.textContent = sp.toFixed(0);

  // Temperature ring (0-500 range mapped to SVG circle)
  var ring = document.getElementById('ring-fill');
  var pct = Math.max(0, Math.min(1, temp / 500));
  var circumference = 326.7;
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
  var badge = document.getElementById('state-badge');
  var state = s.state || 'Idle';
  badge.textContent = state;
  badge.className = 'state-badge ' + state.toLowerCase();

  // Relays
  setRelayStat('relay-auger', s.auger);
  setRelayStat('relay-fan', s.fan);
  setRelayStat('relay-igniter', s.igniter);

  // Info
  if (s.runtime !== undefined) {
    var totalSec = Math.floor(s.runtime / 1000);
    var m = Math.floor(totalSec / 60);
    var sec = totalSec % 60;
    document.getElementById('runtime').textContent = m + ':' + String(sec).padStart(2, '0');
  }
  document.getElementById('error-count').textContent = s.errors || 0;
  if (s.heap !== undefined) {
    var kb = (s.heap / 1024).toFixed(0);
    document.getElementById('heap-free').textContent = kb + ' KB';
  }

  // Firmware version
  if (s.version) {
    document.getElementById('fw-version').textContent = 'v' + s.version;
    document.getElementById('fw-footer-version').textContent = 'v' + s.version;
  }

  // Buttons
  var running = state !== 'Idle' && state !== 'Shutdown' && state !== 'Error';
  document.getElementById('btn-start').disabled = running;
  document.getElementById('btn-stop').disabled = !running;

  // Show/hide reset error button
  var resetBtn = document.getElementById('btn-reset-error');
  if (state === 'Error') {
    resetBtn.classList.remove('hidden');
  } else {
    resetBtn.classList.add('hidden');
  }

  // Sync setpoint input (only if not focused)
  var spInput = document.getElementById('setpoint-input');
  if (document.activeElement !== spInput) {
    spInput.value = sp.toFixed(0);
  }

  // Record PID data and update PID scene state
  recordPidSample(s);
  updatePidVisuals(s);

  // Append to graph
  appendGraphPoint(s);
}

// --- Temperature Graph ---
var STATE_NAMES = ['Idle','Startup','Running','Cooldown','Shutdown','Error','Reignite'];
var STATE_COLORS = ['#555','#ff6b35','#2ecc71','#3498db','#f1c40f','#e74c3c','#e67e22'];

async function fetchHistory() {
  try {
    var r = await fetch(API + '/history');
    if (!r.ok) return;
    var d = await r.json();
    // Compact format: samples are [time, temp*10, setpoint*10, state]
    graphSamples = (d.samples || []).map(function(a) {
      return {t: a[0], c: a[1] / 10, s: a[2] / 10, st: a[3]};
    });
    // Events are [time, state]
    graphEvents = (d.events || []).map(function(a) {
      return {t: a[0], st: a[1]};
    });
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
  // Trim to backend capacity (~24 hours)
  var cutoff = estNow - 86400;
  while (graphSamples.length > 1 && graphSamples[0].t < cutoff) graphSamples.shift();
  while (graphEvents.length > 0 && graphEvents[0].t < cutoff) graphEvents.shift();
  drawGraph();
}

function setGraphRange(sec) {
  graphRangeSec = sec;
  var btns = document.querySelectorAll('#graph-range-btns button');
  btns.forEach(function(b) { b.classList.remove('active'); });
  // Find the clicked button by matching its range
  var vals = [3600, 14400, 28800, 43200, 0];
  var idx = vals.indexOf(sec);
  if (idx >= 0 && btns[idx]) btns[idx].classList.add('active');
  drawGraph();
}

function getVisibleData() {
  if (graphSamples.length === 0) return { samples: [], events: [] };
  if (graphRangeSec === 0) return { samples: graphSamples, events: graphEvents };
  var tMax = graphSamples[graphSamples.length - 1].t;
  var tCut = tMax - graphRangeSec;
  var samples = graphSamples.filter(function(p) { return p.t >= tCut; });
  var events = graphEvents.filter(function(e) { return e.t >= tCut; });
  return { samples: samples, events: events };
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

  var vis = getVisibleData();
  var visSamples = vis.samples;
  var visEvents = vis.events;

  if (visSamples.length < 2) {
    ctx.fillStyle = '#555';
    ctx.font = '13px -apple-system, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('Collecting data\u2026', W / 2, H / 2);
    return;
  }

  // Layout: padding for axis labels
  var padL = 42, padR = 40, padT = 14, padB = 24;
  var gW = W - padL - padR;
  var gH = H - padT - padB;

  // Time range
  var tMax = visSamples[visSamples.length - 1].t;
  var tMin = (graphRangeSec > 0) ? tMax - graphRangeSec : visSamples[0].t;
  if (tMax - tMin < 30) tMax = tMin + 30; // at least 30s window

  // Temp range (auto-scale with padding)
  var cMin = Infinity, cMax = -Infinity;
  for (var i = 0; i < visSamples.length; i++) {
    var p = visSamples[i];
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
  if (visSamples.length > 1) {
    for (var i = 0; i < visSamples.length - 1; i++) {
      var p = visSamples[i];
      var n = visSamples[i + 1];
      var x0 = tx(p.t), x1 = tx(n.t);
      var col = STATE_COLORS[p.st] || '#555';
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
  var lastLabelX = -100;
  for (var i = 0; i < visEvents.length; i++) {
    var e = visEvents[i];
    if (e.t < tMin || e.t > tMax) continue;
    var x = tx(e.t);
    ctx.strokeStyle = STATE_COLORS[e.st] || '#666';
    ctx.globalAlpha = 0.4;
    ctx.lineWidth = 1;
    ctx.setLineDash([3, 3]);
    ctx.beginPath(); ctx.moveTo(x, padT); ctx.lineTo(x, padT + gH); ctx.stroke();
    ctx.setLineDash([]);
    // Only draw label if it won't overlap the previous one
    if (x - lastLabelX > 70) {
      ctx.globalAlpha = 0.7;
      ctx.fillStyle = STATE_COLORS[e.st] || '#666';
      ctx.font = '9px -apple-system, sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText(STATE_NAMES[e.st] || '', x, padT - 2);
      lastLabelX = x;
    }
    ctx.globalAlpha = 1;
  }

  // Setpoint line (dashed)
  ctx.strokeStyle = '#e74c3c';
  ctx.globalAlpha = 0.6;
  ctx.lineWidth = 1.5;
  ctx.setLineDash([6, 4]);
  ctx.beginPath();
  for (var i = 0; i < visSamples.length; i++) {
    var p = visSamples[i];
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
  for (var i = 0; i < visSamples.length; i++) {
    var p = visSamples[i];
    if (p.c < -100 || p.c > 1000) { started = false; continue; }
    var x = tx(p.t), y = ty(p.c);
    if (!started) { ctx.moveTo(x, y); started = true; } else ctx.lineTo(x, y);
  }
  ctx.stroke();

  // Current value label at end of temperature line
  if (visSamples.length > 0) {
    var last = visSamples[visSamples.length - 1];
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
  if (rangeSeconds < 7200) return 900;
  if (rangeSeconds < 14400) return 1800;
  return 3600;
}

function fmtAgo(sec) {
  if (sec < 60) return sec.toFixed(0) + 's';
  if (sec < 3600) return Math.round(sec / 60) + 'm';
  var h = Math.floor(sec / 3600);
  var m = Math.round((sec % 3600) / 60);
  return m > 0 ? h + 'h' + m + 'm' : h + 'h';
}

function setConn(id, ok) {
  var el = document.getElementById(id);
  if (ok) el.classList.add('ok'); else el.classList.remove('ok');
}

function setRelayStat(id, on) {
  var el = document.getElementById(id);
  if (on) el.classList.add('on'); else el.classList.remove('on');
}

// --- Controls ---
async function startSmoking() {
  var temp = parseFloat(document.getElementById('setpoint-input').value);
  var r = await post('/start', { temp: temp });
  if (r) toast('Smoker started at ' + temp + '\u00B0F', 'ok');
}

async function stopSmoking() {
  var r = await post('/stop');
  if (r) toast('Stopping...', 'ok');
}

async function doShutdown() {
  if (!confirm('Shutdown the smoker?')) return;
  var r = await post('/shutdown');
  if (r) toast('Shutting down', 'info');
}

function adjSetpoint(delta) {
  var el = document.getElementById('setpoint-input');
  var v = Math.max(150, Math.min(500, parseInt(el.value) + delta));
  el.value = v;
  el.focus();
}

async function applySetpoint() {
  var v = parseInt(document.getElementById('setpoint-input').value);
  if (v < 150 || v > 500) { toast('150-500\u00B0F range', 'err'); return; }
  var r = await post('/setpoint', { temp: v });
  if (r) toast('Setpoint: ' + v + '\u00B0F', 'ok');
}

// --- Debug ---
function toggleDebug() {
  var panel = document.getElementById('debug-panel');
  var arrow = document.getElementById('debug-arrow');
  panel.classList.toggle('hidden');
  arrow.innerHTML = panel.classList.contains('hidden') ? '&#9662;' : '&#9652;';
}

async function toggleDebugMode() {
  debugActive = !debugActive;
  var r = await post('/debug/mode', { enabled: debugActive });
  if (!r) { debugActive = !debugActive; return; }
  var btn = document.getElementById('btn-debug');
  var ctrl = document.getElementById('debug-controls');
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
  await post('/debug/relay', { relay: relay, state: state });
}

async function setTempOverride() {
  var v = document.getElementById('temp-override').value;
  var r = await post('/debug/temp', { temp: v });
  if (r) toast('Override: ' + v + '\u00B0F', 'info');
}

async function clearTempOverride() {
  try {
    await fetch(API + '/debug/temp', { method: 'DELETE' });
    toast('Override cleared', 'ok');
  } catch (e) { toast('Failed', 'err'); }
}

async function resetError() {
  var r = await post('/debug/reset');
  if (r) toast('Error state cleared', 'ok');
}

// --- Firmware Updates ---
async function checkVersionStatus() {
  try {
    var r = await fetch(API + '/version');
    if (!r.ok) return;
    var v = await r.json();
    if (v.updateAvailable) showUpdateBanner(v.latest);
    updateFastOtaBtn(v.fastCheck);
  } catch (e) { /* ignore on page load */ }
}

async function checkForUpdate() {
  toast('Checking for updates...', 'info');
  var r = await post('/update/check');
  if (!r) return;
  for (var i = 0; i < 10; i++) {
    await new Promise(function(ok) { setTimeout(ok, 2000); });
    try {
      var v = await (await fetch(API + '/version')).json();
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
  var r = await post('/update/apply');
  if (r && r.ok) toast('Installing update, device will reboot...', 'info');
}

var fastOtaActive = false;
async function toggleFastOta() {
  fastOtaActive = !fastOtaActive;
  var r = await post('/update/fast', { enabled: fastOtaActive });
  if (!r) { fastOtaActive = !fastOtaActive; return; }
  updateFastOtaBtn(fastOtaActive);
  toast('OTA check interval: ' + (fastOtaActive ? '60s' : '6hrs'), 'info');
}

function updateFastOtaBtn(active) {
  fastOtaActive = !!active;
  var btn = document.getElementById('btn-fast-ota');
  if (!btn) return;
  btn.textContent = 'Fast OTA: ' + (active ? 'On' : 'Off');
  if (active) btn.classList.add('active'); else btn.classList.remove('active');
}

// --- Tab System ---
function switchTab(tab) {
  document.querySelectorAll('.tab-btn').forEach(function(b) { b.classList.remove('active'); });
  document.querySelectorAll('.tab-panel').forEach(function(p) { p.classList.remove('active'); });
  if (tab === 'pid') {
    document.querySelectorAll('.tab-btn')[1].classList.add('active');
    var el = document.getElementById('tab-pid');
    el.style.display = 'block';
    el.classList.add('active');
    document.getElementById('tab-dashboard').classList.remove('active');
    document.getElementById('tab-dashboard').style.display = 'none';
    startPidAnim();
    requestAnimationFrame(function() { drawPidChart(); });
  } else {
    document.querySelectorAll('.tab-btn')[0].classList.add('active');
    var el = document.getElementById('tab-dashboard');
    el.style.display = 'block';
    el.classList.add('active');
    document.getElementById('tab-pid').classList.remove('active');
    document.getElementById('tab-pid').style.display = 'none';
    stopPidAnim();
    requestAnimationFrame(drawGraph);
  }
}

// =============================================================================
// PID ANIMATED VISUALIZATION
// =============================================================================

// --- PID History (JS-side ring buffer) ---
var pidHistory = [];  // {t, p, i, d, out, temp, sp, err}
var pidRangeSec = 120;
var PID_MAX_SAMPLES = 300;  // 10 minutes at 2s polling
var pidLastData = { p: 0, i: 0, d: 0, output: 0, error: 0, cycleRemaining: 0, augerOn: false, lidOpen: false, reigniteAttempts: 0 };

function recordPidSample(s) {
  if (!s.pid) return;
  pidLastData = s.pid;
  var now = Date.now() / 1000;
  pidHistory.push({
    t: now,
    p: parseFloat(s.pid.p) || 0,
    i: parseFloat(s.pid.i) || 0,
    d: parseFloat(s.pid.d) || 0,
    out: parseFloat(s.pid.output) || 0,
    temp: s.temp,
    sp: s.setpoint,
    err: parseFloat(s.pid.error) || 0
  });
  while (pidHistory.length > PID_MAX_SAMPLES) pidHistory.shift();
}

function setPidRange(sec) {
  pidRangeSec = sec;
  var btns = document.querySelectorAll('#pid-range-btns button');
  btns.forEach(function(b) { b.classList.remove('active'); });
  var vals = [120, 300, 600];
  var idx = vals.indexOf(sec);
  if (idx >= 0 && btns[idx]) btns[idx].classList.add('active');
  drawPidChart();
}

// --- PID Animation State ---
var pidSceneState = {
  temp: 0, setpoint: 225, error: 0,
  p: 0, i: 0, d: 0, output: 0,
  augerOn: false, fanOn: false, igniterOn: false,
  lidOpen: false, reigniteAttempts: 0,
  state: 'Idle', cycleRemaining: 0
};

var PID_FRAME_MS = 55; // ~18 FPS
var pidAnimId = null;
var pidLastFrame = 0;

// --- Particle System (pre-allocated, zero GC) ---
var PID_PARTICLES = [];
var PID_PARTICLE_COUNT = 100; // 0-59 fire, 60-99 smoke

function initParticles() {
  PID_PARTICLES = [];
  for (var j = 0; j < PID_PARTICLE_COUNT; j++) {
    PID_PARTICLES.push({ active: false, x: 0, y: 0, vx: 0, vy: 0, life: 0, maxLife: 0, size: 0, type: j < 60 ? 'fire' : 'smoke' });
  }
}
initParticles();

function spawnFireParticle(x, y) {
  for (var j = 0; j < 60; j++) {
    if (!PID_PARTICLES[j].active) {
      var pp = PID_PARTICLES[j];
      pp.active = true;
      pp.x = x + (Math.random() - 0.5) * 10;
      pp.y = y;
      pp.vx = (Math.random() - 0.5) * 12;
      pp.vy = -20 - Math.random() * 30;
      pp.life = 0;
      pp.maxLife = 0.4 + Math.random() * 0.4;
      pp.size = 2 + Math.random() * 3;
      return;
    }
  }
}

function spawnSmokeParticle(x, y) {
  for (var j = 60; j < 100; j++) {
    if (!PID_PARTICLES[j].active) {
      var pp = PID_PARTICLES[j];
      pp.active = true;
      pp.x = x + (Math.random() - 0.5) * 6;
      pp.y = y;
      pp.vx = 3 + Math.random() * 8;
      pp.vy = -15 - Math.random() * 20;
      pp.life = 0;
      pp.maxLife = 0.8 + Math.random() * 0.6;
      pp.size = 3 + Math.random() * 3;
      return;
    }
  }
}

function updateParticles(dt) {
  for (var j = 0; j < PID_PARTICLE_COUNT; j++) {
    var pp = PID_PARTICLES[j];
    if (!pp.active) continue;
    pp.life += dt;
    if (pp.life >= pp.maxLife) { pp.active = false; continue; }
    pp.x += pp.vx * dt;
    pp.y += pp.vy * dt;
    if (pp.type === 'fire') {
      pp.size *= (1 - dt * 1.5);
      pp.vy -= 10 * dt; // accelerate upward
    } else {
      pp.size += dt * 4; // smoke expands
      pp.vy *= (1 - dt * 0.5); // slow down
      pp.vx += dt * 2; // drift right
    }
  }
}

// --- DPR-Aware Canvas Helper ---
function setupCanvas(id) {
  var canvas = document.getElementById(id);
  if (!canvas) return null;
  var dpr = window.devicePixelRatio || 1;
  var rect = canvas.getBoundingClientRect();
  var W = rect.width, H = rect.height;
  if (W === 0 || H === 0) return null;
  canvas.width = W * dpr;
  canvas.height = H * dpr;
  var ctx = canvas.getContext('2d');
  ctx.scale(dpr, dpr);
  return { ctx: ctx, W: W, H: H };
}

// --- Bridge: status poll -> pidSceneState ---
function updatePidVisuals(s) {
  if (!s.pid) return;
  var pid = s.pid;

  pidSceneState.temp = s.temp || 0;
  pidSceneState.setpoint = s.setpoint || 225;
  pidSceneState.error = parseFloat(pid.error) || 0;
  pidSceneState.p = parseFloat(pid.p) || 0;
  pidSceneState.i = parseFloat(pid.i) || 0;
  pidSceneState.d = parseFloat(pid.d) || 0;
  pidSceneState.output = parseFloat(pid.output) || 0;
  pidSceneState.augerOn = !!pid.augerOn;
  pidSceneState.fanOn = !!s.fan;
  pidSceneState.igniterOn = !!s.igniter;
  pidSceneState.lidOpen = !!pid.lidOpen;
  pidSceneState.reigniteAttempts = pid.reigniteAttempts || 0;
  pidSceneState.state = s.state || 'Idle';
  pidSceneState.cycleRemaining = parseInt(pid.cycleRemaining) || 0;

  // Always update the time-series chart if PID tab is visible
  if (document.getElementById('tab-pid') && document.getElementById('tab-pid').classList.contains('active')) {
    drawPidChart();
  }
}

// --- Animation Loop ---
function pidAnimLoop(ts) {
  if (!pidAnimId) return;
  if (ts - pidLastFrame < PID_FRAME_MS) {
    pidAnimId = requestAnimationFrame(pidAnimLoop);
    return;
  }
  pidLastFrame = ts;
  var dt = PID_FRAME_MS / 1000;

  updateParticles(dt);

  // Draw each canvas
  drawSmokerScene(ts);
  drawPidSpring(ts);
  drawPidBucket(ts);
  drawPidSpeedo(ts);
  drawFeedbackLoop(ts);

  pidAnimId = requestAnimationFrame(pidAnimLoop);
}

function startPidAnim() {
  if (pidAnimId) return;
  pidAnimId = requestAnimationFrame(pidAnimLoop);
}

function stopPidAnim() {
  if (pidAnimId) {
    cancelAnimationFrame(pidAnimId);
    pidAnimId = null;
  }
}

// =============================================================================
// DRAW: Smoker Cross-Section Scene (the hero)
// =============================================================================
function drawSmokerScene(ts) {
  var c = setupCanvas('smoker-scene');
  if (!c) return;
  var ctx = c.ctx, W = c.W, H = c.H;
  var S = pidSceneState;
  var t = ts / 1000; // time in seconds

  // Clear
  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  // --- Layout proportions ---
  var margin = 8;
  var hopperW = W * 0.14;
  var hopperH = H * 0.55;
  var hopperX = margin;
  var hopperY = H * 0.15;

  var augerTubeY = hopperY + hopperH - 14;
  var augerTubeH = 18;
  var augerTubeX1 = hopperX + hopperW;
  var augerTubeX2 = W * 0.36;

  var chamberX = W * 0.22;
  var chamberY = margin + 4;
  var chamberW = W * 0.50;
  var chamberH = H * 0.62;

  var firepotX = augerTubeX2 - 5;
  var firepotY = chamberY + chamberH;
  var firepotW = W * 0.18;
  var firepotH = H * 0.18;

  var fanX = firepotX + firepotW + 8;
  var fanY = firepotY + firepotH / 2;
  var fanR = Math.min(firepotH * 0.4, 18);

  var chimneyX = chamberX + chamberW - 16;
  var chimneyY = 4;
  var chimneyW = 20;
  var chimneyH = chamberY + 10;

  var thermX = chamberX + chamberW + 8;
  var thermY = chamberY + 10;
  var thermH = chamberH - 20;
  var thermW = 14;

  // --- Draw Hopper ---
  var hopGrad = ctx.createLinearGradient(hopperX, hopperY, hopperX + hopperW, hopperY);
  hopGrad.addColorStop(0, '#5a3e28');
  hopGrad.addColorStop(0.5, '#7a5a3e');
  hopGrad.addColorStop(1, '#5a3e28');
  ctx.fillStyle = hopGrad;
  ctx.beginPath();
  ctx.moveTo(hopperX, hopperY);
  ctx.lineTo(hopperX + hopperW, hopperY);
  ctx.lineTo(hopperX + hopperW, hopperY + hopperH);
  ctx.lineTo(hopperX, hopperY + hopperH);
  ctx.closePath();
  ctx.fill();
  ctx.strokeStyle = '#8a6a4e';
  ctx.lineWidth = 1.5;
  ctx.stroke();

  // Pellets inside hopper (seeded random circles)
  var pelletSeed = 42;
  function seededRand() { pelletSeed = (pelletSeed * 16807 + 0) % 2147483647; return pelletSeed / 2147483647; }
  pelletSeed = 42;
  ctx.fillStyle = '#6b4c2e';
  for (var row = 0; row < 8; row++) {
    for (var col = 0; col < 3; col++) {
      var px = hopperX + 6 + col * (hopperW - 12) / 2.5 + seededRand() * 6;
      var py = hopperY + 12 + row * (hopperH - 24) / 7.5 + seededRand() * 4;
      var pr = 2.5 + seededRand() * 1.5;
      ctx.beginPath();
      ctx.arc(px, py, pr, 0, Math.PI * 2);
      ctx.fillStyle = 'rgba(' + Math.floor(80 + seededRand() * 40) + ',' + Math.floor(50 + seededRand() * 30) + ',' + Math.floor(20 + seededRand() * 20) + ',0.9)';
      ctx.fill();
    }
  }

  // Hopper label
  ctx.fillStyle = '#aaa';
  ctx.font = '9px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('HOPPER', hopperX + hopperW / 2, hopperY - 3);

  // --- Draw Auger Tube ---
  ctx.fillStyle = '#3a3a3a';
  ctx.fillRect(augerTubeX1, augerTubeY, augerTubeX2 - augerTubeX1, augerTubeH);
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1;
  ctx.strokeRect(augerTubeX1, augerTubeY, augerTubeX2 - augerTubeX1, augerTubeH);

  // Auger screw (animated sine wave)
  var augerPhase = S.augerOn ? (t * 3) : 0;
  ctx.strokeStyle = '#888';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  var augerLen = augerTubeX2 - augerTubeX1;
  var augerMidY = augerTubeY + augerTubeH / 2;
  for (var ax = 0; ax <= augerLen; ax += 2) {
    var sx = augerTubeX1 + ax;
    var sy = augerMidY + Math.sin(ax * 0.15 + augerPhase) * (augerTubeH * 0.35);
    if (ax === 0) ctx.moveTo(sx, sy); else ctx.lineTo(sx, sy);
  }
  ctx.stroke();

  // --- Draw Cooking Chamber ---
  var chamGrad = ctx.createLinearGradient(chamberX, chamberY, chamberX, chamberY + chamberH);
  chamGrad.addColorStop(0, '#2a2a2a');
  chamGrad.addColorStop(1, '#1a1a1a');
  ctx.fillStyle = chamGrad;

  // Chamber body
  ctx.beginPath();
  ctx.moveTo(chamberX, chamberY + 10);
  ctx.arcTo(chamberX, chamberY, chamberX + 10, chamberY, 10);
  ctx.lineTo(chamberX + chamberW - 10, chamberY);
  ctx.arcTo(chamberX + chamberW, chamberY, chamberX + chamberW, chamberY + 10, 10);
  ctx.lineTo(chamberX + chamberW, chamberY + chamberH);
  ctx.lineTo(chamberX, chamberY + chamberH);
  ctx.closePath();
  ctx.fill();
  ctx.strokeStyle = '#444';
  ctx.lineWidth = 2;
  ctx.stroke();

  // Lid open effect
  if (S.lidOpen) {
    ctx.save();
    ctx.fillStyle = 'rgba(231, 76, 60, 0.12)';
    ctx.fillRect(chamberX, chamberY - 8, chamberW, chamberH + 8);
    // Gap at top showing lid ajar
    ctx.fillStyle = '#111';
    ctx.fillRect(chamberX + 2, chamberY - 4, chamberW - 4, 6);
    ctx.fillStyle = '#e74c3c';
    ctx.font = 'bold 11px -apple-system, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('LID OPEN', chamberX + chamberW / 2, chamberY + 18);
    ctx.restore();
  }

  // Deflector plate (angled)
  var defY = chamberY + chamberH * 0.55;
  ctx.strokeStyle = '#666';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(chamberX + 10, defY + 8);
  ctx.lineTo(chamberX + chamberW * 0.6, defY - 4);
  ctx.stroke();
  // Add metallic sheen
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 0.5;
  ctx.beginPath();
  ctx.moveTo(chamberX + 10, defY + 10);
  ctx.lineTo(chamberX + chamberW * 0.6, defY - 2);
  ctx.stroke();

  // Grill grates
  var grateY1 = chamberY + chamberH * 0.22;
  var grateY2 = chamberY + chamberH * 0.38;
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1.5;
  for (var gy = grateY1; gy <= grateY2; gy += (grateY2 - grateY1) / 3) {
    ctx.beginPath();
    ctx.moveTo(chamberX + 12, gy);
    ctx.lineTo(chamberX + chamberW - 12, gy);
    ctx.stroke();
  }

  // Food silhouettes on grates
  var foodY = grateY1 - 2;
  var foodCx = chamberX + chamberW * 0.3;

  // Brisket (rounded rectangle)
  ctx.fillStyle = '#5a3020';
  ctx.beginPath();
  var bx = foodCx - 20, by = foodY - 12, bw = 40, bh = 12;
  ctx.moveTo(bx + 3, by);
  ctx.arcTo(bx + bw, by, bx + bw, by + bh, 3);
  ctx.arcTo(bx + bw, by + bh, bx, by + bh, 3);
  ctx.arcTo(bx, by + bh, bx, by, 3);
  ctx.arcTo(bx, by, bx + bw, by, 3);
  ctx.fill();
  // Fat cap
  ctx.fillStyle = '#7a5038';
  ctx.fillRect(bx + 3, by, bw - 6, 3);

  // Ribs (curved shape)
  var rx = foodCx + 32, ry = foodY - 8;
  ctx.fillStyle = '#6a3828';
  ctx.beginPath();
  ctx.moveTo(rx, ry);
  ctx.quadraticCurveTo(rx + 20, ry - 6, rx + 35, ry);
  ctx.lineTo(rx + 35, ry + 8);
  ctx.quadraticCurveTo(rx + 20, ry + 2, rx, ry + 8);
  ctx.closePath();
  ctx.fill();
  // Rib bones
  ctx.strokeStyle = '#8a6048';
  ctx.lineWidth = 0.8;
  for (var ri = 0; ri < 4; ri++) {
    var ribX = rx + 5 + ri * 8;
    ctx.beginPath();
    ctx.moveTo(ribX, ry + 1);
    ctx.lineTo(ribX, ry + 7);
    ctx.stroke();
  }

  // Drumstick
  var dx = foodCx - 48, dy = foodY - 10;
  ctx.fillStyle = '#5a3420';
  ctx.beginPath();
  // Meaty part (oval)
  ctx.ellipse(dx, dy + 3, 8, 5, 0, 0, Math.PI * 2);
  ctx.fill();
  // Bone
  ctx.strokeStyle = '#d4c8b0';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(dx + 7, dy + 2);
  ctx.lineTo(dx + 18, dy - 2);
  ctx.stroke();
  // Bone tip
  ctx.fillStyle = '#d4c8b0';
  ctx.beginPath();
  ctx.arc(dx + 18, dy - 2, 2, 0, Math.PI * 2);
  ctx.fill();

  // --- Firepot ---
  ctx.fillStyle = '#2a2020';
  ctx.beginPath();
  // Bowl shape
  ctx.moveTo(firepotX, firepotY);
  ctx.lineTo(firepotX + 4, firepotY + firepotH);
  ctx.lineTo(firepotX + firepotW - 4, firepotY + firepotH);
  ctx.lineTo(firepotX + firepotW, firepotY);
  ctx.closePath();
  ctx.fill();
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1.5;
  ctx.stroke();

  // Igniter rod glow
  if (S.igniterOn) {
    ctx.save();
    var igX = firepotX + firepotW * 0.7;
    var igY1 = firepotY + 4;
    var igY2 = firepotY + firepotH - 4;
    // Glow
    var igGlow = ctx.createRadialGradient(igX, (igY1 + igY2) / 2, 1, igX, (igY1 + igY2) / 2, 12);
    igGlow.addColorStop(0, 'rgba(255, 60, 20, 0.6)');
    igGlow.addColorStop(1, 'rgba(255, 60, 20, 0)');
    ctx.fillStyle = igGlow;
    ctx.fillRect(igX - 12, igY1 - 8, 24, igY2 - igY1 + 16);
    // Rod
    ctx.strokeStyle = '#ff3c14';
    ctx.lineWidth = 2.5;
    ctx.beginPath();
    ctx.moveTo(igX, igY1);
    ctx.lineTo(igX, igY2);
    ctx.stroke();
    ctx.restore();
  }

  // Fire particles - spawn based on output
  var spawnRate = (S.output / 100) * 3; // max 3 per frame at 100%
  if (S.state === 'Running' || S.state === 'Startup' || S.state === 'Reignite') {
    if (Math.random() < spawnRate) {
      spawnFireParticle(firepotX + firepotW / 2, firepotY - 2);
    }
  }

  // Draw fire particles
  for (var j = 0; j < 60; j++) {
    var pp = PID_PARTICLES[j];
    if (!pp.active) continue;
    var frac = pp.life / pp.maxLife;
    var alpha = 1 - frac;
    // Color: yellow -> orange -> red -> transparent
    var r, g, b;
    if (frac < 0.3) {
      r = 255; g = Math.floor(200 - frac * 300); b = 30;
    } else if (frac < 0.6) {
      r = 255; g = Math.floor(110 - (frac - 0.3) * 300); b = 10;
    } else {
      r = Math.floor(255 - (frac - 0.6) * 300); g = 20; b = 5;
    }
    ctx.beginPath();
    ctx.arc(pp.x, pp.y, Math.max(0.5, pp.size), 0, Math.PI * 2);
    ctx.fillStyle = 'rgba(' + r + ',' + g + ',' + b + ',' + (alpha * 0.9).toFixed(2) + ')';
    ctx.fill();
  }

  // --- Fan ---
  ctx.save();
  ctx.translate(fanX, fanY);

  // Fan housing circle
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.arc(0, 0, fanR + 3, 0, Math.PI * 2);
  ctx.stroke();

  // Fan blades (3 arms)
  var fanAngle = S.fanOn ? (t * 8) : 0;
  ctx.fillStyle = '#666';
  for (var fb = 0; fb < 3; fb++) {
    var ba = fanAngle + fb * (Math.PI * 2 / 3);
    ctx.save();
    ctx.rotate(ba);
    ctx.beginPath();
    ctx.moveTo(0, -2);
    ctx.quadraticCurveTo(fanR * 0.5, -4, fanR - 2, -1);
    ctx.arc(fanR - 2, 0, 2, -Math.PI / 2, Math.PI / 2);
    ctx.quadraticCurveTo(fanR * 0.5, 4, 0, 2);
    ctx.closePath();
    ctx.fill();
    ctx.restore();
  }
  // Center hub
  ctx.beginPath();
  ctx.arc(0, 0, 3, 0, Math.PI * 2);
  ctx.fillStyle = '#888';
  ctx.fill();

  ctx.restore();

  // Air flow dashes (from fan toward firepot)
  if (S.fanOn) {
    ctx.strokeStyle = 'rgba(100, 180, 255, 0.3)';
    ctx.lineWidth = 1;
    var airPhase = (t * 40) % 20;
    for (var ai = 0; ai < 5; ai++) {
      var ax = fanX - 8 - ai * 8 - airPhase;
      if (ax > firepotX + firepotW && ax < fanX - 4) {
        ctx.beginPath();
        ctx.moveTo(ax, fanY - 2);
        ctx.lineTo(ax - 5, fanY - 2);
        ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(ax, fanY + 2);
        ctx.lineTo(ax - 5, fanY + 2);
        ctx.stroke();
      }
    }
  }

  // Fan label
  ctx.fillStyle = S.fanOn ? '#64b4ff' : '#555';
  ctx.font = '8px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('FAN', fanX, fanY + fanR + 14);

  // --- Chimney ---
  ctx.fillStyle = '#333';
  ctx.fillRect(chimneyX, chimneyY, chimneyW, chimneyH);
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1;
  ctx.strokeRect(chimneyX, chimneyY, chimneyW, chimneyH);
  // Chimney cap
  ctx.fillStyle = '#444';
  ctx.fillRect(chimneyX - 3, chimneyY, chimneyW + 6, 4);

  // Smoke particles - spawn from chimney
  if (S.state === 'Running' || S.state === 'Startup' || S.state === 'Reignite' || S.state === 'Cooldown') {
    if (Math.random() < 0.4) {
      spawnSmokeParticle(chimneyX + chimneyW / 2, chimneyY);
    }
  }

  // Draw smoke particles
  for (var j = 60; j < 100; j++) {
    var pp = PID_PARTICLES[j];
    if (!pp.active) continue;
    var frac = pp.life / pp.maxLife;
    var alpha = (1 - frac) * 0.5;
    var gray = Math.floor(120 + frac * 80);
    ctx.beginPath();
    ctx.arc(pp.x, pp.y, Math.max(1, pp.size), 0, Math.PI * 2);
    ctx.fillStyle = 'rgba(' + gray + ',' + gray + ',' + gray + ',' + alpha.toFixed(2) + ')';
    ctx.fill();
  }

  // --- Thermometer ---
  // Outer tube
  ctx.fillStyle = '#222';
  ctx.beginPath();
  ctx.moveTo(thermX, thermY + 4);
  ctx.arcTo(thermX, thermY, thermX + thermW, thermY, 4);
  ctx.arcTo(thermX + thermW, thermY, thermX + thermW, thermY + 4, 4);
  ctx.lineTo(thermX + thermW, thermY + thermH - 4);
  ctx.arcTo(thermX + thermW, thermY + thermH, thermX, thermY + thermH, 4);
  ctx.arcTo(thermX, thermY + thermH, thermX, thermY, 4);
  ctx.closePath();
  ctx.fill();
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1;
  ctx.stroke();

  // Mercury fill
  var tempClamped = Math.max(0, Math.min(500, S.temp));
  var fillFrac = tempClamped / 500;
  var mercuryH = (thermH - 8) * fillFrac;
  var mercuryY = thermY + thermH - 4 - mercuryH;

  // Mercury color: blue < 150, orange 150-400, red > 400
  var mercColor;
  if (tempClamped < 150) mercColor = '#3498db';
  else if (tempClamped > 400) mercColor = '#e74c3c';
  else mercColor = '#ff6b35';

  ctx.fillStyle = mercColor;
  ctx.fillRect(thermX + 3, mercuryY, thermW - 6, mercuryH);

  // Setpoint tick
  var spFrac = Math.max(0, Math.min(1, S.setpoint / 500));
  var spTickY = thermY + thermH - 4 - (thermH - 8) * spFrac;
  ctx.strokeStyle = '#fff';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.moveTo(thermX - 2, spTickY);
  ctx.lineTo(thermX + 4, spTickY);
  ctx.stroke();
  ctx.beginPath();
  ctx.moveTo(thermX + thermW - 4, spTickY);
  ctx.lineTo(thermX + thermW + 2, spTickY);
  ctx.stroke();

  // Temp value
  ctx.fillStyle = mercColor;
  ctx.font = 'bold 10px SF Mono, Consolas, monospace';
  ctx.textAlign = 'center';
  ctx.fillText(S.temp > 0 ? S.temp.toFixed(0) + '\u00B0' : '--', thermX + thermW / 2, thermY + thermH + 14);

  // --- Reignite badge ---
  if (S.reigniteAttempts > 0) {
    var badgeX = chamberX + chamberW - 35;
    var badgeY = chamberY + 10;
    ctx.fillStyle = '#e67e22';
    ctx.beginPath();
    ctx.arc(badgeX, badgeY, 10, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = '#fff';
    ctx.font = 'bold 10px -apple-system, sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(S.reigniteAttempts, badgeX, badgeY);
    ctx.textBaseline = 'alphabetic';
    ctx.fillStyle = '#e67e22';
    ctx.font = '8px -apple-system, sans-serif';
    ctx.fillText('REIGNITE', badgeX, badgeY + 18);
  }

  // --- Labels ---
  ctx.fillStyle = '#666';
  ctx.font = '8px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('FIREPOT', firepotX + firepotW / 2, firepotY + firepotH + 12);
  ctx.fillText('CHAMBER', chamberX + chamberW / 2, chamberY + chamberH + 12);
}

// =============================================================================
// DRAW: P = Spring Metaphor
// =============================================================================
function drawPidSpring(ts) {
  var c = setupCanvas('pid-spring');
  if (!c) return;
  var ctx = c.ctx, W = c.W, H = c.H;
  var S = pidSceneState;

  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  // Title
  ctx.fillStyle = '#3498db';
  ctx.font = 'bold 11px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('P = Spring', W / 2, 16);

  ctx.fillStyle = '#666';
  ctx.font = '9px -apple-system, sans-serif';
  ctx.fillText('How far off?', W / 2, 28);

  // Layout
  var cy = H * 0.55;
  var margin = 14;
  var maxStretch = W - margin * 2;

  // Error maps to spring stretch: 0 error = centered, large error = stretched
  var errAbs = Math.abs(S.error);
  var errSign = S.error < 0 ? -1 : 1; // negative = below setpoint
  var stretchFrac = Math.min(1, errAbs / 30); // 30 deg = max stretch

  // NOW and TARGET positions
  var centerX = W / 2;
  var targetX = centerX + maxStretch * 0.3;
  var nowX = centerX - stretchFrac * maxStretch * 0.3 * errSign;
  // Clamp
  nowX = Math.max(margin + 10, Math.min(W - margin - 10, nowX));

  // Spring color: green (small error) -> orange -> red (large error)
  var springColor;
  if (errAbs < 3) springColor = '#2ecc71';
  else if (errAbs < 10) springColor = '#ff6b35';
  else springColor = '#e74c3c';

  // Draw spring (zigzag line)
  var leftX = Math.min(nowX, targetX);
  var rightX = Math.max(nowX, targetX);
  var springLen = rightX - leftX;
  var coils = 8;
  var amplitude = 8 - stretchFrac * 4; // less amplitude when stretched

  ctx.strokeStyle = springColor;
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(leftX, cy);
  for (var si = 1; si <= coils * 2; si++) {
    var sx = leftX + (si / (coils * 2)) * springLen;
    var sy = cy + (si % 2 === 0 ? -amplitude : amplitude);
    ctx.lineTo(sx, sy);
  }
  ctx.lineTo(rightX, cy);
  ctx.stroke();

  // NOW endpoint
  ctx.fillStyle = '#ff6b35';
  ctx.beginPath();
  ctx.arc(nowX, cy, 6, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = '#fff';
  ctx.font = 'bold 8px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('NOW', nowX, cy + 18);
  ctx.fillStyle = '#ff6b35';
  ctx.font = '9px SF Mono, Consolas, monospace';
  ctx.fillText(S.temp.toFixed(0) + '\u00B0', nowX, cy - 12);

  // TARGET endpoint
  ctx.fillStyle = '#2ecc71';
  ctx.beginPath();
  ctx.arc(targetX, cy, 6, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = '#fff';
  ctx.font = 'bold 8px -apple-system, sans-serif';
  ctx.fillText('TARGET', targetX, cy + 18);
  ctx.fillStyle = '#2ecc71';
  ctx.font = '9px SF Mono, Consolas, monospace';
  ctx.fillText(S.setpoint.toFixed(0) + '\u00B0', targetX, cy - 12);

  // P value
  ctx.fillStyle = '#3498db';
  ctx.font = 'bold 12px SF Mono, Consolas, monospace';
  ctx.textAlign = 'center';
  ctx.fillText('P = ' + S.p.toFixed(3), W / 2, H - 8);
}

// =============================================================================
// DRAW: I = Bucket Metaphor
// =============================================================================
function drawPidBucket(ts) {
  var c = setupCanvas('pid-bucket');
  if (!c) return;
  var ctx = c.ctx, W = c.W, H = c.H;
  var S = pidSceneState;
  var t = ts / 1000;

  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  // Title
  ctx.fillStyle = '#2ecc71';
  ctx.font = 'bold 11px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('I = Bucket', W / 2, 16);

  ctx.fillStyle = '#666';
  ctx.font = '9px -apple-system, sans-serif';
  ctx.fillText('Steady offset fix', W / 2, 28);

  // Bucket dimensions
  var bucketTopW = W * 0.6;
  var bucketBotW = W * 0.4;
  var bucketH = H * 0.42;
  var bucketX = (W - bucketTopW) / 2;
  var bucketY = H * 0.30;

  // Integral maps to fill level: I ranges roughly -0.5 to +0.5
  // Map to 0-100% of bucket
  var fillPct = Math.min(1, Math.max(0, (S.i + 0.5))); // 0 at -0.5, 1 at +0.5

  // Draw bucket (trapezoid)
  var tl = bucketX;
  var tr = bucketX + bucketTopW;
  var bl = bucketX + (bucketTopW - bucketBotW) / 2;
  var br = bl + bucketBotW;
  var by = bucketY + bucketH;

  // Water fill
  if (fillPct > 0.01) {
    var waterH = bucketH * fillPct;
    var waterY = by - waterH;
    // Calculate trapezoid width at water level
    var topFrac = 1 - (waterY - bucketY) / bucketH;
    var wl = bl + (tl - bl) * (1 - topFrac);
    var wr = br + (tr - br) * (1 - topFrac);

    // Water gradient
    var wGrad = ctx.createLinearGradient(0, waterY, 0, by);
    wGrad.addColorStop(0, 'rgba(46, 204, 113, 0.4)');
    wGrad.addColorStop(1, 'rgba(46, 204, 113, 0.7)');
    ctx.fillStyle = wGrad;
    ctx.beginPath();
    ctx.moveTo(wl, waterY);
    // Animated wave surface
    var waveAmp = 2;
    var waveFreq = 5;
    for (var wx = wl; wx <= wr; wx += 2) {
      var wfrac = (wx - wl) / (wr - wl);
      var wy = waterY + Math.sin(wfrac * waveFreq + t * 3) * waveAmp;
      ctx.lineTo(wx, wy);
    }
    ctx.lineTo(br, by);
    ctx.lineTo(bl, by);
    ctx.closePath();
    ctx.fill();

    // Warning glow when >80% full
    if (fillPct > 0.8) {
      ctx.save();
      ctx.shadowColor = 'rgba(231, 76, 60, 0.5)';
      ctx.shadowBlur = 10;
      ctx.strokeStyle = '#e74c3c';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(tl, bucketY);
      ctx.lineTo(bl, by);
      ctx.lineTo(br, by);
      ctx.lineTo(tr, bucketY);
      ctx.stroke();
      ctx.restore();
    }
  }

  // Bucket outline
  ctx.strokeStyle = '#666';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(tl, bucketY);
  ctx.lineTo(bl, by);
  ctx.lineTo(br, by);
  ctx.lineTo(tr, bucketY);
  ctx.stroke();

  // Balanced line at 50%
  var balY = bucketY + bucketH * 0.5;
  var balL = bl + (tl - bl) * 0.5;
  var balR = br + (tr - br) * 0.5;
  ctx.strokeStyle = '#888';
  ctx.lineWidth = 1;
  ctx.setLineDash([3, 3]);
  ctx.beginPath();
  ctx.moveTo(balL, balY);
  ctx.lineTo(balR, balY);
  ctx.stroke();
  ctx.setLineDash([]);
  ctx.fillStyle = '#888';
  ctx.font = '7px -apple-system, sans-serif';
  ctx.textAlign = 'right';
  ctx.fillText('balanced', balL - 3, balY + 3);

  // Dripping drops (when error is nonzero)
  if (Math.abs(S.error) > 1) {
    var dropPhase = (t * 2) % 1;
    var dropX = W / 2 + Math.sin(t * 1.5) * 5;
    var dropY = bucketY - 12 + dropPhase * 18;
    var dropAlpha = 1 - dropPhase;
    ctx.fillStyle = 'rgba(46, 204, 113, ' + dropAlpha.toFixed(2) + ')';
    ctx.beginPath();
    ctx.arc(dropX, dropY, 2, 0, Math.PI * 2);
    ctx.fill();
  }

  // I value
  ctx.fillStyle = '#2ecc71';
  ctx.font = 'bold 12px SF Mono, Consolas, monospace';
  ctx.textAlign = 'center';
  ctx.fillText('I = ' + S.i.toFixed(3), W / 2, H - 8);
}

// =============================================================================
// DRAW: D = Speedometer Metaphor
// =============================================================================
function drawPidSpeedo(ts) {
  var c = setupCanvas('pid-speedo');
  if (!c) return;
  var ctx = c.ctx, W = c.W, H = c.H;
  var S = pidSceneState;

  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  // Title
  ctx.fillStyle = '#ff6b35';
  ctx.font = 'bold 11px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('D = Speed', W / 2, 16);

  ctx.fillStyle = '#666';
  ctx.font = '9px -apple-system, sans-serif';
  ctx.fillText('Rate of change', W / 2, 28);

  // Gauge layout
  var cx = W / 2;
  var cy = H * 0.65;
  var gaugeR = Math.min(W * 0.38, H * 0.32);

  // Draw arc zones: COOLING (blue) | STABLE (green) | HEATING (red)
  var startA = Math.PI;
  var endA = 0;
  var zoneAngles = [
    { a1: Math.PI, a2: Math.PI * 0.7, color: '#3498db' },       // COOLING
    { a1: Math.PI * 0.7, a2: Math.PI * 0.3, color: '#2ecc71' }, // STABLE
    { a1: Math.PI * 0.3, a2: 0, color: '#e74c3c' }              // HEATING
  ];

  ctx.lineWidth = 6;
  for (var zi = 0; zi < zoneAngles.length; zi++) {
    var z = zoneAngles[zi];
    ctx.strokeStyle = hexAlpha(z.color, 0.4);
    ctx.beginPath();
    ctx.arc(cx, cy, gaugeR, z.a1, z.a2, true);
    ctx.stroke();
  }

  // Zone labels
  ctx.font = '7px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillStyle = '#3498db';
  ctx.fillText('COOL', cx - gaugeR * 0.7, cy + 14);
  ctx.fillStyle = '#2ecc71';
  ctx.fillText('STABLE', cx, cy - gaugeR + 18);
  ctx.fillStyle = '#e74c3c';
  ctx.fillText('HEAT', cx + gaugeR * 0.7, cy + 14);

  // Needle: D value maps to angle
  // D typically ranges -0.1 to +0.1 (negative = cooling in reverse-acting PID)
  // Map: large negative D (fast cooling) = left, zero = center, large positive D = right
  var dClamped = Math.max(-0.15, Math.min(0.15, S.d));
  var needleFrac = (dClamped + 0.15) / 0.3; // 0 = left (cooling), 1 = right (heating)
  var needleAngle = Math.PI - needleFrac * Math.PI;

  // Smoothed needle
  ctx.save();
  ctx.translate(cx, cy);
  ctx.rotate(-needleAngle + Math.PI);

  // Needle shadow
  ctx.strokeStyle = 'rgba(255,255,255,0.1)';
  ctx.lineWidth = 3;
  ctx.beginPath();
  ctx.moveTo(0, 0);
  ctx.lineTo(gaugeR - 6, 0);
  ctx.stroke();

  // Needle
  ctx.strokeStyle = '#fff';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(-4, 0);
  ctx.lineTo(gaugeR - 6, 0);
  ctx.stroke();

  // Needle tip
  ctx.fillStyle = '#ff6b35';
  ctx.beginPath();
  ctx.arc(gaugeR - 6, 0, 3, 0, Math.PI * 2);
  ctx.fill();

  ctx.restore();

  // Center pivot
  ctx.fillStyle = '#888';
  ctx.beginPath();
  ctx.arc(cx, cy, 4, 0, Math.PI * 2);
  ctx.fill();

  // Rate label
  var rateLabel;
  if (dClamped < -0.05) rateLabel = 'Cooling fast';
  else if (dClamped < -0.01) rateLabel = 'Cooling';
  else if (dClamped > 0.05) rateLabel = 'Heating fast';
  else if (dClamped > 0.01) rateLabel = 'Heating';
  else rateLabel = 'Stable';

  ctx.fillStyle = '#888';
  ctx.font = '9px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText(rateLabel, cx, cy + gaugeR * 0.2);

  // D value
  ctx.fillStyle = '#ff6b35';
  ctx.font = 'bold 12px SF Mono, Consolas, monospace';
  ctx.fillText('D = ' + S.d.toFixed(3), W / 2, H - 8);
}

// =============================================================================
// DRAW: Feedback Loop Band
// =============================================================================
function drawFeedbackLoop(ts) {
  var c = setupCanvas('feedback-loop');
  if (!c) return;
  var ctx = c.ctx, W = c.W, H = c.H;
  var S = pidSceneState;
  var t = ts / 1000;

  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  // Node positions (evenly spaced)
  var nodes = [
    { label: 'TARGET', value: S.setpoint.toFixed(0) + '\u00B0', color: '#2ecc71' },
    { label: 'ERROR', value: S.error.toFixed(1) + '\u00B0', color: S.error > 0 ? '#e74c3c' : '#3498db' },
    { label: 'PID', value: '', color: '#888' },
    { label: 'OUTPUT', value: S.output.toFixed(0) + '%', color: '#ff6b35' },
    { label: 'SMOKER', value: '', color: '#666' },
    { label: 'TEMP', value: S.temp.toFixed(0) + '\u00B0', color: '#ff6b35' }
  ];

  var margin = 20;
  var nodeSpacing = (W - margin * 2) / (nodes.length - 1);
  var cy = 26;
  var nodeR = 4;

  // Draw arrows and flow dots between nodes
  var animPhase = (t * 0.5) % 1;
  for (var ni = 0; ni < nodes.length - 1; ni++) {
    var x1 = margin + ni * nodeSpacing + nodeR + 2;
    var x2 = margin + (ni + 1) * nodeSpacing - nodeR - 2;

    // Arrow line
    ctx.strokeStyle = '#444';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(x1, cy);
    ctx.lineTo(x2, cy);
    ctx.stroke();

    // Arrowhead
    ctx.fillStyle = '#444';
    ctx.beginPath();
    ctx.moveTo(x2, cy);
    ctx.lineTo(x2 - 5, cy - 3);
    ctx.lineTo(x2 - 5, cy + 3);
    ctx.closePath();
    ctx.fill();

    // Flow dots
    for (var fd = 0; fd < 2; fd++) {
      var dotT = ((animPhase + fd * 0.5) % 1);
      var dotX = x1 + (x2 - x1) * dotT;
      var dotAlpha = 0.3 + 0.5 * Math.sin(dotT * Math.PI);
      ctx.beginPath();
      ctx.arc(dotX, cy, 2, 0, Math.PI * 2);
      ctx.fillStyle = hexAlpha(nodes[ni].color, dotAlpha);
      ctx.fill();
    }
  }

  // Draw nodes
  for (var ni = 0; ni < nodes.length; ni++) {
    var nx = margin + ni * nodeSpacing;
    var n = nodes[ni];

    // Node circle
    ctx.fillStyle = hexAlpha(n.color, 0.2);
    ctx.strokeStyle = n.color;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.arc(nx, cy, nodeR, 0, Math.PI * 2);
    ctx.fill();
    ctx.stroke();

    // Label above
    ctx.fillStyle = '#888';
    ctx.font = '7px -apple-system, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText(n.label, nx, cy - 10);

    // Value below
    if (n.value) {
      ctx.fillStyle = n.color;
      ctx.font = 'bold 9px SF Mono, Consolas, monospace';
      ctx.fillText(n.value, nx, cy + 16);
    }
  }

  // Feedback arrow (dashed, from TEMP back to ERROR, looping underneath)
  var tempX = margin + 5 * nodeSpacing;
  var errX = margin + 1 * nodeSpacing;
  var fbY = cy + 28;

  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1;
  ctx.setLineDash([3, 3]);
  ctx.beginPath();
  ctx.moveTo(tempX, cy + nodeR + 2);
  ctx.lineTo(tempX, fbY);
  ctx.lineTo(errX, fbY);
  ctx.lineTo(errX, cy + nodeR + 2);
  ctx.stroke();
  ctx.setLineDash([]);

  // Feedback arrow head
  ctx.fillStyle = '#555';
  ctx.beginPath();
  ctx.moveTo(errX, cy + nodeR + 2);
  ctx.lineTo(errX - 3, cy + nodeR + 7);
  ctx.lineTo(errX + 3, cy + nodeR + 7);
  ctx.closePath();
  ctx.fill();

  // Feedback label
  ctx.fillStyle = '#555';
  ctx.font = '7px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('feedback', (tempX + errX) / 2, fbY - 2);

  // Flow dots on feedback path
  for (var fd = 0; fd < 3; fd++) {
    var feedT = ((animPhase + fd * 0.33) % 1);
    var totalFeedLen = (tempX - errX) + 2 * (fbY - cy - nodeR - 2);
    var dist = feedT * totalFeedLen;
    var fdx, fdy;
    var downLen = fbY - cy - nodeR - 2;
    if (dist < downLen) {
      fdx = tempX;
      fdy = cy + nodeR + 2 + dist;
    } else if (dist < downLen + (tempX - errX)) {
      fdx = tempX - (dist - downLen);
      fdy = fbY;
    } else {
      fdx = errX;
      fdy = fbY - (dist - downLen - (tempX - errX));
    }
    ctx.beginPath();
    ctx.arc(fdx, fdy, 1.5, 0, Math.PI * 2);
    ctx.fillStyle = hexAlpha('#555', 0.5);
    ctx.fill();
  }

  // Speech bubble
  var bubbleX = W / 2;
  var bubbleY = H - 18;
  var msg;

  if (S.lidOpen) {
    msg = 'Lid open! Holding steady...';
  } else if (S.reigniteAttempts > 0) {
    msg = 'Fire recovery in progress...';
  } else if (S.state === 'Startup') {
    msg = 'Warming up...';
  } else if (S.state === 'Idle') {
    msg = 'Ready to smoke!';
  } else if (S.state === 'Cooldown' || S.state === 'Shutdown') {
    msg = 'Cooling down...';
  } else if (S.state === 'Error') {
    msg = 'Error! Check smoker.';
  } else if (S.error < -5) {
    msg = Math.abs(S.error).toFixed(0) + '\u00B0F below target \u2014 adding fuel!';
  } else if (S.error > 5) {
    msg = S.error.toFixed(0) + '\u00B0F above target \u2014 backing off!';
  } else if (Math.abs(S.error) <= 2) {
    msg = 'Right on target \u2014 cruising!';
  } else if (S.error < 0) {
    msg = 'Slightly cool \u2014 feeding more...';
  } else {
    msg = 'Slightly warm \u2014 easing off...';
  }

  // Bubble background
  ctx.font = '10px -apple-system, sans-serif';
  var msgW = ctx.measureText(msg).width + 16;
  var bubbleH = 18;
  ctx.fillStyle = 'rgba(40, 40, 40, 0.9)';
  ctx.beginPath();
  ctx.moveTo(bubbleX - msgW / 2 + 4, bubbleY - bubbleH / 2);
  ctx.arcTo(bubbleX + msgW / 2, bubbleY - bubbleH / 2, bubbleX + msgW / 2, bubbleY + bubbleH / 2, 4);
  ctx.arcTo(bubbleX + msgW / 2, bubbleY + bubbleH / 2, bubbleX - msgW / 2, bubbleY + bubbleH / 2, 4);
  ctx.arcTo(bubbleX - msgW / 2, bubbleY + bubbleH / 2, bubbleX - msgW / 2, bubbleY - bubbleH / 2, 4);
  ctx.arcTo(bubbleX - msgW / 2, bubbleY - bubbleH / 2, bubbleX + msgW / 2, bubbleY - bubbleH / 2, 4);
  ctx.fill();
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 0.5;
  ctx.stroke();

  // Bubble text
  ctx.fillStyle = '#ccc';
  ctx.textAlign = 'center';
  ctx.fillText(msg, bubbleX, bubbleY + 3);
}

// =============================================================================
// PID Time-Series Chart (retained from previous version)
// =============================================================================
function drawPidChart() {
  var canvas = document.getElementById('pid-history-chart');
  if (!canvas) return;
  var dpr = window.devicePixelRatio || 1;
  var rect = canvas.getBoundingClientRect();
  var W = rect.width, H = rect.height;
  if (W === 0 || H === 0) return;
  canvas.width = W * dpr;
  canvas.height = H * dpr;
  var ctx = canvas.getContext('2d');
  ctx.scale(dpr, dpr);

  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  if (pidHistory.length < 2) {
    ctx.fillStyle = '#555';
    ctx.font = '13px -apple-system, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('Collecting PID data\u2026', W / 2, H / 2);
    return;
  }

  var padL = 42, padR = 12, padT = 14, padB = 24;
  var gW = W - padL - padR;
  var gH = H - padT - padB;

  // Time range
  var tMax = pidHistory[pidHistory.length - 1].t;
  var tMin = tMax - pidRangeSec;

  // Filter to visible range
  var vis = pidHistory.filter(function(s) { return s.t >= tMin; });
  if (vis.length < 2) {
    ctx.fillStyle = '#555';
    ctx.font = '13px -apple-system, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('Collecting PID data\u2026', W / 2, H / 2);
    return;
  }

  // Y-axis: PID output 0-100% (left axis)
  var yMin = 0, yMax = 100;

  function tx(t) { return padL + (t - tMin) / (tMax - tMin) * gW; }
  function ty(v) { return padT + (1 - (v - yMin) / (yMax - yMin)) * gH; }

  // Grid
  ctx.strokeStyle = '#222';
  ctx.lineWidth = 0.5;
  for (var v = 0; v <= 100; v += 25) {
    var y = ty(v);
    ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(W - padR, y); ctx.stroke();
    ctx.fillStyle = '#666';
    ctx.font = '10px -apple-system, sans-serif';
    ctx.textAlign = 'right';
    ctx.fillText(v + '%', padL - 4, y + 3);
  }

  // X grid
  ctx.textAlign = 'center';
  var xStep = pidRangeSec <= 120 ? 30 : pidRangeSec <= 300 ? 60 : 120;
  for (var t = Math.ceil(tMin / xStep) * xStep; t <= tMax; t += xStep) {
    var x = tx(t);
    ctx.beginPath(); ctx.moveTo(x, padT); ctx.lineTo(x, padT + gH); ctx.stroke();
    ctx.fillStyle = '#666';
    ctx.font = '10px -apple-system, sans-serif';
    var ago = tMax - t;
    ctx.fillText(ago < 5 ? 'now' : fmtAgo(ago), x, H - 4);
  }

  // Helper: draw line
  function drawLine(data, key, color, scale, offset) {
    ctx.strokeStyle = color;
    ctx.lineWidth = 1.5;
    ctx.lineJoin = 'round';
    ctx.beginPath();
    var started = false;
    for (var i = 0; i < data.length; i++) {
      var val = data[i][key] * (scale || 1) + (offset || 0);
      var x = tx(data[i].t);
      var y = ty(val);
      if (!started) { ctx.moveTo(x, y); started = true; } else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }

  // Draw lines: Output (white, directly 0-100)
  drawLine(vis, 'out', 'rgba(255,255,255,0.9)', 1, 0);

  // PID terms: scale to visible range. Terms are typically -0.5 to +0.5
  // Map them: value * 100 + 50 so that 0 maps to 50% on chart
  ctx.globalAlpha = 0.7;
  // P (blue)
  ctx.strokeStyle = '#3498db';
  ctx.lineWidth = 1.2;
  ctx.beginPath();
  var started = false;
  for (var i = 0; i < vis.length; i++) {
    var val = vis[i].p * 100 + 50;
    val = Math.max(0, Math.min(100, val));
    var x = tx(vis[i].t), y = ty(val);
    if (!started) { ctx.moveTo(x, y); started = true; } else ctx.lineTo(x, y);
  }
  ctx.stroke();

  // I (green)
  ctx.strokeStyle = '#2ecc71';
  ctx.beginPath();
  started = false;
  for (var i = 0; i < vis.length; i++) {
    var val = vis[i].i * 100 + 50;
    val = Math.max(0, Math.min(100, val));
    var x = tx(vis[i].t), y = ty(val);
    if (!started) { ctx.moveTo(x, y); started = true; } else ctx.lineTo(x, y);
  }
  ctx.stroke();

  // D (orange)
  ctx.strokeStyle = '#ff6b35';
  ctx.beginPath();
  started = false;
  for (var i = 0; i < vis.length; i++) {
    var val = vis[i].d * 100 + 50;
    val = Math.max(0, Math.min(100, val));
    var x = tx(vis[i].t), y = ty(val);
    if (!started) { ctx.moveTo(x, y); started = true; } else ctx.lineTo(x, y);
  }
  ctx.stroke();
  ctx.globalAlpha = 1;
}
)rawliteral";

#endif