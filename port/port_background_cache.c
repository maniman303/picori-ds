#include "port_background_cache.h"

#include <stdio.h>
#include <string.h>

bool Port_BackgroundCache_Init(PortBackgroundCache* cache, const char* root) {
    if (!cache) return false;
    cache->root[0] = '\0';
    Port_BackgroundCache_Invalidate(cache);
    if (!root || !*root || strlen(root) >= sizeof(cache->root)) return false;
    memcpy(cache->root, root, strlen(root) + 1);
    return true;
}

void Port_BackgroundCache_Invalidate(PortBackgroundCache* cache) {
    if (!cache) return;
    cache->current = cache->incoming = -1;
    cache->slots[0].state = cache->slots[1].state = PORT_BACKGROUND_CACHE_EMPTY;
}

PortBackgroundCacheState Port_BackgroundCache_Stage(PortBackgroundCache* cache,
                                                    PortBackgroundContext context,
                                                    PortAspectMode aspect,
                                                    uint32_t generation, bool parity) {
    if (!cache) return PORT_BACKGROUND_CACHE_EMPTY;
    cache->incoming = -1;
    const PortScreenBackground* background = Port_Background_Get(context, parity);
    if (!cache->root[0] || background->source != PORT_BACKGROUND_SOURCE_IMAGE ||
        !Port_Viewport_ProfileForAspect(aspect)) return PORT_BACKGROUND_CACHE_EMPTY;
    for (int i = 0; i < 2; ++i) {
        const PortBackgroundCacheSlot* slot = &cache->slots[i];
        if (slot->state != PORT_BACKGROUND_CACHE_EMPTY && slot->context == context &&
            slot->aspect == aspect && slot->renderer_generation == generation) {
            cache->incoming = i;
            return slot->state;
        }
    }
    int index = cache->current == 0 ? 1 : 0;
    PortBackgroundCacheSlot* slot = &cache->slots[index];
    slot->state = PORT_BACKGROUND_CACHE_EMPTY;
    slot->context = context;
    slot->aspect = aspect;
    slot->renderer_generation = generation;
    if (Port_Background_LoadPng(cache->root, context, aspect, false, slot->rgba, sizeof(slot->rgba))) {
        slot->state = PORT_BACKGROUND_CACHE_READY;
    } else {
        slot->state = PORT_BACKGROUND_CACHE_FAILED;
        fprintf(stderr, "[background] unable to load %s/override/%s/%s\n", cache->root,
                Port_Viewport_ProfileForAspect(aspect)->directory, background->filename);
    }
    cache->incoming = index;
    return slot->state;
}

PortBackgroundCacheState Port_BackgroundCache_StageMenu(PortBackgroundCache* cache,
                                                        PortBackgroundMenuFamily family,
                                                        PortBackgroundHealth health,
                                                        PortAspectMode aspect,
                                                        uint32_t generation, bool parity) {
    return Port_BackgroundCache_Stage(cache, Port_Background_MenuContext(family, health),
                                      aspect, generation, parity);
}

void Port_BackgroundCache_Commit(PortBackgroundCache* cache) {
    if (cache) cache->current = cache->incoming;
}

PortBackgroundCacheState Port_BackgroundCache_Reload(PortBackgroundCache* cache) {
    if (!cache) return PORT_BACKGROUND_CACHE_EMPTY;
    int current = cache->current;
    if (current < 0 || current > 1 || cache->slots[current].state == PORT_BACKGROUND_CACHE_EMPTY) {
        Port_BackgroundCache_Invalidate(cache);
        return PORT_BACKGROUND_CACHE_EMPTY;
    }
    PortBackgroundContext context = cache->slots[current].context;
    PortAspectMode aspect = cache->slots[current].aspect;
    uint32_t generation = cache->slots[current].renderer_generation;
    Port_BackgroundCache_Invalidate(cache);
    PortBackgroundCacheState result = Port_BackgroundCache_Stage(cache, context, aspect, generation, false);
    Port_BackgroundCache_Commit(cache);
    return result;
}

const uint8_t* Port_BackgroundCache_Current(const PortBackgroundCache* cache, int* width, int* height) {
    if (width) *width = 0;
    if (height) *height = 0;
    if (!cache || cache->current < 0 || cache->current > 1) return NULL;
    const PortBackgroundCacheSlot* slot = &cache->slots[cache->current];
    if (slot->state != PORT_BACKGROUND_CACHE_READY) return NULL;
    const PortViewportProfile* profile = Port_Viewport_ProfileForAspect(slot->aspect);
    if (!profile) return NULL;
    if (width) *width = profile->width;
    if (height) *height = profile->height;
    return slot->rgba;
}
