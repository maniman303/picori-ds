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
/* Compile-time raster/OBJ capacity, not a profile. Profiles publish their own
 * canvas; only a future profile taller than this bound would need a capacity
 * change (and the matching PPU/GPU work). */
#define PORT_VIEWPORT_MAX_WIDTH 569
#define PORT_VIEWPORT_MAX_HEIGHT 192

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

/* Translations for native gameplay widgets within a logical UI extent.
 * This describes geometry only: callers must establish widget ownership and
 * live-gameplay context before applying it. Invalid extents fail closed. */
typedef enum {
    PORT_UI_TOP_LEFT,
    PORT_UI_TOP_RIGHT,
    PORT_UI_BOTTOM_LEFT,
    PORT_UI_BOTTOM_RIGHT,
    PORT_UI_TOP_CENTER
} PortUIAnchor;
typedef struct { int x, y; } PortUITranslation;
PortUITranslation Port_Viewport_UITranslation(PortUIAnchor anchor, int width, int height);

/* Resolve a stable translation for an ordinary gameplay message. `finalRect`
 * describes the fully open native frame, including its border. Only the
 * standard top/bottom positions are classified; custom windows fail closed.
 * Standard top messages stay at the native top on every canvas; standard
 * bottom messages move to the canvas bottom inset (height - 160). The rule is
 * derived from the published height, not a profile id, so every future canvas
 * behaves like the 160-row one. The same translation must be applied to every
 * animated rectangle and OBJ. */
bool Port_Viewport_StandardMessageTranslation(PortViewportRect finalRect, int width, int height,
                                              PortUITranslation* translation);

/* Activation-time lookup by canonical override directory, e.g. "43".
 * Unknown/NULL keys return NULL; do not silently substitute another ratio.
 * Profiles describe the asset contract, not current renderer capabilities. */
const PortViewportProfile* Port_Viewport_FindProfile(const char* directory);
const PortViewportProfile* Port_Viewport_ProfileForAspect(PortAspectMode aspect);

/* Reject insufficient capacity rather than truncating the selected profile.
 * A renderer must also validate raster/sprite support before exposing a mode. */
bool Port_Viewport_ProfileFits(const PortViewportProfile* profile, int capacityWidth, int capacityHeight);

/* Current raster/OBJ support is separate from allocated capacity. Tall
 * canvases are release-enabled (see PORT_VIEWPORT_RELEASE_TALL below); any
 * width beyond the OBJ-safe bound is rejected. Availability is decided from
 * the published profile/capacity, not from a profile-specific case. */
bool Port_Viewport_AspectSupported(PortAspectMode aspect, int capacityWidth, int capacityHeight);

/* Release capability for canvases taller than the native 160 rows. It is on
 * in production builds; the xmake `tall_aspects` option can turn it off for
 * bisects/native-only test builds. The per-room scene validator plus
 * manager/entity/OAM/effect gates still decide whether a given room uses the
 * tall aperture; Minish Village groups are rendered from host charsets. */
#ifndef PORT_VIEWPORT_RELEASE_TALL
#define PORT_VIEWPORT_RELEASE_TALL 1
#endif

enum {
    PORT_VIEWPORT_PARITY = 1u << 0,
    PORT_VIEWPORT_UNSUPPORTED = 1u << 1
};

/* Semantic inputs supplied by the engine adapter, never inferred in pixel
 * loops. The per-frame validator combines these flags into the tall
 * aperture decision. */
enum {
    PORT_SCENE_WORLD = 1u << 0,
    PORT_SCENE_WIDE_WORLD = 1u << 1,
    PORT_SCENE_MAP_LIVE = 1u << 2,
    PORT_SCENE_NATIVE_TRANSITION = 1u << 3,
    PORT_SCENE_NATIVE_CONTENT = 1u << 4,
    /* The active shake sequence cannot cover a 192-row map window. */
    PORT_SCENE_TALL_SHAKE_UNSAFE = 1u << 5,
    /* Manager-free is a conservative residency gate, not evidence that
     * entity, special-BG or compositor/layout work is complete. */
    PORT_SCENE_TALL_MANAGER_FREE = 1u << 6,
    PORT_SCENE_TALL_VALIDATED = 1u << 7,
    PORT_SCENE_TALL_ROWS_READY = 1u << 8,
    PORT_SCENE_TALL_ENTITIES_EMPTY = 1u << 9,
    PORT_SCENE_TALL_OAM_VERIFIED = 1u << 10,
    /* Complete native screen rendered as a single 240x160 composition. */
    PORT_SCENE_FIXED_SCREEN = 1u << 11,
    /* A classified transition/effect cannot use the selected canvas. Render
     * one native 240x160 composition, centered in the canvas by presentation,
     * with no gameplay or static artwork in the margins. */
    PORT_SCENE_NATIVE_FALLBACK = 1u << 12
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

/* One of the two compositions (plus the classified native fallback) every
 * consumer must agree on. Rendering, layout, HD text, screenshots and
 * presentation read this instead of re-deriving policy from the canvas. */
typedef enum {
    PORT_COMPOSITION_NATIVE = 0,   /* native/wide direct frame */
    PORT_COMPOSITION_LIVE_WORLD,   /* world aperture + canvas HUD/dialogue */
    PORT_COMPOSITION_FIXED_SCREEN, /* one native 240x160 screen + margins */
    PORT_COMPOSITION_NATIVE_FALLBACK /* native frame, procedural margins */
} PortViewportComposition;
PortViewportComposition Port_Viewport_Composition(const PortViewportGeometry* geometry);

/* Publish only at a simulation boundary, before shadow/camera/layout work.
 * Renderers read the same snapshot, including repeat presents while paused.
 * Storage remains valid until the next publication after workers complete.
 * Unsupported preferences are retained but use effective native geometry.
 * False means invalid capacity/pitch; the previous snapshot is retained.
 * Scene decisions are retained and recomputed against the new canvas. */
bool Port_Viewport_Publish(PortAspectMode selected, bool consoleParity,
                           int capacityWidth, int capacityHeight, int pitchPixels);
const PortViewportGeometry* Port_Viewport_Get(void);
/* Game world raster must obey the room aperture, even when the canvas is taller. */
int Port_Viewport_RasterHeight(const PortViewportGeometry* geometry);

/* Simulation-side publication at room/transition and display boundaries.
 * Pure policy: no game globals, file access or allocation. Invalid extents
 * fail closed to native; a room cap never changes canvas or UI dimensions. */
void Port_Viewport_PublishScene(PortViewportScene scene);

/* Room aperture policy, also usable to validate a prospective taller canvas
 * while its profile is still disabled. TALL_VALIDATED must be supplied only
 * after the separate scene, entity and presentation gates are satisfied. */
PortViewportRect Port_Viewport_ResolveWorldRect(PortViewportRect canvas, PortViewportScene scene);

/* Two-phase map staging. A taller aperture may be published only after its
 * exact room/tile origin and row footprint were filled and uploaded. */
typedef struct {
    uint64_t roomKey;
    int tileX, tileY, height, rows;
    bool requested, filled, uploaded;
} PortViewportUpload;
void Port_Viewport_UploadReset(PortViewportUpload* upload);
bool Port_Viewport_UploadRequest(PortViewportUpload* upload, uint64_t roomKey, int tileX, int tileY,
                                 int height, int rows);
void Port_Viewport_UploadFilled(PortViewportUpload* upload, uint64_t roomKey, int tileX, int tileY, int rows);
void Port_Viewport_UploadCompleted(PortViewportUpload* upload, uint64_t roomKey, int tileX, int tileY, int rows);


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
int Port_Viewport_ShakeSequenceRows(int viewHeight, const uint8_t* xyPairs);
bool Port_Viewport_ShakeSequenceFits(int viewHeight, const uint8_t* xyPairs);

#ifdef __cplusplus
}
#endif

#endif
