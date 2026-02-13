#ifndef TEMPERATURE_CONTROL_H
#define TEMPERATURE_CONTROL_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "max31865.h"
#include "relay_control.h"

// Controller state machine
enum ControllerState {
  STATE_IDLE = 0,
  STATE_STARTUP = 1,
  STATE_RUNNING = 2,
  STATE_COOLDOWN = 3,
  STATE_SHUTDOWN = 4,
  STATE_ERROR = 5,
  STATE_REIGNITE = 6
};

// Temperature history sample (for web graph)
// 12 bytes/sample with natural alignment (was 16 with floats)
struct HistorySample {
  uint32_t time;     // seconds since boot (millis()/1000)
  int16_t temp;      // current temperature °F × 10 (2253 = 225.3°F)
  int16_t setpoint;  // target temperature °F × 10
  uint8_t state;     // ControllerState enum value
};

// State change event
struct HistoryEvent {
  uint32_t time;     // seconds since boot
  uint8_t state;     // new state entered
};

// Safety: ESP32-S3 no PSRAM needs ~100KB free heap for WiFi.
// History buffer must not exceed 40KB to leave headroom.
static_assert(
  sizeof(HistorySample) * HISTORY_MAX_SAMPLES + sizeof(HistoryEvent) * HISTORY_MAX_EVENTS <= 40960,
  "History buffers exceed 40KB — WiFi will fail on ESP32-S3 without PSRAM"
);

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

  // Reignite status
  uint8_t getReigniteAttempts(void) { return _reigniteAttempts; }
  uint8_t getReignitePhase(void) { return _reignitePhase; }

  // Lid-open detection
  bool isLidOpen(void) { return _lidOpen; }
  uint32_t getLidOpenDuration(void);

  // History access for web graph
  uint16_t getHistoryCount(void);
  const HistorySample& getHistorySampleAt(uint16_t index);  // 0 = oldest
  uint8_t getEventCount(void);
  const HistoryEvent& getHistoryEventAt(uint8_t index);     // 0 = oldest
  uint32_t getUptime(void);

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
  float _lastP;               // Last proportional term (for getPIDStatus)
  float _lastI;               // Last integral term (for getPIDStatus)
  float _lastD;               // Last derivative term (for getPIDStatus)
  unsigned long _lastPidUpdate;
  unsigned long _augerCycleStart;
  bool _augerCycleState;

  // Persistent integral storage
  Preferences _prefs;
  unsigned long _lastIntegralSave;

  // Temperature history ring buffer
  HistorySample _history[HISTORY_MAX_SAMPLES];
  uint16_t _historyHead;       // next write position
  uint16_t _historyCount;      // number of valid samples
  unsigned long _lastHistorySample;

  // State change event ring buffer
  HistoryEvent _events[HISTORY_MAX_EVENTS];
  uint8_t _eventHead;
  uint8_t _eventCount;

  // Reignite logic
  uint8_t _reigniteAttempts;     // counter for current cook session
  uint8_t _reignitePhase;        // 0=fan clear, 1=preheat, 2=feeding, 3=recovery
  uint32_t _reignitePhaseStart;  // millis timestamp of current phase start
  uint32_t _pidMaxedSince;       // millis when PID output first hit max (0 = not maxed)

  // Lid-open detection
  bool _lidOpen;                 // current lid state
  uint32_t _lidOpenTime;         // millis when lid was detected open
  uint32_t _lidStableTime;       // millis when temp rate stabilized after lid-open

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
  void handleReigniteState();

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

  // Lid detection
  void detectLidOpen();

  // Utility
  unsigned long getStateElapsedTime();
  const char* stateToString(ControllerState state);

  // Persistent integral storage
  void saveIntegralToNVS();
  void loadIntegralFromNVS();

  // History recording
  void recordHistorySample();
  void recordHistoryEvent(ControllerState newState);
};

#endif // TEMPERATURE_CONTROL_H
