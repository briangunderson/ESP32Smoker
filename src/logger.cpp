#include "logger.h"
#include "telnet_server.h"
#include <WiFi.h>

Logger::Logger() : _udpClient(nullptr), _syslog(nullptr), _initialized(false) {}

void Logger::begin() {
#if ENABLE_SYSLOG
  // Initialize UDP client for syslog
  _udpClient = new WiFiUDP();

  // Initialize syslog with server, port, device hostname, app name, and default priority
  _syslog = new Syslog(*_udpClient, SYSLOG_SERVER, SYSLOG_PORT,
                       SYSLOG_DEVICE_NAME, SYSLOG_APP_NAME, LOG_INFO);

  _initialized = true;

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[SYSLOG] Initialized - Server: %s:%d, Device: %s\n",
                  SYSLOG_SERVER, SYSLOG_PORT, SYSLOG_DEVICE_NAME);
  }
#endif
}

void Logger::log(uint16_t priority, const char* message) {
#if ENABLE_SYSLOG
  if (_initialized && _syslog && WiFi.status() == WL_CONNECTED) {
    _syslog->log(priority, message);
  }
#endif
}

void Logger::logf(uint16_t priority, const char* format, ...) {
#if ENABLE_SYSLOG
  if (!_initialized || !_syslog || WiFi.status() != WL_CONNECTED) {
    return;
  }

  char buffer[LOG_BUFFER_SIZE];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, LOG_BUFFER_SIZE, format, args);
  va_end(args);

  _syslog->log(priority, buffer);
#endif
}

bool Logger::isConnected() {
#if ENABLE_SYSLOG
  return _initialized && WiFi.status() == WL_CONNECTED;
#else
  return false;
#endif
}

void Logger::dualLog(uint16_t priority, const char* format, ...) {
  char buffer[LOG_BUFFER_SIZE];
  va_list args;

  // Format the message once
  va_start(args, format);
  vsnprintf(buffer, LOG_BUFFER_SIZE, format, args);
  va_end(args);

  // Output to Serial if enabled
  if (ENABLE_SERIAL_DEBUG) {
    Serial.print(buffer);
  }

  // Output to Telnet if connected
#if ENABLE_TELNET
  extern TelnetServer telnetServer;
  telnetServer.print(buffer);
#endif

  // Output to Syslog if connected
#if ENABLE_SYSLOG
  if (_initialized && _syslog && WiFi.status() == WL_CONNECTED) {
    _syslog->log(priority, buffer);
  }
#endif
}

// Global logger instance
Logger logger;
