/* Repro-only file commit and post-credits Save interaction fixtures. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common.h"
#include "fade.h"
#include "fileselect.h"
#include "flags.h"
#include "game.h"
#include "main.h"
#include "menu.h"
#include "save.h"
#include "port_gba_mem.h"
#include "port_repro.h"

extern const char* Port_Save_GetActivePath(void);
extern void SetActiveSave(u32);
extern void sub_0805070C(void);
extern u32 WriteSaveFile(u32, SaveFile*);
extern void SetFileStatusDeleted(u32);
extern void StaffrollTask_State0(void);
static int sEndingCase = -1, sFileCase = -1, sFault;
static unsigned sFailedWrites;
static u8 sHostDiskBefore[8192];
static void ReadHostDisk(u8* data) {
    FILE* file = fopen(Port_Save_GetActivePath(), "rb");
    if (!file || fread(data, 1, 8192, file) != 8192 || fclose(file)) {
        fprintf(stderr, "[background-audit] cannot inspect durable EEPROM\n"); exit(2);
    }
}
int Port_ReproBackground_EndingCase(void) { return sEndingCase; }
int Port_ReproBackground_FileWriteCase(void) { return sFileCase; }
int Port_ReproBackground_WritesFail(void) {
    if (!sFault) return 0;
    if (!sFailedWrites++) Port_ReproBackground_Event("writes_write_fault", 1);
    return 1;
}
static void Key(unsigned frame, unsigned* since, u16 key, const char* event) {
    if (frame < *since + 8) return;
    Port_ReproBackground_Event(event, key);
    *(vu16*)(gIoMem + 0x130) &= ~key;
    *since = frame;
}
static int Unchanged(u32 slot, const SaveFile* before, int beforeStatus) {
    SaveFile after;
    const int status = ReadSaveFile(slot, &after);
    return status == beforeStatus && (status != 1 || !memcmp(&after, before, sizeof(after)));
}

int Port_ReproBackground_EndingTick(unsigned frame) {
    static int phase, answer;
    static unsigned since, ready, deadline;
    static SaveFile before;
    static int beforeStatus;
    const int prompt = gMain.task == TASK_STAFFROLL && gMain.state == 2 &&
                       gMenu.overlayType == 1 && !gMenu.transitionTimer && !gFadeControl.active;
    if (phase == 9) return 0;
    if (deadline && frame > deadline) { fprintf(stderr, "[background-audit] ending timeout case=%d phase=%d\n", sEndingCase, phase); exit(2); }
    if (!phase) {
        if (sEndingCase < 0 ? !(gMain.task == TASK_GAME && gMain.state == 2 && gMain.substate == 2 && !gFadeControl.active) :
                             !(gMain.task == TASK_TITLE && !gFadeControl.active)) return 0;
        ++sEndingCase;
        beforeStatus = ReadSaveFile(0, &before);
        gSaveHeader->saveFileId = 0;
        gSave.stats.maxHealth = 24;
        gSave.stats.health = sEndingCase == 0 ? 24 : sEndingCase == 2 ? 8 : 15;
        /* Fixture boundary: same State2 initializer reached by retained full
         * credits evidence. State0 supplies real clear flags, then we stage
         * the post-credits entry; no prompt choice/result/timer is assigned. */
        SetTask(TASK_STAFFROLL);
        StaffrollTask_State0();
        gMain.state = 2;
        gMenu.overlayType = 0;
        Port_ReproBackground_Event("ending_request", sEndingCase);
        deadline = frame + 1600;
        ready = 0;
        phase = 1;
    }
    if (phase == 1 && prompt) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        Port_ReproBackground_Event("ending_ready", sEndingCase);
        since = frame;
        answer = 0;
        phase = 2;
        return 1;
    }
    if (phase == 2 && prompt && frame >= since + 8) {
        const u16 key = sEndingCase == 1 && !answer ? DPAD_DOWN : A_BUTTON;
        if (sEndingCase >= 2) { sFault = 1; sFailedWrites = 0; }
        Key(frame, &since, key, "ending_input");
        if (++answer == (sEndingCase == 1 ? 2 : 1)) { ready = 0; phase = sEndingCase >= 2 ? 3 : 7; }
    }
    if (phase == 3 && gMain.task == TASK_STAFFROLL && gMain.state == 2 && gMenu.overlayType == 3 &&
        !gMenu.transitionTimer && !gFadeControl.active) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        if (!Unchanged(0, &before, beforeStatus)) { fprintf(stderr, "[background-audit] ending failed write changed record\n"); exit(2); }
        Port_ReproBackground_Event("ending_error_ready", sEndingCase);
        Port_ReproBackground_Event("ending_error_unchanged", beforeStatus);
        sFault = 0;
        since = frame;
        phase = 4;
        return 1;
    }
    if (phase == 4 && frame >= since + 8) {
        Key(frame, &since, sEndingCase == 2 ? B_BUTTON : START_BUTTON, "ending_dismiss_input");
        ready = 0;
        phase = 5;
    }
    if (phase == 5 && prompt) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        Port_ReproBackground_Event("ending_retry_ready", sEndingCase);
        since = frame;
        answer = 0;
        phase = 6;
        return 1;
    }
    if (phase == 6 && prompt && frame >= since + 8) {
        Key(frame, &since, sEndingCase == 3 && !answer ? DPAD_DOWN : A_BUTTON, "ending_retry_input");
        if (++answer == (sEndingCase == 3 ? 2 : 1)) { ready = 0; phase = 7; }
    }
    if (phase == 7 && gMain.task == TASK_TITLE && !gFadeControl.active) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        const int saved = sEndingCase == 0 || sEndingCase == 2;
        SaveFile after;
        const int status = ReadSaveFile(0, &after);
        if (saved ? status != 1 || after.stats.health != (sEndingCase == 0 ? 24 : 8) || !after.saw_staffroll :
                    !Unchanged(0, &before, beforeStatus)) { fprintf(stderr, "[background-audit] ending readback mismatch\n"); exit(2); }
        Port_ReproBackground_Event("ending_readback", saved ? after.stats.health : 0);
        Port_ReproBackground_Event("ending_title_returned", sEndingCase);
        if (sEndingCase == 3) { Port_ReproBackground_Event("ending_complete", 4); phase = 9; return 3; }
        phase = 8;
        return 1;
    }
    if (phase == 8) { phase = 0; ready = 0; }
    return 0;
}

int Port_ReproBackground_FileWritesTick(unsigned frame) {
    static int phase, step, beforeStatus[3];
    static SaveFile before[3];
    static unsigned ready, since, deadline;
    const int file = gMain.task == TASK_FILE_SELECT && gMain.state == 1 && !gFadeControl.active && !gFileSelectState.isTransitioning;
    const int root = file && gUI.lastState == 0 && gUI.state == 0 && gChooseFileState.state == 2;
    const int create = sFileCase == 0 || sFileCase == 4;
    const int copy = sFileCase == 1 || sFileCase == 5;
    const int error = sFileCase >= 4 && sFileCase <= 7;
    const char* route = getenv("TMC_BACKGROUND_AUDIT");
    const int extended = route && !strcmp(route, "file_errors");
    const int count = extended ? 9 : 7;
    if (phase == 12) return 0;
    if (deadline && frame > deadline) { fprintf(stderr, "[background-audit] files timeout case=%d phase=%d ui=%u menu=%u/%u col=%u row=%u dest=%u\n", sFileCase, phase, gUI.lastState, gMenu.menuType, gMenu.overlayType, gMenu.column_idx, gFileSelectState.unk6, gFileSelectState.unk7); exit(2); }
    if (!phase) {
        if (!root) return 0;
        if (++sFileCase == count) { Port_ReproBackground_Event("file_writes_complete", count); phase = 12; return 2; }
        sFault = 0; sFailedWrites = 0;
        /* Independent durable fixtures, isolated by the capture runner. */
        for (int i = 0; i < 3; ++i) { SetFileStatusDeleted(i); ResetSaveFile(i); }
        SaveFile* source = &gFileSelectState.saves[0];
        source->initialized = 1; source->name[0] = 'S'; source->stats.rupees = 123;
        if (WriteSaveFile(0, source) != 1) { fprintf(stderr, "[background-audit] file fixture write failed\n"); exit(2); }
        gFileSelectState.saveStatus[0] = 1;
        for (int i = 0; i < 3; ++i) { beforeStatus[i] = ReadSaveFile(i, &before[i]); sub_08050AFC(i); }
        sub_0805070C(); /* Redraw staged names through the native text renderer. */
        Port_ReproBackground_Event("file_write_request", sFileCase);
        deadline = frame + 1800;
        ready = 0; since = frame; step = 0; phase = 1;
        return 0;
    }
    if (phase == 1 && root && frame >= since + 8) {
        const int row = create ? 1 : 0;
        if (gFileSelectState.unk6 != row) { Key(frame, &since, gFileSelectState.unk6 > row ? DPAD_UP : DPAD_DOWN, "file_write_input"); return 0; }
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        Port_ReproBackground_Event("file_write_root_ready", sFileCase);
        Key(frame, &since, A_BUTTON, "file_write_input");
        ready = 0; phase = 2;
        return 1;
    }
    if (phase == 2 && file && gUI.lastState == (create ? 1 : 4) && gMenu.menuType == (create ? 1 : 0)) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        Port_ReproBackground_Event("file_write_entry_ready", sFileCase);
        since = frame; phase = 3;
        return 1;
    }
    if (phase == 3 && file && frame >= since + 8) {
        if (create) {
            Key(frame, &since, step == 0 ? A_BUTTON : START_BUTTON, "file_write_input");
            if (++step == 3) { sFault = error; phase = 6; ready = 0; }
        } else {
            const int col = copy ? 1 : 2;
            if (gMenu.column_idx != col) { Key(frame, &since, DPAD_RIGHT, "file_write_input"); return 0; }
            Key(frame, &since, A_BUTTON, "file_write_input");
            ready = 0; phase = 4;
        }
    }
    if (phase == 4 && file && gUI.lastState == (copy ? 5 : 6) && gMenu.menuType == 1) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        Port_ReproBackground_Event("file_write_operation_ready", sFileCase);
        since = frame; phase = 5;
        return 1;
    }
    if (phase == 5 && file && frame >= since + 8) {
        if (copy && gFileSelectState.unk7 != 1) { Key(frame, &since, DPAD_UP, "file_write_input"); return 0; }
        if (!copy && sFileCase != 2 && gMenu.column_idx != 1) { Key(frame, &since, DPAD_RIGHT, "file_write_input"); return 0; }
        sFault = error && sFileCase != 7;
        if (sFileCase == 7) {
            char path[512];
            snprintf(path, sizeof(path), "%s.tmp", Port_Save_GetActivePath());
            ReadHostDisk(sHostDiskBefore);
            if (mkdir(path, 0700)) { perror("[background-audit] block atomic write"); exit(2); }
            Port_ReproBackground_Event("file_host_fault", 1);
        }
        Key(frame, &since, A_BUTTON, "file_write_input");
        ready = 0; phase = 6;
    }
    if (phase == 6 && file && error &&
        gUI.lastState == (create ? 1 : copy ? 5 : 6) && (create ? gMenu.menuType == 2 && gMenu.overlayType == 2 : gMenu.menuType == 3) && !gMenu.transitionTimer) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        if (sFileCase == 7) {
            char path[512];
            snprintf(path, sizeof(path), "%s.tmp", Port_Save_GetActivePath());
            u8 after[8192]; ReadHostDisk(after);
            if (memcmp(after, sHostDiskBefore, sizeof(after))) { fprintf(stderr, "[background-audit] host failure changed durable data\n"); exit(2); }
            Port_ReproBackground_Event("file_host_disk_unchanged", 1);
            if (rmdir(path)) { perror("[background-audit] unblock atomic write"); exit(2); }
            Port_ReproBackground_Event("file_host_recovered", 1);
        }
        Port_ReproBackground_Event("file_write_error_ready", sFileCase);
        since = frame; phase = 7;
        return 1;
    }
    if (phase == 7 && frame >= since + 8) { Key(frame, &since, A_BUTTON, "file_write_dismiss_input"); ready = 0; phase = 8; }
    if ((phase == 6 || phase == 8) && root) {
        if (!ready) ready = frame;
        if (frame < ready + 24) return 0;
        sFault = 0;
        SaveFile after;
        const int target = create || copy ? 1 : 0;
        const int status = ReadSaveFile(target, &after);
        int valid;
        if (error || sFileCase == 2) valid = Unchanged(target, &before[target], beforeStatus[target]);
        else if (create) valid = status == 1 && after.name[0] != 0 && after.stats.health == 24;
        else if (copy) valid = status == 1 && !memcmp(&after, &before[0], sizeof(after));
        else valid = status == 0;
        for (int i = 0; i < 3; ++i) if (i != target && !Unchanged(i, &before[i], beforeStatus[i])) valid = 0;
        if (!valid) { fprintf(stderr, "[background-audit] file readback mismatch case=%d status=%d\n", sFileCase, status); exit(2); }
        Port_ReproBackground_Event("file_write_readback", status);
        Port_ReproBackground_Event("file_write_returned", sFileCase);
        phase = 11;
        return 1;
    }
    if (phase == 11) { phase = 0; ready = 0; }
    return 0;
}
