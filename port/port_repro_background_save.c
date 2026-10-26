/* Repro-only save confirmation, native persistence, refusal and EEPROM write failure. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
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
static unsigned sFailedWrites;
static SaveFile sBefore;
static int sBeforeStatus;
int Port_ReproBackground_SaveWriteFails(void) {
    if (sCase < 0 || sCase >= 18 || sCase % 6 != 5)
        return 0;
    if (!sFailedWrites++)
        Port_ReproBackground_Event("save_write_fault", 1);
    return 1;
}
int Port_ReproBackground_SaveCase(void) { return sCase; }

int Port_ReproBackground_SaveTick(unsigned frame) {
    static int phase, navigation, answer;
    static unsigned ready, since, deadline;
    const int gameplay = gMain.task == TASK_GAME && gMain.state == GAMETASK_MAIN &&
                         gMain.substate == GAMEMAIN_UPDATE && !gFadeControl.active;
    const int pause = gMain.task == TASK_GAME && gMain.substate == GAMEMAIN_SUBTASK &&
                      gUI.lastState == SUBTASK_PAUSEMENU && gUI.nextToLoad == 2 &&
                      gPauseMenuOptions.unk11 == 2 && gMenu.menuType == 1 && !gFadeControl.active;
    if (phase == 14)
        return 0;
    if (phase == 13) {
        ++sCase;
        phase = 0;
    }
    if (deadline && frame > deadline) {
        fprintf(stderr, "[background-audit] save case %d timed out (phase=%d screen=%u status=%u)\n",
                sCase, phase, gPauseMenuOptions.screen, gMain.sleepStatus);
        exit(2);
    }
    if (phase == 0) {
        if (!gameplay)
            return 0;
        if (sCase < 0)
            sCase = 0;
        if (sCase == 18) {
            Port_ReproBackground_Event("save_complete", sCase);
            phase = 14;
            return 2;
        }
        sFailedWrites = 0;
        sBeforeStatus = ReadSaveFile(gSaveHeader->saveFileId, &sBefore);
        gSave.stats.maxHealth = 24;
        gSave.stats.health = sCase / 6 == 0 ? 24 : sCase / 6 == 1 ? 15 : 8;
        gPauseMenuOptions.screen = 1;
        Port_ReproBackground_Event("save_case_request", sCase);
        InitPauseMenu();
        deadline = frame + 1600;
        ready = 0;
        phase = 1;
        return 0;
    }
    if (phase == 1 && pause && gPauseMenuOptions.screen == 1) {
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        *(vu16*)(gIoMem + 0x130) &= ~R_BUTTON;
        ready = 0;
        navigation = 0;
        since = frame;
        phase = 2;
    }
    if (phase == 2 && pause && gPauseMenuOptions.screen == 2) {
        /* Native pause remembers the preceding quest cursor. Return to zero
         * with real navigation before repeating the path to the save button. */
        if (gMenu.field_0x3 != 0) {
            ready = 0;
            if (frame < since + 6)
                return 0;
            u16 key;
            switch (gMenu.field_0x3) {
                case 5: case 4: case 1: key = DPAD_LEFT; break;
                case 8: case 3: key = DPAD_UP; break;
                default:
                    fprintf(stderr, "[background-audit] unexpected remembered quest cursor %u\n", gMenu.field_0x3);
                    exit(2);
            }
            *(vu16*)(gIoMem + 0x130) &= ~key;
            since = frame;
            return 0;
        }
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("save_quest_ready", sCase);
        since = frame;
        phase = 3;
        return 1;
    }
    if (phase == 3 && pause && frame >= since + 6) {
        static const u16 keys[] = {DPAD_LEFT, DPAD_UP, A_BUTTON};
        static const u8 cursors[] = {0, 11, 5};
        if (navigation >= 3 || gMenu.field_0x3 != cursors[navigation]) {
            fprintf(stderr, "[background-audit] unexpected native quest cursor %u\n", gMenu.field_0x3);
            exit(2);
        }
        if (navigation == 2)
            Port_ReproBackground_Event("save_prompt_request", sCase);
        *(vu16*)(gIoMem + 0x130) &= ~keys[navigation++];
        since = frame;
        if (navigation == 3) {
            ready = 0;
            phase = 4;
        }
    }
    if (phase == 4 && pause && gPauseMenuOptions.screen == 10) {
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("save_ready", sCase);
        since = frame;
        phase = 5;
        return 1;
    }
    if (phase == 5 && frame >= since + 4) {
        gSave.stats.health = sCase / 6 == 2 ? 24 : 8;
        Port_ReproBackground_Event("save_health_mutated", gSave.stats.health);
        since = frame;
        phase = 6;
    }
    if (phase == 6 && frame >= since + 12) {
        Port_ReproBackground_Event("save_held", sCase);
        answer = 0;
        since = frame;
        phase = 7;
        return 1;
    }
    if (phase == 7 && frame >= since + 6) {
        const int mode = sCase % 6;
        static const u16 keys[6][3] = {
            {A_BUTTON, 0, 0}, {DPAD_DOWN, DPAD_UP, A_BUTTON},
            {DPAD_DOWN, A_BUTTON, 0}, {B_BUTTON, B_BUTTON, 0}, {DPAD_DOWN, B_BUTTON, 0},
            {A_BUTTON, 0, 0}
        };
        const int count = mode == 0 || mode == 5 ? 1 : mode == 1 ? 3 : 2;
        *(vu16*)(gIoMem + 0x130) &= ~keys[mode][answer++];
        since = frame;
        if (answer == count) {
            ready = 0;
            phase = mode == 5 ? 8 : 11;
        }
    }
    if (phase == 8 && gPauseMenuOptions.screen == 10 && gMenu.menuType == 3 &&
        gMenu.transitionTimer == 0 && !gFadeControl.active) {
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("save_result_ready", sCase);
        SaveFile after;
        const int status = ReadSaveFile(gSaveHeader->saveFileId, &after);
        const int failed = sCase % 6 == 5;
        if ((!failed && (status != 1 || after.stats.health != gSave.stats.health)) ||
            (failed && (status != sBeforeStatus || (status == 1 && memcmp(&after, &sBefore, sizeof(after)))))) {
            fprintf(stderr, "[background-audit] save readback mismatch case %d\n", sCase);
            exit(2);
        }
        Port_ReproBackground_Event("save_readback", failed ? 0 : after.stats.health);
        since = frame;
        phase = 9;
        return 1;
    }
    if (phase == 9 && frame >= since + 4) {
        Port_ReproBackground_Event("save_dismiss_request", sCase);
        *(vu16*)(gIoMem + 0x130) &= ~A_BUTTON;
        ready = 0;
        phase = 11;
    }
    if (phase == 11 && pause && gPauseMenuOptions.screen == 2 && gMain.sleepStatus == DEFAULT) {
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        if (sCase % 6 < 2) {
            SaveFile after;
            if (ReadSaveFile(gSaveHeader->saveFileId, &after) != 1 || after.stats.health != gSave.stats.health) {
                fprintf(stderr, "[background-audit] successful save readback mismatch\n");
                exit(2);
            }
            Port_ReproBackground_Event("save_result_ready", sCase);
            Port_ReproBackground_Event("save_readback", after.stats.health);
        }
        if (sCase % 6 >= 2 && sCase % 6 <= 4) {
            SaveFile after;
            const int status = ReadSaveFile(gSaveHeader->saveFileId, &after);
            if (status != sBeforeStatus || (status == 1 && memcmp(&after, &sBefore, sizeof(after)))) {
                fprintf(stderr, "[background-audit] declined save changed record\n");
                exit(2);
            }
            Port_ReproBackground_Event("save_decline_unchanged", sCase);
        }
        Port_ReproBackground_Event("save_quest_returned", sCase);
        since = frame;
        phase = 12;
        return 1;
    }
    if (phase == 12 && frame >= since + 4) {
        Port_ReproBackground_Event("save_exit_request", sCase);
        *(vu16*)(gIoMem + 0x130) &= ~START_BUTTON;
        ready = 0;
        phase = 15;
    }
    if (phase == 15 && gameplay) {
        if (!ready)
            ready = frame;
        if (frame < ready + 24)
            return 0;
        Port_ReproBackground_Event("save_game_restored", sCase);
        phase = 13;
        return 1;
    }
    return 0;
}
