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
    RUN_TEST(test_wifi_max);
    RUN_TEST(test_wifi_min);
    RUN_TEST(test_wifi_mid);
    RUN_TEST(test_wifi_below_range);
    RUN_TEST(test_wifi_above_range);
    RUN_TEST(test_wifi_at_floor_exact);
    RUN_TEST(test_wifi_at_ceil_exact);
    RUN_TEST(test_wifi_low_quarter);
    RUN_TEST(test_wifi_high_quarter);
    return UNITY_END();
}
