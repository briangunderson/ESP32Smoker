#ifndef MOCK_HELPERS_H
#define MOCK_HELPERS_H

// Sensor mock control
extern float _mock_sensor_temp_c;
extern uint8_t _mock_sensor_fault;

void mock_set_sensor_temp_c(float tempC);
void mock_set_sensor_fault(uint8_t fault);
void mock_reset_sensor(void);

#endif // MOCK_HELPERS_H
