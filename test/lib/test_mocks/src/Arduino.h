#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

// Arduino types
typedef uint8_t byte;
typedef bool boolean;

// Pin modes
#define INPUT         0x0
#define OUTPUT        0x1
#define INPUT_PULLUP  0x2

// Pin states
#define LOW           0x0
#define HIGH          0x1

// Math macros (Arduino defines these as macros)
#ifndef abs
#define abs(x) ((x) > 0 ? (x) : -(x))
#endif

#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef map
#define map(x, in_min, in_max, out_min, out_max) \
    (((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min))
#endif

// Controllable millis() for testing
extern unsigned long _mock_millis;
unsigned long millis(void);
void mock_set_millis(unsigned long ms);
void mock_advance_millis(unsigned long ms);

// GPIO recording
#define MOCK_MAX_PINS 64
struct MockGPIOState {
    uint8_t mode;
    uint8_t value;
    int write_count;
};
extern MockGPIOState _mock_gpio[MOCK_MAX_PINS];

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

// Reset all mock state
void mock_reset_gpio(void);
void mock_reset_all(void);

// No-op delays
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

// Mock Serial
class MockSerial {
public:
    void begin(unsigned long baud) { (void)baud; }
    void print(const char* s) { (void)s; }
    void print(int v) { (void)v; }
    void print(float v) { (void)v; }
    void println(const char* s = "") { (void)s; }
    void println(int v) { (void)v; }
    void println(float v) { (void)v; }
    int printf(const char* fmt, ...) { (void)fmt; return 0; }
    operator bool() const { return true; }
};

extern MockSerial Serial;

// PROGMEM (no-op on native)
#define PROGMEM
#define F(x) (x)

#endif // MOCK_ARDUINO_H
