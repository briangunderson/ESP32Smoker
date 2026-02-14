#include <unity.h>
#include <cmath>
#include "Arduino.h"
#include "mock_helpers.h"

// Standalone Callendar-Van Dusen implementation for testing
// Uses the quadratic formula: R = R0 * (1 + A*T + B*T^2)
// Solving for T: T = [-A + sqrt(A^2 - 4*B*(1 - R/R0))] / (2*B)
static float callendar_van_dusen(float resistance, float rtdR0) {
    const float A = 3.9083e-3f;
    const float B = -5.775e-7f;

    float z = 1.0f - resistance / rtdR0;
    float discriminant = A * A - 4.0f * B * z;
    if (discriminant < 0.0f) return -999.0f;

    return (-A + sqrtf(discriminant)) / (2.0f * B);
}

static float c_to_f(float tempC) {
    return tempC * 9.0f / 5.0f + 32.0f;
}

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// PT1000 CALLENDAR-VAN DUSEN CONVERSION
// ============================================================================

void test_pt1000_at_0c(void) {
    // PT1000: 1000 ohms at 0 degrees C
    float tempC = callendar_van_dusen(1000.0f, 1000.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 0.0, tempC);
}

void test_pt1000_at_25c(void) {
    // PT1000 at 25C: ~1097.9 ohms
    float tempC = callendar_van_dusen(1097.9f, 1000.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.5, 25.0, tempC);
}

void test_pt1000_at_50c(void) {
    // PT1000 at 50C: ~1194.0 ohms
    float tempC = callendar_van_dusen(1194.0f, 1000.0f);
    TEST_ASSERT_FLOAT_WITHIN(1.0, 50.0, tempC);
}

void test_pt1000_at_100c(void) {
    // PT1000 at 100C: ~1385.1 ohms
    float tempC = callendar_van_dusen(1385.1f, 1000.0f);
    TEST_ASSERT_FLOAT_WITHIN(1.0, 100.0, tempC);
}

void test_pt1000_at_150c(void) {
    // PT1000 at 150C: ~1573.1 ohms
    float tempC = callendar_van_dusen(1573.1f, 1000.0f);
    TEST_ASSERT_FLOAT_WITHIN(2.0, 150.0, tempC);
}

void test_pt1000_negative_temp(void) {
    // PT1000 at -20C: ~922 ohms (uses quadratic branch)
    float tempC = callendar_van_dusen(922.0f, 1000.0f);
    TEST_ASSERT_TRUE(tempC < 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(5.0, -20.0, tempC);
}

// ============================================================================
// C-TO-F CONVERSION
// ============================================================================

void test_c_to_f_freezing(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.1, 32.0, c_to_f(0.0));
}

void test_c_to_f_boiling(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.1, 212.0, c_to_f(100.0));
}

void test_c_to_f_smoking_temp(void) {
    // 107.2C = 225F (typical smoking temperature)
    TEST_ASSERT_FLOAT_WITHIN(0.5, 225.0, c_to_f(107.2));
}

// ============================================================================
// MONOTONICITY
// ============================================================================

void test_temperature_increases_with_resistance(void) {
    float prev = -999.0f;
    for (float r = 900.0f; r <= 1600.0f; r += 50.0f) {
        float t = callendar_van_dusen(r, 1000.0f);
        TEST_ASSERT_TRUE_MESSAGE(t > prev, "Temperature should increase with resistance");
        prev = t;
    }
}

// ============================================================================
// SMOKING RANGE
// ============================================================================

void test_smoking_range_150f_to_350f(void) {
    // Typical smoking range: 150F (65.6C) to 350F (176.7C)
    // Verify conversions are in the right ballpark
    float t65 = callendar_van_dusen(1253.0f, 1000.0f);  // ~65C
    float f65 = c_to_f(t65);
    TEST_ASSERT_TRUE(f65 > 140.0f && f65 < 160.0f);

    float t177 = callendar_van_dusen(1680.0f, 1000.0f);  // ~177C
    float f177 = c_to_f(t177);
    TEST_ASSERT_TRUE(f177 > 340.0f && f177 < 360.0f);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_pt1000_at_0c);
    RUN_TEST(test_pt1000_at_25c);
    RUN_TEST(test_pt1000_at_50c);
    RUN_TEST(test_pt1000_at_100c);
    RUN_TEST(test_pt1000_at_150c);
    RUN_TEST(test_pt1000_negative_temp);
    RUN_TEST(test_c_to_f_freezing);
    RUN_TEST(test_c_to_f_boiling);
    RUN_TEST(test_c_to_f_smoking_temp);
    RUN_TEST(test_temperature_increases_with_resistance);
    RUN_TEST(test_smoking_range_150f_to_350f);

    return UNITY_END();
}
