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
#include "npc.h"
#include "pauseMenu.h"
#include "save.h"
#include "subtask.h"
#include "port_repro.h"
#include "port_rom.h"

extern u32 sub_0805F76C(uintptr_t, WStruct*);

extern void InitPauseMenu(void);

/* Inventory precedes its save entry; quest precedes its save and sleep entry.
 * EU prompt rows retain the previous palette, so this order is significant.
 * Dungeon maps need a real dungeon fixture and are deliberately separate. */
static const u8 sScreens[] = { 1, 11, 1, 2, 10, 2, 9, 2, 4, 7, 8 };
enum { THRESHOLDS = 7, MAPS = 22, VIEWS = ARRAY_COUNT(sScreens) + MAPS,
       PAUSE_CASES = THRESHOLDS + 3 * VIEWS, FAMILY_CASES = PAUSE_CASES + 3,
       CASES = FAMILY_CASES + 3 * 7 };

typedef struct {
    int health, maxHealth, decoration, screen, map, mapGroup, fusionLayout;
} MenuCase;

static int sCase = -1;

int Port_ReproBackground_MenuCase(void) { return sCase; }

static MenuCase GetCase(int index) {
    static const u8 health[] = { 24, 16, 15, 9, 8, 1, 8 };
    static const u8 decoration[] = { 0, 0, 1, 1, 2, 2, 2 };
    MenuCase result = { 24, 24, 0, 1, -1, -1, -1 };
    if (index >= PAUSE_CASES) {
        result.decoration = index < FAMILY_CASES ? index - PAUSE_CASES : (index - FAMILY_CASES) / 7;
        if (index >= FAMILY_CASES)
            result.fusionLayout = (index - FAMILY_CASES) % 7;
        result.health = result.decoration == 0 ? 24 : result.decoration == 1 ? 15 : 8;
        result.screen = -1; /* Native fusion initializer. */
    } else if (index < THRESHOLDS) {
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

/* Discover real partner names using the native text renderer and active ROM.
 * The initializer will independently measure the selected name and load its BG
 * group. Never force a layout index or replace its graphics with a test image.
 * Measurement uses private scratch; gameplay VRAM and the text pool stay intact.
 */
static Entity* FusionPartner(int layout) {
    static Entity partners[7], representative;
    static int discovered;
    if (layout < 0) {
        representative.kind = NPC;
        representative.id = TINGLE_SIBLINGS;
        return &representative;
    }
    if (!discovered) {
        unsigned found = 0;
        Entity candidate = {0};
        candidate.kind = NPC;
        const u32 table = gRomOffsets ? gRomOffsets->fuserNpcData : 0;
        for (unsigned i = 1; i < 128 && table && gRomData && table <= gRomSize && (i + 1) * 6 <= gRomSize - table; ++i) {
            const u8* entry = gRomData + table + i * 6;
            if (!entry[0])
                break;
            candidate.id = entry[0];
            candidate.type = entry[1] == 0xff ? 0 : entry[1];
            candidate.type2 = entry[2] == 0xff ? 0 : entry[2];
            u32 pixels[0x300 / sizeof(u32)] = {0};
            WStruct text = {0};
            text.unk00 = 1;
            text.bgColor = 5;
            text.unk4 = 0x60;
            text.unk8 = pixels;
            const u32 textId = Port_GetEntityFuserData(NPC, candidate.id, candidate.type, candidate.type2) >> 32;
            const unsigned width = sub_0805F76C(textId, &text);
            const int variant = min(max((int)((width + 7) / 8) - 6, 0), 6);
            if (!(found & (1u << variant))) {
                partners[variant] = candidate;
                found |= 1u << variant;
                fprintf(stderr, "[background-audit] fusion layout %d: NPC %u/%u/%u name width %u\n",
                        variant, candidate.id, candidate.type, candidate.type2, width);
            }
        }
        if (found != 0x7f) {
            fprintf(stderr, "[background-audit] native partner names cover layout mask 0x%x, expected 0x7f\n", found);
            exit(2);
        }
        discovered = 1;
    }
    return &partners[layout];
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
        if (sCase < 0) {
            FusionPartner(0); /* Fail early if this language cannot cover every layout. */
            sCase = 0;
        }
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
        if (spec.screen < 0) {
            /* A stable fuser identity survives Subtask_Init's entity-pool clear.
             * Native code creates its own portrait NPC; no fusion is confirmed. */
            Entity* fuser = FusionPartner(spec.fusionLayout);
            InitializeFuseInfo(fuser, 0, 0, 0);
            gFuseInfo.kinstoneId = KINSTONE_A;
            MenuFadeIn(SUBTASK_KINSTONEMENU, 0);
        } else {
            InitPauseMenu();
        }
        savedRoom = 0;
        requestedScreen = spec.screen < 0 ? -1 : 1;
        Port_ReproBackground_Event("menu_case_request", sCase);
        phase = 1;
        ready = 0;
    }
    if (phase == 1) {
        const int fusion = requestedScreen < 0;
        if (gMain.task != TASK_GAME || gMain.substate != GAMEMAIN_SUBTASK || gUI.nextToLoad != 2 ||
            gUI.lastState != (fusion ? SUBTASK_KINSTONEMENU : SUBTASK_PAUSEMENU) ||
            (!fusion && (gPauseMenuOptions.unk11 != 2 || gPauseMenuOptions.screen != requestedScreen)) ||
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
            (sCase - THRESHOLDS) % VIEWS == 0 || sCase >= PAUSE_CASES;
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
