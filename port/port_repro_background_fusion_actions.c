/* Repro-only native fusion input/ownership/lifetime audit. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "fade.h"
#include "game.h"
#include "kinstone.h"
#include "main.h"
#include "menu.h"
#include "message.h"
#include "npc.h"
#include "player.h"
#include "save.h"
#include "subtask.h"
#include "port_gba_mem.h"
#include "port_repro.h"

extern u32 GetAmountInKinstoneBag(KinstoneId id);

static int sCase = -1;
int Port_ReproBackground_FusionActionCase(void) { return sCase; }

int Port_ReproBackground_FusionActionsTick(unsigned frame) {
    static int phase, originalArea, originalRoom;
    static unsigned ready, since, deadline;
    static Entity partner;
    const int gameplay = gMain.task == TASK_GAME && gMain.state == GAMETASK_MAIN &&
                         gMain.substate == GAMEMAIN_UPDATE && !gFadeControl.active;
    const int fusion = gMain.task == TASK_GAME && gMain.substate == GAMEMAIN_SUBTASK &&
                       gUI.lastState == SUBTASK_KINSTONEMENU && gUI.nextToLoad == 2;
    const int selecting = fusion && gMenu.menuType == 1 && gMenu.column_idx == 2 && !gFadeControl.active;
    const int mode = sCase < 0 ? 0 : sCase % 6;
    if (phase == 9) return 0;
    if (deadline && frame > deadline) {
        fprintf(stderr, "[background-audit] fusion action %d timeout phase=%d ui=%u menu=%u/%u\n",
                sCase, phase, gUI.lastState, gMenu.menuType, gMenu.overlayType);
        exit(2);
    }
    if (!phase) {
        if (!gameplay) return 0;
        if (++sCase == 18) {
            Port_ReproBackground_Event("fusion_actions_complete", 18);
            phase = 9;
            return 2;
        }
        /* Independent fixtures; native code sorts the bag and consumes pieces.
         * 109 has shape 9, 110 has shape 11 in the EU event table. */
        memset(&gSave.kinstones, 0, sizeof(gSave.kinstones));
        AddKinstoneToBag(109); AddKinstoneToBag(109);
        AddKinstoneToBag(110); AddKinstoneToBag(110);
        gSave.stats.maxHealth = 24;
        gSave.stats.health = sCase / 6 == 0 ? 24 : sCase / 6 == 1 ? 15 : 8;
        originalArea = gRoomControls.area;
        originalRoom = gRoomControls.room;
        partner.kind = NPC;
        partner.id = TINGLE_SIBLINGS;
        InitializeFuseInfo(&partner, 0, 0, 0);
        gFuseInfo.kinstoneId = sCase % 6 == 4 ? KINSTONE_SOURCE_FLOW : KINSTONE_A;
        Port_ReproBackground_Event("fusion_action_request", sCase);
        MenuFadeIn(SUBTASK_KINSTONEMENU, 0);
        ready = 0;
        deadline = frame + 4000;
        phase = 1;
        return 0;
    }
    if (phase == 1 && selecting) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        Port_ReproBackground_Event("fusion_action_ready", sCase);
        since = frame;
        phase = 2;
        return 1;
    }
    if (phase == 2 && frame >= since + 4) {
        gSave.stats.health = sCase / 6 == 2 ? 24 : 8;
        Port_ReproBackground_Event("fusion_action_health_mutated", gSave.stats.health);
        since = frame;
        phase = 3;
    }
    if (phase == 3 && frame >= since + 12) {
        Port_ReproBackground_Event("fusion_action_held", sCase);
        since = frame;
        phase = 4;
        return 1;
    }
    if (phase == 4 && selecting && frame >= since + 6) {
        if (mode == 5 && gGenericMenu.unk28 == 0) {
            *(vu16*)(gIoMem + 0x130) &= ~DPAD_DOWN;
            since = frame;
            return 0;
        }
        const u16 key = mode == 0 ? B_BUTTON : mode == 1 ? START_BUTTON : mode == 2 ? L_BUTTON : A_BUTTON;
        Port_ReproBackground_Event("fusion_action_input", key);
        *(vu16*)(gIoMem + 0x130) &= ~key;
        ready = 0;
        phase = mode == 3 ? 5 : 7;
    }
    if (phase == 5 && selecting) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        Port_ReproBackground_Event("fusion_action_mismatch_returned", sCase);
        if (GetAmountInKinstoneBag(109) != 2 || GetAmountInKinstoneBag(110) != 2 ||
            gSave.kinstones.fusedCount || CheckKinstoneFused(gFuseInfo.kinstoneId)) {
            fprintf(stderr, "[background-audit] mismatched fusion changed ownership\n");
            exit(2);
        }
        since = frame;
        phase = 6;
        return 1;
    }
    if (phase == 6 && frame >= since + 6) {
        Port_ReproBackground_Event("fusion_action_cancel_request", B_BUTTON);
        *(vu16*)(gIoMem + 0x130) &= ~B_BUTTON;
        ready = 0;
        phase = 7;
    }
    if (phase == 7) {
        /* Advance the native success text and later local-map hint with key
         * edges. World-event timers/graphics run without direct state writes. */
        if ((fusion && gMenu.menuType == 5 && gMenu.overlayType == 1 && (gMessage.state & MESSAGE_ACTIVE)) ||
            (gMain.substate == GAMEMAIN_SUBTASK && gUI.lastState == SUBTASK_LOCALMAPHINT &&
             gUI.nextToLoad == 2 && gMenu.menuType == 1 && !gMenu.transitionTimer && !gFadeControl.active)) {
            if (frame % 8 == 0) *(vu16*)(gIoMem + 0x130) &= ~A_BUTTON;
        }
        if (!gameplay) return 0;
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        const u32 success = mode >= 4;
        const u32 piece109 = mode == 4 ? 1 : 2, piece110 = mode == 5 ? 1 : 2;
        if (GetAmountInKinstoneBag(109) != piece109 || GetAmountInKinstoneBag(110) != piece110 ||
            gSave.kinstones.fusedCount != success || CheckKinstoneFused(gFuseInfo.kinstoneId) != success ||
            gRoomControls.area != originalArea || gRoomControls.room != originalRoom ||
            gFuseInfo.fusionState != (success ? FUSION_STATE_6 : FUSION_STATE_5)) {
            fprintf(stderr, "[background-audit] fusion action %d ownership/room/result mismatch\n", sCase);
            exit(2);
        }
        Port_ReproBackground_Event("fusion_action_game_restored", sCase);
        phase = 8;
        return 1;
    }
    if (phase == 8) { ready = 0; phase = 0; }
    return 0;
}
