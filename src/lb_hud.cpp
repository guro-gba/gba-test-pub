#include "lb_hud.h"

#include "bn_string.h"
#include "bn_display.h"

#include "bn_sprite_items_spr_bar.h"
#include "bn_sprite_items_spr_items.h"

#include "common_fixed_8x8_sprite_font.h"
#include "common_variable_8x16_sprite_font.h"

#include "lb_stats.h"

namespace lb
{

namespace
{
    enum bar_color
    {
        BAR_RED = 0,
        BAR_BLUE = 9,
        BAR_YELLOW = 18
    };

    constexpr int bottom_y = (bn::display::height() / 2) - 6;
    constexpr int exp_y = (bn::display::height() / 2) - 16;
    constexpr int left_x = -(bn::display::width() / 2);

    template<int Size>
    void create_bar(bn::vector<bn::sprite_ptr, Size>& bar, int left, int y)
    {
        for(int index = 0; index < Size; ++index)
        {
            bn::sprite_ptr segment = bn::sprite_items::spr_bar.create_sprite(left + (index * 8) + 4, y);
            segment.set_bg_priority(0);
            segment.set_z_order(1);
            bar.push_back(bn::move(segment));
        }
    }

    template<int Size>
    void fill_bar(bn::vector<bn::sprite_ptr, Size>& bar, int value, int max_value, int color)
    {
        int pixels = max_value > 0 ? (value * Size * 8 + max_value - 1) / max_value : 0;

        if(value <= 0)
        {
            pixels = 0;
        }

        for(int index = 0; index < bar.size(); ++index)
        {
            int fill = bn::clamp(pixels - (index * 8), 0, 8);
            bar[index].set_tiles(bn::sprite_items::spr_bar.tiles_item(), color + fill);
        }
    }

    template<int Size>
    void set_bar_visible(bn::vector<bn::sprite_ptr, Size>& bar, bool visible)
    {
        for(bn::sprite_ptr& segment : bar)
        {
            segment.set_visible(visible);
        }
    }
}

hud::hud() :
    _small_text(common::fixed_8x8_sprite_font),
    _big_text(common::variable_8x16_sprite_font)
{
    _small_text.set_bg_priority(0);
    _small_text.set_z_order(0);
    _big_text.set_bg_priority(0);
    _big_text.set_z_order(0);
    _big_text.set_center_alignment();

    _small_text.generate(left_x + 40, bottom_y, "HP", _labels);
    _small_text.generate(left_x + 128, bottom_y, "MP", _labels);
    _small_text.generate(left_x + 40, exp_y, "EX", _labels);

    create_bar(_hp_bar, left_x + 58, bottom_y);
    create_bar(_mp_bar, left_x + 146, bottom_y);
    create_bar(_exp_bar, left_x + 58, exp_y);

    bn::sprite_ptr potion_icon = bn::sprite_items::spr_items.create_sprite(left_x + 204, bottom_y - 2, 0);
    potion_icon.set_bg_priority(0);
    potion_icon.set_z_order(1);
    _labels.push_back(bn::move(potion_icon));
}

void hud::update(const stats& st)
{
    fill_bar(_hp_bar, st.hp, st.max_hp, BAR_RED);
    fill_bar(_mp_bar, st.mp, st.max_mp, BAR_BLUE);
    fill_bar(_exp_bar, st.exp, st.exp_to_next(), BAR_YELLOW);

    if(st.level != _shown_level)
    {
        _shown_level = st.level;
        _level_text.clear();
        _small_text.generate(left_x + 4, bottom_y, bn::string<8>("Lv") + bn::to_string<4>(st.level), _level_text);
        _small_text.set_visible(_visible);
    }

    if(st.potions != _shown_potions)
    {
        _shown_potions = st.potions;
        _potion_text.clear();
        _small_text.generate(left_x + 212, bottom_y, bn::string<8>("x") + bn::to_string<4>(st.potions), _potion_text);
    }

    if(_message_frames)
    {
        --_message_frames;

        if(! _message_frames)
        {
            _message_text.clear();
        }
    }

    for(bn::sprite_ptr& sprite : _level_text)
    {
        sprite.set_visible(_visible);
    }

    for(bn::sprite_ptr& sprite : _potion_text)
    {
        sprite.set_visible(_visible);
    }
}

void hud::set_boss_hp(int hp, int max_hp)
{
    if(_boss_bar.empty())
    {
        create_bar(_boss_bar, -(boss_segments * 4), -(bn::display::height() / 2) + 8);
    }

    fill_bar(_boss_bar, hp, max_hp, BAR_RED);
}

void hud::hide_boss()
{
    _boss_bar.clear();
}

void hud::show_message(const bn::string_view& line_1, const bn::string_view& line_2, int frames)
{
    _message_text.clear();
    _big_text.generate(0, -44, line_1, _message_text);

    if(! line_2.empty())
    {
        _big_text.generate(0, -28, line_2, _message_text);
    }

    _message_frames = frames;
}

void hud::set_visible(bool visible)
{
    _visible = visible;
    set_bar_visible(_hp_bar, visible);
    set_bar_visible(_mp_bar, visible);
    set_bar_visible(_exp_bar, visible);
    set_bar_visible(_boss_bar, visible);

    for(bn::sprite_ptr& sprite : _labels)
    {
        sprite.set_visible(visible);
    }

    for(bn::sprite_ptr& sprite : _message_text)
    {
        sprite.set_visible(visible);
    }
}

}
