#ifndef HTTP_OTA_H
#define HTTP_OTA_H

#include <Arduino.h>

// Update check result
enum HttpOtaResult {
  OTA_CHECK_NO_UPDATE,
  OTA_CHECK_UPDATE_AVAILABLE,
  OTA_CHECK_FAILED,
  OTA_UPDATE_SUCCESS,
  OTA_UPDATE_FAILED,
  OTA_UPDATE_SKIPPED
};

class HttpOTA {
public:
  HttpOTA();

  void begin();
  void update();  // Call from loop() — handles periodic checks

  HttpOtaResult checkForUpdate();
  HttpOtaResult performUpdate();

  // Deferred update (set by web API, executed in loop)
  void requestUpdate() { _updateRequested = true; }
  bool isUpdateRequested() const { return _updateRequested; }
  void clearUpdateRequest() { _updateRequested = false; }

  // Deferred check (set by web API, executed in loop — avoids blocking HTTPS in async handler)
  void requestCheck() { _checkRequested = true; _checkComplete = false; }
  bool isCheckComplete() const { return _checkComplete; }
  HttpOtaResult getLastCheckResult() const { return _lastCheckResult; }

  // Getters for web UI / API
  bool isUpdateAvailable() const { return _updateAvailable; }
  const String& getLatestVersion() const { return _latestVersion; }
  const String& getCurrentVersion() const { return _currentVersion; }
  unsigned long getLastCheckTime() const { return _lastCheckTime; }
  const String& getLastError() const { return _lastError; }

  // Fast check mode (60s interval for dev/testing)
  void setFastCheck(bool enabled) { _fastCheck = enabled; _lastCheckMillis = 0; }
  bool isFastCheck() const { return _fastCheck; }

  // Safety check callback — returns true if safe to update
  typedef bool (*SafetyCheckFn)();
  void setSafetyCheck(SafetyCheckFn fn) { _safetyCheck = fn; }

private:
  String _currentVersion;
  String _latestVersion;
  String _lastError;
  bool _updateAvailable;
  bool _updateRequested;
  bool _checkRequested;
  bool _checkComplete;
  HttpOtaResult _lastCheckResult;
  unsigned long _lastCheckTime;     // uptime in seconds when last checked
  unsigned long _lastCheckMillis;   // millis() at last check
  bool _initialized;
  bool _fastCheck;
  SafetyCheckFn _safetyCheck;

  bool isNewerVersion(const String& remote, const String& local);
  String fetchRemoteVersion();
};

extern HttpOTA httpOTA;

#endif // HTTP_OTA_H
