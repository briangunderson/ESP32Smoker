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
  const temp = parseFloat(document.getElementById('setpoint-slider').value);
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

async function updateSetpoint(value) {
  const setpoint = parseFloat(value);
  document.getElementById('slider-value').textContent = `${setpoint}°F`;
  
  const result = await apiCall('/setpoint', 'POST', { temp: setpoint });
  
  if (result) {
    controllerState.setpoint = setpoint;
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
  
  // Update slider position
  document.getElementById('setpoint-slider').value = status.setpoint;
  document.getElementById('slider-value').textContent = `${status.setpoint}°F`;
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

console.log('ESP32 Smoker Controller UI Ready');
