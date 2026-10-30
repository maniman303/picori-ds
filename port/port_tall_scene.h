#ifndef PORT_TALL_SCENE_H
#define PORT_TALL_SCENE_H

#include <stdbool.h>
#include <stdint.h>

#include "port_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * port_tall_scene.h — pure policy for a taller-than-native live world.
 *
 * This module owns the one decision "may the live world use aperture height H
 * this frame?" It deliberately has no game globals, file access or allocation
 * so the same rules can be exercised without a ROM. The callers in
 * port_linked_stubs.c gather the scene facts and staged-map snapshot; the
 * published viewport consumes only their result.
 *
 * No function here names 192, 4:3 or any other profile. Apertures are derived
 * from the published canvas and the room, so a future profile (for example a
 * proper 288x162 16:9 canvas) needs configuration and acceptance evidence, not
 * another implementation.
 */

/* Candidate live-world aperture height for a canvas/room pair. Rooms at or
 * below native height keep a 160-row aperture (centered by the viewport when
 * the canvas is taller), so the caller can still publish a smaller world. */
int Port_TallScene_ApertureHeight(int canvasHeight, int roomHeight);

/* Map rows required by an aperture, including the active shake offset and the
 * whole shake sequence when the room is shaking. Zero means the requested
 * aperture cannot be covered by one 32-row screenblock. */
int Port_TallScene_RequiredRows(int apertureHeight, int shakeY, const uint8_t* shakePairs, bool shaking);

/* Everything the validator is allowed to know. Residency fields are booleans
 * because the engine-side policies (manager allow-list, entity classes, OAM
 * metadata) are owned by their own audits; this module only combines them. */
typedef struct {
    /* Identity of the room the camera and staged map rows belong to. */
    uint64_t roomKey;
    int tileX, tileY;

    /* Requested live-world aperture and the rows that aperture needs. */
    int apertureHeight;
    int requiredRows;

    /* Scene classification. */
    bool mapLive;
    bool transitionNative;
    bool shakeSafe;
    /* Map shadow rows needed by the candidate world width are installed. */
    bool shadowRowsReady;

    /* Residency policy supplied by the scene/entity/OAM boundaries. */
    bool managersSafe;
    bool entitiesSafe;
    bool oamVerified;

    /* Last completed two-phase map fill/upload. */
    uint64_t uploadRoomKey;
    int uploadTileX, uploadTileY;
    int uploadHeight, uploadRows;
    bool uploadFilled;
    bool uploadUploaded;
    /* True when the upload boundary already committed this exact room (see
     * Port_Widescreen_SetTallValidated). Steady frames keep using that
     * commitment instead of requiring a fresh DMA each frame. */
    bool residencyCommitted;
} PortTallSceneInputs;

enum {
    PORT_TALL_STATUS_NONE = 0,
    /* Independent gates, published for diagnostics and tests. */
    PORT_TALL_STATUS_MANAGERS_SAFE = 1u << 0,
    PORT_TALL_STATUS_ENTITIES_SAFE = 1u << 1,
    PORT_TALL_STATUS_OAM_VERIFIED = 1u << 2,
    PORT_TALL_STATUS_MAP_LIVE = 1u << 3,
    PORT_TALL_STATUS_SHAKE_SAFE = 1u << 4,
    /* Staged fill and VRAM upload cover this room, tile origin and row count. */
    PORT_TALL_STATUS_ROWS_READY = 1u << 5,
    /* Right/bottom shadow tilemap rows are resident for the candidate width. */
    PORT_TALL_STATUS_SHADOW_ROWS_READY = 1u << 6,
    /* Every gate above passed and no classified native transition is active. */
    PORT_TALL_STATUS_VALIDATED = 1u << 7
};

unsigned Port_TallScene_Evaluate(const PortTallSceneInputs* inputs);
bool Port_TallScene_IsValidated(unsigned status);

/* Manager/entity residency policy. These are pure decisions; the engine-side
 * scanners in port_linked_stubs.c supply the live manager/entity state. */
bool Port_TallScene_ManagerIdAllowed(unsigned id, int apertureHeight, bool residencyProven);
bool Port_TallScene_EntityIdAllowed(unsigned kind, unsigned id, int apertureHeight);

/* Classified effects: a live tall aperture is safe only when no hardware
 * window/affine mode is active and any HDMA is the verified constant-brightness
 * form that the PPU can extend. Rolling/portal windows, triangles/ellipses,
 * local lights and unclassified windows therefore stay native. */
bool Port_TallScene_EffectAllowsTall(bool windows, int gbaMode, bool hdmaActive,
                                     bool hdmaConstantBrightness);

/* Per-manager residency proof. Only managers that stream char/palette data
 * from a view-dependent group need to call NoteManagerResidency() each update. */
void Port_TallScene_ManagerCreated(unsigned id);
void Port_TallScene_NoteManagerResidency(unsigned id, int safe);
bool Port_TallScene_ManagerResidencyProven(unsigned id);
void Port_TallScene_InvalidateManagerResidency(void);

#ifdef __cplusplus
}
#endif

#endif
