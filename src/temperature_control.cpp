#include "temperature_control.h"
#include "config.h"
#include "logger.h"

TemperatureController::TemperatureController(MAX31865* tempSensor,
                                             RelayControl* relayControl)
    : _tempSensor(tempSensor), _relayControl(relayControl), _setpoint(225.0),
      _currentTemp(70.0), _state(STATE_IDLE), _previousState(STATE_IDLE),
      _stateStartTime(0), _lastUpdate(0), _consecutiveErrors(0),
      _debugMode(false), _tempOverrideEnabled(false), _tempOverrideValue(70.0),
      _pidOutput(0.0), _integral(0.0), _previousError(0.0), _previousTemp(70.0),
      _lastP(0.0), _lastI(0.0), _lastD(0.0),
      _lastPidUpdate(0), _augerCycleStart(0), _augerCycleState(false),
      _lastIntegralSave(0),
      _historyHead(0), _historyCount(0), _lastHistorySample(0),
      _eventHead(0), _eventCount(0),
      _reigniteAttempts(0), _reignitePhase(0), _reignitePhaseStart(0),
      _pidMaxedSince(0),
      _lidOpen(false), _lidOpenTime(0), _lidStableTime(0) {

  // Calculate PID gains from Proportional Band parameters (PiSmoker method)
  _Kp = -1.0 / PID_PROPORTIONAL_BAND;              // = -0.0167
  _Ki = _Kp / PID_INTEGRAL_TIME;                   // = -0.0000926
  _Kd = _Kp * PID_DERIVATIVE_TIME;                 // = -0.75
}

void TemperatureController::begin() {
  _state = STATE_IDLE;
  _stateStartTime = millis();
  _lastUpdate = millis();
  _relayControl->allOff();

  // Open NVS namespace for persistent PID storage
  if (ENABLE_PID_PERSISTENCE) {
    _prefs.begin("smoker", false);  // false = read/write mode
    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[TEMP] NVS persistence enabled");
    }
  }

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
  // High temp: check in all active states (runaway fire is always dangerous)
  // Low temp: only check during RUNNING/COOLDOWN (smoker is cold during STARTUP/REIGNITE)
  if (_state != STATE_IDLE && _state != STATE_ERROR) {
    if (_currentTemp >= TEMP_MAX_SAFE) {
      handleTemperatureError();
    }
    if ((_state == STATE_RUNNING || _state == STATE_COOLDOWN) && _currentTemp <= TEMP_MIN_SAFE) {
      handleTemperatureError();
    }
  }

  // Lid-open detection (only during RUNNING state)
  if (_state == STATE_RUNNING && ENABLE_LID_DETECTION) {
    detectLidOpen();
  } else {
    _lidOpen = false;
  }

  // Detect and log state transitions
  if (_state != _previousState) {
    // Save integral when leaving RUNNING or REIGNITE state (any exit path)
    if (_previousState == STATE_RUNNING || _previousState == STATE_REIGNITE) {
      saveIntegralToNVS();
    }

    // Record state change event for history graph
    recordHistoryEvent(_state);

    DUAL_LOGF(LOG_INFO, "\n[STATE] Transition: %s -> %s (Temp: %.1f°F)\n\n",
              stateToString(_previousState),
              stateToString(_state),
              _currentTemp);
    _previousState = _state;
  }

  // Record history sample at configured interval
  recordHistorySample();

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
  case STATE_REIGNITE:
    handleReigniteState();
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

  // Reset PID state
  _integral = 0.0;
  _previousError = 0.0;
  _pidOutput = 0.0;
  _lastPidUpdate = millis();
  _augerCycleStart = millis();
  _augerCycleState = false;

  // Reset reignite counter for new cook session
  _reigniteAttempts = 0;
  _pidMaxedSince = 0;
  _lidOpen = false;

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
  case STATE_REIGNITE:
    return "Reignite";
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

TemperatureController::PIDStatus TemperatureController::getPIDStatus(void) {
  unsigned long cyclePos = (millis() - _augerCycleStart) % AUGER_CYCLE_TIME;
  unsigned long onTime = (unsigned long)(AUGER_CYCLE_TIME * _pidOutput);
  uint32_t remaining = (_state == STATE_RUNNING)
    ? (uint32_t)((AUGER_CYCLE_TIME - cyclePos) / 1000)
    : 0;

  return {
    _lastP,                    // proportionalTerm
    _lastI,                    // integralTerm
    _lastD,                    // derivativeTerm
    _pidOutput,                // output (0.0 to 1.0)
    _currentTemp - _setpoint,  // error
    _setpoint,
    _currentTemp,
    _Kp,
    _Ki,
    _Kd,
    remaining,                 // cycleTimeRemaining (seconds)
    _augerCycleState           // augerCycleState
  };
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

  // Turn off igniter once temperature exceeds 100°F (safety and efficiency)
  bool igniterNeeded = (_currentTemp < IGNITER_CUTOFF_TEMP);

  if (elapsed < IGNITER_PREHEAT_TIME) {
    // Phase 1: Preheat igniter
    _relayControl->setIgniter(igniterNeeded ? RELAY_ON : RELAY_OFF);
    _relayControl->setFan(RELAY_OFF);
    _relayControl->setAuger(RELAY_OFF);
  } else if (elapsed < IGNITER_PREHEAT_TIME + FAN_STARTUP_DELAY) {
    // Phase 2: Start fan
    _relayControl->setIgniter(igniterNeeded ? RELAY_ON : RELAY_OFF);
    _relayControl->setFan(RELAY_ON);
    _relayControl->setAuger(RELAY_OFF);
  } else if (elapsed < STARTUP_TIMEOUT) {
    // Phase 3: Waiting for ignition
    _relayControl->setIgniter(igniterNeeded ? RELAY_ON : RELAY_OFF);
    _relayControl->setFan(RELAY_ON);
    _relayControl->setAuger(RELAY_ON);

    // Check if we've reached startup threshold (absolute temperature, not relative)
    // PiSmoker transitions to Hold mode at 115°F regardless of setpoint
    if (_currentTemp >= STARTUP_TEMP_THRESHOLD) {
      _state = STATE_RUNNING;
      _stateStartTime = millis();

      // Restore saved integral term from NVS (if valid)
      loadIntegralFromNVS();
      _lastIntegralSave = millis();

      if (ENABLE_SERIAL_DEBUG) {
        Serial.printf("[TEMP] Startup complete - reached %.1f°F (threshold: %d°F)\n",
                      _currentTemp, STARTUP_TEMP_THRESHOLD);
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
  updatePID();

  // Reignite detection: fire may be dead if temp is low and PID is maxed out
  if (ENABLE_REIGNITE && !_lidOpen) {
    unsigned long now = millis();

    // Track how long PID has been at max output
    if (_pidOutput >= PID_OUTPUT_MAX - 0.01) {
      if (_pidMaxedSince == 0) _pidMaxedSince = now;
    } else {
      _pidMaxedSince = 0;  // Reset if PID drops below max
    }

    // Trigger reignite if temp is low AND PID has been maxed for trigger time
    if (_currentTemp < REIGNITE_TEMP_THRESHOLD &&
        _pidMaxedSince > 0 &&
        (now - _pidMaxedSince) >= REIGNITE_TRIGGER_TIME) {

      if (_reigniteAttempts < REIGNITE_MAX_ATTEMPTS) {
        DUAL_LOGF(LOG_WARNING,
          "[REIGNITE] Fire may be out! Temp=%.1f°F < %.0f°F, PID maxed for %lus. Attempt %d/%d\n",
          _currentTemp, REIGNITE_TEMP_THRESHOLD,
          (now - _pidMaxedSince) / 1000,
          _reigniteAttempts + 1, REIGNITE_MAX_ATTEMPTS);

        _state = STATE_REIGNITE;
        _stateStartTime = now;
        _reignitePhase = 0;
        _reignitePhaseStart = now;
        _pidMaxedSince = 0;
      } else {
        DUAL_LOGF(LOG_CRIT,
          "[REIGNITE] Max attempts (%d) exhausted. Entering ERROR state.\n",
          REIGNITE_MAX_ATTEMPTS);
        _state = STATE_ERROR;
        _relayControl->emergencyStop();
      }
    }
  }
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

  // Log error state details periodically
  static unsigned long lastErrorLog = 0;
  if (millis() - lastErrorLog > 5000) {
    lastErrorLog = millis();

    DUAL_LOGF(LOG_ERR, "========================================\n");
    DUAL_LOGF(LOG_ERR, "[TEMP] ERROR STATE DIAGNOSTICS\n");
    DUAL_LOGF(LOG_ERR, "========================================\n");
    DUAL_LOGF(LOG_ERR, "  Consecutive sensor errors: %d\n", _consecutiveErrors);
    DUAL_LOGF(LOG_ERR, "  Current temperature: %.1f°F\n", _currentTemp);
    DUAL_LOGF(LOG_ERR, "  Setpoint: %.1f°F\n", _setpoint);
    DUAL_LOGF(LOG_ERR, "  Time in error state: %lu ms\n", getStateElapsedTime());

    // Try to read sensor and get fault details
    DUAL_LOGF(LOG_ERR, "\n  Attempting sensor read for diagnostics...\n");
    uint8_t fault = _tempSensor->getFaultStatus();
    if (fault != 0) {
      _tempSensor->printFaultStatus(fault);
    } else {
      DUAL_LOGF(LOG_ERR, "  [MAX31865] No faults reported by sensor\n");
    }

    DUAL_LOGF(LOG_ERR, "\n  To recover from error state:\n");
    DUAL_LOGF(LOG_ERR, "  1. Fix sensor wiring/connections\n");
    DUAL_LOGF(LOG_ERR, "  2. Use web interface to restart\n");
    DUAL_LOGF(LOG_ERR, "  3. Or power cycle the ESP32\n");
    DUAL_LOGF(LOG_ERR, "========================================\n");
  }
}

void TemperatureController::updatePID() {
  unsigned long now = millis();

  // Calculate time delta in seconds
  float dt = (now - _lastPidUpdate) / 1000.0;
  if (dt < 0.001) return; // Avoid division by zero

  _lastPidUpdate = now;

  // Calculate error (NOTE: reversed from standard PID - currentTemp - setpoint)
  float error = _currentTemp - _setpoint;

  // Proportional term with 0.5 centering (Proportional Band method)
  // At setpoint: P = 0.5 (50% output)
  // Below setpoint: P > 0.5 (increase auger)
  // Above setpoint: P < 0.5 (decrease auger)
  float P = _Kp * error + 0.5;

  // Integral term (with anti-windup limiting to 50% of output range)
  // Freeze integral accumulation during lid-open to prevent overshoot
  if (!_lidOpen) {
    _integral += error * dt;
  }

  // Limit integral contribution to 50% of output range
  float integralContribution = 0.5;
  float integralMax = abs(integralContribution / _Ki);
  float integralMin = -integralMax;

  if (_integral > integralMax) _integral = integralMax;
  if (_integral < integralMin) _integral = integralMin;

  float I = _Ki * _integral;

  // Derivative term on measurement (not error) to prevent derivative kick
  // When setpoint changes, derivative won't spike
  float derivative = (_currentTemp - _previousTemp) / dt;
  float D = _Kd * derivative;

  // Calculate PID output (0.0 to 1.0 range)
  _pidOutput = P + I + D;

  // Clamp output to valid range (0.0 - 1.0)
  if (_pidOutput > PID_OUTPUT_MAX) _pidOutput = PID_OUTPUT_MAX;
  if (_pidOutput < PID_OUTPUT_MIN) _pidOutput = PID_OUTPUT_MIN;

  // Store PID terms for status reporting (getPIDStatus / MQTT)
  _lastP = P;
  _lastI = I;
  _lastD = D;

  // Store for next iteration
  _previousError = error;
  _previousTemp = _currentTemp;

  // Apply PID output to auger control
  applyPIDOutput();

  if (ENABLE_SERIAL_DEBUG) {
    static unsigned long lastDebug = 0;
    if (now - lastDebug > 5000) {
      lastDebug = now;
      Serial.printf("[PID] Temp:%.1f Set:%.1f Err:%.1f P:%.3f I:%.3f D:%.3f Out:%.2f\n",
                    _currentTemp, _setpoint, error, P, I, D, _pidOutput);
    }
  }

  // Periodic save of integral to NVS (handles unexpected power loss)
  if (ENABLE_PID_PERSISTENCE && (now - _lastIntegralSave >= PID_SAVE_INTERVAL)) {
    saveIntegralToNVS();
  }
}

void TemperatureController::applyPIDOutput() {
  // Time-proportioning control: auger cycles on/off within AUGER_CYCLE_TIME window
  // based on PID output (0.0 to 1.0)

  unsigned long now = millis();
  unsigned long cyclePosition = (now - _augerCycleStart) % AUGER_CYCLE_TIME;

  // Calculate on-time from PID output (already enforces minimum via PID_OUTPUT_MIN)
  unsigned long onTime = (unsigned long)(AUGER_CYCLE_TIME * _pidOutput);

  // Reset cycle if it wrapped
  if (cyclePosition < 100 && (now - _augerCycleStart) > AUGER_CYCLE_TIME) {
    _augerCycleStart = now;
  }

  // Determine auger state based on position in cycle
  bool shouldBeOn = (cyclePosition < onTime);

  // Only change state if needed (reduces relay wear)
  if (shouldBeOn != _augerCycleState) {
    _augerCycleState = shouldBeOn;
    if (shouldBeOn) {
      _relayControl->setSafeAuger(RELAY_ON);
    } else {
      _relayControl->setAuger(RELAY_OFF);
    }
  }
}

void TemperatureController::manageFan() {
  // Fan stays on during RUNNING state for oxygen supply
  _relayControl->setFan(RELAY_ON);
}

void TemperatureController::manageAuger() {
  // Controlled by PID via applyPIDOutput()
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

  if (tempC < -100 || tempC > 400) { // Error value or physically impossible reading
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

  DUAL_LOGF(LOG_WARNING, "----------------------------------------\n");
  DUAL_LOGF(LOG_WARNING, "[TEMP] SENSOR READ FAILURE #%d of %d\n",
            _consecutiveErrors, SENSOR_ERROR_THRESHOLD);

  // Get detailed fault information from sensor
  uint8_t fault = _tempSensor->getFaultStatus();
  if (fault != 0) {
    _tempSensor->printFaultStatus(fault);
  } else {
    DUAL_LOGF(LOG_WARNING, "[TEMP] Sensor returned error value but no faults reported\n");
    DUAL_LOGF(LOG_WARNING, "  Possible causes:\n");
    DUAL_LOGF(LOG_WARNING, "  - SPI communication problem\n");
    DUAL_LOGF(LOG_WARNING, "  - Sensor not properly initialized\n");
    DUAL_LOGF(LOG_WARNING, "  - Power supply issue\n");
  }
  DUAL_LOGF(LOG_WARNING, "----------------------------------------\n");

  if (_consecutiveErrors >= SENSOR_ERROR_THRESHOLD) {
    _state = STATE_ERROR;
    _relayControl->emergencyStop();

    DUAL_LOGF(LOG_CRIT, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    DUAL_LOGF(LOG_CRIT, "! ENTERING ERROR STATE - EMERGENCY STOP !\n");
    DUAL_LOGF(LOG_CRIT, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    DUAL_LOGF(LOG_CRIT, "! Reason: %d consecutive sensor failures !\n",
              _consecutiveErrors);
    DUAL_LOGF(LOG_CRIT, "! All relays have been turned OFF       !\n");
    DUAL_LOGF(LOG_CRIT, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  }
}

void TemperatureController::handleTemperatureError() {
  DUAL_LOGF(LOG_CRIT, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  DUAL_LOGF(LOG_CRIT, "! TEMPERATURE OUT OF SAFE RANGE        !\n");
  DUAL_LOGF(LOG_CRIT, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  DUAL_LOGF(LOG_CRIT, "! Current: %.1f°F\n", _currentTemp);
  DUAL_LOGF(LOG_CRIT, "! Safe range: %d°F to %d°F\n", TEMP_MIN_SAFE, TEMP_MAX_SAFE);

  if (_currentTemp < TEMP_MIN_SAFE) {
    DUAL_LOGF(LOG_CRIT, "! PROBLEM: Temperature too LOW         !\n");
    DUAL_LOGF(LOG_CRIT, "! Possible causes:                     !\n");
    DUAL_LOGF(LOG_CRIT, "!   - Sensor disconnected/failed       !\n");
    DUAL_LOGF(LOG_CRIT, "!   - Sensor reading incorrect value   !\n");
  } else {
    DUAL_LOGF(LOG_CRIT, "! PROBLEM: Temperature too HIGH        !\n");
    DUAL_LOGF(LOG_CRIT, "! Possible causes:                     !\n");
    DUAL_LOGF(LOG_CRIT, "!   - Runaway fire condition           !\n");
    DUAL_LOGF(LOG_CRIT, "!   - Control system malfunction       !\n");
    DUAL_LOGF(LOG_CRIT, "!   - Sensor reading incorrect value   !\n");
  }
  DUAL_LOGF(LOG_CRIT, "! ENTERING ERROR STATE                 !\n");
  DUAL_LOGF(LOG_CRIT, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

  _state = STATE_ERROR;
  _relayControl->emergencyStop();
}

unsigned long TemperatureController::getStateElapsedTime() {
  return millis() - _stateStartTime;
}

const char* TemperatureController::stateToString(ControllerState state) {
  switch (state) {
  case STATE_IDLE:
    return "IDLE";
  case STATE_STARTUP:
    return "STARTUP";
  case STATE_RUNNING:
    return "RUNNING";
  case STATE_COOLDOWN:
    return "COOLDOWN";
  case STATE_SHUTDOWN:
    return "SHUTDOWN";
  case STATE_ERROR:
    return "ERROR";
  case STATE_REIGNITE:
    return "REIGNITE";
  default:
    return "UNKNOWN";
  }
}

// ============================================================================
// REIGNITE LOGIC
// ============================================================================

void TemperatureController::handleReigniteState() {
  unsigned long now = millis();
  unsigned long phaseElapsed = now - _reignitePhaseStart;

  switch (_reignitePhase) {
    case 0: // Fan Clear — blow out ash
      _relayControl->setFan(RELAY_ON);
      _relayControl->setAuger(RELAY_OFF);
      _relayControl->setIgniter(RELAY_OFF);
      if (phaseElapsed >= REIGNITE_FAN_CLEAR_TIME) {
        _reignitePhase = 1;
        _reignitePhaseStart = now;
        DUAL_LOGF(LOG_INFO, "[REIGNITE] Phase 1: Igniter preheat\n");
      }
      break;

    case 1: // Igniter Preheat
      _relayControl->setFan(RELAY_ON);
      _relayControl->setAuger(RELAY_OFF);
      _relayControl->setIgniter(RELAY_ON);
      if (phaseElapsed >= REIGNITE_PREHEAT_TIME) {
        _reignitePhase = 2;
        _reignitePhaseStart = now;
        DUAL_LOGF(LOG_INFO, "[REIGNITE] Phase 2: Feeding pellets\n");
      }
      break;

    case 2: // Feeding — auger + igniter + fan
      _relayControl->setFan(RELAY_ON);
      _relayControl->setSafeAuger(RELAY_ON);
      _relayControl->setIgniter(RELAY_ON);
      if (phaseElapsed >= REIGNITE_FEED_TIME) {
        _reignitePhase = 3;
        _reignitePhaseStart = now;
        _relayControl->setIgniter(RELAY_OFF);
        DUAL_LOGF(LOG_INFO, "[REIGNITE] Phase 3: Recovery (waiting for temp rise)\n");
      }
      break;

    case 3: // Recovery — fan + 50% auger, wait for temp to rise
      _relayControl->setFan(RELAY_ON);
      _relayControl->setIgniter(RELAY_OFF);
      // 50% duty cycle: auger on for half the cycle
      {
        unsigned long cyclePos = phaseElapsed % AUGER_CYCLE_TIME;
        bool augerOn = (cyclePos < AUGER_CYCLE_TIME / 2);
        if (augerOn) {
          _relayControl->setSafeAuger(RELAY_ON);
        } else {
          _relayControl->setAuger(RELAY_OFF);
        }
      }

      // Success: temp rose above threshold
      if (_currentTemp >= REIGNITE_TEMP_THRESHOLD) {
        _reigniteAttempts++;
        DUAL_LOGF(LOG_INFO,
          "[REIGNITE] Success! Temp=%.1f°F. Returning to RUNNING. (Attempt %d)\n",
          _currentTemp, _reigniteAttempts);
        _state = STATE_RUNNING;
        _stateStartTime = millis();
        _pidMaxedSince = 0;
        // Reset PID to avoid integral windup from reignite period
        _integral = 0.0;
        _previousError = 0.0;
        _lastPidUpdate = millis();
        _augerCycleStart = millis();
        return;
      }

      // Failure: recovery time exceeded
      if (phaseElapsed >= REIGNITE_RECOVERY_TIME) {
        _reigniteAttempts++;
        if (_reigniteAttempts >= REIGNITE_MAX_ATTEMPTS) {
          DUAL_LOGF(LOG_CRIT,
            "[REIGNITE] Recovery failed after %d attempts. Entering ERROR state.\n",
            _reigniteAttempts);
          _state = STATE_ERROR;
          _relayControl->emergencyStop();
        } else {
          DUAL_LOGF(LOG_WARNING,
            "[REIGNITE] Recovery failed (attempt %d/%d). Retrying...\n",
            _reigniteAttempts, REIGNITE_MAX_ATTEMPTS);
          _reignitePhase = 0;
          _reignitePhaseStart = millis();
        }
      }
      break;
  }
}

// ============================================================================
// LID-OPEN DETECTION
// ============================================================================

void TemperatureController::detectLidOpen() {
  unsigned long now = millis();

  // Calculate temperature rate of change (°F/s)
  // Use _previousTemp which is updated each PID cycle
  float dt = TEMP_CONTROL_INTERVAL / 1000.0;  // seconds between updates
  if (dt < 0.001) return;
  float dTdt = (_currentTemp - _previousTemp) / dt;

  if (!_lidOpen) {
    // Detect lid opening: rapid temperature drop
    if (dTdt < LID_OPEN_DERIVATIVE_THRESHOLD) {
      _lidOpen = true;
      _lidOpenTime = now;
      _lidStableTime = 0;
      DUAL_LOGF(LOG_INFO,
        "[LID] Lid opened detected! dT/dt=%.2f°F/s (threshold=%.1f)\n",
        dTdt, LID_OPEN_DERIVATIVE_THRESHOLD);
    }
  } else {
    // Detect lid closing: temperature stabilizing
    if (dTdt > -0.5) {
      // Temperature is stabilizing
      if (_lidStableTime == 0) {
        _lidStableTime = now;
      } else if ((now - _lidStableTime) >= (uint32_t)LID_CLOSE_RECOVERY_TIME) {
        // Stable for long enough — declare lid closed
        uint32_t openDuration = (now - _lidOpenTime) / 1000;
        DUAL_LOGF(LOG_INFO,
          "[LID] Lid closed. Open for %lus. Integral preserved at %.2f\n",
          openDuration, _integral);
        _lidOpen = false;
        _lidOpenTime = 0;
        _lidStableTime = 0;
      }
    } else {
      // Still dropping — reset stable timer
      _lidStableTime = 0;
    }

    // Minimum lid-open duration to avoid false triggers
    if (_lidOpen && (now - _lidOpenTime) < LID_OPEN_MIN_DURATION) {
      // Too soon to be sure — but keep the flag for integral freeze
    }
  }
}

uint32_t TemperatureController::getLidOpenDuration(void) {
  if (!_lidOpen) return 0;
  return (millis() - _lidOpenTime) / 1000;
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

void TemperatureController::resetError(void) {
  if (_state == STATE_ERROR) {
    _state = STATE_IDLE;
    _consecutiveErrors = 0;
    _relayControl->allOff();
    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[TEMP] Error state cleared - returned to IDLE");
    }
  }
}

// ============================================================================
// PERSISTENT INTEGRAL STORAGE (NVS)
// ============================================================================

void TemperatureController::saveIntegralToNVS() {
  if (!ENABLE_PID_PERSISTENCE) return;

  _prefs.putFloat("integral", _integral);
  _prefs.putFloat("setpoint", _setpoint);
  _lastIntegralSave = millis();

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[PID] Saved integral=%.1f setpoint=%.1f to NVS\n",
                  _integral, _setpoint);
  }
}

// ============================================================================
// TEMPERATURE HISTORY (Ring Buffer for Web Graph)
// ============================================================================

void TemperatureController::recordHistorySample() {
  unsigned long now = millis();
  if (now - _lastHistorySample < HISTORY_SAMPLE_INTERVAL) return;
  _lastHistorySample = now;

  HistorySample& s = _history[_historyHead];
  s.time = now / 1000;
  float clampedTemp = constrain(_currentTemp, -3276.0f, 3276.0f);
  s.temp = (int16_t)(clampedTemp * 10.0f);
  s.setpoint = (int16_t)(_setpoint * 10.0f);
  s.state = (uint8_t)_state;

  _historyHead = (_historyHead + 1) % HISTORY_MAX_SAMPLES;
  if (_historyCount < HISTORY_MAX_SAMPLES) _historyCount++;
}

void TemperatureController::recordHistoryEvent(ControllerState newState) {
  HistoryEvent& e = _events[_eventHead];
  e.time = millis() / 1000;
  e.state = (uint8_t)newState;

  _eventHead = (_eventHead + 1) % HISTORY_MAX_EVENTS;
  if (_eventCount < HISTORY_MAX_EVENTS) _eventCount++;
}

uint16_t TemperatureController::getHistoryCount(void) {
  return _historyCount;
}

const HistorySample& TemperatureController::getHistorySampleAt(uint16_t index) {
  // index 0 = oldest sample
  uint16_t start = (_historyCount < HISTORY_MAX_SAMPLES)
                       ? 0
                       : _historyHead;
  uint16_t pos = (start + index) % HISTORY_MAX_SAMPLES;
  return _history[pos];
}

uint8_t TemperatureController::getEventCount(void) {
  return _eventCount;
}

const HistoryEvent& TemperatureController::getHistoryEventAt(uint8_t index) {
  uint8_t start = (_eventCount < HISTORY_MAX_EVENTS)
                      ? 0
                      : _eventHead;
  uint8_t pos = (start + index) % HISTORY_MAX_EVENTS;
  return _events[pos];
}

uint32_t TemperatureController::getUptime(void) {
  return millis() / 1000;
}

void TemperatureController::loadIntegralFromNVS() {
  if (!ENABLE_PID_PERSISTENCE) return;

  float savedIntegral = _prefs.getFloat("integral", 0.0f);
  float savedSetpoint = _prefs.getFloat("setpoint", 0.0f);

  // Validate: only restore if saved setpoint is close to current setpoint
  float setpointDiff = abs(_setpoint - savedSetpoint);

  if (savedSetpoint > 0.0f && setpointDiff <= PID_SETPOINT_TOLERANCE) {
    _integral = savedIntegral;
    DUAL_LOGF(LOG_INFO,
      "[PID] Restored integral=%.1f from NVS (saved@%.0f, current@%.0f, diff=%.0f)\n",
      savedIntegral, savedSetpoint, _setpoint, setpointDiff);
  } else if (savedSetpoint > 0.0f) {
    _integral = 0.0f;
    DUAL_LOGF(LOG_INFO,
      "[PID] Discarding saved integral (setpoint diff=%.0f > tolerance=%.0f)\n",
      setpointDiff, PID_SETPOINT_TOLERANCE);
  } else {
    _integral = 0.0f;
    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[PID] No saved integral in NVS - starting at 0.0");
    }
  }
}
