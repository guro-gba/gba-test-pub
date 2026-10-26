#include "lb_physics.h"

#include "lb_level.h"

namespace lb
{

namespace
{
    [[nodiscard]] int left_column(const body& b)
    {
        return (b.x - b.half_width).floor_integer() >> 3;
    }

    [[nodiscard]] int right_column(const body& b)
    {
        return ((b.x + b.half_width).floor_integer() - 1) >> 3;
    }

    [[nodiscard]] bool row_has(const level& lvl, int first_column, int last_column, int row, int tile)
    {
        for(int column = first_column; column <= last_column; ++column)
        {
            if(lvl.tile(column, row) == tile)
            {
                return true;
            }
        }

        return false;
    }

    void move_x(const level& lvl, body& b)
    {
        b.hit_wall = false;

        if(b.vx == 0)
        {
            return;
        }

        bn::fixed new_x = b.x + b.vx;
        int top_row = b.top().floor_integer() >> 3;
        int bottom_row = (b.y.floor_integer() - 1) >> 3;

        if(b.vx > 0)
        {
            int column = ((new_x + b.half_width).floor_integer() - 1) >> 3;

            for(int row = top_row; row <= bottom_row; ++row)
            {
                if(lvl.tile(column, row) == TILE_SOLID)
                {
                    new_x = (column * 8) - b.half_width;
                    b.vx = 0;
                    b.hit_wall = true;
                    break;
                }
            }
        }
        else
        {
            int column = (new_x - b.half_width).floor_integer() >> 3;

            for(int row = top_row; row <= bottom_row; ++row)
            {
                if(lvl.tile(column, row) == TILE_SOLID)
                {
                    new_x = ((column + 1) * 8) + b.half_width;
                    b.vx = 0;
                    b.hit_wall = true;
                    break;
                }
            }
        }

        b.x = new_x;
    }

    void move_y(const level& lvl, body& b, bool ignore_platforms)
    {
        b.on_ground = false;

        int first_column = left_column(b);
        int last_column = right_column(b);

        if(b.vy >= 0)
        {
            bn::fixed new_y = b.y + b.vy;
            int row = new_y.floor_integer() >> 3;

            // Land only when the feet cross (or rest on) the top of the row this frame.
            if(row * 8 >= b.y)
            {
                if(row_has(lvl, first_column, last_column, row, TILE_SOLID) ||
                        (! ignore_platforms && row_has(lvl, first_column, last_column, row, TILE_PLATFORM)))
                {
                    b.y = row * 8;
                    b.vy = 0;
                    b.on_ground = true;
                    return;
                }
            }

            b.y = new_y;
        }
        else
        {
            bn::fixed new_y = b.y + b.vy;
            int row = (new_y - b.height).floor_integer() >> 3;

            if(row_has(lvl, first_column, last_column, row, TILE_SOLID))
            {
                b.y = ((row + 1) * 8) + b.height;
                b.vy = 0;
                return;
            }

            b.y = new_y;
        }
    }
}

void move_body(const level& lvl, body& b, bool ignore_platforms)
{
    move_x(lvl, b);
    move_y(lvl, b, ignore_platforms);
}

bool ground_at(const level& lvl, bn::fixed x, bn::fixed y)
{
    int tile = lvl.tile_at(x, y + 1);
    return tile == TILE_SOLID || tile == TILE_PLATFORM;
}

bool on_platform(const level& lvl, const body& b)
{
    int row = (b.y.floor_integer() + 1) >> 3;
    int first_column = left_column(b);
    int last_column = right_column(b);
    return row_has(lvl, first_column, last_column, row, TILE_PLATFORM) &&
            ! row_has(lvl, first_column, last_column, row, TILE_SOLID);
}

bool touches_tile(const level& lvl, const body& b, int tile)
{
    int first_column = left_column(b);
    int last_column = right_column(b);
    int top_row = b.top().floor_integer() >> 3;
    int bottom_row = (b.y.floor_integer() - 1) >> 3;

    for(int row = top_row; row <= bottom_row; ++row)
    {
        if(row_has(lvl, first_column, last_column, row, tile))
        {
            return true;
        }
    }

    return false;
}

}
