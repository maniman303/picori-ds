#ifndef PORT_BACKGROUND_RUNTIME_H
#define PORT_BACKGROUND_RUNTIME_H

#include "port_background_cache.h"
#include "port_background_effect.h"
#include "port_background_policy.h"
#include "port_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Immutable until the next simulation-boundary publication or reset. The
 * image is NULL for non-image contexts, failed decodes and native/parity
 * geometry. Pixel storage is owned by the cache and must not be retained. */
typedef struct {
    PortBackgroundContext context;
    PortBackgroundSource source;
    PortViewportRect canvas, native;
    PortAspectMode aspect;
    uint32_t renderer_generation;
    const uint8_t* rgba;
    int width, height;
    PortPaletteFade palette_fades[16];
    uint16_t valid_palette_fades;
    uint8_t brightness[16];
    PortBackgroundEffectPolicy effect_policy;
    bool effect_policy_valid;
} PortBackgroundRuntimeSnapshot;

bool Port_BackgroundRuntime_Init(const char* root);
void Port_BackgroundRuntime_Reset(void);
/* Request an artwork refresh at the next publication boundary. Safe to call
 * while a published snapshot is in use; no files or pixels change here. */
void Port_BackgroundRuntime_RequestReload(void);
/* Runs only after prior readers finish, alongside the native VBlank latch.
 * Decode and I/O are confined to a changed key at this boundary. */
void Port_BackgroundRuntime_Publish(PortBackgroundContext context, const PortViewportGeometry* geometry,
                                    uint32_t renderer_generation, bool verified_effect_coverage);
const PortBackgroundRuntimeSnapshot* Port_BackgroundRuntime_Get(void);
/* Resolve from the latched descriptor and explicit native raster inputs.
 * The caller supplies verified coverage and the actual second layer; no game
 * state or settings are read by the compositor. */
bool Port_BackgroundRuntime_ResolveEffect(const PortBackgroundRuntimeSnapshot* snapshot,
                                         uint8_t layers, uint16_t banks,
                                         uint16_t bldcnt, uint16_t alpha, uint16_t bldy,
                                         int window_effects, int second_layer, const uint8_t second_rgb[3],
                                         PortBackgroundComposeRow* output);

#ifdef __cplusplus
}
#endif
#endif
