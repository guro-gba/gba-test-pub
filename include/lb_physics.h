#ifndef LB_PHYSICS_H
#define LB_PHYSICS_H

#include "bn_fixed_rect.h"

namespace lb
{
    class level;

    constexpr bn::fixed gravity = 0.22;
    constexpr bn::fixed max_fall_speed = 5;

    // An axis aligned box that collides with the map. x is the center, y is the bottom.
    struct body
    {
        bn::fixed x;
        bn::fixed y;
        bn::fixed vx;
        bn::fixed vy;
        int half_width = 6;
        int height = 30;
        bool on_ground = false;
        bool hit_wall = false;

        [[nodiscard]] bn::fixed_rect rect() const
        {
            return bn::fixed_rect(x, y - (height / 2), half_width * 2, height);
        }

        [[nodiscard]] bn::fixed top() const
        {
            return y - height;
        }
    };

    // Moves the body by its velocity, resolving collisions with solid tiles and (unless ignored) one-way platforms.
    void move_body(const level& lvl, body& b, bool ignore_platforms);

    // Indicates if the tile below the given foot position can be stood on.
    [[nodiscard]] bool ground_at(const level& lvl, bn::fixed x, bn::fixed y);

    // Indicates if the body stands on a one-way platform it could drop through.
    [[nodiscard]] bool on_platform(const level& lvl, const body& b);

    // Indicates if the body overlaps any tile of the given type.
    [[nodiscard]] bool touches_tile(const level& lvl, const body& b, int tile);
}

#endif
