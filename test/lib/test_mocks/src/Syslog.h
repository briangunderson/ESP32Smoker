#ifndef MOCK_SYSLOG_H
#define MOCK_SYSLOG_H

#include "WiFiUdp.h"

// Facility codes
#ifndef LOG_LOCAL0
#define LOG_LOCAL0 (16 << 3)
#endif

class Syslog {
public:
    Syslog(WiFiUDP &client, const char* server, uint16_t port,
           const char* deviceHostname, const char* appName,
           uint16_t priDefault, uint8_t mask = 0xFF) {
        (void)client; (void)server; (void)port;
        (void)deviceHostname; (void)appName;
        (void)priDefault; (void)mask;
    }
    Syslog(WiFiUDP &client, uint16_t priDefault) {
        (void)client; (void)priDefault;
    }
    bool log(uint16_t pri, const char* msg) { (void)pri; (void)msg; return true; }
    bool logf(uint16_t pri, const char* fmt, ...) { (void)pri; (void)fmt; return true; }
};

#endif // MOCK_SYSLOG_H
