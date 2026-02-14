#ifndef MOCK_SPI_H
#define MOCK_SPI_H

#include "Arduino.h"

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

#define MSBFIRST 1
#define LSBFIRST 0

class SPISettings {
public:
    SPISettings() {}
    SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) {
        (void)clock; (void)bitOrder; (void)dataMode;
    }
};

class SPIClass {
public:
    void begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1) {
        (void)sck; (void)miso; (void)mosi; (void)ss;
    }
    void end() {}
    void beginTransaction(SPISettings settings) { (void)settings; }
    void endTransaction() {}
    uint8_t transfer(uint8_t data) { (void)data; return 0; }
    void transfer(void *buf, size_t count) { (void)buf; (void)count; }
    void setBitOrder(uint8_t bitOrder) { (void)bitOrder; }
    void setDataMode(uint8_t dataMode) { (void)dataMode; }
    void setClockDivider(uint32_t clockDiv) { (void)clockDiv; }
};

extern SPIClass SPI;

#endif // MOCK_SPI_H
