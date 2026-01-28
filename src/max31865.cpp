#include "max31865.h"
#include "config.h"

MAX31865::MAX31865(uint8_t chipSelectPin, float refResistance, float rtdResistance)
    : _chipSelectPin(chipSelectPin), _refResistance(refResistance),
      _rtdResistance(rtdResistance), _lastFaultStatus(0) {}

bool MAX31865::begin(WireMode wireMode) {
  // Initialize SPI
  SPI.begin(PIN_SPI_CLK, PIN_SPI_MISO, PIN_SPI_MOSI, _chipSelectPin);

  // Configure chip select pin
  pinMode(_chipSelectPin, OUTPUT);
  digitalWrite(_chipSelectPin, HIGH);

  // Configure for RTD mode
  uint8_t config = MAX31865_CONFIG_BIAS | MAX31865_CONFIG_MODEAUTO;

  if (wireMode == THREE_WIRE) {
    config |= MAX31865_CONFIG_3WIRE;
  }

  writeRegister(MAX31865_CONFIG_REG, config);
  delay(100);

  // Clear any faults
  clearFaults();

  return isHealthy();
}

uint8_t MAX31865::readRegister(uint8_t addr) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(_chipSelectPin, LOW);

  SPI.transfer(addr & 0x7F); // Read bit = 0
  uint8_t value = SPI.transfer(0);

  digitalWrite(_chipSelectPin, HIGH);
  SPI.endTransaction();

  return value;
}

uint16_t MAX31865::readRegister16(uint8_t addr) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(_chipSelectPin, LOW);

  SPI.transfer(addr & 0x7F); // Read bit = 0
  uint8_t msb = SPI.transfer(0);
  uint8_t lsb = SPI.transfer(0);

  digitalWrite(_chipSelectPin, HIGH);
  SPI.endTransaction();

  return ((uint16_t)msb << 8) | lsb;
}

void MAX31865::writeRegister(uint8_t addr, uint8_t val) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(_chipSelectPin, LOW);

  SPI.transfer(addr | 0x80); // Write bit = 1
  SPI.transfer(val);

  digitalWrite(_chipSelectPin, HIGH);
  SPI.endTransaction();
}

float MAX31865::readTemperature(void) {
  if (!isHealthy()) {
    return -999.0; // Error indicator
  }

  // Trigger conversion
  oneShot();
  delay(100); // Wait for conversion

  uint16_t rawRTD = readRawRTD();
  float resistance = (float)rawRTD * _refResistance / 32768.0;

  float tempC = rtdResistanceToTemperature(resistance);
  return (tempC * 9.0 / 5.0) + 32.0 + TEMP_SENSOR_OFFSET; // Convert to Fahrenheit
}

float MAX31865::readTemperatureC(void) {
  if (!isHealthy()) {
    return -999.0;
  }

  oneShot();
  delay(100);

  uint16_t rawRTD = readRawRTD();
  float resistance = (float)rawRTD * _refResistance / 32768.0;

  return rtdResistanceToTemperature(resistance);
}

uint16_t MAX31865::readRawRTD(void) {
  uint16_t raw = readRegister16(MAX31865_RTD_MSB);
  return raw >> 1; // Fault bit is LSB
}

uint8_t MAX31865::getFaultStatus(void) {
  _lastFaultStatus = readRegister(MAX31865_FAULT_STATUS);
  return _lastFaultStatus;
}

void MAX31865::clearFaults(void) {
  uint8_t config = readRegister(MAX31865_CONFIG_REG);
  config &= ~0x0C; // Clear fault bits
  writeRegister(MAX31865_CONFIG_REG, config);
}

bool MAX31865::isHealthy(void) {
  uint8_t fault = getFaultStatus();
  return (fault == 0);
}

void MAX31865::oneShot(void) {
  uint8_t config = readRegister(MAX31865_CONFIG_REG);
  config |= MAX31865_CONFIG_ONESHOT;
  writeRegister(MAX31865_CONFIG_REG, config);
}

float MAX31865::rtdResistanceToTemperature(float resistance) {
  // Callendar-Van Dusen equation (simplified for positive temperatures)
  // Solving for temperature given resistance
  const float A = 3.9083e-3;
  const float B = -5.775e-7;

  float tempC;
  if (resistance < _rtdResistance) {
    // Quadratic formula for negative temperatures
    float a = -A / (2 * B);
    float b = 1 + A / B * (1 - _rtdResistance / resistance);
    tempC = a + sqrt(a * a + b);
  } else {
    // For positive temps (simplification)
    tempC = (resistance - _rtdResistance) / (_rtdResistance * A);
  }

  return tempC;
}

float MAX31865::resistanceToTemperatureC(float resistance) {
  return rtdResistanceToTemperature(resistance);
}

void MAX31865::setHighFaultThreshold(float tempF) {
  float tempC = (tempF - 32.0) * 5.0 / 9.0;
  // Implementation would set HIGH_FAULT threshold registers
  // Simplified for now
}

void MAX31865::setLowFaultThreshold(float tempF) {
  float tempC = (tempF - 32.0) * 5.0 / 9.0;
  // Implementation would set LOW_FAULT threshold registers
  // Simplified for now
}
