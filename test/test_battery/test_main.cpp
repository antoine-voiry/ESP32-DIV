#include <unity.h>
#include "battery_logic.h"

void setUp()    {}
void tearDown() {}

// computeVoltage — ADC mV * 2 (voltage divider)
void test_voltage_zero()    { TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f,  computeVoltage(0)); }
void test_voltage_mid()     { TEST_ASSERT_FLOAT_WITHIN(0.01f,  3.30f, computeVoltage(1650)); }
void test_voltage_full()    { TEST_ASSERT_FLOAT_WITHIN(0.01f,  4.20f, computeVoltage(2100)); }
void test_voltage_floor()   { TEST_ASSERT_FLOAT_WITHIN(0.01f,  3.00f, computeVoltage(1500)); }

// computeBatteryPercent — clamps [0, 100], maps 3.0V–4.2V
void test_percent_full()         { TEST_ASSERT_EQUAL_INT(100, computeBatteryPercent(4.2f)); }
void test_percent_empty()        { TEST_ASSERT_EQUAL_INT(0,   computeBatteryPercent(3.0f)); }
void test_percent_mid()          { TEST_ASSERT_EQUAL_INT(50,  computeBatteryPercent(3.6f)); }
void test_percent_below_range()  { TEST_ASSERT_EQUAL_INT(0,   computeBatteryPercent(2.5f)); }
void test_percent_above_range()  { TEST_ASSERT_EQUAL_INT(100, computeBatteryPercent(5.0f)); }
void test_percent_at_floor_exact() { TEST_ASSERT_EQUAL_INT(0, computeBatteryPercent(3.0f)); }
void test_percent_at_ceil_exact()  { TEST_ASSERT_EQUAL_INT(100, computeBatteryPercent(4.2f)); }
void test_percent_low_quarter()  { TEST_ASSERT_EQUAL_INT(25,  computeBatteryPercent(3.3f)); }
void test_percent_high_quarter() { TEST_ASSERT_EQUAL_INT(75,  computeBatteryPercent(3.9f)); }

// computeTempStatus — ESP32 die temperature thresholds (0=OK, 1=WARN, 2=ERR)
// Regression guard: thresholds were previously 50/55°C (ambient), causing ESP32
// idle die temp (~50°C) to always show orange. Recalibrated to 65/80°C for die temp.
void test_temp_idle_ok()       { TEST_ASSERT_EQUAL_INT(0, computeTempStatus(53.0f)); }
void test_temp_warm_ok()       { TEST_ASSERT_EQUAL_INT(0, computeTempStatus(64.9f)); }
void test_temp_warn_boundary() { TEST_ASSERT_EQUAL_INT(1, computeTempStatus(65.0f)); }
void test_temp_warn_mid()      { TEST_ASSERT_EQUAL_INT(1, computeTempStatus(72.0f)); }
void test_temp_err_boundary()  { TEST_ASSERT_EQUAL_INT(2, computeTempStatus(80.0f)); }
void test_temp_err_high()      { TEST_ASSERT_EQUAL_INT(2, computeTempStatus(95.0f)); }
void test_temp_cold()          { TEST_ASSERT_EQUAL_INT(0, computeTempStatus(20.0f)); }

// computeWifiStrength — clamps [0, 100], maps -100 dBm to -30 dBm
void test_wifi_max()          { TEST_ASSERT_EQUAL_INT(100, computeWifiStrength(-30)); }
void test_wifi_min()          { TEST_ASSERT_EQUAL_INT(0,   computeWifiStrength(-100)); }
void test_wifi_mid()          { TEST_ASSERT_EQUAL_INT(50,  computeWifiStrength(-65)); }
void test_wifi_below_range()  { TEST_ASSERT_EQUAL_INT(0,   computeWifiStrength(-120)); }
void test_wifi_above_range()  { TEST_ASSERT_EQUAL_INT(100, computeWifiStrength(-10)); }
void test_wifi_at_floor_exact() { TEST_ASSERT_EQUAL_INT(0,   computeWifiStrength(-100)); }
void test_wifi_at_ceil_exact()  { TEST_ASSERT_EQUAL_INT(100, computeWifiStrength(-30)); }
void test_wifi_low_quarter()  { TEST_ASSERT_EQUAL_INT(25,  computeWifiStrength(-82)); }
void test_wifi_high_quarter() { TEST_ASSERT_EQUAL_INT(75,  computeWifiStrength(-47)); }

// ── computeWifiDisplayStrength adversarial tests ──────────────────────────────

// Failure mode: connected branch not taken — returns 0 for any connected state
void test_wifi_display_connected_good_signal() {
    // computeWifiStrength(-50) = ((-50)-(-100))*100/70 = 5000/70 = 71
    TEST_ASSERT_EQUAL_INT(71, computeWifiDisplayStrength(true, false, -50));
}

// Failure mode: RSSI floor not applied — returns positive value at -100 dBm
void test_wifi_display_connected_floor_signal() {
    // computeWifiStrength(-100) = 0
    TEST_ASSERT_EQUAL_INT(0, computeWifiDisplayStrength(true, false, -100));
}

// Failure mode: AP branch not taken — returns 0 instead of 100 in AP mode
void test_wifi_display_ap_mode_returns_full() {
    TEST_ASSERT_EQUAL_INT(100, computeWifiDisplayStrength(false, true, -80));
}

// Failure mode: default branch does not return 0 (returns garbage or previous stack value)
void test_wifi_display_neither_returns_zero() {
    TEST_ASSERT_EQUAL_INT(0, computeWifiDisplayStrength(false, false, -80));
}

// Failure mode: isAp check runs before isConnected check — returns 100 instead of RSSI value
void test_wifi_display_connected_wins_over_ap() {
    TEST_ASSERT_EQUAL_INT(71, computeWifiDisplayStrength(true, true, -50));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_voltage_zero);
    RUN_TEST(test_voltage_mid);
    RUN_TEST(test_voltage_full);
    RUN_TEST(test_voltage_floor);
    RUN_TEST(test_percent_full);
    RUN_TEST(test_percent_empty);
    RUN_TEST(test_percent_mid);
    RUN_TEST(test_percent_below_range);
    RUN_TEST(test_percent_above_range);
    RUN_TEST(test_percent_at_floor_exact);
    RUN_TEST(test_percent_at_ceil_exact);
    RUN_TEST(test_percent_low_quarter);
    RUN_TEST(test_percent_high_quarter);
    RUN_TEST(test_temp_idle_ok);
    RUN_TEST(test_temp_warm_ok);
    RUN_TEST(test_temp_warn_boundary);
    RUN_TEST(test_temp_warn_mid);
    RUN_TEST(test_temp_err_boundary);
    RUN_TEST(test_temp_err_high);
    RUN_TEST(test_temp_cold);
    RUN_TEST(test_wifi_max);
    RUN_TEST(test_wifi_min);
    RUN_TEST(test_wifi_mid);
    RUN_TEST(test_wifi_below_range);
    RUN_TEST(test_wifi_above_range);
    RUN_TEST(test_wifi_at_floor_exact);
    RUN_TEST(test_wifi_at_ceil_exact);
    RUN_TEST(test_wifi_low_quarter);
    RUN_TEST(test_wifi_high_quarter);
    RUN_TEST(test_wifi_display_connected_good_signal);
    RUN_TEST(test_wifi_display_connected_floor_signal);
    RUN_TEST(test_wifi_display_ap_mode_returns_full);
    RUN_TEST(test_wifi_display_neither_returns_zero);
    RUN_TEST(test_wifi_display_connected_wins_over_ap);
    return UNITY_END();
}
