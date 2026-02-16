#include "encoder.h"

// M5Stack Unit Encoder (U135) I2C Register Map
#define ENCODER_REG_ENCODER  0x10  // Encoder counter (int32_t, 4 bytes, cumulative)
#define ENCODER_REG_BUTTON   0x20  // Button state (1 byte: 0=released, 1=pressed)
#define ENCODER_REG_RGB      0x30  // RGB LED (3 bytes: R, G, B)
#define ENCODER_REG_RESET    0x40  // Reset encoder counter (write 1 byte)

Encoder::Encoder()
    : _connected(false),
      _increment(0),
      _lastCounter(0),
      _counterValid(false),
      _buttonState(false),
      _lastButtonState(false),
      _buttonPressed(false),
      _lastPoll(0),
      _lastButtonTime(0) {}

bool Encoder::begin() {
  if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[ENCODER] Initializing M5Stack Unit Encoder...");
    Serial.printf("[ENCODER] I2C: SDA=%d, SCL=%d, Addr=0x%02X\n",
                  PIN_I2C_SDA, PIN_I2C_SCL, ENCODER_I2C_ADDR);
  }

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Probe device
  Wire.beginTransmission(ENCODER_I2C_ADDR);
  uint8_t error = Wire.endTransmission();
  _connected = (error == 0);

  if (_connected) {
    // Reset the encoder counter to zero
    Wire.beginTransmission(ENCODER_I2C_ADDR);
    Wire.write(ENCODER_REG_RESET);
    Wire.write((uint8_t)1);
    Wire.endTransmission();

    delay(10);

    // Read initial counter value
    uint8_t buf[4];
    if (writeRegThenRead(ENCODER_REG_ENCODER, buf, 4)) {
      _lastCounter = (int32_t)((uint32_t)buf[0] |
                               ((uint32_t)buf[1] << 8) |
                               ((uint32_t)buf[2] << 16) |
                               ((uint32_t)buf[3] << 24));
      _counterValid = true;
    }

    // Read initial button state to prevent false edge detection on first poll
    uint8_t btnBuf[1];
    if (writeRegThenRead(ENCODER_REG_BUTTON, btnBuf, 1)) {
      _lastButtonState = (btnBuf[0] != 0);
      _buttonState = _lastButtonState;
    }

    setLEDColor(0, 20, 0); // Dim green = idle

    if (ENABLE_SERIAL_DEBUG) {
      Serial.println("[ENCODER] Device found and initialized");
    }
  } else {
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[ENCODER] Device NOT found at 0x%02X (error: %d)\n",
                    ENCODER_I2C_ADDR, error);
    }
  }

  return _connected;
}

bool Encoder::writeRegThenRead(uint8_t reg, uint8_t* buf, uint8_t len) {
  Wire.beginTransmission(ENCODER_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;  // repeated start

  uint8_t bytesRead = Wire.requestFrom((uint8_t)ENCODER_I2C_ADDR, len);
  if (bytesRead != len) return false;

  for (uint8_t i = 0; i < len; i++) {
    buf[i] = Wire.read();
  }
  return true;
}

bool Encoder::update() {
  if (!_connected) return false;

  unsigned long now = millis();
  if (now - _lastPoll < ENCODER_POLL_INTERVAL) return false;
  _lastPoll = now;

  return readDevice();
}

bool Encoder::readDevice() {
  _increment = 0;
  _buttonPressed = false;

  // Read encoder counter (register 0x10, 4 bytes, int32_t little-endian)
  uint8_t encBuf[4];
  if (writeRegThenRead(ENCODER_REG_ENCODER, encBuf, 4)) {
    int32_t counter = (int32_t)((uint32_t)encBuf[0] |
                                ((uint32_t)encBuf[1] << 8) |
                                ((uint32_t)encBuf[2] << 16) |
                                ((uint32_t)encBuf[3] << 24));

    if (_counterValid) {
      int32_t delta = counter - _lastCounter;
      // Clamp to int8_t range
      if (delta > 127) delta = 127;
      if (delta < -128) delta = -128;
      _increment = (int8_t)delta;
    } else {
      _counterValid = true;
    }
    _lastCounter = counter;
  }

  // Read button state (register 0x20, 1 byte)
  uint8_t btnBuf[1];
  if (writeRegThenRead(ENCODER_REG_BUTTON, btnBuf, 1)) {
    bool currentButton = (btnBuf[0] != 0);

    // Edge detection with debounce (press = transition to pressed)
    if (currentButton && !_lastButtonState) {
      if (millis() - _lastButtonTime >= ENCODER_BTN_DEBOUNCE) {
        _buttonPressed = true;
        _lastButtonTime = millis();
      }
    }
    _lastButtonState = currentButton;
    _buttonState = currentButton;
  }

  bool changed = (_increment != 0 || _buttonPressed);

  if (changed && ENABLE_SERIAL_DEBUG) {
    if (_increment != 0) {
      Serial.printf("[ENCODER] Rotation: %+d clicks\n", _increment);
    }
    if (_buttonPressed) {
      Serial.println("[ENCODER] Button pressed");
    }
  }

  return changed;
}

int8_t Encoder::getIncrement() {
  return _increment;
}

bool Encoder::wasButtonPressed() {
  return _buttonPressed;
}

void Encoder::setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  if (!_connected) return;

  Wire.beginTransmission(ENCODER_I2C_ADDR);
  Wire.write(ENCODER_REG_RGB);
  Wire.write(r);
  Wire.write(g);
  Wire.write(b);
  Wire.endTransmission();
}

bool Encoder::isConnected() {
  return _connected;
}
