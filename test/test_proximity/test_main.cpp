#include <unity.h>
#include "proximity_logic.h"

void setUp()    {}
void tearDown() {}

// classifyRssi — 4-way branch: NONE / LOW / MODERATE / CRITICAL
void test_classify_none_deep()       { TEST_ASSERT_EQUAL_INT(ZONE_NONE,     classifyRssi(-100)); }
void test_classify_none_boundary()   { TEST_ASSERT_EQUAL_INT(ZONE_NONE,     classifyRssi(-91)); }
void test_classify_low_at_90()       { TEST_ASSERT_EQUAL_INT(ZONE_LOW,      classifyRssi(-90)); }
void test_classify_low_mid()         { TEST_ASSERT_EQUAL_INT(ZONE_LOW,      classifyRssi(-80)); }
void test_classify_low_boundary()    { TEST_ASSERT_EQUAL_INT(ZONE_LOW,      classifyRssi(-71)); }
void test_classify_moderate_at_70()  { TEST_ASSERT_EQUAL_INT(ZONE_MODERATE, classifyRssi(-70)); }
void test_classify_moderate_mid()    { TEST_ASSERT_EQUAL_INT(ZONE_MODERATE, classifyRssi(-60)); }
void test_classify_moderate_bound()  { TEST_ASSERT_EQUAL_INT(ZONE_MODERATE, classifyRssi(-51)); }
void test_classify_critical_at_50()  { TEST_ASSERT_EQUAL_INT(ZONE_CRITICAL, classifyRssi(-50)); }
void test_classify_critical_strong() { TEST_ASSERT_EQUAL_INT(ZONE_CRITICAL, classifyRssi(-20)); }

// rssiToBarWidth — clamps [-100, -30] dBm onto [0, displayWidth] px
void test_bar_at_floor_exact()   { TEST_ASSERT_EQUAL_UINT16(0,   rssiToBarWidth(-100, 100)); }
void test_bar_below_floor()      { TEST_ASSERT_EQUAL_UINT16(0,   rssiToBarWidth(-120, 100)); }
void test_bar_at_ceil_exact()    { TEST_ASSERT_EQUAL_UINT16(100, rssiToBarWidth(-30, 100)); }
void test_bar_above_ceil()       { TEST_ASSERT_EQUAL_UINT16(100, rssiToBarWidth(-10, 100)); }
void test_bar_mid()              { TEST_ASSERT_EQUAL_UINT16(50,  rssiToBarWidth(-65, 100)); }
void test_bar_zero_width()       { TEST_ASSERT_EQUAL_UINT16(0,   rssiToBarWidth(-65, 0)); }

// ewmaRssi — straight-line fixed-point smoothing (no branches; correctness only)
void test_ewma_first_sample_exact() {
    volatile int16_t smoothed = 0;
    int8_t out = ewmaRssi(smoothed, -16);  // (5*-16 + 11*0) >> 4 == -5 exactly
    TEST_ASSERT_EQUAL_INT8(-5, out);
    TEST_ASSERT_EQUAL_INT16(-5, smoothed);
}

void test_ewma_tracks_rising_signal() {
    volatile int16_t smoothed = -100;
    int8_t prev = static_cast<int8_t>(smoothed);
    for (int i = 0; i < 20; ++i) {
        int8_t out = ewmaRssi(smoothed, -30);
        TEST_ASSERT_TRUE(out >= prev);  // monotonic approach toward stronger signal
        prev = out;
    }
    TEST_ASSERT_TRUE(prev > -100);
}

void test_ewma_tracks_falling_signal() {
    volatile int16_t smoothed = -30;
    int8_t prev = static_cast<int8_t>(smoothed);
    for (int i = 0; i < 20; ++i) {
        int8_t out = ewmaRssi(smoothed, -100);
        TEST_ASSERT_TRUE(out <= prev);  // monotonic approach toward weaker signal
        prev = out;
    }
    TEST_ASSERT_TRUE(prev < -30);
}

// lookupOui — found / not-found branches, multi-entry table, NUL-termination
static const OuiEntry kTestTable[] = {
    {{0xAA, 0xBB, 0xCC}, "FirstVendor",  false},
    {{0x11, 0x22, 0x33}, "ThreatVendor", true },
    {{0x00, 0x00, 0x00}, "",             false},  // sentinel, excluded from entryCount
};
static const uint8_t kTestTableLen = 2;

void test_lookup_found_first_entry() {
    uint8_t mac[3] = {0xAA, 0xBB, 0xCC};
    char vendor[16];
    bool threat = true;  // start true to confirm it gets overwritten to false
    bool found = lookupOui(mac, kTestTable, kTestTableLen, vendor, &threat);
    TEST_ASSERT_TRUE(found);
    TEST_ASSERT_EQUAL_STRING("FirstVendor", vendor);
    TEST_ASSERT_FALSE(threat);
}

void test_lookup_found_threat_entry() {
    uint8_t mac[3] = {0x11, 0x22, 0x33};
    char vendor[16];
    bool threat = false;
    bool found = lookupOui(mac, kTestTable, kTestTableLen, vendor, &threat);
    TEST_ASSERT_TRUE(found);
    TEST_ASSERT_EQUAL_STRING("ThreatVendor", vendor);
    TEST_ASSERT_TRUE(threat);
}

void test_lookup_not_found_clears_output() {
    uint8_t mac[3] = {0xDE, 0xAD, 0xBE};
    char vendor[16] = "stale";
    bool threat = true;
    bool found = lookupOui(mac, kTestTable, kTestTableLen, vendor, &threat);
    TEST_ASSERT_FALSE(found);
    TEST_ASSERT_EQUAL_STRING("", vendor);
    TEST_ASSERT_FALSE(threat);
}

void test_lookup_vendor_truncated_and_terminated() {
    // Source vendor field fills all 16 bytes with no embedded NUL — simulates
    // a non-terminated source entry. lookupOui must still force-terminate
    // vendorOut at index 15 rather than trusting the source field.
    static const OuiEntry longNameTable[] = {
        {{0x01, 0x02, 0x03},
         {'A','A','A','A','A','A','A','A','A','A','A','A','A','A','A','A'},
         false},
    };
    uint8_t mac[3] = {0x01, 0x02, 0x03};
    char vendor[16];
    bool threat = false;
    bool found = lookupOui(mac, longNameTable, 1, vendor, &threat);
    TEST_ASSERT_TRUE(found);
    TEST_ASSERT_EQUAL_INT(15, strlen(vendor));
    TEST_ASSERT_EQUAL_CHAR('\0', vendor[15]);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_classify_none_deep);
    RUN_TEST(test_classify_none_boundary);
    RUN_TEST(test_classify_low_at_90);
    RUN_TEST(test_classify_low_mid);
    RUN_TEST(test_classify_low_boundary);
    RUN_TEST(test_classify_moderate_at_70);
    RUN_TEST(test_classify_moderate_mid);
    RUN_TEST(test_classify_moderate_bound);
    RUN_TEST(test_classify_critical_at_50);
    RUN_TEST(test_classify_critical_strong);
    RUN_TEST(test_bar_at_floor_exact);
    RUN_TEST(test_bar_below_floor);
    RUN_TEST(test_bar_at_ceil_exact);
    RUN_TEST(test_bar_above_ceil);
    RUN_TEST(test_bar_mid);
    RUN_TEST(test_bar_zero_width);
    RUN_TEST(test_ewma_first_sample_exact);
    RUN_TEST(test_ewma_tracks_rising_signal);
    RUN_TEST(test_ewma_tracks_falling_signal);
    RUN_TEST(test_lookup_found_first_entry);
    RUN_TEST(test_lookup_found_threat_entry);
    RUN_TEST(test_lookup_not_found_clears_output);
    RUN_TEST(test_lookup_vendor_truncated_and_terminated);
    return UNITY_END();
}
