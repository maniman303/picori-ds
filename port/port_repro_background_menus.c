/* Native pause-menu matrix for the background audit; never a runtime adapter.
 * Requests native transition phases/initializers with staged save/room flags.
 * Does not skip fades or patch the graphics selected by those initializers.
 */
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "fade.h"
#include "flags.h"
#include "game.h"
#include "kinstone.h"
#include "main.h"
#include "menu.h"
#include "pauseMenu.h"
#include "save.h"
#include "subtask.h"
#include "port_repro.h"

extern void InitPauseMenu(void);

/* Inventory precedes its save entry; quest precedes its save and sleep entry.
 * EU prompt rows retain the previous palette, so this order is significant.
 * Dungeon maps need a real dungeon fixture and are deliberately separate. */
static const u8 sScreens[] = { 1, 11, 1, 2, 10, 2, 9, 2, 4, 7, 8 };
enum { THRESHOLDS = 7, MAPS = 22, VIEWS = ARRAY_COUNT(sScreens) + MAPS,
       CASES = THRESHOLDS + 3 * VIEWS };

typedef struct {
    int health, maxHealth, decoration, screen, map, mapGroup;
} MenuCase;

static int sCase = -1;

int Port_ReproBackground_MenuCase(void) { return sCase; }

static MenuCase GetCase(int index) {
    static const u8 health[] = { 24, 16, 15, 9, 8, 1, 8 };
    static const u8 decoration[] = { 0, 0, 1, 1, 2, 2, 2 };
    MenuCase result = { 24, 24, 0, 1, -1, -1 };
    if (index < THRESHOLDS) {
        result.health = health[index];
        result.maxHealth = index == 6 ? 8 : 24;
        result.decoration = decoration[index];
    } else {
        const int group = (index - THRESHOLDS) / VIEWS;
        const int view = (index - THRESHOLDS) % VIEWS;
        result.decoration = group;
        result.health = group == 0 ? 24 : group == 1 ? 15 : 8;
        if (view < (int)ARRAY_COUNT(sScreens)) {
            result.screen = sScreens[view];
        } else {
            result.screen = 6;
            result.map = view - ARRAY_COUNT(sScreens);
            result.mapGroup = result.map < 17 ? 0x5f + result.map : 0x70 + result.map - 17;
        }
    }
    return result;
}

int Port_ReproBackground_MenusTick(unsigned int frame, int decoration, int screen, int mapGroup) {
    static int phase, requestedScreen, originalArea, originalRoom, savedRoom;
    static unsigned ready, since, deadline;
    if (phase == 5)
        return 0;
    if (!deadline)
        deadline = frame + 15000;
    if (frame > deadline) {
        fprintf(stderr, "[background-audit] menu case %d timed out (phase=%d screen=%d)\n", sCase, phase, screen);
        exit(2);
    }
    if (phase == 0) {
        if (gMain.task != TASK_GAME || gMain.state != GAMETASK_MAIN ||
            gMain.substate != GAMEMAIN_UPDATE || gFadeControl.active)
            return 0;
        if (sCase < 0)
            sCase = 0;
        if (sCase == CASES) {
            Port_ReproBackground_Event("menus_complete", CASES);
            phase = 5;
            return 2;
        }
        const MenuCase spec = GetCase(sCase);
        gSave.stats.health = spec.health;
        gSave.stats.maxHealth = spec.maxHealth;
        gPauseMenuOptions.screen = 1;
        gPauseMenuOptions.unk16 = 0;
        InitPauseMenu();
        savedRoom = 0;
        requestedScreen = 1;
        Port_ReproBackground_Event("menu_case_request", sCase);
        phase = 1;
        ready = 0;
    }
    if (phase == 1) {
        if (gMain.task != TASK_GAME || gMain.substate != GAMEMAIN_SUBTASK ||
            gUI.lastState != SUBTASK_PAUSEMENU || gUI.nextToLoad != 2 ||
            gPauseMenuOptions.unk11 != 2 || gPauseMenuOptions.screen != requestedScreen ||
            !gMenu.menuType || gFadeControl.active)
            return 0;
        if (!ready) {
            ready = frame;
        }
        if (!savedRoom) {
            originalArea = gUI.roomControls.area;
            originalRoom = gUI.roomControls.room;
            savedRoom = 1;
        }
        if (frame < ready + 24)
            return 0;
        const MenuCase spec = GetCase(sCase);
        if (decoration != spec.decoration || screen != spec.screen ||
            (spec.map >= 0 && mapGroup != spec.mapGroup)) {
            fprintf(stderr, "[background-audit] case %d expected decoration=%d screen=%d map=%d; got %d/%d/%d\n",
                    sCase, spec.decoration, spec.screen, spec.mapGroup, decoration, screen, mapGroup);
            exit(2);
        }
        Port_ReproBackground_Event("menu_case_ready", sCase);
        since = frame;
        phase = 2;
        return 1;
    }
    if (phase == 2 && frame >= since + 4) {
        /* A changed save value must not silently replace the installed field.
         * For the full-one-heart fixture, simulate a heart-container/heal too. */
        const MenuCase spec = GetCase(sCase);
        gSave.stats.maxHealth = 24;
        gSave.stats.health = spec.decoration == 2 ? 24 : 8;
        Port_ReproBackground_Event("menu_health_mutated", gSave.stats.health);
        since = frame;
        phase = 3;
    }
    if (phase == 3 && frame >= since + 12) {
        if (decoration != GetCase(sCase).decoration) {
            fprintf(stderr, "[background-audit] decoration changed after health mutation, case %d\n", sCase);
            exit(2);
        }
        Port_ReproBackground_Event("menu_case_held", sCase);
        since = frame;
        phase = 4;
        return 1;
    }
    if (phase == 4 && frame >= since + 4) {
        /* Restore the saved room before any native restore-to-game path. */
        gUI.roomControls.area = originalArea;
        gUI.roomControls.room = originalRoom;
        ++sCase;
        const int newOpening = sCase <= THRESHOLDS ||
            (sCase - THRESHOLDS) % VIEWS == 0 || sCase == CASES;
        if (newOpening) {
            Subtask_Exit();
            phase = 0;
            return 0;
        }
        const MenuCase spec = GetCase(sCase);
        ClearGlobalFlag(TATEKAKE_HOUSE);
        gSave.kinstones.fusedKinstones[KINSTONE_E >> 3] &= ~(1 << (KINSTONE_E & 7));
        if (spec.map >= 0) {
            int area = spec.map;
            if (spec.map == 17) {
                area = 9;
                SetGlobalFlag(TATEKAKE_HOUSE);
            } else if (spec.map == 18) {
                area = 11;
                gSave.kinstones.fusedKinstones[KINSTONE_E >> 3] |= 1 << (KINSTONE_E & 7);
            } else if (spec.map >= 19) {
                area = 14;
                gUI.roomControls.area = 8;
                gUI.roomControls.room = spec.map - 19;
            }
            gPauseMenuOptions.unk2[4] = area;
        }
        sub_080A4E84(spec.screen);
        /* Staged direct-screen requests still execute the native fade-out,
         * screen initializer and fade-in, including same-screen map changes. */
        sub_080A4E90(3);
        requestedScreen = spec.screen;
        ready = 0;
        phase = 1;
        Port_ReproBackground_Event("menu_case_request", sCase);
    }
    return 0;
}
