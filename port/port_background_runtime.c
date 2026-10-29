#include "port_background_runtime.h"
#include <string.h>
#include <stdatomic.h>

static PortBackgroundCache sCache;
static PortBackgroundRuntimeSnapshot sSnapshot;
static bool sKeyValid;
static PortBackgroundContext sContext;
static PortAspectMode sAspect;
static uint32_t sRendererGeneration;
static bool sEnabled;
static atomic_bool sReloadRequested;

bool Port_BackgroundRuntime_Init(const char* root) {
    bool initialized = Port_BackgroundCache_Init(&sCache, root);
    sKeyValid = false;
    memset(&sSnapshot, 0, sizeof(sSnapshot));
    return initialized;
}

void Port_BackgroundRuntime_Reset(void) {
    Port_BackgroundCache_Invalidate(&sCache);
    sKeyValid = false;
    memset(&sSnapshot, 0, sizeof(sSnapshot));
}

void Port_BackgroundRuntime_RequestReload(void) {
    atomic_store_explicit(&sReloadRequested, true, memory_order_relaxed);
}

void Port_BackgroundRuntime_Publish(PortBackgroundContext context, const PortViewportGeometry* g,
                                    uint32_t renderer_generation, bool verified_effect_coverage) {
    if (!g) { Port_BackgroundRuntime_Reset(); return; }
    if (atomic_exchange_explicit(&sReloadRequested, false, memory_order_relaxed)) {
        Port_BackgroundCache_Invalidate(&sCache);
        sKeyValid = false;
    }
    bool enabled = !g->flags &&
                   (g->canvas.width != 240 || g->canvas.height != 160);
    const PortScreenBackground* background = Port_Background_Get(context, !enabled);
    if (!sKeyValid || sContext != context || sAspect != g->effectiveAspect ||
        sRendererGeneration != renderer_generation || sEnabled != enabled) {
        Port_BackgroundCache_Stage(&sCache, context, g->effectiveAspect, renderer_generation, !enabled);
        Port_BackgroundCache_Commit(&sCache);
        sContext = context; sAspect = g->effectiveAspect;
        sRendererGeneration = renderer_generation; sEnabled = enabled; sKeyValid = true;
    }
    sSnapshot.context = enabled ? context : PORT_BACKGROUND_UNCLASSIFIED;
    sSnapshot.source = background->source;
    sSnapshot.effect_policy_valid = verified_effect_coverage && background->source != PORT_BACKGROUND_SOURCE_NONE &&
        Port_Background_EffectPolicy(context, &sSnapshot.effect_policy);
    sSnapshot.artwork_edges = sSnapshot.effect_policy_valid ? Port_Background_ArtworkEdges(context) : NULL;
    if (!sSnapshot.effect_policy_valid) memset(&sSnapshot.effect_policy, 0, sizeof(sSnapshot.effect_policy));
    sSnapshot.canvas = g->canvas;
    sSnapshot.native = g->nativeContent;
    sSnapshot.aspect = g->effectiveAspect;
    sSnapshot.renderer_generation = renderer_generation;
    memcpy(sSnapshot.palette_fades, Port_Background_CommittedPaletteFades(), sizeof(sSnapshot.palette_fades));
    sSnapshot.valid_palette_fades = Port_Background_ValidPaletteFades();
    memcpy(sSnapshot.brightness, Port_Background_CommittedBrightness(), sizeof(sSnapshot.brightness));
    sSnapshot.rgba = Port_BackgroundCache_Current(&sCache, &sSnapshot.width, &sSnapshot.height);
}

const PortBackgroundRuntimeSnapshot* Port_BackgroundRuntime_Get(void) { return &sSnapshot; }


bool Port_BackgroundRuntime_ResolveEffect(const PortBackgroundRuntimeSnapshot* s, uint8_t layers, uint16_t banks,
                                         uint16_t bldcnt, uint16_t alpha, uint16_t bldy,
                                         int window_effects, int second_layer, const uint8_t second_rgb[3],
                                         PortBackgroundComposeRow* output) {
    if (!s || !s->effect_policy_valid) return false;
    return Port_Background_ResolveEffect(&s->effect_policy, layers, banks, s->palette_fades,
                                         s->valid_palette_fades, bldcnt, alpha, bldy,
                                         window_effects, second_layer, second_rgb, output);
}
