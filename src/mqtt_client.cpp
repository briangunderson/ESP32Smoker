#include "mqtt_client.h"
#include "config.h"
#include <ArduinoJson.h>

// Static instance for PubSubClient callback routing
MQTTClient* MQTTClient::_instance = nullptr;

// ============================================================================
// Device info shared across all discovery messages (JSON fragment)
// ============================================================================
static const char DEVICE_JSON_FMT[] PROGMEM =
    "\"dev\":{\"ids\":[\"gundergrill\"],\"name\":\"GunderGrill\","
    "\"mf\":\"GunderGrill\",\"mdl\":\"ESP32-S3 Pellet Smoker\","
    "\"sw\":\"%s\",\"cu\":\"http://esp32-smoker.local\"}";

MQTTClient::MQTTClient(TemperatureController* controller,
                       const char* brokerHost, uint16_t brokerPort)
    : _mqttClient(_wifiClient), _controller(controller),
      _brokerHost(brokerHost), _brokerPort(brokerPort),
      _clientId(MQTT_CLIENT_ID), _rootTopic(MQTT_ROOT_TOPIC),
      _lastPublish(0), _lastTelemetry(0),
      _subscribed(false), _discoveryPublished(false), _subscribeTime(0) {}

// ============================================================================
// LIFECYCLE
// ============================================================================

bool MQTTClient::begin(const char* clientId) {
  _clientId = clientId;
  _instance = this;

  _mqttClient.setServer(_brokerHost, _brokerPort);
  _mqttClient.setBufferSize(1024);  // Required for HA discovery payloads
  _mqttClient.setCallback(staticCallback);

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf(
        "[MQTT] Initialized - Broker: %s:%d, Root topic: %s\n", _brokerHost,
        _brokerPort, _rootTopic);
  }

  return reconnect();
}

void MQTTClient::disconnect() {
  _mqttClient.disconnect();
  _subscribed = false;

  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[MQTT] Disconnected");
  }
}

bool MQTTClient::isConnected(void) {
  return _mqttClient.connected();
}

void MQTTClient::update() {
  if (!_mqttClient.connected()) {
    unsigned long now = millis();
    static unsigned long lastReconnectAttempt = 0;

    if (now - lastReconnectAttempt > MQTT_RECONNECT_INTERVAL) {
      lastReconnectAttempt = now;
      reconnect();
    }
  } else {
    _mqttClient.loop();

    unsigned long now = millis();

    // Publish core status periodically
    if (now - _lastPublish > MQTT_STATUS_INTERVAL) {
      _lastPublish = now;
      publishStatus();
    }

    // Publish extended telemetry less frequently
    if (now - _lastTelemetry > MQTT_TELEMETRY_INTERVAL) {
      _lastTelemetry = now;
      publishTelemetry();
    }
  }
}

// ============================================================================
// CONNECTION
// ============================================================================

bool MQTTClient::reconnect() {
  // Configure LWT (Last Will & Testament) — broker publishes this if we disconnect
  String availTopic = String(_rootTopic) + "/status/online";

  if (_mqttClient.connect(_clientId, MQTT_USERNAME, MQTT_PASSWORD,
                          availTopic.c_str(), 1, true, "false")) {
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[MQTT] Connected as %s (authenticated)\n", _clientId);
    }

    // Publish birth message (retained)
    _mqttClient.publish(availTopic.c_str(), "true", true);

    // Publish HA MQTT Discovery (retained, only needs to happen once per boot)
    if (!_discoveryPublished) {
      publishDiscovery();
      _discoveryPublished = true;
    }

    subscribe();
    return true;
  } else {
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[MQTT] Connection failed, rc=%d\n",
                    _mqttClient.state());
    }
    return false;
  }
}

// ============================================================================
// SUBSCRIPTIONS & COMMAND HANDLING
// ============================================================================

void MQTTClient::subscribe() {
  if (_subscribed)
    return;

  String startTopic = String(_rootTopic) + "/command/start";
  String stopTopic = String(_rootTopic) + "/command/stop";
  String setpointTopic = String(_rootTopic) + "/command/setpoint";
  String emergencyStopTopic = String(_rootTopic) + "/command/emergency_stop";

  _mqttClient.subscribe(startTopic.c_str());
  _mqttClient.subscribe(stopTopic.c_str());
  _mqttClient.subscribe(setpointTopic.c_str());
  _mqttClient.subscribe(emergencyStopTopic.c_str());

  _subscribed = true;
  _subscribeTime = millis();

  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[MQTT] Subscribed to control topics");
  }
}

void MQTTClient::setupTopics() {
  // Topic structure defined in config.h and subscribe()
}

void MQTTClient::staticCallback(char* topic, byte* payload,
                                unsigned int length) {
  if (_instance) {
    _instance->handleMessage(topic, payload, length);
  }
}

void MQTTClient::handleMessage(char* topic, byte* payload,
                               unsigned int length) {
  // Null-terminate the payload
  char message[64];
  unsigned int copyLen = (length < sizeof(message) - 1) ? length : sizeof(message) - 1;
  memcpy(message, payload, copyLen);
  message[copyLen] = '\0';

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[MQTT] Received: %s → %s\n", topic, message);
  }

  String topicStr(topic);
  String prefix = String(_rootTopic) + "/command/";

  if (!topicStr.startsWith(prefix)) return;

  // Ignore retained messages delivered immediately after subscribing
  if (_subscribeTime > 0 && millis() - _subscribeTime < 2000) {
    Serial.printf("[MQTT] Ignoring retained message during subscribe grace period: %s\n", topic);
    return;
  }

  String command = topicStr.substring(prefix.length());

  if (command == "start") {
    float temp = 225.0;  // Default
    if (copyLen > 0) {
      float parsed = atof(message);
      if (parsed >= TEMP_MIN_SETPOINT && parsed <= TEMP_MAX_SETPOINT) {
        temp = parsed;
      }
    }
    _controller->startSmoking(temp);
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[MQTT] Command: START at %.0f°F\n", temp);
    }
  } else if (command == "stop") {
    _controller->stop();
    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[MQTT] Command: END_COOK");
    }
  } else if (command == "emergency_stop") {
    _controller->shutdown();
    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[MQTT] Command: EMERGENCY_STOP");
    }
  } else if (command == "setpoint") {
    if (copyLen > 0) {
      float temp = atof(message);
      if (temp >= TEMP_MIN_SETPOINT && temp <= TEMP_MAX_SETPOINT) {
        _controller->setSetpoint(temp);
        if (ENABLE_SERIAL_DEBUG) {
          Serial.printf("[MQTT] Command: SETPOINT %.0f°F\n", temp);
        }
      }
    }
  }
}

// ============================================================================
// STATUS PUBLISHING (every 5 seconds)
// ============================================================================

void MQTTClient::publishStatus(void) {
  if (!_mqttClient.connected())
    return;

  auto status = _controller->getStatus();

  // Build topic prefix once to avoid repeated String allocations
  String prefix = String(_rootTopic) + "/sensor/";
  char buf[16];

  // Temperature
  snprintf(buf, sizeof(buf), "%.1f", status.currentTemp);
  _mqttClient.publish((prefix + "temperature").c_str(), buf);

  // Setpoint
  snprintf(buf, sizeof(buf), "%.1f", status.setpoint);
  _mqttClient.publish((prefix + "setpoint").c_str(), buf);

  // State
  _mqttClient.publish((prefix + "state").c_str(),
                      _controller->getStateName());

  // Flush TCP buffer so remaining publishes don't get dropped
  _mqttClient.loop();

  // Relay states
  _mqttClient.publish((prefix + "auger").c_str(),
                      status.auger ? "ON" : "OFF");
  _mqttClient.publish((prefix + "fan").c_str(),
                      status.fan ? "ON" : "OFF");
  _mqttClient.publish((prefix + "igniter").c_str(),
                      status.igniter ? "ON" : "OFF");

  // Flush again before PID batch
  _mqttClient.loop();

  // PID data (only meaningful in RUNNING state, but always publish for graphs)
  auto pid = _controller->getPIDStatus();
  snprintf(buf, sizeof(buf), "%.1f", pid.output * 100.0);
  _mqttClient.publish((prefix + "pid_output").c_str(), buf);
  snprintf(buf, sizeof(buf), "%.4f", pid.proportionalTerm);
  _mqttClient.publish((prefix + "pid_p").c_str(), buf);
  snprintf(buf, sizeof(buf), "%.4f", pid.integralTerm);
  _mqttClient.publish((prefix + "pid_i").c_str(), buf);
  snprintf(buf, sizeof(buf), "%.4f", pid.derivativeTerm);
  _mqttClient.publish((prefix + "pid_d").c_str(), buf);

  // Flush before lid/reignite batch
  _mqttClient.loop();

  // Lid-open and reignite status
  _mqttClient.publish((prefix + "lid_open").c_str(),
                      _controller->isLidOpen() ? "ON" : "OFF");
  snprintf(buf, sizeof(buf), "%d", _controller->getReigniteAttempts());
  _mqttClient.publish((prefix + "reignite_attempts").c_str(), buf);

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[MQTT] Published status - Temp: %.1f°F, State: %s\n",
                  status.currentTemp, _controller->getStateName());
  }
}

// ============================================================================
// EXTENDED TELEMETRY (every 60 seconds)
// ============================================================================

void MQTTClient::publishTelemetry() {
  if (!_mqttClient.connected())
    return;

  char buf[16];

  // WiFi RSSI
  snprintf(buf, sizeof(buf), "%d", WiFi.RSSI());
  _mqttClient.publish((String(_rootTopic) + "/sensor/wifi_rssi").c_str(), buf);

  // Uptime in seconds
  snprintf(buf, sizeof(buf), "%lu", millis() / 1000UL);
  _mqttClient.publish((String(_rootTopic) + "/sensor/uptime").c_str(), buf);

  // Free heap
  snprintf(buf, sizeof(buf), "%u", ESP.getFreeHeap());
  _mqttClient.publish((String(_rootTopic) + "/sensor/free_heap").c_str(), buf);
}

// ============================================================================
// HOME ASSISTANT MQTT DISCOVERY
// ============================================================================

void MQTTClient::publishDiscoveryEntity(const char* component,
                                        const char* objectId,
                                        const char* payload) {
  char topic[128];
  snprintf(topic, sizeof(topic), "homeassistant/%s/gundergrill/%s/config",
           component, objectId);
  _mqttClient.publish(topic, payload, true);  // retained

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[MQTT] Discovery: %s/%s\n", component, objectId);
  }
}

void MQTTClient::publishDiscovery() {
  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[MQTT] Publishing Home Assistant discovery...");
  }

  // Build the device JSON fragment once
  char device[256];
  snprintf(device, sizeof(device), DEVICE_JSON_FMT, FIRMWARE_VERSION);

  // Availability config shared by all entities
  const char* availFmt =
      "\"avty_t\":\"%s/status/online\","
      "\"pl_avail\":\"true\",\"pl_not_avail\":\"false\"";
  char avail[128];
  snprintf(avail, sizeof(avail), availFmt, _rootTopic);

  char payload[768];

  // --- SENSORS ---

  // Temperature
  snprintf(payload, sizeof(payload),
      "{\"name\":\"Temperature\","
      "\"stat_t\":\"%s/sensor/temperature\","
      "\"unit_of_meas\":\"°F\",\"dev_cla\":\"temperature\","
      "\"stat_cla\":\"measurement\","
      "\"uniq_id\":\"gundergrill_temperature\","
      "\"ic\":\"mdi:thermometer\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("sensor", "temperature", payload);

  // Setpoint (read-only sensor)
  snprintf(payload, sizeof(payload),
      "{\"name\":\"Setpoint\","
      "\"stat_t\":\"%s/sensor/setpoint\","
      "\"unit_of_meas\":\"°F\",\"dev_cla\":\"temperature\","
      "\"stat_cla\":\"measurement\","
      "\"uniq_id\":\"gundergrill_setpoint\","
      "\"ic\":\"mdi:thermometer-lines\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("sensor", "setpoint", payload);

  // State
  snprintf(payload, sizeof(payload),
      "{\"name\":\"State\","
      "\"stat_t\":\"%s/sensor/state\","
      "\"uniq_id\":\"gundergrill_state\","
      "\"ic\":\"mdi:state-machine\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("sensor", "state", payload);

  // Flush TCP buffer between discovery batches (PubSubClient drops messages
  // if too many large retained publishes happen without processing network I/O)
  _mqttClient.loop();

  // PID Output
  snprintf(payload, sizeof(payload),
      "{\"name\":\"PID Output\","
      "\"stat_t\":\"%s/sensor/pid_output\","
      "\"unit_of_meas\":\"%%\","
      "\"stat_cla\":\"measurement\","
      "\"uniq_id\":\"gundergrill_pid_output\","
      "\"ic\":\"mdi:gauge\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("sensor", "pid_output", payload);

  // PID Proportional
  snprintf(payload, sizeof(payload),
      "{\"name\":\"PID Proportional\","
      "\"stat_t\":\"%s/sensor/pid_p\","
      "\"stat_cla\":\"measurement\","
      "\"uniq_id\":\"gundergrill_pid_p\","
      "\"ic\":\"mdi:alpha-p-circle\","
      "\"ent_cat\":\"diagnostic\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("sensor", "pid_p", payload);

  // PID Integral
  snprintf(payload, sizeof(payload),
      "{\"name\":\"PID Integral\","
      "\"stat_t\":\"%s/sensor/pid_i\","
      "\"stat_cla\":\"measurement\","
      "\"uniq_id\":\"gundergrill_pid_i\","
      "\"ic\":\"mdi:alpha-i-circle\","
      "\"ent_cat\":\"diagnostic\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("sensor", "pid_i", payload);

  // PID Derivative
  snprintf(payload, sizeof(payload),
      "{\"name\":\"PID Derivative\","
      "\"stat_t\":\"%s/sensor/pid_d\","
      "\"stat_cla\":\"measurement\","
      "\"uniq_id\":\"gundergrill_pid_d\","
      "\"ic\":\"mdi:alpha-d-circle\","
      "\"ent_cat\":\"diagnostic\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("sensor", "pid_d", payload);

  _mqttClient.loop();

  // WiFi RSSI
  snprintf(payload, sizeof(payload),
      "{\"name\":\"WiFi Signal\","
      "\"stat_t\":\"%s/sensor/wifi_rssi\","
      "\"unit_of_meas\":\"dBm\","
      "\"dev_cla\":\"signal_strength\","
      "\"stat_cla\":\"measurement\","
      "\"uniq_id\":\"gundergrill_wifi_rssi\","
      "\"ent_cat\":\"diagnostic\","
      "\"ic\":\"mdi:wifi\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("sensor", "wifi_rssi", payload);

  // Uptime
  snprintf(payload, sizeof(payload),
      "{\"name\":\"Uptime\","
      "\"stat_t\":\"%s/sensor/uptime\","
      "\"unit_of_meas\":\"s\","
      "\"dev_cla\":\"duration\","
      "\"stat_cla\":\"total_increasing\","
      "\"uniq_id\":\"gundergrill_uptime\","
      "\"ent_cat\":\"diagnostic\","
      "\"ic\":\"mdi:clock-outline\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("sensor", "uptime", payload);

  // Free Heap
  snprintf(payload, sizeof(payload),
      "{\"name\":\"Free Memory\","
      "\"stat_t\":\"%s/sensor/free_heap\","
      "\"unit_of_meas\":\"B\","
      "\"stat_cla\":\"measurement\","
      "\"uniq_id\":\"gundergrill_free_heap\","
      "\"ent_cat\":\"diagnostic\","
      "\"ic\":\"mdi:memory\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("sensor", "free_heap", payload);

  _mqttClient.loop();

  // --- BINARY SENSORS ---

  // Auger
  snprintf(payload, sizeof(payload),
      "{\"name\":\"Auger\","
      "\"stat_t\":\"%s/sensor/auger\","
      "\"uniq_id\":\"gundergrill_auger\","
      "\"ic\":\"mdi:screw-lag\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("binary_sensor", "auger", payload);

  // Fan
  snprintf(payload, sizeof(payload),
      "{\"name\":\"Fan\","
      "\"stat_t\":\"%s/sensor/fan\","
      "\"uniq_id\":\"gundergrill_fan\","
      "\"ic\":\"mdi:fan\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("binary_sensor", "fan", payload);

  // Igniter
  snprintf(payload, sizeof(payload),
      "{\"name\":\"Igniter\","
      "\"stat_t\":\"%s/sensor/igniter\","
      "\"uniq_id\":\"gundergrill_igniter\","
      "\"ic\":\"mdi:fire\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("binary_sensor", "igniter", payload);

  // Lid Open
  snprintf(payload, sizeof(payload),
      "{\"name\":\"Lid Open\","
      "\"stat_t\":\"%s/sensor/lid_open\","
      "\"uniq_id\":\"gundergrill_lid_open\","
      "\"ic\":\"mdi:door-open\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("binary_sensor", "lid_open", payload);

  _mqttClient.loop();

  // Reignite Attempts
  snprintf(payload, sizeof(payload),
      "{\"name\":\"Reignite Attempts\","
      "\"stat_t\":\"%s/sensor/reignite_attempts\","
      "\"stat_cla\":\"measurement\","
      "\"uniq_id\":\"gundergrill_reignite_attempts\","
      "\"ic\":\"mdi:fire-alert\","
      "\"ent_cat\":\"diagnostic\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("sensor", "reignite_attempts", payload);

  _mqttClient.loop();

  // --- NUMBER (setpoint control) ---

  snprintf(payload, sizeof(payload),
      "{\"name\":\"Target Temperature\","
      "\"stat_t\":\"%s/sensor/setpoint\","
      "\"cmd_t\":\"%s/command/setpoint\","
      "\"min\":%d,\"max\":%d,\"step\":5,"
      "\"unit_of_meas\":\"°F\","
      "\"uniq_id\":\"gundergrill_setpoint_control\","
      "\"ic\":\"mdi:thermometer-lines\","
      "%s,%s}", _rootTopic, _rootTopic,
      TEMP_MIN_SETPOINT, TEMP_MAX_SETPOINT, device, avail);
  publishDiscoveryEntity("number", "setpoint", payload);

  // --- BUTTONS (end cook / emergency stop) ---

  snprintf(payload, sizeof(payload),
      "{\"name\":\"End Cook\","
      "\"cmd_t\":\"%s/command/stop\","
      "\"uniq_id\":\"gundergrill_stop\","
      "\"ic\":\"mdi:stop\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("button", "stop", payload);

  snprintf(payload, sizeof(payload),
      "{\"name\":\"Emergency Stop\","
      "\"cmd_t\":\"%s/command/emergency_stop\","
      "\"uniq_id\":\"gundergrill_emergency_stop\","
      "\"ic\":\"mdi:alert-octagon\","
      "%s,%s}", _rootTopic, device, avail);
  publishDiscoveryEntity("button", "emergency_stop", payload);

  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[MQTT] Home Assistant discovery published");
  }
}
