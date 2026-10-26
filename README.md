# gba-test-pub

A small Game Boy Advance project built with [Butano](https://github.com/GValiente/butano) (21.9.0).
It shows a title and a sprite you can move around the screen.

## Controls

- **D-Pad**: move the sprite
- **A**: return the sprite to the center

## Building

1. Clone with the Butano submodule:

   ```sh
   git clone --recurse-submodules https://github.com/guro-gba/gba-test-pub.git
   # or, in an existing clone:
   git submodule update --init
   ```

2. Install a GBA toolchain: [devkitARM](https://devkitpro.org/wiki/Getting_Started)
   (`DEVKITARM` set) or [Wonderful Toolchain](https://wonderful.asie.pl/) (`WONDERFUL_TOOLCHAIN` set),
   plus Python 3.

3. Build:

   ```sh
   make -j$(nproc)
   ```

   This produces `gba-test-pub.gba`, which runs in any GBA emulator (mGBA, NanoBoyAdvance, ...)
   or on hardware via a flash cart.

CI builds the ROM on every push (see `.github/workflows/build.yml`) and uploads it as an artifact.

## Layout

| Path | Contents |
| --- | --- |
| `src/` | C++ sources (`main.cpp`) |
| `include/` | Project headers |
| `graphics/` | Images (indexed BMP + JSON descriptor per asset) |
| `audio/`, `dmg_audio/` | Music and sound effects (empty for now) |
| `butano/` | Butano engine, as a git submodule |
| `tools/make_player_bmp.py` | Regenerates `graphics/player.bmp` |
