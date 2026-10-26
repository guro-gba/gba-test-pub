#include "lb_player.h"

#include "bn_math.h"
#include "bn_keypad.h"

#include "bn_sprite_items_spr_player.h"
#include "bn_sprite_items_spr_slash.h"
#include "bn_sprite_items_spr_skill.h"

#include "lb_level.h"
#include "lb_stats.h"

namespace lb
{

namespace
{
    constexpr bn::fixed walk_speed = 1.5;
    constexpr bn::fixed ground_accel = 0.3;
    constexpr bn::fixed air_accel = 0.08;
    constexpr bn::fixed air_drag = 0.04;
    constexpr bn::fixed jump_speed = 4;
    constexpr bn::fixed spring_jump_speed = 5.6;
    constexpr bn::fixed flash_jump_speed_x = 4.5;
    constexpr bn::fixed flash_jump_speed_y = -1.5;
    constexpr bn::fixed climb_speed = 1.25;
    constexpr bn::fixed rope_jump_speed = 2.6;

    constexpr int attack_frames = 22;
    constexpr int attack_active_start = 6;
    constexpr int attack_active_end = 13;
    constexpr int skill_frames = 30;
    constexpr int skill_active_start = 8;
    constexpr int skill_active_end = 17;
    constexpr int hurt_frames = 24;
    constexpr int invulnerable_frames = 90;

    enum player_frame
    {
        FRAME_IDLE = 0,
        FRAME_WALK = 1,
        FRAME_JUMP = 3,
        FRAME_CLIMB = 4,
        FRAME_WINDUP = 6,
        FRAME_SWING = 7,
        FRAME_PRONE = 8,
        FRAME_HURT = 9
    };

    [[nodiscard]] bn::fixed approach(bn::fixed value, bn::fixed target, bn::fixed step)
    {
        if(value < target)
        {
            return bn::min(value + step, target);
        }

        return bn::max(value - step, target);
    }

    [[nodiscard]] int input_direction()
    {
        return int(bn::keypad::right_held()) - int(bn::keypad::left_held());
    }
}

player::player(const bn::camera_ptr& camera) :
    _sprite(bn::sprite_items::spr_player.create_sprite(0, 0)),
    _slash_sprite(bn::sprite_items::spr_slash.create_sprite(0, 0)),
    _skill_sprite(bn::sprite_items::spr_skill.create_sprite(0, 0))
{
    _body.half_width = 6;
    _body.height = 30;
    _sprite.set_camera(camera);
    _sprite.set_z_order(5);
    _slash_sprite.set_camera(camera);
    _slash_sprite.set_z_order(4);
    _slash_sprite.set_visible(false);
    _skill_sprite.set_camera(camera);
    _skill_sprite.set_z_order(4);
    _skill_sprite.set_visible(false);
}

void player::place(bn::fixed x, bn::fixed y)
{
    _body.x = x;
    _body.y = y;
    _body.vx = 0;
    _body.vy = 0;
    _state = state::NORMAL;
    _state_frames = 0;
    _active_attack.reset();
    _update_graphics();
}

bool player::update(const level& lvl, stats& st)
{
    bool portal_request = false;
    ++_state_frames;
    ++_anim_frames;

    if(_invulnerable_frames)
    {
        --_invulnerable_frames;
    }

    if(_drop_frames)
    {
        --_drop_frames;
    }

    if(_rope_cooldown)
    {
        --_rope_cooldown;
    }

    if(_state != state::DEAD)
    {
        ++_regen_frames;

        if(_regen_frames % 60 == 0)
        {
            st.mp = bn::min(st.max_mp, st.mp + 1);
        }

        if(_regen_frames % 150 == 0)
        {
            st.heal(1);
        }
    }

    _active_attack.reset();

    switch(_state)
    {

    case state::NORMAL:
        _update_normal(lvl, st, portal_request);
        break;

    case state::CLIMB:
        _update_climb(lvl);
        break;

    case state::ATTACK:
        _update_attack(lvl);
        break;

    case state::HURT:
    case state::DEAD:
        if(_body.on_ground && _state_frames > 8)
        {
            _body.vx = approach(_body.vx, 0, ground_accel);
        }

        _body.vy = bn::min(_body.vy + gravity, max_fall_speed);
        move_body(lvl, _body, false);

        if(_state == state::HURT && _state_frames >= hurt_frames)
        {
            _state = state::NORMAL;
            _state_frames = 0;
        }
        break;

    default:
        break;
    }

    _update_graphics();
    return portal_request;
}

bool player::take_damage(int damage, int direction, stats& st)
{
    if(invulnerable())
    {
        return false;
    }

    st.hp = bn::max(0, st.hp - damage);
    _state = st.hp ? state::HURT : state::DEAD;
    _state_frames = 0;
    _invulnerable_frames = invulnerable_frames;
    _active_attack.reset();
    _prone = false;
    _body.vx = direction * bn::fixed(1.6);
    _body.vy = -2.6;
    _body.on_ground = false;
    return true;
}

void player::set_visible(bool visible)
{
    _visible = visible;
    _update_graphics();
}

void player::_update_normal(const level& lvl, stats& st, bool& portal_request)
{
    int direction = input_direction();
    bool on_ground = _body.on_ground;
    _prone = on_ground && bn::keypad::down_held() && direction == 0;

    if(on_ground)
    {
        _flash_jump_used = false;
    }

    if(bn::keypad::up_held() && _try_grab_rope(lvl, false))
    {
        return;
    }

    if(on_ground && bn::keypad::down_held() && ! bn::keypad::a_held() && _try_grab_rope(lvl, true))
    {
        return;
    }

    if(on_ground && bn::keypad::up_pressed())
    {
        portal_request = true;
    }

    if(bn::keypad::a_pressed())
    {
        if(on_ground)
        {
            if(bn::keypad::down_held() && on_platform(lvl, _body))
            {
                _drop_frames = 10;
                _body.vy = 0.5;
            }
            else
            {
                _body.vy = st.spring_boots ? -spring_jump_speed : -jump_speed;
            }

            _body.on_ground = false;
            on_ground = false;
            _prone = false;
        }
        else if(st.flash_jump && ! _flash_jump_used)
        {
            _flash_jump_used = true;
            _body.vx = _facing * flash_jump_speed_x;
            _body.vy = bn::min(_body.vy, flash_jump_speed_y);
        }
    }

    if(bn::keypad::b_pressed())
    {
        _start_attack(false);
        return;
    }

    if(bn::keypad::r_pressed() && st.mp >= stats::skill_mp_cost)
    {
        st.mp -= stats::skill_mp_cost;
        _start_attack(true);
        return;
    }

    if(bn::keypad::l_pressed() && st.potions > 0 && st.hp < st.max_hp)
    {
        --st.potions;
        st.heal(stats::potion_heal);
    }

    if(on_ground)
    {
        bn::fixed target = _prone ? bn::fixed(0) : direction * walk_speed;
        _body.vx = approach(_body.vx, target, ground_accel);

        if(direction && ! _prone)
        {
            _facing = direction;
        }
    }
    else
    {
        bn::fixed speed = bn::abs(_body.vx);

        if(direction)
        {
            _facing = direction;

            if(speed <= walk_speed)
            {
                _body.vx = bn::clamp(_body.vx + (direction * air_accel), -walk_speed, walk_speed);
            }
            else if((_body.vx > 0) != (direction > 0))
            {
                _body.vx += direction * air_accel;
            }
        }

        if(bn::abs(_body.vx) > walk_speed)
        {
            _body.vx = approach(_body.vx, 0, air_drag);
        }
    }

    _body.vy = bn::min(_body.vy + gravity, max_fall_speed);
    move_body(lvl, _body, _drop_frames > 0);
}

void player::_update_climb(const level& lvl)
{
    int direction = input_direction();

    if(bn::keypad::a_pressed() && direction)
    {
        _state = state::NORMAL;
        _state_frames = 0;
        _facing = direction;
        _body.vx = direction * walk_speed;
        _body.vy = -rope_jump_speed;
        _rope_cooldown = 20;
        return;
    }

    if(bn::keypad::up_held())
    {
        _body.y -= climb_speed;
        _anim_frames += 1;

        int feet_tile = lvl.tile_at(_body.x, _body.y - 1);

        if(feet_tile != TILE_ROPE)
        {
            _state = state::NORMAL;
            _state_frames = 0;
            _rope_cooldown = 10;

            if(feet_tile == TILE_PLATFORM || feet_tile == TILE_SOLID)
            {
                // Climbed past the top of the rope: step onto the platform it hangs from.
                _body.y = ((_body.y.floor_integer() - 1) >> 3) * 8;
                _body.on_ground = true;
            }
        }
    }
    else if(bn::keypad::down_held())
    {
        _body.y += climb_speed;
        _anim_frames += 1;

        if(ground_at(lvl, _body.x, _body.y))
        {
            _body.y = ((_body.y.floor_integer() + 1) >> 3) * 8;
            _state = state::NORMAL;
            _state_frames = 0;
            _body.on_ground = true;
            _rope_cooldown = 10;
        }
        else if(lvl.tile_at(_body.x, _body.y - 1) != TILE_ROPE && lvl.tile_at(_body.x, _body.y - 16) != TILE_ROPE)
        {
            _state = state::NORMAL;
            _state_frames = 0;
            _rope_cooldown = 20;
        }
    }
    else
    {
        // Keep the climbing animation still.
        _anim_frames -= 1;
    }
}

void player::_update_attack(const level& lvl)
{
    int total_frames = _skill ? skill_frames : attack_frames;
    int active_start = _skill ? skill_active_start : attack_active_start;
    int active_end = _skill ? skill_active_end : attack_active_end;

    if(_state_frames >= active_start && _state_frames <= active_end)
    {
        if(_skill)
        {
            _active_attack = attack_info{ bn::fixed_rect(_body.x + (_facing * 36), _body.y - 16, 72, 40),
                                          _attack_id, 6, 220, _facing };
        }
        else
        {
            _active_attack = attack_info{ bn::fixed_rect(_body.x + (_facing * 18), _body.y - 16, 36, 34),
                                          _attack_id, 1, 100, _facing };
        }
    }

    if(_body.on_ground)
    {
        _body.vx = approach(_body.vx, 0, ground_accel);
    }
    else if(bn::abs(_body.vx) > walk_speed)
    {
        _body.vx = approach(_body.vx, 0, air_drag);
    }

    _body.vy = bn::min(_body.vy + gravity, max_fall_speed);
    move_body(lvl, _body, false);

    if(_state_frames >= total_frames)
    {
        _state = state::NORMAL;
        _state_frames = 0;
    }
}

bool player::_try_grab_rope(const level& lvl, bool from_above)
{
    if(_rope_cooldown)
    {
        return false;
    }

    if(from_above)
    {
        if(lvl.tile_at(_body.x, _body.y + 9) != TILE_ROPE)
        {
            return false;
        }

        _body.y += 20;
    }
    else if(lvl.tile_at(_body.x, _body.y - 16) != TILE_ROPE && lvl.tile_at(_body.x, _body.y - 1) != TILE_ROPE)
    {
        return false;
    }

    _body.x = ((_body.x.floor_integer() >> 3) * 8) + 4;
    _body.vx = 0;
    _body.vy = 0;
    _body.on_ground = false;
    _flash_jump_used = false;
    _prone = false;
    _state = state::CLIMB;
    _state_frames = 0;
    return true;
}

void player::_start_attack(bool skill)
{
    _skill = skill;
    _prone = false;
    _state = state::ATTACK;
    _state_frames = 0;
    ++_attack_id;
}

void player::_update_graphics()
{
    int frame = FRAME_IDLE;
    bool slash_visible = false;
    bool skill_visible = false;

    switch(_state)
    {

    case state::NORMAL:
        if(! _body.on_ground)
        {
            frame = FRAME_JUMP;
        }
        else if(_prone)
        {
            frame = FRAME_PRONE;
        }
        else if(bn::abs(_body.vx) > bn::fixed(0.2))
        {
            frame = FRAME_WALK + ((_anim_frames >> 3) & 1);
        }
        break;

    case state::CLIMB:
        frame = FRAME_CLIMB + ((_anim_frames >> 3) & 1);
        break;

    case state::ATTACK:
        if(_skill)
        {
            frame = _state_frames < skill_active_start ? FRAME_WINDUP : FRAME_SWING;

            if(_state_frames >= skill_active_start)
            {
                skill_visible = true;
                _skill_sprite.set_tiles(bn::sprite_items::spr_skill.tiles_item(),
                                        _state_frames < skill_active_start + 5 ? 0 : 1);
                _skill_sprite.set_position(_body.x + (_facing * 34), _body.y - 16);
                _skill_sprite.set_horizontal_flip(_facing < 0);
            }
        }
        else
        {
            slash_visible = true;

            if(_state_frames < attack_active_start)
            {
                frame = FRAME_WINDUP;
                _slash_sprite.set_tiles(bn::sprite_items::spr_slash.tiles_item(), 0);
                _slash_sprite.set_position(_body.x - (_facing * 6), _body.y - 28);
            }
            else
            {
                frame = FRAME_SWING;
                _slash_sprite.set_tiles(bn::sprite_items::spr_slash.tiles_item(),
                                        _state_frames <= attack_active_end ? 1 : 2);
                _slash_sprite.set_position(_body.x + (_facing * 16), _body.y - 16);
            }

            _slash_sprite.set_horizontal_flip(_facing < 0);
        }
        break;

    case state::HURT:
    case state::DEAD:
        frame = FRAME_HURT;
        break;

    default:
        break;
    }

    bool blink = _invulnerable_frames && _state != state::DEAD && ((_invulnerable_frames >> 2) & 1);
    _sprite.set_tiles(bn::sprite_items::spr_player.tiles_item(), frame);
    _sprite.set_position(_body.x, _body.y - 16);
    _sprite.set_horizontal_flip(_facing < 0 && _state != state::CLIMB);
    _sprite.set_visible(_visible && ! blink);
    _slash_sprite.set_visible(_visible && slash_visible);
    _skill_sprite.set_visible(_visible && skill_visible);
}

}
