#include "lb_game.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_random.h"
#include "bn_display.h"
#include "bn_optional.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_palettes.h"

#include "bn_sprite_items_spr_items.h"
#include "bn_sprite_items_spr_portal.h"

#include "lb_fx.h"
#include "lb_hud.h"
#include "lb_enemy.h"
#include "lb_level.h"
#include "lb_stats.h"
#include "lb_player.h"

namespace lb
{

namespace
{
    constexpr int max_enemies = 20;
    constexpr int max_pickups = 12;
    constexpr int drop_frames = 60 * 15;
    constexpr int spike_damage = 10;
    constexpr int shockwave_damage = 15;
    constexpr int heart_max_hp = 25;

    struct pickup
    {
        item_type type;
        int flag;
        bn::fixed x;
        bn::fixed y;
        bn::sprite_ptr sprite;
        int frames = 0;

        [[nodiscard]] bn::fixed_rect rect() const
        {
            return bn::fixed_rect(x, y - 8, 14, 16);
        }
    };

    [[nodiscard]] int sign(bn::fixed value)
    {
        return value < 0 ? -1 : 1;
    }

    class world
    {

    public:
        world(stats& st, int& frames) :
            _stats(st),
            _frames(frames),
            _camera(bn::camera_ptr::create(0, 0)),
            _player(_camera),
            _fx(_camera)
        {
        }

        play_result run(int map_index, int portal_index)
        {
            _load(map_index, portal_index);

            while(true)
            {
                ++_frames;

                if(bn::keypad::start_pressed() && ! _player.dead() && ! _victory_frames)
                {
                    _pause();
                }

                bool portal_request = _player.update(*_level, _stats);
                _update_hazards();
                _update_enemies();
                _update_attacks();
                _update_contacts();
                _update_pickups();

                if(_victory_frames)
                {
                    if(++_victory_frames > 150)
                    {
                        fade_out();
                        return play_result::VICTORY;
                    }
                }
                else if(_player.dead())
                {
                    if(++_dead_frames > 90)
                    {
                        fade_out();
                        return play_result::DIED;
                    }
                }
                else if(portal_request)
                {
                    if(const portal_def* portal = _touched_portal())
                    {
                        _load(portal->target_map, portal->target_portal);
                        continue;
                    }
                }

                _update_camera(false);
                _hud.update(_stats);
                _fx.update();
                bn::core::update();
            }
        }

    private:
        stats& _stats;
        int& _frames;
        bn::camera_ptr _camera;
        bn::optional<level> _level;
        player _player;
        hud _hud;
        fx _fx;
        bn::random _random;
        bn::vector<enemy, max_enemies> _enemies;
        bn::vector<bn::sprite_ptr, 4> _portal_sprites;
        bn::vector<pickup, max_pickups> _pickups;
        int _shake_frames = 0;
        int _dead_frames = 0;
        int _victory_frames = 0;
        bn::fixed_point _camera_position;

        void _load(int map_index, int portal_index)
        {
            if(_level)
            {
                fade_out();
            }

            _enemies.clear();
            _pickups.clear();
            _portal_sprites.clear();
            _fx.clear();
            _hud.hide_boss();
            _level.reset();
            _level.emplace(map_index, _camera);

            const map_def& def = _level->def();

            if(portal_index >= 0)
            {
                const portal_def& portal = def.portals[portal_index];
                _player.place(portal.x, portal.y);
            }
            else
            {
                _player.place(def.spawn_x, def.spawn_y);
            }

            for(const enemy_spawn& spawn : def.enemies)
            {
                _enemies.emplace_back(spawn, _camera);
            }

            for(const portal_def& portal : def.portals)
            {
                bn::sprite_ptr sprite = bn::sprite_items::spr_portal.create_sprite(portal.x, portal.y - 16);
                sprite.set_camera(_camera);
                sprite.set_z_order(20);
                _portal_sprites.push_back(bn::move(sprite));
            }

            for(const item_def& item : def.items)
            {
                if(! _stats.collected(item.flag))
                {
                    _add_pickup(item.type, item.flag, item.x, item.y);
                }
            }

            _update_camera(true);
            _hud.update(_stats);
            _hud.show_message(def.name, "", 120);
            bn::core::update();
            fade_in();
        }

        void _add_pickup(item_type type, int flag, bn::fixed x, bn::fixed y)
        {
            if(_pickups.full())
            {
                return;
            }

            bn::sprite_ptr sprite = bn::sprite_items::spr_items.create_sprite(x, y - 8, int(type));
            sprite.set_camera(_camera);
            sprite.set_z_order(15);
            _pickups.push_back(pickup{ type, flag, x, y, bn::move(sprite) });
        }

        void _pause()
        {
            _hud.show_message("Paused", "START: resume", 2);
            bn::core::update();

            while(! bn::keypad::start_pressed())
            {
                bn::core::update();
            }

            _hud.show_message("", "", 1);
        }

        void _update_hazards()
        {
            if(! _player.invulnerable() && touches_tile(*_level, _player.physics(), TILE_SPIKE))
            {
                int direction = _player.physics().vx > 0 ? -1 : 1;
                _hurt_player(spike_damage, direction);
            }
        }

        void _hurt_player(int damage, int direction)
        {
            if(_player.take_damage(damage, direction, _stats))
            {
                const body& player_body = _player.physics();
                _fx.add_number(player_body.x, player_body.top() - 6, damage, false);
            }
        }

        void _update_enemies()
        {
            const body& player_body = _player.physics();

            for(enemy& current : _enemies)
            {
                const body& enemy_body = current.physics();

                // Only simulate enemies near the player to save CPU.
                if(! current.boss() && (bn::abs(enemy_body.x - player_body.x) > 360 ||
                                        bn::abs(enemy_body.y - player_body.y) > 260))
                {
                    continue;
                }

                if(current.update(*_level, player_body))
                {
                    _shake_frames = 20;

                    if(player_body.on_ground && bn::abs(player_body.x - enemy_body.x) < 120)
                    {
                        _hurt_player(shockwave_damage, sign(player_body.x - enemy_body.x));
                    }
                }

                if(current.boss())
                {
                    if(current.alive())
                    {
                        _hud.set_boss_hp(current.hp(), current.max_hp());
                    }
                    else
                    {
                        _hud.hide_boss();
                    }
                }
            }
        }

        void _update_attacks()
        {
            const bn::optional<attack_info>& attack = _player.active_attack();

            if(! attack)
            {
                return;
            }

            const body& player_body = _player.physics();

            for(int hit = 0; hit < attack->max_targets; ++hit)
            {
                // Pick the closest enemy in range that this swing hasn't hit yet.
                enemy* target = nullptr;
                bn::fixed target_distance;

                for(enemy& current : _enemies)
                {
                    if(current.alive() && current.last_attack_id() != attack->id &&
                            current.hit_box().intersects(attack->area))
                    {
                        bn::fixed distance = bn::abs(current.physics().x - player_body.x);

                        if(! target || distance < target_distance)
                        {
                            target = &current;
                            target_distance = distance;
                        }
                    }
                }

                if(! target)
                {
                    break;
                }

                int damage = (_stats.attack() * attack->damage_percent) / 100;
                damage = bn::max(1, (damage * (80 + _random.get_int(41))) / 100);
                bool critical = _random.get_int(100) < 12;

                if(critical)
                {
                    damage = (damage * 3) / 2;
                }

                const body& target_body = target->physics();
                _fx.add_number(target_body.x, target_body.top() - 6, damage, critical);

                if(target->damage(damage, attack->direction, attack->id))
                {
                    _on_enemy_killed(*target);
                }
            }
        }

        void _on_enemy_killed(const enemy& killed)
        {
            const body& killed_body = killed.physics();

            if(_stats.add_exp(killed.exp()))
            {
                const body& player_body = _player.physics();
                _fx.add_text(player_body.x, player_body.top() - 16, "LEVEL UP!");
                _hud.show_message("Level up!", "HP and MP restored", 120);
            }

            if(killed.boss())
            {
                _victory_frames = 1;
                _shake_frames = 40;
                _hud.show_message("The Golem crumbles!", "Victory!", 150);
            }
            else if(_random.get_int(100) < 25)
            {
                _add_pickup(item_type::POTION, -1, killed_body.x, killed_body.y);
            }
        }

        void _update_contacts()
        {
            if(_player.invulnerable())
            {
                return;
            }

            bn::fixed_rect player_rect = _player.hurt_box();
            const body& player_body = _player.physics();

            for(const enemy& current : _enemies)
            {
                if(current.alive() && current.hit_box().intersects(player_rect))
                {
                    int damage = (current.contact_damage() * (90 + _random.get_int(21))) / 100;
                    _hurt_player(damage, sign(player_body.x - current.physics().x));
                    return;
                }
            }
        }

        void _update_pickups()
        {
            bn::fixed_rect player_rect = _player.hurt_box();

            for(auto it = _pickups.begin(); it != _pickups.end();)
            {
                pickup& current = *it;
                ++current.frames;

                bool persistent = current.flag >= 0;
                bn::fixed bob = ((current.frames >> 4) & 1) ? -1 : 0;
                current.sprite.set_y(current.y - 8 + bob);

                if(! persistent)
                {
                    if(current.frames > drop_frames)
                    {
                        it = _pickups.erase(it);
                        continue;
                    }

                    current.sprite.set_visible(current.frames < drop_frames - 180 || ((current.frames >> 2) & 1));
                }

                if(! _player.dead() && current.rect().intersects(player_rect))
                {
                    _collect(current);
                    it = _pickups.erase(it);
                    continue;
                }

                ++it;
            }
        }

        void _collect(const pickup& item)
        {
            if(item.flag >= 0)
            {
                _stats.collect(item.flag);
            }

            switch(item.type)
            {

            case item_type::POTION:
                _stats.potions = bn::min(stats::max_potions, _stats.potions + 1);
                _fx.add_text(item.x, item.y - 24, "+POTION");
                break;

            case item_type::HEART:
                _stats.max_hp += heart_max_hp;
                _stats.hp = _stats.max_hp;
                _hud.show_message("Heart Crystal!", "Max HP +25", 180);
                break;

            case item_type::FLASH_JUMP:
                _stats.flash_jump = true;
                _hud.show_message("Flash Jump learned!", "Press A again in mid-air", 240);
                break;

            case item_type::SPRING_BOOTS:
                _stats.spring_boots = true;
                _hud.show_message("Spring Boots found!", "You can jump much higher", 240);
                break;

            default:
                break;
            }
        }

        [[nodiscard]] const portal_def* _touched_portal() const
        {
            bn::fixed_rect player_rect = _player.hurt_box();

            for(const portal_def& portal : _level->def().portals)
            {
                if(bn::fixed_rect(portal.x, portal.y - 16, 16, 32).intersects(player_rect))
                {
                    return &portal;
                }
            }

            return nullptr;
        }

        void _update_camera(bool snap)
        {
            constexpr int half_width = bn::display::width() / 2;
            constexpr int half_height = bn::display::height() / 2;

            const body& player_body = _player.physics();
            bn::fixed target_x = bn::clamp(player_body.x, bn::fixed(half_width),
                                           bn::fixed(_level->width() - half_width));
            bn::fixed target_y = bn::clamp(player_body.y - 24, bn::fixed(half_height),
                                           bn::fixed(_level->height() - half_height));
            if(snap)
            {
                _camera_position = bn::fixed_point(target_x, target_y);
            }
            else
            {
                _camera_position.set_x(_camera_position.x() + ((target_x - _camera_position.x()) / 6));
                _camera_position.set_y(_camera_position.y() + ((target_y - _camera_position.y()) / 6));
            }

            for(bn::sprite_ptr& sprite : _portal_sprites)
            {
                sprite.set_tiles(bn::sprite_items::spr_portal.tiles_item(), (_frames >> 4) & 1);
            }

            bn::fixed_point shake;

            if(_shake_frames)
            {
                --_shake_frames;
                shake = bn::fixed_point(((_shake_frames >> 1) & 1) ? 2 : -2, ((_shake_frames >> 2) & 1) ? 1 : -1);
            }

            _camera.set_position(_camera_position + shake);
        }

    };
}

void fade_out()
{
    for(int step = 1; step <= 16; ++step)
    {
        bn::fixed intensity = bn::fixed(step) / 16;
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), intensity);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), intensity);
        bn::core::update();
    }
}

void fade_in()
{
    for(int step = 15; step >= 0; --step)
    {
        bn::fixed intensity = bn::fixed(step) / 16;
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), intensity);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), intensity);
        bn::core::update();
    }
}

play_result play(stats& st, int map_index, int portal_index, int& frames)
{
    world game_world(st, frames);
    return game_world.run(map_index, portal_index);
}

}
