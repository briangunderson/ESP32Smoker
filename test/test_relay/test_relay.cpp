#include <unity.h>
#include "Arduino.h"
#include "mock_helpers.h"
#include "relay_control.h"
#include "config.h"

static RelayControl* relay;

void setUp(void) {
    mock_reset_all();
    relay = new RelayControl();
    relay->begin();
}

void tearDown(void) {
    delete relay;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void test_begin_sets_all_relays_off(void) {
    // Active-low: OFF = HIGH
    TEST_ASSERT_EQUAL(HIGH, _mock_gpio[PIN_RELAY_AUGER].value);
    TEST_ASSERT_EQUAL(HIGH, _mock_gpio[PIN_RELAY_FAN].value);
    TEST_ASSERT_EQUAL(HIGH, _mock_gpio[PIN_RELAY_IGNITER].value);
}

void test_begin_configures_pins_as_output(void) {
    TEST_ASSERT_EQUAL(OUTPUT, _mock_gpio[PIN_RELAY_AUGER].mode);
    TEST_ASSERT_EQUAL(OUTPUT, _mock_gpio[PIN_RELAY_FAN].mode);
    TEST_ASSERT_EQUAL(OUTPUT, _mock_gpio[PIN_RELAY_IGNITER].mode);
}

// ============================================================================
// BASIC RELAY CONTROL
// ============================================================================

void test_set_fan_on(void) {
    relay->setFan(RELAY_ON);
    TEST_ASSERT_EQUAL(RELAY_ON, relay->getFan());
    // Active-low: ON = LOW
    TEST_ASSERT_EQUAL(LOW, _mock_gpio[PIN_RELAY_FAN].value);
}

void test_set_fan_off(void) {
    relay->setFan(RELAY_ON);
    relay->setFan(RELAY_OFF);
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getFan());
    TEST_ASSERT_EQUAL(HIGH, _mock_gpio[PIN_RELAY_FAN].value);
}

void test_set_igniter_on(void) {
    relay->setIgniter(RELAY_ON);
    TEST_ASSERT_EQUAL(RELAY_ON, relay->getIgniter());
    TEST_ASSERT_EQUAL(LOW, _mock_gpio[PIN_RELAY_IGNITER].value);
}

void test_set_auger_on(void) {
    relay->setAuger(RELAY_ON);
    TEST_ASSERT_EQUAL(RELAY_ON, relay->getAuger());
    TEST_ASSERT_EQUAL(LOW, _mock_gpio[PIN_RELAY_AUGER].value);
}

// ============================================================================
// SAFETY INTERLOCK: setSafeAuger
// ============================================================================

void test_safe_auger_rejected_without_fan(void) {
    // Fan is OFF, setSafeAuger(ON) should be rejected
    relay->setSafeAuger(RELAY_ON);
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getAuger());
    TEST_ASSERT_EQUAL(HIGH, _mock_gpio[PIN_RELAY_AUGER].value);
}

void test_safe_auger_accepted_with_fan(void) {
    // Fan ON, setSafeAuger(ON) should succeed
    relay->setFan(RELAY_ON);
    relay->setSafeAuger(RELAY_ON);
    TEST_ASSERT_EQUAL(RELAY_ON, relay->getAuger());
    TEST_ASSERT_EQUAL(LOW, _mock_gpio[PIN_RELAY_AUGER].value);
}

void test_safe_auger_off_always_allowed(void) {
    // Turning auger OFF should always work, even without fan
    relay->setFan(RELAY_ON);
    relay->setSafeAuger(RELAY_ON);
    relay->setFan(RELAY_OFF);
    relay->setSafeAuger(RELAY_OFF);
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getAuger());
}

void test_safe_auger_rejected_after_fan_turned_off(void) {
    // Fan was on, auger was on, then fan turned off
    // New setSafeAuger(ON) should be rejected
    relay->setFan(RELAY_ON);
    relay->setSafeAuger(RELAY_ON);
    relay->setFan(RELAY_OFF);
    relay->setAuger(RELAY_OFF);
    relay->setSafeAuger(RELAY_ON);
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getAuger());
}

// ============================================================================
// EMERGENCY STOP
// ============================================================================

void test_emergency_stop_turns_all_off(void) {
    relay->setFan(RELAY_ON);
    relay->setAuger(RELAY_ON);
    relay->setIgniter(RELAY_ON);

    relay->emergencyStop();

    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getAuger());
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getFan());
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getIgniter());
}

void test_all_off_turns_all_off(void) {
    relay->setFan(RELAY_ON);
    relay->setIgniter(RELAY_ON);

    relay->allOff();

    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getAuger());
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getFan());
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getIgniter());
}

// ============================================================================
// STATE REPORTING
// ============================================================================

void test_get_states_reports_correctly(void) {
    relay->setFan(RELAY_ON);
    relay->setIgniter(RELAY_ON);

    auto states = relay->getStates();
    TEST_ASSERT_FALSE(states.auger);
    TEST_ASSERT_TRUE(states.fan);
    TEST_ASSERT_TRUE(states.igniter);
}

// ============================================================================
// OUT OF RANGE
// ============================================================================

void test_set_relay_out_of_range(void) {
    // Should be a no-op
    relay->setRelay((RelayID)99, RELAY_ON);
    auto states = relay->getStates();
    TEST_ASSERT_FALSE(states.auger);
    TEST_ASSERT_FALSE(states.fan);
    TEST_ASSERT_FALSE(states.igniter);
}

void test_get_relay_out_of_range(void) {
    TEST_ASSERT_EQUAL(RELAY_OFF, relay->getRelay((RelayID)99));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_begin_sets_all_relays_off);
    RUN_TEST(test_begin_configures_pins_as_output);
    RUN_TEST(test_set_fan_on);
    RUN_TEST(test_set_fan_off);
    RUN_TEST(test_set_igniter_on);
    RUN_TEST(test_set_auger_on);
    RUN_TEST(test_safe_auger_rejected_without_fan);
    RUN_TEST(test_safe_auger_accepted_with_fan);
    RUN_TEST(test_safe_auger_off_always_allowed);
    RUN_TEST(test_safe_auger_rejected_after_fan_turned_off);
    RUN_TEST(test_emergency_stop_turns_all_off);
    RUN_TEST(test_all_off_turns_all_off);
    RUN_TEST(test_get_states_reports_correctly);
    RUN_TEST(test_set_relay_out_of_range);
    RUN_TEST(test_get_relay_out_of_range);

    return UNITY_END();
}
