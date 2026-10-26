#include "lb_fx.h"

#include "bn_string.h"

#include "common_fixed_8x8_sprite_font.h"

namespace lb
{

namespace
{
    constexpr int popup_frames = 45;
}

fx::fx(const bn::camera_ptr& camera) :
    _text_generator(common::fixed_8x8_sprite_font)
{
    _text_generator.set_center_alignment();
    _text_generator.set_camera(camera);
    _text_generator.set_z_order(2);
}

void fx::add_number(bn::fixed x, bn::fixed y, int value, bool critical)
{
    bn::string<12> text = bn::to_string<8>(value);

    if(critical)
    {
        text.push_back('!');
    }

    add_text(x, y, text);
}

void fx::add_text(bn::fixed x, bn::fixed y, const bn::string_view& text)
{
    if(_popups.full())
    {
        _popups.erase(_popups.begin());
    }

    popup& new_popup = _popups.emplace_back();
    _text_generator.generate(x, y, text, new_popup.sprites);
}

void fx::update()
{
    for(auto it = _popups.begin(); it != _popups.end();)
    {
        popup& current = *it;
        ++current.frames;

        if(current.frames >= popup_frames)
        {
            it = _popups.erase(it);
            continue;
        }

        if(current.frames < 24)
        {
            for(bn::sprite_ptr& sprite : current.sprites)
            {
                sprite.set_y(sprite.y() - bn::fixed(0.75));
            }
        }

        if(current.frames > 32)
        {
            for(bn::sprite_ptr& sprite : current.sprites)
            {
                sprite.set_visible((current.frames >> 1) & 1);
            }
        }

        ++it;
    }
}

}
