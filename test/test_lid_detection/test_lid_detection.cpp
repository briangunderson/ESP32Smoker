#include <unity.h>
#include "Arduino.h"
#include "mock_helpers.h"
#include "temperature_control.h"
#include "relay_control.h"
#include "max31865.h"
#include "config.h"

static MAX31865* sensor;
static RelayControl* relay;
static TemperatureController* ctrl;

// Helper: advance to RUNNING state
static void get_to_running(float tempF = 225.0f) {
    ctrl->setTempOverride(120.0f);
    ctrl->startSmoking(225.0f);
    mock_set_millis(66000);
    ctrl->update();
    TEST_ASSERT_EQUAL(STATE_RUNNING, ctrl->getState());

    // Run a few stable updates at target temp to establish baseline
    for (int i = 0; i < 5; i++) {
        mock_advance_millis(2000);
        ctrl->setTempOverride(tempF);
        ctrl->update();
    }
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
// LID OPEN DETECTION
// ============================================================================

void test_rapid_temp_drop_detects_lid_open(void) {
    get_to_running(225.0f);

    // Simulate rapid temp drop: 25F drop over 2s interval
    // dT/dt = -25 / 2 = -12.5 F/s (below LID_OPEN_DERIVATIVE_THRESHOLD of -10.0)
    ctrl->setTempOverride(200.0f);  // 25F drop from 225
    mock_advance_millis(2000);
    ctrl->update();

    TEST_ASSERT_TRUE(ctrl->isLidOpen());
}

void test_gradual_temp_drop_no_lid_detection(void) {
    get_to_running(225.0f);

    // Gradual drop: -1F over 2s = -0.5 F/s (above threshold)
    ctrl->setTempOverride(224.0f);
    mock_advance_millis(2000);
    ctrl->update();

    TEST_ASSERT_FALSE(ctrl->isLidOpen());
}

void test_lid_only_detected_in_running_state(void) {
    // In IDLE, lid detection should not activate
    ctrl->setTempOverride(225.0f);
    mock_set_millis(2000);
    ctrl->update();

    TEST_ASSERT_FALSE(ctrl->isLidOpen());
}

// ============================================================================
// LID CLOSE RECOVERY
// ============================================================================

void test_lid_closes_after_stable_period(void) {
    get_to_running(225.0f);

    // Open the lid (rapid 25F drop = -12.5 F/s, below -10.0 threshold)
    ctrl->setTempOverride(200.0f);
    mock_advance_millis(2000);
    ctrl->update();
    TEST_ASSERT_TRUE(ctrl->isLidOpen());

    // Temperature stabilizes (small changes, no rapid drop)
    for (unsigned long t = 0; t < (uint32_t)LID_CLOSE_RECOVERY_TIME + 4000; t += 2000) {
        mock_advance_millis(2000);
        ctrl->setTempOverride(200.0f);  // Stable temp
        ctrl->update();
    }

    TEST_ASSERT_FALSE(ctrl->isLidOpen());
}

void test_lid_stays_open_with_continued_drop(void) {
    get_to_running(225.0f);

    // Open the lid (rapid 25F drop = -12.5 F/s, below -10.0 threshold)
    ctrl->setTempOverride(200.0f);
    mock_advance_millis(2000);
    ctrl->update();
    TEST_ASSERT_TRUE(ctrl->isLidOpen());

    // Continue dropping fast — lid should stay open, stable timer resets
    float temp = 200.0f;
    for (int i = 0; i < 5; i++) {
        temp -= 22.0f;  // -11 F/s, still below -10.0 threshold
        ctrl->setTempOverride(temp);
        mock_advance_millis(2000);
        ctrl->update();
    }

    TEST_ASSERT_TRUE(ctrl->isLidOpen());
}

// ============================================================================
// PID INTEGRAL FREEZE DURING LID OPEN
// ============================================================================

void test_integral_frozen_during_lid_open(void) {
    get_to_running(225.0f);

    // Record PID state before lid open
    auto pid_before = ctrl->getPIDStatus();
    float integral_before = pid_before.integralTerm;

    // Open the lid (rapid 25F drop = -12.5 F/s, below -10.0 threshold)
    ctrl->setTempOverride(200.0f);
    mock_advance_millis(2000);
    ctrl->update();
    TEST_ASSERT_TRUE(ctrl->isLidOpen());

    // Run several updates with lid open — integral should not accumulate
    float integral_after_open = ctrl->getPIDStatus().integralTerm;

    for (int i = 0; i < 10; i++) {
        mock_advance_millis(2000);
        ctrl->setTempOverride(200.0f);  // Well below setpoint
        ctrl->update();
    }

    auto pid_during = ctrl->getPIDStatus();
    // Integral should be frozen at the value from when lid opened
    TEST_ASSERT_FLOAT_WITHIN(0.001, integral_after_open, pid_during.integralTerm);
}

// ============================================================================
// LID OPEN DURATION
// ============================================================================

void test_lid_open_duration_reports_correctly(void) {
    get_to_running(225.0f);

    // Lid not open — duration should be 0
    TEST_ASSERT_EQUAL(0, ctrl->getLidOpenDuration());

    // Open lid (rapid 25F drop = -12.5 F/s, below -10.0 threshold)
    ctrl->setTempOverride(200.0f);
    mock_advance_millis(2000);
    ctrl->update();
    TEST_ASSERT_TRUE(ctrl->isLidOpen());

    // Wait 10 seconds
    mock_advance_millis(10000);
    ctrl->update();

    // Duration should be approximately 12s (2s for detection + 10s wait)
    uint32_t duration = ctrl->getLidOpenDuration();
    TEST_ASSERT_TRUE(duration >= 10);
    TEST_ASSERT_TRUE(duration <= 15);
}

void test_lid_closed_duration_is_zero(void) {
    get_to_running(225.0f);
    TEST_ASSERT_EQUAL(0, ctrl->getLidOpenDuration());
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_rapid_temp_drop_detects_lid_open);
    RUN_TEST(test_gradual_temp_drop_no_lid_detection);
    RUN_TEST(test_lid_only_detected_in_running_state);
    RUN_TEST(test_lid_closes_after_stable_period);
    RUN_TEST(test_lid_stays_open_with_continued_drop);
    RUN_TEST(test_integral_frozen_during_lid_open);
    RUN_TEST(test_lid_open_duration_reports_correctly);
    RUN_TEST(test_lid_closed_duration_is_zero);

    return UNITY_END();
}
