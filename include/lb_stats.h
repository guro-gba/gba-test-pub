#ifndef LB_STATS_H
#define LB_STATS_H

#include "bn_algorithm.h"

namespace lb
{
    // Character progression and everything collected so far.
    struct stats
    {
        static constexpr int max_level = 30;
        static constexpr int max_potions = 9;
        static constexpr int skill_mp_cost = 8;
        static constexpr int potion_heal = 60;

        int level = 1;
        int exp = 0;
        int max_hp = 60;
        int hp = 60;
        int max_mp = 30;
        int mp = 30;
        int potions = 3;
        bool flash_jump = false;
        bool spring_boots = false;
        unsigned collected_flags = 0;

        [[nodiscard]] int exp_to_next() const
        {
            return 10 + (level * level * 6);
        }

        [[nodiscard]] int attack() const
        {
            return 12 + ((level - 1) * 3);
        }

        [[nodiscard]] bool collected(int flag) const
        {
            return collected_flags & (1u << flag);
        }

        void collect(int flag)
        {
            collected_flags |= 1u << flag;
        }

        // Returns the number of levels gained.
        int add_exp(int amount)
        {
            int levels = 0;
            exp += amount;

            while(level < max_level && exp >= exp_to_next())
            {
                exp -= exp_to_next();
                ++level;
                ++levels;
                max_hp += 12;
                max_mp += 6;
                hp = max_hp;
                mp = max_mp;
            }

            if(level == max_level)
            {
                exp = bn::min(exp, exp_to_next());
            }

            return levels;
        }

        void heal(int amount)
        {
            hp = bn::min(max_hp, hp + amount);
        }
    };
}

#endif
