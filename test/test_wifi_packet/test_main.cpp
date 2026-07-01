// test/test_wifi_packet/test_main.cpp
#include <unity.h>
#include "wifi_packet_logic.h"

void setUp()    {}
void tearDown() {}

// computePacketGraphScale — Y-axis scale for packet monitor graph

// Failure mode: divide-by-zero or NaN when all counts are 0 (maxVal must init to 1, not 0)
void test_all_zeros_returns_one() {
    uint32_t counts[5] = {0, 0, 0, 0, 0};
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, (float)computePacketGraphScale(counts, 5, 320.0));
}

// Failure mode: loop starts at index 1, misses max at index 0
void test_max_at_first_index() {
    uint32_t counts[5] = {640, 0, 0, 0, 0};
    // 320.0 / 640 = 0.5
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, (float)computePacketGraphScale(counts, 5, 320.0));
}

// Failure mode: loop stops one short (i < len-1), misses max at last index
void test_max_at_last_index() {
    uint32_t counts[5] = {0, 0, 0, 0, 640};
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, (float)computePacketGraphScale(counts, 5, 320.0));
}

// Failure mode: off-by-one in max search when all values are equal
void test_all_equal_above_display_height() {
    uint32_t counts[4] = {1000, 1000, 1000, 1000};
    // 320.0 / 1000 = 0.32
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.32f, (float)computePacketGraphScale(counts, 4, 320.0));
}

// Failure mode: single-element array crashes or is skipped
void test_single_element_above_display_height() {
    uint32_t counts[1] = {800};
    // 320.0 / 800 = 0.4
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.4f, (float)computePacketGraphScale(counts, 1, 320.0));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_all_zeros_returns_one);
    RUN_TEST(test_max_at_first_index);
    RUN_TEST(test_max_at_last_index);
    RUN_TEST(test_all_equal_above_display_height);
    RUN_TEST(test_single_element_above_display_height);
    return UNITY_END();
}
