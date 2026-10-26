#ifndef LB_GAME_H
#define LB_GAME_H

namespace lb
{
    struct stats;

    enum class play_result
    {
        DIED,
        VICTORY
    };

    // Plays from the given map (portal -1 means the map spawn point) until the player dies or beats the boss.
    [[nodiscard]] play_result play(stats& st, int map_index, int portal_index, int& frames);

    void fade_out();

    void fade_in();
}

#endif
