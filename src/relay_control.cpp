#include "relay_control.h"
#include "config.h"

RelayControl::RelayControl() {
  _pins[RELAY_AUGER] = PIN_RELAY_AUGER;
  _pins[RELAY_FAN] = PIN_RELAY_FAN;
  _pins[RELAY_IGNITER] = PIN_RELAY_IGNITER;

  for (int i = 0; i < RELAY_COUNT; i++) {
    _states[i] = RELAY_OFF;
  }
}

void RelayControl::begin() {
  for (int i = 0; i < RELAY_COUNT; i++) {
    pinMode(_pins[i], OUTPUT);
    digitalWrite(_pins[i], HIGH); // Active LOW: HIGH = off
  }
}

void RelayControl::setRelay(RelayID relay, RelayState state) {
  if (relay >= RELAY_COUNT)
    return;

  _states[relay] = state;
  digitalWrite(_pins[relay], state ? LOW : HIGH); // Active LOW: LOW = on

  if (ENABLE_SERIAL_DEBUG) {
    const char* relayName[] = {"AUGER", "FAN", "IGNITER"};
    Serial.printf("[RELAY] %s = %s\n", relayName[relay],
                  state ? "ON" : "OFF");
  }
}

void RelayControl::setAuger(RelayState state) {
  setRelay(RELAY_AUGER, state);
}

void RelayControl::setFan(RelayState state) {
  setRelay(RELAY_FAN, state);
}

void RelayControl::setIgniter(RelayState state) {
  setRelay(RELAY_IGNITER, state);
}

RelayState RelayControl::getRelay(RelayID relay) {
  if (relay < RELAY_COUNT) {
    return _states[relay];
  }
  return RELAY_OFF;
}

RelayState RelayControl::getAuger(void) {
  return getRelay(RELAY_AUGER);
}

RelayState RelayControl::getFan(void) {
  return getRelay(RELAY_FAN);
}

RelayState RelayControl::getIgniter(void) {
  return getRelay(RELAY_IGNITER);
}

void RelayControl::emergencyStop(void) {
  allOff();
  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[RELAY] EMERGENCY STOP - All relays OFF");
  }
}

void RelayControl::allOff(void) {
  for (int i = 0; i < RELAY_COUNT; i++) {
    setRelay((RelayID)i, RELAY_OFF);
  }
}

void RelayControl::setSafeAuger(RelayState state) {
  // Safety interlock: don't allow auger unless fan is running
  if (state == RELAY_ON && _states[RELAY_FAN] == RELAY_OFF) {
    if (ENABLE_SERIAL_DEBUG) {
      Serial.println(
          "[RELAY] Safety: Fan must be running to enable auger");
    }
    return;
  }
  setRelay(RELAY_AUGER, state);
}

RelayControl::RelayStates RelayControl::getStates(void) {
  return {_states[RELAY_AUGER] == RELAY_ON, _states[RELAY_FAN] == RELAY_ON,
          _states[RELAY_IGNITER] == RELAY_ON};
}
