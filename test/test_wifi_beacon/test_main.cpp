#include <unity.h>
#include "wifi_beacon_logic.h"
#include <string.h>

void setUp()    {}
void tearDown() {}

void test_channelDown_normal() { TEST_ASSERT_EQUAL_UINT8(5,  channelDown(6,  14)); }
void test_channelDown_wraps()  { TEST_ASSERT_EQUAL_UINT8(14, channelDown(1,  14)); }
void test_channelUp_normal()   { TEST_ASSERT_EQUAL_UINT8(7,  channelUp(6,   14)); }
void test_channelUp_wraps()    { TEST_ASSERT_EQUAL_UINT8(1,  channelUp(14,  14)); }
void test_toggleAttack_false() { TEST_ASSERT_TRUE(toggleAttack(false)); }
void test_toggleAttack_true()  { TEST_ASSERT_FALSE(toggleAttack(true)); }

// buildBeaconTlv: ssidLen=4 ("TEST") → totalLen = 51 + 4 = 55
void test_buildBeaconTlv_size() {
    uint8_t pkt[128] = {};
    int result = buildBeaconTlv(pkt, 128, "TEST", 4, 6);
    TEST_ASSERT_EQUAL_INT(55, result);
}

// ssid bytes written starting at byte 38
void test_buildBeaconTlv_ssid_written() {
    uint8_t pkt[128] = {};
    buildBeaconTlv(pkt, 128, "AB", 2, 6);
    TEST_ASSERT_EQUAL_UINT8('A', pkt[38]);
    TEST_ASSERT_EQUAL_UINT8('B', pkt[39]);
}

// ssid length byte written at byte 37
void test_buildBeaconTlv_ssidlen_byte() {
    uint8_t pkt[128] = {};
    buildBeaconTlv(pkt, 128, "HI", 2, 6);
    TEST_ASSERT_EQUAL_UINT8(2, pkt[37]);
}

// ssidLen=3 → ratesOffset=41; pkt[41] must be Supported Rates tag 0x01
void test_buildBeaconTlv_rates_tag() {
    uint8_t pkt[128] = {};
    buildBeaconTlv(pkt, 128, "ABC", 3, 6);
    TEST_ASSERT_EQUAL_UINT8(0x01, pkt[41]);
}

// ssidLen=3 → dsOffset=51; pkt[51] must be DS Parameter tag 0x03
void test_buildBeaconTlv_ds_tag() {
    uint8_t pkt[128] = {};
    buildBeaconTlv(pkt, 128, "ABC", 3, 6);
    TEST_ASSERT_EQUAL_UINT8(0x03, pkt[51]);
}

// pkt[dsOffset+2] = channel
void test_buildBeaconTlv_ds_channel() {
    uint8_t pkt[128] = {};
    buildBeaconTlv(pkt, 128, "ABC", 3, 11);
    TEST_ASSERT_EQUAL_UINT8(11, pkt[53]);
}

// bufLen too small → returns -1
void test_buildBeaconTlv_overflow() {
    uint8_t pkt[10] = {};
    int result = buildBeaconTlv(pkt, 10, "AB", 2, 6);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Failure mode: off-by-one → SSID length byte written as 1 instead of 0
void test_buildBeaconTlv_ssid_zero_len_byte() {
    uint8_t buf[64] = {};
    buildBeaconTlv(buf, sizeof(buf), "", 0, 6);
    TEST_ASSERT_EQUAL_UINT8(0, buf[37]);
}

// Failure mode: size calculation adds 1 for empty SSID → returns 52 instead of 51
void test_buildBeaconTlv_ssid_zero_return_size() {
    uint8_t buf[64] = {};
    int result = buildBeaconTlv(buf, sizeof(buf), "", 0, 6);
    TEST_ASSERT_EQUAL_INT(51, result);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_channelDown_normal);
    RUN_TEST(test_channelDown_wraps);
    RUN_TEST(test_channelUp_normal);
    RUN_TEST(test_channelUp_wraps);
    RUN_TEST(test_toggleAttack_false);
    RUN_TEST(test_toggleAttack_true);
    RUN_TEST(test_buildBeaconTlv_size);
    RUN_TEST(test_buildBeaconTlv_ssid_written);
    RUN_TEST(test_buildBeaconTlv_ssidlen_byte);
    RUN_TEST(test_buildBeaconTlv_rates_tag);
    RUN_TEST(test_buildBeaconTlv_ds_tag);
    RUN_TEST(test_buildBeaconTlv_ds_channel);
    RUN_TEST(test_buildBeaconTlv_overflow);
    RUN_TEST(test_buildBeaconTlv_ssid_zero_len_byte);
    RUN_TEST(test_buildBeaconTlv_ssid_zero_return_size);
    return UNITY_END();
}
