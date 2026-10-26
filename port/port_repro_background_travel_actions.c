/* Repro-only EU interaction route; native text/input/fades/arrival run intact. */
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "fade.h"
#include "flags.h"
#include "item.h"
#include "game.h"
#include "main.h"
#include "menu.h"
#include "message.h"
#include "player.h"
#include "save.h"
#include "subtask.h"
#include "port_gba_mem.h"
#include "port_repro.h"

extern const Transition gUnk_08128024[];
static int sCase = -1;
int Port_ReproBackground_TravelActionCase(void) { return sCase; }

int Port_ReproBackground_TravelActionsTick(unsigned frame) {
    static int phase;
    static unsigned ready, since, deadline;
    if (phase == 8)
        return 0;
    if (!REGION_IS_EU) {
        fprintf(stderr, "[background-audit] travel_actions currently requires an EU ROM\n");
        exit(2);
    }
    if (phase == 9) {
        ++sCase;
        phase = 0;
    }
    if (deadline && frame > deadline) {
        fprintf(stderr, "[background-audit] travel action %d timed out (phase=%d menu=%u choice=%d)\n",
                sCase, phase, gMenu.menuType, Port_ReproBackground_MessageChoice());
        exit(2);
    }
    const int gameplay = gMain.task == TASK_GAME && gMain.state == GAMETASK_MAIN &&
                         gMain.substate == GAMEMAIN_UPDATE && !gFadeControl.active &&
                         gPlayerEntity.base.action == PLAYER_NORMAL && gPlayerState.controlMode == CONTROL_ENABLED;
    if (phase == 0) {
        if (!gameplay)
            return 0;
        if (sCase < 0) {
            sCase = 0;
            /* Legitimate ocarina-era story state avoids the gp=1 festival
             * redirect at Hyrule Town. WARP_1ST remains native/unset so the
             * first arrival still runs its real tutorial. */
            SetGlobalFlag(TABIDACHI);
            SetGlobalFlag(EZERO_1ST);
            SetLocalFlagByBankB(FLAG_BANK_1, SOUGEN_01_ZELDA);
            SetLocalFlagByBankB(FLAG_BANK_1, MORI_ENTRANCE_1ST);
            SetLocalFlagByBankB(FLAG_BANK_1, MORI_00_KOBITO);
            SetGlobalFlag(LV1_CLEAR);
            SetGlobalFlag(LV2_CLEAR);
            SetGlobalFlag(LV3_CLEAR);
            SetInventoryValue(ITEM_OCARINA, 1);
            UpdateGlobalProgress();
        }
        if (sCase == 54) {
            Port_ReproBackground_Event("travel_actions_complete", sCase);
            phase = 8;
            return 2;
        }
        /* Story hints can precede the first-use tutorial on arrival. */
        deadline = frame + 4000;
        const int health = sCase / 18;
        gSave.stats.maxHealth = 24;
        gSave.stats.health = health == 0 ? 24 : health == 1 ? 15 : 8;
        gSave.windcrests = ((sCase / 2) % 9 == 8 ? 0x80000000u : 0xff000000u) | 0x1ffffu;
        /* Request precedes MenuFadeIn, while live room coordinates are valid. */
        Port_ReproBackground_Event("travel_action_request", sCase);
        MenuFadeIn(SUBTASK_FASTTRAVEL, 0);
        ready = 0;
        phase = 1;
        return 0;
    }
    const int view = (sCase / 2) % 9;
    const int destination = view == 8 ? 7 : view;
    const int accept = sCase % 2;
    if (phase == 1) {
        if (gMain.task != TASK_GAME || gMain.substate != GAMEMAIN_SUBTASK ||
            gUI.nextToLoad != 2 || gUI.lastState != SUBTASK_FASTTRAVEL ||
            gMenu.menuType != 1 || gFadeControl.active)
            return 0;
        if (gMenu.field_0x3 != destination) {
            ready = 0;
            if (frame % 6 == 0)
                *(vu16*)(gIoMem + 0x130) &= ~DPAD_RIGHT;
            return 0;
        }
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("travel_action_ready", sCase);
        since = frame;
        phase = 2;
        return 1;
    }
    if (phase == 2 && frame >= since + 4) {
        gSave.stats.health = sCase / 18 == 2 ? 24 : 8;
        Port_ReproBackground_Event("travel_action_health_mutated", gSave.stats.health);
        since = frame;
        phase = 3;
    }
    if (phase == 3 && frame >= since + 12) {
        Port_ReproBackground_Event("travel_action_held", sCase);
        since = frame;
        phase = 4;
        return 1;
    }
    if (phase == 4 && frame >= since + 4) {
        *(vu16*)(gIoMem + 0x130) &= ~(view % 2 ? START_BUTTON : A_BUTTON);
        ready = 0;
        phase = 5;
    }
    if (phase == 5 && gMenu.menuType == 2) {
        const int choice = Port_ReproBackground_MessageChoice();
        if (choice < 0)
            return 0;
        if (choice != (accept ? 0 : 1)) {
            ready = 0;
            if (frame % 6 == 0)
                *(vu16*)(gIoMem + 0x130) &= ~DPAD_RIGHT;
            return 0;
        }
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("travel_action_prompt_ready", sCase);
        since = frame;
        phase = 6;
        return 1;
    }
    if (phase == 6 && frame >= since + 4) {
        *(vu16*)(gIoMem + 0x130) &= ~(view % 2 ? START_BUTTON : A_BUTTON);
        ready = 0;
        phase = 7;
    }
    /* A real arrival can trigger native Ezlo/tutorial pages. Advance only a
     * completed waiting page; never assign message results or player control. */
    if (phase == 7 && Port_ReproBackground_MessageWaiting() && frame % 6 == 0) {
        *(vu16*)(gIoMem + 0x130) &= ~A_BUTTON;
        Port_ReproBackground_Event("travel_arrival_message_advance", gMessage.textIndex);
    }
    if (phase == 7 && gameplay) {
        const Transition* target = &gUnk_08128024[destination];
        if (accept && (gRoomControls.area != target->area || gRoomControls.room != target->room))
            return 0;
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event(accept ? "travel_action_arrived" : "travel_action_restored", sCase);
        /* Keep event/capture identity until the next simulation boundary. */
        phase = 9;
        return 1;
    }
    return 0;
}
