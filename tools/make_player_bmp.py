#!/usr/bin/env python3
"""Generates graphics/player.bmp: a 16x16, 16-color indexed BMP for Butano.

Palette index 0 is the transparent color.
"""

import struct
import sys

ART = [
    "................",
    ".....111111.....",
    "...1122222211...",
    "..122222222221..",
    "..122332233221..",
    ".12223322332221.",
    ".12222222222221.",
    ".12222222222221.",
    ".12244222244221.",
    ".12224444442221.",
    "..122224422221..",
    "..122222222221..",
    "...1122222211...",
    ".....111111.....",
    "................",
    "................",
]

PALETTE = [
    (255, 0, 255),   # 0: transparent
    (24, 24, 48),    # 1: outline
    (255, 200, 40),  # 2: body
    (255, 255, 255), # 3: eyes
    (200, 40, 60),   # 4: mouth
] + [(0, 0, 0)] * 11


def main(path):
    width = height = 16
    row_size = ((width * 4 + 31) // 32) * 4
    pixels = bytearray()
    for line in reversed(ART):  # BMP rows are stored bottom-up
        row = bytearray()
        for x in range(0, width, 2):
            hi = 0 if line[x] == "." else int(line[x])
            lo = 0 if line[x + 1] == "." else int(line[x + 1])
            row.append((hi << 4) | lo)
        row.extend(b"\0" * (row_size - len(row)))
        pixels += row

    palette = b"".join(struct.pack("<BBBB", b, g, r, 0) for r, g, b in PALETTE)
    offset = 14 + 40 + len(palette)
    header = struct.pack("<2sIHHI", b"BM", offset + len(pixels), 0, 0, offset)
    info = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 4, 0, len(pixels), 2835, 2835, 16, 0)

    with open(path, "wb") as f:
        f.write(header + info + palette + pixels)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "graphics/player.bmp")
