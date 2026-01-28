#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// PIN CONFIGURATION
// ============================================================================

// SPI Pins for MAX31865 (RTD Temperature Sensor)
#define PIN_SPI_MOSI    23  // GPIO 23
#define PIN_SPI_MISO    19  // GPIO 19
#define PIN_SPI_CLK     18  // GPIO 18
#define PIN_MAX31865_CS 5   // GPIO 5 (Chip Select)

// Relay Control Pins
#define PIN_RELAY_AUGER   12  // GPIO 12 - Pellet Auger Motor
#define PIN_RELAY_FAN     13  // GPIO 13 - Combustion Fan
#define PIN_RELAY_IGNITER 14  // GPIO 14 - Hot Rod Igniter

// Optional: LED Status Indicators
#define PIN_LED_STATUS    2   // GPIO 2 - Built-in LED

// ============================================================================
// SENSOR CONFIGURATION
// ============================================================================

// MAX31865 RTD Configuration
#define MAX31865_REFERENCE_RESISTANCE 430.0  // 430 ohms for PT100
#define MAX31865_RTD_RESISTANCE_AT_0  100.0  // PT100 = 100 ohms at 0°C
#define MAX31865_WIRE_MODE            3      // 3-wire RTD (most common)

// Temperature Sensor Calibration
#define TEMP_SENSOR_OFFSET 0.0  // °F offset calibration
#define TEMP_SAMPLE_COUNT  5    // Number of samples to average
#define TEMP_SAMPLE_DELAY  100  // ms between samples

// ============================================================================
// CONTROL CONFIGURATION
// ============================================================================

// Temperature Control Parameters (Hysteresis)
#define TEMP_CONTROL_INTERVAL    2000  // ms between control updates
#define TEMP_HYSTERESIS_BAND     10    // ±5°F band (±10°F total)
#define TEMP_MIN_SETPOINT        150   // Minimum allowed setpoint (°F)
#define TEMP_MAX_SETPOINT        350   // Maximum allowed setpoint (°F)

// Startup/Shutdown Behavior
#define IGNITER_PREHEAT_TIME     60000 // ms to preheat igniter before starting
#define FAN_STARTUP_DELAY        5000  // ms to delay fan after igniter
#define STARTUP_TIMEOUT          180000 // ms before startup failure
#define SHUTDOWN_COOL_TIMEOUT    300000 // ms to wait before shutdown complete

// Safety Limits
#define TEMP_MAX_SAFE            500   // Maximum safe temperature (°F)
#define TEMP_MIN_SAFE            50    // Minimum safe temperature (°F)
#define SENSOR_ERROR_THRESHOLD   3     // Max consecutive sensor errors before shutdown

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

// Wi-Fi Settings (configure in web interface for production)
#define WIFI_SSID    ""  // Leave empty to use AP mode or configure via web UI
#define WIFI_PASS    ""

// Wi-Fi AP Mode (fallback if no SSID configured)
#define WIFI_AP_SSID "ESP32Smoker"
#define WIFI_AP_PASS "your-ap-password"

// Web Server Port
#define WEB_SERVER_PORT 80

// ============================================================================
// MQTT CONFIGURATION
// ============================================================================

#define MQTT_BROKER_HOST    "192.168.1.100"  // Configure via web UI
#define MQTT_BROKER_PORT    1883
#define MQTT_CLIENT_ID      "esp32-smoker"
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

// ============================================================================
// FIRMWARE METADATA
// ============================================================================

#define FIRMWARE_VERSION  "1.0.0"
#define FIRMWARE_BUILD    __DATE__ " " __TIME__

#endif // CONFIG_H
