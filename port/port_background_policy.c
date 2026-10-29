#include "port_background_policy.h"
#include <string.h>

bool Port_Background_EffectPolicy(PortBackgroundContext c, PortBackgroundEffectPolicy* out) {
    PortBackgroundEffectPolicy p = {0, 0, false};
    if (!out) return false;
    switch (c) {
        case PORT_BACKGROUND_BOOT_LOGOS: p = (PortBackgroundEffectPolicy){0x20, 1, true}; break;
        case PORT_BACKGROUND_TITLE: p = (PortBackgroundEffectPolicy){2, 7, false}; break;
        case PORT_BACKGROUND_FILE_SELECT: p = (PortBackgroundEffectPolicy){8, 0x7800, false}; break;
        case PORT_BACKGROUND_MENU_NORMAL: case PORT_BACKGROUND_MENU_DAMAGED: case PORT_BACKGROUND_MENU_CRITICAL:
        case PORT_BACKGROUND_MENU_TITLED_NORMAL: case PORT_BACKGROUND_MENU_TITLED_DAMAGED: case PORT_BACKGROUND_MENU_TITLED_CRITICAL:
            p = (PortBackgroundEffectPolicy){8, 0x4000, false}; break;
        case PORT_BACKGROUND_KINSTONE_PIECES_NORMAL: case PORT_BACKGROUND_KINSTONE_PIECES_DAMAGED:
        case PORT_BACKGROUND_KINSTONE_PIECES_CRITICAL: case PORT_BACKGROUND_SWORD_TECHNIQUES_NORMAL:
        case PORT_BACKGROUND_SWORD_TECHNIQUES_DAMAGED: case PORT_BACKGROUND_SWORD_TECHNIQUES_CRITICAL:
            p = (PortBackgroundEffectPolicy){4, 0x2000, false}; break;
        case PORT_BACKGROUND_KINSTONE_NORMAL: case PORT_BACKGROUND_KINSTONE_DAMAGED: case PORT_BACKGROUND_KINSTONE_CRITICAL:
            p = (PortBackgroundEffectPolicy){0x0e, 0x7800, false}; break;
        case PORT_BACKGROUND_FAST_TRAVEL: p = (PortBackgroundEffectPolicy){8, 0x20, false}; break;
        case PORT_BACKGROUND_PROLOGUE_01: p = (PortBackgroundEffectPolicy){4, 0x7800, false}; break;
        case PORT_BACKGROUND_PROLOGUE_02: case PORT_BACKGROUND_PROLOGUE_03: case PORT_BACKGROUND_PROLOGUE_04:
        case PORT_BACKGROUND_PROLOGUE_05: case PORT_BACKGROUND_PROLOGUE_06:
            p = (PortBackgroundEffectPolicy){4, 0x7f80, false}; break;
        case PORT_BACKGROUND_CREDITS_01: case PORT_BACKGROUND_CREDITS_04: case PORT_BACKGROUND_CREDITS_05:
        case PORT_BACKGROUND_CREDITS_08: case PORT_BACKGROUND_ENDING:
            p = (PortBackgroundEffectPolicy){4, 0x7f80, false}; break;
        case PORT_BACKGROUND_CREDITS_02: case PORT_BACKGROUND_CREDITS_03:
        case PORT_BACKGROUND_CREDITS_06: case PORT_BACKGROUND_CREDITS_07:
            p = (PortBackgroundEffectPolicy){0x24, 0x7f81, false}; break;
        case PORT_BACKGROUND_GAME_OVER: case PORT_BACKGROUND_CREDITS_CLEARED: case PORT_BACKGROUND_ENDING_SAVE:
            p = (PortBackgroundEffectPolicy){0x20, 1, true}; break;
        case PORT_BACKGROUND_FIGURINES: case PORT_BACKGROUND_MAP_HINT:
            p = (PortBackgroundEffectPolicy){0x20, 1, true}; break;
        case PORT_BACKGROUND_LOCAL_MAP_HINT: p = (PortBackgroundEffectPolicy){8, 0x20, true}; break;
        default: return false;
    }
    *out = p;
    return true;
}

bool Port_Background_ResolveEffect(const PortBackgroundEffectPolicy* p, uint8_t layers, uint16_t source_banks,
                                   const PortPaletteFade banks[16], uint16_t valid_banks,
                                   uint16_t bldcnt, uint16_t alpha, uint16_t bldy, int window_effects,
                                   int second_layer, const uint8_t second_rgb[3], PortBackgroundComposeRow* out) {
    PortBackgroundComposeRow r;
    if (!p || !out || !layers || !source_banks || (layers & ~p->layers) || (source_banks & ~p->banks)) return false;
    memset(&r, 0, sizeof(r));
    if (!p->live_palette) {
        if (!banks || (valid_banks & source_banks) != source_banks ||
            Port_Background_ResolvePaletteFade(source_banks, banks, &r.palette_fade) != PORT_PALETTE_FADE_UNIFORM)
            return false;
        r.apply_palette_fade = true;
    }
    r.layer_effect = Port_Background_ResolveLayerEffect(layers, bldcnt, window_effects);
    if (r.layer_effect == PORT_LAYER_EFFECT_INVALID || r.layer_effect == PORT_LAYER_EFFECT_MIXED) return false;
    /* The backdrop is the bottommost layer and has no alpha second input. */
    if (layers == 0x20 && r.layer_effect == PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT)
        r.layer_effect = PORT_LAYER_EFFECT_NONE;
    if (r.layer_effect == PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT) {
        if (second_layer < 0 || second_layer > 5 || (layers & (1u << second_layer))) return false;
        if (!(bldcnt & (1u << (8 + second_layer)))) r.layer_effect = PORT_LAYER_EFFECT_NONE;
        else {
            if (!second_rgb) return false;
            memcpy(r.second_rgb, second_rgb, 3);
            r.second_valid = true;
            r.coefficient = alpha;
        }
    } else r.coefficient = bldy;
    *out = r;
    return true;
}
