#ifndef PORT_BACKGROUND_COMPOSE_H
#define PORT_BACKGROUND_COMPOSE_H

#include <stdbool.h>
#include <stdint.h>
#include "port_background_effect.h"
#include "port_effect_geometry.h"
#include "port_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* All effect inputs are resolved by the caller for each logical row. A live
 * palette source is already faded and must set apply_palette_fade to false. */
typedef struct {
    PortPaletteFade palette_fade;
    PortLayerEffect layer_effect;
    uint16_t coefficient; /* BLDY or BLDALPHA according to layer_effect */
    uint8_t second_rgb[3];
    const uint8_t (*brightness_curve)[256]; /* NULL for identity/already-faded sources */
    bool second_valid; /* required for NEEDS_ALPHA_INPUT */
    bool apply_palette_fade;
    bool forced_blank;
} PortBackgroundComposeRow;

typedef struct {
    PortViewportRect canvas, native;
    /* RGBA8888 source at canvas coordinates, or NULL for solid_rgb. */
    const uint8_t* rgba;
    int rgba_pitch_bytes;
    uint8_t solid_rgb[3];
    const PortBackgroundComposeRow* rows;
    int row_count;
} PortBackgroundComposition;

/* The canvas already contains the native image at native.x/native.y. Write
 * only margins in ABGR8888, preserving native pixels and pitch padding. All
 * inputs are validated before writing; unresolved effects fail closed. The
 * source, rows and output must not overlap. No allocation or game state. */
bool Port_Background_ComposeMargins(uint32_t* output, int pitch,
                                    const PortBackgroundComposition* composition);

/* Apply a previously classified scene-wide window to the margins. Pixels
 * inside each span use composition's source and effects; pixels outside use
 * the explicit solid source and per-row effects. Native pixels remain owned
 * by the PPU. The caller resolves which native window policy is scene-wide;
 * this function does not infer policy from register values. */
bool Port_Background_ComposeWindowMargins(uint32_t* output, int pitch,
                                          const PortBackgroundComposition* composition,
                                          const PortEffectSpan* spans, int span_count,
                                          const PortBackgroundComposeRow* outside_rows,
                                          int outside_row_count, const uint8_t outside_rgb[3]);

/* As above, with independent resolved effects for left and right edges.
 * Additional top/bottom rows split at the native horizontal midpoint. */
bool Port_Background_ComposeEdgeMargins(uint32_t* output, int pitch,
                                        const PortBackgroundComposition* composition,
                                        const PortBackgroundComposeRow* right_rows, int right_count,
                                        const PortEffectSpan* spans, int span_count,
                                        const PortBackgroundComposeRow* outside_rows, int outside_count,
                                        const uint8_t outside_rgb[3]);

#ifdef __cplusplus
}
#endif
#endif
