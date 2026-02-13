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
  window.addEventListener('resize', function() { drawGraph(); if (document.getElementById('tab-pid') && document.getElementById('tab-pid').classList.contains('active')) { drawPidDiagram(); drawPidTermsBar(); drawAugerGauge(); drawPidChart(); } });
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
    requestAnimationFrame(function() { drawPidDiagram(); drawPidTermsBar(); drawAugerGauge(); drawPidChart(); });
  } else {
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

// --- PID Visual Updates ---
function updatePidVisuals(s) {
  if (!s.pid) return;
  var pid = s.pid;

  // Update numeric values
  document.getElementById('pv-p').textContent = parseFloat(pid.p).toFixed(3);
  document.getElementById('pv-i').textContent = parseFloat(pid.i).toFixed(3);
  document.getElementById('pv-d').textContent = parseFloat(pid.d).toFixed(3);
  document.getElementById('pv-out').textContent = parseFloat(pid.output).toFixed(1) + '%';
  document.getElementById('pv-duty').textContent = parseFloat(pid.output).toFixed(0) + '%';
  var remaining = Math.max(0, Math.round((parseInt(pid.cycleRemaining) || 0) / 1000));
  document.getElementById('pv-cycle').textContent = remaining + 's';
  document.getElementById('pv-reignite').textContent = pid.reigniteAttempts || 0;

  // Status indicators
  var lidInd = document.getElementById('pid-ind-lid');
  if (pid.lidOpen) lidInd.classList.add('active'); else lidInd.classList.remove('active');

  var reigniteInd = document.getElementById('pid-ind-reignite');
  if ((pid.reigniteAttempts || 0) > 0) reigniteInd.classList.add('active'); else reigniteInd.classList.remove('active');

  var satInd = document.getElementById('pid-ind-saturated');
  if (parseFloat(pid.output) >= 99.5 || parseFloat(pid.output) <= 15.5) satInd.classList.add('active'); else satInd.classList.remove('active');

  // Redraw canvases
  drawPidDiagram();
  drawPidTermsBar();
  drawAugerGauge();
  drawPidChart();
}

// --- PID Block Diagram (Canvas) ---
function drawPidDiagram() {
  var canvas = document.getElementById('pid-block-diagram');
  if (!canvas) return;
  var dpr = window.devicePixelRatio || 1;
  var rect = canvas.getBoundingClientRect();
  var W = rect.width, H = rect.height;
  if (W === 0 || H === 0) return;
  canvas.width = W * dpr;
  canvas.height = H * dpr;
  var ctx = canvas.getContext('2d');
  ctx.scale(dpr, dpr);

  var pid = pidLastData;
  var p = parseFloat(pid.p) || 0;
  var i = parseFloat(pid.i) || 0;
  var d = parseFloat(pid.d) || 0;
  var out = parseFloat(pid.output) || 0;
  var err = parseFloat(pid.error) || 0;

  // Clear
  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, W, H);

  // Layout
  var cy = H / 2;           // vertical center
  var margin = 16;
  var boxH = 36;
  var boxW = 56;
  var sumR = 14;             // summing junction radius

  // Positions (left to right flow)
  var xSp = margin + 30;            // setpoint label
  var xSum = xSp + 60;              // summing junction
  var xPID = xSum + 65;             // PID block center
  var xOut = xPID + boxW/2 + 50;    // output label
  var xPlant = xOut + 50;           // plant block
  var xTemp = Math.min(xPlant + boxW/2 + 40, W - margin - 20);  // temp output

  // Flow arrow color based on output
  var flowColor = out > 80 ? '#e74c3c' : out > 40 ? '#ff6b35' : '#2ecc71';
  var animPhase = (Date.now() % 2000) / 2000;  // 0-1 animation phase

  // Helper: draw block
  function drawBlock(x, y, w, h, label, value, color) {
    ctx.fillStyle = hexAlpha(color, 0.15);
    ctx.strokeStyle = color;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.roundRect(x - w/2, y - h/2, w, h, 4);
    ctx.fill(); ctx.stroke();
    ctx.fillStyle = '#999';
    ctx.font = '9px -apple-system, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText(label, x, y - 5);
    ctx.fillStyle = color;
    ctx.font = 'bold 12px SF Mono, Consolas, monospace';
    ctx.fillText(value, x, y + 10);
  }

  // Helper: draw arrow
  function drawArrow(x1, y1, x2, y2, color) {
    ctx.strokeStyle = color;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.stroke();
    // Arrowhead
    var angle = Math.atan2(y2 - y1, x2 - x1);
    var aLen = 6;
    ctx.beginPath();
    ctx.moveTo(x2, y2);
    ctx.lineTo(x2 - aLen * Math.cos(angle - 0.4), y2 - aLen * Math.sin(angle - 0.4));
    ctx.lineTo(x2 - aLen * Math.cos(angle + 0.4), y2 - aLen * Math.sin(angle + 0.4));
    ctx.closePath();
    ctx.fillStyle = color;
    ctx.fill();
  }

  // Helper: animated flow dots
  function drawFlowDots(x1, y1, x2, y2, color) {
    var dx = x2 - x1, dy = y2 - y1;
    var len = Math.sqrt(dx*dx + dy*dy);
    var dots = 3;
    for (var j = 0; j < dots; j++) {
      var t = ((animPhase + j / dots) % 1);
      var fx = x1 + dx * t, fy = y1 + dy * t;
      ctx.beginPath();
      ctx.arc(fx, fy, 2, 0, Math.PI * 2);
      ctx.fillStyle = hexAlpha(color, 0.4 + 0.4 * Math.sin(t * Math.PI));
      ctx.fill();
    }
  }

  // --- Draw the diagram ---

  // Setpoint label
  ctx.fillStyle = '#fff';
  ctx.font = 'bold 13px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('SP', xSp, cy - 10);
  ctx.fillStyle = '#aaa';
  ctx.font = '11px SF Mono, Consolas, monospace';
  ctx.fillText((parseFloat(pidLastData.error) ? (parseFloat(pidLastData.error) + (pidHistory.length > 0 ? pidHistory[pidHistory.length-1].sp : 225)).toFixed(0) : '--') + '\u00B0', xSp, cy + 6);

  // Arrow: SP -> Sum
  drawArrow(xSp + 20, cy, xSum - sumR, cy, '#888');
  drawFlowDots(xSp + 20, cy, xSum - sumR, cy, '#888');

  // Summing junction (circle with +/-)
  ctx.strokeStyle = '#888';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  ctx.arc(xSum, cy, sumR, 0, Math.PI * 2);
  ctx.stroke();
  ctx.fillStyle = '#aaa';
  ctx.font = 'bold 11px sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('+', xSum - 5, cy - 2);
  ctx.fillText('\u2013', xSum + 6, cy + 10);

  // Error value below summing junction
  ctx.fillStyle = err > 0 ? '#e74c3c' : err < 0 ? '#3498db' : '#888';
  ctx.font = '10px SF Mono, Consolas, monospace';
  ctx.fillText('e=' + err.toFixed(1), xSum, cy + sumR + 14);

  // Arrow: Sum -> PID
  drawArrow(xSum + sumR, cy, xPID - boxW/2, cy, flowColor);
  drawFlowDots(xSum + sumR, cy, xPID - boxW/2, cy, flowColor);

  // PID Block (show 3 sub-blocks stacked)
  var pidTop = cy - 46;
  var pidBlockH = 28;
  var pidGap = 4;
  // P block
  drawBlock(xPID, pidTop + pidBlockH/2, boxW, pidBlockH, 'P', p.toFixed(3), '#3498db');
  // I block
  drawBlock(xPID, pidTop + pidBlockH + pidGap + pidBlockH/2, boxW, pidBlockH, 'I', i.toFixed(3), '#2ecc71');
  // D block
  drawBlock(xPID, pidTop + 2*(pidBlockH + pidGap) + pidBlockH/2, boxW, pidBlockH, 'D', d.toFixed(3), '#ff6b35');

  // Sum symbol after PID blocks
  var xPidSum = xPID + boxW/2 + 20;
  ctx.strokeStyle = '#888';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.arc(xPidSum, cy, 8, 0, Math.PI * 2);
  ctx.stroke();
  ctx.fillStyle = '#aaa';
  ctx.font = '10px sans-serif';
  ctx.fillText('\u03A3', xPidSum, cy + 3);

  // Lines from P/I/D to sum circle
  var pBlockRight = xPID + boxW/2;
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1;
  [pidTop + pidBlockH/2, pidTop + pidBlockH + pidGap + pidBlockH/2, pidTop + 2*(pidBlockH + pidGap) + pidBlockH/2].forEach(function(by) {
    ctx.beginPath();
    ctx.moveTo(pBlockRight, by);
    ctx.lineTo(xPidSum - 8, cy);
    ctx.stroke();
  });

  // Arrow: PID Sum -> Output
  drawArrow(xPidSum + 8, cy, xOut - 16, cy, flowColor);
  drawFlowDots(xPidSum + 8, cy, xOut - 16, cy, flowColor);

  // Output value
  ctx.fillStyle = '#fff';
  ctx.font = 'bold 14px SF Mono, Consolas, monospace';
  ctx.textAlign = 'center';
  ctx.fillText(out.toFixed(1) + '%', xOut, cy - 2);
  ctx.fillStyle = '#666';
  ctx.font = '9px -apple-system, sans-serif';
  ctx.fillText('OUTPUT', xOut, cy + 12);

  // Arrow: Output -> Plant
  drawArrow(xOut + 26, cy, xPlant - boxW/2, cy, flowColor);

  // Plant block (Smoker)
  drawBlock(xPlant, cy, boxW + 8, boxH + 4, 'SMOKER', '', '#666');
  // Draw a small flame icon inside
  ctx.fillStyle = out > 50 ? '#ff6b35' : out > 20 ? '#ff9f43' : '#666';
  ctx.font = '16px sans-serif';
  ctx.fillText('\uD83D\uDD25', xPlant, cy + 4);

  // Arrow: Plant -> Temp
  drawArrow(xPlant + boxW/2 + 4, cy, xTemp, cy, '#ff6b35');

  // Temperature output
  ctx.fillStyle = '#ff6b35';
  ctx.font = 'bold 14px SF Mono, Consolas, monospace';
  ctx.textAlign = 'center';
  var curTemp = pidHistory.length > 0 ? pidHistory[pidHistory.length - 1].temp : 0;
  ctx.fillText(curTemp ? curTemp.toFixed(0) + '\u00B0' : '--', xTemp, cy - 2);
  ctx.fillStyle = '#666';
  ctx.font = '9px -apple-system, sans-serif';
  ctx.fillText('TEMP', xTemp, cy + 12);

  // Feedback loop (bottom)
  var fbY = cy + 60;
  ctx.strokeStyle = '#555';
  ctx.lineWidth = 1;
  ctx.setLineDash([4, 3]);
  ctx.beginPath();
  ctx.moveTo(xTemp, cy + 8);
  ctx.lineTo(xTemp, fbY);
  ctx.lineTo(xSum, fbY);
  ctx.lineTo(xSum, cy + sumR);
  ctx.stroke();
  ctx.setLineDash([]);
  // Feedback label
  ctx.fillStyle = '#555';
  ctx.font = '9px -apple-system, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText('feedback', (xTemp + xSum) / 2, fbY - 4);
  // Negative sign at feedback entry
  ctx.fillStyle = '#e74c3c';
  ctx.font = 'bold 10px sans-serif';
  ctx.fillText('\u2013', xSum + 8, cy + sumR + 4);
}

// --- PID Terms Breakdown Bar ---
function drawPidTermsBar() {
  var canvas = document.getElementById('pid-terms-bar');
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

  var pid = pidLastData;
  var p = parseFloat(pid.p) || 0;
  var i = parseFloat(pid.i) || 0;
  var d = parseFloat(pid.d) || 0;
  var out = parseFloat(pid.output) || 0;

  var barH = 18;
  var barY = (H - barH) / 2;
  var barMargin = 8;
  var barW = W - barMargin * 2;

  // Background bar (0-100% scale)
  ctx.fillStyle = '#1a1a1a';
  ctx.beginPath();
  ctx.roundRect(barMargin, barY, barW, barH, 4);
  ctx.fill();

  // The PID output is the sum: output = (P + I + D + 0.5) * 100, clamped 15-100
  // Show each term as a proportion of the total
  // P, I, D can be negative. Map them relative to the output bar.
  // Simpler approach: show output as filled bar with colored segments
  var fillW = (out / 100) * barW;

  // Proportions of P, I, D relative to their absolute sum
  var absSum = Math.abs(p) + Math.abs(i) + Math.abs(d) + 0.001;
  var pFrac = Math.abs(p) / absSum;
  var iFrac = Math.abs(i) / absSum;
  var dFrac = 1 - pFrac - iFrac;

  var x = barMargin;
  var pW = fillW * pFrac;
  var iW = fillW * iFrac;
  var dW = fillW * dFrac;

  // Draw segments with rounded ends
  ctx.save();
  ctx.beginPath();
  ctx.roundRect(barMargin, barY, barW, barH, 4);
  ctx.clip();

  ctx.fillStyle = '#3498db';
  ctx.fillRect(x, barY, pW, barH);
  x += pW;
  ctx.fillStyle = '#2ecc71';
  ctx.fillRect(x, barY, iW, barH);
  x += iW;
  ctx.fillStyle = '#ff6b35';
  ctx.fillRect(x, barY, dW, barH);
  ctx.restore();

  // Center line at 50% (the PID centering offset)
  var cx = barMargin + barW * 0.5;
  ctx.strokeStyle = '#fff';
  ctx.globalAlpha = 0.3;
  ctx.lineWidth = 1;
  ctx.setLineDash([2, 2]);
  ctx.beginPath();
  ctx.moveTo(cx, barY - 2);
  ctx.lineTo(cx, barY + barH + 2);
  ctx.stroke();
  ctx.setLineDash([]);
  ctx.globalAlpha = 1;

  // Output percentage text
  ctx.fillStyle = '#fff';
  ctx.font = 'bold 10px SF Mono, Consolas, monospace';
  ctx.textAlign = 'right';
  ctx.fillText(out.toFixed(1) + '%', barMargin + fillW - 4, barY + barH/2 + 3.5);
}

// --- Auger Duty Cycle Gauge ---
function drawAugerGauge() {
  var canvas = document.getElementById('pid-auger-gauge');
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

  var pid = pidLastData;
  var out = parseFloat(pid.output) || 0;
  var cycleRemaining = parseInt(pid.cycleRemaining) || 0;
  var augerOn = pid.augerOn;

  var cx = W / 2;
  var cy = H / 2;
  var r = Math.min(W, H) / 2 - 12;

  // Background ring
  ctx.beginPath();
  ctx.arc(cx, cy, r, 0, Math.PI * 2);
  ctx.strokeStyle = '#222';
  ctx.lineWidth = 10;
  ctx.stroke();

  // Fill arc (duty cycle percentage)
  var fillAngle = (out / 100) * Math.PI * 2;
  var startAngle = -Math.PI / 2;
  ctx.beginPath();
  ctx.arc(cx, cy, r, startAngle, startAngle + fillAngle);
  ctx.strokeStyle = augerOn ? '#2ecc71' : '#ff6b35';
  ctx.lineWidth = 10;
  ctx.lineCap = 'round';
  ctx.stroke();

  // Cycle progress tick (shows where in the 20s cycle we are)
  var cycleTotal = 20000; // 20 seconds
  var cycleElapsed = cycleTotal - cycleRemaining;
  var tickAngle = startAngle + (cycleElapsed / cycleTotal) * Math.PI * 2;
  var tickR = r + 8;
  ctx.beginPath();
  ctx.arc(cx + tickR * Math.cos(tickAngle), cy + tickR * Math.sin(tickAngle), 3, 0, Math.PI * 2);
  ctx.fillStyle = '#fff';
  ctx.fill();

  // Center text
  ctx.fillStyle = augerOn ? '#2ecc71' : '#ff6b35';
  ctx.font = 'bold 22px SF Mono, Consolas, monospace';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.fillText(out.toFixed(0) + '%', cx, cy - 6);

  ctx.fillStyle = '#666';
  ctx.font = '10px -apple-system, sans-serif';
  ctx.fillText(augerOn ? 'AUGER ON' : 'AUGER OFF', cx, cy + 14);
  ctx.textBaseline = 'alphabetic';
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
