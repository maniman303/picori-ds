/* Repro-only detail-map scrolling. Stage recipes; native code owns offsets. */
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "fade.h"
#include "flags.h"
#include "game.h"
#include "kinstone.h"
#include "item.h"
#include "main.h"
#include "menu.h"
#include "pauseMenu.h"
#include "save.h"
#include "subtask.h"
#include "port_gba_mem.h"
#include "port_repro.h"

extern void InitPauseMenu(void);
static int sCase = -1;
int Port_ReproBackground_DetailMapCase(void) { return sCase; }

int Port_ReproBackground_DetailMapsTick(unsigned frame) {
    static int phase, originalArea, originalRoom;
    static unsigned ready, since, deadline;
    if (phase == 13)
        return 0;
    if (deadline && frame > deadline) {
        fprintf(stderr, "[background-audit] detail-map case %d timed out (phase=%d)\n", sCase, phase);
        exit(2);
    }
    const int gameplay = gMain.task == TASK_GAME && gMain.state == GAMETASK_MAIN &&
                         gMain.substate == GAMEMAIN_UPDATE && !gFadeControl.active;
    const int pause = gMain.task == TASK_GAME && gMain.substate == GAMEMAIN_SUBTASK &&
                      gUI.lastState == SUBTASK_PAUSEMENU && gUI.nextToLoad == 2 &&
                      gPauseMenuOptions.unk11 == 2 && gMenu.menuType && !gFadeControl.active;
    if (phase == 0) {
        if (!gameplay)
            return 0;
        if (sCase < 0)
            sCase = 0;
        if (sCase == 66) {
            Port_ReproBackground_Event("detail_maps_complete", sCase);
            phase = 13;
            return 2;
        }
        deadline = frame + 12000;
        SetInventoryValue(ITEM_MAP, 1); /* B can return to the world-map tab. */
        gSave.stats.maxHealth = 24;
        gSave.stats.health = sCase / 22 == 0 ? 24 : sCase / 22 == 1 ? 15 : 8;
        gPauseMenuOptions.screen = 1;
        gPauseMenuOptions.unk16 = 0;
        Port_ReproBackground_Event("detail_open_request", sCase / 22);
        InitPauseMenu();
        ready = 0;
        phase = 1;
    }
    if (phase == 1 && pause && gPauseMenuOptions.screen == 1) {
        originalArea = gUI.roomControls.area;
        originalRoom = gUI.roomControls.room;
        phase = 2;
    }
    if (phase == 2 && pause) {
        const int recipe = sCase % 22;
        int area = recipe;
        gUI.roomControls.area = originalArea;
        gUI.roomControls.room = originalRoom;
        ClearGlobalFlag(TATEKAKE_HOUSE);
        gSave.kinstones.fusedKinstones[KINSTONE_E >> 3] &= ~(1 << (KINSTONE_E & 7));
        if (recipe == 17) {
            area = 9;
            SetGlobalFlag(TATEKAKE_HOUSE);
        } else if (recipe == 18) {
            area = 11;
            gSave.kinstones.fusedKinstones[KINSTONE_E >> 3] |= 1 << (KINSTONE_E & 7);
        } else if (recipe >= 19) {
            area = 14;
            gUI.roomControls.area = 8;
            gUI.roomControls.room = recipe - 19;
        }
        gSave.stats.health = sCase / 22 == 0 ? 24 : sCase / 22 == 1 ? 15 : 8;
        gPauseMenuOptions.unk2[4] = area;
        Port_ReproBackground_Event("detail_case_request", sCase);
        sub_080A4E84(6);
        sub_080A4E90(3);
        ready = 0;
        phase = 3;
        return 0;
    }
    if (phase == 3 && pause && gPauseMenuOptions.screen == 6) {
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("detail_ready", sCase);
        since = frame;
        phase = 4;
        return 1;
    }
    if (phase == 4 && frame >= since + 4) {
        gSave.stats.health = sCase / 22 == 2 ? 24 : 8;
        Port_ReproBackground_Event("detail_health_mutated", gSave.stats.health);
        since = frame;
        phase = 5;
    }
    if (phase == 5 && frame >= since + 12) {
        Port_ReproBackground_Event("detail_held", sCase);
        phase = 6;
        ready = 0;
        return 1;
    }
    if (phase == 6 || phase == 8 || phase == 10) {
        const int bottom = phase == 8;
        const int target = bottom ? gUnk_08128E94[gMenu.field_0x3].unk2 : 0;
        if (!ready) {
            *(vu16*)(gIoMem + 0x130) &= ~(bottom ? DPAD_DOWN : DPAD_UP);
            /* Hold an additional native tick at the bound to exercise clamps. */
            if (gMenu.field_0xa == target) {
                ready = frame + 1;
            }
            return 0;
        }
        if (frame == ready) {
            *(vu16*)(gIoMem + 0x130) &= ~(bottom ? DPAD_DOWN : DPAD_UP);
            return 0;
        }
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event(phase == 6 ? "detail_top" : bottom ? "detail_bottom" : "detail_top_returned", sCase);
        phase++;
        since = frame;
        return 1;
    }
    if ((phase == 7 || phase == 9) && frame >= since + 4) {
        phase++;
        ready = 0;
    }
    if (phase == 11 && frame >= since + 4) {
        gUI.roomControls.area = originalArea;
        gUI.roomControls.room = originalRoom;
        Port_ReproBackground_Event("detail_back_request", sCase);
        *(vu16*)(gIoMem + 0x130) &= ~B_BUTTON;
        ready = 0;
        phase = 12;
    }
    if (phase == 12 && pause && gPauseMenuOptions.screen == 4) {
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("detail_map_returned", sCase);
        /* Keep the case ID coherent through this capture boundary. */
        phase = 14;
        return 1;
    }
    if (phase == 14) {
        ++sCase;
        if (sCase % 22) {
            phase = 2;
        } else {
            *(vu16*)(gIoMem + 0x130) &= ~START_BUTTON;
            Port_ReproBackground_Event("detail_exit_request", sCase / 22 - 1);
            ready = 0;
            phase = 15;
        }
    }
    if (phase == 15 && gameplay) {
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("detail_game_restored", sCase / 22 - 1);
        phase = 0;
        return 1;
    }
    return 0;
}
