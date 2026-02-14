#include "logger.h"

// Global logger instance (required by logger.h extern declaration)
Logger logger;

Logger::Logger() : _udpClient(nullptr), _syslog(nullptr), _initialized(false) {}
void Logger::begin() { _initialized = true; }
void Logger::log(uint16_t priority, const char* message) { (void)priority; (void)message; }
void Logger::logf(uint16_t priority, const char* format, ...) { (void)priority; (void)format; }
bool Logger::isConnected() { return false; }
void Logger::dualLog(uint16_t priority, const char* format, ...) { (void)priority; (void)format; }
