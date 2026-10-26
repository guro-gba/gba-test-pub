#ifndef LB_SCENES_H
#define LB_SCENES_H

namespace lb
{
    struct stats;

    void title_scene();

    void game_over_scene();

    void victory_scene(const stats& st, int frames);
}

#endif
