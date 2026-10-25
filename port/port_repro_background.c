/* Native fixed-screen audit. Only compiled with repro_harness=y.
 * TMC_BACKGROUND_AUDIT=credits|prologue|intro|files|local_hint|menus;
 * TMC_BACKGROUND_AUDIT_OUT=<directory>.
 * The driver stages a synthetic save in memory, then runs native task/subtask
 * code at normal simulation speed. No timer, graphics or fade is skipped.
 * Events describe installation, never infer installed art from a text command.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "fade.h"
#include "fileselect.h"
#include "flags.h"
#include "game.h"
#include "main.h"
#include "menu.h"
#include "screen.h"
#include "subtask.h"
#include "port_gba_mem.h"
#include "port_repro.h"

extern int Port_CaptureBaseFramebufferPNG(const char* path);
extern void SetActiveSave(u32 idx);

static FILE* sTrace;
static const char* sDirectory;
static unsigned sFrame, sEvent, sLastEventFrame;
static int sInstalled = -1; /* observation only; not a runtime descriptor */
static int sFinished;
static int sDecoration = -1, sPauseScreen = -1, sMapGroup = -1;

void Port_ReproBackground_Event(const char* event, unsigned int value) {
    if (!sTrace)
        return;
    if (strcmp(event, "menu_decoration") == 0)
        sDecoration = value;
    if (strcmp(event, "pause_screen") == 0)
        sPauseScreen = value;
    if (strcmp(event, "map_detail") == 0)
        sMapGroup = value;
    if (strcmp(event, "credits_install") == 0 || strcmp(event, "prologue_install") == 0)
        sInstalled = (int)value;
    if (strcmp(event, "credits_clear") == 0 || strcmp(event, "credits_save") == 0 ||
        strcmp(event, "prologue_clear") == 0 || strcmp(event, "prologue_world") == 0)
        sInstalled = -1;
    fprintf(sTrace, "{\"event\":\"%s\",\"value\":%u,\"frame\":%u,\"installed\":%d,"
            "\"case\":%d,\"decoration\":%d,\"pause_screen\":%d,\"map_group\":%d,\"health\":%u,\"max_health\":%u}\n",
            event, value, sFrame, sInstalled, Port_ReproBackground_MenuCase(),
            sDecoration, sPauseScreen, sMapGroup, gSave.stats.health, gSave.stats.maxHealth);
    sEvent++;
    sLastEventFrame = sFrame;
    if (strcmp(event, "credits_save") == 0 || strcmp(event, "prologue_clear") == 0 ||
        strcmp(event, "prologue_world") == 0)
        sFinished = 1;
}

void Port_ReproBackground_Tick(unsigned int frame) {
    static int mode = -1, stage;
    static unsigned readyFrame, finishFrame, lastCapturedEvent, fileStart, hintReady;
    if (mode == -1) {
        const char* value = getenv("TMC_BACKGROUND_AUDIT");
        mode = 0;
        if (!value || !*value)
            return;
        mode = strcmp(value, "credits") == 0 ? 1 : strcmp(value, "prologue") == 0 ? 2 :
               strcmp(value, "files") == 0 ? 3 : strcmp(value, "intro") == 0 ? 4 :
               strcmp(value, "local_hint") == 0 ? 5 : strcmp(value, "menus") == 0 ? 6 : 0;
        sDirectory = getenv("TMC_BACKGROUND_AUDIT_OUT");
        char path[4096];
        if (!mode || !sDirectory || snprintf(path, sizeof(path), "%s/trace.jsonl", sDirectory) >= (int)sizeof(path)) {
            fprintf(stderr, "[background-audit] invalid scenario/output directory\n");
            exit(2);
        }
        sTrace = fopen(path, "w");
        if (!sTrace) {
            perror("[background-audit] trace");
            exit(2);
        }
    }
    if (!mode)
        return;
    sFrame = frame;
    const int menuResult = mode == 6 ? Port_ReproBackground_MenusTick(frame, sDecoration, sPauseScreen, sMapGroup) : 0;
    if (menuResult & 2)
        sFinished = 1;
    if (frame > 30000) {
        fprintf(stderr, "[background-audit] timed out\n");
        exit(2);
    }

    /* Trace every simulation boundary. PNGs sample the completed framebuffer;
     * mutable engine state can lead that image by one tick at an installation.
     * Also capture two ticks after events, after the VBlank upload/present. */
    const int capture = (menuResult & 1) || frame % 60 == 0 || (gFadeControl.active && frame % 8 == 0) ||
        (sEvent != lastCapturedEvent && frame >= sLastEventFrame + 2);
    fprintf(sTrace, "{\"frame\":%u,\"task\":%u,\"state\":%u,\"substate\":%u,"
            "\"ui\":%u,\"menu\":%u,\"overlay\":%u,\"installed\":%d,"
            "\"fade_active\":%u,\"fade_type\":%u,\"fade_progress\":%u,\"fade_mask\":%u,"
            "\"bldcnt\":%u,\"alpha\":%u,\"case\":%d,\"decoration\":%d,\"pause_screen\":%d,"
            "\"map_group\":%d,\"health\":%u,\"max_health\":%u,\"capture\":%s}\n",
            frame, gMain.task, gMain.state, gMain.substate, gUI.lastState, gMenu.menuType,
            gMenu.overlayType, sInstalled, gFadeControl.active, gFadeControl.type,
            gFadeControl.progress, gFadeControl.mask, gScreen.controls.layerFXControl,
            gScreen.controls.alphaBlend, Port_ReproBackground_MenuCase(), sDecoration, sPauseScreen,
            sMapGroup, gSave.stats.health, gSave.stats.maxHealth, capture ? "true" : "false");
    if (capture) {
        char path[4096];
        if (snprintf(path, sizeof(path), "%s/%06u.png", sDirectory, frame) >= (int)sizeof(path) ||
            !Port_CaptureBaseFramebufferPNG(path)) {
            fprintf(stderr, "[background-audit] capture failed at %u\n", frame);
            exit(2);
        }
        lastCapturedEvent = sEvent;
    }
    if (sFinished && !finishFrame)
        finishFrame = frame + 120;
    if (finishFrame && frame >= finishFrame) {
        const int traceError = ferror(sTrace);
        if (fclose(sTrace) != 0 || traceError)
            exit(2);
        fprintf(stderr, "[background-audit] complete at %u\n", frame);
        exit(0);
    }
    if (gMain.task == TASK_TITLE && frame % 30 == 0)
        *(vu16*)(gIoMem + 0x130) &= ~START_BUTTON;

    /* Wait for the real file screen to finish initializing before staging. */
    if (!stage && gMain.task == TASK_FILE_SELECT && !gFadeControl.active) {
        if (!readyFrame)
            readyFrame = frame;
        if (frame < readyFrame + 120)
            return;
        ResetSaveFile(0);
        SaveFile* save = &gFileSelectState.saves[0];
        save->initialized = 1;
        save->name[0] = 'A';
        save->global_progress = 1;
        save->saved_status.area_next = 3;
        save->saved_status.room_next = 8;
        save->saved_status.start_pos_x = 0x1d8;
        save->saved_status.start_pos_y = 0x138;
        save->saved_status.layer = 1;
        gFileSelectState.saveStatus[0] = 1;
        SetActiveSave(0);
        SetGlobalFlag(START);
        if (mode == 3) {
            gFileSelectState.unk6 = 0;
            fileStart = frame;
        } else {
            SetTask(mode == 1 ? TASK_STAFFROLL : TASK_GAME);
        }
        stage = 1;
    }
    if (mode == 3 && stage == 1) {
        /* Real input paths: options/back, view/copy/back, view/erase/back,
         * empty slot/name entry/cancel, then EU language selection. Copy and
         * erase are cancelled; the synthetic slot exists only in memory. */
        static const u16 keys[] = {
            R_BUTTON, B_BUTTON, A_BUTTON, DPAD_RIGHT, A_BUTTON, B_BUTTON,
            A_BUTTON, DPAD_RIGHT, DPAD_RIGHT, A_BUTTON, B_BUTTON,
            DPAD_DOWN, A_BUTTON, B_BUTTON, START_BUTTON,
            DPAD_DOWN, DPAD_DOWN, A_BUTTON,
        };
        const unsigned age = frame - fileStart;
        if (age > 0 && age % 90 == 0) {
            const unsigned index = age / 90 - 1;
            if (index < ARRAY_COUNT(keys))
                *(vu16*)(gIoMem + 0x130) &= ~keys[index];
            else {
                Port_ReproBackground_Event("files_complete", 0);
                sFinished = 1;
                stage = 2;
            }
        }
    }
    if ((mode == 2 || mode == 4 || mode == 5) && stage == 1 &&
        gMain.task == TASK_GAME && gMain.state == GAMETASK_MAIN &&
        gMain.substate == GAMEMAIN_UPDATE && !gFadeControl.active) {
        /* Sanctuary retelling has six pages; the opening has five. */
        MenuFadeIn(mode == 5 ? SUBTASK_LOCALMAPHINT : SUBTASK_AUXCUTSCENE,
                   mode == 5 ? 0 : mode == 2 ? 3 : 1);
        stage = 2;
    }
    if (mode == 5 && stage == 2 && gMain.substate == GAMEMAIN_SUBTASK &&
        gUI.lastState == SUBTASK_LOCALMAPHINT && gMenu.menuType == 1 && !gFadeControl.active) {
        if (!hintReady)
            hintReady = frame;
        if (frame >= hintReady + 120) {
            Port_ReproBackground_Event("local_hint_ready", 0);
            sFinished = 1;
            stage = 3;
        }
    }
}
