#!/usr/bin/env python3
"""Unit test for loadThemeFromEEPROM() bounds-check logic.

Simulates the C++ bounds check in Python so it can run without hardware.
Exit 0 = all assertions pass. Exit 1 = assertion failure.
"""

THEME_DARK  = 0
THEME_LIGHT = 1


def load_theme(byte_val):
    """Mirror of loadThemeFromEEPROM() logic in theme.cpp."""
    id_ = byte_val
    if id_ != THEME_DARK and id_ != THEME_LIGHT:
        id_ = THEME_DARK
    return id_


assert load_theme(0)    == THEME_DARK,  "byte 0x00 must load THEME_DARK"
assert load_theme(1)    == THEME_LIGHT, "byte 0x01 must load THEME_LIGHT"
assert load_theme(0xFF) == THEME_DARK,  "virgin EEPROM (0xFF) must default to THEME_DARK"
assert load_theme(2)    == THEME_DARK,  "out-of-range byte must default to THEME_DARK"
assert load_theme(0x80) == THEME_DARK,  "mid-range invalid byte must default to THEME_DARK"

print("test_theme_logic.py: all 5 assertions passed.")
