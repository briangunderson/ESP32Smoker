#include "temperature_control.h"
#include "config.h"

TemperatureController::TemperatureController(MAX31865* tempSensor,
                                             RelayControl* relayControl)
    : _tempSensor(tempSensor), _relayControl(relayControl), _setpoint(225.0),
      _currentTemp(70.0), _state(STATE_IDLE), _previousState(STATE_IDLE),
      _stateStartTime(0), _lastUpdate(0), _consecutiveErrors(0),
      _debugMode(false), _tempOverrideEnabled(false), _tempOverrideValue(70.0) {}

void TemperatureController::begin() {
  _state = STATE_IDLE;
  _stateStartTime = millis();
  _lastUpdate = millis();
  _relayControl->allOff();

  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[TEMP] Temperature controller initialized");
  }
}

void TemperatureController::update() {
  unsigned long now = millis();
  if (now - _lastUpdate < TEMP_CONTROL_INTERVAL)
    return;

  _lastUpdate = now;

  // Skip automatic control if in debug mode
  if (_debugMode) {
    // Still read temperature for display
    readTemperature();
    return;
  }

  // Read current temperature
  if (!readTemperature()) {
    handleSensorError();
    return;
  }

  // Check for temperature faults
  if (_currentTemp <= TEMP_MIN_SAFE || _currentTemp >= TEMP_MAX_SAFE) {
    handleTemperatureError();
  }

  // Run state machine
  switch (_state) {
  case STATE_IDLE:
    handleIdleState();
    break;
  case STATE_STARTUP:
    handleStartupState();
    break;
  case STATE_RUNNING:
    handleRunningState();
    break;
  case STATE_COOLDOWN:
    handleCooldownState();
    break;
  case STATE_SHUTDOWN:
    handleShutdownState();
    break;
  case STATE_ERROR:
    handleErrorState();
    break;
  }
}

void TemperatureController::startSmoking(float targetTemp) {
  // Validate setpoint
  if (targetTemp < TEMP_MIN_SETPOINT || targetTemp > TEMP_MAX_SETPOINT) {
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[TEMP] Invalid setpoint: %.1f°F (valid range: %d-%d°F)\n",
                    targetTemp, TEMP_MIN_SETPOINT, TEMP_MAX_SETPOINT);
    }
    return;
  }

  _setpoint = targetTemp;
  _state = STATE_STARTUP;
  _stateStartTime = millis();
  _consecutiveErrors = 0;

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[TEMP] Starting up - target: %.1f°F\n", _setpoint);
  }
}

void TemperatureController::stop() {
  _state = STATE_COOLDOWN;
  _stateStartTime = millis();

  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[TEMP] Initiating cooldown");
  }
}

void TemperatureController::shutdown() {
  _state = STATE_SHUTDOWN;
  _stateStartTime = millis();
  _relayControl->allOff();

  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[TEMP] Shutdown commanded");
  }
}

void TemperatureController::setSetpoint(float targetTemp) {
  if (targetTemp < TEMP_MIN_SETPOINT || targetTemp > TEMP_MAX_SETPOINT) {
    return;
  }
  _setpoint = targetTemp;
}

float TemperatureController::getCurrentTemp(void) {
  return _currentTemp;
}

float TemperatureController::getSetpoint(void) {
  return _setpoint;
}

ControllerState TemperatureController::getState(void) {
  return _state;
}

const char* TemperatureController::getStateName(void) {
  switch (_state) {
  case STATE_IDLE:
    return "Idle";
  case STATE_STARTUP:
    return "Starting";
  case STATE_RUNNING:
    return "Running";
  case STATE_COOLDOWN:
    return "Cooling Down";
  case STATE_SHUTDOWN:
    return "Shutdown";
  case STATE_ERROR:
    return "Error";
  default:
    return "Unknown";
  }
}

TemperatureController::Status TemperatureController::getStatus(void) {
  return {_currentTemp, _setpoint, _state, 
          _relayControl->getAuger() == RELAY_ON,
          _relayControl->getFan() == RELAY_ON, 
          _relayControl->getIgniter() == RELAY_ON,
          getStateElapsedTime(), _consecutiveErrors};
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void TemperatureController::handleIdleState() {
  _relayControl->allOff();
  // Wait for startSmoking() command
}

void TemperatureController::handleStartupState() {
  unsigned long elapsed = getStateElapsedTime();

  if (elapsed < IGNITER_PREHEAT_TIME) {
    // Phase 1: Preheat igniter
    _relayControl->setIgniter(RELAY_ON);
    _relayControl->setFan(RELAY_OFF);
    _relayControl->setAuger(RELAY_OFF);
  } else if (elapsed < IGNITER_PREHEAT_TIME + FAN_STARTUP_DELAY) {
    // Phase 2: Start fan
    _relayControl->setIgniter(RELAY_ON);
    _relayControl->setFan(RELAY_ON);
    _relayControl->setAuger(RELAY_OFF);
  } else if (elapsed < STARTUP_TIMEOUT) {
    // Phase 3: Waiting for ignition
    _relayControl->setIgniter(RELAY_ON);
    _relayControl->setFan(RELAY_ON);
    _relayControl->setAuger(RELAY_ON);

    // Check if we've reached temperature
    if (_currentTemp >= (_setpoint - TEMP_HYSTERESIS_BAND / 2)) {
      _state = STATE_RUNNING;
      _stateStartTime = millis();

      if (ENABLE_SERIAL_DEBUG) {
        Serial.printf("[TEMP] Startup complete - reached %.1f°F\n",
                      _currentTemp);
      }
    }
  } else {
    // Startup timeout
    _state = STATE_ERROR;
    _relayControl->emergencyStop();

    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[TEMP] Startup timeout - entering error state");
    }
  }
}

void TemperatureController::handleRunningState() {
  _relayControl->setIgniter(RELAY_OFF); // Turn off igniter during normal operation
  manageFan();
  manageAuger();
  updateHysteresis();
}

void TemperatureController::handleCooldownState() {
  unsigned long elapsed = getStateElapsedTime();

  _relayControl->setIgniter(RELAY_OFF);
  _relayControl->setAuger(RELAY_OFF);

  // Keep fan running during cooldown for safety
  if (elapsed < SHUTDOWN_COOL_TIMEOUT && _currentTemp > TEMP_MIN_SAFE + 20) {
    _relayControl->setFan(RELAY_ON);
  } else {
    _state = STATE_SHUTDOWN;
    _stateStartTime = millis();
    _relayControl->allOff();

    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[TEMP] Cooldown complete");
    }
  }
}

void TemperatureController::handleShutdownState() {
  _relayControl->allOff();
  // Final state - wait for next startSmoking() call
  _state = STATE_IDLE;
}

void TemperatureController::handleErrorState() {
  _relayControl->emergencyStop();

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[TEMP] Error state - consecutive errors: %d\n",
                  _consecutiveErrors);
  }
}

void TemperatureController::updateHysteresis() {
  float lowerBound = _setpoint - (TEMP_HYSTERESIS_BAND / 2.0);
  float upperBound = _setpoint + (TEMP_HYSTERESIS_BAND / 2.0);

  if (_currentTemp < lowerBound) {
    // Need heat - enable auger if fan is on
    _relayControl->setSafeAuger(RELAY_ON);
  } else if (_currentTemp > upperBound) {
    // Too hot - disable auger
    _relayControl->setAuger(RELAY_OFF);
  }
  // else: stay in current state (hysteresis band)
}

void TemperatureController::manageFan() {
  // Fan stays on during RUNNING state for oxygen supply
  _relayControl->setFan(RELAY_ON);
}

void TemperatureController::manageAuger() {
  // Controlled by updateHysteresis()
}

void TemperatureController::manageIgniter() {
  // Igniter is OFF during RUNNING state (already handled in handleRunningState)
}

bool TemperatureController::readTemperature() {
  // Check if temperature override is enabled (debug mode)
  if (_tempOverrideEnabled) {
    _currentTemp = _tempOverrideValue;
    _consecutiveErrors = 0;
    return true;
  }

  float tempC = _tempSensor->readTemperatureC();

  if (tempC < -100) { // Error value from sensor
    return false;
  }

  // Average multiple samples for stability
  static float tempBuffer[TEMP_SAMPLE_COUNT] = {0};
  static uint8_t bufferIndex = 0;
  static bool bufferFull = false;

  tempBuffer[bufferIndex] = tempC;
  bufferIndex = (bufferIndex + 1) % TEMP_SAMPLE_COUNT;

  if (bufferIndex == 0) {
    bufferFull = true;
  }

  if (bufferFull) {
    float sum = 0;
    for (uint8_t i = 0; i < TEMP_SAMPLE_COUNT; i++) {
      sum += tempBuffer[i];
    }
    _currentTemp = (sum / TEMP_SAMPLE_COUNT * 9.0 / 5.0) + 32.0;
  } else {
    _currentTemp = tempC * 9.0 / 5.0 + 32.0;
  }

  _consecutiveErrors = 0;
  return true;
}

void TemperatureController::handleSensorError() {
  _consecutiveErrors++;

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[TEMP] Sensor error #%d\n", _consecutiveErrors);
  }

  if (_consecutiveErrors >= SENSOR_ERROR_THRESHOLD) {
    _state = STATE_ERROR;
    _relayControl->emergencyStop();

    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[TEMP] Sensor error threshold exceeded - emergency stop");
    }
  }
}

void TemperatureController::handleTemperatureError() {
  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[TEMP] Temperature out of safe range: %.1f°F\n",
                  _currentTemp);
  }

  _state = STATE_ERROR;
  _relayControl->emergencyStop();
}

unsigned long TemperatureController::getStateElapsedTime() {
  return millis() - _stateStartTime;
}

// Debug/Testing Methods
void TemperatureController::setDebugMode(bool enabled) {
  _debugMode = enabled;

  if (enabled) {
    // When entering debug mode, turn off all relays
    _relayControl->allOff();
    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[TEMP] Debug mode ENABLED - manual control active");
    }
  } else {
    // When exiting debug mode, return to idle state
    _state = STATE_IDLE;
    _relayControl->allOff();
    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[TEMP] Debug mode DISABLED - automatic control active");
    }
  }
}

bool TemperatureController::isDebugMode(void) {
  return _debugMode;
}

void TemperatureController::setManualRelay(const char* relay, bool state) {
  if (!_debugMode) {
    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[TEMP] Manual relay control requires debug mode");
    }
    return;
  }

  if (strcmp(relay, "auger") == 0) {
    _relayControl->setAuger(state ? RELAY_ON : RELAY_OFF);
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[TEMP] Manual: Auger %s\n", state ? "ON" : "OFF");
    }
  } else if (strcmp(relay, "fan") == 0) {
    _relayControl->setFan(state ? RELAY_ON : RELAY_OFF);
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[TEMP] Manual: Fan %s\n", state ? "ON" : "OFF");
    }
  } else if (strcmp(relay, "igniter") == 0) {
    _relayControl->setIgniter(state ? RELAY_ON : RELAY_OFF);
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[TEMP] Manual: Igniter %s\n", state ? "ON" : "OFF");
    }
  }
}

void TemperatureController::setTempOverride(float temp) {
  _tempOverrideEnabled = true;
  _tempOverrideValue = temp;
  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[TEMP] Temperature override set to %.1f°F\n", temp);
  }
}

void TemperatureController::clearTempOverride(void) {
  _tempOverrideEnabled = false;
  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[TEMP] Temperature override cleared");
  }
}
