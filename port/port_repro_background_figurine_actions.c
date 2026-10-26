/* Repro-only native capsule/prize/duplicate and two-view description scrolling. */
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "fade.h"
#include "figurineMenu.h"
#include "flags.h"
#include "game.h"
#include "main.h"
#include "menu.h"
#include "save.h"
#include "subtask.h"
#include "port_gba_mem.h"
#include "port_repro.h"

static int sCase = -1;
int Port_ReproBackground_FigurineActionCase(void) { return sCase; }

int Port_ReproBackground_FigurineActionsTick(unsigned frame) {
    static int phase;
    static unsigned ready, since, deadline;
    if (phase == 11)
        return 0;
    if (phase == 10) {
        ++sCase;
        phase = 0;
    }
    if (deadline && frame > deadline) {
        fprintf(stderr, "[background-audit] figurine action %d timed out (phase=%d menu=%u scroll=%d/%u)\n",
                sCase, phase, gMenu.menuType, gFigurineMenu.unk1f, gFigurineMenu.unk1e);
        exit(2);
    }
    const int gameplay = gMain.task == TASK_GAME && gMain.state == GAMETASK_MAIN &&
                         gMain.substate == GAMEMAIN_UPDATE && !gFadeControl.active;
    if (phase == 0) {
        if (!gameplay)
            return 0;
        if (sCase < 0) {
            sCase = 0;
            const char* text = getenv("TMC_FIGURINE_ACTION_START");
            if (text) {
                char* end;
                const long first = strtol(text, &end, 10);
                if (!*text || *end || first < 0 || first >= 54) {
                    fprintf(stderr, "[background-audit] invalid figurine action start case\n");
                    exit(2);
                }
                sCase = first;
            }
            Port_ReproBackground_Event("figurine_actions_start", sCase);
        }
        if (sCase == 54) {
            Port_ReproBackground_Event("figurine_actions_complete", sCase);
            phase = 11;
            return 2;
        }
        const int health = sCase / 18;
        const int post = (sCase / 9) % 2;
        const int view = sCase % 9;
        const u8 figures[] = {47, 1, 68, post ? 136 : 130};
        const int target = view == 8 ? figures[3] : figures[view / 2];
        const struct_080FC3E4* table = REGION_IS_EU ? gUnk_080FC3E4_eu :
                                     REGION_IS_JP ? gUnk_080FC3E4_jp : gUnk_080FC3E4;
        if (view != 8 && table[target].type != view / 2) {
            fprintf(stderr, "[background-audit] figure %d has unexpected native capsule type\n", target);
            exit(2);
        }
        gSave.stats.maxHealth = 24;
        gSave.stats.health = health == 0 ? 24 : health == 1 ? 15 : 8;
        gSave.saw_staffroll = post;
        MemClear(gSave.figurines, sizeof(gSave.figurines));
        if (view == 8 || view % 2)
            WriteBit(gSave.figurines, target);
        Port_ReproBackground_Event("figurine_action_request", sCase);
        MenuFadeIn(SUBTASK_FIGURINEMENU, view == 8 ? 0xff : target);
        deadline = frame + 2000;
        ready = 0;
        phase = 1;
        return 0;
    }
    const int view = sCase % 9;
    const int post = (sCase / 9) % 2;
    const u8 figures[] = {47, 1, 68, post ? 136 : 130};
    const int target = view == 8 ? figures[3] : figures[view / 2];
    const int viewer = gMain.task == TASK_GAME && gMain.substate == GAMEMAIN_SUBTASK &&
                       gUI.nextToLoad == 2 && gUI.lastState == SUBTASK_FIGURINEMENU &&
                       !gFadeControl.active && gMenu.menuType == (view == 8 ? 1 : 3);
    if (phase == 1 && viewer) {
        if (gFigurineMenu.figure_idx != target) {
            ready = 0;
            if (view == 8 && frame % 6 == 0)
                *(vu16*)(gIoMem + 0x130) &= ~(target - gFigurineMenu.figure_idx >= 5 ? R_BUTTON : DPAD_DOWN);
            return 0;
        }
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        if ((view >= 6) && !gFigurineMenu.unk1e) {
            fprintf(stderr, "[background-audit] expected a scrollable description for figure %d\n", target);
            exit(2);
        }
        Port_ReproBackground_Event("figurine_action_ready", sCase);
        since = frame;
        phase = 2;
        return 1;
    }
    if (phase == 2 && frame >= since + 4) {
        gSave.stats.health = sCase / 18 == 2 ? 24 : 8;
        Port_ReproBackground_Event("figurine_action_health_mutated", gSave.stats.health);
        since = frame;
        phase = 3;
    }
    if (phase == 3 && frame >= since + 12) {
        Port_ReproBackground_Event("figurine_action_held", sCase);
        since = frame;
        phase = 4;
        return 1;
    }
    if (phase == 4 && frame >= since + 4 && frame % 6 == 0) {
        *(vu16*)(gIoMem + 0x130) &= ~DPAD_RIGHT;
        if (gFigurineMenu.unk1f == gFigurineMenu.unk1e) {
            since = frame;
            phase = 5; /* One extra real edge tests the upper clamp. */
        }
    }
    if (phase == 5 && frame >= since + 24) {
        Port_ReproBackground_Event("figurine_action_bottom", sCase);
        since = frame;
        phase = 6;
        return 1;
    }
    if (phase == 6 && frame >= since + 4 && frame % 6 == 0) {
        *(vu16*)(gIoMem + 0x130) &= ~DPAD_LEFT;
        if (gFigurineMenu.unk1f == 0) {
            since = frame;
            phase = 7; /* One extra real edge tests the lower clamp. */
        }
    }
    if (phase == 7 && frame >= since + 24) {
        Port_ReproBackground_Event("figurine_action_top", sCase);
        since = frame;
        phase = 8;
        return 1;
    }
    if (phase == 8 && frame >= since + 4) {
        *(vu16*)(gIoMem + 0x130) &= ~(sCase % 2 ? START_BUTTON : B_BUTTON);
        ready = 0;
        phase = 9;
    }
    if (phase == 9 && gameplay) {
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("figurine_action_restored", sCase);
        phase = 10;
        return 1;
    }
    return 0;
}
