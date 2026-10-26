/* Native fast-travel selection/cancel audit. Repro only, no runtime adapter. */
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "fade.h"
#include "game.h"
#include "main.h"
#include "menu.h"
#include "save.h"
#include "subtask.h"
#include "port_gba_mem.h"
#include "port_repro.h"

enum { VIEWS = 9, CASES = 3 * VIEWS };
static int sCase = -1;

int Port_ReproBackground_TravelCase(void) { return sCase; }

int Port_ReproBackground_TravelTick(unsigned frame, int decoration) {
    static int phase;
    static unsigned ready, since, deadline;
    if (phase == 6)
        return 0;
    if (!deadline)
        deadline = frame + 8000;
    if (frame > deadline) {
        fprintf(stderr, "[background-audit] travel case %d timed out (phase=%d)\n", sCase, phase);
        exit(2);
    }
    const int gameplay = gMain.task == TASK_GAME && gMain.state == GAMETASK_MAIN &&
                         gMain.substate == GAMEMAIN_UPDATE && !gFadeControl.active;
    if (phase == 5 && gameplay) {
        Port_ReproBackground_Event("travel_case_restored", sCase);
        ++sCase;
        phase = 0;
    }
    if (phase == 0) {
        if (!gameplay)
            return 0;
        if (sCase < 0)
            sCase = 0;
        if (sCase == CASES) {
            Port_ReproBackground_Event("travel_complete", CASES);
            phase = 6;
            return 2;
        }
        const int health = sCase / VIEWS;
        gSave.stats.maxHealth = 24;
        gSave.stats.health = health == 0 ? 24 : health == 1 ? 15 : 8;
        /* Keep the lower visited-area bits deterministic. The ninth view has
         * only destination seven unlocked; native initialization must find it. */
        gSave.windcrests = (sCase % VIEWS == 8 ? 0x80000000u : 0xff000000u) | 0x1ffffu;
        MenuFadeIn(SUBTASK_FASTTRAVEL, 0);
        Port_ReproBackground_Event("travel_case_request", sCase);
        ready = 0;
        phase = 1;
    }
    if (phase == 1) {
        if (gMain.task != TASK_GAME || gMain.substate != GAMEMAIN_SUBTASK ||
            gUI.nextToLoad != 2 || gUI.lastState != SUBTASK_FASTTRAVEL ||
            gMenu.menuType != 1 || gFadeControl.active)
            return 0;
        const int view = sCase % VIEWS;
        const int destination = view == 8 ? 7 : view;
        if (gMenu.field_0x3 != destination) {
            ready = 0;
            /* Real key edges exercise native selection; never assign its cursor. */
            if (frame % 4 == 0)
                *(vu16*)(gIoMem + 0x130) &= ~DPAD_RIGHT;
            return 0;
        }
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        if (decoration != sCase / VIEWS) {
            fprintf(stderr, "[background-audit] travel decoration mismatch in case %d\n", sCase);
            exit(2);
        }
        Port_ReproBackground_Event("travel_case_ready", sCase);
        since = frame;
        phase = 2;
        return 1;
    }
    if (phase == 2 && frame >= since + 4) {
        gSave.stats.health = sCase / VIEWS == 2 ? 24 : 8;
        Port_ReproBackground_Event("travel_health_mutated", gSave.stats.health);
        since = frame;
        phase = 3;
    }
    if (phase == 3 && frame >= since + 12) {
        if (decoration != sCase / VIEWS) {
            fprintf(stderr, "[background-audit] travel decoration changed after health mutation\n");
            exit(2);
        }
        Port_ReproBackground_Event("travel_case_held", sCase);
        since = frame;
        phase = 4;
        return 1;
    }
    if (phase == 4 && frame >= since + 4) {
        *(vu16*)(gIoMem + 0x130) &= ~B_BUTTON;
        phase = 5;
    }
    return 0;
}
