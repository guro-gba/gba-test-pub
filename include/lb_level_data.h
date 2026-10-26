#ifndef LB_LEVEL_DATA_H
#define LB_LEVEL_DATA_H

#include "bn_span.h"
#include "bn_regular_bg_item.h"

namespace lb
{
    enum tile_type : uint8_t
    {
        TILE_EMPTY,
        TILE_SOLID,
        TILE_PLATFORM,
        TILE_ROPE,
        TILE_SPIKE
    };

    enum class enemy_type : uint8_t
    {
        SNAIL,
        SLIME,
        MUSHROOM,
        BAT,
        GOLEM
    };

    enum class item_type : uint8_t
    {
        POTION,
        HEART,
        FLASH_JUMP,
        SPRING_BOOTS
    };

    // Positions are in map pixels: x is the horizontal center, y is the bottom (feet).
    struct enemy_spawn
    {
        enemy_type type;
        int16_t x;
        int16_t y;
    };

    struct portal_def
    {
        int16_t x;
        int16_t y;
        int8_t target_map;
        int8_t target_portal;
    };

    // flag identifies the item in the save state so it is only collected once.
    struct item_def
    {
        item_type type;
        uint8_t flag;
        int16_t x;
        int16_t y;
    };

    struct map_def
    {
        const char* name;
        const bn::regular_bg_item& bg;
        int width;
        int height;
        const uint8_t* tiles;
        int spawn_x;
        int spawn_y;
        bn::span<const enemy_spawn> enemies;
        bn::span<const portal_def> portals;
        bn::span<const item_def> items;
    };

    [[nodiscard]] const map_def& map_at(int index);

    [[nodiscard]] int maps_count();
}

#endif
