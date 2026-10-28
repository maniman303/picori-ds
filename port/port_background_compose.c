#include "port_background_compose.h"
#include <stddef.h>

bool Port_Background_ComposeMargins(uint32_t* output, int pitch, const PortBackgroundComposition* c) {
    if (!output || !c || !c->rows || c->canvas.x != 0 || c->canvas.y != 0 ||
        c->canvas.width < 240 || c->canvas.width > 569 || c->canvas.height < 160 ||
        c->canvas.height > 192 || pitch < c->canvas.width || c->row_count < c->canvas.height ||
        c->native.width != 240 || c->native.height != 160 || c->native.x < 0 || c->native.y < 0 ||
        c->native.x > c->canvas.width - 240 || c->native.y > c->canvas.height - 160 ||
        (c->rgba && c->rgba_pitch_bytes < c->canvas.width * 4)) return false;
    for (int y = 0; y < c->canvas.height; ++y) {
        const PortBackgroundComposeRow* r = &c->rows[y];
        if (r->layer_effect != PORT_LAYER_EFFECT_NONE && r->layer_effect != PORT_LAYER_EFFECT_BRIGHTEN &&
            r->layer_effect != PORT_LAYER_EFFECT_DARKEN && r->layer_effect != PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT)
            return false;
        if (r->layer_effect == PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT && !r->second_valid) return false;
        if (r->apply_palette_fade && (r->palette_fade.intensity > 256 ||
            (r->palette_fade.intensity && r->palette_fade.color != 0 && r->palette_fade.color != 248)))
            return false;
    }
    for (int y = 0; y < c->canvas.height; ++y) {
        const PortBackgroundComposeRow* r = &c->rows[y];
        for (int x = 0; x < c->canvas.width; ++x) {
            if (x >= c->native.x && x < c->native.x + 240 && y >= c->native.y && y < c->native.y + 160)
                continue;
            uint8_t rgb[3], alpha = 255;
            if (c->rgba) {
                const uint8_t* p = c->rgba + (size_t)y * c->rgba_pitch_bytes + (size_t)x * 4;
                rgb[0] = p[0]; rgb[1] = p[1]; rgb[2] = p[2]; alpha = p[3];
            } else {
                rgb[0] = c->solid_rgb[0]; rgb[1] = c->solid_rgb[1]; rgb[2] = c->solid_rgb[2];
            }
            if (r->forced_blank) rgb[0] = rgb[1] = rgb[2] = 255;
            else {
                /* Even live sources use source-over-black for fractional alpha. */
                Port_Background_ApplyPaletteFade(rgb, alpha, r->apply_palette_fade ? r->palette_fade : (PortPaletteFade){0, 0});
                if (r->layer_effect == PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT)
                    Port_Background_ApplyLayerAlpha(rgb, r->second_rgb, r->coefficient);
                else Port_Background_ApplyLayerBrightness(rgb, r->layer_effect, r->coefficient);
            }
            output[(size_t)y * pitch + x] = 0xff000000u | (uint32_t)rgb[2] << 16 | (uint32_t)rgb[1] << 8 | rgb[0];
        }
    }
    return true;
}
