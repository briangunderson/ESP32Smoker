#include "tm1638_display.h"

TM1638Display::TM1638Display()
    : _display(nullptr),
      _currentTemp(0.0),
      _targetTemp(0.0),
      _lastButtons(0),
      _ledState(0) {}

void TM1638Display::begin() {
  // Initialize TM1638 with DIO, CLK, STB pins
  _display = new TM1638(PIN_TM1638_DIO, PIN_TM1638_CLK, PIN_TM1638_STB);

  // Clear display and LEDs
  clear();

  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[TM1638] Display initialized");
  }
}

void TM1638Display::update() {
  if (!_display) return;

  // Format and display current temperature (left 4 digits)
  char leftBuffer[5];
  formatTemperature(_currentTemp, leftBuffer);

  // Format and display target temperature (right 4 digits)
  char rightBuffer[5];
  formatTemperature(_targetTemp, rightBuffer);

  // Combine into 8-character string for display
  char displayBuffer[9];
  snprintf(displayBuffer, 9, "%s%s", leftBuffer, rightBuffer);

  // Display the string
  _display->setDisplayToString(displayBuffer);
}

void TM1638Display::clear() {
  if (_display) {
    _display->clearDisplay();
    _ledState = 0;
    _display->setLEDs(_ledState);
  }
}

void TM1638Display::setCurrentTemp(float temp) {
  _currentTemp = temp;
}

void TM1638Display::setTargetTemp(float temp) {
  _targetTemp = temp;
}

void TM1638Display::formatTemperature(float temp, char* buffer) {
  // Format temperature to 4 characters
  if (temp >= 1000.0) {
    // Display as integer for 4 digits (e.g., "1107")
    snprintf(buffer, 5, "%4.0f", temp);
  } else if (temp >= 100.0) {
    // Display as integer (e.g., " 225")
    snprintf(buffer, 5, "%4.0f", temp);
  } else if (temp >= 10.0) {
    // Display with 1 decimal (e.g., " 99")
    snprintf(buffer, 5, "%4.0f", temp);
  } else {
    // Display with 1 decimal (e.g., "  0")
    snprintf(buffer, 5, "%4.0f", temp);
  }
}

void TM1638Display::setLED(uint8_t ledNum, bool state) {
  if (!_display || ledNum > 7) return;

  if (state) {
    _ledState |= (1 << ledNum);
  } else {
    _ledState &= ~(1 << ledNum);
  }

  _display->setLEDs(_ledState);
}

void TM1638Display::setRelayLEDs(bool auger, bool fan, bool igniter) {
  setLED(LED_AUGER, auger);
  setLED(LED_FAN, fan);
  setLED(LED_IGNITER, igniter);
}

void TM1638Display::setStatusLEDs(bool wifi, bool mqtt, bool error, bool running) {
  setLED(LED_WIFI, wifi);
  setLED(LED_MQTT, mqtt);
  setLED(LED_ERROR, error);
  setLED(LED_RUNNING, running);
}

uint8_t TM1638Display::readButtons() {
  if (!_display) return 0;

  _lastButtons = _display->getButtons();
  return _lastButtons;
}

bool TM1638Display::isButtonPressed(uint8_t button) {
  return (_lastButtons & button) != 0;
}
