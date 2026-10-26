#ifndef LB_PLAYER_H
#define LB_PLAYER_H

#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"

#include "lb_physics.h"

namespace lb
{
    class level;
    struct stats;

    // An attack swing that is currently able to hit enemies.
    struct attack_info
    {
        bn::fixed_rect area;
        int id;
        int max_targets;
        int damage_percent;
        int direction;
    };

    class player
    {

    public:
        explicit player(const bn::camera_ptr& camera);

        void place(bn::fixed x, bn::fixed y);

        // Returns true if Up was pressed while standing still on the ground (used to enter portals).
        bool update(const level& lvl, stats& st);

        [[nodiscard]] const body& physics() const
        {
            return _body;
        }

        [[nodiscard]] bn::fixed_rect hurt_box() const
        {
            return _body.rect();
        }

        [[nodiscard]] bool dead() const
        {
            return _state == state::DEAD;
        }

        [[nodiscard]] bool invulnerable() const
        {
            return _invulnerable_frames > 0 || _state == state::DEAD;
        }

        [[nodiscard]] const bn::optional<attack_info>& active_attack() const
        {
            return _active_attack;
        }

        // Returns false if the hit was ignored (invulnerable).
        bool take_damage(int damage, int direction, stats& st);

        void set_visible(bool visible);

    private:
        enum class state
        {
            NORMAL,
            CLIMB,
            ATTACK,
            HURT,
            DEAD
        };

        body _body;
        bn::sprite_ptr _sprite;
        bn::sprite_ptr _slash_sprite;
        bn::sprite_ptr _skill_sprite;
        bn::optional<attack_info> _active_attack;
        state _state = state::NORMAL;
        int _facing = 1;
        int _state_frames = 0;
        int _invulnerable_frames = 0;
        int _drop_frames = 0;
        int _rope_cooldown = 0;
        int _anim_frames = 0;
        int _attack_id = 0;
        int _regen_frames = 0;
        bool _skill = false;
        bool _flash_jump_used = false;
        bool _prone = false;
        bool _visible = true;

        void _update_normal(const level& lvl, stats& st, bool& portal_request);
        void _update_climb(const level& lvl);
        void _update_attack(const level& lvl);
        bool _try_grab_rope(const level& lvl, bool from_above);
        void _start_attack(bool skill);
        void _update_graphics();
    };
}

#endif
