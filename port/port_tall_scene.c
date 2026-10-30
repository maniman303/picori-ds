#include "port_tall_scene.h"

#include <stddef.h>

#include "manager.h"
#include "object.h"
#include "projectile.h"

int Port_TallScene_ApertureHeight(int canvasHeight, int roomHeight) {
    if (canvasHeight <= PORT_VIEWPORT_NATIVE_HEIGHT || roomHeight <= PORT_VIEWPORT_NATIVE_HEIGHT)
        return PORT_VIEWPORT_NATIVE_HEIGHT;
    int aperture = canvasHeight;
    if (aperture > PORT_VIEWPORT_MAX_HEIGHT)
        aperture = PORT_VIEWPORT_MAX_HEIGHT;
    if (roomHeight < aperture)
        aperture = roomHeight;
    return aperture > PORT_VIEWPORT_NATIVE_HEIGHT ? aperture : PORT_VIEWPORT_NATIVE_HEIGHT;
}

int Port_TallScene_RequiredRows(int apertureHeight, int shakeY, const uint8_t* shakePairs, bool shaking) {
    if (apertureHeight <= PORT_VIEWPORT_NATIVE_HEIGHT || apertureHeight > PORT_VIEWPORT_MAX_HEIGHT)
        return 0;
    int rows = Port_Viewport_MapTileRowsWithShake(apertureHeight, shakeY);
    if (!rows)
        return 0;
    if (shaking) {
        const int sequenceRows = Port_Viewport_ShakeSequenceRows(apertureHeight, shakePairs);
        if (!sequenceRows)
            return 0;
        if (sequenceRows > rows)
            rows = sequenceRows;
    }
    return rows;
}

static bool Port_TallScene_UploadMatches(const PortTallSceneInputs* in) {
    return in->uploadFilled && in->uploadUploaded &&
           in->uploadRoomKey == in->roomKey &&
           in->uploadTileX == in->tileX && in->uploadTileY == in->tileY &&
           in->uploadHeight == in->apertureHeight &&
           in->uploadRows >= in->requiredRows;
}

unsigned Port_TallScene_Evaluate(const PortTallSceneInputs* in) {
    if (!in || in->apertureHeight <= PORT_VIEWPORT_NATIVE_HEIGHT || in->apertureHeight > PORT_VIEWPORT_MAX_HEIGHT ||
        in->requiredRows <= 0 || in->requiredRows > 32)
        return PORT_TALL_STATUS_NONE;

    unsigned status = PORT_TALL_STATUS_NONE;
    if (in->managersSafe)
        status |= PORT_TALL_STATUS_MANAGERS_SAFE;
    if (in->entitiesSafe)
        status |= PORT_TALL_STATUS_ENTITIES_SAFE;
    if (in->oamVerified)
        status |= PORT_TALL_STATUS_OAM_VERIFIED;
    if (in->mapLive)
        status |= PORT_TALL_STATUS_MAP_LIVE;
    if (in->shakeSafe)
        status |= PORT_TALL_STATUS_SHAKE_SAFE;
    if (in->shadowRowsReady)
        status |= PORT_TALL_STATUS_SHADOW_ROWS_READY;
    if (Port_TallScene_UploadMatches(in) || in->residencyCommitted)
        status |= PORT_TALL_STATUS_ROWS_READY;

    const unsigned required = PORT_TALL_STATUS_MANAGERS_SAFE | PORT_TALL_STATUS_ENTITIES_SAFE |
                              PORT_TALL_STATUS_OAM_VERIFIED | PORT_TALL_STATUS_MAP_LIVE |
                              PORT_TALL_STATUS_SHAKE_SAFE | PORT_TALL_STATUS_SHADOW_ROWS_READY |
                              PORT_TALL_STATUS_ROWS_READY;
    if (!in->transitionNative && (status & required) == required)
        status |= PORT_TALL_STATUS_VALIDATED;
    return status;
}

bool Port_TallScene_IsValidated(unsigned status) {
    return (status & PORT_TALL_STATUS_VALIDATED) != 0;
}

/* Managers that only own logic, entity spawning, audio, HUD text or a tile
 * edit do not change which char/palette data the map rows reference. They are
 * safe at every supported aperture. */
static bool Port_TallSceneManagerIsLogic(unsigned id) {
    switch (id) {
        case MINISH_PORTAL_MANAGER:
        case DIGGING_CAVE_ENTRANCE_MANAGER:
        case BRIDGE_MANAGER:
        case SPECIAL_WARP_MANAGER:
        case EZLO_HINT_MANAGER:
        case FIGHT_MANAGER:
        case TILE_CHANGE_OBSERVE_MANAGER:
        case ENTITY_SPAWN_MANAGER:
        case MISC_MANAGER:
        case WEATHER_CHANGE_MANAGER:
        case FLAG_AND_OPERATOR_MANAGER:
        case HOUSE_SIGN_MANAGER:
        case DELAYED_ENTITY_LOAD_MANAGER:
        case FALLING_ITEM_MANAGER:
        case HOLE_MANAGER:
        case REGION_TRIGGER_MANAGER:
        case RAIL_INTERSECTION_MANAGER:
        case MOVEABLE_OBJECT_MANAGER:
        case MINISH_SIZED_ENTRANCE_MANAGER:
        case BOMBABLE_WALL_MANAGER:
        case FLAME_MANAGER:
        case PUSHABLE_FURNITURE_MANAGER:
        case ARMOS_INTERIOR_MANAGER:
        case ENEMY_INTERACTION_MANAGER:
        case MANAGER_29:
        case DESTRUCTIBLE_TILE_OBSERVE_MANAGER:
        case ANGRY_STATUE_MANAGER:
        case WATERFALL_BOTTOM_MANAGER:
        case SECRET_MANAGER:
        case TILE_PUZZLE_MANAGER:
        case GORON_MERCHANT_SHOP_MANAGER:
        case VAATI_APPARATE_MANAGER:
        case HYRULE_TOWN_BELL_MANAGER:
        case CAMERA_TARGET_MANAGER:
        case REPEATED_SOUND_MANAGER:
        case FLOATING_PLATFORM_MANAGER:
        case ENTER_ROOM_TEXTBOX_MANAGER:
        /* BG3 overlays backed by static/repeating assets: the live-world
         * composition clips them to the aperture and they never stream map
         * char data from a view-dependent group. */
        case CLOUD_OVERLAY_MANAGER:
        case STEAM_OVERLAY_MANAGER:
        case RAINFALL_MANAGER:
        case ANIMATED_BACKGROUND_MANAGER:
        case STATIC_BACKGROUND_MANAGER:
        case POW_BACKGROUND_MANAGER:
        /* Parallax path/rafter backgrounds generate a 32-row (256 px) map
         * from the complete room source and now normalize against the
         * candidate aperture, so 192 rows stay covered. */
        case HORIZONTAL_MINISH_PATH_BACKGROUND_MANAGER:
        case VERTICAL_MINISH_PATH_BACKGROUND_MANAGER:
        case MINISH_RAFTERS_BACKGROUND_MANAGER:
            return true;
        default:
            return false;
    }
}

/* Managers that stream char/palette residency from the view-dependent region
 * group. They must prove a single group covers the candidate aperture. */
static bool Port_TallSceneManagerNeedsResidencyProof(unsigned id) {
    return id == HYRULE_TOWN_TILESET_MANAGER || id == MINISH_VILLAGE_MANAGER;
}

bool Port_TallScene_ManagerIdAllowed(unsigned id, int apertureHeight, bool residencyProven) {
    if (apertureHeight <= PORT_VIEWPORT_NATIVE_HEIGHT)
        return true;
    if (Port_TallSceneManagerNeedsResidencyProof(id))
        return residencyProven;
    return Port_TallSceneManagerIsLogic(id);
}

bool Port_TallScene_EntityIdAllowed(unsigned kind, unsigned id, int apertureHeight) {
    if (apertureHeight <= PORT_VIEWPORT_NATIVE_HEIGHT)
        return true;
    switch (kind) {
        case PLAYER:
        case NPC:
        case ENEMY:
        case PROJECTILE:
        case OBJECT:
        case PLAYER_ITEM:
            break;
        default:
            return false;
    }
    if (kind == OBJECT) {
        switch (id) {
            /* Native-only special painters/affine tables. */
            case BARREL_SPIDERWEB:
            case PARALLAX_ROOM_VIEW:
            case MINISH_PORTAL_CLOSEUP:
                return false;
            default:
                break;
        }
    }
    if (kind == PROJECTILE && id == SPIDER_WEB)
        return false;
    return true;
}

static uint64_t Port_TallSceneResidencyMask(void) {
    return (1ull << HYRULE_TOWN_TILESET_MANAGER) | (1ull << MINISH_VILLAGE_MANAGER);
}

static uint64_t sManagerResidencyUnsafe;

void Port_TallScene_ManagerCreated(unsigned id) {
    if (id < 64 && Port_TallSceneManagerNeedsResidencyProof(id))
        sManagerResidencyUnsafe |= 1ull << id;
}

void Port_TallScene_NoteManagerResidency(unsigned id, int safe) {
    if (id >= 64 || !Port_TallSceneManagerNeedsResidencyProof(id))
        return;
    if (safe)
        sManagerResidencyUnsafe &= ~(1ull << id);
    else
        sManagerResidencyUnsafe |= 1ull << id;
}

bool Port_TallScene_ManagerResidencyProven(unsigned id) {
    if (id >= 64 || !Port_TallSceneManagerNeedsResidencyProof(id))
        return true;
    return (sManagerResidencyUnsafe & (1ull << id)) == 0;
}

void Port_TallScene_InvalidateManagerResidency(void) {
    sManagerResidencyUnsafe = Port_TallSceneResidencyMask();
}
