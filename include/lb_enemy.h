#ifndef LB_ENEMY_H
#define LB_ENEMY_H

#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_sprite_item.h"

#include "lb_physics.h"
#include "lb_level_data.h"

namespace lb
{
    class level;

    class enemy
    {

    public:
        enemy(const enemy_spawn& spawn, const bn::camera_ptr& camera);

        // Returns true the frame a boss lands from a stomp (shockwave).
        bool update(const level& lvl, const body& player_body);

        [[nodiscard]] enemy_type type() const
        {
            return _type;
        }

        [[nodiscard]] bool alive() const
        {
            return _hp > 0;
        }

        [[nodiscard]] bool boss() const
        {
            return _type == enemy_type::GOLEM;
        }

        [[nodiscard]] int hp() const
        {
            return _hp;
        }

        [[nodiscard]] int max_hp() const
        {
            return _max_hp;
        }

        [[nodiscard]] int contact_damage() const
        {
            return _contact_damage;
        }

        [[nodiscard]] int exp() const
        {
            return _exp;
        }

        [[nodiscard]] const body& physics() const
        {
            return _body;
        }

        [[nodiscard]] bn::fixed_rect hit_box() const
        {
            return _body.rect();
        }

        [[nodiscard]] int last_attack_id() const
        {
            return _last_attack_id;
        }

        // Returns true if the enemy died.
        bool damage(int amount, int direction, int attack_id);

    private:
        const bn::sprite_item* _item;
        bn::sprite_ptr _sprite;
        bn::optional<bn::sprite_ptr> _hp_bar;
        bn::camera_ptr _camera;
        body _body;
        enemy_type _type;
        bn::fixed _spawn_x;
        bn::fixed _spawn_y;
        bn::fixed _speed;
        int _max_hp = 0;
        int _hp = 0;
        int _contact_damage = 0;
        int _exp = 0;
        int _direction = -1;
        int _frames = 0;
        int _action_frames = 0;
        int _stun_frames = 0;
        int _flash_frames = 0;
        int _hp_bar_frames = 0;
        int _dead_frames = 0;
        int _last_attack_id = -1;
        bool _aggro = false;

        void _respawn();
        void _update_walker(const level& lvl, const body& player_body);
        void _update_slime(const level& lvl, const body& player_body);
        void _update_bat(const body& player_body);
        bool _update_golem(const level& lvl, const body& player_body);
        void _update_graphics();
    };
}

#endif
