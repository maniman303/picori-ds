#include "port_background_runtime.h"
#include <string.h>

static PortBackgroundCache sCache;
static PortBackgroundRuntimeSnapshot sSnapshot;
static bool sKeyValid;
static PortBackgroundContext sContext;
static PortAspectMode sAspect;
static uint32_t sRendererGeneration;
static bool sEnabled;

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

void Port_BackgroundRuntime_Publish(PortBackgroundContext context, const PortViewportGeometry* g,
                                    uint32_t renderer_generation) {
    if (!g) { Port_BackgroundRuntime_Reset(); return; }
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
    sSnapshot.canvas = g->canvas;
    sSnapshot.native = g->nativeContent;
    sSnapshot.aspect = g->effectiveAspect;
    sSnapshot.renderer_generation = renderer_generation;
    sSnapshot.rgba = Port_BackgroundCache_Current(&sCache, &sSnapshot.width, &sSnapshot.height);
}

const PortBackgroundRuntimeSnapshot* Port_BackgroundRuntime_Get(void) { return &sSnapshot; }
