# UI Visual Redesign Blueprint — HAL 9000 Identity

**Date:** 2026-06-28  
**Status:** Implemented (2026-06-30)  
**Scope:** Bitmap asset replacement + color system migration roadmap

---

## Section 1 — 16-Bit Color Mapping Table

| Role | Old Variable | Old RGB565 | New Variable | New RGB565 | Hex24 | Notes |
|---|---|---|---|---|---|---|
| Background | `HALEHOUND_BLACK` | 0x0000 | `BG_BASE` | 0x18C3 | #1A1A1A | Near-black, not pure |
| Surface/card | `HALEHOUND_DARK` | 0x1082 | `BG_SURFACE` | 0x2969 | #2D2D2D | Card & status bar |
| Body text | `SHREDDY_TEAL` | 0x07FF | `TEXT_PRIMARY` | 0xF79D | #F5F0E8 | Warm white, not pure |
| Dim text | `HALEHOUND_GUNMETAL` | 0x4208 | `TEXT_SECONDARY` | 0x9492 | #909090 | ≥4.5:1 on BG_BASE |
| Selection | `ORANGE` | 0xFD20 | `ACCENT_SELECT` | 0xFEA0 | #FFD600 | 7:1 on BG_BASE |
| Border | `HALEHOUND_CYAN` | 0x07FF | `ACCENT_BORDER` | 0x5ACD | #5A5A6A | Unselected frame |
| OK/green | `HALEHOUND_GREEN` | 0x07E0 | `STATUS_OK` | 0x072E | #00E676 | Battery full, SD ok |
| Warn/amber | `HALEHOUND_MAGENTA` | 0xF81F | `STATUS_WARN` | 0xFD80 | #FFB300 | Battery mid, temp |
| Error/red | `RED` | 0xF800 | `STATUS_ERR` | 0xFA8A | #FF5252 | Battery low, errors; **also HAL lens color** |

**HAL lens render color:** `0xF800` (pure red) used for the home screen background bitmap, matching the HAL 9000 red eye aesthetic. Loading animation uses `STATUS_ERR` (0xFA8A ≈ #FF5252) for a slightly softer red.

---

## Section 2 — Theme Isolation (theme.h)

**Current state:** Color `extern` declarations live in `shared.h` lines 14–49 with definitions in `src/theme.cpp`.

**Proposed change:** Extract color declarations into `include/theme.h`, keep `shared.h` for non-color shared types.

```cpp
// include/theme.h (new, thin header)
#pragma once
#include <Arduino.h>

extern uint16_t BG_BASE, BG_SURFACE;
extern uint16_t TEXT_PRIMARY, TEXT_SECONDARY;
extern uint16_t ACCENT_SELECT, ACCENT_BORDER;
extern uint16_t STATUS_OK, STATUS_WARN, STATUS_ERR;
// Legacy aliases (remove these after full migration):
extern uint16_t ORANGE, GRAY, HALEHOUND_MAGENTA, HALEHOUND_CYAN;
```

`shared.h` gets `#include "theme.h"` — zero source-file changes required in callers.

**Migration path:** Once `theme.h` exists, migrate call sites from `HALEHOUND_*` names to semantic `BG_*`/`TEXT_*`/`ACCENT_*`/`STATUS_*` names one module at a time. The legacy alias block in `theme.h` keeps the build green during migration.

---

## Section 3 — Typography & Menu Layout Adjustments

### Current State

- `tft.setTextSize(1)` (6×8 px glyphs) throughout menus — too small at arm's length
- Menu card geometry: 100×60 per cell
  - Icon: 16×16 at offset (+42, +10)
  - Label: `setTextSize(1)` at (+8, +42)

### Proposed Changes

**Text size:** `setTextSize(2)` → 12×16px glyphs, minimum readable at arm's length.

| Element | Current | Proposed |
|---|---|---|
| Card height | 60px | 75px (Y_SPACING already 75) |
| Label y-offset | +42 | +50 |
| Icon y-offset | +10 | +10 (unchanged) |
| Line height (multi-line) | — | 20px (16px glyph + 4px gap) |

**Status bar:** Scale battery/icon bitmaps ×2. Bump `STATUS_BAR_HEIGHT` from 24 → 32px. Adjust `Y_START` for menu grid from 34 → 42 to clear the taller bar.

---

## Section 4 — Accessibility Verification Checklist

1. **Contrast ≥ 4.5:1 (text):**  
   `TEXT_PRIMARY` (0xF79D) on `BG_BASE` (0x18C3) ≈ 12:1. ✓

2. **Contrast ≥ 7:1 (critical indicators):**  
   `ACCENT_SELECT` (0xFEA0, amber) on `BG_BASE` ≈ 9.2:1. ✓  
   `STATUS_ERR` (0xFA8A) on `BG_BASE` ≥ 5:1. ✓

3. **Active cursor ≥ 3px visible delta:**  
   Selected cell border width ≥ 2px + fill color swap (`ACCENT_SELECT` vs `ACCENT_BORDER`). ✓

4. **Status bar icons ≥ 16×16 px:**  
   Battery outline 22×10 → scale to 22×14 with ×2 stroke. SD/temp icons 16×16 maintained.

5. **Text clip prevention:**  
   Use `tft.getTextBounds()` before drawing any dynamic string. If `width > (cardWidth - 8)`, truncate with `"…"` before render.

---

## Implemented Changes (2026-06-30)

| File | Change |
|---|---|
| `include/skull_bg.h` | Deleted |
| `include/hal9000_bg.h` | New — 211×280 XBM HAL face panel (frame + grill + lens + glint) |
| `include/home_bg.h` | Replaced bitmap data — horizontal scanlines every 4px (240×320) |
| `include/icon.h` | `bitmap_icon_skull` → `bitmap_icon_hal9000_eye`; 10 loading frames renamed; 8 menu icons replaced with HAL eye variants |
| `src/main.cpp` | Include swapped, menu icon array renamed, background render color `0x4808` → `0xF800` |
| `src/utils.cpp` | Loading frame names renamed; alternating MAGENTA/CYAN → single `STATUS_ERR` red |
