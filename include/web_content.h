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
                            <span class="temp-unit">°F</span>
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
                            <span class="temp-unit">°F</span>
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
                    <span class="sp-unit">°F</span>
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
                            <input type="number" id="temp-override" min="0" max="600" value="225" placeholder="°F">
                            <button class="btn btn-apply" onclick="setTempOverride()">Set</button>
                            <button class="btn btn-stop" onclick="clearTempOverride()">Clear</button>
                        </div>
                    </div>
                </div>
            </section>
        </main>

        <div id="tab-pid" class="tab-panel" style="display:none;">
            <!-- Animated Smoker Cross-Section -->
            <section class="card smoker-scene-card">
                <h3>How Your Smoker Works</h3>
                <canvas id="smoker-scene"></canvas>
                <div class="smoker-legend">
                    <span><span class="legend-swatch" style="background:#ff6b35"></span>Fire</span>
                    <span><span class="legend-swatch" style="background:#aaa"></span>Smoke</span>
                    <span><span class="legend-swatch" style="background:#8B6914"></span>Pellets</span>
                    <span><span class="legend-swatch" style="background:#3498db"></span>Air</span>
                </div>
            </section>

            <!-- PID Brain: Three Visual Metaphors -->
            <section class="card pid-brain-card">
                <h3>The PID Brain</h3>
                <div class="pid-brain-row">
                    <div class="brain-col">
                        <canvas id="pid-spring"></canvas>
                        <span class="brain-label" style="color:#3498db">Proportional</span>
                        <span class="brain-desc">How far am I from target?</span>
                    </div>
                    <div class="brain-col">
                        <canvas id="pid-bucket"></canvas>
                        <span class="brain-label" style="color:#2ecc71">Integral</span>
                        <span class="brain-desc">How long have I been off?</span>
                    </div>
                    <div class="brain-col">
                        <canvas id="pid-speedo"></canvas>
                        <span class="brain-label" style="color:#ff6b35">Derivative</span>
                        <span class="brain-desc">How fast am I changing?</span>
                    </div>
                </div>
            </section>

            <!-- Feedback Loop Flow -->
            <section class="card feedback-card">
                <canvas id="feedback-loop"></canvas>
            </section>

            <!-- PID Time Series Chart (retained) -->
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

/* PID Visualizer — Animated Educational View */
.smoker-scene-card { padding-bottom: 8px; }
#smoker-scene {
  width: 100%; height: 280px; display: block;
  border-radius: 6px; background: #0a0a0a;
}
.smoker-legend {
  display: flex; justify-content: center; gap: 14px;
  margin-top: 6px; font-size: 11px; color: var(--text2);
}
.smoker-legend span { display: flex; align-items: center; gap: 4px; }

.pid-brain-card { padding-bottom: 10px; }
.pid-brain-row {
  display: flex; gap: 12px;
}
.brain-col {
  flex: 1; display: flex; flex-direction: column;
  align-items: center; min-width: 0;
}
.brain-col canvas {
  width: 100%; height: 150px; display: block;
  border-radius: 6px; background: #0a0a0a;
}
.brain-label {
  font-size: 11px; font-weight: 700; text-transform: uppercase;
  letter-spacing: 1px; margin-top: 6px;
}
.brain-desc {
  font-size: 10px; color: #666; text-align: center;
  line-height: 1.3; margin-top: 2px;
}

.feedback-card { padding: 10px 14px; }
#feedback-loop {
  width: 100%; height: 90px; display: block;
  border-radius: 6px; background: #0a0a0a;
}

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
  .pid-brain-row { flex-direction: column; }
  .brain-col canvas { height: 120px; }
  #smoker-scene { height: 220px; }
  #feedback-loop { height: 70px; }
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
document.addEventListener('DOMContentLoaded', () => {
  fetchHistory();
  updateStatus();
  setInterval(updateStatus, POLL_MS);
  window.addEventListener('resize', function() { drawGraph(); if (document.getElementById('tab-pid') && document.getElementById('tab-pid').classList.contains('active')) { drawPidChart(); } });
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

  // Temperature ring (0-500°F range mapped to SVG circle)
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

  // Record PID data and update PID tab if visible
  recordPidSample(s);
  if (document.getElementById('tab-pid') && document.getElementById('tab-pid').classList.contains('active')) {
    updatePidVisuals(s);
  }

  // Append to graph
  appendGraphPoint(s);
}

// --- Temperature Graph ---
const STATE_NAMES = ['Idle','Startup','Running','Cooldown','Shutdown','Error','Reignite'];
const STATE_COLORS = ['#555','#ff6b35','#2ecc71','#3498db','#f1c40f','#e74c3c','#e67e22'];

async function fetchHistory() {
  try {
    const r = await fetch(API + '/history');
    if (!r.ok) return;
    const d = await r.json();
    // Compact format: samples are [time, temp×10, setpoint×10, state]
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

// --- Tab System ---
function switchTab(tab) {
  document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
  document.querySelectorAll('.tab-panel').forEach(p => p.classList.remove('active'));
  if (tab === 'pid') {
    document.querySelectorAll('.tab-btn')[1].classList.add('active');
    var el = document.getElementById('tab-pid');
    el.style.display = 'block';
    el.classList.add('active');
    document.getElementById('tab-dashboard').classList.remove('active');
    document.getElementById('tab-dashboard').style.display = 'none';
    // Trigger initial draw
    initParticles();
    startPidAnim();
    drawPidChart();
  } else {
    stopPidAnim();
    document.querySelectorAll('.tab-btn')[0].classList.add('active');
    var el = document.getElementById('tab-dashboard');
    el.style.display = 'block';
    el.classList.add('active');
    document.getElementById('tab-pid').classList.remove('active');
    document.getElementById('tab-pid').style.display = 'none';
    requestAnimationFrame(drawGraph);
  }
}

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

// --- PID Animated Educational Visualization ---

// Scene state updated from main updateUI loop
var pidSceneState = { fan: false, auger: false, igniter: false, temp: 70, setpoint: 225, output: 0 };

// Particle system - pre-allocated pool, zero GC during animation
var MAX_FIRE = 60;
var MAX_SMOKE = 40;
var MAX_PARTICLES = MAX_FIRE + MAX_SMOKE;
var particles = [];
var displayNeedleAngle = Math.PI * 0.5; // speedometer needle, starts center

function initParticles() {
  particles = [];
  for (var i = 0; i < MAX_PARTICLES; i++) {
    particles.push({ active: false, type: 'fire', x: 0, y: 0, vx: 0, vy: 0, life: 0, maxLife: 1, size: 3, opacity: 1 });
  }
}

function spawnFireParticle(x, y) {
  for (var i = 0; i < MAX_FIRE; i++) {
    if (!particles[i].active) {
      var p = particles[i];
      p.active = true;
      p.type = 'fire';
      p.x = x + (Math.random() - 0.5) * 16;
      p.y = y;
      p.vx = (Math.random() - 0.5) * 12;
      p.vy = -(30 + Math.random() * 40);
      p.life = 0;
      p.maxLife = 0.4 + Math.random() * 0.5;
      p.size = 2 + Math.random() * 3;
      p.opacity = 0.8 + Math.random() * 0.2;
      return;
    }
  }
}

function spawnSmokeParticle(x, y) {
  for (var i = MAX_FIRE; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) {
      var p = particles[i];
      p.active = true;
      p.type = 'smoke';
      p.x = x + (Math.random() - 0.5) * 6;
      p.y = y;
      p.vx = (Math.random() - 0.5) * 8;
      p.vy = -(10 + Math.random() * 15);
      p.life = 0;
      p.maxLife = 1.0 + Math.random() * 1.0;
      p.size = 3 + Math.random() * 3;
      p.opacity = 0.3 + Math.random() * 0.2;
      return;
    }
  }
}

function updateParticles(dt) {
  for (var i = 0; i < MAX_PARTICLES; i++) {
    var p = particles[i];
    if (!p.active) continue;
    p.life += dt;
    if (p.life >= p.maxLife) { p.active = false; continue; }
    p.x += p.vx * dt;
    p.y += p.vy * dt;
    if (p.type === 'fire') {
      p.size *= (1 - dt * 1.5);
      if (p.size < 0.3) p.size = 0.3;
    } else {
      p.size += dt * 4;
      p.vx += (Math.random() - 0.5) * 20 * dt;
    }
  }
}

// Animation loop controller
var pidAnimRunning = false;
var pidAnimId = 0;
var TARGET_FPS = 18;
var PID_FRAME_MS = 1000 / TARGET_FPS;
var pidLastFrameTime = 0;
var lastAugerAngle = 0;
var lastFanAngle = 0;

function pidAnimLoop(timestamp) {
  if (!pidAnimRunning) return;
  pidAnimId = requestAnimationFrame(pidAnimLoop);
  var elapsed = timestamp - pidLastFrameTime;
  if (elapsed < PID_FRAME_MS) return;
  var dt = Math.min(elapsed / 1000, 0.1);
  pidLastFrameTime = timestamp;

  updateParticles(dt);
  drawSmokerScene(timestamp);
  drawPidSpring(timestamp);
  drawPidBucket(timestamp);
  drawPidSpeedo(timestamp);
  drawFeedbackLoop(timestamp);
}

function startPidAnim() {
  if (pidAnimRunning) return;
  pidAnimRunning = true;
  pidLastFrameTime = performance.now();
  pidAnimId = requestAnimationFrame(pidAnimLoop);
}

function stopPidAnim() {
  pidAnimRunning = false;
  if (pidAnimId) { cancelAnimationFrame(pidAnimId); pidAnimId = 0; }
}

// Canvas DPR helper
function setupCanvas(id) {
  var c = document.getElementById(id);
  if (!c) return null;
  var dpr = window.devicePixelRatio || 1;
  var r = c.getBoundingClientRect();
  var W = r.width, H = r.height;
  if (W === 0 || H === 0) return null;
  c.width = W * dpr;
  c.height = H * dpr;
  var ctx = c.getContext('2d');
  ctx.scale(dpr, dpr);
  return { ctx: ctx, W: W, H: H };
}

// --- Smoker Scene Renderer ---
function drawSmokerScene(timestamp) {
  var s = setupCanvas('smoker-scene');
  if (!s) return;
  var ctx = s.ctx, W = s.W, H = s.H;
  var st = pidSceneState;
  var pid = pidLastData;

  ctx.fillStyle = '#0d0d0d';
  ctx.fillRect(0, 0, W, H);

  // Reference layout 400x280, scaled to fit canvas
  var scaleX = W / 400, scaleY = H / 280;
  var sc = Math.min(scaleX, scaleY);
  var ox = (W - 400 * sc) / 2;
  var oy = (H - 280 * sc) / 2;
  function sx(v) { return ox + v * sc; }
  function sy(v) { return oy + v * sc; }
  function sw(v) { return v * sc; }

  // Smoker body (main chamber)
  ctx.fillStyle = '#1e1e1e';
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.roundRect(sx(80), sy(60), sw(220), sw(160), sw(8));
  ctx.fill();
  ctx.stroke();

  // Hopper (left, trapezoid)
  ctx.fillStyle = '#2a2a2a';
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.moveTo(sx(10), sy(60));
  ctx.lineTo(sx(70), sy(60));
  ctx.lineTo(sx(60), sy(160));
  ctx.lineTo(sx(20), sy(160));
  ctx.closePath();
  ctx.fill();
  ctx.stroke();

  // Pellet dots inside hopper
  ctx.fillStyle = '#8B6914';
  var pelletSeed = 42;
  for (var pi = 0; pi < 18; pi++) {
    pelletSeed = (pelletSeed * 1103515245 + 12345) & 0x7fffffff;
    var px = sx(25 + (pelletSeed % 35));
    pelletSeed = (pelletSeed * 1103515245 + 12345) & 0x7fffffff;
    var py = sy(75 + (pelletSeed % 75));
    ctx.beginPath();
    ctx.arc(px, py, sw(2.5), 0, Math.PI * 2);
    ctx.fill();
  }

  // Auger tube
  ctx.fillStyle = '#333';
  ctx.fillRect(sx(60), sy(150), sw(50), sw(14));
  ctx.strokeStyle = '#555';
  ctx.strokeRect(sx(60), sy(150), sw(50), sw(14));

  // Auger screw (sine wave inside tube, rotates when auger ON)
  if (pid.augerOn) lastAugerAngle += 0.15;
  ctx.strokeStyle = '#888';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  for (var ai = 0; ai <= 30; ai++) {
    var at = ai / 30;
    var ax = sx(62 + at * 46);
    var ay = sy(157) + Math.sin(at * 8 * Math.PI + lastAugerAngle) * sw(4);
    if (ai === 0) ctx.moveTo(ax, ay); else ctx.lineTo(ax, ay);
  }
  ctx.stroke();

  // Firepot
  var fpX = sx(120), fpY = sy(190), fpW = sw(60), fpH = sw(28);
  ctx.fillStyle = '#2a2a2a';
  ctx.strokeStyle = '#666';
  ctx.lineWidth = 1.5;
  ctx.fillRect(fpX, fpY, fpW, fpH);
  ctx.strokeRect(fpX, fpY, fpW, fpH);

  // Spawn fire particles (rate proportional to PID output)
  if (st.output > 5 && st.temp > 80) {
    var spawnRate = Math.max(1, Math.floor(st.output / 25));
    for (var fi = 0; fi < spawnRate; fi++) {
      spawnFireParticle(120 + 30, 190 - 5);
    }
  }

  // Draw fire particles
  for (var i = 0; i < MAX_FIRE; i++) {
    var p = particles[i];
    if (!p.active) continue;
    var frac = p.life / p.maxLife;
    var hue = 20 + (1 - frac) * 25;
    var light = 90 - frac * 40;
    var alpha = p.opacity * (1 - frac);
    ctx.globalAlpha = alpha;
    ctx.fillStyle = 'hsl(' + hue + ',100%,' + light + '%)';
    ctx.beginPath();
    ctx.arc(sx(p.x), sy(p.y), sw(p.size), 0, Math.PI * 2);
    ctx.fill();
  }
  ctx.globalAlpha = 1;

  // Deflector plate
  ctx.strokeStyle = '#777';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(sx(105), sy(155));
  ctx.lineTo(sx(200), sy(140));
  ctx.stroke();

  // Grate (dashed lines)
  ctx.strokeStyle = '#666';
  ctx.lineWidth = 1;
  ctx.setLineDash([sw(4), sw(3)]);
  for (var gi = 0; gi < 3; gi++) {
    var gy = sy(110 + gi * 12);
    ctx.beginPath();
    ctx.moveTo(sx(95), gy);
    ctx.lineTo(sx(270), gy);
    ctx.stroke();
  }
  ctx.setLineDash([]);

  // Food shapes on grate
  ctx.fillStyle = '#8B4513';
  ctx.beginPath();
  ctx.ellipse(sx(160), sy(104), sw(22), sw(8), 0, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = '#A0522D';
  ctx.beginPath();
  ctx.ellipse(sx(220), sy(106), sw(16), sw(6), 0.2, 0, Math.PI * 2);
  ctx.fill();

  // Chimney
  ctx.fillStyle = '#1e1e1e';
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1.5;
  ctx.fillRect(sx(250), sy(10), sw(24), sw(55));
  ctx.strokeRect(sx(250), sy(10), sw(24), sw(55));
  ctx.fillStyle = '#333';
  ctx.fillRect(sx(245), sy(8), sw(34), sw(6));

  // Spawn smoke at chimney top
  if (st.output > 5 && st.temp > 80) {
    if (Math.random() < 0.3) spawnSmokeParticle(262, 8);
  }

  // Draw smoke particles
  for (var i = MAX_FIRE; i < MAX_PARTICLES; i++) {
    var p = particles[i];
    if (!p.active) continue;
    var frac = p.life / p.maxLife;
    var alpha = p.opacity * (1 - frac);
    ctx.globalAlpha = alpha;
    ctx.fillStyle = '#888';
    ctx.beginPath();
    ctx.arc(sx(p.x), sy(p.y), sw(p.size), 0, Math.PI * 2);
    ctx.fill();
  }
  ctx.globalAlpha = 1;

  // Fan (bottom left, 3 rotating blades)
  var fanCx = sx(50), fanCy = sy(210), fanR = sw(16);
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.arc(fanCx, fanCy, fanR, 0, Math.PI * 2);
  ctx.stroke();
  ctx.fillStyle = '#444';
  ctx.beginPath();
  ctx.arc(fanCx, fanCy, sw(3), 0, Math.PI * 2);
  ctx.fill();
  if (st.fan) lastFanAngle += 0.2;
  ctx.fillStyle = '#777';
  for (var bi = 0; bi < 3; bi++) {
    var ba = lastFanAngle + bi * (Math.PI * 2 / 3);
    ctx.beginPath();
    ctx.moveTo(fanCx, fanCy);
    ctx.arc(fanCx, fanCy, fanR - sw(2), ba - 0.3, ba + 0.3);
    ctx.closePath();
    ctx.fill();
  }

  // Air flow dashes (fan to firepot)
  if (st.fan) {
    ctx.strokeStyle = hexAlpha('#3498db', 0.5);
    ctx.lineWidth = 1.5;
    ctx.setLineDash([sw(3), sw(4)]);
    var airPhase = (timestamp / 300) % 20;
    for (var afi = 0; afi < 5; afi++) {
      var afX = sx(75 + ((afi * 10 + airPhase) % 50));
      ctx.beginPath();
      ctx.moveTo(afX, sy(205));
      ctx.lineTo(afX + sw(6), sy(200));
      ctx.stroke();
    }
    ctx.setLineDash([]);
  }

  // Thermometer (right side)
  var thX = sx(310), thY = sy(75), thH = sw(130), thW = sw(8);
  ctx.fillStyle = '#222';
  ctx.beginPath();
  ctx.roundRect(thX, thY, thW, thH, sw(4));
  ctx.fill();
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.roundRect(thX, thY, thW, thH, sw(4));
  ctx.stroke();

  // Mercury fill
  var tempFrac = Math.max(0, Math.min(1, (st.temp - 50) / 450));
  var mercuryH = thH * tempFrac;
  ctx.fillStyle = '#e74c3c';
  ctx.save();
  ctx.beginPath();
  ctx.roundRect(thX, thY, thW, thH, sw(4));
  ctx.clip();
  ctx.fillRect(thX, thY + thH - mercuryH, thW, mercuryH);
  ctx.restore();

  // Setpoint tick
  var spFrac = Math.max(0, Math.min(1, (st.setpoint - 50) / 450));
  var spTickY = thY + thH - thH * spFrac;
  ctx.strokeStyle = '#2ecc71';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(thX - sw(3), spTickY);
  ctx.lineTo(thX + thW + sw(3), spTickY);
  ctx.stroke();

  // Temp label
  ctx.fillStyle = '#ff6b35';
  ctx.font = 'bold ' + sw(11) + 'px SF Mono, Consolas, monospace';
  ctx.textAlign = 'left';
  ctx.fillText(st.temp.toFixed(0) + '\u00B0F', thX + thW + sw(6), thY + thH - mercuryH + sw(4));
  ctx.fillStyle = '#2ecc71';
  ctx.font = sw(9) + 'px SF Mono, Consolas, monospace';
  ctx.fillText(st.setpoint.toFixed(0) + '\u00B0', thX + thW + sw(6), spTickY + sw(3));

  // Lid Open overlay
  if (pid.lidOpen) {
    ctx.fillStyle = 'rgba(200, 0, 0, 0.15)';
    ctx.fillRect(0, 0, W, H);
    ctx.fillStyle = 'rgba(200, 0, 0, 0.7)';
    var lbW = sw(100), lbH = sw(24);
    ctx.beginPath();
    ctx.roundRect((W - lbW) / 2, sy(30), lbW, lbH, sw(4));
    ctx.fill();
    ctx.fillStyle = '#fff';
    ctx.font = 'bold ' + sw(13) + 'px -apple-system, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('LID OPEN', W / 2, sy(30) + lbH / 2 + sw(4));
  }

  // Reignite badge
  var reigniteAttempts = pid.reigniteAttempts || 0;
  if (reigniteAttempts > 0) {
    var badgeR = sw(11);
    var badgeX = W - sw(18), badgeY = sw(18);
    ctx.fillStyle = '#e74c3c';
    ctx.beginPath();
    ctx.arc(badgeX, badgeY, badgeR, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = '#fff';
    ctx.font = 'bold ' + sw(10) + 'px -apple-system, sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(reigniteAttempts, badgeX, badgeY);
    ctx.textBaseline = 'alphabetic';
  }

  // Labels
  ctx.fillStyle = '#666';
  ctx.font = sw(9) + 'px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('HOPPER', sx(40), sy(50));
  ctx.fillText('FAN', sx(50), sy(240));
  ctx.fillText('FIREPOT', sx(150), sy(230));
  ctx.fillText('CHIMNEY', sx(262), sy(55));
}

// --- PID Spring (P-term: "How far am I from target?") ---
function drawPidSpring(timestamp) {
  var s = setupCanvas('pid-spring');
  if (!s) return;
  var ctx = s.ctx, W = s.W, H = s.H;
  var pid = pidLastData;
  var pVal = parseFloat(pid.p) || 0;
  var err = parseFloat(pid.error) || 0;

  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  var cy = H * 0.42;
  var margin = 20;
  var anchorR = Math.min(8, W * 0.05);

  // Error maps to spring stretch
  var errClamped = Math.max(-30, Math.min(30, err));
  var errNorm = errClamped / 30;
  var targetX = W - margin - 10;
  var nowX = margin + 10 + errNorm * (W * 0.15);

  var errAbs = Math.abs(errNorm);
  var coils = 6 + (1 - errAbs) * 6;
  var amplitude = 4 + errAbs * (H * 0.18);
  var segments = 80;

  var springColor = errAbs < 0.15 ? '#2ecc71' : errAbs < 0.5 ? '#ff6b35' : '#e74c3c';

  // Draw spring (sine wave between anchors)
  ctx.strokeStyle = springColor;
  ctx.lineWidth = 2;
  ctx.beginPath();
  for (var i = 0; i <= segments; i++) {
    var t = i / segments;
    var x = nowX + (targetX - nowX) * t;
    var y = cy + Math.sin(t * coils * Math.PI * 2) * amplitude;
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.stroke();

  // NOW anchor (orange)
  ctx.fillStyle = '#ff6b35';
  ctx.beginPath();
  ctx.arc(nowX, cy, anchorR, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = '#fff';
  ctx.font = 'bold 9px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('NOW', nowX, cy - anchorR - 4);
  ctx.fillStyle = '#ff6b35';
  ctx.font = '10px SF Mono, Consolas, monospace';
  ctx.fillText(pidSceneState.temp.toFixed(0) + '\u00B0', nowX, cy + anchorR + 13);

  // TARGET anchor (green)
  ctx.fillStyle = '#2ecc71';
  ctx.beginPath();
  ctx.arc(targetX, cy, anchorR, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = '#fff';
  ctx.font = 'bold 9px -apple-system, sans-serif';
  ctx.fillText('TARGET', targetX, cy - anchorR - 4);
  ctx.fillStyle = '#2ecc71';
  ctx.font = '10px SF Mono, Consolas, monospace';
  ctx.fillText(pidSceneState.setpoint.toFixed(0) + '\u00B0', targetX, cy + anchorR + 13);

  // Error label in the middle
  var midX = (nowX + targetX) / 2;
  ctx.fillStyle = err > 0 ? '#e74c3c' : err < -0.5 ? '#3498db' : '#888';
  ctx.font = '11px SF Mono, Consolas, monospace';
  ctx.textAlign = 'center';
  ctx.fillText(err.toFixed(1) + '\u00B0F error', midX, cy - amplitude - 10);

  // P value
  ctx.fillStyle = '#3498db';
  ctx.font = 'bold 12px SF Mono, Consolas, monospace';
  ctx.fillText('P = ' + pVal.toFixed(3), W / 2, H - 8);
}

// --- PID Bucket (I-term: "How long have I been off?") ---
function drawPidBucket(timestamp) {
  var s = setupCanvas('pid-bucket');
  if (!s) return;
  var ctx = s.ctx, W = s.W, H = s.H;
  var pid = pidLastData;
  var iVal = parseFloat(pid.i) || 0;
  var err = parseFloat(pid.error) || 0;

  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  var cx = W / 2;
  var bucketTop = 18;
  var bucketBot = H - 28;
  var bucketH = bucketBot - bucketTop;
  var topHalfW = W * 0.35;
  var botHalfW = W * 0.25;

  // Fill level: iTerm roughly -0.5 to +0.5 -> 0..1
  var fillLevel = Math.max(0, Math.min(1, iVal + 0.5));

  // Bucket outline
  ctx.strokeStyle = '#666';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(cx - topHalfW, bucketTop);
  ctx.lineTo(cx - botHalfW, bucketBot);
  ctx.lineTo(cx + botHalfW, bucketBot);
  ctx.lineTo(cx + topHalfW, bucketTop);
  ctx.stroke();

  // Liquid fill (clipped to bucket shape)
  if (fillLevel > 0.01) {
    ctx.save();
    ctx.beginPath();
    ctx.moveTo(cx - topHalfW, bucketTop);
    ctx.lineTo(cx - botHalfW, bucketBot);
    ctx.lineTo(cx + botHalfW, bucketBot);
    ctx.lineTo(cx + topHalfW, bucketTop);
    ctx.closePath();
    ctx.clip();

    var fillY = bucketBot - bucketH * fillLevel;
    ctx.fillStyle = hexAlpha('#2ecc71', 0.5);
    ctx.beginPath();
    var waveSegs = 30;
    for (var wi = 0; wi <= waveSegs; wi++) {
      var wt = wi / waveSegs;
      var wx = (cx - topHalfW - 10) + (topHalfW * 2 + 20) * wt;
      var wy = fillY + Math.sin(wt * Math.PI * 4 + timestamp / 400) * 2;
      if (wi === 0) ctx.moveTo(wx, wy); else ctx.lineTo(wx, wy);
    }
    ctx.lineTo(cx + topHalfW + 10, bucketBot + 5);
    ctx.lineTo(cx - topHalfW - 10, bucketBot + 5);
    ctx.closePath();
    ctx.fill();
    ctx.restore();
  }

  // Balanced line at 50%
  var balY = bucketTop + bucketH * 0.5;
  var balLeftW = botHalfW + (topHalfW - botHalfW) * 0.5;
  ctx.strokeStyle = '#888';
  ctx.lineWidth = 1;
  ctx.setLineDash([3, 3]);
  ctx.beginPath();
  ctx.moveTo(cx - balLeftW, balY);
  ctx.lineTo(cx + balLeftW, balY);
  ctx.stroke();
  ctx.setLineDash([]);
  ctx.fillStyle = '#888';
  ctx.font = '8px -apple-system, sans-serif';
  ctx.textAlign = 'right';
  ctx.fillText('balanced', cx - balLeftW - 3, balY + 3);

  // Dripping drops when error is nonzero
  if (Math.abs(err) > 0.5) {
    var dropPhase = (timestamp / 600) % 1;
    var dropX = cx + (Math.sin(timestamp / 700) * W * 0.08);
    var dropY = 5 + dropPhase * (bucketTop + 5);
    ctx.fillStyle = hexAlpha('#2ecc71', 0.6 * (1 - dropPhase));
    ctx.beginPath();
    ctx.arc(dropX, dropY, 2.5, 0, Math.PI * 2);
    ctx.fill();
  }

  // Warning glow when very full
  if (fillLevel > 0.8) {
    ctx.strokeStyle = hexAlpha('#e74c3c', 0.3 + 0.15 * Math.sin(timestamp / 300));
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(cx - topHalfW, bucketTop);
    ctx.lineTo(cx - botHalfW, bucketBot);
    ctx.lineTo(cx + botHalfW, bucketBot);
    ctx.lineTo(cx + topHalfW, bucketTop);
    ctx.stroke();
  }

  // I value
  ctx.fillStyle = '#2ecc71';
  ctx.font = 'bold 12px SF Mono, Consolas, monospace';
  ctx.textAlign = 'center';
  ctx.fillText('I = ' + iVal.toFixed(3), cx, H - 6);
}

// --- PID Speedometer (D-term: "How fast am I changing?") ---
function drawPidSpeedo(timestamp) {
  var s = setupCanvas('pid-speedo');
  if (!s) return;
  var ctx = s.ctx, W = s.W, H = s.H;
  var pid = pidLastData;
  var dVal = parseFloat(pid.d) || 0;

  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  var cx = W / 2;
  var cy = H * 0.55;
  var r = Math.min(W, H) * 0.36;

  // D-term ranges roughly -0.5 to +0.5 -> angle PI(left) to 0(right)
  var dClamped = Math.max(-0.5, Math.min(0.5, dVal));
  var targetAngle = Math.PI * (0.5 - dClamped);
  displayNeedleAngle += (targetAngle - displayNeedleAngle) * 0.1;

  // Three color zones
  var zoneAngles = [Math.PI, Math.PI * 0.67, Math.PI * 0.33, 0];
  var zoneColors = ['#3498db', '#2ecc71', '#ff6b35'];

  ctx.lineWidth = 8;
  ctx.lineCap = 'butt';
  for (var zi = 0; zi < 3; zi++) {
    ctx.strokeStyle = hexAlpha(zoneColors[zi], 0.35);
    ctx.beginPath();
    ctx.arc(cx, cy, r, zoneAngles[zi], zoneAngles[zi + 1], true);
    ctx.stroke();
  }

  // Tick marks
  ctx.lineWidth = 1;
  ctx.strokeStyle = '#555';
  for (var ti = 0; ti <= 10; ti++) {
    var ta = Math.PI - (ti / 10) * Math.PI;
    var t1 = r - 5, t2 = r + 5;
    ctx.beginPath();
    ctx.moveTo(cx + t1 * Math.cos(ta), cy + t1 * Math.sin(ta));
    ctx.lineTo(cx + t2 * Math.cos(ta), cy + t2 * Math.sin(ta));
    ctx.stroke();
  }

  // Zone labels
  ctx.font = '8px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillStyle = '#3498db';
  ctx.fillText('COOLING', cx - r * 0.7, cy + r * 0.4);
  ctx.fillStyle = '#2ecc71';
  ctx.fillText('STABLE', cx, cy - r * 0.15);
  ctx.fillStyle = '#ff6b35';
  ctx.fillText('HEATING', cx + r * 0.7, cy + r * 0.4);

  // Needle
  var needleLen = r - 10;
  var needleAngle = displayNeedleAngle;
  var needleNorm = needleAngle / Math.PI;
  var needleColor = needleNorm > 0.67 ? '#3498db' : needleNorm > 0.33 ? '#2ecc71' : '#ff6b35';
  ctx.strokeStyle = needleColor;
  ctx.lineWidth = 2.5;
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.lineTo(cx + needleLen * Math.cos(needleAngle), cy - needleLen * Math.sin(needleAngle));
  ctx.stroke();

  // Center pivot
  ctx.fillStyle = '#fff';
  ctx.beginPath();
  ctx.arc(cx, cy, 4, 0, Math.PI * 2);
  ctx.fill();

  // D value
  ctx.fillStyle = '#ff6b35';
  ctx.font = 'bold 12px SF Mono, Consolas, monospace';
  ctx.textAlign = 'center';
  ctx.fillText('D = ' + dVal.toFixed(3), cx, H - 6);
}

// --- Feedback Loop Renderer ---
function drawFeedbackLoop(timestamp) {
  var s = setupCanvas('feedback-loop');
  if (!s) return;
  var ctx = s.ctx, W = s.W, H = s.H;
  var pid = pidLastData;
  var out = parseFloat(pid.output) || 0;
  var err = parseFloat(pid.error) || 0;
  var temp = pidSceneState.temp;
  var setpoint = pidSceneState.setpoint;

  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  var nodeCount = 6;
  var nodeR = Math.min(16, W * 0.035);
  var topY = H * 0.5;
  var margin = Math.max(24, W * 0.05);
  var spacing = (W - margin * 2) / (nodeCount - 1);

  var nodes = [
    { label: 'TARGET', value: setpoint.toFixed(0) + '\u00B0', color: '#2ecc71' },
    { label: 'ERROR', value: err.toFixed(1) + '\u00B0', color: err > 0 ? '#e74c3c' : '#3498db' },
    { label: 'PID', value: '', color: '#3498db' },
    { label: 'OUTPUT', value: out.toFixed(0) + '%', color: '#fff' },
    { label: 'SMOKER', value: '', color: '#ff6b35' },
    { label: 'TEMP', value: temp.toFixed(0) + '\u00B0', color: '#ff6b35' }
  ];

  var icons = ['\u2316', '\u00B1', 'PID', '%', '\u2248', '\u00B0'];
  var animPhase = (timestamp / 1500) % 1;

  // Arrows and flow dots between nodes
  for (var ni = 0; ni < nodeCount - 1; ni++) {
    var x1 = margin + ni * spacing + nodeR + 2;
    var x2 = margin + (ni + 1) * spacing - nodeR - 2;
    ctx.strokeStyle = '#444';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(x1, topY);
    ctx.lineTo(x2, topY);
    ctx.stroke();
    ctx.fillStyle = '#444';
    ctx.beginPath();
    ctx.moveTo(x2, topY);
    ctx.lineTo(x2 - 5, topY - 3);
    ctx.lineTo(x2 - 5, topY + 3);
    ctx.closePath();
    ctx.fill();
    for (var di = 0; di < 2; di++) {
      var dt = ((animPhase + di * 0.5) % 1);
      var dx = x1 + (x2 - x1) * dt;
      ctx.globalAlpha = 0.3 + 0.5 * Math.sin(dt * Math.PI);
      ctx.fillStyle = nodes[ni].color;
      ctx.beginPath();
      ctx.arc(dx, topY, 2, 0, Math.PI * 2);
      ctx.fill();
    }
    ctx.globalAlpha = 1;
  }

  // Feedback arrow (Temp back to Error)
  var fbX1 = margin + 5 * spacing;
  var fbX2 = margin + 1 * spacing;
  var fbY = topY + nodeR + 16;
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1;
  ctx.setLineDash([4, 3]);
  ctx.beginPath();
  ctx.moveTo(fbX1, topY + nodeR);
  ctx.lineTo(fbX1, fbY);
  ctx.lineTo(fbX2, fbY);
  ctx.lineTo(fbX2, topY + nodeR);
  ctx.stroke();
  ctx.setLineDash([]);
  ctx.fillStyle = '#555';
  ctx.beginPath();
  ctx.moveTo(fbX2, topY + nodeR);
  ctx.lineTo(fbX2 - 3, topY + nodeR + 5);
  ctx.lineTo(fbX2 + 3, topY + nodeR + 5);
  ctx.closePath();
  ctx.fill();
  ctx.fillStyle = '#555';
  ctx.font = '8px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('measurement', (fbX1 + fbX2) / 2, fbY - 2);

  // Draw node circles
  for (var ni = 0; ni < nodeCount; ni++) {
    var nx = margin + ni * spacing;
    var node = nodes[ni];
    ctx.fillStyle = hexAlpha(node.color, 0.12);
    ctx.strokeStyle = node.color;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.arc(nx, topY, nodeR, 0, Math.PI * 2);
    ctx.fill();
    ctx.stroke();
    ctx.fillStyle = node.color;
    ctx.font = 'bold ' + Math.min(10, nodeR * 0.7) + 'px SF Mono, Consolas, monospace';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(icons[ni], nx, topY);
    ctx.textBaseline = 'alphabetic';
    ctx.fillStyle = '#666';
    ctx.font = '7px -apple-system, sans-serif';
    ctx.fillText(node.label, nx, topY - nodeR - 3);
    if (node.value) {
      ctx.fillStyle = node.color;
      ctx.font = 'bold 9px SF Mono, Consolas, monospace';
      ctx.fillText(node.value, nx, topY + nodeR + 12);
    }
  }

  // Speech bubble annotation
  var msg = '';
  var errAbs = Math.abs(err);
  if (pid.lidOpen) msg = 'Lid open! Holding steady...';
  else if ((pid.reigniteAttempts || 0) > 0) msg = 'Relighting the fire...';
  else if (out >= 99) msg = 'Maximum heat!';
  else if (out <= 16 && errAbs < 3) msg = 'Minimal fuel \u2014 coasting';
  else if (errAbs > 10) msg = errAbs.toFixed(0) + '\u00B0F off \u2014 compensating!';
  else if (errAbs < 2) msg = 'Right on target';
  else msg = 'Adjusting \u2014 ' + err.toFixed(1) + '\u00B0F error';

  if (msg) {
    ctx.font = '10px -apple-system, sans-serif';
    var tw = ctx.measureText(msg).width;
    var bubW = tw + 16, bubH = 18;
    var bubX = (W - bubW) / 2, bubY = 2;
    ctx.fillStyle = 'rgba(30,30,30,0.85)';
    ctx.beginPath();
    ctx.roundRect(bubX, bubY, bubW, bubH, 4);
    ctx.fill();
    ctx.strokeStyle = '#555';
    ctx.lineWidth = 0.5;
    ctx.beginPath();
    ctx.roundRect(bubX, bubY, bubW, bubH, 4);
    ctx.stroke();
    ctx.fillStyle = 'rgba(30,30,30,0.85)';
    ctx.beginPath();
    ctx.moveTo(W / 2 - 4, bubY + bubH);
    ctx.lineTo(W / 2, bubY + bubH + 4);
    ctx.lineTo(W / 2 + 4, bubY + bubH);
    ctx.closePath();
    ctx.fill();
    ctx.fillStyle = '#ccc';
    ctx.textAlign = 'center';
    ctx.fillText(msg, W / 2, bubY + bubH / 2 + 3.5);
  }
}

// --- Bridge: update scene state from main status poll ---
function updatePidVisuals(s) {
  if (!s.pid) return;
  var pid = s.pid;
  pidSceneState.fan = s.fan;
  pidSceneState.auger = s.auger;
  pidSceneState.igniter = !!s.igniter;
  pidSceneState.temp = s.temp;
  pidSceneState.setpoint = s.setpoint;
  pidSceneState.output = parseFloat(pid.output) || 0;
  drawPidChart();
}

// --- PID History Time-Series Chart ---
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


#endif // WEB_CONTENT_H
