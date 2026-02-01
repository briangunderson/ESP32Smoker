#include "max31865.h"
#include "config.h"
#include "logger.h"

MAX31865::MAX31865(uint8_t chipSelectPin, float refResistance, float rtdResistance)
    : _chipSelectPin(chipSelectPin), _refResistance(refResistance),
      _rtdResistance(rtdResistance), _lastFaultStatus(0) {}

bool MAX31865::begin(WireMode wireMode) {
  // Initialize SPI
  SPI.begin(PIN_SPI_CLK, PIN_SPI_MISO, PIN_SPI_MOSI, _chipSelectPin);

  // Configure chip select pin
  pinMode(_chipSelectPin, OUTPUT);
  digitalWrite(_chipSelectPin, HIGH);

  // Wait for MAX31865 to power up and stabilize
  delay(250);

  // Configure for RTD mode
  uint8_t config = MAX31865_CONFIG_BIAS | MAX31865_CONFIG_MODEAUTO;

  if (wireMode == THREE_WIRE) {
    config |= MAX31865_CONFIG_3WIRE;
  }

  // Try to initialize with retry logic (up to 3 attempts)
  bool success = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    if (ENABLE_SERIAL_DEBUG && attempt > 0) {
      Serial.printf("[MAX31865] Initialization attempt %d/3\n", attempt + 1);
    }

    // Write configuration
    writeRegister(MAX31865_CONFIG_REG, config);
    delay(100);

    // Verify configuration was written
    uint8_t readBack = readRegister(MAX31865_CONFIG_REG);
    if (readBack != config) {
      if (ENABLE_SERIAL_DEBUG) {
        Serial.printf("[MAX31865] Config mismatch: wrote 0x%02X, read 0x%02X\n",
                      config, readBack);
      }
      delay(200);
      continue;
    }

    // Clear any faults
    clearFaults();
    delay(50);

    // Check if healthy
    if (isHealthy()) {
      success = true;
      break;
    }

    // Not healthy, report fault status
    if (ENABLE_SERIAL_DEBUG) {
      uint8_t fault = getFaultStatus();
      Serial.printf("[MAX31865] Fault detected: 0x%02X\n", fault);
    }

    delay(200);
  }

  if (success && ENABLE_SERIAL_DEBUG) {
    Serial.println("[MAX31865] Successfully initialized and verified");
  } else if (ENABLE_SERIAL_DEBUG) {
    Serial.println("[MAX31865] WARNING: Initialization failed after 3 attempts");
  }

  return success;
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
  // Check for faults first
  uint8_t fault = getFaultStatus();
  if (fault != 0) {
    DUAL_LOGF(LOG_ERR, "[MAX31865] ERROR: Sensor has faults!\n");
    printFaultStatus(fault);
    return -999.0;
  }

  oneShot();
  delay(100);

  uint16_t rawRTD = readRawRTD();
  float resistance = (float)rawRTD * _refResistance / 32768.0;
  float tempC = rtdResistanceToTemperature(resistance);

#if ENABLE_MAX31865_VERBOSE
  // Verbose mode: log every single read with full details
  DUAL_LOGF(LOG_DEBUG, "[MAX31865] RAW=0x%04X (%u) | R=%.2fΩ | RefR=%.0fΩ | T=%.2f°C (%.1f°F)\n",
            rawRTD, rawRTD, resistance, _refResistance, tempC, tempC * 9.0 / 5.0 + 32.0);

  // Detailed diagnostics every 30 seconds
  static unsigned long lastDetailedLog = 0;
  if (millis() - lastDetailedLog > 30000) {
    lastDetailedLog = millis();
    printDetailedDiagnostics();
  }
#else
  // Normal mode: periodic summary only
  static unsigned long lastDetailedLog = 0;
  if (millis() - lastDetailedLog > 10000) {
    lastDetailedLog = millis();
    DUAL_LOGF(LOG_DEBUG, "[MAX31865] Raw ADC: %u, Resistance: %.2f Ω, Temp: %.2f°C (%.2f°F)\n",
              rawRTD, resistance, tempC, tempC * 9.0 / 5.0 + 32.0);
  }
#endif

  return tempC;
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

void MAX31865::printFaultStatus(uint8_t fault) {
  if (fault == 0) {
    DUAL_LOGF(LOG_INFO, "[MAX31865] No faults\n");
    return;
  }

  DUAL_LOGF(LOG_ERR, "[MAX31865] FAULT DETECTED:\n");
  if (fault & MAX31865_FAULT_HIGHTEMP) {
    DUAL_LOGF(LOG_ERR, "  - RTD High Threshold exceeded\n");
  }
  if (fault & MAX31865_FAULT_LOWTEMP) {
    DUAL_LOGF(LOG_ERR, "  - RTD Low Threshold exceeded\n");
  }
  if (fault & MAX31865_FAULT_RTDIN) {
    DUAL_LOGF(LOG_ERR, "  - RTDIN- > 0.85 x VBIAS (force open - likely disconnected RTD)\n");
  }
  if (fault & MAX31865_FAULT_REFIN) {
    DUAL_LOGF(LOG_ERR, "  - REFIN- > 0.85 x VBIAS (force open - likely disconnected reference)\n");
  }
  if (fault & MAX31865_FAULT_REFIN_LO) {
    DUAL_LOGF(LOG_ERR, "  - REFIN- < 0.85 x VBIAS (FORCE- open)\n");
  }
  if (fault & MAX31865_FAULT_RTDIN_LO) {
    DUAL_LOGF(LOG_ERR, "  - RTDIN- < 0.85 x VBIAS (FORCE- open)\n");
  }
  DUAL_LOGF(LOG_ERR, "[MAX31865] Fault byte: 0x%02X\n", fault);
}

void MAX31865::printDetailedDiagnostics(void) {
  DUAL_LOGF(LOG_INFO, "\n========================================\n");
  DUAL_LOGF(LOG_INFO, "[MAX31865] DETAILED DIAGNOSTICS\n");
  DUAL_LOGF(LOG_INFO, "========================================\n");

  // Read all registers
  uint8_t config = readRegister(MAX31865_CONFIG_REG);
  uint16_t rtdRaw = readRegister16(MAX31865_RTD_MSB);
  uint16_t rtdValue = rtdRaw >> 1;  // Remove fault bit
  bool faultBit = rtdRaw & 0x01;
  uint16_t highThresh = readRegister16(0x03);
  uint16_t lowThresh = readRegister16(0x05);
  uint8_t faultStatus = getFaultStatus();

  // Configuration register breakdown
  DUAL_LOGF(LOG_INFO, "Configuration Register: 0x%02X\n", config);
  DUAL_LOGF(LOG_INFO, "  - VBIAS:      %s\n", (config & 0x80) ? "ON" : "OFF");
  DUAL_LOGF(LOG_INFO, "  - Conversion: %s\n", (config & 0x40) ? "AUTO" : "NORM");
  DUAL_LOGF(LOG_INFO, "  - 1-Shot:     %s\n", (config & 0x20) ? "YES" : "NO");
  DUAL_LOGF(LOG_INFO, "  - Wire Mode:  %s\n", (config & 0x10) ? "3-WIRE" : "2/4-WIRE");
  DUAL_LOGF(LOG_INFO, "  - Fault Det:  %d\n", (config & 0x0C) >> 2);
  DUAL_LOGF(LOG_INFO, "  - Clear Flt:  %s\n", (config & 0x02) ? "YES" : "NO");
  DUAL_LOGF(LOG_INFO, "  - 50/60Hz:    %s\n", (config & 0x01) ? "50Hz" : "60Hz");

  // RTD register analysis
  DUAL_LOGF(LOG_INFO, "\nRTD Register:\n");
  DUAL_LOGF(LOG_INFO, "  - Raw 16-bit:    0x%04X (%u decimal)\n", rtdRaw, rtdRaw);
  DUAL_LOGF(LOG_INFO, "  - RTD Value:     0x%04X (%u decimal)\n", rtdValue, rtdValue);
  DUAL_LOGF(LOG_INFO, "  - Fault bit:     %s\n", faultBit ? "SET (FAULT!)" : "clear");

  // Resistance calculation
  float resistance = (float)rtdValue * _refResistance / 32768.0;
  DUAL_LOGF(LOG_INFO, "\nResistance Calculation:\n");
  DUAL_LOGF(LOG_INFO, "  - RTD ADC Value: %u (0x%04X)\n", rtdValue, rtdValue);
  DUAL_LOGF(LOG_INFO, "  - Reference R:   %.2f Ω (configured)\n", _refResistance);
  DUAL_LOGF(LOG_INFO, "  - RTD R at 0°C:  %.2f Ω (configured)\n", _rtdResistance);
  DUAL_LOGF(LOG_INFO, "  - Calculated R:  %.2f Ω\n", resistance);

  // Temperature calculation
  float tempC = rtdResistanceToTemperature(resistance);
  float tempF = tempC * 9.0 / 5.0 + 32.0;
  DUAL_LOGF(LOG_INFO, "  - Temperature:   %.2f°C (%.1f°F)\n", tempC, tempF);

  // Expected resistance for common temperatures
  DUAL_LOGF(LOG_INFO, "\nExpected PT1000 Resistance Values:\n");
  DUAL_LOGF(LOG_INFO, "  - At   0°C (32°F):   ~1000Ω\n");
  DUAL_LOGF(LOG_INFO, "  - At  25°C (77°F):   ~1098Ω\n");
  DUAL_LOGF(LOG_INFO, "  - At 100°C (212°F):  ~1385Ω\n");
  DUAL_LOGF(LOG_INFO, "  - At 225°C (437°F):  ~1878Ω\n");

  // Thresholds
  DUAL_LOGF(LOG_INFO, "\nFault Thresholds:\n");
  DUAL_LOGF(LOG_INFO, "  - High Threshold: 0x%04X\n", highThresh);
  DUAL_LOGF(LOG_INFO, "  - Low Threshold:  0x%04X\n", lowThresh);

  // Fault status
  DUAL_LOGF(LOG_INFO, "\nFault Status: 0x%02X\n", faultStatus);
  if (faultStatus != 0) {
    printFaultStatus(faultStatus);
  } else {
    DUAL_LOGF(LOG_INFO, "  - No faults detected\n");
  }

  // Troubleshooting hints
  DUAL_LOGF(LOG_INFO, "\nTroubleshooting:\n");
  if (rtdValue == 0 || rtdValue == 0xFFFF) {
    DUAL_LOGF(LOG_WARNING, "  ! RTD value is %s - check SPI wiring!\n",
              rtdValue == 0 ? "0x0000" : "0xFFFF");
  } else if (resistance < 500) {
    DUAL_LOGF(LOG_WARNING, "  ! Resistance very low (%.2fΩ) - possible short circuit\n", resistance);
  } else if (resistance > 3000) {
    DUAL_LOGF(LOG_WARNING, "  ! Resistance very high (%.2fΩ) - possible open circuit or wrong ref resistor\n", resistance);
  } else if (resistance > 800 && resistance < 1200) {
    DUAL_LOGF(LOG_INFO, "  ✓ Resistance in expected range for room temp PT1000\n");
  }

  DUAL_LOGF(LOG_INFO, "========================================\n\n");
}
