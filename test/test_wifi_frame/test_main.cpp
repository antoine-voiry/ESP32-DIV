#include <unity.h>
#include "wifi_frame_logic.h"

void setUp()    {}
void tearDown() {}

void test_isDeauth_0xC0()    { TEST_ASSERT_TRUE(isDeauthFrame(0xC0)); }
void test_isDeauth_other()   { TEST_ASSERT_FALSE(isDeauthFrame(0xA0)); }
void test_isDeauth_zero()    { TEST_ASSERT_FALSE(isDeauthFrame(0x00)); }
void test_isDisassoc_0xA0()  { TEST_ASSERT_TRUE(isDisassocFrame(0xA0)); }
void test_isDisassoc_other() { TEST_ASSERT_FALSE(isDisassocFrame(0xC0)); }
void test_wrap_in_range()    { TEST_ASSERT_EQUAL_UINT8(6,  wrapChannel(6,  1, 13)); }
void test_wrap_above_max()   { TEST_ASSERT_EQUAL_UINT8(1,  wrapChannel(14, 1, 13)); }
void test_wrap_below_min()   { TEST_ASSERT_EQUAL_UINT8(1,  wrapChannel(0,  1, 13)); }
void test_wrap_at_min()      { TEST_ASSERT_EQUAL_UINT8(1,  wrapChannel(1,  1, 13)); }
void test_wrap_at_max()      { TEST_ASSERT_EQUAL_UINT8(13, wrapChannel(13, 1, 13)); }

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_isDeauth_0xC0);
    RUN_TEST(test_isDeauth_other);
    RUN_TEST(test_isDeauth_zero);
    RUN_TEST(test_isDisassoc_0xA0);
    RUN_TEST(test_isDisassoc_other);
    RUN_TEST(test_wrap_in_range);
    RUN_TEST(test_wrap_above_max);
    RUN_TEST(test_wrap_below_min);
    RUN_TEST(test_wrap_at_min);
    RUN_TEST(test_wrap_at_max);
    return UNITY_END();
}
