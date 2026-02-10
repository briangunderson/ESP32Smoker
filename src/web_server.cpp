#include "web_server.h"
#include "config.h"
#include "web_content.h"

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
  // Serve web interface from PROGMEM (no filesystem needed)
  _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", web_index_html);
  });
  _server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", web_index_html);
  });
  _server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/css", web_style_css);
  });
  _server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "application/javascript", web_script_js);
  });

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

  // Debug API: Enable/disable debug mode
  _server.on("/api/debug/mode", HTTP_POST,
             [this](AsyncWebServerRequest* request) {
               if (request->hasParam("enabled", true)) {
                 bool enabled = request->getParam("enabled", true)->value() == "true";
                 _controller->setDebugMode(enabled);
                 request->send(200, "application/json", "{\"ok\":true}");
               } else {
                 request->send(400, "application/json",
                               "{\"error\":\"Missing enabled parameter\"}");
               }
             });

  // Debug API: Get debug mode status
  _server.on("/api/debug/status", HTTP_GET,
             [this](AsyncWebServerRequest* request) {
               StaticJsonDocument<128> doc;
               doc["debugMode"] = _controller->isDebugMode();
               String response;
               serializeJson(doc, response);
               request->send(200, "application/json", response);
             });

  // Debug API: Manual relay control
  _server.on("/api/debug/relay", HTTP_POST,
             [this](AsyncWebServerRequest* request) {
               if (request->hasParam("relay", true) &&
                   request->hasParam("state", true)) {
                 String relay = request->getParam("relay", true)->value();
                 bool state = request->getParam("state", true)->value() == "true";
                 _controller->setManualRelay(relay.c_str(), state);
                 request->send(200, "application/json", "{\"ok\":true}");
               } else {
                 request->send(400, "application/json",
                               "{\"error\":\"Missing relay or state parameter\"}");
               }
             });

  // Debug API: Set temperature override
  _server.on("/api/debug/temp", HTTP_POST,
             [this](AsyncWebServerRequest* request) {
               if (request->hasParam("temp", true)) {
                 float temp = request->getParam("temp", true)->value().toFloat();
                 _controller->setTempOverride(temp);
                 request->send(200, "application/json", "{\"ok\":true}");
               } else {
                 request->send(400, "application/json",
                               "{\"error\":\"Missing temp parameter\"}");
               }
             });

  // Debug API: Clear temperature override
  _server.on("/api/debug/temp", HTTP_DELETE,
             [this](AsyncWebServerRequest* request) {
               _controller->clearTempOverride();
               request->send(200, "application/json", "{\"ok\":true}");
             });

  // Debug API: Reset error state back to idle
  _server.on("/api/debug/reset", HTTP_POST,
             [this](AsyncWebServerRequest* request) {
               _controller->resetError();
               request->send(200, "application/json", "{\"ok\":true}");
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
