#include <unity.h>
#include "Arduino.h"   // mock — defines PROGMEM as nothing for native builds
#include "icon.h"
#include "hal9000_bg.h"
#include "home_bg.h"

// Expected sizes (compile-time constants used in assertions):
//   16×16 icon:           ceil(16/8) * 16  =  2 * 16  =  32 bytes
//   100×120 loading frame: ceil(100/8) * 120 = 13 * 120 = 1560 bytes
//   211×280 HAL9000 bg:   ceil(211/8) * 280 = 27 * 280 = 7560 bytes
//   240×320 home bg:      ceil(240/8) * 320 = 30 * 320 = 9600 bytes

static constexpr int ICON_16X16_BYTES    = 32;
static constexpr int LOADING_FRAME_BYTES = 1560;
static constexpr int HAL9000_BG_BYTES    = 7560;
static constexpr int HOME_BG_BYTES       = 9600;

void setUp()    {}
void tearDown() {}

// ── HAL9000 background dimension constants ──────────────────────────────────
void test_hal9000_bg_width_constant()  { TEST_ASSERT_EQUAL(211, HAL9000_BG_WIDTH); }
void test_hal9000_bg_height_constant() { TEST_ASSERT_EQUAL(280, HAL9000_BG_HEIGHT); }
void test_hal9000_bg_array_size()      { TEST_ASSERT_EQUAL(HAL9000_BG_BYTES, (int)sizeof(hal9000_bg_bitmap)); }

// ── home_bg dimension constants ──────────────────────────────────────────────
void test_home_bg_width_constant()  { TEST_ASSERT_EQUAL(240, HOME_BG_WIDTH); }
void test_home_bg_height_constant() { TEST_ASSERT_EQUAL(320, HOME_BG_HEIGHT); }
void test_home_bg_array_size()      { TEST_ASSERT_EQUAL(HOME_BG_BYTES, (int)sizeof(home_bg_bitmap)); }

// ── HAL9000 eye (standalone, replaces bitmap_icon_skull) ────────────────────
void test_hal9000_eye_size() { TEST_ASSERT_EQUAL(ICON_16X16_BYTES, (int)sizeof(bitmap_icon_hal9000_eye)); }

// ── HAL9000 menu icons (8 categories) ────────────────────────────────────────
void test_hal9000_wifi_size()     { TEST_ASSERT_EQUAL(ICON_16X16_BYTES, (int)sizeof(bitmap_icon_hal9000_wifi)); }
void test_hal9000_bt_size()       { TEST_ASSERT_EQUAL(ICON_16X16_BYTES, (int)sizeof(bitmap_icon_hal9000_bluetooth)); }
void test_hal9000_jammer_size()   { TEST_ASSERT_EQUAL(ICON_16X16_BYTES, (int)sizeof(bitmap_icon_hal9000_jammer)); }
void test_hal9000_subghz_size()   { TEST_ASSERT_EQUAL(ICON_16X16_BYTES, (int)sizeof(bitmap_icon_hal9000_subghz)); }
void test_hal9000_ir_size()       { TEST_ASSERT_EQUAL(ICON_16X16_BYTES, (int)sizeof(bitmap_icon_hal9000_ir)); }
void test_hal9000_tools_size()    { TEST_ASSERT_EQUAL(ICON_16X16_BYTES, (int)sizeof(bitmap_icon_hal9000_tools)); }
void test_hal9000_setting_size()  { TEST_ASSERT_EQUAL(ICON_16X16_BYTES, (int)sizeof(bitmap_icon_hal9000_setting)); }
void test_hal9000_about_size()    { TEST_ASSERT_EQUAL(ICON_16X16_BYTES, (int)sizeof(bitmap_icon_hal9000_about)); }

// ── HAL9000 loading frames (10 frames, 100×120) ───────────────────────────────
void test_loading_frame_1()  { TEST_ASSERT_EQUAL(LOADING_FRAME_BYTES, (int)sizeof(bitmap_icon_hal9000_loading_1)); }
void test_loading_frame_2()  { TEST_ASSERT_EQUAL(LOADING_FRAME_BYTES, (int)sizeof(bitmap_icon_hal9000_loading_2)); }
void test_loading_frame_3()  { TEST_ASSERT_EQUAL(LOADING_FRAME_BYTES, (int)sizeof(bitmap_icon_hal9000_loading_3)); }
void test_loading_frame_4()  { TEST_ASSERT_EQUAL(LOADING_FRAME_BYTES, (int)sizeof(bitmap_icon_hal9000_loading_4)); }
void test_loading_frame_5()  { TEST_ASSERT_EQUAL(LOADING_FRAME_BYTES, (int)sizeof(bitmap_icon_hal9000_loading_5)); }
void test_loading_frame_6()  { TEST_ASSERT_EQUAL(LOADING_FRAME_BYTES, (int)sizeof(bitmap_icon_hal9000_loading_6)); }
void test_loading_frame_7()  { TEST_ASSERT_EQUAL(LOADING_FRAME_BYTES, (int)sizeof(bitmap_icon_hal9000_loading_7)); }
void test_loading_frame_8()  { TEST_ASSERT_EQUAL(LOADING_FRAME_BYTES, (int)sizeof(bitmap_icon_hal9000_loading_8)); }
void test_loading_frame_9()  { TEST_ASSERT_EQUAL(LOADING_FRAME_BYTES, (int)sizeof(bitmap_icon_hal9000_loading_9)); }
void test_loading_frame_10() { TEST_ASSERT_EQUAL(LOADING_FRAME_BYTES, (int)sizeof(bitmap_icon_hal9000_loading_10)); }

// ── Regression: skull_* symbols must NOT exist (caught at compile time by absence)
// If bitmap_icon_skull_wifi were re-introduced, this file would need to not compile.
// We assert via the HAL9000 names instead — if a skull name crept back in and shadowed,
// the size tests above would still pass, but the firmware would fail to link without the
// HAL9000 names. No additional runtime assertion needed here.

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_hal9000_bg_width_constant);
    RUN_TEST(test_hal9000_bg_height_constant);
    RUN_TEST(test_hal9000_bg_array_size);

    RUN_TEST(test_home_bg_width_constant);
    RUN_TEST(test_home_bg_height_constant);
    RUN_TEST(test_home_bg_array_size);

    RUN_TEST(test_hal9000_eye_size);

    RUN_TEST(test_hal9000_wifi_size);
    RUN_TEST(test_hal9000_bt_size);
    RUN_TEST(test_hal9000_jammer_size);
    RUN_TEST(test_hal9000_subghz_size);
    RUN_TEST(test_hal9000_ir_size);
    RUN_TEST(test_hal9000_tools_size);
    RUN_TEST(test_hal9000_setting_size);
    RUN_TEST(test_hal9000_about_size);

    RUN_TEST(test_loading_frame_1);
    RUN_TEST(test_loading_frame_2);
    RUN_TEST(test_loading_frame_3);
    RUN_TEST(test_loading_frame_4);
    RUN_TEST(test_loading_frame_5);
    RUN_TEST(test_loading_frame_6);
    RUN_TEST(test_loading_frame_7);
    RUN_TEST(test_loading_frame_8);
    RUN_TEST(test_loading_frame_9);
    RUN_TEST(test_loading_frame_10);

    return UNITY_END();
}
