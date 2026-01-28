#include "web_server.h"
#include "config.h"
#include <LittleFS.h>

WebServer::WebServer(TemperatureController* controller, uint16_t port)
    : _server(port), _controller(controller), _port(port), _running(false) {}

void WebServer::begin() {
  setupRoutes();
  setupWebSocket();

  // Start server
  _server.begin();
  _running = true;

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[WEB] Server started on port %d\n", _port);
  }
}

void WebServer::stop() {
  _server.end();
  _running = false;

  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[WEB] Server stopped");
  }
}

bool WebServer::isRunning(void) {
  return _running;
}

void WebServer::notifyClients(const String& message) {
  // WebSocket broadcast would go here
}

void WebServer::setupRoutes() {
  // Serve static files from LittleFS
  _server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");

  // API: Get current status
  _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    auto status = _controller->getStatus();

    StaticJsonDocument<256> doc;
    doc["temp"] = status.currentTemp;
    doc["setpoint"] = status.setpoint;
    doc["state"] = _controller->getStateName();
    doc["auger"] = status.auger;
    doc["fan"] = status.fan;
    doc["igniter"] = status.igniter;
    doc["runtime"] = status.runtime;
    doc["errors"] = status.errorCount;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // API: Set setpoint temperature
  _server.on("/api/setpoint", HTTP_POST,
             [this](AsyncWebServerRequest* request) {
               if (request->hasParam("temp", true)) {
                 float temp = request->getParam("temp", true)->value().toFloat();
                 _controller->setSetpoint(temp);
                 request->send(200, "application/json", "{\"ok\":true}");
               } else {
                 request->send(400, "application/json",
                               "{\"error\":\"Missing temp parameter\"}");
               }
             });

  // API: Start smoking
  _server.on("/api/start", HTTP_POST, [this](AsyncWebServerRequest* request) {
    float temp = 225.0; // Default setpoint
    if (request->hasParam("temp", true)) {
      temp = request->getParam("temp", true)->value().toFloat();
    }

    _controller->startSmoking(temp);
    request->send(200, "application/json", "{\"ok\":true}");

    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[WEB] Start command received - setpoint: %.1f°F\n", temp);
    }
  });

  // API: Stop smoking
  _server.on("/api/stop", HTTP_POST, [this](AsyncWebServerRequest* request) {
    _controller->stop();
    request->send(200, "application/json", "{\"ok\":true}");

    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[WEB] Stop command received");
    }
  });

  // API: Shutdown
  _server.on("/api/shutdown", HTTP_POST,
             [this](AsyncWebServerRequest* request) {
               _controller->shutdown();
               request->send(200, "application/json", "{\"ok\":true}");

               if (ENABLE_SERIAL_DEBUG) {
                 Serial.println("[WEB] Shutdown command received");
               }
             });

  // 404 handler
  _server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"error\":\"Not found\"}");
  });
}

void WebServer::setupWebSocket() {
  // WebSocket setup would go here for real-time updates
  // This is a simplified implementation
}
