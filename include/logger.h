#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <Syslog.h>
#include "config.h"

// Syslog severity levels (RFC 5424)
#define LOG_EMERG   0  // System is unusable
#define LOG_ALERT   1  // Action must be taken immediately
#define LOG_CRIT    2  // Critical conditions
#define LOG_ERR     3  // Error conditions
#define LOG_WARNING 4  // Warning conditions
#define LOG_NOTICE  5  // Normal but significant condition
#define LOG_INFO    6  // Informational messages
#define LOG_DEBUG   7  // Debug-level messages

#if ENABLE_LOG_RING
struct LogEntry {
  uint32_t timestamp;               // millis() / 1000 (uptime seconds)
  uint32_t sequence;                // monotonic counter for incremental polling
  uint8_t  priority;                // RFC 5424 severity (0-7)
  char     tag[LOG_RING_TAG_LEN];   // e.g., "STATE", "PID", "WIFI"
  char     message[LOG_RING_MSG_LEN]; // truncated log message
};
#endif

class Logger {
public:
  Logger();
  void begin();
  void log(uint16_t priority, const char* message);
  void logf(uint16_t priority, const char* format, ...);
  bool isConnected();

  // Dual logging - outputs to Serial, Syslog, Telnet, and ring buffer
  void dualLog(uint16_t priority, const char* format, ...);

#if ENABLE_LOG_RING
  // Ring buffer access for web API
  uint8_t getLogCount();
  const LogEntry& getLogAt(uint8_t index);  // 0 = oldest
  uint32_t getLatestSequence();
#endif

private:
  WiFiUDP* _udpClient;
  Syslog* _syslog;
  bool _initialized;

#if ENABLE_LOG_RING
  LogEntry _logRing[LOG_RING_SIZE];
  uint8_t  _logHead;
  uint8_t  _logCount;
  uint32_t _logSequence;

  void appendToRing(uint8_t priority, const char* formatted);
#endif
};

// Global logger instance
extern Logger logger;

// Dual logging macros - log to both Serial and Syslog
// These should be used instead of Serial.print* for production logging
#define DUAL_LOGF(priority, ...) logger.dualLog(priority, __VA_ARGS__)

// Convenience macros for logging
#define LOG_INFO_F(...)    logger.logf(LOG_INFO, __VA_ARGS__)
#define LOG_WARNING_F(...) logger.logf(LOG_WARNING, __VA_ARGS__)
#define LOG_ERROR_F(...)   logger.logf(LOG_ERR, __VA_ARGS__)
#define LOG_DEBUG_F(...)   logger.logf(LOG_DEBUG, __VA_ARGS__)

#endif // LOGGER_H
