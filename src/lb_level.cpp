#include "lb_level.h"

namespace lb
{

level::level(int map_index, const bn::camera_ptr& camera) :
    _index(map_index),
    _def(&map_at(map_index)),
    _bg(_def->bg.create_bg(_def->width * 4, _def->height * 4))
{
    _bg.set_camera(camera);
    _bg.set_priority(3);
}

}
