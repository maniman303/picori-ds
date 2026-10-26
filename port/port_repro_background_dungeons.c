/* Repro-only: enter real dungeon rooms before opening their native map. */
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "area.h"
#include "fade.h"
#include "game.h"
#include "main.h"
#include "menu.h"
#include "pauseMenu.h"
#include "save.h"
#include "subtask.h"
#include "port_gba_mem.h"
#include "port_repro.h"

extern void InitPauseMenu(void);
static int sCase = -1;
int Port_ReproBackground_DungeonCase(void) { return sCase; }

int Port_ReproBackground_DungeonsTick(unsigned frame) {
    static int phase, dungeon = 1, opening, floor;
    static unsigned since, ready, deadline;
    static const u8 areas[] = {0, AREA_DEEPWOOD_SHRINE, AREA_CAVE_OF_FLAMES, AREA_FORTRESS_OF_WINDS,
                              AREA_TEMPLE_OF_DROPLETS, AREA_PALACE_OF_WINDS, AREA_DARK_HYRULE_CASTLE};
    if (phase == 9)
        return 0;
    if (!deadline)
        deadline = frame + 28000;
    if (frame > deadline) {
        fprintf(stderr, "[background-audit] dungeon %d case %d timed out (phase=%d)\n", dungeon, sCase, phase);
        exit(2);
    }
    const int gameplay = gMain.task == TASK_GAME && gMain.state == GAMETASK_MAIN &&
                         gMain.substate == GAMEMAIN_UPDATE && !gFadeControl.active;
    if (phase == 8 && gameplay) {
        Port_ReproBackground_Event("dungeon_restored", opening);
        if (++opening == 6) {
            opening = 0;
            ++dungeon;
            phase = 0;
        } else {
            phase = 2;
        }
    }
    if (phase == 0) {
        if (!gameplay)
            return 0;
        if (dungeon == 7) {
            Port_ReproBackground_Event("dungeons_complete", sCase + 1);
            phase = 9;
            return 2;
        }
        const RoomHeader* room = &gAreaRoomHeaders[areas[dungeon]][0];
        const Transition entry = { .warp_type = WARP_TYPE_AREA, .area = areas[dungeon], .room = 0,
                                  .endX = room->pixel_width / 2, .endY = room->pixel_height / 2,
                                  .layer = 1, .transition_type = TRANSITION_TYPE_INSTANT };
        Port_ReproBackground_Event("dungeon_room_request", dungeon);
        DoExitTransition(&entry);
        phase = 1;
        return 0;
    }
    if (phase == 1) {
        if (!gameplay || gRoomControls.area != areas[dungeon] || gArea.dungeon_idx != dungeon)
            return 0;
        Port_ReproBackground_Event("dungeon_room_ready", dungeon);
        phase = 2;
    }
    if (phase == 2) {
        if (!gameplay)
            return 0;
        gSave.stats.maxHealth = 24;
        gSave.stats.health = opening / 2 == 0 ? 24 : opening / 2 == 1 ? 15 : 8;
        gSave.dungeonItems[dungeon] = opening % 2 ? 7 : 0;
        floor = 0;
        ++sCase;
        gPauseMenuOptions.screen = 1;
        InitPauseMenu();
        Port_ReproBackground_Event("dungeon_request", opening);
        phase = 3;
        return 0;
    }
    const int pause = gMain.task == TASK_GAME && gMain.substate == GAMEMAIN_SUBTASK &&
                      gUI.lastState == SUBTASK_PAUSEMENU && gUI.nextToLoad == 2 &&
                      gPauseMenuOptions.unk11 == 2 && gMenu.menuType && !gFadeControl.active;
    if (phase == 3 && pause && gPauseMenuOptions.screen == 1) {
        sub_080A4E84(5);
        sub_080A4E90(3);
        phase = 4;
        ready = 0;
        return 0;
    }
    if (phase == 4 && pause && gPauseMenuOptions.screen == 5) {
        if (gMenu.field_0x3 != floor) {
            ready = 0;
            if (frame % 6 == 0)
                *(vu16*)(gIoMem + 0x130) &= ~(gMenu.field_0x3 > floor ? DPAD_UP : DPAD_DOWN);
            return 0;
        }
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("dungeon_ready", sCase);
        since = frame;
        phase = 5;
        return 1;
    }
    if (phase == 5 && frame >= since + 4) {
        gSave.stats.health = opening / 2 == 2 ? 24 : 8;
        Port_ReproBackground_Event("dungeon_health_mutated", gSave.stats.health);
        since = frame;
        phase = 6;
    }
    if (phase == 6 && frame >= since + 12) {
        Port_ReproBackground_Event("dungeon_held", sCase);
        since = frame;
        phase = 7;
        return 1;
    }
    if (phase == 7 && frame >= since + 4) {
        if (++floor == gDungeonFloorMetadatas[dungeon].numFloors) {
            *(vu16*)(gIoMem + 0x130) &= ~START_BUTTON;
            phase = 8;
        } else {
            ++sCase;
            ready = 0;
            phase = 4;
        }
    }
    return 0;
}
