#ifndef PORT_BACKGROUND_POLICY_H
#define PORT_BACKGROUND_POLICY_H

#include "port_background_compose.h"
#include "port_screen_background.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Union of independently audited native source owners. A mixed union must not
 * be assigned one effect unless all its owners resolve identically. */
typedef struct {
    uint8_t layers;
    uint16_t banks;
    bool live_palette;
} PortBackgroundEffectPolicy;

bool Port_Background_EffectPolicy(PortBackgroundContext context, PortBackgroundEffectPolicy* output);

/* Audited EU artwork ownership at the native perimeter. Each byte encodes
 * (GBA layer << 4) | palette bank. NULL for non-image contexts. */
typedef struct {
    uint8_t left[160], right[160], top[240], bottom[240];
} PortBackgroundArtworkEdges;
const PortBackgroundArtworkEdges* Port_Background_ArtworkEdges(PortBackgroundContext context);

/* Resolve a single region/row. source_layers and source_banks may narrow the
 * policy to verified edge ownership; callers must never invent that coverage.
 * second_layer is 0..5, or -1 when the actual underlying layer is unknown.
 * Alpha colors are the already-faded live second input. Returns false without
 * modifying output if coverage, bank fade, window or alpha input is unknown. */
bool Port_Background_ResolveEffect(const PortBackgroundEffectPolicy* policy,
                                   uint8_t source_layers, uint16_t source_banks,
                                   const PortPaletteFade banks[16], uint16_t valid_banks,
                                   uint16_t bldcnt, uint16_t bldalpha, uint16_t bldy,
                                   int window_effects, int second_layer, const uint8_t second_rgb[3],
                                   PortBackgroundComposeRow* output);

#ifdef __cplusplus
}
#endif
#endif
