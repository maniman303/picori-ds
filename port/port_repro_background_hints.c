/* Repro-only standalone map hints. Native timers/reveal/input remain intact. */
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

static int sCase = -1;
int Port_ReproBackground_HintCase(void) { return sCase; }

int Port_ReproBackground_HintsTick(unsigned frame) {
    static int phase;
    static unsigned since, deadline;
    if (phase == 8)
        return 0;
    if (!deadline)
        deadline = frame + 14000;
    if (frame > deadline) {
        fprintf(stderr, "[background-audit] hint case %d timed out (phase=%d)\n", sCase, phase);
        exit(2);
    }
    const int gameplay = gMain.task == TASK_GAME && gMain.state == GAMETASK_MAIN &&
                         gMain.substate == GAMEMAIN_UPDATE && !gFadeControl.active;
    const int hint = gMain.task == TASK_GAME && gMain.substate == GAMEMAIN_SUBTASK &&
                     gUI.nextToLoad == 2 && gUI.lastState == SUBTASK_MAPHINT &&
                     gMenu.menuType == 1 && !gFadeControl.active;
    if (phase == 7 && gameplay) {
        Port_ReproBackground_Event("hint_restored", sCase);
        phase = 0;
    }
    if (phase == 0) {
        if (!gameplay)
            return 0;
        if (++sCase == 36) {
            Port_ReproBackground_Event("hints_complete", sCase);
            phase = 8;
            return 2;
        }
        const int health = sCase / 12;
        gSave.stats.maxHealth = 24;
        gSave.stats.health = health == 0 ? 24 : health == 1 ? 15 : 8;
        gSave.map_hints = (sCase / 6) % 2 ? 0xffff : 0;
        gSave.windcrests = 0x1ffff;
        MenuFadeIn(SUBTASK_MAPHINT, sCase % 6);
        Port_ReproBackground_Event("hint_request", sCase);
        phase = 1;
        return 0;
    }
    if (phase == 1 && hint && gMenu.transitionTimer <= 54 && gMenu.transitionTimer > 30) {
        Port_ReproBackground_Event("hint_before", sCase);
        phase = 2;
        return 1;
    }
    if (phase == 2 && hint && !gMenu.transitionTimer) {
        since = frame;
        phase = 3;
    }
    if (phase == 3 && frame >= since + 24) {
        Port_ReproBackground_Event("hint_ready", sCase);
        since = frame;
        phase = 4;
        return 1;
    }
    if (phase == 4 && frame >= since + 4) {
        gSave.stats.health = sCase / 12 == 2 ? 24 : 8;
        Port_ReproBackground_Event("hint_health_mutated", gSave.stats.health);
        since = frame;
        phase = 5;
    }
    if (phase == 5 && frame >= since + 12) {
        Port_ReproBackground_Event("hint_held", sCase);
        since = frame;
        phase = 6;
        return 1;
    }
    if (phase == 6 && frame >= since + 4) {
        static const u16 keys[] = { A_BUTTON, B_BUTTON, START_BUTTON };
        *(vu16*)(gIoMem + 0x130) &= ~keys[sCase % 3];
        phase = 7;
    }
    return 0;
}
