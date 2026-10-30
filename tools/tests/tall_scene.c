/* Pure tall-scene policy: profile-general aperture, row coverage and the
 * single validation decision. No game state or ROM data is involved. */
#include "port_tall_scene.h"

#include "manager.h"
#include "object.h"
#include "projectile.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static PortTallSceneInputs valid_inputs(void) {
    PortTallSceneInputs in;
    memset(&in, 0, sizeof(in));
    in.roomKey = 0x0102030405060708ull;
    in.tileX = 4;
    in.tileY = 2;
    in.apertureHeight = 192;
    in.requiredRows = 27;
    in.mapLive = true;
    in.shakeSafe = true;
    in.shadowRowsReady = true;
    in.managersSafe = true;
    in.entitiesSafe = true;
    in.oamVerified = true;
    in.uploadRoomKey = in.roomKey;
    in.uploadTileX = in.tileX;
    in.uploadTileY = in.tileY;
    in.uploadHeight = 192;
    in.uploadRows = 27;
    in.uploadFilled = true;
    in.uploadUploaded = true;
    return in;
}

int main(void) {
    /* Apertures are published from canvas and room, never a profile constant. */
    assert(Port_TallScene_ApertureHeight(160, 192) == 160);
    assert(Port_TallScene_ApertureHeight(192, 160) == 160);
    assert(Port_TallScene_ApertureHeight(192, 192) == 192);
    assert(Port_TallScene_ApertureHeight(192, 176) == 176);
    assert(Port_TallScene_ApertureHeight(162, 160) == 160);
    assert(Port_TallScene_ApertureHeight(162, 176) == 162);
    assert(Port_TallScene_ApertureHeight(162, 192) == 162);
    assert(Port_TallScene_ApertureHeight(192, 1024) == 192);
    assert(Port_TallScene_ApertureHeight(384, 384) == 192);
    assert(Port_TallScene_ApertureHeight(240, 240) == 192);
    assert(Port_TallScene_ApertureHeight(151, 192) == 160);
    assert(Port_TallScene_ApertureHeight(192, 144) == 160);

    /* Row coverage, parameterized across every supported aperture. */
    assert(Port_TallScene_RequiredRows(160, 0, NULL, false) == 0);
    assert(Port_TallScene_RequiredRows(159, 0, NULL, false) == 0);
    assert(Port_TallScene_RequiredRows(193, 0, NULL, false) == 0);
    assert(Port_TallScene_RequiredRows(162, 0, NULL, false) == 24);
    assert(Port_TallScene_RequiredRows(176, 0, NULL, false) == 25);
    assert(Port_TallScene_RequiredRows(192, 0, NULL, false) == 27);
    assert(Port_TallScene_RequiredRows(192, 8, NULL, false) == 28);
    /* A negative offset keeps the unshaken window as the minimum. */
    assert(Port_TallScene_RequiredRows(192, -8, NULL, false) == 27);
    /* Coverage that cannot fit one 32-row screenblock fails closed. */
    assert(Port_TallScene_RequiredRows(192, 64, NULL, false) == 0);

    uint8_t shake[16];
    memset(shake, 0, sizeof(shake));
    shake[1] = 8;
    assert(Port_TallScene_RequiredRows(192, 0, shake, false) == 27);
    assert(Port_TallScene_RequiredRows(192, 0, shake, true) == 28);
    memset(shake, 0, sizeof(shake));
    shake[1] = (uint8_t)-9; /* whole-sequence wrap is unsafe */
    assert(Port_TallScene_RequiredRows(192, 0, shake, true) == 0);

    PortTallSceneInputs in = valid_inputs();
    unsigned status = Port_TallScene_Evaluate(&in);
    assert(Port_TallScene_IsValidated(status));
    assert((status & PORT_TALL_STATUS_ROWS_READY) != 0);
    assert((status & PORT_TALL_STATUS_MAP_LIVE) != 0);
    assert((status & PORT_TALL_STATUS_SHAKE_SAFE) != 0);
    assert((status & PORT_TALL_STATUS_SHADOW_ROWS_READY) != 0);
    assert((status & PORT_TALL_STATUS_MANAGERS_SAFE) != 0);
    assert((status & PORT_TALL_STATUS_ENTITIES_SAFE) != 0);
    assert((status & PORT_TALL_STATUS_OAM_VERIFIED) != 0);

    /* Every gate is independently required. */
    const unsigned gates[] = {
        PORT_TALL_STATUS_MANAGERS_SAFE, PORT_TALL_STATUS_ENTITIES_SAFE,
        PORT_TALL_STATUS_OAM_VERIFIED, PORT_TALL_STATUS_MAP_LIVE,
        PORT_TALL_STATUS_SHAKE_SAFE, PORT_TALL_STATUS_SHADOW_ROWS_READY,
        PORT_TALL_STATUS_ROWS_READY
    };
    for (unsigned i = 0; i < sizeof(gates) / sizeof(gates[0]); ++i) {
        in = valid_inputs();
        status = Port_TallScene_Evaluate(&in);
        assert((status & gates[i]) != 0);
        assert((status & PORT_TALL_STATUS_VALIDATED) != 0);
        switch (gates[i]) {
            case PORT_TALL_STATUS_MANAGERS_SAFE: in.managersSafe = false; break;
            case PORT_TALL_STATUS_ENTITIES_SAFE: in.entitiesSafe = false; break;
            case PORT_TALL_STATUS_OAM_VERIFIED: in.oamVerified = false; break;
            case PORT_TALL_STATUS_MAP_LIVE: in.mapLive = false; break;
            case PORT_TALL_STATUS_SHAKE_SAFE: in.shakeSafe = false; break;
            case PORT_TALL_STATUS_SHADOW_ROWS_READY: in.shadowRowsReady = false; break;
            default: in.uploadUploaded = false; break;
        }
        status = Port_TallScene_Evaluate(&in);
        assert(!Port_TallScene_IsValidated(status));
    }

    /* Stale/mismatched staging never validates. */
    in = valid_inputs();
    in.uploadRoomKey += 1;
    assert(!Port_TallScene_IsValidated(Port_TallScene_Evaluate(&in)));
    in = valid_inputs();
    in.uploadTileX += 1;
    assert(!Port_TallScene_IsValidated(Port_TallScene_Evaluate(&in)));
    in = valid_inputs();
    in.uploadTileY += 1;
    assert(!Port_TallScene_IsValidated(Port_TallScene_Evaluate(&in)));
    in = valid_inputs();
    in.uploadHeight = 176;
    assert(!Port_TallScene_IsValidated(Port_TallScene_Evaluate(&in)));
    in = valid_inputs();
    in.uploadRows = 26; /* required 27 */
    assert(!Port_TallScene_IsValidated(Port_TallScene_Evaluate(&in)));
    in = valid_inputs();
    in.uploadFilled = false;
    assert(!Port_TallScene_IsValidated(Port_TallScene_Evaluate(&in)));
    in = valid_inputs();
    in.uploadUploaded = false;
    assert(!Port_TallScene_IsValidated(Port_TallScene_Evaluate(&in)));

    /* A commitment from the upload boundary keeps steady frames validated
     * without requiring a fresh DMA, but only for the same aperture rows. */
    in = valid_inputs();
    in.uploadFilled = false;
    in.uploadUploaded = false;
    in.residencyCommitted = true;
    assert(Port_TallScene_IsValidated(Port_TallScene_Evaluate(&in)));
    in.entitiesSafe = false;
    assert(!Port_TallScene_IsValidated(Port_TallScene_Evaluate(&in)));

    /* A classified native transition suppresses publication but preserves
     * the independent gate diagnostics. */
    in = valid_inputs();
    in.transitionNative = true;
    status = Port_TallScene_Evaluate(&in);
    assert((status & PORT_TALL_STATUS_ROWS_READY) != 0);
    assert(!Port_TallScene_IsValidated(status));

    /* Non-tall apertures and unusable row counts are not tall scenes. */
    in = valid_inputs();
    in.apertureHeight = 160;
    in.uploadHeight = 160;
    assert(Port_TallScene_Evaluate(&in) == PORT_TALL_STATUS_NONE);
    in = valid_inputs();
    in.requiredRows = 0;
    assert(Port_TallScene_Evaluate(&in) == PORT_TALL_STATUS_NONE);
    in = valid_inputs();
    in.requiredRows = 33;
    assert(Port_TallScene_Evaluate(&in) == PORT_TALL_STATUS_NONE);
    in = valid_inputs();
    in.apertureHeight = 193;
    assert(Port_TallScene_Evaluate(&in) == PORT_TALL_STATUS_NONE);

    /* A 162-row canvas (future proper 16:9) uses its own row count. */
    in = valid_inputs();
    in.apertureHeight = 162;
    in.requiredRows = 24;
    in.uploadHeight = 162;
    in.uploadRows = 24;
    assert(Port_TallScene_IsValidated(Port_TallScene_Evaluate(&in)));

    assert(!Port_TallScene_IsValidated(Port_TallScene_Evaluate(NULL)));

    /* Classified effects: steady rooms may use the canvas; windows, affine
     * modes and non-constant HDMA keep the native aperture. */
    assert(Port_TallScene_EffectAllowsTall(false, 0, false, false));
    assert(Port_TallScene_EffectAllowsTall(false, 0, true, true));
    assert(!Port_TallScene_EffectAllowsTall(true, 0, false, false));
    assert(!Port_TallScene_EffectAllowsTall(true, 0, true, true));
    assert(!Port_TallScene_EffectAllowsTall(false, 1, false, false));
    assert(!Port_TallScene_EffectAllowsTall(false, 2, false, false));
    assert(!Port_TallScene_EffectAllowsTall(false, 0, true, false));

    /* Manager capability list: logic/HUD/overlay managers are allowed; the
     * view-dependent streamers require a single-group proof; genuinely
     * effect/streaming-owned managers stay native. */
    assert(Port_TallScene_ManagerIdAllowed(MISC_MANAGER, 192, false));
    assert(Port_TallScene_ManagerIdAllowed(ENTER_ROOM_TEXTBOX_MANAGER, 192, false));
    assert(Port_TallScene_ManagerIdAllowed(CLOUD_OVERLAY_MANAGER, 192, false));
    assert(Port_TallScene_ManagerIdAllowed(ANIMATED_BACKGROUND_MANAGER, 192, false));
    assert(!Port_TallScene_ManagerIdAllowed(ROLLING_BARREL_MANAGER, 192, true));
    assert(Port_TallScene_ManagerIdAllowed(HORIZONTAL_MINISH_PATH_BACKGROUND_MANAGER, 192, false));
    assert(Port_TallScene_ManagerIdAllowed(VERTICAL_MINISH_PATH_BACKGROUND_MANAGER, 162, false));
    assert(Port_TallScene_ManagerIdAllowed(MINISH_RAFTERS_BACKGROUND_MANAGER, 192, false));
    assert(!Port_TallScene_ManagerIdAllowed(TEMPLE_OF_DROPLETS_MANAGER, 192, true));
    assert(!Port_TallScene_ManagerIdAllowed(VAATI3_BACKGROUND_MANAGER, 192, true));
    assert(!Port_TallScene_ManagerIdAllowed(LIGHT_MANAGER, 162, true));
    assert(!Port_TallScene_ManagerIdAllowed(HYRULE_TOWN_TILESET_MANAGER, 192, false));
    assert(Port_TallScene_ManagerIdAllowed(HYRULE_TOWN_TILESET_MANAGER, 192, true));
    assert(!Port_TallScene_ManagerIdAllowed(MINISH_VILLAGE_MANAGER, 162, false));
    assert(Port_TallScene_ManagerIdAllowed(MINISH_VILLAGE_MANAGER, 162, true));
    assert(Port_TallScene_ManagerIdAllowed(ROLLING_BARREL_MANAGER, 160, true));
    assert(!Port_TallScene_ManagerIdAllowed(0xFF, 192, true));

    Port_TallScene_InvalidateManagerResidency();
    assert(!Port_TallScene_ManagerResidencyProven(HYRULE_TOWN_TILESET_MANAGER));
    assert(!Port_TallScene_ManagerResidencyProven(MINISH_VILLAGE_MANAGER));
    assert(Port_TallScene_ManagerResidencyProven(MISC_MANAGER));
    Port_TallScene_NoteManagerResidency(HYRULE_TOWN_TILESET_MANAGER, 1);
    assert(Port_TallScene_ManagerResidencyProven(HYRULE_TOWN_TILESET_MANAGER));
    Port_TallScene_NoteManagerResidency(HYRULE_TOWN_TILESET_MANAGER, 0);
    assert(!Port_TallScene_ManagerResidencyProven(HYRULE_TOWN_TILESET_MANAGER));
    Port_TallScene_ManagerCreated(MINISH_VILLAGE_MANAGER);
    assert(!Port_TallScene_ManagerResidencyProven(MINISH_VILLAGE_MANAGER));
    Port_TallScene_InvalidateManagerResidency();

    /* Entity classes: common gameplay is admitted; unique affine/special
     * painters stay native. Native apertures always admit. */
    assert(Port_TallScene_EntityIdAllowed(PLAYER, 0, 192));
    assert(Port_TallScene_EntityIdAllowed(NPC, 0, 192));
    assert(Port_TallScene_EntityIdAllowed(ENEMY, 0, 192));
    assert(Port_TallScene_EntityIdAllowed(PROJECTILE, 0, 192));
    assert(Port_TallScene_EntityIdAllowed(OBJECT, 0, 192));
    assert(Port_TallScene_EntityIdAllowed(PLAYER_ITEM, 0, 192));
    assert(!Port_TallScene_EntityIdAllowed(OBJECT, BARREL_SPIDERWEB, 192));
    assert(!Port_TallScene_EntityIdAllowed(OBJECT, PARALLAX_ROOM_VIEW, 192));
    assert(!Port_TallScene_EntityIdAllowed(OBJECT, MINISH_PORTAL_CLOSEUP, 192));
    assert(!Port_TallScene_EntityIdAllowed(PROJECTILE, SPIDER_WEB, 192));
    assert(!Port_TallScene_EntityIdAllowed(0, 0, 192));
    assert(!Port_TallScene_EntityIdAllowed(2, 0, 192));
    assert(Port_TallScene_EntityIdAllowed(OBJECT, BARREL_SPIDERWEB, 160));

    puts("tall scene validation: aperture, rows, managers, entities and gates PASS");
    return 0;
}
