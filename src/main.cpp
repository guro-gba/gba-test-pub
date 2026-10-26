#include "bn_core.h"
#include "bn_vector.h"
#include "bn_keypad.h"
#include "bn_display.h"
#include "bn_algorithm.h"
#include "bn_sprite_ptr.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_text_generator.h"

#include "bn_sprite_items_player.h"

#include "common_variable_8x16_sprite_font.h"

namespace
{
    constexpr bn::fixed speed = 2;
    constexpr bn::fixed half_size = 16;
    constexpr bn::fixed max_x = (bn::display::width() / 2) - half_size;
    constexpr bn::fixed max_y = (bn::display::height() / 2) - half_size;
}

int main()
{
    bn::core::init();

    bn::bg_palettes::set_transparent_color(bn::color(21, 30, 25));

    bn::sprite_text_generator text_generator(common::variable_8x16_sprite_font);
    text_generator.set_center_alignment();

    bn::vector<bn::sprite_ptr, 16> text_sprites;
    text_generator.generate(0, -64, "Hello, Bodhi!", text_sprites);
    text_generator.generate(0, 64, "D-Pad: move   A: reset", text_sprites);

    bn::sprite_ptr player = bn::sprite_items::player.create_sprite(0, 0);

    while(true)
    {
        bn::fixed x = player.x();
        bn::fixed y = player.y();

        if(bn::keypad::left_held())
        {
            x -= speed;
        }
        else if(bn::keypad::right_held())
        {
            x += speed;
        }

        if(bn::keypad::up_held())
        {
            y -= speed;
        }
        else if(bn::keypad::down_held())
        {
            y += speed;
        }

        if(bn::keypad::a_pressed())
        {
            x = 0;
            y = 0;
        }

        player.set_position(bn::clamp(x, -max_x, max_x), bn::clamp(y, -max_y, max_y));
        bn::core::update();
    }
}
