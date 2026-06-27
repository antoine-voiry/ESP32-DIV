#!/usr/bin/env python3
"""Host-side WCAG contrast gate for ESP32-DIV accessibility theme palette.

Runs against the RGB565 palette values baked into theme.cpp.
Exit 0 = all checks pass. Exit 1 = one or more failures.

Threshold rationale:
  7.0 : WCAG AAA for body text
  4.5 : WCAG AA for UI text and actionable elements
  3.0 : minimum for color-coded status indicators (hue carries meaning)
"""

PALETTES = {
    "DARK": {
        "BG_BASE":        0x18C3,
        "BG_SURFACE":     0x2969,
        "TEXT_PRIMARY":   0xF79D,
        "TEXT_SECONDARY": 0x9492,
        "ACCENT_SELECT":  0xFEA0,
        "STATUS_OK":      0x072E,
        "STATUS_WARN":    0xFD80,
        "STATUS_ERR":     0xFA8A,
    },
    "LIGHT": {
        "BG_BASE":        0xF79E,
        "BG_SURFACE":     0xEF5C,
        "TEXT_PRIMARY":   0x18C3,
        "TEXT_SECONDARY": 0x528A,
        "ACCENT_SELECT":  0x01F0,
        "STATUS_OK":      0x03E6,
        "STATUS_WARN":    0xE280,
        "STATUS_ERR":     0xB0E3,
    },
}

# (foreground_token, background_token, min_ratio, label)
CHECKS = [
    ("TEXT_PRIMARY",   "BG_BASE",        7.0, "body text — WCAG AAA"),
    ("TEXT_SECONDARY", "BG_BASE",        4.5, "secondary text — WCAG AA"),
    ("ACCENT_SELECT",  "BG_BASE",        4.5, "accent on base — WCAG AA"),
    ("STATUS_OK",      "BG_BASE",        4.5, "status OK — WCAG AA"),
    ("STATUS_WARN",    "BG_BASE",        3.0, "status WARN — color-coded; hue carries meaning"),
    ("STATUS_ERR",     "BG_BASE",        4.5, "status ERR — WCAG AA"),
    ("BG_BASE",        "ACCENT_SELECT",  4.5, "inverted text on selection row — WCAG AA"),
]


def rgb565_to_srgb(c565):
    r = ((c565 >> 11) & 0x1F) / 31.0
    g = ((c565 >> 5)  & 0x3F) / 63.0
    b = (c565         & 0x1F) / 31.0
    return r, g, b


def linearize(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def luminance(c565):
    r, g, b = (linearize(x) for x in rgb565_to_srgb(c565))
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast(fg_c565, bg_c565):
    lf = luminance(fg_c565) + 0.05
    lb = luminance(bg_c565) + 0.05
    return max(lf, lb) / min(lf, lb)


failures = []
for theme_name, palette in PALETTES.items():
    print(f"\n--- {theme_name} ---")
    for fg_tok, bg_tok, min_ratio, label in CHECKS:
        ratio = contrast(palette[fg_tok], palette[bg_tok])
        ok = ratio >= min_ratio
        status = "PASS" if ok else "FAIL"
        print(f"  {label:45s}  {ratio:5.2f}:1  {status}")
        if not ok:
            failures.append(
                f"[{theme_name}] {label}: {ratio:.2f}:1 < required {min_ratio}:1"
            )

if failures:
    print("\n\nFAILED:")
    for f in failures:
        print(f"  {f}")
    raise SystemExit(1)

print("\n\nAll contrast checks passed.")
