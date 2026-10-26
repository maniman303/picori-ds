/* Repro-only collection viewer: real input, synthetic in-memory ownership. */
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "fade.h"
#include "flags.h"
#include "game.h"
#include "main.h"
#include "menu.h"
#include "save.h"
#include "subtask.h"
#include "port_gba_mem.h"
#include "port_repro.h"

static int sCase = -1;
int Port_ReproBackground_FigurineCase(void) { return sCase; }

int Port_ReproBackground_FigurinesTick(unsigned frame) {
    static int phase, opening, view;
    static unsigned ready, since, deadline;
    const int post = opening % 2;
    const int targets[] = {1, 2, 7, 130, 136};
    const int target = targets[view];
    const int gameplay = gMain.task == TASK_GAME && gMain.state == GAMETASK_MAIN &&
                         gMain.substate == GAMEMAIN_UPDATE && !gFadeControl.active;
    if (phase == 6)
        return 0;
    if (!deadline)
        deadline = frame + 16000;
    if (frame > deadline) {
        fprintf(stderr, "[background-audit] figurine case %d timed out (phase=%d)\n", sCase, phase);
        exit(2);
    }
    if (phase == 5 && gameplay) {
        Port_ReproBackground_Event("figurine_restored", opening);
        ++opening;
        phase = 0;
        view = 0;
    }
    if (phase == 0) {
        if (!gameplay)
            return 0;
        if (opening == 6) {
            Port_ReproBackground_Event("figurines_complete", sCase + 1);
            phase = 6;
            return 2;
        }
        gSave.stats.maxHealth = 24;
        gSave.stats.health = opening / 2 == 0 ? 24 : opening / 2 == 1 ? 15 : 8;
        gSave.saw_staffroll = opening % 2;
        MemClear(gSave.figurines, sizeof(gSave.figurines));
        /* Own the first and last entries; 2 and 7 exercise missing details. */
        WriteBit(gSave.figurines, 1);
        WriteBit(gSave.figurines, 130);
        WriteBit(gSave.figurines, 136);
        ++sCase;
        MenuFadeIn(SUBTASK_FIGURINEMENU, 0xff);
        Port_ReproBackground_Event("figurine_request", opening);
        ready = 0;
        phase = 1;
        return 0;
    }
    if (phase == 1) {
        if (gMain.task != TASK_GAME || gMain.substate != GAMEMAIN_SUBTASK ||
            gUI.nextToLoad != 2 || gUI.lastState != SUBTASK_FIGURINEMENU ||
            gMenu.menuType != 1 || gFadeControl.active)
            return 0;
        if (gFigurineMenu.figure_idx != target) {
            ready = 0;
            if (frame % 6 == 0)
                *(vu16*)(gIoMem + 0x130) &= ~(target - gFigurineMenu.figure_idx >= 5 ? R_BUTTON : DPAD_DOWN);
            return 0;
        }
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("figurine_ready", sCase);
        since = frame;
        phase = 2;
        return 1;
    }
    if (phase == 2 && frame >= since + 4) {
        gSave.stats.health = opening / 2 == 2 ? 24 : 8;
        Port_ReproBackground_Event("figurine_health_mutated", gSave.stats.health);
        since = frame;
        phase = 3;
    }
    if (phase == 3 && frame >= since + 12) {
        Port_ReproBackground_Event("figurine_held", sCase);
        since = frame;
        phase = 4;
        return 1;
    }
    if (phase == 4 && frame >= since + 4) {
        if (view == (post ? 4 : 3)) {
            *(vu16*)(gIoMem + 0x130) &= ~(post ? START_BUTTON : B_BUTTON);
            phase = 5;
        } else {
            ++view;
            ++sCase;
            ready = 0;
            phase = 1;
        }
    }
    return 0;
}
