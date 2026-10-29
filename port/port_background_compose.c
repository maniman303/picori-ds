#include "port_background_compose.h"
#include <stddef.h>

static bool valid_row(const PortBackgroundComposeRow* r) {
    /* Forced blank bypasses palette and layer effects entirely. */
    if (r->forced_blank) return true;
    if (r->layer_effect != PORT_LAYER_EFFECT_NONE && r->layer_effect != PORT_LAYER_EFFECT_BRIGHTEN &&
        r->layer_effect != PORT_LAYER_EFFECT_DARKEN && r->layer_effect != PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT)
        return false;
    if (r->layer_effect == PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT && !r->second_valid) return false;
    return !r->apply_palette_fade || (r->palette_fade.intensity <= 256 &&
        (!r->palette_fade.intensity || r->palette_fade.color == 0 || r->palette_fade.color == 248));
}

static bool compose(uint32_t* output, int pitch, const PortBackgroundComposition* c,
                    const PortEffectSpan* spans, int span_count,
                    const PortBackgroundComposeRow* outside_rows, int outside_count,
                    const uint8_t outside_rgb[3], const PortBackgroundComposeRow* right_rows, int right_count,
                    const PortBackgroundComposeRow* top, const PortBackgroundComposeRow* bottom, int edge_count) {
    if (!output || !c || !c->rows || c->canvas.x != 0 || c->canvas.y != 0 ||
        c->canvas.width < 240 || c->canvas.width > 569 || c->canvas.height < 160 ||
        c->canvas.height > 192 || pitch < c->canvas.width || c->row_count < c->canvas.height ||
        c->native.width != 240 || c->native.height != 160 || c->native.x < 0 || c->native.y < 0 ||
        c->native.x > c->canvas.width - 240 || c->native.y > c->canvas.height - 160 ||
        (c->rgba && c->rgba_pitch_bytes < c->canvas.width * 4)) return false;
    if (right_rows && right_count < c->canvas.height) return false;
    if ((top || bottom) && (!top || !bottom || edge_count < 240)) return false;
    if (spans && (!outside_rows || !outside_rgb || span_count < c->canvas.height ||
                  outside_count < c->canvas.height)) return false;
    for (int y = 0; y < c->canvas.height; ++y) {
        if (!valid_row(&c->rows[y]) || (right_rows && !valid_row(&right_rows[y]))) return false;
        if (spans && (!valid_row(&outside_rows[y]) || spans[y].left > spans[y].right ||
                      spans[y].right > c->canvas.width)) return false;
    }
    if (top) for (int x = 0; x < 240; ++x)
        if (!valid_row(&top[x]) || !valid_row(&bottom[x])) return false;
    for (int y = 0; y < c->canvas.height; ++y) {
        for (int x = 0; x < c->canvas.width; ++x) {
            if (x >= c->native.x && x < c->native.x + 240 && y >= c->native.y && y < c->native.y + 160)
                continue;
            const bool outside = spans && (x < spans[y].left || x >= spans[y].right);
            const PortBackgroundComposeRow* r = outside ? &outside_rows[y] :
                right_rows && x >= c->native.x + 120 ? &right_rows[y] : &c->rows[y];
            if (!outside && top && (y < c->native.y || y >= c->native.y + 160)) {
                int edge_x = x - c->native.x;
                if (edge_x < 0) edge_x = 0;
                if (edge_x > 239) edge_x = 239;
                r = y < c->native.y ? &top[edge_x] : &bottom[edge_x];
            }
            uint8_t rgb[3], alpha = 255;
            if (outside) {
                rgb[0] = outside_rgb[0]; rgb[1] = outside_rgb[1]; rgb[2] = outside_rgb[2];
            } else if (c->rgba) {
                const uint8_t* p = c->rgba + (size_t)y * c->rgba_pitch_bytes + (size_t)x * 4;
                rgb[0] = p[0]; rgb[1] = p[1]; rgb[2] = p[2]; alpha = p[3];
            } else {
                rgb[0] = c->solid_rgb[0]; rgb[1] = c->solid_rgb[1]; rgb[2] = c->solid_rgb[2];
            }
            if (r->forced_blank) rgb[0] = rgb[1] = rgb[2] = 255;
            else {
                /* Even live sources use source-over-black for fractional alpha. */
                Port_Background_ApplyPaletteFade(rgb, alpha, r->apply_palette_fade ? r->palette_fade : (PortPaletteFade){0, 0});
                if (r->brightness_curve)
                    for (int ch = 0; ch < 3; ++ch) rgb[ch] = r->brightness_curve[ch][rgb[ch]];
                if (r->layer_effect == PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT)
                    Port_Background_ApplyLayerAlpha(rgb, r->second_rgb, r->coefficient);
                else Port_Background_ApplyLayerBrightness(rgb, r->layer_effect, r->coefficient);
            }
            output[(size_t)y * pitch + x] = 0xff000000u | (uint32_t)rgb[2] << 16 | (uint32_t)rgb[1] << 8 | rgb[0];
        }
    }
    return true;
}


bool Port_Background_ComposeMargins(uint32_t* output, int pitch, const PortBackgroundComposition* c) {
    return compose(output, pitch, c, NULL, 0, NULL, 0, NULL, NULL, 0, NULL, NULL, 0);
}

bool Port_Background_ComposeWindowMargins(uint32_t* output, int pitch, const PortBackgroundComposition* c,
                                          const PortEffectSpan* spans, int span_count,
                                          const PortBackgroundComposeRow* outside_rows, int outside_count,
                                          const uint8_t outside_rgb[3]) {
    if (!spans) return false;
    return compose(output, pitch, c, spans, span_count, outside_rows, outside_count, outside_rgb, NULL, 0, NULL, NULL, 0);
}


bool Port_Background_ComposeEdgeMargins(uint32_t* output, int pitch, const PortBackgroundComposition* c,
                                        const PortBackgroundComposeRow* right_rows, int right_count,
                                        const PortEffectSpan* spans, int span_count,
                                        const PortBackgroundComposeRow* outside_rows, int outside_count,
                                        const uint8_t outside_rgb[3]) {
    if (!right_rows) return false;
    return compose(output, pitch, c, spans, span_count, outside_rows, outside_count, outside_rgb,
                   right_rows, right_count, NULL, NULL, 0);
}

bool Port_Background_ComposePerimeter(uint32_t* output, int pitch, const PortBackgroundComposition* c,
                                      const PortBackgroundComposeRow* right_rows, int right_count,
                                      const PortBackgroundComposeRow* top, const PortBackgroundComposeRow* bottom,
                                      int edge_count, const PortEffectSpan* spans, int span_count,
                                      const PortBackgroundComposeRow* outside_rows, int outside_count,
                                      const uint8_t outside_rgb[3]) {
    if (!right_rows || !top || !bottom) return false;
    return compose(output, pitch, c, spans, span_count, outside_rows, outside_count, outside_rgb,
                   right_rows, right_count, top, bottom, edge_count);
}
