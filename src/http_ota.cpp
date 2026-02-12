#include "http_ota.h"
#include "config.h"
#include "logger.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

// Defined in main.cpp
extern void logMessage(uint16_t priority, const char* tag, const char* format, ...);

HttpOTA httpOTA;

HttpOTA::HttpOTA()
    : _currentVersion(FIRMWARE_VERSION),
      _latestVersion(""),
      _lastError(""),
      _updateAvailable(false),
      _updateRequested(false),
      _checkRequested(false),
      _checkComplete(false),
      _lastCheckResult(OTA_CHECK_NO_UPDATE),
      _lastCheckTime(0),
      _lastCheckMillis(0),
      _initialized(false),
      _fastCheck(false),
      _safetyCheck(nullptr) {}

void HttpOTA::begin() {
  if (!ENABLE_HTTP_OTA) return;
  _initialized = true;
  Serial.printf("[HTTP_OTA] Initialized - current version: %s\n", _currentVersion.c_str());
  Serial.printf("[HTTP_OTA] Check interval: %lu hours\n",
                (unsigned long)(HTTP_OTA_CHECK_INTERVAL / 3600000UL));
  Serial.printf("[HTTP_OTA] URL base: %s\n", HTTP_OTA_URL_BASE);
}

void HttpOTA::update() {
  if (!_initialized || !ENABLE_HTTP_OTA) return;
  if (WiFi.status() != WL_CONNECTED) return;

  // Handle deferred manual check (triggered by web API)
  if (_checkRequested) {
    _checkRequested = false;
    Serial.println("[HTTP_OTA] Manual check requested via web UI");
    _lastCheckResult = checkForUpdate();
    _checkComplete = true;
    _lastCheckMillis = millis();  // Reset periodic timer
    if (_lastCheckResult == OTA_CHECK_UPDATE_AVAILABLE) {
      if (_safetyCheck && _safetyCheck()) {
        Serial.println("[HTTP_OTA] Smoker is idle, proceeding with auto-update...");
        performUpdate();
      } else {
        Serial.println("[HTTP_OTA] Smoker is active, deferring update");
        logMessage(LOG_INFO, "HTTP_OTA", "Update deferred - smoker is active");
      }
    }
    return;
  }

  unsigned long now = millis();

  // First check after boot delay, then every check interval (fast or normal)
  unsigned long checkInterval = _fastCheck ? HTTP_OTA_FAST_INTERVAL : HTTP_OTA_CHECK_INTERVAL;
  unsigned long interval = (_lastCheckMillis == 0) ? HTTP_OTA_BOOT_DELAY : checkInterval;
  if (now - _lastCheckMillis < interval) return;
  _lastCheckMillis = now;

  HttpOtaResult result = checkForUpdate();

  if (result == OTA_CHECK_UPDATE_AVAILABLE) {
    Serial.printf("[HTTP_OTA] Update available: %s -> %s\n",
                  _currentVersion.c_str(), _latestVersion.c_str());
    logMessage(LOG_INFO, "HTTP_OTA", "Update available: %s -> %s",
               _currentVersion.c_str(), _latestVersion.c_str());

    // Auto-update only if safe
    if (_safetyCheck && _safetyCheck()) {
      Serial.println("[HTTP_OTA] Smoker is idle, proceeding with auto-update...");
      performUpdate();
    } else {
      Serial.println("[HTTP_OTA] Smoker is active, deferring update");
      logMessage(LOG_INFO, "HTTP_OTA", "Update deferred - smoker is active");
    }
  }
}

String HttpOTA::fetchRemoteVersion() {
  WiFiClientSecure client;
  client.setInsecure();  // Skip cert validation (URL is hardcoded to GitHub)

  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setTimeout(15000);  // 15 second timeout

  String url = String(HTTP_OTA_URL_BASE) + "/version.txt";
  Serial.printf("[HTTP_OTA] Checking: %s\n", url.c_str());

  if (!http.begin(client, url)) {
    _lastError = "HTTP begin failed";
    return "";
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    _lastError = "HTTP " + String(httpCode);
    http.end();
    return "";
  }

  String version = http.getString();
  version.trim();
  http.end();
  return version;
}

HttpOtaResult HttpOTA::checkForUpdate() {
  _lastCheckTime = millis() / 1000;

  String remoteVersion = fetchRemoteVersion();
  if (remoteVersion.length() == 0) {
    Serial.printf("[HTTP_OTA] Check failed: %s\n", _lastError.c_str());
    return OTA_CHECK_FAILED;
  }

  Serial.printf("[HTTP_OTA] Remote: %s, Local: %s\n",
                remoteVersion.c_str(), _currentVersion.c_str());

  if (isNewerVersion(remoteVersion, _currentVersion)) {
    _latestVersion = remoteVersion;
    _updateAvailable = true;
    return OTA_CHECK_UPDATE_AVAILABLE;
  }

  _updateAvailable = false;
  return OTA_CHECK_NO_UPDATE;
}

HttpOtaResult HttpOTA::performUpdate() {
  if (_safetyCheck && !_safetyCheck()) {
    _lastError = "Smoker is active";
    return OTA_UPDATE_SKIPPED;
  }

  WiFiClientSecure client;
  client.setInsecure();

  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(false);  // Reboot manually after logging

  String url = String(HTTP_OTA_URL_BASE) + "/firmware.bin";
  Serial.printf("[HTTP_OTA] Downloading firmware: %s\n", url.c_str());
  logMessage(LOG_INFO, "HTTP_OTA", "Starting update: %s -> %s",
             _currentVersion.c_str(), _latestVersion.c_str());

  t_httpUpdate_return ret = httpUpdate.update(client, url);

  switch (ret) {
    case HTTP_UPDATE_OK:
      Serial.println("[HTTP_OTA] Update successful! Rebooting...");
      logMessage(LOG_INFO, "HTTP_OTA", "Update successful, rebooting");
      delay(500);
      ESP.restart();
      return OTA_UPDATE_SUCCESS;

    case HTTP_UPDATE_FAILED:
      _lastError = httpUpdate.getLastErrorString();
      Serial.printf("[HTTP_OTA] Update failed: %s\n", _lastError.c_str());
      logMessage(LOG_ERR, "HTTP_OTA", "Update failed: %s", _lastError.c_str());
      return OTA_UPDATE_FAILED;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[HTTP_OTA] No update needed");
      return OTA_CHECK_NO_UPDATE;
  }

  return OTA_UPDATE_FAILED;
}

bool HttpOTA::isNewerVersion(const String& remote, const String& local) {
  int rMajor = 0, rMinor = 0, rPatch = 0;
  int lMajor = 0, lMinor = 0, lPatch = 0;

  sscanf(remote.c_str(), "%d.%d.%d", &rMajor, &rMinor, &rPatch);
  sscanf(local.c_str(), "%d.%d.%d", &lMajor, &lMinor, &lPatch);

  if (rMajor != lMajor) return rMajor > lMajor;
  if (rMinor != lMinor) return rMinor > lMinor;
  return rPatch > lPatch;
}
