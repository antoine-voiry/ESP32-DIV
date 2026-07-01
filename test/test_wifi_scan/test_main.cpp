#include <unity.h>
#include "wifi_scan_logic.h"
#include <string.h>

void setUp()    {}
void tearDown() {}

static WifiApInfo makeAp(const char* ssid, uint8_t b0, uint8_t b1, uint8_t b2,
                         uint8_t b3, uint8_t b4, uint8_t b5,
                         int8_t rssi, uint8_t channel) {
    WifiApInfo ap = {};
    strncpy(ap.ssid, ssid, 32);
    ap.ssid[32] = '\0';
    ap.bssid[0]=b0; ap.bssid[1]=b1; ap.bssid[2]=b2;
    ap.bssid[3]=b3; ap.bssid[4]=b4; ap.bssid[5]=b5;
    ap.rssi    = rssi;
    ap.channel = channel;
    return ap;
}

void test_normal_ap() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 0);
    TEST_ASSERT_FALSE(r.isHidden);
    TEST_ASSERT_FALSE(r.isEvilTwin);
    TEST_ASSERT_FALSE(r.isWeirdChannel);
    TEST_ASSERT_FALSE(r.isUnderAttack);
}

void test_hidden_ssid() {
    WifiApInfo ap = makeAp("",0x01,0x02,0x03,0x04,0x05,0x06,-70,6);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 0);
    TEST_ASSERT_TRUE(r.isHidden);
}

void test_evil_twin() {
    WifiApInfo allAps[2];
    allAps[0] = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    allAps[1] = makeAp("Home",0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,-65,6);
    NetworkAnomaly r = analyzeAp(&allAps[0], allAps, 2, 0);
    TEST_ASSERT_TRUE(r.isEvilTwin);
}

void test_no_evil_twin_same_bssid() {
    WifiApInfo allAps[2];
    allAps[0] = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    allAps[1] = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-65,6);
    NetworkAnomaly r = analyzeAp(&allAps[0], allAps, 2, 0);
    TEST_ASSERT_FALSE(r.isEvilTwin);
}

void test_no_evil_twin_single_ap() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 0);
    TEST_ASSERT_FALSE(r.isEvilTwin);
}

void test_weird_channel_14() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,14);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 0);
    TEST_ASSERT_TRUE(r.isWeirdChannel);
}

void test_normal_channel_13() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,13);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 0);
    TEST_ASSERT_FALSE(r.isWeirdChannel);
}

void test_under_attack_6() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 6);
    TEST_ASSERT_TRUE(r.isUnderAttack);
}

void test_not_under_attack_5() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 5);
    TEST_ASSERT_FALSE(r.isUnderAttack);
}

void test_compare_higher_rssi_first() {
    WifiApInfo a = {}; a.rssi = -30;
    WifiApInfo b = {}; b.rssi = -80;
    TEST_ASSERT_TRUE(compareApByRssi(&a, &b) < 0);
}

void test_compare_equal_rssi() {
    WifiApInfo a = {}; a.rssi = -50;
    WifiApInfo b = {}; b.rssi = -50;
    TEST_ASSERT_EQUAL_INT(0, compareApByRssi(&a, &b));
}

void test_hidden_ssid_evil_twin() {
    WifiApInfo allAps[2];
    allAps[0] = makeAp("", 0xAA,0xBB,0xCC,0xDD,0xEE,0x01, -70, 6);
    allAps[1] = makeAp("", 0xAA,0xBB,0xCC,0xDD,0xEE,0x02, -75, 6);
    NetworkAnomaly r = analyzeAp(&allAps[0], allAps, 2, 0);
    TEST_ASSERT_TRUE(r.isHidden);
    TEST_ASSERT_TRUE(r.isEvilTwin);
}

// Failure mode: sign inversion → ascending sort instead of descending (worse rssi sorts first)
void test_compare_lower_rssi_sorts_after() {
    WifiApInfo worse = {}; worse.rssi = -80;
    WifiApInfo better = {}; better.rssi = -60;
    int result = compareApByRssi(&worse, &better);
    TEST_ASSERT_GREATER_THAN(0, result);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_normal_ap);
    RUN_TEST(test_hidden_ssid);
    RUN_TEST(test_hidden_ssid_evil_twin);
    RUN_TEST(test_evil_twin);
    RUN_TEST(test_no_evil_twin_same_bssid);
    RUN_TEST(test_no_evil_twin_single_ap);
    RUN_TEST(test_weird_channel_14);
    RUN_TEST(test_normal_channel_13);
    RUN_TEST(test_under_attack_6);
    RUN_TEST(test_not_under_attack_5);
    RUN_TEST(test_compare_higher_rssi_first);
    RUN_TEST(test_compare_equal_rssi);
    RUN_TEST(test_compare_lower_rssi_sorts_after);
    return UNITY_END();
}
