#ifndef PORT_BACKGROUND_EFFECT_H
#define PORT_BACKGROUND_EFFECT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Values from the committed per-bank fade state, after the engine's mask has
 * been applied. Intensity is 0..256; the native fade colors are 0 and 248. */
typedef struct {
    uint16_t intensity;
    uint8_t color;
} PortPaletteFade;

typedef enum {
    PORT_PALETTE_FADE_INVALID,
    PORT_PALETTE_FADE_UNIFORM,
    PORT_PALETTE_FADE_MIXED
} PortPaletteFadeResult;

/* Resolve only the source banks in the supplied coverage mask. A mixed result
 * cannot safely be applied to a single image without further source coverage;
 * do not pick an arbitrary bank or silently replace it with the global mask.
 * No engine state is read here. On failure/mixed the output is unchanged. */
PortPaletteFadeResult Port_Background_ResolvePaletteFade(uint16_t source_banks,
                                                        const PortPaletteFade banks[16],
                                                        PortPaletteFade* output);

/* Composite one straight-alpha RGB888 sample over black, then apply a
 * previously resolved fade. Neutral opaque samples retain all eight bits.
 * Returns 0 for invalid input without modifying rgb. */
int Port_Background_ApplyPaletteFade(uint8_t rgb[3], uint8_t alpha, PortPaletteFade fade);

typedef enum {
    PORT_LAYER_EFFECT_INVALID,
    PORT_LAYER_EFFECT_NONE,
    PORT_LAYER_EFFECT_BRIGHTEN,
    PORT_LAYER_EFFECT_DARKEN,
    PORT_LAYER_EFFECT_MIXED,
    PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT
} PortLayerEffect;

/* Layer bits use BLDCNT positions (BG0..BG3 and backdrop bit 5). OBJ is
 * rejected because semi-transparent OBJ has different hardware rules.
 * window_effects is the effective window's SFX-enable bit. Local alpha on
 * a different first target does not affect the source; alpha targeting the
 * source needs its actual second input before it can be composed. */
PortLayerEffect Port_Background_ResolveLayerEffect(uint8_t source_layers, uint16_t bldcnt,
                                                  int window_effects);
/* Apply a resolved brightness effect in RGB888; bldy has hardware 5-bit
 * masking and a 16-step clamp. Invalid or unresolved effects fail closed. */
int Port_Background_ApplyLayerBrightness(uint8_t rgb[3], PortLayerEffect effect, uint16_t bldy);
/* Use only after resolving the actual second target and its RGB888 color.
 * GBA EVA/EVB fields are masked and clamped to 16; channels saturate at 255. */
int Port_Background_ApplyLayerAlpha(uint8_t rgb[3], const uint8_t second[3], uint16_t bldalpha);

#ifdef __cplusplus
}
#endif
#endif
