#include "max31865.h"
#include "mock_helpers.h"

// Mock sensor state
float _mock_sensor_temp_c = 25.0f;
uint8_t _mock_sensor_fault = 0;

void mock_set_sensor_temp_c(float tempC) { _mock_sensor_temp_c = tempC; }
void mock_set_sensor_fault(uint8_t fault) { _mock_sensor_fault = fault; }
void mock_reset_sensor(void) {
    _mock_sensor_temp_c = 25.0f;
    _mock_sensor_fault = 0;
}

// MAX31865 mock implementation
MAX31865::MAX31865(uint8_t chipSelectPin, float refResistance, float rtdResistance)
    : _chipSelectPin(chipSelectPin), _refResistance(refResistance),
      _rtdResistance(rtdResistance), _lastFaultStatus(0) {}

bool MAX31865::begin(WireMode wireMode) {
    (void)wireMode;
    return true;
}

float MAX31865::readTemperature(void) {
    float c = readTemperatureC();
    if (c < -100) return -999.0f;
    return c * 9.0f / 5.0f + 32.0f;
}

float MAX31865::readTemperatureC(void) {
    if (_mock_sensor_fault != 0) {
        _lastFaultStatus = _mock_sensor_fault;
        return -999.0f;
    }
    return _mock_sensor_temp_c;
}

uint8_t MAX31865::getFaultStatus(void) {
    return _mock_sensor_fault;
}

void MAX31865::clearFaults(void) {
    _lastFaultStatus = 0;
}

bool MAX31865::isHealthy(void) {
    return _mock_sensor_fault == 0;
}

void MAX31865::setHighFaultThreshold(float tempF) { (void)tempF; }
void MAX31865::setLowFaultThreshold(float tempF) { (void)tempF; }

uint16_t MAX31865::readRawRTD(void) { return 0; }
void MAX31865::printFaultStatus(uint8_t fault) { (void)fault; }
void MAX31865::printDetailedDiagnostics(void) {}
void MAX31865::runHardwareDiagnostic(void) {}

MAX31865::DiagData MAX31865::getDiagnostics(void) {
    DiagData d = {};
    d.faultStatus = _mock_sensor_fault;
    d.tempC = _mock_sensor_temp_c;
    d.tempF = _mock_sensor_temp_c * 9.0f / 5.0f + 32.0f;
    d.refResistance = _refResistance;
    d.rtdNominal = _rtdResistance;
    return d;
}

// Private methods (stubs)
uint8_t MAX31865::readRegister(uint8_t addr) { (void)addr; return 0; }
uint16_t MAX31865::readRegister16(uint8_t addr) { (void)addr; return 0; }
void MAX31865::writeRegister(uint8_t addr, uint8_t val) { (void)addr; (void)val; }
void MAX31865::enableBias(bool enable) { (void)enable; }
void MAX31865::autoConvert(bool enable) { (void)enable; }
void MAX31865::oneShot(void) {}

float MAX31865::rtdResistanceToTemperature(float resistance) {
    const float A = 3.9083e-3f;
    const float B = -5.775e-7f;

    float tempC;
    if (resistance < _rtdResistance) {
        float a = -A / (2.0f * B);
        float b = 1.0f + A / B * (1.0f - _rtdResistance / resistance);
        tempC = a + sqrtf(a * a + b);
    } else {
        tempC = (resistance - _rtdResistance) / (_rtdResistance * A);
    }
    return tempC;
}

float MAX31865::resistanceToTemperatureC(float resistance) {
    return rtdResistanceToTemperature(resistance);
}
