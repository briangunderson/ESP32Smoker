/* ESP32 Smoker Controller */
const API = '/api';
const POLL_MS = 2000;
let apiOk = false;
let debugActive = false;

// --- Init ---
document.addEventListener('DOMContentLoaded', () => {
  updateStatus();
  setInterval(updateStatus, POLL_MS);
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
    const m = Math.floor(s.runtime / 60);
    const sec = s.runtime % 60;
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
  const v = Math.max(150, Math.min(350, parseInt(el.value) + delta));
  el.value = v;
}

async function applySetpoint() {
  const v = parseInt(document.getElementById('setpoint-input').value);
  if (v < 150 || v > 350) { toast('150-350\u00B0F range', 'err'); return; }
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
