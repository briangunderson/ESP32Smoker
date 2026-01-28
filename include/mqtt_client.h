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
  bool _subscribed;

  // Topic management
  void subscribe();
  void setupTopics();

  // MQTT message handlers
  static void messageCallback(char* topic, byte* payload,
                              unsigned int length);
};

#endif // MQTT_CLIENT_H
