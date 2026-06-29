#include <unity.h>
#include "theme_types.h"
#include "theme_logic.h"

void setUp()    {}
void tearDown() {}

// ── buildColorSet: THEME_DARK branch ─────────────────────────────────────────
void test_dark_bg_base()        { TEST_ASSERT_EQUAL_HEX16(0x18C3, buildColorSet(THEME_DARK).bg_base); }
void test_dark_bg_surface()     { TEST_ASSERT_EQUAL_HEX16(0x2969, buildColorSet(THEME_DARK).bg_surface); }
void test_dark_text_primary()   { TEST_ASSERT_EQUAL_HEX16(0xF79D, buildColorSet(THEME_DARK).text_primary); }
void test_dark_text_secondary() { TEST_ASSERT_EQUAL_HEX16(0x9492, buildColorSet(THEME_DARK).text_secondary); }
void test_dark_accent_select()  { TEST_ASSERT_EQUAL_HEX16(0xFEA0, buildColorSet(THEME_DARK).accent_select); }
void test_dark_accent_border()  { TEST_ASSERT_EQUAL_HEX16(0x5ACD, buildColorSet(THEME_DARK).accent_border); }
void test_dark_status_ok()      { TEST_ASSERT_EQUAL_HEX16(0x072E, buildColorSet(THEME_DARK).status_ok); }
void test_dark_status_warn()    { TEST_ASSERT_EQUAL_HEX16(0xFD80, buildColorSet(THEME_DARK).status_warn); }
void test_dark_status_err()     { TEST_ASSERT_EQUAL_HEX16(0xFA8A, buildColorSet(THEME_DARK).status_err); }

// ── buildColorSet: THEME_LIGHT branch ────────────────────────────────────────
void test_light_bg_base()        { TEST_ASSERT_EQUAL_HEX16(0xF79E, buildColorSet(THEME_LIGHT).bg_base); }
void test_light_bg_surface()     { TEST_ASSERT_EQUAL_HEX16(0xEF5C, buildColorSet(THEME_LIGHT).bg_surface); }
void test_light_text_primary()   { TEST_ASSERT_EQUAL_HEX16(0x18C3, buildColorSet(THEME_LIGHT).text_primary); }
void test_light_text_secondary() { TEST_ASSERT_EQUAL_HEX16(0x528A, buildColorSet(THEME_LIGHT).text_secondary); }
void test_light_accent_select()  { TEST_ASSERT_EQUAL_HEX16(0x01F0, buildColorSet(THEME_LIGHT).accent_select); }
void test_light_accent_border()  { TEST_ASSERT_EQUAL_HEX16(0xAD55, buildColorSet(THEME_LIGHT).accent_border); }
void test_light_status_ok()      { TEST_ASSERT_EQUAL_HEX16(0x03E6, buildColorSet(THEME_LIGHT).status_ok); }
void test_light_status_warn()    { TEST_ASSERT_EQUAL_HEX16(0xE280, buildColorSet(THEME_LIGHT).status_warn); }
void test_light_status_err()     { TEST_ASSERT_EQUAL_HEX16(0xB0E3, buildColorSet(THEME_LIGHT).status_err); }

// ── Legacy alias resolution (dark — aliases are aliased to core fields) ───────
void test_dark_alias_cyan_eq_text_primary()   {
    ColorSet cs = buildColorSet(THEME_DARK);
    TEST_ASSERT_EQUAL_HEX16(cs.text_primary,  cs.halehound_cyan);
}
void test_dark_alias_magenta_eq_accent()      {
    ColorSet cs = buildColorSet(THEME_DARK);
    TEST_ASSERT_EQUAL_HEX16(cs.accent_select, cs.halehound_magenta);
}
void test_dark_alias_black_eq_bg_base()       {
    ColorSet cs = buildColorSet(THEME_DARK);
    TEST_ASSERT_EQUAL_HEX16(cs.bg_base, cs.black);
}
void test_dark_alias_white_eq_text_primary()  {
    ColorSet cs = buildColorSet(THEME_DARK);
    TEST_ASSERT_EQUAL_HEX16(cs.text_primary, cs.white);
}
void test_dark_alias_green_eq_status_ok()     {
    ColorSet cs = buildColorSet(THEME_DARK);
    TEST_ASSERT_EQUAL_HEX16(cs.status_ok, cs.green);
}
void test_dark_alias_red_eq_status_err()      {
    ColorSet cs = buildColorSet(THEME_DARK);
    TEST_ASSERT_EQUAL_HEX16(cs.status_err, cs.red);
}

// ── validateThemeRaw ──────────────────────────────────────────────────────────
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
    RUN_TEST(test_dark_bg_surface);
    RUN_TEST(test_dark_text_primary);
    RUN_TEST(test_dark_text_secondary);
    RUN_TEST(test_dark_accent_select);
    RUN_TEST(test_dark_accent_border);
    RUN_TEST(test_dark_status_ok);
    RUN_TEST(test_dark_status_warn);
    RUN_TEST(test_dark_status_err);
    RUN_TEST(test_light_bg_base);
    RUN_TEST(test_light_bg_surface);
    RUN_TEST(test_light_text_primary);
    RUN_TEST(test_light_text_secondary);
    RUN_TEST(test_light_accent_select);
    RUN_TEST(test_light_accent_border);
    RUN_TEST(test_light_status_ok);
    RUN_TEST(test_light_status_warn);
    RUN_TEST(test_light_status_err);
    RUN_TEST(test_dark_alias_cyan_eq_text_primary);
    RUN_TEST(test_dark_alias_magenta_eq_accent);
    RUN_TEST(test_dark_alias_black_eq_bg_base);
    RUN_TEST(test_dark_alias_white_eq_text_primary);
    RUN_TEST(test_dark_alias_green_eq_status_ok);
    RUN_TEST(test_dark_alias_red_eq_status_err);
    RUN_TEST(test_validate_raw_dark);
    RUN_TEST(test_validate_raw_light);
    RUN_TEST(test_validate_raw_out_of_range);
    return UNITY_END();
}
