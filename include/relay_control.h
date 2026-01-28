#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include <Arduino.h>

// Relay state enumeration
enum RelayState {
  RELAY_OFF = 0,
  RELAY_ON = 1
};

// Relay identification
enum RelayID {
  RELAY_AUGER = 0,   // Pellet auger motor
  RELAY_FAN = 1,     // Combustion fan
  RELAY_IGNITER = 2, // Hot rod igniter
  RELAY_COUNT = 3
};

class RelayControl {
public:
  // Constructor
  RelayControl();

  // Initialize relay pins
  void begin();

  // Set individual relay state
  void setRelay(RelayID relay, RelayState state);
  void setAuger(RelayState state);
  void setFan(RelayState state);
  void setIgniter(RelayState state);

  // Get current relay state
  RelayState getRelay(RelayID relay);
  RelayState getAuger(void);
  RelayState getFan(void);
  RelayState getIgniter(void);

  // Emergency shutdown (turn off all relays)
  void emergencyStop(void);

  // Set all relays to OFF
  void allOff(void);

  // Interlock protection: prevent auger from running without fan
  void setSafeAuger(RelayState state);

  // Get relay state as JSON-friendly object
  struct RelayStates {
    bool auger;
    bool fan;
    bool igniter;
  };
  RelayStates getStates(void);

private:
  uint8_t _pins[RELAY_COUNT];
  RelayState _states[RELAY_COUNT];
};

#endif // RELAY_CONTROL_H
