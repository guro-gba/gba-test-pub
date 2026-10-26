#!/usr/bin/env python3
"""Generates graphics/player.bmp: a 32x32, 16-color indexed BMP for Butano.

The sprite is "Sprig", an original little seedling creature drawn in a
soft, outlined handheld-RPG style. Palette index 0 is the transparent color.

Usage: make_player_bmp.py [output.bmp] [--preview preview.png]
"""

import struct
import sys

SIZE = 32

TRANSPARENT, OUTLINE, BODY, LIGHT, SHADOW, LEAF, LEAF_DARK, EYE, EYE_SHINE, CHEEK, BELLY, STEM = range(12)

PALETTE = [
    (255, 0, 255),   # transparent
    (24, 56, 40),    # outline
    (104, 196, 128), # body
    (168, 232, 168), # body highlight
    (60, 144, 96),   # body shadow
    (88, 200, 72),   # leaf
    (40, 128, 56),   # leaf vein / shadow
    (20, 20, 32),    # eye
    (255, 255, 255), # eye shine
    (244, 144, 156), # cheek
    (240, 236, 196), # belly
    (132, 96, 56),   # stem
] + [(0, 0, 0)] * 4


def in_ellipse(x, y, cx, cy, rx, ry):
    return ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0


def draw():
    img = [[TRANSPARENT] * SIZE for _ in range(SIZE)]

    def put(x, y, c):
        if 0 <= x < SIZE and 0 <= y < SIZE:
            img[y][x] = c

    # Feet, drawn first so the body overlaps them.
    for fx in (10.5, 21.5):
        for y in range(SIZE):
            for x in range(SIZE):
                if in_ellipse(x, y, fx, 27.5, 3.2, 2.2):
                    put(x, y, SHADOW)

    # Separate the feet from the body with an outline.
    for y in range(SIZE):
        for x in range(SIZE):
            if img[y][x] == SHADOW and not in_ellipse(x, y, 16, 19.5, 10.5, 8.5) and \
                    in_ellipse(x, y, 16, 19.5, 11.5, 9.5):
                put(x, y, OUTLINE)

    # Round body with light from the top left.
    for y in range(SIZE):
        for x in range(SIZE):
            if in_ellipse(x, y, 16, 19.5, 10.5, 8.5):
                light = (x - 16) / 10.5 + (y - 19.5) / 8.5
                put(x, y, LIGHT if light < -0.9 else SHADOW if light > 0.75 else BODY)

    # Cream belly.
    for y in range(SIZE):
        for x in range(SIZE):
            if in_ellipse(x, y, 16, 24, 5.5, 3.5):
                put(x, y, BELLY)

    # Stem and two leaves sprouting from the head.
    for y in range(7, 12):
        put(16, y, STEM)
    for (cx, cy, rx, ry) in ((11.5, 7, 4.5, 2.2), (20.5, 5, 4.5, 2.2)):
        for y in range(SIZE):
            for x in range(SIZE):
                if in_ellipse(x, y, cx, cy, rx, ry):
                    put(x, y, LEAF_DARK if y == round(cy) else LEAF)

    # Eyes: 2x3 with a shine pixel.
    for ex in (11, 20):
        for y in range(16, 19):
            for x in (ex, ex + 1):
                put(x, y, EYE)
        put(ex, 16, EYE_SHINE)

    # Cheeks and a small smile.
    for x in (7, 8, 23, 24):
        put(x, 20, CHEEK)
    for (x, y) in ((14, 20), (15, 21), (16, 21), (17, 20)):
        put(x, y, OUTLINE)

    # Outline every empty pixel touching the drawing (8-neighborhood).
    outline = []
    for y in range(SIZE):
        for x in range(SIZE):
            if img[y][x] != TRANSPARENT:
                continue
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < SIZE and 0 <= ny < SIZE and img[ny][nx] not in (TRANSPARENT, OUTLINE):
                        outline.append((x, y))
    for (x, y) in outline:
        img[y][x] = OUTLINE

    return img


def write_bmp(img, path):
    row_size = ((SIZE * 4 + 31) // 32) * 4
    pixels = bytearray()
    for line in reversed(img):  # BMP rows are stored bottom-up
        row = bytearray((line[x] << 4) | line[x + 1] for x in range(0, SIZE, 2))
        row.extend(b"\0" * (row_size - len(row)))
        pixels += row

    palette = b"".join(struct.pack("<BBBB", b, g, r, 0) for r, g, b in PALETTE)
    offset = 14 + 40 + len(palette)
    header = struct.pack("<2sIHHI", b"BM", offset + len(pixels), 0, 0, offset)
    info = struct.pack("<IiiHHIIiiII", 40, SIZE, SIZE, 1, 4, 0, len(pixels), 2835, 2835, 16, 0)

    with open(path, "wb") as f:
        f.write(header + info + palette + pixels)


def write_preview(img, path, scale=10):
    from PIL import Image

    preview = Image.new("RGB", (SIZE, SIZE))
    mint = (170, 240, 200)
    preview.putdata([mint if c == TRANSPARENT else PALETTE[c] for line in img for c in line])
    preview.resize((SIZE * scale, SIZE * scale), Image.NEAREST).save(path)


def main(args):
    preview = None
    if "--preview" in args:
        i = args.index("--preview")
        preview = args[i + 1]
        args = args[:i] + args[i + 2:]

    img = draw()
    write_bmp(img, args[0] if args else "graphics/player.bmp")
    if preview:
        write_preview(img, preview)


if __name__ == "__main__":
    main(sys.argv[1:])
