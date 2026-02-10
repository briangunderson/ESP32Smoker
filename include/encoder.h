#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

class Encoder {
public:
  Encoder();

  // Initialize I2C and verify device is present
  bool begin();

  // Poll encoder, returns true if input changed
  bool update();

  // Get rotation delta since last update (signed, in clicks)
  int8_t getIncrement();

  // Edge-detected button press (true once per press)
  bool wasButtonPressed();

  // Set RGB LED color (0-255 each)
  void setLEDColor(uint8_t r, uint8_t g, uint8_t b);

  // Check if device was found on I2C bus
  bool isConnected();

private:
  bool     _connected;
  int8_t   _increment;
  int32_t  _lastCounter;     // Previous cumulative encoder counter
  bool     _counterValid;    // Have we read at least one counter value?
  bool     _buttonState;
  bool     _lastButtonState;
  bool     _buttonPressed;
  unsigned long _lastPoll;
  unsigned long _lastButtonTime;

  bool readDevice();
  bool writeRegThenRead(uint8_t reg, uint8_t* buf, uint8_t len);
};

#endif // ENCODER_H
