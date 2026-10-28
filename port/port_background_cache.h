#ifndef PORT_BACKGROUND_CACHE_H
#define PORT_BACKGROUND_CACHE_H

#include "port_background_png.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Two bounded slots, owned by the caller. Keep this off the stack. The
 * conservative bound covers every profile, including the taller raster. */
#define PORT_BACKGROUND_CACHE_SLOT_BYTES (569u * 192u * 4u)
#define PORT_BACKGROUND_CACHE_ROOT_BYTES 4096

typedef enum {
    PORT_BACKGROUND_CACHE_EMPTY,
    PORT_BACKGROUND_CACHE_FAILED,
    PORT_BACKGROUND_CACHE_READY
} PortBackgroundCacheState;

typedef struct {
    PortBackgroundContext context;
    PortAspectMode aspect;
    uint32_t renderer_generation;
    PortBackgroundCacheState state;
    uint8_t rgba[PORT_BACKGROUND_CACHE_SLOT_BYTES];
} PortBackgroundCacheSlot;

typedef struct {
    char root[PORT_BACKGROUND_CACHE_ROOT_BYTES];
    PortBackgroundCacheSlot slots[2];
    int current, incoming;
} PortBackgroundCache;

/* Init/invalidate/stage/commit must run at a scene boundary, after readers
 * have finished. Stage is the only operation that may do I/O or allocate.
 * A failed or non-image stage can be committed to withdraw stale artwork.
 * Invalidation discards both successes and cached failures for artist reload. */
bool Port_BackgroundCache_Init(PortBackgroundCache* cache, const char* asset_root);
void Port_BackgroundCache_Invalidate(PortBackgroundCache* cache);
PortBackgroundCacheState Port_BackgroundCache_Stage(PortBackgroundCache* cache,
                                                    PortBackgroundContext context,
                                                    PortAspectMode aspect,
                                                    uint32_t renderer_generation,
                                                    bool parity);
/* Resolve an installed family/health pair before staging. Health is captured
 * when the decoration is installed, never read from live game state here. */
PortBackgroundCacheState Port_BackgroundCache_StageMenu(PortBackgroundCache* cache,
                                                        PortBackgroundMenuFamily family,
                                                        PortBackgroundHealth installed_health,
                                                        PortAspectMode aspect,
                                                        uint32_t renderer_generation,
                                                        bool parity);
void Port_BackgroundCache_Commit(PortBackgroundCache* cache);
/* Explicitly discard both cached keys and re-read the current image, including
 * a previously failed one. Commit the new result before returning. */
PortBackgroundCacheState Port_BackgroundCache_Reload(PortBackgroundCache* cache);
/* Read-only and allocation/I/O free. Returned pixels remain valid until the
 * next mutation; a failed or empty current slot returns NULL. */
const uint8_t* Port_BackgroundCache_Current(const PortBackgroundCache* cache,
                                           int* width, int* height);

#ifdef __cplusplus
}
#endif
#endif
