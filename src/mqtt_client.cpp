#include "mqtt_client.h"
#include "config.h"

MQTTClient::MQTTClient(TemperatureController* controller,
                       const char* brokerHost, uint16_t brokerPort)
    : _mqttClient(_wifiClient), _controller(controller),
      _brokerHost(brokerHost), _brokerPort(brokerPort),
      _clientId(MQTT_CLIENT_ID), _rootTopic(MQTT_ROOT_TOPIC),
      _lastPublish(0), _subscribed(false) {}

bool MQTTClient::begin(const char* clientId) {
  _clientId = clientId;
  _mqttClient.setServer(_brokerHost, _brokerPort);

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
  // Reconnect if necessary
  if (!_mqttClient.connected()) {
    unsigned long now = millis();
    static unsigned long lastReconnectAttempt = 0;

    if (now - lastReconnectAttempt > MQTT_RECONNECT_INTERVAL) {
      lastReconnectAttempt = now;
      reconnect();
    }
  } else {
    _mqttClient.loop();

    // Publish status periodically
    unsigned long now = millis();
    if (now - _lastPublish > MQTT_STATUS_INTERVAL) {
      _lastPublish = now;
      publishStatus();
    }
  }
}

bool MQTTClient::reconnect() {
  if (_mqttClient.connect(_clientId, MQTT_USERNAME, MQTT_PASSWORD)) {
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[MQTT] Connected as %s (authenticated)\n", _clientId);
    }

    // Publish birth message
    String birthTopic = String(_rootTopic) + "/status/online";
    _mqttClient.publish(birthTopic.c_str(), "true", true);

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

void MQTTClient::publishStatus(void) {
  if (!_mqttClient.connected())
    return;

  auto status = _controller->getStatus();

  // Publish temperature
  String tempTopic = String(_rootTopic) + "/sensor/temperature";
  char tempBuffer[10];
  snprintf(tempBuffer, sizeof(tempBuffer), "%.1f", status.currentTemp);
  _mqttClient.publish(tempTopic.c_str(), tempBuffer);

  // Publish setpoint
  String setpointTopic = String(_rootTopic) + "/sensor/setpoint";
  char setpointBuffer[10];
  snprintf(setpointBuffer, sizeof(setpointBuffer), "%.1f",
           status.setpoint);
  _mqttClient.publish(setpointTopic.c_str(), setpointBuffer);

  // Publish state
  String stateTopic = String(_rootTopic) + "/sensor/state";
  _mqttClient.publish(stateTopic.c_str(),
                      _controller->getStateName());

  // Publish relay states
  String atugerTopic = String(_rootTopic) + "/sensor/auger";
  _mqttClient.publish(atugerTopic.c_str(), status.auger ? "on" : "off");

  String fanTopic = String(_rootTopic) + "/sensor/fan";
  _mqttClient.publish(fanTopic.c_str(), status.fan ? "on" : "off");

  String igniterTopic = String(_rootTopic) + "/sensor/igniter";
  _mqttClient.publish(igniterTopic.c_str(),
                      status.igniter ? "on" : "off");

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[MQTT] Published status - Temp: %.1f°F, State: %s\n",
                  status.currentTemp, _controller->getStateName());
  }
}

void MQTTClient::subscribe() {
  if (_subscribed)
    return;

  // Subscribe to control topics
  String startTopic = String(_rootTopic) + "/command/start";
  String stopTopic = String(_rootTopic) + "/command/stop";
  String setpointTopic = String(_rootTopic) + "/command/setpoint";

  _mqttClient.subscribe(startTopic.c_str());
  _mqttClient.subscribe(stopTopic.c_str());
  _mqttClient.subscribe(setpointTopic.c_str());

  _subscribed = true;

  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[MQTT] Subscribed to control topics");
  }
}

void MQTTClient::setupTopics() {
  // Topic structure already defined in config.h and subscribe()
}

void MQTTClient::messageCallback(char* topic, byte* payload,
                                 unsigned int length) {
  // This would be implemented to handle incoming MQTT messages
  // In a real implementation, this would be a non-static method or
  // use a callback pattern
}
