#include "web_server.h"
#include "config.h"
#include "web_content.h"
#include "http_ota.h"

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
    AsyncWebServerResponse* response = request->beginResponse_P(200, "text/css", web_style_css_gz, web_style_css_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  _server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "application/javascript", web_script_js_gz, web_script_js_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  // API: Get current status
  _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    auto status = _controller->getStatus();
    auto pid = _controller->getPIDStatus();

    StaticJsonDocument<512> doc;
    doc["temp"] = status.currentTemp;
    doc["setpoint"] = status.setpoint;
    doc["state"] = _controller->getStateName();
    doc["auger"] = status.auger;
    doc["fan"] = status.fan;
    doc["igniter"] = status.igniter;
    doc["runtime"] = status.runtime;
    doc["errors"] = status.errorCount;
    doc["version"] = FIRMWARE_VERSION;
    doc["heap"] = ESP.getFreeHeap();

    // PID data for web UI visualization
    JsonObject pidObj = doc.createNestedObject("pid");
    pidObj["p"] = serialized(String(pid.proportionalTerm, 4));
    pidObj["i"] = serialized(String(pid.integralTerm, 4));
    pidObj["d"] = serialized(String(pid.derivativeTerm, 4));
    pidObj["output"] = serialized(String(pid.output * 100.0, 1));
    pidObj["error"] = serialized(String(pid.error, 1));
    pidObj["cycleRemaining"] = pid.cycleTimeRemaining;
    pidObj["augerOn"] = pid.augerCycleState;
    pidObj["lidOpen"] = _controller->isLidOpen();
    pidObj["reigniteAttempts"] = _controller->getReigniteAttempts();

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
      Serial.println("[WEB] End Cook command received");
    }
  });

  // API: Shutdown
  _server.on("/api/shutdown", HTTP_POST,
             [this](AsyncWebServerRequest* request) {
               _controller->shutdown();
               request->send(200, "application/json", "{\"ok\":true}");

               if (ENABLE_SERIAL_DEBUG) {
                 Serial.println("[WEB] Emergency Stop command received");
               }
             });

  // API: Temperature history for graph
  // Compact format: arrays instead of objects, temps as int (°F×10)
  // Sample: [time, temp×10, setpoint×10, state]  Event: [time, state]
  _server.on("/api/history", HTTP_GET, [this](AsyncWebServerRequest* request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print("{\"now\":");
    response->print(_controller->getUptime());
    response->print(",\"samples\":[");
    uint16_t count = _controller->getHistoryCount();
    for (uint16_t i = 0; i < count; i++) {
      const HistorySample& s = _controller->getHistorySampleAt(i);
      if (i > 0) response->print(',');
      response->printf("[%u,%d,%d,%d]", s.time, s.temp, s.setpoint, s.state);
    }
    response->print("],\"events\":[");
    uint8_t ecount = _controller->getEventCount();
    for (uint8_t i = 0; i < ecount; i++) {
      const HistoryEvent& e = _controller->getHistoryEventAt(i);
      if (i > 0) response->print(',');
      response->printf("[%u,%d]", e.time, e.state);
    }
    response->print("]}");
    request->send(response);
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

  // Debug API: Raw sensor diagnostics
  _server.on("/api/debug/sensor", HTTP_GET,
             [this](AsyncWebServerRequest* request) {
               auto d = _controller->getSensor()->getDiagnostics();
               StaticJsonDocument<384> doc;
               doc["configReg"] = String("0x") + String(d.configReg, HEX);
               doc["rtdRaw"] = String("0x") + String(d.rtdRaw, HEX);
               doc["adcValue"] = d.adcValue;
               doc["faultStatus"] = String("0x") + String(d.faultStatus, HEX);
               doc["resistance"] = d.resistance;
               doc["tempC"] = d.tempC;
               doc["tempF"] = d.tempF;
               doc["refResistance"] = d.refResistance;
               doc["rtdNominal"] = d.rtdNominal;
               doc["faultBit"] = (bool)(d.rtdRaw & 0x01);
               JsonArray regs = doc.createNestedArray("registers");
               for (int r = 0; r < 8; r++) {
                 regs.add(String("0x") + String(d.registers[r], HEX));
               }
               String response;
               serializeJson(doc, response);
               request->send(200, "application/json", response);
             });

  // Debug API: Reset error state back to idle
  _server.on("/api/debug/reset", HTTP_POST,
             [this](AsyncWebServerRequest* request) {
               _controller->resetError();
               request->send(200, "application/json", "{\"ok\":true}");
             });

  // API: Firmware version and update status
  _server.on("/api/version", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<384> doc;
    doc["current"] = httpOTA.getCurrentVersion();
    doc["latest"] = httpOTA.getLatestVersion();
    doc["updateAvailable"] = httpOTA.isUpdateAvailable();
    doc["lastCheck"] = httpOTA.getLastCheckTime();
    doc["lastError"] = httpOTA.getLastError();
    doc["fastCheck"] = httpOTA.isFastCheck();
    doc["checkComplete"] = httpOTA.isCheckComplete();

    // Include last manual check result for polling
    if (httpOTA.isCheckComplete()) {
      HttpOtaResult r = httpOTA.getLastCheckResult();
      switch (r) {
        case OTA_CHECK_NO_UPDATE:        doc["checkResult"] = "no_update"; break;
        case OTA_CHECK_UPDATE_AVAILABLE: doc["checkResult"] = "update_available"; break;
        case OTA_CHECK_FAILED:           doc["checkResult"] = "failed"; break;
        default:                         doc["checkResult"] = "unknown"; break;
      }
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // API: Manually trigger firmware update check (deferred — executes in loop)
  _server.on("/api/update/check", HTTP_POST, [](AsyncWebServerRequest* request) {
    httpOTA.requestCheck();
    request->send(200, "application/json", "{\"result\":\"checking\"}");
  });

  // API: Toggle fast OTA check interval (60s for dev/testing)
  _server.on("/api/update/fast", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->hasParam("enabled", true)) {
      bool enabled = request->getParam("enabled", true)->value() == "true";
      httpOTA.setFastCheck(enabled);
      StaticJsonDocument<64> doc;
      doc["ok"] = true;
      doc["fastCheck"] = enabled;
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
    } else {
      request->send(400, "application/json",
                    "{\"error\":\"Missing enabled parameter\"}");
    }
  });

  // API: Apply firmware update (deferred — executes in loop)
  _server.on("/api/update/apply", HTTP_POST, [this](AsyncWebServerRequest* request) {
    if (!httpOTA.isUpdateAvailable()) {
      request->send(400, "application/json", "{\"error\":\"No update available\"}");
      return;
    }
    ControllerState state = _controller->getState();
    if (state != STATE_IDLE && state != STATE_SHUTDOWN && state != STATE_ERROR) {
      request->send(409, "application/json",
                    "{\"error\":\"Cannot update while smoker is active\"}");
      return;
    }
    httpOTA.requestUpdate();
    request->send(200, "application/json",
                  "{\"ok\":true,\"message\":\"Update starting, device will reboot\"}");
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
