#!/usr/bin/env python3
"""Generates every asset Leafbound needs from code.

Outputs (all committed, so the build doesn't need to run this script):
  graphics/spr_*.bmp/.json   placeholder sprites (shapes and colors)
  graphics/map_*.bmp/.json   one big background per map, drawn from the layout
  src/level_data.cpp         collision tiles, enemy / portal / item spawns per map

The map layouts below are the single source of truth for both what a map
looks like and how it collides. Edit them and re-run:

  python3 tools/gen_assets.py
"""

import json
import os
import struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GRAPHICS = os.path.join(ROOT, "graphics")
SRC = os.path.join(ROOT, "src")

# ---------------------------------------------------------------------------------------------------------------------
# BMP writing
# ---------------------------------------------------------------------------------------------------------------------


def write_bmp(path, pixels, palette):
    """Writes a 4bpp indexed BMP. pixels is a list of rows of palette indices."""
    height = len(pixels)
    width = len(pixels[0])
    row_size = ((width * 4 + 31) // 32) * 4
    data = bytearray()
    for line in reversed(pixels):
        row = bytearray((line[x] << 4) | line[x + 1] for x in range(0, width, 2))
        row.extend(b"\0" * (row_size - len(row)))
        data += row

    palette = list(palette) + [(0, 0, 0)] * (16 - len(palette))
    pal = b"".join(struct.pack("<BBBB", b, g, r, 0) for r, g, b in palette)
    offset = 14 + 40 + len(pal)
    header = struct.pack("<2sIHHI", b"BM", offset + len(data), 0, 0, offset)
    info = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 4, 0, len(data), 2835, 2835, 16, 0)
    with open(path, "wb") as f:
        f.write(header + info + pal + data)


def write_json(path, data):
    with open(path, "w") as f:
        json.dump(data, f, indent=4)
        f.write("\n")


class Canvas:
    def __init__(self, w, h, fill=0):
        self.w = w
        self.h = h
        self.px = [[fill] * w for _ in range(h)]

    def put(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = c

    def rect(self, x, y, w, h, c):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.put(xx, yy, c)

    def ellipse(self, cx, cy, rx, ry, c):
        for yy in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for xx in range(int(cx - rx) - 1, int(cx + rx) + 2):
                if ((xx + 0.5 - cx) / rx) ** 2 + ((yy + 0.5 - cy) / ry) ** 2 <= 1.0:
                    self.put(xx, yy, c)

    def outline(self, c):
        """Draws an outline of color c around every non-transparent pixel."""
        marks = []
        for y in range(self.h):
            for x in range(self.w):
                if self.px[y][x] == 0:
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < self.w and 0 <= ny < self.h and self.px[ny][nx] not in (0, c):
                            marks.append((x, y))
                            break
        for x, y in marks:
            self.px[y][x] = c


# ---------------------------------------------------------------------------------------------------------------------
# Sprites. They all share one palette, so they only use one hardware sprite palette.
# ---------------------------------------------------------------------------------------------------------------------

SPRITE_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (20, 20, 32),     # 1 outline
    (248, 248, 248),  # 2 white
    (64, 112, 224),   # 3 tunic blue
    (248, 200, 160),  # 4 skin
    (224, 48, 48),    # 5 red
    (120, 24, 40),    # 6 dark red
    (48, 120, 248),   # 7 mp blue
    (248, 216, 48),   # 8 yellow
    (240, 136, 40),   # 9 orange
    (88, 192, 72),    # 10 green
    (32, 104, 48),    # 11 dark green
    (152, 80, 208),   # 12 purple
    (120, 80, 48),    # 13 brown
    (140, 140, 160),  # 14 grey
    (136, 232, 248),  # 15 cyan
]
T, K, W, BLUE, SKIN, RED, DRED, MP, YEL, ORA, GRN, DGRN, PUR, BRN, GRY, CYA = range(16)


def sprite_sheet(name, frames, height):
    w = frames[0].w
    pixels = []
    for f in frames:
        pixels += f.px
    write_bmp(os.path.join(GRAPHICS, name + ".bmp"), pixels, SPRITE_PALETTE)
    write_json(os.path.join(GRAPHICS, name + ".json"), {"type": "sprite", "height": height})
    return w


def player_frame(leg_a=0, leg_b=0, arm=0, crouch=0, climb=False):
    """16x32 hero facing right. leg_a/leg_b shift the legs, arm 0=down 1=up/back 2=forward."""
    c = Canvas(16, 32)
    top = 2 + crouch
    if climb:
        # Seen from behind: hair, back, arms up.
        c.rect(4, top, 8, 9, BRN)
        c.rect(4, top + 9, 8, 11, BLUE)
        c.rect(2, top + 2 + leg_a, 2, 8, SKIN)
        c.rect(12, top + 2 + leg_b, 2, 8, SKIN)
        c.rect(5, top + 20, 2, 8 - leg_a, DRED)
        c.rect(9, top + 20, 2, 8 - leg_b, DRED)
        c.outline(K)
        return c
    c.rect(4, top, 8, 4, BRN)               # hair
    c.rect(4, top + 4, 8, 6, SKIN)           # face
    c.rect(4, top + 4, 2, 3, BRN)            # sideburn
    c.put(10, top + 6, K)                    # eye
    c.rect(3, top + 10, 10, 10 - crouch, BLUE)  # tunic
    c.rect(3, top + 16 - crouch, 10, 1, BRN)    # belt
    leg_y = top + 20 - crouch
    leg_h = 30 - leg_y
    c.rect(4 + leg_a, leg_y, 3, leg_h, DRED)
    c.rect(9 + leg_b, leg_y, 3, leg_h, DRED)
    if arm == 0:
        c.rect(7, top + 11, 3, 7, SKIN)
    elif arm == 1:
        c.rect(5, top + 6, 3, 6, SKIN)
    else:
        c.rect(9, top + 12, 6, 3, SKIN)
    c.outline(K)
    return c


def make_sprites():
    # Player: 0 idle, 1-2 walk, 3 jump, 4-5 climb, 6 attack windup, 7 attack swing, 8 prone, 9 hurt
    frames = [
        player_frame(),
        player_frame(leg_a=-1, leg_b=1),
        player_frame(leg_a=1, leg_b=-1),
        player_frame(leg_a=-1, leg_b=2, arm=1),
        player_frame(climb=True, leg_a=0, leg_b=3),
        player_frame(climb=True, leg_a=3, leg_b=0),
        player_frame(arm=1),
        player_frame(arm=2, leg_a=-1, leg_b=1),
        player_frame(crouch=6),
        player_frame(arm=1, leg_a=1, leg_b=1),
    ]
    sprite_sheet("spr_player", frames, 32)

    # Sword slash (32x32, drawn in front of the player): 0 raised sword, 1 swing arc, 2 fading arc.
    raised = Canvas(32, 32)
    for i in range(12):
        raised.rect(4 + i // 2, 2 + i, 2, 2, W)
    raised.rect(9, 13, 4, 2, BRN)
    raised.outline(K)
    arc = Canvas(32, 32)
    arc.ellipse(10, 16, 18, 14, CYA)
    arc.ellipse(10, 16, 15, 11, W)
    arc.ellipse(4, 16, 15, 12, T)
    arc.outline(K)
    fade = Canvas(32, 32)
    fade.ellipse(10, 16, 18, 14, CYA)
    fade.ellipse(6, 16, 16, 13, T)
    sprite_sheet("spr_slash", [raised, arc, fade], 32)

    # Skill wave (64x32): a wide blue crescent, two frames.
    waves = []
    for grow in (0, 4):
        c = Canvas(64, 32)
        c.ellipse(16 + grow, 16, 40 + grow, 15, MP)
        c.ellipse(16 + grow, 16, 36 + grow, 12, CYA)
        c.ellipse(8 + grow, 16, 36 + grow, 13, T)
        c.outline(K)
        waves.append(c)
    sprite_sheet("spr_skill", waves, 32)

    # Snail: shell spiral + body, facing left.
    snails = []
    for stretch in (0, 1):
        c = Canvas(16, 16)
        c.rect(1 - stretch, 11, 13 + stretch, 4, YEL)
        c.rect(1 - stretch, 7, 3, 4, YEL)
        c.put(1 - stretch, 5, K)
        c.put(3 - stretch, 5, K)
        c.ellipse(9, 8, 5.5, 5.5, ORA)
        c.ellipse(9, 8, 3, 3, BRN)
        c.ellipse(9, 8, 1.5, 1.5, ORA)
        c.put(2 - stretch, 8, K)
        c.outline(K)
        snails.append(c)
    sprite_sheet("spr_snail", snails, 16)

    # Slime: green blob, normal and squashed.
    slimes = []
    for squash in (0, 3):
        c = Canvas(16, 16)
        c.ellipse(8, 10 + squash / 2, 7 + squash / 2, 5.5 - squash / 2, GRN)
        c.ellipse(6, 8 + squash, 2, 1.2, W)
        c.rect(4, 10 + squash // 2, 1, 2, K)
        c.rect(10, 10 + squash // 2, 1, 2, K)
        c.outline(K)
        slimes.append(c)
    sprite_sheet("spr_slime", slimes, 16)

    # Mushroom: orange cap with spots on a stubby stem.
    mushrooms = []
    for bob in (0, 1):
        c = Canvas(16, 16)
        c.rect(4, 8 + bob, 8, 7 - bob, SKIN)
        c.put(5, 10 + bob, K)
        c.put(10, 10 + bob, K)
        c.ellipse(8, 6 + bob, 7.5, 5, ORA)
        c.rect(0, 7 + bob, 16, 2, T)
        c.ellipse(8, 6 + bob, 7.5, 5, ORA)
        c.rect(1, 8 + bob, 14, 1, ORA)
        c.ellipse(5, 4 + bob, 1.5, 1.5, W)
        c.ellipse(11, 5 + bob, 1.5, 1.5, W)
        c.outline(K)
        mushrooms.append(c)
    sprite_sheet("spr_mushroom", mushrooms, 16)

    # Bat: purple, wings up / wings down.
    bats = []
    for up in (True, False):
        c = Canvas(16, 16)
        c.ellipse(8, 8, 3, 3.5, PUR)
        if up:
            c.rect(1, 3, 5, 3, PUR)
            c.rect(10, 3, 5, 3, PUR)
            c.rect(4, 5, 2, 2, PUR)
            c.rect(10, 5, 2, 2, PUR)
        else:
            c.rect(1, 9, 5, 3, PUR)
            c.rect(10, 9, 5, 3, PUR)
            c.rect(4, 8, 2, 2, PUR)
            c.rect(10, 8, 2, 2, PUR)
        c.put(7, 7, RED)
        c.put(9, 7, RED)
        c.outline(K)
        bats.append(c)
    sprite_sheet("spr_bat", bats, 16)

    # Golem boss (64x64): stone body with glowing eyes, two stomp frames.
    golems = []
    for lift in (0, 4):
        c = Canvas(64, 64)
        c.rect(14, 14 - lift // 2, 36, 34, GRY)
        c.rect(18, 4 - lift // 2, 28, 14, GRY)
        c.rect(22, 9 - lift // 2, 6, 4, CYA)
        c.rect(36, 9 - lift // 2, 6, 4, CYA)
        c.rect(26, 22, 12, 4, K)
        c.rect(4, 18 + lift, 10, 22, GRY)
        c.rect(50, 18 - lift, 10, 22, GRY)
        c.rect(16, 48, 12, 15 - lift, GRY)
        c.rect(36, 48, 12, 15, GRY)
        for (x, y) in ((20, 30), (40, 36), (30, 42), (8, 24), (54, 26)):
            c.rect(x, y, 3, 2, K)
        c.outline(K)
        golems.append(c)
    sprite_sheet("spr_golem", golems, 64)

    # Portal: glowing oval, two frames.
    portals = []
    for phase in (0, 1):
        c = Canvas(16, 32)
        c.ellipse(8, 16, 7, 15, PUR if phase else CYA)
        c.ellipse(8, 16, 5, 12, CYA if phase else W)
        c.ellipse(8, 16, 2.5, 8, PUR)
        portals.append(c)
    sprite_sheet("spr_portal", portals, 32)

    # Items: 0 potion, 1 max HP heart, 2 flash jump feather, 3 spring boots.
    potion = Canvas(16, 16)
    potion.ellipse(8, 10, 5, 5, RED)
    potion.rect(6, 2, 4, 4, W)
    potion.rect(6, 1, 4, 2, BRN)
    potion.put(6, 8, W)
    potion.outline(K)
    heart = Canvas(16, 16)
    heart.ellipse(5, 6, 4, 4, RED)
    heart.ellipse(11, 6, 4, 4, RED)
    for i in range(7):
        heart.rect(1 + i, 7 + i, 14 - 2 * i, 1, RED)
    heart.put(4, 4, W)
    heart.outline(K)
    feather = Canvas(16, 16)
    for i in range(12):
        feather.rect(3 + i, 12 - i, 3, 3, CYA)
    for i in range(12):
        feather.put(3 + i, 14 - i, W)
    feather.outline(K)
    boots = Canvas(16, 16)
    boots.rect(3, 2, 5, 9, ORA)
    boots.rect(3, 9, 10, 4, ORA)
    boots.rect(3, 13, 10, 2, BRN)
    boots.rect(4, 4, 3, 1, YEL)
    boots.outline(K)
    sprite_sheet("spr_items", [potion, heart, feather, boots], 16)

    # HUD bar segments (8x8): fill levels 0..8 for red (HP), blue (MP) and yellow (EXP).
    segs = []
    for color in (RED, MP, YEL):
        for fill in range(9):
            c = Canvas(8, 8)
            c.rect(0, 1, 8, 6, K)
            c.rect(0, 2, 8, 4, DRED if color == RED else (BRN if color == YEL else 1))
            c.rect(0, 2, fill, 4, color)
            c.rect(0, 2, fill, 1, W if fill else 0)
            segs.append(c)
    sprite_sheet("spr_bar", segs, 8)

    # Enemy HP bar (16x8): fill levels 0..10.
    minis = []
    for fill in range(11):
        c = Canvas(16, 8)
        c.rect(0, 0, 16, 4, K)
        c.rect(1, 1, 14, 2, DRED)
        c.rect(1, 1, round(14 * fill / 10), 2, RED)
        minis.append(c)
    sprite_sheet("spr_minibar", minis, 8)


# ---------------------------------------------------------------------------------------------------------------------
# Maps
# ---------------------------------------------------------------------------------------------------------------------

EMPTY, SOLID, PLATFORM, ROPE, SPIKE = range(5)

ENEMY_TYPES = ["SNAIL", "SLIME", "MUSHROOM", "BAT", "GOLEM"]
ITEM_TYPES = ["POTION", "HEART", "FLASH_JUMP", "SPRING_BOOTS"]


class Map:
    def __init__(self, name, title, w, h, theme):
        assert (w * 8) % 256 == 0 and (h * 8) % 256 == 0
        self.name = name
        self.title = title
        self.w = w
        self.h = h
        self.theme = theme
        self.t = [[EMPTY] * w for _ in range(h)]
        self.enemies = []
        self.portals = []
        self.items = []
        self.spawn = (0, 0)
        self.decor = []

    def fill(self, x0, y0, x1, y1, v=SOLID):
        """Fills tiles x0..x1-1, y0..y1-1."""
        for y in range(max(0, y0), min(self.h, y1)):
            for x in range(max(0, x0), min(self.w, x1)):
                self.t[y][x] = v

    def platform(self, x0, x1, y):
        self.fill(x0, y, x1, y + 1, PLATFORM)

    def rope(self, x, y0, y1):
        """Rope on column x from row y0 to y1 inclusive. It must hang below a platform tile at row y0 - 1."""
        assert self.t[y0 - 1][x] in (PLATFORM, SOLID), (self.name, x, y0)
        self.fill(x, y0, x + 1, y1 + 1, ROPE)

    def enemy(self, kind, tx, ty):
        """Enemy standing on the surface whose top is row ty (bats: hover around there)."""
        self.enemies.append((ENEMY_TYPES.index(kind), tx * 8 + 4, ty * 8))

    def portal(self, tx, ty, target_map, target_portal):
        self.portals.append((tx * 8 + 4, ty * 8, target_map, target_portal))
        return len(self.portals) - 1

    def item(self, kind, flag, tx, ty):
        self.items.append((ITEM_TYPES.index(kind), flag, tx * 8 + 4, ty * 8))


def build_maps():
    maps = []

    # Map 0: Mossy Meadow (192x64 tiles). Start here. The west chasm needs Flash Jump.
    m = Map("meadow", "Mossy Meadow", 192, 64, "meadow")
    m.fill(0, 0, 3, 64)
    m.fill(189, 0, 192, 64)
    m.fill(3, 56, 21, 64)            # west ledge (cave entrance side)
    m.fill(21, 62, 33, 64)           # chasm floor
    m.fill(21, 61, 32, 62, SPIKE)
    m.fill(32, 61, 33, 62)
    m.fill(33, 56, 189, 64)          # main ground
    m.platform(32, 33, 56)
    m.rope(32, 57, 60)               # climb out of the chasm
    m.fill(70, 53, 92, 56)           # raised hill
    m.platform(76, 86, 49)
    m.platform(84, 94, 45)
    m.platform(104, 126, 46)         # big tree
    m.rope(110, 47, 55)
    m.platform(112, 132, 36)
    m.rope(122, 37, 45)
    m.platform(144, 150, 36)         # secret ledge, Flash Jump from the tree top
    m.platform(150, 162, 52)
    m.platform(166, 176, 48)
    m.spawn = (48 * 8 + 4, 56 * 8)
    m.portal(10, 56, 2, 0)           # -> caves
    m.portal(184, 56, 1, 0)          # -> forest bottom
    for kind, tx, ty in (("SNAIL", 42, 56), ("SNAIL", 60, 56), ("SNAIL", 66, 56), ("SLIME", 80, 53),
                         ("SLIME", 88, 53), ("SNAIL", 80, 49), ("SLIME", 96, 56), ("SLIME", 114, 46),
                         ("MUSHROOM", 118, 36), ("SLIME", 126, 56), ("SNAIL", 140, 56), ("MUSHROOM", 156, 56),
                         ("SLIME", 154, 52), ("MUSHROOM", 170, 56), ("SNAIL", 12, 56)):
        m.enemy(kind, tx, ty)
    m.item("HEART", 0, 147, 36)
    m.decor = [(20, 10, 3), (60, 6, 4), (100, 12, 3), (140, 8, 5), (175, 14, 3), (40, 22, 2), (120, 24, 2)]
    maps.append(m)

    # Map 1: Whispering Forest (96x128 tiles). A tall climb to the Flash Jump feather.
    m = Map("forest", "Whispering Forest", 96, 128, "forest")
    m.fill(0, 0, 3, 128)
    m.fill(93, 0, 96, 128)
    m.fill(3, 124, 93, 128)
    rows = [114, 104, 94, 84, 74, 64, 54, 44, 34]
    left, right = (6, 52), (42, 90)
    for i, row in enumerate(rows):
        x0, x1 = left if i % 2 == 0 else right
        m.platform(x0, x1, row)
    m.platform(30, 66, 24)
    m.rope(28, 115, 123)
    ropes = [46, 48, 47, 49, 45, 50, 46, 48]
    for i, x in enumerate(ropes):
        m.rope(x, rows[i + 1] + 1, rows[i] - 1)
    m.rope(40, 25, 33)
    m.platform(79, 86, 24)            # heart ledge: Flash Jump from the east end of the top platform
    m.spawn = (8 * 8, 124 * 8)
    m.portal(8, 124, 0, 1)           # -> meadow east
    m.portal(60, 24, 0, 1)           # shortcut back down from the top
    for i, row in enumerate(rows):
        x0, x1 = left if i % 2 == 0 else right
        kinds = ["SLIME", "MUSHROOM", "SNAIL"]
        m.enemy(kinds[i % 3], x0 + 4, row)
        if i % 2 == 1:
            m.enemy("MUSHROOM", x1 - 6, row)
    m.enemy("SLIME", 40, 124)
    m.enemy("MUSHROOM", 70, 124)
    m.enemy("BAT", 30, 60)
    m.enemy("BAT", 70, 40)
    m.item("FLASH_JUMP", 1, 48, 24)
    m.item("HEART", 2, 83, 24)
    maps.append(m)

    # Map 2: Crystal Caves (192x96 tiles). Spring Boots at the bottom, boss portal up a tall shaft.
    m = Map("caves", "Crystal Caves", 192, 96, "caves")
    m.fill(0, 0, 192, 4)
    m.fill(0, 0, 3, 96)
    m.fill(189, 0, 192, 96)
    m.fill(3, 92, 189, 96)
    m.platform(3, 31, 20)            # entry ledge
    m.platform(24, 62, 30)
    m.rope(26, 21, 29)
    m.rope(32, 31, 91)               # long rope back up from the floor
    m.platform(56, 92, 42)
    m.platform(86, 122, 54)
    m.platform(116, 150, 66)
    m.platform(60, 100, 78)
    m.rope(64, 79, 91)
    m.fill(66, 91, 74, 92, SPIKE)
    m.fill(104, 91, 114, 92, SPIKE)
    m.fill(130, 91, 138, 92, SPIKE)
    m.fill(140, 80, 150, 92)         # rock in the way
    # Spring Boots shaft (one-way ledges 8 rows apart, too high for a normal jump).
    shaft = [84, 76, 68, 60, 52, 44, 36, 28]
    for i, row in enumerate(shaft):
        if i % 2 == 0:
            m.platform(172, 189, row)
        else:
            m.platform(158, 175, row)
    m.platform(162, 189, 20)
    m.platform(125, 131, 46)         # heart ledge, Spring Boots jump from the end of row 54
    m.spawn = (8 * 8, 20 * 8)
    m.portal(8, 20, 0, 0)            # -> meadow west
    m.portal(184, 20, 3, 0)          # -> boss
    for kind, tx, ty in (("MUSHROOM", 40, 30), ("BAT", 50, 24), ("MUSHROOM", 70, 42), ("BAT", 90, 34),
                         ("MUSHROOM", 100, 54), ("BAT", 110, 46), ("MUSHROOM", 130, 66), ("BAT", 140, 60),
                         ("MUSHROOM", 80, 78), ("SLIME", 20, 92), ("SLIME", 50, 92), ("MUSHROOM", 90, 92),
                         ("MUSHROOM", 120, 92), ("BAT", 160, 84), ("SLIME", 175, 92), ("BAT", 170, 50),
                         ("BAT", 165, 30)):
        m.enemy(kind, tx, ty)
    m.item("SPRING_BOOTS", 3, 182, 92)
    m.item("HEART", 4, 128, 46)
    m.decor = [(30, 50, 2), (80, 70, 3), (120, 30, 2), (150, 75, 3), (20, 70, 2), (100, 86, 2), (175, 10, 2)]
    maps.append(m)

    # Map 3: Golem Hall (64x32 tiles). The boss.
    m = Map("boss", "Golem Hall", 64, 32, "boss")
    m.fill(0, 0, 64, 2)
    m.fill(0, 0, 3, 32)
    m.fill(61, 0, 64, 32)
    m.fill(3, 28, 61, 32)
    m.platform(10, 20, 21)
    m.platform(44, 54, 21)
    m.platform(27, 37, 15)
    m.spawn = (8 * 8, 28 * 8)
    m.portal(6, 28, 2, 1)            # -> caves
    m.enemy("GOLEM", 46, 28)
    maps.append(m)

    return maps


THEMES = {
    # sky gradient (5), ground fill, ground speck, surface, surface dark, wood, wood dark, rope, spike, spike dark, decor
    "meadow": [(120, 192, 248), (136, 200, 248), (152, 212, 248), (176, 224, 248), (200, 236, 240),
               (136, 88, 56), (104, 64, 40), (96, 192, 72), (48, 136, 56), (200, 144, 88), (120, 80, 48),
               (232, 216, 160), (220, 220, 232), (112, 112, 136), (248, 248, 248)],
    "forest": [(40, 88, 72), (48, 104, 80), (56, 120, 88), (72, 136, 96), (88, 152, 104),
               (88, 64, 48), (64, 48, 32), (72, 168, 64), (32, 112, 48), (184, 128, 80), (112, 72, 40),
               (216, 200, 144), (220, 220, 232), (112, 112, 136), (32, 72, 56)],
    "caves": [(16, 16, 40), (20, 24, 52), (28, 32, 64), (36, 40, 76), (44, 48, 88),
              (72, 72, 96), (52, 52, 72), (120, 120, 152), (88, 88, 120), (168, 120, 80), (104, 72, 40),
              (208, 192, 136), (232, 232, 248), (120, 120, 144), (120, 224, 248)],
    "boss": [(48, 16, 32), (64, 20, 40), (80, 24, 44), (96, 28, 48), (112, 32, 52),
             (80, 64, 72), (56, 44, 52), (144, 120, 128), (104, 88, 96), (168, 120, 80), (104, 72, 40),
             (208, 192, 136), (232, 232, 248), (120, 120, 144), (248, 160, 64)],
}
SKY0 = 1
GROUND, SPECK, SURF, SURF_DARK, WOOD, WOOD_DARK, ROPE_C, SPIKE_C, SPIKE_DARK, DECOR = range(6, 16)


def draw_map(m):
    W, H = m.w * 8, m.h * 8
    c = Canvas(W, H)
    band_rows = max(1, m.h // 5)
    for ty in range(m.h):
        band = SKY0 + min(4, ty // band_rows)
        c.rect(0, ty * 8, W, 8, band)

    # Decorations in the sky (clouds / crystals), tile aligned so they stay cheap.
    for (tx, ty, size) in m.decor:
        if m.theme == "meadow":
            c.ellipse(tx * 8 + 4, ty * 8 + 4, size * 6, size * 2.5, DECOR)
            c.ellipse(tx * 8 + size * 4, ty * 8, size * 4, size * 2.5, DECOR)
        else:
            for i in range(size):
                c.rect(tx * 8 + i * 8, ty * 8 - i * 4, 4, 8 + i * 4, DECOR)

    # Forest: background tree trunks.
    if m.theme == "forest":
        for tx in range(8, m.w - 4, 14):
            c.rect(tx * 8 + 2, 0, 12, H, SKY0 + 4 if (tx // 14) % 2 else SKY0)

    for ty in range(m.h):
        for tx in range(m.w):
            v = m.t[ty][tx]
            x, y = tx * 8, ty * 8
            if v == SOLID:
                c.rect(x, y, 8, 8, GROUND)
                if (tx * 7 + ty * 13) % 5 == 0:
                    c.rect(x + 2, y + 3, 2, 2, SPECK)
                if ty > 0 and m.t[ty - 1][tx] != SOLID:
                    c.rect(x, y, 8, 3, SURF)
                    c.rect(x, y + 3, 8, 1, SURF_DARK)
                    if tx % 3 == 0:
                        c.put(x + 3, y + 4, SURF_DARK)
            elif v == PLATFORM:
                c.rect(x, y, 8, 3, WOOD)
                c.rect(x, y + 3, 8, 1, WOOD_DARK)
                c.put(x + 7, y + 1, WOOD_DARK)
            elif v == ROPE:
                c.rect(x + 3, y, 2, 8, ROPE_C)
                c.rect(x + 3, y + 2, 2, 1, WOOD_DARK)
                c.rect(x + 3, y + 6, 2, 1, WOOD_DARK)
            elif v == SPIKE:
                for sx in (0, 4):
                    for i in range(4):
                        c.rect(x + sx + 2 - i // 2, y + i * 2, 1 + (i // 2) * 2, 2, SPIKE_C if i < 2 else SPIKE_DARK)

    palette = [(255, 0, 255)] + THEMES[m.theme]
    write_bmp(os.path.join(GRAPHICS, "map_" + m.name + ".bmp"), c.px, palette)
    write_json(os.path.join(GRAPHICS, "map_" + m.name + ".json"), {"type": "regular_bg"})


def write_level_data(maps):
    out = []
    out.append("// Generated by tools/gen_assets.py. Do not edit by hand.\n")
    out.append('#include "lb_level_data.h"\n')
    for m in maps:
        out.append('#include "bn_regular_bg_items_map_%s.h"' % m.name)
    out.append("\nnamespace lb\n{\n\nnamespace\n{\n")
    for m in maps:
        out.append("    constexpr uint8_t %s_tiles[] = {" % m.name)
        for row in m.t:
            out.append("        " + ",".join(str(v) for v in row) + ",")
        out.append("    };\n")
        out.append("    constexpr enemy_spawn %s_enemies[] = {" % m.name)
        for (k, x, y) in m.enemies:
            out.append("        { enemy_type::%s, %d, %d }," % (ENEMY_TYPES[k], x, y))
        out.append("    };\n")
        out.append("    constexpr portal_def %s_portals[] = {" % m.name)
        for (x, y, tm, tp) in m.portals:
            out.append("        { %d, %d, %d, %d }," % (x, y, tm, tp))
        out.append("    };\n")
        if m.items:
            out.append("    constexpr item_def %s_items[] = {" % m.name)
            for (k, flag, x, y) in m.items:
                out.append("        { item_type::%s, %d, %d, %d }," % (ITEM_TYPES[k], flag, x, y))
            out.append("    };\n")
    out.append("    constexpr map_def maps[] = {")
    for m in maps:
        items = "bn::span<const item_def>(%s_items)" % m.name if m.items else "bn::span<const item_def>()"
        out.append('        { "%s", bn::regular_bg_items::map_%s, %d, %d, %s_tiles, %d, %d,\n'
                   '          bn::span<const enemy_spawn>(%s_enemies), bn::span<const portal_def>(%s_portals),\n'
                   '          %s },'
                   % (m.title, m.name, m.w, m.h, m.name, m.spawn[0], m.spawn[1], m.name, m.name, items))
    out.append("    };\n}\n")
    out.append("const map_def& map_at(int index)\n{\n    return maps[index];\n}\n")
    out.append("int maps_count()\n{\n    return %d;\n}\n\n}" % len(maps))
    with open(os.path.join(SRC, "lb_level_data.cpp"), "w") as f:
        f.write("\n".join(out) + "\n")


def main():
    os.makedirs(GRAPHICS, exist_ok=True)
    make_sprites()
    maps = build_maps()
    for m in maps:
        draw_map(m)
    write_level_data(maps)


if __name__ == "__main__":
    main()
