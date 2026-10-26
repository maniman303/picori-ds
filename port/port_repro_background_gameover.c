/* Repro-only Game Over input driver. Native death, saves and transitions run normally. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "fade.h"
#include "game.h"
#include "main.h"
#include "menu.h"
#include "player.h"
#include "save.h"
#include "port_gba_mem.h"
#include "port_repro.h"

static int sCase = -1, sFault;
static unsigned sFailedWrites;
static SaveFile sBefore;
static int sBeforeStatus;

int Port_ReproBackground_GameOverCase(void) { return sCase; }
int Port_ReproBackground_GameOverWriteFails(void) {
    if (!sFault)
        return 0;
    if (!sFailedWrites++)
        Port_ReproBackground_Event("gameover_write_fault", 1);
    return 1;
}

int Port_ReproBackground_GameOverTick(unsigned frame) {
    static int phase, step;
    static unsigned since, deadline, ready;
    const char* route = getenv("TMC_BACKGROUND_AUDIT");
    const int extended = route && !strcmp(route, "gameover_returns");
    const int quitCase = extended ? 5 : 4;
    const int writeError = sCase == 3 || (extended && sCase == 4);
    const int gameplay = gMain.task == TASK_GAME && gMain.state == GAMETASK_MAIN &&
                         gMain.substate == GAMEMAIN_UPDATE && !gFadeControl.active &&
                         gPlayerEntity.base.action == PLAYER_NORMAL && gPlayerState.controlMode == CONTROL_ENABLED;
    const int prompt = gMain.task == TASK_GAMEOVER && gMain.state == 2 &&
                       gMenu.menuType == 1 && !gMenu.transitionTimer && !gFadeControl.active;
    const int continuation = gMain.task == TASK_GAMEOVER && gMain.state == 3 &&
                             gMenu.menuType == 1 && !gMenu.transitionTimer && !gFadeControl.active;
    if (phase == 10)
        return 0;
    if (deadline && frame > deadline) {
        fprintf(stderr, "[background-audit] gameover case %d timeout phase=%d task=%u state=%u menu=%u\n",
                sCase, phase, gMain.task, gMain.state, gMenu.menuType);
        exit(2);
    }
    if (!phase) {
        if (!gameplay)
            return 0;
        ++sCase;
        sFailedWrites = 0;
        sBeforeStatus = ReadSaveFile(gSaveHeader->saveFileId, &sBefore);
        gSave.stats.health = 0;
        gPlayerEntity.base.health = 0;
        Port_ReproBackground_Event("gameover_death_request", sCase);
        deadline = frame + 2200;
        phase = 1;
    }
    if (phase == 1 && prompt) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        Port_ReproBackground_Event("gameover_save_ready", sCase);
        step = 0;
        since = frame;
        phase = 2;
        return 1;
    }
    if (phase == 2 && prompt && frame >= since + 6) {
        static const u16 keys[6][3] = {
            {A_BUTTON, 0, 0}, {DPAD_DOWN, DPAD_UP, A_BUTTON},
            {DPAD_DOWN, A_BUTTON, 0}, {A_BUTTON, 0, 0}, {DPAD_DOWN, A_BUTTON, 0}, {A_BUTTON, 0, 0}
        };
        const int count = sCase == 1 ? 3 : (sCase == 2 || (!extended && sCase == 4)) ? 2 : 1;
        if (writeError) sFault = 1;
        *(vu16*)(gIoMem + 0x130) &= ~(extended && sCase == 4 ? (step++, A_BUTTON) : keys[sCase][step++]);
        since = frame;
        if (step == count) {
            ready = 0;
            phase = writeError ? 3 : 6;
        }
    }
    if (phase == 3 && gMain.task == TASK_GAMEOVER && gMain.state == 2 &&
        gMenu.menuType == 3 && !gMenu.transitionTimer && !gFadeControl.active) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        SaveFile after;
        int status = ReadSaveFile(gSaveHeader->saveFileId, &after);
        if (status != sBeforeStatus || (status == 1 && memcmp(&after, &sBefore, sizeof(after)))) {
            fprintf(stderr, "[background-audit] gameover failure changed prior save\n");
            exit(2);
        }
        Port_ReproBackground_Event("gameover_error_ready", sCase);
        Port_ReproBackground_Event("gameover_error_unchanged", status);
        sFault = 0;
        since = frame;
        phase = 4;
        return 1;
    }
    if (phase == 4 && frame >= since + 6) {
        Port_ReproBackground_Event("gameover_dismiss_request", sCase);
        *(vu16*)(gIoMem + 0x130) &= ~(extended ? (sCase == 3 ? B_BUTTON : START_BUTTON) : A_BUTTON);
        ready = 0;
        phase = 5;
    }
    if (phase == 5 && prompt) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        Port_ReproBackground_Event("gameover_retry_ready", sCase);
        if (extended && sCase == 3) {
            *(vu16*)(gIoMem + 0x130) &= ~DPAD_DOWN;
            since = frame; phase = 11; return 1;
        }
        *(vu16*)(gIoMem + 0x130) &= ~A_BUTTON;
        ready = 0;
        phase = 6;
        return 1;
    }
    if (phase == 11 && prompt && frame >= since + 6) {
        *(vu16*)(gIoMem + 0x130) &= ~A_BUTTON;
        ready = 0; phase = 6;
    }
    if (phase == 6 && continuation) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        SaveFile after;
        int status = ReadSaveFile(gSaveHeader->saveFileId, &after);
        const int saved = sCase == 0 || sCase == 1 || (extended ? sCase >= 4 : sCase == 3);
        if ((saved && (status != 1 || after.stats.health != 24)) ||
            (!saved && (status != sBeforeStatus || (status == 1 && memcmp(&after, &sBefore, sizeof(after)))))) {
            fprintf(stderr, "[background-audit] gameover save readback mismatch\n");
            exit(2);
        }
        Port_ReproBackground_Event("gameover_readback", saved ? 24 : 0);
        Port_ReproBackground_Event("gameover_continue_ready", sCase);
        since = frame;
        step = 0;
        phase = 7;
        return 1;
    }
    if (phase == 7 && continuation && frame >= since + 6) {
        const int navigate = sCase == 2 || sCase == quitCase;
        const int count = sCase == 2 ? 3 : sCase == quitCase ? 2 : 1;
        const u16 key = navigate && step == 0 ? DPAD_DOWN : sCase == 2 && step == 1 ? DPAD_UP : A_BUTTON;
        *(vu16*)(gIoMem + 0x130) &= ~key;
        ++step;
        since = frame;
        if (step == count) { ready = 0; phase = 8; }
    }
    if (phase == 8 && ((sCase == quitCase && gMain.task == TASK_TITLE && !gFadeControl.active) ||
                      (sCase != quitCase && gameplay))) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        Port_ReproBackground_Event(sCase == quitCase ? "gameover_title_returned" : "gameover_game_returned", sCase);
        if (sCase == quitCase) {
            Port_ReproBackground_Event("gameover_complete", quitCase + 1);
            phase = 10;
            return 3;
        }
        ready = 0;
        phase = 0;
        return 1;
    }
    return 0;
}
