#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "temperature_control.h"

class MQTTClient {
public:
  // Constructor
  MQTTClient(TemperatureController* controller, const char* brokerHost,
             uint16_t brokerPort = 1883);

  // Initialize and connect to MQTT broker
  bool begin(const char* clientId = MQTT_CLIENT_ID);

  // Disconnect from broker
  void disconnect();

  // Check connection status
  bool isConnected(void);

  // Publish current status to MQTT
  void publishStatus(void);

  // Main loop - handles connection and subscriptions
  void update();

  // Reconnect to broker if disconnected
  bool reconnect();

private:
  WiFiClient _wifiClient;
  PubSubClient _mqttClient;
  TemperatureController* _controller;

  const char* _brokerHost;
  uint16_t _brokerPort;
  const char* _clientId;
  const char* _rootTopic;

  uint32_t _lastPublish;
  uint32_t _lastTelemetry;
  bool _subscribed;
  bool _discoveryPublished;
  unsigned long _subscribeTime;  // millis() when subscribed — ignore retained msgs briefly

  // Static instance for callback routing
  static MQTTClient* _instance;

  // Topic management
  void subscribe();
  void setupTopics();

  // MQTT message handlers
  static void staticCallback(char* topic, byte* payload, unsigned int length);
  void handleMessage(char* topic, byte* payload, unsigned int length);

  // Home Assistant MQTT Discovery
  void publishDiscovery();
  void publishDiscoveryEntity(const char* component, const char* objectId,
                              const char* payload);

  // Extended telemetry
  void publishTelemetry();
};

#endif // MQTT_CLIENT_H
