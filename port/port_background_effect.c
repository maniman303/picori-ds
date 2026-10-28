#include "port_background_effect.h"
#include <string.h>

static PortPaletteFade sCommittedFades[16];
static uint16_t sValidFades;
static uint8_t sCommittedBrightness[16];

void Port_Background_InvalidatePaletteFades(void) {
    memset(sCommittedFades, 0, sizeof(sCommittedFades));
    sValidFades = 0;
    memset(sCommittedBrightness, 0, sizeof(sCommittedBrightness));
}

void Port_Background_RecordPaletteFade(unsigned bank, PortPaletteFade fade, uint8_t brightness) {
    if (bank >= 16) return;
    if (brightness > 2 || fade.intensity > 256 || (fade.intensity && fade.color != 0 && fade.color != 248)) {
        sValidFades &= (uint16_t)~(1u << bank);
        return;
    }
    sCommittedFades[bank] = fade;
    sCommittedBrightness[bank] = brightness;
    sValidFades |= (uint16_t)(1u << bank);
}

const PortPaletteFade* Port_Background_CommittedPaletteFades(void) { return sCommittedFades; }
uint16_t Port_Background_ValidPaletteFades(void) { return sValidFades; }
const uint8_t* Port_Background_CommittedBrightness(void) { return sCommittedBrightness; }

static int valid_fade(PortPaletteFade fade) {
    return fade.intensity <= 256 &&
           (fade.intensity == 0 || fade.color == 0 || fade.color == 248);
}

PortPaletteFadeResult Port_Background_ResolvePaletteFade(uint16_t source_banks,
                                                        const PortPaletteFade banks[16],
                                                        PortPaletteFade* output) {
    PortPaletteFade value = {0, 0};
    int found = 0;
    if (!source_banks || !banks || !output)
        return PORT_PALETTE_FADE_INVALID;
    for (unsigned i = 0; i < 16; ++i) {
        PortPaletteFade current;
        if (!(source_banks & (1u << i)))
            continue;
        current = banks[i];
        if (!valid_fade(current))
            return PORT_PALETTE_FADE_INVALID;
        if (!current.intensity)
            current.color = 0; /* the target is irrelevant at zero intensity */
        if (found && (current.intensity != value.intensity || current.color != value.color))
            return PORT_PALETTE_FADE_MIXED;
        value = current;
        found = 1;
    }
    *output = value;
    return PORT_PALETTE_FADE_UNIFORM;
}

PortPaletteFadeResult Port_Background_ResolveCommittedPaletteFade(uint16_t source_banks, PortPaletteFade* output) {
    if (!source_banks || (source_banks & sValidFades) != source_banks) return PORT_PALETTE_FADE_INVALID;
    return Port_Background_ResolvePaletteFade(source_banks, sCommittedFades, output);
}

void Port_Background_BuildBrightnessCurve(const uint16_t tables[3][32], uint8_t curves[3][256]) {
    if (!tables || !curves) return;
    for (unsigned ch = 0; ch < 3; ++ch) for (unsigned x = 0; x < 256; ++x) {
        unsigned position = x * 31, lo = position / 255, rem = position % 255;
        unsigned a = (tables[ch][lo] >> (ch * 5)) & 31;
        unsigned b = (tables[ch][lo < 31 ? lo + 1 : lo] >> (ch * 5)) & 31;
        curves[ch][x] = (uint8_t)((a * (255 - rem) + b * rem + 15) / 31);
    }
}

int Port_Background_ApplyPaletteFade(uint8_t rgb[3], uint8_t alpha, PortPaletteFade fade) {
    if (!rgb || !valid_fade(fade))
        return 0;
    unsigned target = fade.color == 248 ? 255 : 0;
    for (unsigned i = 0; i < 3; ++i) {
        unsigned base = ((unsigned)rgb[i] * alpha + 127u) / 255u;
        rgb[i] = (uint8_t)((base * (256u - fade.intensity) + target * fade.intensity) >> 8);
    }
    return 1;
}

PortLayerEffect Port_Background_ResolveLayerEffect(uint8_t source_layers, uint16_t bldcnt,
                                                  int window_effects) {
    if (!source_layers || (source_layers & ~0x2fu) || (window_effects != 0 && window_effects != 1))
        return PORT_LAYER_EFFECT_INVALID;
    unsigned mode = (bldcnt >> 6) & 3u;
    if (!window_effects || mode == 0 || (mode == 1 && !(bldcnt & 0x3f00)))
        return PORT_LAYER_EFFECT_NONE;
    unsigned targeted = source_layers & (bldcnt & 0x3fu);
    if (!targeted)
        return PORT_LAYER_EFFECT_NONE;
    if (targeted != source_layers)
        return PORT_LAYER_EFFECT_MIXED;
    if (mode == 1)
        return PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT;
    return mode == 2 ? PORT_LAYER_EFFECT_BRIGHTEN : PORT_LAYER_EFFECT_DARKEN;
}

int Port_Background_ApplyLayerBrightness(uint8_t rgb[3], PortLayerEffect effect, uint16_t bldy) {
    if (!rgb || (effect != PORT_LAYER_EFFECT_NONE && effect != PORT_LAYER_EFFECT_BRIGHTEN &&
                 effect != PORT_LAYER_EFFECT_DARKEN))
        return 0;
    unsigned amount = bldy & 31u;
    if (amount > 16) amount = 16;
    for (unsigned i = 0; i < 3; ++i) {
        unsigned value = rgb[i];
        if (effect == PORT_LAYER_EFFECT_BRIGHTEN)
            value += ((255u - value) * amount) >> 4;
        else if (effect == PORT_LAYER_EFFECT_DARKEN)
            value -= (value * amount) >> 4;
        rgb[i] = (uint8_t)value;
    }
    return 1;
}

int Port_Background_ApplyLayerAlpha(uint8_t rgb[3], const uint8_t second[3], uint16_t bldalpha) {
    if (!rgb || !second)
        return 0;
    unsigned eva = bldalpha & 31u, evb = (bldalpha >> 8) & 31u;
    if (eva > 16) eva = 16;
    if (evb > 16) evb = 16;
    const uint8_t bottom[3] = {second[0], second[1], second[2]};
    for (unsigned i = 0; i < 3; ++i) {
        unsigned value = ((unsigned)rgb[i] * eva + (unsigned)bottom[i] * evb) >> 4;
        rgb[i] = (uint8_t)(value > 255 ? 255 : value);
    }
    return 1;
}
