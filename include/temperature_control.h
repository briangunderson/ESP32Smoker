#ifndef TEMPERATURE_CONTROL_H
#define TEMPERATURE_CONTROL_H

#include <Arduino.h>
#include "max31865.h"
#include "relay_control.h"

// Controller state machine
enum ControllerState {
  STATE_IDLE = 0,
  STATE_STARTUP = 1,
  STATE_RUNNING = 2,
  STATE_COOLDOWN = 3,
  STATE_SHUTDOWN = 4,
  STATE_ERROR = 5
};

// PID-like control using hysteresis
class TemperatureController {
public:
  // Constructor
  TemperatureController(MAX31865* tempSensor, RelayControl* relayControl);

  // Initialize controller
  void begin();

  // Main control loop - call this regularly (every TEMP_CONTROL_INTERVAL ms)
  void update();

  // User commands
  void startSmoking(float targetTemp);
  void stop();
  void shutdown();
  void setSetpoint(float targetTemp);

  // Getters
  float getCurrentTemp(void);
  float getSetpoint(void);
  ControllerState getState(void);
  const char* getStateName(void);
  
  // Status structure
  struct Status {
    float currentTemp;
    float setpoint;
    ControllerState state;
    bool auger;
    bool fan;
    bool igniter;
    uint32_t runtime;
    uint8_t errorCount;
  };
  Status getStatus(void);

private:
  MAX31865* _tempSensor;
  RelayControl* _relayControl;
  
  float _setpoint;
  float _currentTemp;
  ControllerState _state;
  ControllerState _previousState;
  uint32_t _stateStartTime;
  uint32_t _lastUpdate;
  uint8_t _consecutiveErrors;

  // State machine handlers
  void handleIdleState();
  void handleStartupState();
  void handleRunningState();
  void handleCooldownState();
  void handleShutdownState();
  void handleErrorState();

  // Control logic
  void updateHysteresis();
  void manageFan();
  void manageAuger();
  void manageIgniter();

  // Fault detection
  bool readTemperature();
  void handleSensorError();
  void handleTemperatureError();

  // Utility
  unsigned long getStateElapsedTime();
};

#endif // TEMPERATURE_CONTROL_H
