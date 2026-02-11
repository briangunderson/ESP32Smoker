#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <FFat.h>
#include <ArduinoJson.h>
#include "config.h"
#include "max31865.h"
#include "relay_control.h"
#include "temperature_control.h"
#include "web_server.h"
#include "mqtt_client.h"
#include "tm1638_display.h"
#include "logger.h"
#include "telnet_server.h"
#include "tui_server.h"
#include "encoder.h"

// Global objects
MAX31865* tempSensor = nullptr;
RelayControl* relayControl = nullptr;
TemperatureController* controller = nullptr;
WebServer* webServer = nullptr;
MQTTClient* mqttClient = nullptr;
TM1638Display* display = nullptr;
TUIServer* tuiServer = nullptr;
Encoder* encoder = nullptr;

// Helper function to log to both Serial and Syslog
void logMessage(uint16_t priority, const char* tag, const char* format, ...) {
  char buffer[LOG_BUFFER_SIZE];
  va_list args;

  // Format the message
  va_start(args, format);
  vsnprintf(buffer, LOG_BUFFER_SIZE, format, args);
  va_end(args);

  // Print to Serial
  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[%s] %s\n", tag, buffer);
  }

  // Send to Syslog
  char syslogMsg[LOG_BUFFER_SIZE];
  snprintf(syslogMsg, LOG_BUFFER_SIZE, "[%s] %s", tag, buffer);
  logger.log(priority, syslogMsg);
}

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
  if (!FFat.begin(true)) {  // true = format on fail
    Serial.println("[FS] Failed to initialize FFat file system");
  } else {
    Serial.printf("[FS] FFat initialized - %u KB used / %u KB total\n",
                  FFat.usedBytes() / 1024, FFat.totalBytes() / 1024);
    // List files for debugging
    File root = FFat.open("/");
    File f = root.openNextFile();
    while (f) {
      Serial.printf("[FS]   %s (%d bytes)\n", f.path(), f.size());
      f = root.openNextFile();
    }
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
    controller->startSmoking(controller->getSetpoint());
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
// ENCODER HANDLING
// ============================================================================

void handleEncoder() {
  if (!encoder || !controller) return;
  if (!encoder->isConnected()) return;

  if (!encoder->update()) return;

  // Handle rotation: adjust setpoint
  int8_t clicks = encoder->getIncrement();
  if (clicks != 0) {
    float currentSetpoint = controller->getSetpoint();
    float newSetpoint = currentSetpoint + (clicks * ENCODER_STEP_DEGREES);

    if (newSetpoint > TEMP_MAX_SETPOINT) newSetpoint = TEMP_MAX_SETPOINT;
    if (newSetpoint < TEMP_MIN_SETPOINT) newSetpoint = TEMP_MIN_SETPOINT;

    if (newSetpoint != currentSetpoint) {
      controller->setSetpoint(newSetpoint);
      Serial.printf("[ENCODER] Setpoint changed to %.0f°F (%+d clicks)\n",
                    newSetpoint, clicks);
    }
  }

  // Handle button press: toggle start/stop
  if (encoder->wasButtonPressed()) {
    ControllerState state = controller->getState();
    if (state == STATE_IDLE || state == STATE_SHUTDOWN) {
      controller->startSmoking(controller->getSetpoint());
      Serial.println("[ENCODER] Button: Starting smoker");
    } else if (state == STATE_RUNNING || state == STATE_STARTUP) {
      controller->stop();
      Serial.println("[ENCODER] Button: Stopping smoker");
    }
  }

  // Update LED color on state change
  static ControllerState lastLedState = STATE_IDLE;
  ControllerState currentState = controller->getState();
  if (currentState != lastLedState) {
    lastLedState = currentState;
    switch (currentState) {
      case STATE_IDLE:     encoder->setLEDColor(0, 20, 0);    break;
      case STATE_STARTUP:  encoder->setLEDColor(40, 20, 0);   break;
      case STATE_RUNNING:  encoder->setLEDColor(0, 60, 0);    break;
      case STATE_COOLDOWN: encoder->setLEDColor(0, 0, 40);    break;
      case STATE_SHUTDOWN: encoder->setLEDColor(10, 10, 10);  break;
      case STATE_ERROR:    encoder->setLEDColor(60, 0, 0);    break;
    }
  }
}

// ============================================================================
// OTA FUNCTIONS
// ============================================================================

void initializeOTA() {
  // Set OTA hostname
  ArduinoOTA.setHostname("esp32-smoker");

  // Set OTA password for security (defined in secrets.h)
  ArduinoOTA.setPassword(OTA_PASSWORD);

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
  Serial.println("[OTA] Over-The-Air updates configured");
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Initialize Serial
  Serial.begin(SERIAL_BAUD_RATE);
  // ESP32-S3 native USB CDC: prevent indefinite blocking on full TX buffer.
  // 100ms is safe (WDT is 5s) and avoids silent data loss from timeout=0.
  Serial.setTxTimeoutMs(100);
  // Wait for USB CDC connection (up to 5 seconds) so boot messages are captured
  unsigned long serialWait = millis();
  while (!Serial && millis() - serialWait < 5000) delay(10);
  delay(500); // Extra settle time after USB enumerates

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

  // Rotary Encoder (M5Stack Unit Encoder U135)
  encoder = new Encoder();
  if (!encoder->begin()) {
    Serial.println("[SETUP] WARNING: Encoder not found on I2C bus");
  }

  // Initialize WiFi
  initializeWiFi();

  // Initialize Syslog (after WiFi)
  logger.begin();

  // Initialize Telnet Server (after WiFi)
  telnetServer.begin();

  // Initialize TUI Server (after WiFi)
  if (ENABLE_TUI) {
    tuiServer = new TUIServer(controller, tempSensor);
    tuiServer->begin(TUI_PORT);
    Serial.printf("[SETUP] TUI server started on port %d\n", TUI_PORT);
  }

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
  logMessage(LOG_INFO, "SETUP", "ESP32 Smoker Controller v%s initialized successfully", FIRMWARE_VERSION);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Run MAX31865 hardware diagnostic once, 10 seconds after boot (USB CDC is connected by then)
  static bool diagRan = false;
  if (!diagRan && millis() > 10000 && tempSensor) {
    diagRan = true;
    Serial.println("\n*** RUNNING DEFERRED MAX31865 HARDWARE DIAGNOSTIC ***");
    tempSensor->runHardwareDiagnostic();
    // Re-initialize sensor for normal operation after diagnostic
    Serial.println("*** RE-INITIALIZING MAX31865 FOR NORMAL OPERATION ***");
    tempSensor->begin(MAX31865::THREE_WIRE);
  }

  // Handle OTA updates
  ArduinoOTA.handle();

  // Handle telnet server
  telnetServer.loop();

  // Handle TUI server
  if (tuiServer) {
    tuiServer->update();
  }

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

    // Heartbeat LED (blink LED 8 every second)
    static unsigned long lastBlink = 0;
    static bool heartbeatState = false;
    if (millis() - lastBlink > 1000) {
      lastBlink = millis();
      heartbeatState = !heartbeatState;
      display->setLED(LED_8, heartbeatState);
    }

    // Handle button presses
    handleDisplayButtons();
  }

  // Handle rotary encoder input
  handleEncoder();

  // Built-in LED heartbeat
  static unsigned long lastBuiltinBlink = 0;
  static bool builtinState = false;
  if (millis() - lastBuiltinBlink > 1000) {
    lastBuiltinBlink = millis();
    builtinState = !builtinState;
    digitalWrite(PIN_LED_STATUS, builtinState);
  }

  // Periodic status print (debugging)
  if (ENABLE_SERIAL_DEBUG) {
    if (millis() - lastStatusPrint > 10000) {
      lastStatusPrint = millis();

      auto status = controller->getStatus();
      Serial.printf(
          "[STATUS] Temp: %.1f°F | Setpoint: %.1f°F | State: %s | "
          "Auger: %s | Fan: %s | MQTT: %s | Heap: %u/%u\n",
          status.currentTemp, status.setpoint,
          controller->getStateName(), status.auger ? "ON" : "OFF",
          status.fan ? "ON" : "OFF",
          mqttClient->isConnected() ? "Connected" : "Offline",
          ESP.getFreeHeap(), ESP.getMinFreeHeap());

      // Also send to syslog
      logMessage(LOG_INFO, "STATUS",
                 "Temp: %.1f°F | Setpoint: %.1f°F | State: %s | Auger: %s | Fan: %s",
                 status.currentTemp, status.setpoint,
                 controller->getStateName(),
                 status.auger ? "ON" : "OFF",
                 status.fan ? "ON" : "OFF");
    }
  }

  // Small delay to prevent watchdog timeout
  delay(10);
}
