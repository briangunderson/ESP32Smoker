#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "max31865.h"
#include "relay_control.h"
#include "temperature_control.h"
#include "web_server.h"
#include "mqtt_client.h"
#include "tm1638_display.h"

// Global objects
MAX31865* tempSensor = nullptr;
RelayControl* relayControl = nullptr;
TemperatureController* controller = nullptr;
WebServer* webServer = nullptr;
MQTTClient* mqttClient = nullptr;
TM1638Display* display = nullptr;

// Network configuration
String wifiSSID = WIFI_SSID;
String wifiPassword = WIFI_PASS;
String mqttBroker = MQTT_BROKER_HOST;
uint16_t mqttPort = MQTT_BROKER_PORT;

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
// DISPLAY BUTTON HANDLING
// ============================================================================

void handleDisplayButtons() {
  if (!display || !controller) return;

  uint8_t buttons = display->readButtons();
  if (buttons == 0) return;  // No buttons pressed

  // Debounce - wait a bit between button presses
  static unsigned long lastButtonPress = 0;
  if (millis() - lastButtonPress < 300) return;
  lastButtonPress = millis();

  // Button 1: Start smoking
  if (display->isButtonPressed(BTN_START)) {
    Serial.println("[BTN] Start button pressed");
    controller->start();
  }

  // Button 2: Stop/Cooldown
  if (display->isButtonPressed(BTN_STOP)) {
    Serial.println("[BTN] Stop button pressed");
    controller->stop();
  }

  // Button 3: Increase setpoint (+5°F)
  if (display->isButtonPressed(BTN_TEMP_UP)) {
    float currentSetpoint = controller->getSetpoint();
    float newSetpoint = currentSetpoint + 5.0;
    if (newSetpoint <= TEMP_MAX_SETPOINT) {
      controller->setSetpoint(newSetpoint);
      Serial.printf("[BTN] Setpoint increased to %.0f°F\n", newSetpoint);
    }
  }

  // Button 4: Decrease setpoint (-5°F)
  if (display->isButtonPressed(BTN_TEMP_DOWN)) {
    float currentSetpoint = controller->getSetpoint();
    float newSetpoint = currentSetpoint - 5.0;
    if (newSetpoint >= TEMP_MIN_SETPOINT) {
      controller->setSetpoint(newSetpoint);
      Serial.printf("[BTN] Setpoint decreased to %.0f°F\n", newSetpoint);
    }
  }

  // Button 5: Cycle display mode (reserved for future use)
  if (display->isButtonPressed(BTN_MODE)) {
    Serial.println("[BTN] Mode button pressed (not yet implemented)");
  }

  // Buttons 6-8: Reserved for future use
  if (display->isButtonPressed(BTN_6)) {
    Serial.println("[BTN] Button 6 pressed (reserved)");
  }
  if (display->isButtonPressed(BTN_7)) {
    Serial.println("[BTN] Button 7 pressed (reserved)");
  }
  if (display->isButtonPressed(BTN_8)) {
    Serial.println("[BTN] Button 8 pressed (reserved)");
  }
}

// ============================================================================
// OTA FUNCTIONS
// ============================================================================

void initializeOTA() {
  // Set OTA hostname
  ArduinoOTA.setHostname("esp32-smoker");

  // Set OTA password for security
  ArduinoOTA.setPassword("smoker2026");

  // Configure OTA callbacks
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.printf("\n[OTA] Starting update: %s\n", type.c_str());

    // Stop critical operations during update
    if (controller) {
      controller->shutdown();
    }
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Update complete!");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("\n[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("[OTA] Over-The-Air updates enabled");
  Serial.printf("[OTA] Hostname: esp32-smoker, Password: smoker2026\n");
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

  // TM1638 Display
  display = new TM1638Display();
  display->begin();
  Serial.println("[SETUP] TM1638 display initialized");

  // Initialize WiFi
  initializeWiFi();

  // Initialize OTA updates
  initializeOTA();

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
  // Handle OTA updates
  ArduinoOTA.handle();

  // Update temperature control loop
  controller->update();

  // Update MQTT connection and publish
  mqttClient->update();

  // Check WiFi connection
  checkWiFiConnection();

  // Update TM1638 display
  if (display) {
    auto status = controller->getStatus();

    // Update temperature displays
    display->setCurrentTemp(status.currentTemp);
    display->setTargetTemp(status.setpoint);
    display->update();

    // Update relay status LEDs
    display->setRelayLEDs(status.auger, status.fan, status.igniter);

    // Update status LEDs
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    bool mqttConnected = mqttClient->isConnected();
    bool isError = (controller->getState() == STATE_ERROR);
    bool isRunning = (controller->getState() == STATE_RUNNING ||
                      controller->getState() == STATE_STARTUP);
    display->setStatusLEDs(wifiConnected, mqttConnected, isError, isRunning);

    // Handle button presses
    handleDisplayButtons();
  }

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
