#include "Arduino.h"
#include "SPI.h"

// Global mock state
unsigned long _mock_millis = 0;
MockGPIOState _mock_gpio[MOCK_MAX_PINS] = {};
MockSerial Serial;
SPIClass SPI;

unsigned long millis(void) {
    return _mock_millis;
}

void mock_set_millis(unsigned long ms) {
    _mock_millis = ms;
}

void mock_advance_millis(unsigned long ms) {
    _mock_millis += ms;
}

void pinMode(uint8_t pin, uint8_t mode) {
    if (pin < MOCK_MAX_PINS) {
        _mock_gpio[pin].mode = mode;
    }
}

void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin < MOCK_MAX_PINS) {
        _mock_gpio[pin].value = val;
        _mock_gpio[pin].write_count++;
    }
}

int digitalRead(uint8_t pin) {
    if (pin < MOCK_MAX_PINS) {
        return _mock_gpio[pin].value;
    }
    return LOW;
}

void mock_reset_gpio(void) {
    memset(_mock_gpio, 0, sizeof(_mock_gpio));
}

void mock_reset_all(void) {
    _mock_millis = 0;
    mock_reset_gpio();
}

void delay(unsigned long ms) { (void)ms; }
void delayMicroseconds(unsigned int us) { (void)us; }
