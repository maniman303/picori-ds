#ifndef PORT_BACKGROUND_RUNTIME_H
#define PORT_BACKGROUND_RUNTIME_H

#include "port_background_cache.h"
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
} PortBackgroundRuntimeSnapshot;

bool Port_BackgroundRuntime_Init(const char* root);
void Port_BackgroundRuntime_Reset(void);
/* Runs only after prior readers finish, alongside the native VBlank latch.
 * Decode and I/O are confined to a changed key at this boundary. */
void Port_BackgroundRuntime_Publish(PortBackgroundContext context, const PortViewportGeometry* geometry,
                                    uint32_t renderer_generation);
const PortBackgroundRuntimeSnapshot* Port_BackgroundRuntime_Get(void);

#ifdef __cplusplus
}
#endif
#endif
