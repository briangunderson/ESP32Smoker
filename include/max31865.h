#ifndef MAX31865_H
#define MAX31865_H

#include <Arduino.h>
#include <SPI.h>

// MAX31865 Register Addresses
#define MAX31865_CONFIG_REG      0x00
#define MAX31865_CONFIG_BIAS     0x80
#define MAX31865_CONFIG_MODEAUTO 0x40
#define MAX31865_CONFIG_ONESHOT  0x20
#define MAX31865_CONFIG_3WIRE    0x10
#define MAX31865_CONFIG_FAULTDET 0x04
#define MAX31865_CONFIG_FAULT    0x02
#define MAX31865_CONFIG_50HZ     0x01

#define MAX31865_RTD_MSB         0x01
#define MAX31865_RTD_LSB         0x02
#define MAX31865_HIGH_FAULT_MSB  0x03
#define MAX31865_HIGH_FAULT_LSB  0x04
#define MAX31865_LOW_FAULT_MSB   0x05
#define MAX31865_LOW_FAULT_LSB   0x06
#define MAX31865_FAULT_STATUS    0x07

// Fault Status Bits
#define MAX31865_FAULT_HIGHTEMP  0x80
#define MAX31865_FAULT_LOWTEMP   0x40
#define MAX31865_FAULT_RTDIN     0x20
#define MAX31865_FAULT_REFIN     0x10
#define MAX31865_FAULT_REFIN_LO  0x08
#define MAX31865_FAULT_RTDIN_LO  0x04

class MAX31865 {
public:
  enum WireMode {
    TWO_WIRE = 0,
    THREE_WIRE = 1,
    FOUR_WIRE = 0
  };

  // Constructor
  MAX31865(uint8_t chipSelectPin, float refResistance = 430.0, float rtdResistance = 100.0);

  // Initialize sensor
  bool begin(WireMode wireMode = THREE_WIRE);

  // Read temperature in Fahrenheit
  float readTemperature(void);

  // Read temperature in Celsius
  float readTemperatureC(void);

  // Get last fault status
  uint8_t getFaultStatus(void);

  // Clear fault status
  void clearFaults(void);

  // Check if sensor is functioning
  bool isHealthy(void);

  // Set temperature thresholds for fault detection
  void setHighFaultThreshold(float tempF);
  void setLowFaultThreshold(float tempF);

  // Get raw RTD resistance value
  uint16_t readRawRTD(void);

  // Decode fault status to human-readable string
  void printFaultStatus(uint8_t fault);

  // Print detailed diagnostics (all registers, resistance, calculations)
  void printDetailedDiagnostics(void);

private:
  uint8_t _chipSelectPin;
  float _refResistance;
  float _rtdResistance;
  uint8_t _lastFaultStatus;

  // SPI Communication
  uint8_t readRegister(uint8_t addr);
  uint16_t readRegister16(uint8_t addr);
  void writeRegister(uint8_t addr, uint8_t val);
  void enableBias(bool enable);
  void autoConvert(bool enable);
  void oneShot(void);

  // Resistance to Temperature Conversion (Callendar-Van Dusen equation)
  float rtdResistanceToTemperature(float resistance);
  float resistanceToTemperatureC(float resistance);
};

#endif // MAX31865_H
