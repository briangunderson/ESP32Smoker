#include "tui_server.h"
#include "ansi_utils.h"
#include "config.h"
#include <WiFi.h>

// Static instance pointer for callbacks
static TUIServer* _instance = nullptr;

// Callback functions (declared in header as friends)
void onTUIConnect(String ip) {
  if (_instance) {
    Serial.printf("[TUI] Client connected from %s\n", ip.c_str());
    _instance->clearScreen();
    _instance->renderScreen();
  }
}

void onTUIConnectionAttempt(String ip) {
  Serial.printf("[TUI] Connection attempt from %s\n", ip.c_str());
}

void onTUIReconnect(String ip) {
  if (_instance) {
    Serial.printf("[TUI] Client reconnected from %s\n", ip.c_str());
    _instance->clearScreen();
    _instance->renderScreen();
  }
}

void onTUIDisconnect(String ip) {
  Serial.printf("[TUI] Client disconnected from %s\n", ip.c_str());
}

TUIServer::TUIServer(TemperatureController* controller, MAX31865* sensor)
  : _controller(controller), _sensor(sensor), _lastUpdate(0), _updateInterval(1000) {
  _instance = this;
}

void TUIServer::begin(uint16_t port) {
  // Set up telnet event handlers with static function pointers
  _telnet.onConnect(onTUIConnect);
  _telnet.onConnectionAttempt(onTUIConnectionAttempt);
  _telnet.onReconnect(onTUIReconnect);
  _telnet.onDisconnect(onTUIDisconnect);

  // Start telnet server on specified port
  if (_telnet.begin(port)) {
    Serial.printf("[TUI] Telnet TUI server started on port %d\n", port);
  } else {
    Serial.println("[TUI] Failed to start telnet TUI server");
  }
}

void TUIServer::update() {
  _telnet.loop();

  // Update display at specified interval if clients are connected
  if (_telnet.isConnected() && (millis() - _lastUpdate >= _updateInterval)) {
    _lastUpdate = millis();
    renderScreen();
  }
}

bool TUIServer::hasClients() {
  return _telnet.isConnected();
}

void TUIServer::clearScreen() {
  _telnet.print(ANSI::CLEAR_SCREEN);
  _telnet.print(ANSI::CURSOR_HOME);
  _telnet.print(ANSI::HIDE_CURSOR);
}

void TUIServer::moveCursor(int row, int col) {
  _telnet.print(ANSI::cursorTo(row, col));
}

void TUIServer::print(const String& text) {
  _telnet.print(text);
}

void TUIServer::println(const String& text) {
  _telnet.println(text);
}

void TUIServer::renderScreen() {
  moveCursor(1, 1);
  renderHeader();
  renderTemperature();
  renderPIDStatus();
  renderStateMachine();
  renderRelayStatus();
  renderMAX31865Diagnostics();
  renderNetworkStatus();
  renderFooter();
}

void TUIServer::renderHeader() {
  print(ANSI::BOLD);
  print(ANSI::FG_BRIGHT_CYAN);
  println("╔════════════════════════════════════════════════════════════════════════════╗");
  println("║              ESP32 WOOD PELLET SMOKER CONTROLLER - TUI                    ║");
  println("╚════════════════════════════════════════════════════════════════════════════╝");
  print(ANSI::RESET);
  println("");
}

void TUIServer::renderTemperature() {
  auto status = _controller->getStatus();

  print(ANSI::BOLD);
  print(ANSI::FG_BRIGHT_WHITE);
  print("┌─ TEMPERATURE ");
  print(ANSI::RESET);
  println("─────────────────────────────────────────────────────────────────┐");

  // Current temperature
  print("│ ");
  print(ANSI::BOLD);
  print(ANSI::FG_YELLOW);
  print("Current Temp: ");
  print(ANSI::FG_BRIGHT_YELLOW);
  print(padRight(String(status.currentTemp, 1) + "°F", 15));
  print(ANSI::RESET);

  // Setpoint
  print("  ");
  print(ANSI::BOLD);
  print(ANSI::FG_CYAN);
  print("Setpoint: ");
  print(ANSI::FG_BRIGHT_CYAN);
  print(padRight(String(status.setpoint, 1) + "°F", 15));
  print(ANSI::RESET);

  // Error
  print("  ");
  print(ANSI::BOLD);
  print(ANSI::FG_MAGENTA);
  print("Error: ");
  float error = status.currentTemp - status.setpoint;
  if (error > 0) {
    print(ANSI::FG_BRIGHT_RED);
    print("+");
  } else {
    print(ANSI::FG_BRIGHT_GREEN);
  }
  print(padLeft(String(error, 1) + "°F", 10));
  print(ANSI::RESET);
  println(" │");

  println("└────────────────────────────────────────────────────────────────────────────┘");
  println("");
}

void TUIServer::renderPIDStatus() {
  auto pidStatus = _controller->getPIDStatus();

  print(ANSI::BOLD);
  print(ANSI::FG_BRIGHT_WHITE);
  print("┌─ PID CONTROLLER ");
  print(ANSI::RESET);
  println("──────────────────────────────────────────────────────────────┐");

  // First row: P, I, D terms
  print("│ ");
  print(ANSI::FG_GREEN);
  print("P: ");
  print(ANSI::FG_BRIGHT_GREEN);
  print(padRight(String(pidStatus.proportionalTerm, 4), 10));
  print(ANSI::RESET);

  print("  ");
  print(ANSI::FG_BLUE);
  print("I: ");
  print(ANSI::FG_BRIGHT_BLUE);
  print(padRight(String(pidStatus.integralTerm, 4), 10));
  print(ANSI::RESET);

  print("  ");
  print(ANSI::FG_MAGENTA);
  print("D: ");
  print(ANSI::FG_BRIGHT_MAGENTA);
  print(padRight(String(pidStatus.derivativeTerm, 4), 10));
  print(ANSI::RESET);

  print("  ");
  print(ANSI::BOLD);
  print(ANSI::FG_YELLOW);
  print("Output: ");
  print(ANSI::FG_BRIGHT_YELLOW);
  print(padRight(String(pidStatus.output * 100.0, 1) + "%", 10));
  print(ANSI::RESET);
  println(" │");

  // Second row: Gains
  print("│ ");
  print(ANSI::FG_GREEN);
  print("Kp: ");
  print(ANSI::FG_BRIGHT_GREEN);
  print(padRight(String(pidStatus.Kp, 6), 9));
  print(ANSI::RESET);

  print("  ");
  print(ANSI::FG_BLUE);
  print("Ki: ");
  print(ANSI::FG_BRIGHT_BLUE);
  print(padRight(String(pidStatus.Ki, 8), 9));
  print(ANSI::RESET);

  print("  ");
  print(ANSI::FG_MAGENTA);
  print("Kd: ");
  print(ANSI::FG_BRIGHT_MAGENTA);
  print(padRight(String(pidStatus.Kd, 6), 9));
  print(ANSI::RESET);

  println("                    │");

  // Third row: Auger cycle info
  print("│ ");
  print(ANSI::FG_CYAN);
  print("Auger Cycle: ");
  print(ANSI::FG_BRIGHT_CYAN);
  print(pidStatus.augerCycleState ? "ON " : "OFF");
  print(ANSI::RESET);
  print("  ");
  print(ANSI::FG_CYAN);
  print("Time Remaining: ");
  print(ANSI::FG_BRIGHT_CYAN);
  print(padRight(String(pidStatus.cycleTimeRemaining / 1000.0, 1) + "s", 10));
  print(ANSI::RESET);
  println("                │");

  println("└────────────────────────────────────────────────────────────────────────────┘");
  println("");
}

void TUIServer::renderStateMachine() {
  auto status = _controller->getStatus();
  ControllerState state = status.state;

  print(ANSI::BOLD);
  print(ANSI::FG_BRIGHT_WHITE);
  print("┌─ STATE MACHINE ");
  print(ANSI::RESET);
  println("────────────────────────────────────────────────────────────────┐");

  print("│ ");
  print(ANSI::BOLD);
  print("State: ");
  print(getStateColor(state));
  print(padRight(_controller->getStateName(), 15));
  print(ANSI::RESET);

  print("  ");
  print(ANSI::BOLD);
  print("Runtime: ");
  print(ANSI::FG_BRIGHT_WHITE);
  print(padRight(formatUptime(status.runtime), 20));
  print(ANSI::RESET);

  print("  ");
  print(ANSI::BOLD);
  print("Errors: ");
  if (status.errorCount > 0) {
    print(ANSI::FG_BRIGHT_RED);
  } else {
    print(ANSI::FG_BRIGHT_GREEN);
  }
  print(String(status.errorCount));
  print(ANSI::RESET);
  println("    │");

  println("└────────────────────────────────────────────────────────────────────────────┘");
  println("");
}

void TUIServer::renderRelayStatus() {
  auto relayStates = _controller->getStatus();

  print(ANSI::BOLD);
  print(ANSI::FG_BRIGHT_WHITE);
  print("┌─ RELAY STATUS ");
  print(ANSI::RESET);
  println("───────────────────────────────────────────────────────────────┐");

  print("│ ");

  // Auger
  print(ANSI::BOLD);
  print("Auger: ");
  if (relayStates.auger) {
    print(ANSI::FG_BRIGHT_GREEN);
    print("ON ");
  } else {
    print(ANSI::FG_BRIGHT_BLACK);
    print("OFF");
  }
  print(ANSI::RESET);

  print("     ");

  // Fan
  print(ANSI::BOLD);
  print("Fan: ");
  if (relayStates.fan) {
    print(ANSI::FG_BRIGHT_GREEN);
    print("ON ");
  } else {
    print(ANSI::FG_BRIGHT_BLACK);
    print("OFF");
  }
  print(ANSI::RESET);

  print("     ");

  // Igniter
  print(ANSI::BOLD);
  print("Igniter: ");
  if (relayStates.igniter) {
    print(ANSI::FG_BRIGHT_GREEN);
    print("ON ");
  } else {
    print(ANSI::FG_BRIGHT_BLACK);
    print("OFF");
  }
  print(ANSI::RESET);

  println("                                │");
  println("└────────────────────────────────────────────────────────────────────────────┘");
  println("");
}

void TUIServer::renderMAX31865Diagnostics() {
  print(ANSI::BOLD);
  print(ANSI::FG_BRIGHT_WHITE);
  print("┌─ MAX31865 RTD SENSOR ");
  print(ANSI::RESET);
  println("───────────────────────────────────────────────────────┐");

  // Read raw RTD value
  uint16_t rawRTD = _sensor->readRawRTD();
  float resistance = (rawRTD * MAX31865_REFERENCE_RESISTANCE) / 32768.0;

  print("│ ");
  print(ANSI::FG_CYAN);
  print("Raw ADC: ");
  print(ANSI::FG_BRIGHT_CYAN);
  print(padRight(String(rawRTD), 10));
  print(ANSI::RESET);

  print("  ");
  print(ANSI::FG_CYAN);
  print("Resistance: ");
  print(ANSI::FG_BRIGHT_CYAN);
  print(padRight(String(resistance, 2) + "Ω", 15));
  print(ANSI::RESET);

  print("  ");
  print(ANSI::FG_CYAN);
  print("Ref: ");
  print(ANSI::FG_BRIGHT_CYAN);
  print(padRight(String(MAX31865_REFERENCE_RESISTANCE, 0) + "Ω", 10));
  print(ANSI::RESET);
  println(" │");

  // Fault status
  uint8_t faultStatus = _sensor->getFaultStatus();
  print("│ ");
  print(ANSI::BOLD);
  print("Fault Status: ");
  if (faultStatus == 0) {
    print(ANSI::FG_BRIGHT_GREEN);
    print("OK (0x00)");
  } else {
    print(ANSI::FG_BRIGHT_RED);
    print("FAULT (0x");
    print(String(faultStatus, HEX));
    print(")");
  }
  print(ANSI::RESET);

  // Decode faults
  if (faultStatus != 0) {
    print("  [");
    if (faultStatus & MAX31865_FAULT_HIGHTEMP) print("HIGH_TEMP ");
    if (faultStatus & MAX31865_FAULT_LOWTEMP) print("LOW_TEMP ");
    if (faultStatus & MAX31865_FAULT_RTDIN) print("RTDIN_HIGH ");
    if (faultStatus & MAX31865_FAULT_REFIN) print("REFIN_HIGH ");
    if (faultStatus & MAX31865_FAULT_REFIN_LO) print("REFIN_LOW ");
    if (faultStatus & MAX31865_FAULT_RTDIN_LO) print("RTDIN_LOW ");
    print("]");
  }

  println("          │");

  // Health status
  print("│ ");
  print(ANSI::BOLD);
  print("Health: ");
  if (_sensor->isHealthy()) {
    print(ANSI::FG_BRIGHT_GREEN);
    print("HEALTHY");
  } else {
    print(ANSI::FG_BRIGHT_RED);
    print("UNHEALTHY");
  }
  print(ANSI::RESET);
  println("                                                              │");

  println("└────────────────────────────────────────────────────────────────────────────┘");
  println("");
}

void TUIServer::renderNetworkStatus() {
  print(ANSI::BOLD);
  print(ANSI::FG_BRIGHT_WHITE);
  print("┌─ NETWORK STATUS ");
  print(ANSI::RESET);
  println("──────────────────────────────────────────────────────────────┐");

  // WiFi status
  print("│ ");
  print(ANSI::BOLD);
  print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    print(ANSI::FG_BRIGHT_GREEN);
    print("CONNECTED");
  } else {
    print(ANSI::FG_BRIGHT_RED);
    print("DISCONNECTED");
  }
  print(ANSI::RESET);

  print("  ");
  print(ANSI::FG_CYAN);
  print("SSID: ");
  print(ANSI::FG_BRIGHT_CYAN);
  print(padRight(WiFi.SSID(), 20));
  print(ANSI::RESET);

  print("  ");
  print(ANSI::FG_CYAN);
  print("RSSI: ");
  int rssi = WiFi.RSSI();
  if (rssi > -50) {
    print(ANSI::FG_BRIGHT_GREEN);
  } else if (rssi > -70) {
    print(ANSI::FG_BRIGHT_YELLOW);
  } else {
    print(ANSI::FG_BRIGHT_RED);
  }
  print(padRight(String(rssi) + "dBm", 10));
  print(ANSI::RESET);
  println(" │");

  // IP address
  print("│ ");
  print(ANSI::FG_CYAN);
  print("IP Address: ");
  print(ANSI::FG_BRIGHT_CYAN);
  print(padRight(WiFi.localIP().toString(), 20));
  print(ANSI::RESET);

  print("  ");
  print(ANSI::FG_CYAN);
  print("Hostname: ");
  print(ANSI::FG_BRIGHT_CYAN);
  print(padRight(WiFi.getHostname(), 25));
  print(ANSI::RESET);
  println(" │");

  println("└────────────────────────────────────────────────────────────────────────────┘");
  println("");
}

void TUIServer::renderFooter() {
  print(ANSI::FG_BRIGHT_BLACK);
  print("Firmware: ");
  print(FIRMWARE_VERSION);
  print("  │  Uptime: ");
  print(formatUptime(millis()));
  print("  │  Press Ctrl+] then 'quit' to disconnect");
  print(ANSI::RESET);
  println("");
}

String TUIServer::padRight(String text, int width) {
  while (text.length() < width) {
    text += " ";
  }
  if (text.length() > width) {
    text = text.substring(0, width);
  }
  return text;
}

String TUIServer::padLeft(String text, int width) {
  while (text.length() < width) {
    text = " " + text;
  }
  if (text.length() > width) {
    text = text.substring(0, width);
  }
  return text;
}

const char* TUIServer::boolToOnOff(bool value) {
  return value ? "ON" : "OFF";
}

const char* TUIServer::getStateColor(ControllerState state) {
  switch (state) {
    case STATE_IDLE:
      return ANSI::FG_BRIGHT_BLACK;
    case STATE_STARTUP:
      return ANSI::FG_BRIGHT_YELLOW;
    case STATE_RUNNING:
      return ANSI::FG_BRIGHT_GREEN;
    case STATE_COOLDOWN:
      return ANSI::FG_BRIGHT_BLUE;
    case STATE_SHUTDOWN:
      return ANSI::FG_BRIGHT_MAGENTA;
    case STATE_ERROR:
      return ANSI::FG_BRIGHT_RED;
    default:
      return ANSI::FG_WHITE;
  }
}

String TUIServer::formatUptime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;

  seconds %= 60;
  minutes %= 60;
  hours %= 24;

  String uptime = "";
  if (days > 0) {
    uptime += String(days) + "d ";
  }
  if (hours > 0 || days > 0) {
    uptime += String(hours) + "h ";
  }
  if (minutes > 0 || hours > 0 || days > 0) {
    uptime += String(minutes) + "m ";
  }
  uptime += String(seconds) + "s";

  return uptime;
}
