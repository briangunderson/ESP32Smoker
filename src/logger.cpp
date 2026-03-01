#include "logger.h"
#include "telnet_server.h"
#include <WiFi.h>

Logger::Logger() : _udpClient(nullptr), _syslog(nullptr), _initialized(false)
#if ENABLE_LOG_RING
  , _logHead(0), _logCount(0), _logSequence(0)
#endif
{}

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

  // Append to ring buffer for web UI
#if ENABLE_LOG_RING
  appendToRing(priority, buffer);
#endif
}

#if ENABLE_LOG_RING
void Logger::appendToRing(uint8_t priority, const char* formatted) {
  LogEntry& entry = _logRing[_logHead];
  entry.timestamp = millis() / 1000;
  entry.priority = priority;
  entry.sequence = _logSequence++;

  // Parse [TAG] prefix from formatted message
  const char* p = formatted;
  // Skip leading newlines (some logs start with \n for readability)
  while (*p == '\n') p++;

  if (*p == '[') {
    p++;
    const char* end = strchr(p, ']');
    if (end && (size_t)(end - p) < LOG_RING_TAG_LEN) {
      size_t len = end - p;
      memcpy(entry.tag, p, len);
      entry.tag[len] = '\0';
      p = end + 1;
      while (*p == ' ') p++;  // skip space after ]
    } else {
      strncpy(entry.tag, "LOG", LOG_RING_TAG_LEN);
      entry.tag[LOG_RING_TAG_LEN - 1] = '\0';
      p = formatted;
    }
  } else {
    strncpy(entry.tag, "LOG", LOG_RING_TAG_LEN);
    entry.tag[LOG_RING_TAG_LEN - 1] = '\0';
  }

  // Copy message, strip trailing newlines
  strncpy(entry.message, p, LOG_RING_MSG_LEN - 1);
  entry.message[LOG_RING_MSG_LEN - 1] = '\0';
  size_t mlen = strlen(entry.message);
  while (mlen > 0 && (entry.message[mlen - 1] == '\n' || entry.message[mlen - 1] == '\r')) {
    entry.message[--mlen] = '\0';
  }

  _logHead = (_logHead + 1) % LOG_RING_SIZE;
  if (_logCount < LOG_RING_SIZE) _logCount++;
}

uint8_t Logger::getLogCount() {
  return _logCount;
}

const LogEntry& Logger::getLogAt(uint8_t index) {
  // index 0 = oldest entry
  uint8_t pos;
  if (_logCount < LOG_RING_SIZE) {
    pos = index;
  } else {
    pos = (_logHead + index) % LOG_RING_SIZE;
  }
  return _logRing[pos];
}

uint32_t Logger::getLatestSequence() {
  return _logSequence > 0 ? _logSequence - 1 : 0;
}
#endif

// Global logger instance
Logger logger;
