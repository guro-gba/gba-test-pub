#include "lb_scenes.h"

#include "bn_core.h"
#include "bn_string.h"
#include "bn_keypad.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_text_generator.h"

#include "bn_sprite_items_spr_player.h"
#include "bn_sprite_items_spr_snail.h"
#include "bn_sprite_items_spr_slime.h"
#include "bn_sprite_items_spr_mushroom.h"

#include "common_fixed_8x8_sprite_font.h"
#include "common_variable_8x16_sprite_font.h"
#include "common_variable_16x16_sprite_font.h"

#include "lb_game.h"
#include "lb_stats.h"

namespace lb
{

namespace
{
    void wait_for_start(bn::ivector<bn::sprite_ptr>& blinking_sprites)
    {
        int frames = 0;

        while(! bn::keypad::start_pressed() || frames < 20)
        {
            ++frames;

            for(bn::sprite_ptr& sprite : blinking_sprites)
            {
                sprite.set_visible(((frames >> 5) & 1) == 0);
            }

            bn::core::update();
        }
    }
}

void title_scene()
{
    bn::bg_palettes::set_transparent_color(bn::color(15, 24, 31));

    bn::sprite_text_generator big_text(common::variable_16x16_sprite_font);
    bn::sprite_text_generator text(common::variable_8x16_sprite_font);
    bn::sprite_text_generator small_text(common::fixed_8x8_sprite_font);
    big_text.set_center_alignment();
    text.set_center_alignment();
    small_text.set_center_alignment();

    bn::vector<bn::sprite_ptr, 16> title_sprites;
    bn::vector<bn::sprite_ptr, 8> start_sprites;
    bn::vector<bn::sprite_ptr, 40> info_sprites;
    big_text.generate(0, -56, "LEAFBOUND", title_sprites);
    text.generate(0, -36, "A tiny side-scrolling adventure", info_sprites);
    text.generate(0, 0, "PRESS START", start_sprites);
    small_text.generate(0, 28, "D-Pad move/climb  A jump", info_sprites);
    small_text.generate(0, 40, "B attack  R skill  L potion", info_sprites);
    small_text.generate(0, 52, "Up at a portal to enter", info_sprites);

    bn::vector<bn::sprite_ptr, 4> actors;
    actors.push_back(bn::sprite_items::spr_player.create_sprite(-40, -10));
    actors.push_back(bn::sprite_items::spr_snail.create_sprite(-80, -2));
    actors.push_back(bn::sprite_items::spr_slime.create_sprite(64, -2));
    actors.push_back(bn::sprite_items::spr_mushroom.create_sprite(88, -2));

    fade_in();
    wait_for_start(start_sprites);
    fade_out();
}

void game_over_scene()
{
    bn::bg_palettes::set_transparent_color(bn::color(4, 0, 2));

    bn::sprite_text_generator big_text(common::variable_16x16_sprite_font);
    bn::sprite_text_generator text(common::variable_8x16_sprite_font);
    big_text.set_center_alignment();
    text.set_center_alignment();

    bn::vector<bn::sprite_ptr, 16> sprites;
    bn::vector<bn::sprite_ptr, 8> start_sprites;
    big_text.generate(0, -32, "YOU FELL", sprites);
    text.generate(0, 0, "You wake up in Mossy Meadow,", sprites);
    text.generate(0, 14, "a little less experienced.", sprites);
    text.generate(0, 48, "PRESS START", start_sprites);

    fade_in();
    wait_for_start(start_sprites);
    fade_out();
}

void victory_scene(const stats& st, int frames)
{
    bn::bg_palettes::set_transparent_color(bn::color(6, 14, 8));

    bn::sprite_text_generator big_text(common::variable_16x16_sprite_font);
    bn::sprite_text_generator text(common::variable_8x16_sprite_font);
    big_text.set_center_alignment();
    text.set_center_alignment();

    int seconds = frames / 60;
    int hearts = int(st.collected(0)) + int(st.collected(2)) + int(st.collected(4));
    bn::string<32> level_line = "Level ";
    level_line += bn::to_string<4>(st.level);
    level_line += "   Heart Crystals ";
    level_line += bn::to_string<4>(hearts);
    level_line += "/3";

    bn::string<32> time_line = "Time ";
    time_line += bn::to_string<4>(seconds / 60);
    time_line += ":";

    if(seconds % 60 < 10)
    {
        time_line += "0";
    }

    time_line += bn::to_string<4>(seconds % 60);

    bn::vector<bn::sprite_ptr, 40> sprites;
    bn::vector<bn::sprite_ptr, 8> start_sprites;
    big_text.generate(0, -50, "VICTORY!", sprites);
    text.generate(0, -20, "The Golem is defeated and", sprites);
    text.generate(0, -6, "the caves are quiet again.", sprites);
    text.generate(0, 20, level_line, sprites);
    text.generate(0, 34, time_line, sprites);
    text.generate(0, 62, "PRESS START", start_sprites);

    fade_in();
    wait_for_start(start_sprites);
    fade_out();
}

}
