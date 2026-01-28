#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "temperature_control.h"

class WebServer {
public:
  // Constructor
  WebServer(TemperatureController* controller, uint16_t port = 80);

  // Initialize web server
  void begin();

  // Stop web server
  void stop();

  // Notifies all connected clients of status update
  void notifyClients(const String& message);

  // Check if server is running
  bool isRunning(void);

private:
  AsyncWebServer _server;
  TemperatureController* _controller;
  uint16_t _port;
  bool _running;

  // Route handlers
  void setupRoutes();
  void setupWebSocket();

  // API Endpoints (handlers defined in web_server.cpp)
  friend class AsyncCallbackWebHandler;
};

#endif // WEB_SERVER_H
