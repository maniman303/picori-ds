#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "port_background_effect.h"

int main(void) {
    uint16_t tables[3][32]; uint8_t curves[3][256];
    for (unsigned c = 0; c < 3; ++c) for (unsigned i = 0; i < 32; ++i)
        tables[c][i] = (uint16_t)(i << (c * 5));
    Port_Background_BuildBrightnessCurve(tables, curves);
    for (unsigned c = 0; c < 3; ++c) for (unsigned i = 0; i < 256; ++i)
        assert(curves[c][i] == i);
    for (unsigned i = 0; i < 32; ++i) tables[0][i] = (uint16_t)(i / 2);
    Port_Background_BuildBrightnessCurve(tables, curves);
    assert(curves[0][0] == 0 && curves[0][255] == 123);
    PortPaletteFade banks[16] = {{0, 0}}, result = {123, 99};
    /* Credits use BG2 banks 7..14 while a native transition excludes bank 15. */
    for (int i = 7; i <= 14; ++i) banks[i] = (PortPaletteFade){128, 248};
    banks[15] = (PortPaletteFade){0, 0};
    assert(Port_Background_ResolvePaletteFade(0x7f80, banks, &result) == PORT_PALETTE_FADE_UNIFORM);
    assert(result.intensity == 128 && result.color == 248);
    result = (PortPaletteFade){123, 99};
    assert(Port_Background_ResolvePaletteFade(0xff80, banks, &result) == PORT_PALETTE_FADE_MIXED);
    assert(result.intensity == 123 && result.color == 99);
    banks[14] = (PortPaletteFade){257, 0};
    assert(Port_Background_ResolvePaletteFade(0x7f80, banks, &result) == PORT_PALETTE_FADE_INVALID);
    assert(Port_Background_ResolvePaletteFade(0, banks, &result) == PORT_PALETTE_FADE_INVALID);
    banks[0] = (PortPaletteFade){0, 248};
    banks[1] = (PortPaletteFade){0, 0};
    assert(Port_Background_ResolvePaletteFade(3, banks, &result) == PORT_PALETTE_FADE_UNIFORM);
    for (int value = 0; value <= 255; ++value) {
        uint8_t rgb[3] = {(uint8_t)value, (uint8_t)(255 - value), 37};
        assert(Port_Background_ApplyPaletteFade(rgb, 255, (PortPaletteFade){0, 0}));
        assert(rgb[0] == value && rgb[1] == 255 - value && rgb[2] == 37);
        assert(Port_Background_ApplyPaletteFade(rgb, 255, (PortPaletteFade){128, 248}));
        assert(rgb[0] == (value + 255) / 2 && rgb[1] == (510 - value) / 2 && rgb[2] == 146);
    }
    uint8_t partial[] = {200, 100, 50};
    assert(Port_Background_ApplyPaletteFade(partial, 128, (PortPaletteFade){0, 248}));
    assert(partial[0] == 100 && partial[1] == 50 && partial[2] == 25);
    uint8_t translucent[] = {200, 100, 50};
    assert(Port_Background_ApplyPaletteFade(translucent, 128, (PortPaletteFade){256, 0}));
    assert(translucent[0] == 0 && translucent[1] == 0 && translucent[2] == 0);
    uint8_t invalid[] = {200, 100, 50};
    assert(!Port_Background_ApplyPaletteFade(invalid, 128, (PortPaletteFade){257, 0}));
    assert(invalid[0] == 200 && invalid[1] == 100 && invalid[2] == 50);
    /* Local title/file/credits text or rays blend over, rather than fade,
     * their respective static background sources. Fusion's flash targets OBJ. */
    assert(Port_Background_ResolveLayerEffect(1u << 1, 0x0241, 1) == PORT_LAYER_EFFECT_NONE);
    assert(Port_Background_ResolveLayerEffect(1u << 3, 0x0844, 1) == PORT_LAYER_EFFECT_NONE);
    assert(Port_Background_ResolveLayerEffect(1u << 2, 0x2442, 1) == PORT_LAYER_EFFECT_NONE);
    assert(Port_Background_ResolveLayerEffect(1u << 2, 0x2244, 1) == PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT);
    assert(Port_Background_ResolveLayerEffect(0x0e, 0x3f90, 1) == PORT_LAYER_EFFECT_NONE);
    /* During a native pause-tab transition BG2 is first target and BG3 can be
     * its second target; unlike the title ray this alpha needs both inputs. */
    assert(Port_Background_ResolveLayerEffect(1u << 2, 0x0e46, 1) == PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT);
    assert(Port_Background_ResolveLayerEffect(1u << 3, 0x0e46, 1) == PORT_LAYER_EFFECT_NONE);
    assert(Port_Background_ResolveLayerEffect(1u << 3, 0x0088, 1) == PORT_LAYER_EFFECT_BRIGHTEN);
    assert(Port_Background_ResolveLayerEffect(0x0a, 0x0088, 1) == PORT_LAYER_EFFECT_MIXED);
    assert(Port_Background_ResolveLayerEffect(1u << 3, 0x0148, 1) == PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT);
    assert(Port_Background_ResolveLayerEffect(1u << 3, 0x3648, 1) == PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT);
    assert(Port_Background_ResolveLayerEffect(1u << 3, 0x2f40, 1) == PORT_LAYER_EFFECT_NONE);
    assert(Port_Background_ResolveLayerEffect(1u << 5, 0x3648, 1) == PORT_LAYER_EFFECT_NONE);
    assert(Port_Background_ResolveLayerEffect(1u << 3, 0x0148, 0) == PORT_LAYER_EFFECT_NONE);
    assert(Port_Background_ResolveLayerEffect(1u << 3, 0x0048, 1) == PORT_LAYER_EFFECT_NONE);
    assert(Port_Background_ResolveLayerEffect(1u << 5, 0x00a0, 1) == PORT_LAYER_EFFECT_BRIGHTEN);
    assert(Port_Background_ResolveLayerEffect(1u << 5, 0x00bf, 1) == PORT_LAYER_EFFECT_BRIGHTEN);
    assert(Port_Background_ResolveLayerEffect(1u << 4, 0x3f90, 1) == PORT_LAYER_EFFECT_INVALID);
    uint8_t bright[] = {1, 128, 254};
    assert(Port_Background_ApplyLayerBrightness(bright, PORT_LAYER_EFFECT_BRIGHTEN, 8));
    assert(bright[0] == 128 && bright[1] == 191 && bright[2] == 254);
    assert(Port_Background_ApplyLayerBrightness(bright, PORT_LAYER_EFFECT_DARKEN, 31));
    assert(bright[0] == 0 && bright[1] == 0 && bright[2] == 0);
    assert(!Port_Background_ApplyLayerBrightness(bright, PORT_LAYER_EFFECT_MIXED, 8));
    uint8_t top[] = {255, 128, 1}, bottom[] = {0, 100, 255};
    assert(Port_Background_ApplyLayerAlpha(top, bottom, 0x0b07));
    assert(top[0] == 111 && top[1] == 124 && top[2] == 175);
    uint8_t saturated[] = {255, 128, 1};
    assert(Port_Background_ApplyLayerAlpha(saturated, bottom, 0x1f1f));
    assert(saturated[0] == 255 && saturated[1] == 228 && saturated[2] == 255);
    uint8_t overlapping[] = {1, 2, 3, 4};
    assert(Port_Background_ApplyLayerAlpha(overlapping + 1, overlapping, 0x1010));
    assert(overlapping[1] == 3 && overlapping[2] == 5 && overlapping[3] == 7);
    puts("Background palette ownership and full-precision fade: PASS");
    return 0;
}
