#ifndef PORT_VIEWPORT_H
#define PORT_VIEWPORT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Version 1 is the public PNG size/directory contract. These are logical
 * square-pixel dimensions, independent of window size and internal scale.
 * Except for DS at 256x192, height is 160 and width is ceil(160 * aspect).
 * Fractional widths round up to a pixel, without even-width/tile alignment.
 * Hardware timing and original source coordinates remain 240x160. */
#define PORT_VIEWPORT_PROFILE_VERSION 1
#define PORT_VIEWPORT_NATIVE_WIDTH 240
#define PORT_VIEWPORT_NATIVE_HEIGHT 160

/* Persisted IDs: append new modes, never reorder existing ones. */
typedef enum {
    PORT_ASPECT_NATIVE_3_2 = 0,
    PORT_ASPECT_WIDESCREEN_16_9 = 1,
    PORT_ASPECT_ULTRAWIDE_21_9 = 2,
    PORT_ASPECT_SUPER_ULTRAWIDE_32_9 = 3,
    PORT_ASPECT_DS_4_3 = 4,
    PORT_ASPECT_COUNT
} PortAspectMode;

typedef struct {
    const char* aspect;
    const char* directory;
    int width;
    int height;
} PortViewportProfile;

typedef struct {
    int x, y, width, height;
} PortViewportRect;

/* Activation-time lookup by canonical override directory, e.g. "43".
 * Unknown/NULL keys return NULL; do not silently substitute another ratio.
 * Profiles describe the asset contract, not current renderer capabilities. */
const PortViewportProfile* Port_Viewport_FindProfile(const char* directory);
const PortViewportProfile* Port_Viewport_ProfileForAspect(PortAspectMode aspect);

/* Reject insufficient capacity rather than truncating the selected profile.
 * A renderer must also validate raster/sprite support before exposing a mode. */
bool Port_Viewport_ProfileFits(const PortViewportProfile* profile, int capacityWidth, int capacityHeight);

/* Current raster/OBJ support is separate from allocated capacity. Taller
 * frames and X > 511 remain unavailable until their rendering paths land. */
bool Port_Viewport_AspectSupported(PortAspectMode aspect, int capacityWidth, int capacityHeight);

enum {
    PORT_VIEWPORT_PARITY = 1u << 0,
    PORT_VIEWPORT_UNSUPPORTED = 1u << 1
};

/* Semantic inputs supplied by the engine adapter, never inferred in pixel
 * loops. Vertical world reveal remains disabled regardless of these flags. */
enum {
    PORT_SCENE_WORLD = 1u << 0,
    PORT_SCENE_WIDE_WORLD = 1u << 1,
    PORT_SCENE_MAP_LIVE = 1u << 2,
    PORT_SCENE_NATIVE_TRANSITION = 1u << 3,
    PORT_SCENE_NATIVE_CONTENT = 1u << 4,
    /* The active shake sequence cannot cover a 192-row map window. */
    PORT_SCENE_TALL_SHAKE_UNSAFE = 1u << 5
};

typedef struct {
    uint32_t capabilities;
    int roomWidth, roomHeight;
} PortViewportScene;

typedef struct {
    PortAspectMode selectedAspect;
    PortAspectMode effectiveAspect;
    PortViewportRect canvas;
    PortViewportRect nativeContent;
    PortViewportRect uiSafe;
    PortViewportRect world;
    PortViewportScene scene;
    int capacityWidth, capacityHeight, pitchPixels;
    uint32_t flags;
    uint32_t generation;
} PortViewportGeometry;

/* Publish only at a simulation boundary, before shadow/camera/layout work.
 * Renderers read the same snapshot, including repeat presents while paused.
 * Storage remains valid until the next publication after workers complete.
 * Unsupported preferences are retained but use effective native geometry.
 * False means invalid capacity/pitch; the previous snapshot is retained.
 * Scene decisions are retained and recomputed against the new canvas. */
bool Port_Viewport_Publish(PortAspectMode selected, bool consoleParity,
                           int capacityWidth, int capacityHeight, int pitchPixels);
const PortViewportGeometry* Port_Viewport_Get(void);

/* Simulation-side publication at room/transition and display boundaries.
 * Pure policy: no game globals, file access or allocation. Invalid extents
 * fail closed to native; a room cap never changes canvas or UI dimensions. */
void Port_Viewport_PublishScene(PortViewportScene scene);

/* One translation for the complete original fixed-screen composition. This
 * rectangle does not describe the world aperture or live-gameplay UI bounds. */
PortViewportRect Port_Viewport_NativeContentRect(const PortViewportProfile* profile);

/* Camera rest on either axis, in world pixels. Oversized views/small rooms
 * pin to origin. Invalid extents fail closed to origin. Arithmetic is widened
 * before clamping so even extreme input coordinates cannot overflow. */
int Port_Viewport_CameraRest(int target, int origin, int roomExtent, int viewExtent);

/* BG rows needed by the unshaken (scroll & 15) + 8 vertical
 * offset. Zero means the requested height cannot fit a 32-row screenblock.
 * 160 lines require 23 rows (0x5c0 bytes); 192 require 27 (0x6c0 bytes). */
int Port_Viewport_MapTileRows(int viewHeight);
/* Classify the full fractional-scroll envelope with a vertical shake offset.
 * Zero means the visible interval can wrap outside one 32-row screenblock and
 * requires a native aperture or an explicit remapping policy. */
int Port_Viewport_MapTileRowsWithShake(int viewHeight, int shakeY);
/* Classify all eight XY pairs of a shake sequence before its first frame. */
bool Port_Viewport_ShakeSequenceFits(int viewHeight, const uint8_t* xyPairs);

#ifdef __cplusplus
}
#endif

#endif
