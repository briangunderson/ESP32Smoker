#include "max31865.h"
#include "config.h"
#include "logger.h"

MAX31865::MAX31865(uint8_t chipSelectPin, float refResistance, float rtdResistance)
    : _chipSelectPin(chipSelectPin), _refResistance(refResistance),
      _rtdResistance(rtdResistance), _lastFaultStatus(0) {}

bool MAX31865::begin(WireMode wireMode) {
  // Initialize SPI bus (do NOT pass CS pin — we manage CS manually via digitalWrite)
  SPI.begin(PIN_SPI_CLK, PIN_SPI_MISO, PIN_SPI_MOSI);

  // Configure chip select pin manually
  pinMode(_chipSelectPin, OUTPUT);
  digitalWrite(_chipSelectPin, HIGH);

  if (ENABLE_SERIAL_DEBUG) {
    Serial.printf("[MAX31865] SPI pins: SCK=%d, MOSI=%d, MISO=%d, CS=%d\n",
                  PIN_SPI_CLK, PIN_SPI_MOSI, PIN_SPI_MISO, _chipSelectPin);
  }

  // Wait for MAX31865 to power up and stabilize
  delay(250);

  // SPI diagnostic: read config register before writing (should be 0x00 at power-on)
  if (ENABLE_SERIAL_DEBUG) {
    uint8_t preRead = readRegister(MAX31865_CONFIG_REG);
    Serial.printf("[MAX31865] Pre-init config register read: 0x%02X (expect 0x00)\n", preRead);
    if (preRead == 0xFF) {
      Serial.println("[MAX31865] WARNING: Read 0xFF — MISO may be floating/disconnected");
    }

    // Read all registers raw for diagnostics
    Serial.print("[MAX31865] Raw register dump: ");
    for (int r = 0; r <= 7; r++) {
      Serial.printf("[%02X]=0x%02X ", r, readRegister(r));
    }
    Serial.println();
  }

  // Explicitly set fault thresholds to wide-open defaults
  // High threshold = 0xFFFF (max), Low threshold = 0x0000 (min)
  writeRegister(0x03, 0xFF); // High fault MSB
  writeRegister(0x04, 0xFF); // High fault LSB
  writeRegister(0x05, 0x00); // Low fault MSB
  writeRegister(0x06, 0x00); // Low fault LSB
  delay(10);

  if (ENABLE_SERIAL_DEBUG) {
    uint16_t highThresh = readRegister16(0x03);
    uint16_t lowThresh = readRegister16(0x05);
    Serial.printf("[MAX31865] Fault thresholds set: High=0x%04X, Low=0x%04X\n",
                  highThresh, lowThresh);
  }

  // Configure for RTD mode
  uint8_t config = MAX31865_CONFIG_BIAS | MAX31865_CONFIG_MODEAUTO;

  if (wireMode == THREE_WIRE) {
    config |= MAX31865_CONFIG_3WIRE;
  }

  // Try to initialize with retry logic (up to 3 attempts)
  bool success = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[MAX31865] Initialization attempt %d/3, writing config 0x%02X\n",
                    attempt + 1, config);
    }

    // Write configuration
    writeRegister(MAX31865_CONFIG_REG, config);
    delay(100);

    // Verify configuration was written
    uint8_t readBack = readRegister(MAX31865_CONFIG_REG);
    if (ENABLE_SERIAL_DEBUG) {
      Serial.printf("[MAX31865] Config readback: wrote 0x%02X, read 0x%02X\n",
                    config, readBack);
    }

    if (readBack != config) {
      if (ENABLE_SERIAL_DEBUG) {
        if (readBack == 0x00) {
          Serial.println("[MAX31865] DIAG: Read 0x00 — chip not responding. Check:");
          Serial.println("  - CS wiring (must go to pin labeled '5' on Feather)");
          Serial.println("  - SCK wiring (must go to pin labeled 'SCK' on Feather)");
          Serial.println("  - SDI/MOSI wiring (must go to pin labeled 'MO' on Feather)");
          Serial.println("  - SDO/MISO wiring (must go to pin labeled 'MI' on Feather)");
          Serial.println("  - VIN to 3.3V and GND connected");
        } else if (readBack == 0xFF) {
          Serial.println("[MAX31865] DIAG: Read 0xFF — MISO line may be floating");
        }
      }
      delay(200);
      continue;
    }

    // Clear any faults (must set bit 1 to clear fault status register)
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
      printFaultStatus(fault);
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
  delayMicroseconds(1); // CS setup time

  SPI.transfer(addr & 0x7F); // Read bit = 0
  uint8_t value = SPI.transfer(0xFF);

  digitalWrite(_chipSelectPin, HIGH);
  SPI.endTransaction();

  return value;
}

uint16_t MAX31865::readRegister16(uint8_t addr) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(_chipSelectPin, LOW);
  delayMicroseconds(1); // CS setup time

  SPI.transfer(addr & 0x7F); // Read bit = 0
  uint8_t msb = SPI.transfer(0xFF);
  uint8_t lsb = SPI.transfer(0xFF);

  digitalWrite(_chipSelectPin, HIGH);
  SPI.endTransaction();

  return ((uint16_t)msb << 8) | lsb;
}

void MAX31865::writeRegister(uint8_t addr, uint8_t val) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(_chipSelectPin, LOW);
  delayMicroseconds(1); // CS setup time

  SPI.transfer(addr | 0x80); // Write bit = 1
  SPI.transfer(val);

  digitalWrite(_chipSelectPin, HIGH);
  SPI.endTransaction();
}

float MAX31865::readTemperature(void) {
  if (!isHealthy()) {
    return -999.0; // Error indicator
  }

  // In auto-conversion mode, the register always has a fresh value.
  // No need for oneShot() or delay().
  uint16_t rawRTD = readRawRTD();
  if (rawRTD == 0) {
    return -999.0; // No sensor connected or SPI failure
  }
  float resistance = (float)rawRTD * _refResistance / 32768.0;

  float tempC = rtdResistanceToTemperature(resistance);
  return (tempC * 9.0 / 5.0) + 32.0 + TEMP_SENSOR_OFFSET; // Convert to Fahrenheit
}

float MAX31865::readTemperatureC(void) {
  // Check for faults first
  uint8_t fault = getFaultStatus();
  if (fault != 0) {
    DUAL_LOGF(LOG_ERR, "[MAX31865] Fault 0x%02X detected, clearing and retrying...\n", fault);
    printFaultStatus(fault);

    // Dump RTD and threshold registers for diagnosis
    uint16_t rtdRaw = readRegister16(MAX31865_RTD_MSB);
    uint16_t highThresh = readRegister16(0x03);
    uint16_t lowThresh = readRegister16(0x05);
    DUAL_LOGF(LOG_ERR, "[MAX31865] RTD=0x%04X, HighTh=0x%04X, LowTh=0x%04X\n",
              rtdRaw, highThresh, lowThresh);

    // Clear fault and retry once
    clearFaults();
    delay(10);
    fault = getFaultStatus();
    if (fault != 0) {
      DUAL_LOGF(LOG_ERR, "[MAX31865] Fault persists after clear: 0x%02X\n", fault);
      return -999.0;
    }
    DUAL_LOGF(LOG_INFO, "[MAX31865] Fault cleared successfully, reading temp\n");
  }

  // In auto-conversion mode, the register always has a fresh value.
  // No need for oneShot() or delay().
  uint16_t rawRTD = readRawRTD();
  if (rawRTD == 0) {
    return -999.0; // No sensor connected or SPI failure
  }
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
  if (raw & 0x01) {
    // Fault bit is set in RTD register — reading is unreliable
    DUAL_LOGF(LOG_WARNING, "[MAX31865] RTD fault bit set (raw=0x%04X)\n", raw);
    return 0; // Triggers error path in caller
  }
  return raw >> 1;
}

uint8_t MAX31865::getFaultStatus(void) {
  _lastFaultStatus = readRegister(MAX31865_FAULT_STATUS);
  return _lastFaultStatus;
}

void MAX31865::clearFaults(void) {
  uint8_t config = readRegister(MAX31865_CONFIG_REG);
  config &= ~0x2C; // Clear 1-shot bit and fault detection cycle bits
  config |= 0x02;  // Set bit 1 to clear fault status register (auto-clears)
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
  // Callendar-Van Dusen equation: R = R0 * (1 + A*T + B*T^2)
  // Rearranging: B*T^2 + A*T + (1 - R/R0) = 0
  // Quadratic formula gives two roots; the physical root uses (-A + sqrt(...))
  // This works correctly for both positive and negative temperatures.
  const float A = 3.9083e-3;
  const float B = -5.775e-7;

  float Rratio = 1.0 - resistance / _rtdResistance;
  float discriminant = A * A - 4.0 * B * Rratio;
  if (discriminant < 0) return -999.0;
  return (-A + sqrt(discriminant)) / (2.0 * B);
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

void MAX31865::runHardwareDiagnostic(void) {
  Serial.println("\n========================================");
  Serial.println("[MAX31865] HARDWARE DIAGNOSTIC");
  Serial.println("========================================");

  // --- Step 1: Reset to known state ---
  Serial.println("\n[Step 1] Reset chip");
  writeRegister(MAX31865_CONFIG_REG, 0x00);
  delay(100);
  Serial.print("  Registers after reset: ");
  for (int r = 0; r <= 7; r++) {
    Serial.printf("[%02X]=0x%02X ", r, readRegister(r));
  }
  Serial.println();

  // --- Step 2: SPI write/read verification ---
  Serial.println("\n[Step 2] SPI verification");
  writeRegister(0x03, 0xAA);
  writeRegister(0x04, 0x55);
  uint8_t r03 = readRegister(0x03);
  uint8_t r04 = readRegister(0x04);
  Serial.printf("  Write 0xAA->reg03, read: 0x%02X %s\n", r03, r03 == 0xAA ? "OK" : "FAIL!");
  Serial.printf("  Write 0x55->reg04, read: 0x%02X %s\n", r04, r04 == 0x55 ? "OK" : "FAIL!");
  // Restore thresholds
  writeRegister(0x03, 0xFF);
  writeRegister(0x04, 0xFF);
  writeRegister(0x05, 0x00);
  writeRegister(0x06, 0x00);

  // --- Step 3: Enable VBIAS, run fault detection (3-wire) ---
  Serial.println("\n[Step 3] Fault detection cycle (3-WIRE mode)");
  // Must disable auto-conversion for fault detection per datasheet
  writeRegister(MAX31865_CONFIG_REG, 0x80 | 0x10); // VBIAS + 3-wire, no auto-convert
  delay(100); // Bias settle time

  // Clear existing faults
  uint8_t cfg = readRegister(MAX31865_CONFIG_REG);
  cfg |= 0x02; // Set fault clear bit
  writeRegister(MAX31865_CONFIG_REG, cfg);
  delay(10);

  // Start automatic fault detection (D3:D2 = 01)
  cfg = readRegister(MAX31865_CONFIG_REG);
  cfg &= ~0x0C; // Clear D3:D2
  cfg |= 0x04;  // Set D2 = automatic fault detection
  writeRegister(MAX31865_CONFIG_REG, cfg);
  Serial.printf("  Config: 0x%02X (fault det started)\n", cfg);

  // Wait for D3:D2 to return to 00 (cycle complete)
  int elapsed = 0;
  for (int i = 0; i < 100; i++) {
    delay(10);
    elapsed += 10;
    cfg = readRegister(MAX31865_CONFIG_REG);
    if ((cfg & 0x0C) == 0) break;
  }
  Serial.printf("  Completed in ~%dms\n", elapsed);

  uint8_t fault = getFaultStatus();
  Serial.printf("  Fault status: 0x%02X\n", fault);
  if (fault) {
    printFaultStatus(fault);
    if (fault & 0x20) Serial.println("  >> REFIN- too high: check reference resistor");
    if (fault & 0x10) Serial.println("  >> REFIN- too low: FORCE- open (no current through ref resistor)");
    if (fault & 0x08) Serial.println("  >> RTDIN- too low: FORCE- open (no current through RTD)");
    if (fault & 0x04) Serial.println("  >> Over/undervoltage on RTD inputs");
  } else {
    Serial.println("  No hardware faults (wiring looks OK in 3-wire mode)");
  }

  // --- Step 4: One-shot conversion (3-wire) ---
  Serial.println("\n[Step 4] One-shot conversion (3-WIRE mode)");
  writeRegister(MAX31865_CONFIG_REG, 0x80 | 0x10); // VBIAS + 3-wire
  delay(100);
  // Clear faults
  cfg = readRegister(MAX31865_CONFIG_REG);
  cfg |= 0x02;
  writeRegister(MAX31865_CONFIG_REG, cfg);
  delay(10);
  // Trigger one-shot
  cfg = readRegister(MAX31865_CONFIG_REG);
  cfg |= 0x20;
  writeRegister(MAX31865_CONFIG_REG, cfg);
  delay(100); // Conversion time ~65ms

  uint16_t rtdRaw = readRegister16(MAX31865_RTD_MSB);
  uint16_t adcVal = rtdRaw >> 1;
  bool faultBit = rtdRaw & 0x01;
  float resistance = (float)adcVal * _refResistance / 32768.0;
  Serial.printf("  RTD raw=0x%04X, ADC=%u, Fault=%d, R=%.2f ohm\n",
                rtdRaw, adcVal, faultBit, resistance);
  fault = getFaultStatus();
  if (fault) { Serial.printf("  Fault: 0x%02X - ", fault); printFaultStatus(fault); }

  // --- Step 5: Fault detection (2/4-wire) ---
  Serial.println("\n[Step 5] Fault detection cycle (2/4-WIRE mode)");
  writeRegister(MAX31865_CONFIG_REG, 0x80); // VBIAS only, no 3-wire bit
  delay(100);
  cfg = readRegister(MAX31865_CONFIG_REG);
  cfg |= 0x02;
  writeRegister(MAX31865_CONFIG_REG, cfg);
  delay(10);
  cfg = readRegister(MAX31865_CONFIG_REG);
  cfg &= ~0x0C;
  cfg |= 0x04;
  writeRegister(MAX31865_CONFIG_REG, cfg);
  for (int i = 0; i < 100; i++) {
    delay(10);
    cfg = readRegister(MAX31865_CONFIG_REG);
    if ((cfg & 0x0C) == 0) break;
  }
  fault = getFaultStatus();
  Serial.printf("  Fault status: 0x%02X\n", fault);
  if (fault) {
    printFaultStatus(fault);
  } else {
    Serial.println("  No hardware faults (wiring looks OK in 2/4-wire mode)");
  }

  // --- Step 6: One-shot conversion (2/4-wire) ---
  Serial.println("\n[Step 6] One-shot conversion (2/4-WIRE mode)");
  writeRegister(MAX31865_CONFIG_REG, 0x80); // VBIAS, no 3-wire
  delay(100);
  cfg = readRegister(MAX31865_CONFIG_REG);
  cfg |= 0x02;
  writeRegister(MAX31865_CONFIG_REG, cfg);
  delay(10);
  cfg = readRegister(MAX31865_CONFIG_REG);
  cfg |= 0x20;
  writeRegister(MAX31865_CONFIG_REG, cfg);
  delay(100);

  rtdRaw = readRegister16(MAX31865_RTD_MSB);
  adcVal = rtdRaw >> 1;
  faultBit = rtdRaw & 0x01;
  resistance = (float)adcVal * _refResistance / 32768.0;
  Serial.printf("  RTD raw=0x%04X, ADC=%u, Fault=%d, R=%.2f ohm\n",
                rtdRaw, adcVal, faultBit, resistance);
  fault = getFaultStatus();
  if (fault) { Serial.printf("  Fault: 0x%02X - ", fault); printFaultStatus(fault); }

  // --- Step 7: Read RTD registers individually ---
  Serial.println("\n[Step 7] Individual register reads");
  uint8_t rtd_msb = readRegister(0x01);
  uint8_t rtd_lsb = readRegister(0x02);
  uint16_t rtd16 = readRegister16(0x01);
  Serial.printf("  Reg[01]=0x%02X, Reg[02]=0x%02X, combined=0x%04X\n", rtd_msb, rtd_lsb,
                ((uint16_t)rtd_msb << 8) | rtd_lsb);
  Serial.printf("  readRegister16(0x01)=0x%04X\n", rtd16);

  // --- Summary ---
  Serial.println("\n--- DIAGNOSTIC SUMMARY ---");
  Serial.println("If ADC=0 in BOTH wire modes with no D5-D2 faults:");
  Serial.println("  -> RTD wires shorted together, or probe not connected to terminals");
  Serial.println("If fault 0x10/0x08 (FORCE- open):");
  Serial.println("  -> No current path. RTD probe wire disconnected");
  Serial.println("If fault 0x20 (REFIN too high):");
  Serial.println("  -> Reference resistor disconnected or wrong value");
  Serial.println("If ADC reads OK in 2/4-wire but not 3-wire:");
  Serial.println("  -> Your probe is 2-wire. Change MAX31865_WIRE_MODE to 2 in config.h");
  Serial.println("\nPT1000 probe wiring on typical MAX31865 breakout:");
  Serial.println("  2-wire: Connect to F+ and F- (or RTD+ and RTD-)");
  Serial.println("  3-wire: Two same-color wires to one side, different to other");
  Serial.println("  Check if breakout has 2/3/4-wire jumper/solder pad");
  Serial.printf("  Configured ref resistor: %.0f ohm\n", _refResistance);
  Serial.printf("  Configured RTD R0: %.0f ohm\n", _rtdResistance);
  Serial.println("========================================\n");
}

MAX31865::DiagData MAX31865::getDiagnostics(void) {
  DiagData d;
  d.configReg = readRegister(MAX31865_CONFIG_REG);
  d.rtdRaw = readRegister16(MAX31865_RTD_MSB);
  d.adcValue = d.rtdRaw >> 1;
  d.faultStatus = readRegister(MAX31865_FAULT_STATUS);
  d.resistance = (float)d.adcValue * _refResistance / 32768.0;
  d.tempC = rtdResistanceToTemperature(d.resistance);
  d.tempF = d.tempC * 9.0 / 5.0 + 32.0;
  d.refResistance = _refResistance;
  d.rtdNominal = _rtdResistance;
  for (int i = 0; i < 8; i++) {
    d.registers[i] = readRegister(i);
  }
  return d;
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
