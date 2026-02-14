#include <unity.h>
#include "Arduino.h"
#include "mock_helpers.h"
#include "temperature_control.h"
#include "relay_control.h"
#include "max31865.h"

static MAX31865* sensor;
static RelayControl* relay;
static TemperatureController* ctrl;

// Helper: advance controller to RUNNING state at given temp/setpoint
static void advance_to_running(float setpointF, float currentTempF) {
    ctrl->setTempOverride(currentTempF);
    ctrl->startSmoking(setpointF);
    // Advance past startup (igniter preheat + fan delay = 65s)
    mock_set_millis(70000);
    ctrl->update();
    TEST_ASSERT_EQUAL(STATE_RUNNING, ctrl->getState());
}

void setUp(void) {
    mock_reset_all();
    mock_reset_sensor();
    sensor = new MAX31865(5, 4300.0, 1000.0);
    relay = new RelayControl();
    relay->begin();
    ctrl = new TemperatureController(sensor, relay);
    ctrl->begin();
}

void tearDown(void) {
    delete ctrl;
    delete relay;
    delete sensor;
}

// ============================================================================
// GAIN CALCULATION
// ============================================================================

void test_pid_gains_calculated_correctly(void) {
    // Gains are set in constructor from config.h constants
    // PB=60, Ti=180, Td=45
    // Kp = -1/PB = -1/60 = -0.01667
    // Ki = Kp/Ti = -0.01667/180 = -0.00009259
    // Kd = Kp*Td = -0.01667*45 = -0.75
    advance_to_running(225.0, 225.0);

    auto pid = ctrl->getPIDStatus();
    TEST_ASSERT_FLOAT_WITHIN(0.001, -0.01667, pid.Kp);
    TEST_ASSERT_FLOAT_WITHIN(0.00001, -0.00009259, pid.Ki);
    TEST_ASSERT_FLOAT_WITHIN(0.01, -0.75, pid.Kd);
}

// ============================================================================
// PROPORTIONAL TERM
// ============================================================================

void test_p_term_at_setpoint_is_half(void) {
    // When currentTemp == setpoint, P = Kp * 0 + 0.5 = 0.5
    advance_to_running(225.0, 225.0);

    // Run PID update
    mock_advance_millis(2000);
    ctrl->setTempOverride(225.0);
    ctrl->update();

    auto pid = ctrl->getPIDStatus();
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.5, pid.proportionalTerm);
}

void test_p_term_below_setpoint_greater_than_half(void) {
    // Below setpoint: error is negative, P = Kp * (negative) + 0.5 > 0.5
    // (because Kp is negative, negative * negative = positive)
    advance_to_running(225.0, 220.0);

    mock_advance_millis(2000);
    ctrl->setTempOverride(220.0);
    ctrl->update();

    auto pid = ctrl->getPIDStatus();
    TEST_ASSERT_TRUE(pid.proportionalTerm > 0.5);
    // P = -0.01667 * (220-225) + 0.5 = -0.01667 * -5 + 0.5 = 0.0833 + 0.5 = 0.583
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.583, pid.proportionalTerm);
}

void test_p_term_above_setpoint_less_than_half(void) {
    // Above setpoint: error is positive, P = Kp * (positive) + 0.5 < 0.5
    advance_to_running(225.0, 230.0);

    mock_advance_millis(2000);
    ctrl->setTempOverride(230.0);
    ctrl->update();

    auto pid = ctrl->getPIDStatus();
    TEST_ASSERT_TRUE(pid.proportionalTerm < 0.5);
    // P = -0.01667 * (230-225) + 0.5 = -0.01667 * 5 + 0.5 = -0.0833 + 0.5 = 0.417
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.417, pid.proportionalTerm);
}

// ============================================================================
// OUTPUT CLAMPING
// ============================================================================

void test_output_clamped_to_minimum(void) {
    // Well above setpoint should clamp output to PID_OUTPUT_MIN (0.15)
    advance_to_running(225.0, 300.0);

    mock_advance_millis(2000);
    ctrl->setTempOverride(300.0);
    ctrl->update();

    auto pid = ctrl->getPIDStatus();
    TEST_ASSERT_FLOAT_WITHIN(0.001, PID_OUTPUT_MIN, pid.output);
}

void test_output_clamped_to_maximum(void) {
    // Well below setpoint should clamp output to PID_OUTPUT_MAX (1.0)
    advance_to_running(225.0, 120.0);

    mock_advance_millis(2000);
    ctrl->setTempOverride(120.0);
    ctrl->update();

    auto pid = ctrl->getPIDStatus();
    TEST_ASSERT_FLOAT_WITHIN(0.001, PID_OUTPUT_MAX, pid.output);
}

// ============================================================================
// INTEGRAL ANTI-WINDUP
// ============================================================================

void test_integral_anti_windup(void) {
    // Run many iterations below setpoint to drive integral up
    advance_to_running(225.0, 200.0);

    for (int i = 0; i < 500; i++) {
        mock_advance_millis(2000);
        ctrl->setTempOverride(200.0);
        ctrl->update();
    }

    auto pid = ctrl->getPIDStatus();
    // Integral term should be clamped — output can't exceed PID_OUTPUT_MAX
    TEST_ASSERT_TRUE(pid.output <= PID_OUTPUT_MAX);
    TEST_ASSERT_TRUE(pid.output >= PID_OUTPUT_MIN);
}

// ============================================================================
// DERIVATIVE ON MEASUREMENT
// ============================================================================

void test_derivative_on_measurement_no_kick(void) {
    // When setpoint changes but temperature stays constant,
    // D term should be ~0 (no derivative kick)
    advance_to_running(225.0, 225.0);

    // Steady temperature for a few updates
    for (int i = 0; i < 3; i++) {
        mock_advance_millis(2000);
        ctrl->setTempOverride(225.0);
        ctrl->update();
    }

    // Change setpoint (but temp stays same)
    ctrl->setSetpoint(250.0);
    mock_advance_millis(2000);
    ctrl->setTempOverride(225.0);
    ctrl->update();

    auto pid = ctrl->getPIDStatus();
    // D should be near zero since temperature didn't change
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, pid.derivativeTerm);
}

void test_derivative_responds_to_temp_change(void) {
    advance_to_running(225.0, 225.0);

    // Stable for a bit
    mock_advance_millis(2000);
    ctrl->setTempOverride(225.0);
    ctrl->update();

    // Temperature drops rapidly
    mock_advance_millis(2000);
    ctrl->setTempOverride(220.0);
    ctrl->update();

    auto pid = ctrl->getPIDStatus();
    // D = Kd * (currentTemp - previousTemp) / dt
    // D = -0.75 * (220 - 225) / 2 = -0.75 * -2.5 = 1.875
    TEST_ASSERT_TRUE(pid.derivativeTerm > 0.0);
}

// ============================================================================
// STEADY STATE
// ============================================================================

void test_steady_state_at_setpoint(void) {
    // At setpoint with stable temp, output should be near 0.5
    advance_to_running(225.0, 225.0);

    // Run several PID updates at setpoint
    for (int i = 0; i < 10; i++) {
        mock_advance_millis(2000);
        ctrl->setTempOverride(225.0);
        ctrl->update();
    }

    auto pid = ctrl->getPIDStatus();
    // P = 0.5, I ≈ 0, D ≈ 0, so output ≈ 0.5
    TEST_ASSERT_FLOAT_WITHIN(0.05, 0.5, pid.output);
}

// ============================================================================
// DT GUARD
// ============================================================================

void test_pid_skips_tiny_dt(void) {
    advance_to_running(225.0, 225.0);

    // First normal update
    mock_advance_millis(2000);
    ctrl->update();
    auto pid1 = ctrl->getPIDStatus();

    // Try immediate update (dt ≈ 0) — should be skipped by interval check
    // Actually, the update() function has its own interval check too
    ctrl->update();
    auto pid2 = ctrl->getPIDStatus();

    // Output should not change if dt was too small
    TEST_ASSERT_FLOAT_WITHIN(0.001, pid1.output, pid2.output);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_pid_gains_calculated_correctly);
    RUN_TEST(test_p_term_at_setpoint_is_half);
    RUN_TEST(test_p_term_below_setpoint_greater_than_half);
    RUN_TEST(test_p_term_above_setpoint_less_than_half);
    RUN_TEST(test_output_clamped_to_minimum);
    RUN_TEST(test_output_clamped_to_maximum);
    RUN_TEST(test_integral_anti_windup);
    RUN_TEST(test_derivative_on_measurement_no_kick);
    RUN_TEST(test_derivative_responds_to_temp_change);
    RUN_TEST(test_steady_state_at_setpoint);
    RUN_TEST(test_pid_skips_tiny_dt);

    return UNITY_END();
}
