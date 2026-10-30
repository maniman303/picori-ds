#include "port_tall_scene.h"

#include <stddef.h>

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
