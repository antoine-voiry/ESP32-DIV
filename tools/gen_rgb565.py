#!/usr/bin/env python3
"""Convert hal9000_bg_bitmap (1-bit XBM) to RGB565 PROGMEM array.
Set bits → 0xF800 (red), clear bits → 0x0000 (black).
Run from repo root: python3 tools/gen_rgb565.py
"""
import re, math, sys
from pathlib import Path

SRC  = Path('include/hal9000_bg.h')
DEST = Path('include/hal9000_bg_rgb.h')

src = SRC.read_text()

w = int(re.search(r'HAL9000_BG_WIDTH\s+(\d+)',  src).group(1))
h = int(re.search(r'HAL9000_BG_HEIGHT\s+(\d+)', src).group(1))

raw_bytes = [int(x, 16) for x in re.findall(r'0x[0-9a-fA-F]+', src)]
bytes_per_row = math.ceil(w / 8)

expected = bytes_per_row * h
if len(raw_bytes) < expected:
    sys.exit(f"ERROR: expected {expected} bytes, got {len(raw_bytes)}")

pixels = []
for row in range(h):
    for col in range(w):
        b   = raw_bytes[row * bytes_per_row + col // 8]
        bit = (b >> (col % 8)) & 1          # XBM is LSB-first
        pixels.append(0xF800 if bit else 0x0000)

lines = [
    '#pragma once',
    '#include <stdint.h>',
    f'#define HAL9000_BG_RGB_WIDTH  {w}',
    f'#define HAL9000_BG_RGB_HEIGHT {h}',
    f'const uint16_t hal9000_bg_rgb[{w * h}] PROGMEM = {{',
]
for i in range(0, len(pixels), 16):
    chunk = pixels[i:i+16]
    row_str = ', '.join(f'0x{p:04X}' for p in chunk)
    suffix  = ',' if (i + 16) < len(pixels) else ''
    lines.append(f'    {row_str}{suffix}')
lines.append('};')

DEST.write_text('\n'.join(lines) + '\n')
print(f"Written {DEST}: {w}x{h} = {len(pixels)} pixels, {len(pixels)*2} bytes")
