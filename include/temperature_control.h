#ifndef TEMPERATURE_CONTROL_H
#define TEMPERATURE_CONTROL_H

#include <Arduino.h>
#include <Preferences.h>
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

// PID temperature control
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

  // Sensor access for diagnostics
  MAX31865* getSensor(void) { return _tempSensor; }

  // Debug/Testing methods
  void setDebugMode(bool enabled);
  bool isDebugMode(void);
  void setManualRelay(const char* relay, bool state);
  void setTempOverride(float temp);
  void clearTempOverride(void);
  void resetError(void);

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

  // PID Status structure for detailed diagnostics
  struct PIDStatus {
    float proportionalTerm;
    float integralTerm;
    float derivativeTerm;
    float output;
    float error;
    float setpoint;
    float currentTemp;
    float Kp;
    float Ki;
    float Kd;
    uint32_t cycleTimeRemaining;
    bool augerCycleState;
  };
  PIDStatus getPIDStatus(void);

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

  // Debug mode
  bool _debugMode;
  bool _tempOverrideEnabled;
  float _tempOverrideValue;

  // PID variables
  float _pidOutput;
  float _integral;
  float _previousError;
  float _previousTemp;        // For derivative-on-measurement
  unsigned long _lastPidUpdate;
  unsigned long _augerCycleStart;
  bool _augerCycleState;

  // Persistent integral storage
  Preferences _prefs;
  unsigned long _lastIntegralSave;

  // Calculated PID gains (from Proportional Band parameters)
  float _Kp;
  float _Ki;
  float _Kd;

  // State machine handlers
  void handleIdleState();
  void handleStartupState();
  void handleRunningState();
  void handleCooldownState();
  void handleShutdownState();
  void handleErrorState();

  // Control logic
  void updatePID();
  void applyPIDOutput();
  void manageFan();
  void manageAuger();
  void manageIgniter();

  // Fault detection
  bool readTemperature();
  void handleSensorError();
  void handleTemperatureError();

  // Utility
  unsigned long getStateElapsedTime();
  const char* stateToString(ControllerState state);

  // Persistent integral storage
  void saveIntegralToNVS();
  void loadIntegralFromNVS();
};

#endif // TEMPERATURE_CONTROL_H
