#include "bn_core.h"

#include "lb_game.h"
#include "lb_stats.h"
#include "lb_scenes.h"

int main()
{
    bn::core::init();

    while(true)
    {
        lb::title_scene();

        lb::stats st;
        int frames = 0;
        int map_index = 0;
        int portal_index = -1;

        while(lb::play(st, map_index, portal_index, frames) == lb::play_result::DIED)
        {
            lb::game_over_scene();

            // Wake up in town with full health, losing some experience.
            st.hp = st.max_hp;
            st.mp = st.max_mp;
            st.exp -= st.exp / 10;
            map_index = 0;
            portal_index = -1;
        }

        lb::victory_scene(st, frames);
    }
}
