#ifndef TELNET_SERVER_H
#define TELNET_SERVER_H

#include <Arduino.h>
#include <ESPTelnet.h>
#include "config.h"

class TelnetServer {
public:
  TelnetServer();
  void begin();
  void loop();
  void print(const char* message);
  void println(const char* message);
  void printf(const char* format, ...);
  bool isConnected();

private:
  ESPTelnet* _telnet;
  bool _initialized;

  // Callbacks
  static void onConnect(String ip);
  static void onConnectionAttempt(String ip);
  static void onReconnect(String ip);
  static void onDisconnect(String ip);
  static void onInputReceived(String str);
};

// Global telnet server instance
extern TelnetServer telnetServer;

// Unified print macros - output to Serial, Syslog, AND Telnet
#define UNIFIED_PRINTF(priority, ...) do { \
  char buffer[LOG_BUFFER_SIZE]; \
  snprintf(buffer, LOG_BUFFER_SIZE, __VA_ARGS__); \
  if (ENABLE_SERIAL_DEBUG) Serial.print(buffer); \
  logger.logf(priority, __VA_ARGS__); \
  telnetServer.print(buffer); \
} while(0)

#endif // TELNET_SERVER_H
