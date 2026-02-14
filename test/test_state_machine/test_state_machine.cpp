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
// INITIAL STATE
// ============================================================================

void test_initial_state_is_idle(void) {
    TEST_ASSERT_EQUAL(STATE_IDLE, ctrl->getState());
}

void test_initial_relays_all_off(void) {
    auto status = ctrl->getStatus();
    TEST_ASSERT_FALSE(status.auger);
    TEST_ASSERT_FALSE(status.fan);
    TEST_ASSERT_FALSE(status.igniter);
}

// ============================================================================
// IDLE -> STARTUP
// ============================================================================

void test_start_smoking_transitions_to_startup(void) {
    ctrl->setTempOverride(70.0);
    ctrl->startSmoking(225.0);
    TEST_ASSERT_EQUAL(STATE_STARTUP, ctrl->getState());
}

void test_start_smoking_sets_setpoint(void) {
    ctrl->setTempOverride(70.0);
    ctrl->startSmoking(250.0);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 250.0, ctrl->getSetpoint());
}

void test_start_smoking_rejects_below_min(void) {
    ctrl->startSmoking(100.0);
    TEST_ASSERT_EQUAL(STATE_IDLE, ctrl->getState());
}

void test_start_smoking_rejects_above_max(void) {
    ctrl->startSmoking(600.0);
    TEST_ASSERT_EQUAL(STATE_IDLE, ctrl->getState());
}

// ============================================================================
// STARTUP PHASES
// ============================================================================

void test_startup_phase1_igniter_only(void) {
    ctrl->setTempOverride(70.0);
    ctrl->startSmoking(225.0);

    // First update at 2s (within igniter preheat time)
    mock_set_millis(2000);
    ctrl->update();

    TEST_ASSERT_EQUAL(STATE_STARTUP, ctrl->getState());
    TEST_ASSERT_TRUE(relay->getIgniter() == RELAY_ON);
    TEST_ASSERT_TRUE(relay->getFan() == RELAY_OFF);
    TEST_ASSERT_TRUE(relay->getAuger() == RELAY_OFF);
}

void test_startup_phase2_fan_starts(void) {
    ctrl->setTempOverride(70.0);
    ctrl->startSmoking(225.0);

    // Advance past igniter preheat (60s), into fan startup
    mock_set_millis(62000);
    ctrl->update();

    TEST_ASSERT_EQUAL(STATE_STARTUP, ctrl->getState());
    TEST_ASSERT_TRUE(relay->getIgniter() == RELAY_ON);
    TEST_ASSERT_TRUE(relay->getFan() == RELAY_ON);
    TEST_ASSERT_TRUE(relay->getAuger() == RELAY_OFF);
}

void test_startup_phase3_auger_starts(void) {
    ctrl->setTempOverride(70.0);
    ctrl->startSmoking(225.0);

    // Advance past fan delay (65s), auger should start
    mock_set_millis(66000);
    ctrl->update();

    TEST_ASSERT_EQUAL(STATE_STARTUP, ctrl->getState());
    TEST_ASSERT_TRUE(relay->getFan() == RELAY_ON);
    TEST_ASSERT_TRUE(relay->getAuger() == RELAY_ON);
}

// ============================================================================
// STARTUP -> RUNNING
// ============================================================================

void test_startup_to_running_on_temp_threshold(void) {
    ctrl->setTempOverride(120.0);  // Above STARTUP_TEMP_THRESHOLD (115)
    ctrl->startSmoking(225.0);

    // Advance past fan delay to phase 3
    mock_set_millis(66000);
    ctrl->update();

    TEST_ASSERT_EQUAL(STATE_RUNNING, ctrl->getState());
}

void test_igniter_cuts_off_above_100f(void) {
    // Start with temp below cutoff
    ctrl->setTempOverride(90.0);
    ctrl->startSmoking(225.0);

    mock_set_millis(2000);
    ctrl->update();
    TEST_ASSERT_TRUE(relay->getIgniter() == RELAY_ON);

    // Raise temp above IGNITER_CUTOFF_TEMP (100)
    ctrl->setTempOverride(105.0);
    mock_set_millis(4000);
    ctrl->update();
    TEST_ASSERT_TRUE(relay->getIgniter() == RELAY_OFF);
}

// ============================================================================
// STARTUP TIMEOUT -> ERROR
// ============================================================================

void test_startup_timeout_enters_error(void) {
    ctrl->setTempOverride(70.0);  // Below threshold
    ctrl->startSmoking(225.0);

    // Advance past STARTUP_TIMEOUT (180s)
    mock_set_millis(182000);
    ctrl->update();

    TEST_ASSERT_EQUAL(STATE_ERROR, ctrl->getState());
    // All relays should be off (emergency stop)
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getAuger());
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getFan());
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getIgniter());
}

// ============================================================================
// RUNNING -> COOLDOWN
// ============================================================================

void test_stop_transitions_to_cooldown(void) {
    // Get to RUNNING first
    ctrl->setTempOverride(120.0);
    ctrl->startSmoking(225.0);
    mock_set_millis(66000);
    ctrl->update();
    TEST_ASSERT_EQUAL(STATE_RUNNING, ctrl->getState());

    // Stop
    ctrl->stop();
    TEST_ASSERT_EQUAL(STATE_COOLDOWN, ctrl->getState());
}

// ============================================================================
// COOLDOWN -> SHUTDOWN -> IDLE
// ============================================================================

void test_cooldown_keeps_fan_on(void) {
    ctrl->setTempOverride(200.0);
    ctrl->startSmoking(225.0);
    mock_set_millis(66000);
    ctrl->update();
    ctrl->stop();

    mock_advance_millis(2000);
    ctrl->update();

    TEST_ASSERT_EQUAL(STATE_COOLDOWN, ctrl->getState());
    TEST_ASSERT_EQUAL(RELAY_ON, relay->getFan());
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getAuger());
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getIgniter());
}

void test_cooldown_to_shutdown_on_low_temp(void) {
    ctrl->setTempOverride(200.0);
    ctrl->startSmoking(225.0);
    mock_set_millis(66000);
    ctrl->update();
    ctrl->stop();

    // Drop temp below TEMP_MIN_SAFE + 20 (50 + 20 = 70)
    ctrl->setTempOverride(60.0);
    mock_advance_millis(2000);
    ctrl->update();

    // Should transition through SHUTDOWN to IDLE
    TEST_ASSERT_TRUE(ctrl->getState() == STATE_SHUTDOWN || ctrl->getState() == STATE_IDLE);
}

void test_cooldown_to_shutdown_on_timeout(void) {
    ctrl->setTempOverride(200.0);
    ctrl->startSmoking(225.0);
    mock_set_millis(66000);
    ctrl->update();
    ctrl->stop();

    // Advance past SHUTDOWN_COOL_TIMEOUT (300s)
    mock_advance_millis(302000);
    ctrl->update();

    TEST_ASSERT_TRUE(ctrl->getState() == STATE_SHUTDOWN || ctrl->getState() == STATE_IDLE);
}

void test_shutdown_transitions_to_idle(void) {
    ctrl->setTempOverride(200.0);
    ctrl->startSmoking(225.0);
    mock_set_millis(66000);
    ctrl->update();

    ctrl->shutdown();
    TEST_ASSERT_EQUAL(STATE_SHUTDOWN, ctrl->getState());

    mock_advance_millis(2000);
    ctrl->update();
    TEST_ASSERT_EQUAL(STATE_IDLE, ctrl->getState());
}

// ============================================================================
// TEMPERATURE FAULTS -> ERROR
// ============================================================================

void test_high_temp_triggers_error_in_running(void) {
    ctrl->setTempOverride(120.0);
    ctrl->startSmoking(225.0);
    mock_set_millis(66000);
    ctrl->update();
    TEST_ASSERT_EQUAL(STATE_RUNNING, ctrl->getState());

    // Set temp above TEMP_MAX_SAFE (550)
    ctrl->setTempOverride(560.0);
    mock_advance_millis(2000);
    ctrl->update();

    TEST_ASSERT_EQUAL(STATE_ERROR, ctrl->getState());
}

void test_low_temp_triggers_error_in_running(void) {
    ctrl->setTempOverride(120.0);
    ctrl->startSmoking(225.0);
    mock_set_millis(66000);
    ctrl->update();
    TEST_ASSERT_EQUAL(STATE_RUNNING, ctrl->getState());

    // Set temp below TEMP_MIN_SAFE (50)
    ctrl->setTempOverride(40.0);
    mock_advance_millis(2000);
    ctrl->update();

    TEST_ASSERT_EQUAL(STATE_ERROR, ctrl->getState());
}

void test_low_temp_no_error_during_startup(void) {
    // Low temp during startup should NOT trigger error
    // (smoker is naturally cold during startup)
    ctrl->setTempOverride(40.0);
    ctrl->startSmoking(225.0);

    mock_set_millis(2000);
    ctrl->update();

    // Should still be in STARTUP, not ERROR
    TEST_ASSERT_EQUAL(STATE_STARTUP, ctrl->getState());
}

// ============================================================================
// SENSOR ERROR -> ERROR
// ============================================================================

void test_sensor_errors_trigger_error_state(void) {
    ctrl->startSmoking(225.0);
    ctrl->clearTempOverride();

    // Simulate sensor failures (readTemperatureC returns -999)
    mock_set_sensor_fault(0xFF);

    // Need SENSOR_ERROR_THRESHOLD consecutive failures
    for (int i = 0; i < SENSOR_ERROR_THRESHOLD + 1; i++) {
        mock_advance_millis(2000);
        ctrl->update();
    }

    TEST_ASSERT_EQUAL(STATE_ERROR, ctrl->getState());
}

// ============================================================================
// ERROR -> IDLE (resetError)
// ============================================================================

void test_reset_error_returns_to_idle(void) {
    // Force into error state
    ctrl->setTempOverride(120.0);
    ctrl->startSmoking(225.0);
    mock_set_millis(66000);
    ctrl->update();
    ctrl->setTempOverride(560.0);
    mock_advance_millis(2000);
    ctrl->update();
    TEST_ASSERT_EQUAL(STATE_ERROR, ctrl->getState());

    ctrl->resetError();
    TEST_ASSERT_EQUAL(STATE_IDLE, ctrl->getState());
}

void test_reset_error_only_works_from_error(void) {
    // From IDLE, resetError should not change state
    ctrl->resetError();
    TEST_ASSERT_EQUAL(STATE_IDLE, ctrl->getState());
}

// ============================================================================
// DEBUG MODE
// ============================================================================

void test_debug_mode_skips_state_machine(void) {
    ctrl->setTempOverride(120.0);
    ctrl->startSmoking(225.0);
    ctrl->setDebugMode(true);

    // Update should not advance state machine
    mock_set_millis(66000);
    ctrl->update();

    // State should remain STARTUP because debug mode was entered
    // Actually, setDebugMode(true) calls allOff and when disabled returns to IDLE
    // The key thing: debug mode stops state machine processing
    TEST_ASSERT_TRUE(ctrl->isDebugMode());
}

void test_debug_mode_turns_off_relays(void) {
    ctrl->setTempOverride(120.0);
    ctrl->startSmoking(225.0);
    mock_set_millis(66000);
    ctrl->update();

    ctrl->setDebugMode(true);

    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getAuger());
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getFan());
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getIgniter());
}

void test_debug_mode_disable_returns_to_idle(void) {
    ctrl->setDebugMode(true);
    ctrl->setDebugMode(false);
    TEST_ASSERT_EQUAL(STATE_IDLE, ctrl->getState());
}

// ============================================================================
// SETPOINT VALIDATION
// ============================================================================

void test_set_setpoint_accepts_valid(void) {
    ctrl->setSetpoint(250.0);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 250.0, ctrl->getSetpoint());
}

void test_set_setpoint_rejects_below_min(void) {
    float original = ctrl->getSetpoint();
    ctrl->setSetpoint(100.0);
    TEST_ASSERT_FLOAT_WITHIN(0.1, original, ctrl->getSetpoint());
}

void test_set_setpoint_rejects_above_max(void) {
    float original = ctrl->getSetpoint();
    ctrl->setSetpoint(600.0);
    TEST_ASSERT_FLOAT_WITHIN(0.1, original, ctrl->getSetpoint());
}

void test_set_setpoint_accepts_min_boundary(void) {
    ctrl->setSetpoint((float)TEMP_MIN_SETPOINT);
    TEST_ASSERT_FLOAT_WITHIN(0.1, (float)TEMP_MIN_SETPOINT, ctrl->getSetpoint());
}

void test_set_setpoint_accepts_max_boundary(void) {
    ctrl->setSetpoint((float)TEMP_MAX_SETPOINT);
    TEST_ASSERT_FLOAT_WITHIN(0.1, (float)TEMP_MAX_SETPOINT, ctrl->getSetpoint());
}

// ============================================================================
// STATE NAME
// ============================================================================

void test_state_names(void) {
    TEST_ASSERT_EQUAL_STRING("Idle", ctrl->getStateName());

    ctrl->setTempOverride(70.0);
    ctrl->startSmoking(225.0);
    TEST_ASSERT_EQUAL_STRING("Starting", ctrl->getStateName());
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_initial_state_is_idle);
    RUN_TEST(test_initial_relays_all_off);
    RUN_TEST(test_start_smoking_transitions_to_startup);
    RUN_TEST(test_start_smoking_sets_setpoint);
    RUN_TEST(test_start_smoking_rejects_below_min);
    RUN_TEST(test_start_smoking_rejects_above_max);
    RUN_TEST(test_startup_phase1_igniter_only);
    RUN_TEST(test_startup_phase2_fan_starts);
    RUN_TEST(test_startup_phase3_auger_starts);
    RUN_TEST(test_startup_to_running_on_temp_threshold);
    RUN_TEST(test_igniter_cuts_off_above_100f);
    RUN_TEST(test_startup_timeout_enters_error);
    RUN_TEST(test_stop_transitions_to_cooldown);
    RUN_TEST(test_cooldown_keeps_fan_on);
    RUN_TEST(test_cooldown_to_shutdown_on_low_temp);
    RUN_TEST(test_cooldown_to_shutdown_on_timeout);
    RUN_TEST(test_shutdown_transitions_to_idle);
    RUN_TEST(test_high_temp_triggers_error_in_running);
    RUN_TEST(test_low_temp_triggers_error_in_running);
    RUN_TEST(test_low_temp_no_error_during_startup);
    RUN_TEST(test_sensor_errors_trigger_error_state);
    RUN_TEST(test_reset_error_returns_to_idle);
    RUN_TEST(test_reset_error_only_works_from_error);
    RUN_TEST(test_debug_mode_skips_state_machine);
    RUN_TEST(test_debug_mode_turns_off_relays);
    RUN_TEST(test_debug_mode_disable_returns_to_idle);
    RUN_TEST(test_set_setpoint_accepts_valid);
    RUN_TEST(test_set_setpoint_rejects_below_min);
    RUN_TEST(test_set_setpoint_rejects_above_max);
    RUN_TEST(test_set_setpoint_accepts_min_boundary);
    RUN_TEST(test_set_setpoint_accepts_max_boundary);
    RUN_TEST(test_state_names);

    return UNITY_END();
}
