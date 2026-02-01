#include "telnet_server.h"
#include <WiFi.h>

TelnetServer::TelnetServer() : _telnet(nullptr), _initialized(false) {}

void TelnetServer::begin() {
#if ENABLE_TELNET
  _telnet = new ESPTelnet();

  // Set callbacks
  _telnet->onConnect(onConnect);
  _telnet->onConnectionAttempt(onConnectionAttempt);
  _telnet->onReconnect(onReconnect);
  _telnet->onDisconnect(onDisconnect);
  _telnet->onInputReceived(onInputReceived);

  // Start telnet server
  if (_telnet->begin(TELNET_PORT)) {
    _initialized = true;
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[TELNET] Server started on port %d\n", TELNET_PORT);
      Serial.println("[TELNET] Connect with: telnet <device_ip> 23");
    }
  } else {
    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[TELNET] ERROR: Failed to start telnet server");
    }
  }
#endif
}

void TelnetServer::loop() {
#if ENABLE_TELNET
  if (_initialized && _telnet) {
    _telnet->loop();
  }
#endif
}

void TelnetServer::print(const char* message) {
#if ENABLE_TELNET
  if (_initialized && _telnet && _telnet->isConnected()) {
    _telnet->print(message);
  }
#endif
}

void TelnetServer::println(const char* message) {
#if ENABLE_TELNET
  if (_initialized && _telnet && _telnet->isConnected()) {
    _telnet->println(message);
  }
#endif
}

void TelnetServer::printf(const char* format, ...) {
#if ENABLE_TELNET
  if (!_initialized || !_telnet || !_telnet->isConnected()) {
    return;
  }

  char buffer[LOG_BUFFER_SIZE];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, LOG_BUFFER_SIZE, format, args);
  va_end(args);

  _telnet->print(buffer);
#endif
}

bool TelnetServer::isConnected() {
#if ENABLE_TELNET
  return _initialized && _telnet && _telnet->isConnected();
#else
  return false;
#endif
}

// Callback implementations
void TelnetServer::onConnect(String ip) {
  Serial.printf("[TELNET] Client connected from %s\n", ip.c_str());
}

void TelnetServer::onConnectionAttempt(String ip) {
  Serial.printf("[TELNET] Connection attempt from %s\n", ip.c_str());
}

void TelnetServer::onReconnect(String ip) {
  Serial.printf("[TELNET] Client reconnected from %s\n", ip.c_str());
}

void TelnetServer::onDisconnect(String ip) {
  Serial.printf("[TELNET] Client %s disconnected\n", ip.c_str());
}

void TelnetServer::onInputReceived(String str) {
  // Handle commands received from telnet client
  Serial.printf("[TELNET] Received: %s\n", str.c_str());

  // Echo back
  if (str == "help") {
    telnetServer.println("\n=== ESP32 Telnet Commands ===");
    telnetServer.println("  help    - Show this help");
    telnetServer.println("  status  - Show current status");
    telnetServer.println("  quit    - Disconnect");
    telnetServer.println("=============================\n");
  } else if (str == "status") {
    telnetServer.println("\n=== ESP32 Status ===");
    telnetServer.printf("  Uptime: %lu ms\n", millis());
    telnetServer.printf("  Free heap: %u bytes\n", ESP.getFreeHeap());
    telnetServer.printf("  WiFi RSSI: %d dBm\n", WiFi.RSSI());
    telnetServer.println("====================\n");
  } else if (str == "quit") {
    telnetServer.println("Goodbye!");
    // Client will disconnect
  }
}

// Global telnet server instance
TelnetServer telnetServer;
