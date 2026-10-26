#include "lb_enemy.h"

#include "bn_math.h"

#include "bn_sprite_items_spr_snail.h"
#include "bn_sprite_items_spr_slime.h"
#include "bn_sprite_items_spr_mushroom.h"
#include "bn_sprite_items_spr_bat.h"
#include "bn_sprite_items_spr_golem.h"
#include "bn_sprite_items_spr_minibar.h"

#include "lb_level.h"

namespace lb
{

namespace
{
    constexpr int respawn_frames = 60 * 12;
    constexpr int dying_frames = 24;

    struct enemy_info
    {
        const bn::sprite_item& item;
        int half_width;
        int height;
        int sprite_height;
        int hp;
        int contact_damage;
        int exp;
        bn::fixed speed;
    };

    [[nodiscard]] const enemy_info& info(enemy_type type)
    {
        static constexpr enemy_info infos[] = {
            { bn::sprite_items::spr_snail, 6, 11, 16, 25, 6, 3, 0.35 },
            { bn::sprite_items::spr_slime, 6, 11, 16, 45, 9, 7, 1 },
            { bn::sprite_items::spr_mushroom, 6, 13, 16, 80, 14, 14, 0.6 },
            { bn::sprite_items::spr_bat, 6, 10, 16, 50, 11, 11, 0.8 },
            { bn::sprite_items::spr_golem, 22, 58, 64, 1200, 22, 400, 0.45 },
        };

        return infos[int(type)];
    }

    [[nodiscard]] int sign(bn::fixed value)
    {
        return value < 0 ? -1 : 1;
    }

    [[nodiscard]] bool walkable(const level& lvl, bn::fixed x, bn::fixed y)
    {
        return ground_at(lvl, x, y) && lvl.tile_at(x, y + 1) != TILE_SPIKE;
    }
}

enemy::enemy(const enemy_spawn& spawn, const bn::camera_ptr& camera) :
    _item(&info(spawn.type).item),
    _sprite(_item->create_sprite(0, 0)),
    _camera(camera),
    _type(spawn.type),
    _spawn_x(spawn.x),
    _spawn_y(spawn.y)
{
    const enemy_info& enemy_info = info(_type);
    _body.half_width = enemy_info.half_width;
    _body.height = enemy_info.height;
    _max_hp = enemy_info.hp;
    _contact_damage = enemy_info.contact_damage;
    _exp = enemy_info.exp;
    _speed = enemy_info.speed;
    _sprite.set_camera(camera);
    _sprite.set_z_order(boss() ? 8 : 10);
    _respawn();
}

void enemy::_respawn()
{
    _hp = _max_hp;
    _body.x = _spawn_x;
    _body.y = _spawn_y;
    _body.vx = 0;
    _body.vy = 0;
    _direction = (_spawn_x.integer() >> 3) & 1 ? 1 : -1;
    _aggro = false;
    _stun_frames = 0;
    _flash_frames = 0;
    _hp_bar_frames = 0;
    _dead_frames = 0;
    _action_frames = 40 + ((_spawn_x.integer() * 7) % 60);
    _last_attack_id = -1;
    _hp_bar.reset();
    _sprite.set_visible(true);
    _update_graphics();
}

bool enemy::update(const level& lvl, const body& player_body)
{
    ++_frames;

    if(! alive())
    {
        ++_dead_frames;

        if(_dead_frames == dying_frames)
        {
            _sprite.set_visible(false);
            _hp_bar.reset();
        }
        else if(_dead_frames < dying_frames)
        {
            _sprite.set_visible((_dead_frames >> 1) & 1);
        }
        else if(! boss() && _dead_frames >= respawn_frames)
        {
            bn::fixed dx = bn::abs(player_body.x - _spawn_x);
            bn::fixed dy = bn::abs(player_body.y - _spawn_y);

            if(dx > 150 || dy > 100)
            {
                _respawn();
            }
        }

        return false;
    }

    bool shockwave = false;

    if(_flash_frames)
    {
        --_flash_frames;
    }

    if(_hp_bar_frames)
    {
        --_hp_bar_frames;

        if(! _hp_bar_frames)
        {
            _hp_bar.reset();
        }
    }

    if(_stun_frames)
    {
        --_stun_frames;

        if(_type == enemy_type::BAT)
        {
            _body.x += _body.vx;
            _body.vx = _body.vx * bn::fixed(0.85);
        }
        else
        {
            _body.vy = bn::min(_body.vy + gravity, max_fall_speed);

            // Don't get knocked off ledges.
            if(! walkable(lvl, _body.x + _body.vx + (sign(_body.vx) * _body.half_width), _body.y) && _body.on_ground)
            {
                _body.vx = 0;
            }

            move_body(lvl, _body, false);
        }
    }
    else
    {
        switch(_type)
        {

        case enemy_type::SNAIL:
        case enemy_type::MUSHROOM:
            _update_walker(lvl, player_body);
            break;

        case enemy_type::SLIME:
            _update_slime(lvl, player_body);
            break;

        case enemy_type::BAT:
            _update_bat(player_body);
            break;

        case enemy_type::GOLEM:
            shockwave = _update_golem(lvl, player_body);
            break;

        default:
            break;
        }
    }

    _update_graphics();
    return shockwave;
}

bool enemy::damage(int amount, int direction, int attack_id)
{
    _last_attack_id = attack_id;
    _hp = bn::max(0, _hp - amount);
    _flash_frames = 8;
    _aggro = true;

    if(! boss())
    {
        _stun_frames = 12;
        _body.vx = direction * bn::fixed(1.4);
        _hp_bar_frames = 180;

        if(! _hp_bar)
        {
            _hp_bar = bn::sprite_items::spr_minibar.create_sprite(0, 0);
            _hp_bar->set_camera(_camera);
            _hp_bar->set_z_order(3);
        }
    }

    if(! _hp)
    {
        _dead_frames = 0;
        _hp_bar.reset();
        return true;
    }

    _update_graphics();
    return false;
}

void enemy::_update_walker(const level& lvl, const body& player_body)
{
    bn::fixed dx = player_body.x - _body.x;
    bn::fixed dy = player_body.y - _body.y;
    bool moving = true;

    if(_aggro && bn::abs(dy) < 48 && bn::abs(dx) < 160)
    {
        _direction = sign(dx);
        moving = bn::abs(dx) > 4;

        // Mushrooms hop at players standing above them.
        if(_type == enemy_type::MUSHROOM && _body.on_ground && dy < -16 && ! (_frames % 50))
        {
            _body.vy = -3.6;
        }
    }
    else
    {
        if(_action_frames)
        {
            --_action_frames;
        }
        else
        {
            // Idle patrol: walk, then occasionally pause or turn around.
            _action_frames = 60 + ((_frames * 13) % 90);

            if((_frames % 3) == 0)
            {
                _direction = -_direction;
            }
        }

        moving = _action_frames > 20;
    }

    bn::fixed front_x = _body.x + (_direction * (_body.half_width + 1));

    if(_body.on_ground && ! walkable(lvl, front_x, _body.y))
    {
        if(! _aggro)
        {
            _direction = -_direction;
        }

        moving = false;
    }

    _body.vx = moving ? _direction * _speed : bn::fixed(0);
    _body.vy = bn::min(_body.vy + gravity, max_fall_speed);
    move_body(lvl, _body, false);

    if(_body.hit_wall && ! _aggro)
    {
        _direction = -_direction;
    }
}

void enemy::_update_slime(const level& lvl, const body& player_body)
{
    if(_body.on_ground)
    {
        _body.vx = 0;

        if(_action_frames)
        {
            --_action_frames;
        }
        else
        {
            if(_aggro)
            {
                _direction = sign(player_body.x - _body.x);
            }
            else if((_frames % 4) == 0)
            {
                _direction = -_direction;
            }

            bn::fixed landing_x = _body.x + (_direction * 28);

            if(! walkable(lvl, landing_x, _body.y))
            {
                _direction = -_direction;
            }

            _body.vy = -3;
            _body.vx = _direction * _speed;
            _action_frames = _aggro ? 30 : 50 + ((_frames * 7) % 40);
        }
    }

    _body.vy = bn::min(_body.vy + gravity, max_fall_speed);
    move_body(lvl, _body, false);
}

void enemy::_update_bat(const body& player_body)
{
    bn::fixed dx = player_body.x - _body.x;
    bn::fixed dy = (player_body.y - 20) - _body.y;

    if(! _aggro && bn::abs(dx) < 90 && bn::abs(dy) < 60)
    {
        _aggro = true;
    }

    bn::fixed target_x;
    bn::fixed target_y;

    if(_aggro && bn::abs(player_body.x - _spawn_x) < 200 && bn::abs(player_body.y - _spawn_y) < 140)
    {
        target_x = player_body.x;
        target_y = player_body.y - 20;
    }
    else
    {
        _aggro = false;

        int angle = (_frames * 8) & 2047;
        target_x = _spawn_x + (bn::lut_sin(angle) * 40);
        target_y = _spawn_y - 24 + (bn::lut_sin((angle * 2) & 2047) * 8);
    }

    bn::fixed tx = target_x - _body.x;
    bn::fixed ty = target_y - _body.y;
    _body.x += bn::clamp(tx, -_speed, _speed);
    _body.y += bn::clamp(ty, -_speed, _speed);
    _direction = sign(tx);
}

bool enemy::_update_golem(const level& lvl, const body& player_body)
{
    bool enraged = _hp * 2 < _max_hp;
    bn::fixed dx = player_body.x - _body.x;
    bool was_on_ground = _body.on_ground;

    if(_body.on_ground)
    {
        _direction = sign(dx);
        _body.vx = bn::abs(dx) > 8 ? _direction * (enraged ? bn::fixed(0.8) : _speed) : bn::fixed(0);

        if(_action_frames)
        {
            --_action_frames;
        }
        else
        {
            _body.vy = -4.5;
            _body.vx = _direction * bn::fixed(1.2);
            _action_frames = enraged ? 100 : 160;
        }
    }

    _body.vy = bn::min(_body.vy + gravity, max_fall_speed);
    move_body(lvl, _body, true);
    return ! was_on_ground && _body.on_ground && _frames > 30;
}

void enemy::_update_graphics()
{
    const enemy_info& enemy_info = info(_type);
    int frame;

    switch(_type)
    {

    case enemy_type::SLIME:
        frame = _body.on_ground && _action_frames < 8 ? 1 : 0;
        break;

    case enemy_type::BAT:
        frame = (_frames >> 3) & 1;
        break;

    case enemy_type::GOLEM:
        frame = _body.on_ground ? ((_frames >> 4) & 1) : 1;
        break;

    default:
        frame = bn::abs(_body.vx) > 0 ? (_frames >> 4) & 1 : 0;
        break;
    }

    _sprite.set_tiles(_item->tiles_item(), frame);
    _sprite.set_position(_body.x, _body.y - (enemy_info.sprite_height / 2));

    // Sprites face left; snails and mushrooms flip when walking right.
    _sprite.set_horizontal_flip(_direction > 0);

    if(_flash_frames)
    {
        _sprite.set_visible((_flash_frames >> 1) & 1);
    }
    else
    {
        _sprite.set_visible(true);
    }

    if(_hp_bar)
    {
        int fill = (_hp * 10 + _max_hp - 1) / _max_hp;
        _hp_bar->set_tiles(bn::sprite_items::spr_minibar.tiles_item(), bn::clamp(fill, 0, 10));
        _hp_bar->set_position(_body.x, _body.y - _body.height - 6);
    }
}

}
