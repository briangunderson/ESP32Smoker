/* ========================================
   ESP32 Smoker Controller - JavaScript
   ======================================== */

// Configuration
const API_BASE = '/api';
const UPDATE_INTERVAL = 2000; // ms
let statusUpdateInterval = null;

// State tracking
let controllerState = {
  temp: 0,
  setpoint: 225,
  state: 'Idle',
  auger: false,
  fan: false,
  igniter: false,
  runtime: 0,
  errors: 0,
  isRunning: false
};

// ============================================================================
// INITIALIZATION
// ============================================================================

document.addEventListener('DOMContentLoaded', function() {
  console.log('Initializing ESP32 Smoker Controller UI...');
  
  // Start status updates
  updateStatus();
  statusUpdateInterval = setInterval(updateStatus, UPDATE_INTERVAL);
  
  // Update uptime timer
  setInterval(updateUptime, 1000);
});

// ============================================================================
// API CALLS
// ============================================================================

async function apiCall(endpoint, method = 'GET', data = null) {
  try {
    const options = {
      method: method,
      headers: {
        'Content-Type': 'application/json'
      }
    };

    if (data) {
      options.body = JSON.stringify(data);
    }

    const response = await fetch(`${API_BASE}${endpoint}`, options);
    
    if (!response.ok) {
      console.error(`API Error: ${response.status} ${response.statusText}`);
      return null;
    }

    return await response.json();
  } catch (error) {
    console.error(`API Call Error: ${error}`);
    return null;
  }
}

async function updateStatus() {
  const status = await apiCall('/status', 'GET');
  
  if (status) {
    updateUI(status);
  } else {
    console.warn('Failed to fetch status');
  }
}

async function startSmoking() {
  const temp = parseFloat(document.getElementById('setpoint-input').value);
  const result = await apiCall('/start', 'POST', { temp: temp });

  if (result) {
    controllerState.isRunning = true;
    updateButtonStates();
    console.log(`Starting smoker at ${temp}°F`);
  }
}

async function stopSmoking() {
  const result = await apiCall('/stop', 'POST');
  
  if (result) {
    controllerState.isRunning = false;
    updateButtonStates();
    console.log('Stopping smoker');
  }
}

async function shutdown() {
  if (confirm('Are you sure you want to shutdown the smoker?')) {
    const result = await apiCall('/shutdown', 'POST');
    
    if (result) {
      console.log('Shutdown command sent');
    }
  }
}

function incrementSetpoint() {
  const input = document.getElementById('setpoint-input');
  const current = parseFloat(input.value);
  const newValue = Math.min(350, current + 5);
  input.value = newValue;
}

function decrementSetpoint() {
  const input = document.getElementById('setpoint-input');
  const current = parseFloat(input.value);
  const newValue = Math.max(150, current - 5);
  input.value = newValue;
}

async function applySetpoint() {
  const setpoint = parseFloat(document.getElementById('setpoint-input').value);

  if (setpoint < 150 || setpoint > 350) {
    alert('Temperature must be between 150°F and 350°F');
    return;
  }

  const result = await apiCall('/setpoint', 'POST', { temp: setpoint });

  if (result) {
    controllerState.setpoint = setpoint;
    console.log(`Setpoint updated to ${setpoint}°F`);
  } else {
    alert('Failed to update setpoint');
  }
}

// ============================================================================
// UI UPDATES
// ============================================================================

function updateUI(status) {
  // Update temperature displays
  document.getElementById('current-temp').textContent = status.temp.toFixed(1);
  document.getElementById('setpoint-temp').textContent = status.setpoint.toFixed(1);
  
  // Update state
  document.getElementById('state-display').textContent = status.state;
  controllerState.state = status.state;
  
  // Update relay status
  updateRelayStatus('relay-auger', status.auger);
  updateRelayStatus('relay-fan', status.fan);
  updateRelayStatus('relay-igniter', status.igniter);
  
  // Update error count
  document.getElementById('error-count').textContent = status.errors;
  
  // Update button states
  updateButtonStates();
  
  // Update setpoint input
  document.getElementById('setpoint-input').value = status.setpoint;
}

function updateRelayStatus(elementId, isOn) {
  const element = document.getElementById(elementId);
  if (isOn) {
    element.textContent = 'ON';
    element.classList.remove('off');
    element.classList.add('on');
  } else {
    element.textContent = 'OFF';
    element.classList.remove('on');
    element.classList.add('off');
  }
}

function updateButtonStates() {
  const isRunning = controllerState.state !== 'Idle' && 
                    controllerState.state !== 'Shutdown';
  
  document.getElementById('btn-start').disabled = isRunning;
  document.getElementById('btn-stop').disabled = !isRunning;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

let uptimeSeconds = 0;

function updateUptime() {
  uptimeSeconds++;
  const hours = Math.floor(uptimeSeconds / 3600);
  const minutes = Math.floor((uptimeSeconds % 3600) / 60);
  document.getElementById('uptime').textContent = `${hours}h ${minutes}m`;
}

// WiFi/MQTT Status indicators (would be real in production)
function updateNetworkStatus() {
  // Check WiFi
  const wifiStatus = document.getElementById('wifi-status');
  const mqttStatus = document.getElementById('mqtt-status');
  const sensorStatus = document.getElementById('sensor-status');
  
  // These would be updated from API responses
  // For now, assume connected
  wifiStatus.classList.add('connected');
  mqttStatus.classList.add('connected');
  sensorStatus.classList.add('connected');
}

// Initialize network status
updateNetworkStatus();

// ============================================================================
// DEBUG/TESTING FUNCTIONS
// ============================================================================

let debugModeActive = false;

function toggleDebugPanel() {
  const panel = document.getElementById('debug-panel');
  const toggle = document.getElementById('debug-toggle');

  if (panel.style.display === 'none') {
    panel.style.display = 'block';
    toggle.textContent = '▲';
  } else {
    panel.style.display = 'none';
    toggle.textContent = '▼';
  }
}

async function toggleDebugMode() {
  debugModeActive = !debugModeActive;

  const formData = new FormData();
  formData.append('enabled', debugModeActive.toString());

  try {
    const response = await fetch(`${API_BASE}/debug/mode`, {
      method: 'POST',
      body: formData
    });

    if (response.ok) {
      const btn = document.getElementById('btn-debug-toggle');
      const controls = document.getElementById('debug-controls');

      if (debugModeActive) {
        btn.textContent = 'Disable Debug Mode';
        btn.classList.remove('btn-secondary');
        btn.classList.add('btn-danger');
        controls.style.display = 'block';
        console.log('Debug mode ENABLED');
      } else {
        btn.textContent = 'Enable Debug Mode';
        btn.classList.remove('btn-danger');
        btn.classList.add('btn-secondary');
        controls.style.display = 'none';
        console.log('Debug mode DISABLED');
      }
    }
  } catch (error) {
    console.error('Failed to toggle debug mode:', error);
  }
}

async function setRelay(relay, state) {
  const formData = new FormData();
  formData.append('relay', relay);
  formData.append('state', state.toString());

  try {
    const response = await fetch(`${API_BASE}/debug/relay`, {
      method: 'POST',
      body: formData
    });

    if (response.ok) {
      console.log(`Manual relay control: ${relay} = ${state ? 'ON' : 'OFF'}`);
    }
  } catch (error) {
    console.error('Failed to control relay:', error);
  }
}

async function setTempOverride() {
  const temp = document.getElementById('temp-override-input').value;

  const formData = new FormData();
  formData.append('temp', temp);

  try {
    const response = await fetch(`${API_BASE}/debug/temp`, {
      method: 'POST',
      body: formData
    });

    if (response.ok) {
      console.log(`Temperature override set to ${temp}°F`);
    }
  } catch (error) {
    console.error('Failed to set temperature override:', error);
  }
}

async function clearTempOverride() {
  try {
    const response = await fetch(`${API_BASE}/debug/temp`, {
      method: 'DELETE'
    });

    if (response.ok) {
      console.log('Temperature override cleared');
    }
  } catch (error) {
    console.error('Failed to clear temperature override:', error);
  }
}

console.log('ESP32 Smoker Controller UI Ready');
