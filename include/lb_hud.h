#ifndef LB_HUD_H
#define LB_HUD_H

#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"

namespace lb
{
    struct stats;

    class hud
    {

    public:
        hud();

        void update(const stats& st);

        void set_boss_hp(int hp, int max_hp);

        void hide_boss();

        // Shows a centered message for a while (two lines, the second may be empty).
        void show_message(const bn::string_view& line_1, const bn::string_view& line_2, int frames);

        void set_visible(bool visible);

    private:
        static constexpr int hp_segments = 8;
        static constexpr int mp_segments = 6;
        static constexpr int exp_segments = 12;
        static constexpr int boss_segments = 16;

        bn::sprite_text_generator _small_text;
        bn::sprite_text_generator _big_text;
        bn::vector<bn::sprite_ptr, hp_segments> _hp_bar;
        bn::vector<bn::sprite_ptr, mp_segments> _mp_bar;
        bn::vector<bn::sprite_ptr, exp_segments> _exp_bar;
        bn::vector<bn::sprite_ptr, boss_segments> _boss_bar;
        bn::vector<bn::sprite_ptr, 8> _labels;
        bn::vector<bn::sprite_ptr, 4> _level_text;
        bn::vector<bn::sprite_ptr, 4> _potion_text;
        bn::vector<bn::sprite_ptr, 12> _message_text;
        int _shown_level = -1;
        int _shown_potions = -1;
        int _message_frames = 0;
        bool _visible = true;
    };
}

#endif
