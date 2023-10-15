#pragma once

#include <string>
#include "irrlichttypes.h"
#include "irr_v2d.h"
#include "hud.h"

struct HotbarParams
{
    std::string image;
    f32 image_margin;
    u16 image_size;
    u16 item_count; 
    f32 item_padding;
    v2f pos; 
    std::string selected_image;

    static const HotbarParams getDefaults()
    {
        HotbarParams params;
        params.image = "";
        params.image_margin = 0.0f;
        params.image_size = HOTBAR_IMAGE_SIZE;
        params.item_count = HUD_HOTBAR_ITEMCOUNT_DEFAULT;
        params.item_padding = 0.08f;
        params.pos = v2f(0.5f, 0.99f);
        params.selected_image = "";
        return params;
    } 
};
