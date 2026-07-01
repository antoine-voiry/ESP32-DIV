#include <unity.h>
#include "subghz_logic.h"

void setUp()    {}
void tearDown() {}

// Target failure mode: inverted return values (detected→1, not detected→0)
void test_cc1101_detected_returns_ok() {
    TEST_ASSERT_EQUAL_INT(0, computeCC1101Status(true));
}

void test_cc1101_absent_returns_not_detected() {
    TEST_ASSERT_EQUAL_INT(1, computeCC1101Status(false));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_cc1101_detected_returns_ok);
    RUN_TEST(test_cc1101_absent_returns_not_detected);
    return UNITY_END();
}
