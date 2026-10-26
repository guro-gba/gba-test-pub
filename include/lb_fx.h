#ifndef LB_FX_H
#define LB_FX_H

#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_sprite_text_generator.h"

namespace lb
{
    // Floating damage numbers and short texts in map space.
    class fx
    {

    public:
        explicit fx(const bn::camera_ptr& camera);

        void add_number(bn::fixed x, bn::fixed y, int value, bool critical);

        void add_text(bn::fixed x, bn::fixed y, const bn::string_view& text);

        void update();

        void clear()
        {
            _popups.clear();
        }

    private:
        struct popup
        {
            bn::vector<bn::sprite_ptr, 3> sprites;
            int frames = 0;
        };

        bn::sprite_text_generator _text_generator;
        bn::vector<popup, 6> _popups;
    };
}

#endif
