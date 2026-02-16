#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Include secrets (copy secrets.h.example to secrets.h and fill in your values)
#if __has_include("secrets.h")
  #include "secrets.h"
#endif

// ============================================================================
// PIN CONFIGURATION
// ============================================================================

// ==========================================================================
// Adafruit Feather ESP32-S3 No PSRAM Pin Assignments
// NOTE: GPIO 19/20 = USB (off limits), GPIO 25-27 = not on headers
// ==========================================================================

// SPI Pins for MAX31865 (RTD Temperature Sensor)
#define PIN_SPI_CLK     36  // SCK header pin
#define PIN_SPI_MOSI    35  // MO header pin
#define PIN_SPI_MISO    37  // MI header pin
#define PIN_MAX31865_CS 5   // D5 header pin

// Relay Control Pins
#define PIN_RELAY_AUGER   12  // D12 header pin
#define PIN_RELAY_FAN     13  // D13 header pin
#define PIN_RELAY_IGNITER 10  // D10 header pin

// Status LED
#define PIN_LED_STATUS    11  // D11 header pin

// TM1638 Display Module (7-segment displays, LEDs, buttons)
#define PIN_TM1638_STB    6   // D6 header pin - Strobe
#define PIN_TM1638_CLK    9   // D9 header pin - Clock
#define PIN_TM1638_DIO    14  // A4 header pin - Data I/O

// I2C Pins (STEMMA QT connector on Feather ESP32-S3)
#define PIN_I2C_SDA           3   // GPIO3 - STEMMA QT SDA
#define PIN_I2C_SCL           4   // GPIO4 - STEMMA QT SCL

// M5Stack Unit Encoder (U135) - I2C rotary encoder with button + RGB LED
#define ENCODER_I2C_ADDR      0x40  // Default I2C address
#define ENCODER_POLL_INTERVAL 50    // ms between I2C reads (20 Hz)
#define ENCODER_STEP_DEGREES  5     // °F per encoder click
#define ENCODER_BTN_DEBOUNCE  300   // ms button debounce

// ============================================================================
// SENSOR CONFIGURATION
// ============================================================================

// MAX31865 RTD Configuration
#define MAX31865_REFERENCE_RESISTANCE 4300.0  // 4300 ohm reference resistor (for PT1000)
#define MAX31865_RTD_RESISTANCE_AT_0  1000.0  // PT1000 = 1000 ohms at 0°C
#define MAX31865_WIRE_MODE            3       // 3-wire RTD (most common)

// Temperature Sensor Calibration
#define TEMP_SENSOR_OFFSET 0.0  // °F offset calibration
#define TEMP_SAMPLE_COUNT  5    // Number of samples to average
#define TEMP_SAMPLE_DELAY  100  // ms between samples

// ============================================================================
// CONTROL CONFIGURATION
// ============================================================================

// Temperature Control Parameters (PID)
#define TEMP_CONTROL_INTERVAL    2000  // ms between PID updates
#define TEMP_MIN_SETPOINT        150   // Minimum allowed setpoint (°F)
#define TEMP_MAX_SETPOINT        500   // Maximum allowed setpoint (°F)

// PID Configuration - Proportional Band Method (from PiSmoker)
// This method uses negative gains with 0.5 centering for stable control
#define PID_PROPORTIONAL_BAND    60.0  // Proportional band in °F
#define PID_INTEGRAL_TIME        180.0 // Integral time in seconds
#define PID_DERIVATIVE_TIME      45.0  // Derivative time in seconds


// PID Output Limits (0.0 to 1.0 range, where 1.0 = 100%)
#define PID_OUTPUT_MIN           0.15  // 15% minimum to keep fire alive
#define PID_OUTPUT_MAX           1.0   // 100% maximum

// Auger Cycle Time for time-proportioning control
#define AUGER_CYCLE_TIME         20000 // ms (20 seconds - matches PiSmoker)

// Persistent PID Integral Storage (NVS)
#define ENABLE_PID_PERSISTENCE     true     // Save/restore integral across sessions
#define PID_SETPOINT_TOLERANCE     20.0     // °F - only restore if setpoint within this range
#define PID_SAVE_INTERVAL          300000   // ms (5 min) - periodic save during RUNNING

// Reignite Logic (auto-recovery from dead fire)
#define ENABLE_REIGNITE            true
#define REIGNITE_TEMP_THRESHOLD    140.0    // °F - below this, fire may be out
#define REIGNITE_TRIGGER_TIME      120000   // ms - PID maxed for this long triggers reignite
#define REIGNITE_MAX_ATTEMPTS      3        // Max reignite attempts per cook session
#define REIGNITE_FAN_CLEAR_TIME    30000    // ms - fan clears ash from firepot
#define REIGNITE_PREHEAT_TIME      60000    // ms - igniter preheats
#define REIGNITE_FEED_TIME         30000    // ms - auger feeds fresh pellets
#define REIGNITE_RECOVERY_TIME     120000   // ms - wait for temp to rise

// Lid-Open Detection (freezes PID integral during lid events)
#define ENABLE_LID_DETECTION           true
#define LID_OPEN_DERIVATIVE_THRESHOLD  -2.0    // °F/s - rate of temp drop to trigger
#define LID_CLOSE_RECOVERY_TIME        30000   // ms - stable before declaring lid closed
#define LID_OPEN_MIN_DURATION          5000    // ms - minimum to avoid false triggers

// Temperature History (ring buffer for web graph)
// Budget: ~30KB for history (ESP32-S3 no PSRAM needs ~100KB free for WiFi)
// 2500 samples × 12 bytes = 30KB → ~14 hours at 20s intervals
#define HISTORY_MAX_SAMPLES        2500     // ~14 hours at 20-second intervals
#define HISTORY_SAMPLE_INTERVAL    20000    // ms between history samples
#define HISTORY_MAX_EVENTS         64       // State change events to keep

// Temperature thresholds
#define STARTUP_TEMP_THRESHOLD   115   // Absolute °F to transition from startup to running
#define IGNITER_CUTOFF_TEMP      100   // Turn off igniter when temp exceeds this

// Startup/Shutdown Behavior
#define IGNITER_PREHEAT_TIME     60000 // ms to preheat igniter before starting
#define FAN_STARTUP_DELAY        5000  // ms to delay fan after igniter
#define STARTUP_TIMEOUT          180000 // ms before startup failure
#ifndef BOOT_GRACE_PERIOD_MS
#define BOOT_GRACE_PERIOD_MS     10000  // ms to ignore start commands after boot
#endif
#define SHUTDOWN_COOL_TIMEOUT    300000 // ms to wait before shutdown complete

// Safety Limits
#define TEMP_MAX_SAFE            550   // Maximum safe temperature (°F)
#define TEMP_MIN_SAFE            50    // Minimum safe temperature (°F)
#define SENSOR_ERROR_THRESHOLD   5     // Max consecutive sensor errors before shutdown

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

// Wi-Fi Settings (set in secrets.h)
#ifndef WIFI_SSID
  #define WIFI_SSID    "your-wifi-ssid"
#endif
#ifndef WIFI_PASS
  #define WIFI_PASS    "your-wifi-password"
#endif

// Wi-Fi AP Mode (fallback if no SSID configured)
#ifndef WIFI_AP_SSID
  #define WIFI_AP_SSID "ESP32Smoker"
#endif
#ifndef WIFI_AP_PASS
  #define WIFI_AP_PASS "your-ap-password"
#endif

// Web Server Port
#define WEB_SERVER_PORT 80

// ============================================================================
// MQTT CONFIGURATION
// ============================================================================

// MQTT credentials (set in secrets.h)
#ifndef MQTT_BROKER_HOST
  #define MQTT_BROKER_HOST    "192.168.1.100"
#endif
#define MQTT_BROKER_PORT    1883
#define MQTT_CLIENT_ID      "esp32-smoker"
#ifndef MQTT_USERNAME
  #define MQTT_USERNAME       "your-mqtt-user"
#endif
#ifndef MQTT_PASSWORD
  #define MQTT_PASSWORD       "your-mqtt-password"
#endif
#define MQTT_ROOT_TOPIC     "home/smoker"
#define MQTT_RECONNECT_INTERVAL 5000  // ms

// MQTT Publish Intervals
#define MQTT_STATUS_INTERVAL    5000   // Publish status every 5 seconds
#define MQTT_TELEMETRY_INTERVAL 60000  // Publish telemetry every minute

// ============================================================================
// STORAGE CONFIGURATION
// ============================================================================

#define SPIFFS_CONFIG_FILE   "/config.json"
#define SPIFFS_CALIB_FILE    "/calibration.json"
#define SPIFFS_LOG_FILE      "/session.log"
#define SPIFFS_PARTITION     "littlefs"

// ============================================================================
// LOGGING & DEBUG
// ============================================================================

#define ENABLE_SERIAL_DEBUG  true
#define SERIAL_BAUD_RATE     115200
#define LOG_BUFFER_SIZE      256

// Syslog Configuration (Cribl/Elastic Stack)
#define ENABLE_SYSLOG        true                    // Enable remote syslog
#ifndef SYSLOG_SERVER
  #define SYSLOG_SERVER      "192.168.1.100"         // Set in secrets.h
#endif
#define SYSLOG_PORT          9543                    // Cribl syslog UDP port
#define SYSLOG_DEVICE_NAME   "ESP32Smoker"           // Device hostname for syslog
#define SYSLOG_APP_NAME      "smoker"                // Application name in logs
#define SYSLOG_FACILITY      LOG_LOCAL0              // Syslog facility (local0 = 16)

// Telnet Server Configuration (Remote Serial Monitor)
#define ENABLE_TELNET        true                    // Enable telnet server
#define TELNET_PORT          23                      // Standard telnet port

// TUI Server Configuration (Real-time Status Interface)
#define ENABLE_TUI           false                   // Enable TUI telnet server (DISABLED - debugging)
#define TUI_PORT             2323                    // TUI telnet port

// MAX31865 Verbose Debugging (logs every sensor read with resistance values)
#define ENABLE_MAX31865_VERBOSE  false               // Disable to reduce Serial load

// ============================================================================
// OTA CONFIGURATION
// ============================================================================

#ifndef OTA_PASSWORD
  #define OTA_PASSWORD       "your-ota-password"     // Set in secrets.h
#endif

// ============================================================================
// HTTP OTA CONFIGURATION (Pull-based updates from GitHub Releases)
// ============================================================================

#define ENABLE_HTTP_OTA          true
#define HTTP_OTA_CHECK_INTERVAL  21600000UL  // ms (6 hours)
#define HTTP_OTA_FAST_INTERVAL   60000UL     // ms (60s — dev/testing mode)
#define HTTP_OTA_BOOT_DELAY      60000UL     // ms (60s after boot before first check)
#define HTTP_OTA_URL_BASE        "https://github.com/briangunderson/ESP32Smoker/releases/latest/download"

// GitHub Personal Access Token for private repo OTA (set in secrets.h)
#ifndef GITHUB_PAT
  #define GITHUB_PAT ""  // Empty = no auth (works for public repos)
#endif

// ============================================================================
// FIRMWARE METADATA
// ============================================================================

#define FIRMWARE_VERSION  "1.5.3"
#define FIRMWARE_BUILD    __DATE__ " " __TIME__

#endif // CONFIG_H
