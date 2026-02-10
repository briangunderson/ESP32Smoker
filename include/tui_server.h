#ifndef TUI_SERVER_H
#define TUI_SERVER_H

#include <Arduino.h>
#include <ESPTelnet.h>
#include "temperature_control.h"
#include "max31865.h"

// TUI Server for real-time status display
class TUIServer {
public:
  // Constructor
  TUIServer(TemperatureController* controller, MAX31865* sensor);

  // Initialize TUI server
  void begin(uint16_t port = 2323);

  // Main loop - call this regularly
  void update();

  // Check if clients are connected
  bool hasClients();

  // Public methods for callbacks
  void clearScreen();
  void renderScreen();

private:
  ESPTelnet _telnet;
  TemperatureController* _controller;
  MAX31865* _sensor;

  unsigned long _lastUpdate;
  uint16_t _updateInterval;

  // Rendering functions
  void renderHeader();
  void renderTemperature();
  void renderPIDStatus();
  void renderStateMachine();
  void renderRelayStatus();
  void renderMAX31865Diagnostics();
  void renderNetworkStatus();
  void renderFooter();

  // Helper functions
  void moveCursor(int row, int col);
  void print(const String& text);
  void println(const String& text);
  String padRight(String text, int width);
  String padLeft(String text, int width);
  const char* boolToOnOff(bool value);
  const char* getStateColor(ControllerState state);
  String formatUptime(unsigned long ms);
};

#endif // TUI_SERVER_H
