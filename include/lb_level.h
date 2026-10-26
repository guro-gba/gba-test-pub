#ifndef LB_LEVEL_H
#define LB_LEVEL_H

#include "bn_fixed.h"
#include "bn_camera_ptr.h"
#include "bn_regular_bg_ptr.h"

#include "lb_level_data.h"

namespace lb
{
    class level
    {

    public:
        level(int map_index, const bn::camera_ptr& camera);

        [[nodiscard]] int index() const
        {
            return _index;
        }

        [[nodiscard]] const map_def& def() const
        {
            return *_def;
        }

        [[nodiscard]] int width() const
        {
            return _def->width * 8;
        }

        [[nodiscard]] int height() const
        {
            return _def->height * 8;
        }

        // Outside the map: walls on the sides and bottom, open sky above.
        [[nodiscard]] int tile(int tile_x, int tile_y) const
        {
            if(tile_x < 0 || tile_x >= _def->width || tile_y >= _def->height)
            {
                return TILE_SOLID;
            }

            if(tile_y < 0)
            {
                return TILE_EMPTY;
            }

            return _def->tiles[tile_y * _def->width + tile_x];
        }

        [[nodiscard]] int tile_at(bn::fixed x, bn::fixed y) const
        {
            return tile(x.floor_integer() >> 3, y.floor_integer() >> 3);
        }

    private:
        int _index;
        const map_def* _def;
        bn::regular_bg_ptr _bg;
    };
}

#endif
