/* ESP32 Smoker Controller */
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
