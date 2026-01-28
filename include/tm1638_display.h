#ifndef TM1638_DISPLAY_H
#define TM1638_DISPLAY_H

#include <Arduino.h>
#include <TM1638.h>
#include "config.h"

// Button definitions
#define BTN_START       0x01  // Button 1
#define BTN_STOP        0x02  // Button 2
#define BTN_TEMP_UP     0x04  // Button 3
#define BTN_TEMP_DOWN   0x08  // Button 4
#define BTN_MODE        0x10  // Button 5
#define BTN_6           0x20  // Button 6
#define BTN_7           0x40  // Button 7
#define BTN_8           0x80  // Button 8

// LED definitions (bit positions)
#define LED_AUGER       0  // LED 1
#define LED_FAN         1  // LED 2
#define LED_IGNITER     2  // LED 3
#define LED_WIFI        3  // LED 4
#define LED_MQTT        4  // LED 5
#define LED_ERROR       5  // LED 6
#define LED_RUNNING     6  // LED 7
#define LED_8           7  // LED 8

class TM1638Display {
public:
  TM1638Display();
  void begin();
  void update();

  // Display functions
  void setCurrentTemp(float temp);
  void setTargetTemp(float temp);
  void clear();

  // LED control
  void setLED(uint8_t ledNum, bool state);
  void setRelayLEDs(bool auger, bool fan, bool igniter);
  void setStatusLEDs(bool wifi, bool mqtt, bool error, bool running);

  // Button handling
  uint8_t readButtons();
  bool isButtonPressed(uint8_t button);

private:
  TM1638* _display;
  float _currentTemp;
  float _targetTemp;
  uint8_t _lastButtons;
  uint8_t _ledState;

  void displayTemperature(float temp, bool rightDisplay);
  void formatTemperature(float temp, char* buffer);
};

#endif // TM1638_DISPLAY_H
