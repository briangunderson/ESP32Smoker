#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "max31865.h"
#include "relay_control.h"
#include "temperature_control.h"
#include "web_server.h"
#include "mqtt_client.h"

// Global objects
MAX31865* tempSensor = nullptr;
RelayControl* relayControl = nullptr;
TemperatureController* controller = nullptr;
WebServer* webServer = nullptr;
MQTTClient* mqttClient = nullptr;

// Network configuration
String wifiSSID = "";
String wifiPassword = "";
String mqttBroker = "";
uint16_t mqttPort = 1883;

// Timing variables
unsigned long lastStatusPrint = 0;
unsigned long lastWiFiCheck = 0;

// ============================================================================
// WIFI FUNCTIONS
// ============================================================================

void initializeWiFi() {
  // Try to load saved WiFi credentials
  // For now, fall back to AP mode

  Serial.println("\n[WIFI] Starting WiFi...");

  if (wifiSSID.length() > 0 && wifiPassword.length() > 0) {
    // Connect to saved network
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

    Serial.printf("[WIFI] Connecting to %s...\n", wifiSSID.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("\n[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("\n[WIFI] Connection failed, starting AP mode");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
      Serial.printf("[WIFI] AP Mode - SSID: %s, IP: %s\n", WIFI_AP_SSID,
                    WiFi.softAPIP().toString().c_str());
    }
  } else {
    // Start in AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    Serial.printf("[WIFI] AP Mode - SSID: %s, Pass: %s, IP: %s\n",
                  WIFI_AP_SSID, WIFI_AP_PASS,
                  WiFi.softAPIP().toString().c_str());
  }
}

void checkWiFiConnection() {
  static unsigned long lastCheck = 0;
  unsigned long now = millis();

  if (now - lastCheck < 10000)
    return; // Check every 10 seconds

  lastCheck = now;

  if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Connection lost, reconnecting...");
    WiFi.reconnect();
  }
}

// ============================================================================
// FILESYSTEM FUNCTIONS
// ============================================================================

void initializeFileSystem() {
  if (!LittleFS.begin()) {
    Serial.println("[FS] Failed to initialize file system");
    // Create default www directory structure
  } else {
    Serial.println("[FS] File system initialized");
  }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Initialize Serial
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);

  Serial.println("\n\n========================================");
  Serial.println("  ESP32 Wood Pellet Smoker Controller");
  Serial.printf("  Version: %s\n", FIRMWARE_VERSION);
  Serial.printf("  Build: %s\n", FIRMWARE_BUILD);
  Serial.println("========================================\n");

  // Initialize file system
  initializeFileSystem();

  // Initialize hardware
  Serial.println("[SETUP] Initializing hardware...");

  // Temperature Sensor (MAX31865)
  tempSensor = new MAX31865(PIN_MAX31865_CS, MAX31865_REFERENCE_RESISTANCE,
                            MAX31865_RTD_RESISTANCE_AT_0);
  if (!tempSensor->begin(MAX31865::THREE_WIRE)) {
    Serial.println("[SETUP] WARNING: MAX31865 initialization failed!");
  } else {
    Serial.println("[SETUP] MAX31865 sensor initialized");
  }

  // Relay Control
  relayControl = new RelayControl();
  relayControl->begin();
  Serial.println("[SETUP] Relay control initialized");

  // Temperature Controller
  controller = new TemperatureController(tempSensor, relayControl);
  controller->begin();
  Serial.println("[SETUP] Temperature controller initialized");

  // Initialize WiFi
  initializeWiFi();

  // Web Server
  webServer = new WebServer(controller, WEB_SERVER_PORT);
  webServer->begin();
  Serial.printf("[SETUP] Web server started on port %d\n", WEB_SERVER_PORT);

  // MQTT Client
  mqttClient = new MQTTClient(controller, MQTT_BROKER_HOST, MQTT_BROKER_PORT);
  mqttClient->begin(MQTT_CLIENT_ID);
  Serial.printf("[SETUP] MQTT client initialized\n");

  // Status LED
  pinMode(PIN_LED_STATUS, OUTPUT);
  digitalWrite(PIN_LED_STATUS, LOW);

  Serial.println("\n[SETUP] Initialization complete!\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Update temperature control loop
  controller->update();

  // Update MQTT connection and publish
  mqttClient->update();

  // Check WiFi connection
  checkWiFiConnection();

  // Status blink LED
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
  }

  // Periodic status print (debugging)
  if (ENABLE_SERIAL_DEBUG) {
    if (millis() - lastStatusPrint > 10000) {
      lastStatusPrint = millis();

      auto status = controller->getStatus();
      Serial.printf(
          "[STATUS] Temp: %.1f°F | Setpoint: %.1f°F | State: %s | "
          "Auger: %s | Fan: %s | MQTT: %s\n",
          status.currentTemp, status.setpoint,
          controller->getStateName(), status.auger ? "ON" : "OFF",
          status.fan ? "ON" : "OFF",
          mqttClient->isConnected() ? "Connected" : "Offline");
    }
  }

  // Small delay to prevent watchdog timeout
  delay(10);
}
