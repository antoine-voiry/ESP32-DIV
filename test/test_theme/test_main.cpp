#include <unity.h>
#include "theme_types.h"
#include "theme_logic.h"

void setUp()    {}
void tearDown() {}

void test_dark_bg_base() {
    ColorSet cs = buildColorSet(THEME_DARK);
    TEST_ASSERT_EQUAL_HEX16(0x18C3, cs.bg_base);
}

void test_light_bg_base() {
    ColorSet cs = buildColorSet(THEME_LIGHT);
    TEST_ASSERT_EQUAL_HEX16(0xF79E, cs.bg_base);
}

void test_dark_status_colors() {
    ColorSet cs = buildColorSet(THEME_DARK);
    TEST_ASSERT_EQUAL_HEX16(0x072E, cs.status_ok);
    TEST_ASSERT_EQUAL_HEX16(0xFD80, cs.status_warn);
    TEST_ASSERT_EQUAL_HEX16(0xFA8A, cs.status_err);
}

void test_light_status_colors() {
    ColorSet cs = buildColorSet(THEME_LIGHT);
    TEST_ASSERT_EQUAL_HEX16(0x03E6, cs.status_ok);
    TEST_ASSERT_EQUAL_HEX16(0xE280, cs.status_warn);
    TEST_ASSERT_EQUAL_HEX16(0xB0E3, cs.status_err);
}

void test_dark_legacy_aliases() {
    ColorSet cs = buildColorSet(THEME_DARK);
    TEST_ASSERT_EQUAL_HEX16(cs.text_primary,  cs.halehound_cyan);
    TEST_ASSERT_EQUAL_HEX16(cs.accent_select, cs.halehound_magenta);
    TEST_ASSERT_EQUAL_HEX16(cs.bg_base,       cs.black);
    TEST_ASSERT_EQUAL_HEX16(cs.text_primary,  cs.white);
}

void test_validate_raw_dark()         { TEST_ASSERT_EQUAL(THEME_DARK,  validateThemeRaw(0)); }
void test_validate_raw_light()        { TEST_ASSERT_EQUAL(THEME_LIGHT, validateThemeRaw(1)); }
void test_validate_raw_out_of_range() {
    TEST_ASSERT_EQUAL(THEME_DARK, validateThemeRaw(2));
    TEST_ASSERT_EQUAL(THEME_DARK, validateThemeRaw(99));
    TEST_ASSERT_EQUAL(THEME_DARK, validateThemeRaw(255));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_dark_bg_base);
    RUN_TEST(test_light_bg_base);
    RUN_TEST(test_dark_status_colors);
    RUN_TEST(test_light_status_colors);
    RUN_TEST(test_dark_legacy_aliases);
    RUN_TEST(test_validate_raw_dark);
    RUN_TEST(test_validate_raw_light);
    RUN_TEST(test_validate_raw_out_of_range);
    return UNITY_END();
}
