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

class Logger {
public:
  Logger();
  void begin();
  void log(uint16_t priority, const char* message);
  void logf(uint16_t priority, const char* format, ...);
  bool isConnected();

  // Dual logging - outputs to both Serial and Syslog
  void dualLog(uint16_t priority, const char* format, ...);

private:
  WiFiUDP* _udpClient;
  Syslog* _syslog;
  bool _initialized;
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
