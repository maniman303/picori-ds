/* Native world cursor entry and distinct local-hint source variants. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "fade.h"
#include "flags.h"
#include "game.h"
#include "item.h"
#include "kinstone.h"
#include "main.h"
#include "menu.h"
#include "pauseMenu.h"
#include "room.h"
#include "save.h"
#include "subtask.h"
#include "port_gba_mem.h"
#include "port_repro.h"
extern void InitPauseMenu(void);
extern const u8 gUnk_08128DE8[];
static int sCase = -1, sTarget, sHintIds[100], sHintCount;
int Port_ReproBackground_MapNavigationCase(void) { return sCase; }
int Port_ReproBackground_MapNavigationTarget(void) { return sTarget; }
int Port_ReproBackground_MapNavigationHints(void) { return sHintCount; }
static u16 NextKey(int from, int target) {
    static const u16 keys[] = {DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT};
    int queue[17], first[17] = {0}, seen[17] = {0}, head = 0, tail = 0;
    if ((unsigned)from >= 17 || (unsigned)target >= 17) exit(2);
    queue[tail++] = from; seen[from] = 1;
    while (head < tail) {
        int at = queue[head++];
        const u8* p = &gUnk_08128DE8[at * 8];
        u16 vertical = p[0] | p[1] << 8, horizontal = p[2] | p[3] << 8;
        int neighbors[] = {vertical & 31, (vertical >> 5) & 31, horizontal & 31, (horizontal >> 5) & 31};
        for (int i = 0; i < 4; ++i) {
            int next = neighbors[i];
            if (next >= 17 || seen[next]) continue;
            first[next] = at == from ? keys[i] : first[at];
            if (next == target) return first[next];
            seen[next] = 1; queue[tail++] = next;
        }
    }
    fprintf(stderr, "[background-audit] unreachable map cursor %d -> %d\n", from, target); exit(2);
}
static void DiscoverHints(void) {
    int seen[17][9] = {{0}};
    const WorldEvent* events = GetWorldEvents();
    for (int id = 10; id <= 100; ++id) {
        const KinstoneWorldEvent* k = &gKinstoneWorldEvents_eu[id];
        if (!k->mapMarkerIcon || k->mapMarkerIcon > 8) continue;
        const WorldEvent* event = &events[k->worldEventId];
        const OverworldLocation* loc = GetOverworldLocation(event->_c, event->_e);
        const int area = loc ? loc->windcrestId : 0;
        if (area < 17 && !seen[area][k->mapMarkerIcon]++) sHintIds[sHintCount++] = id;
    }
}
int Port_ReproBackground_MapNavigationTick(unsigned frame) {
    static int phase, originalArea, originalRoom;
    static unsigned ready, since, deadline;
    const int gameplay = gMain.task == TASK_GAME && gMain.state == 2 && gMain.substate == 2 && !gFadeControl.active;
    const int pause = gMain.task == TASK_GAME && gMain.substate == 7 && gUI.lastState == SUBTASK_PAUSEMENU &&
                      gUI.nextToLoad == 2 && gPauseMenuOptions.unk11 == 2 && gMenu.menuType == 1 && !gFadeControl.active;
    const int hint = gMain.task == TASK_GAME && gMain.substate == 7 && gUI.lastState == SUBTASK_LOCALMAPHINT &&
                     gUI.nextToLoad == 2 && gMenu.menuType == 1 && !gMenu.transitionTimer && !gFadeControl.active;
    if (phase == 15) return 0;
    if (deadline && frame > deadline) { fprintf(stderr, "[background-audit] navigation timeout case=%d phase=%d screen=%u cursor=%u\n", sCase, phase, gPauseMenuOptions.screen, gMenu.field_0x3); exit(2); }
    if (!phase) {
        if (!gameplay) return 0;
        if (sCase < 0) { DiscoverHints(); sCase = 0; }
        if (sCase == 44 + sHintCount) { Port_ReproBackground_Event("map_navigation_complete", sCase); phase = 15; return 2; }
        deadline = frame + 15000;
        if (sCase < 44) {
            SetInventoryValue(ITEM_MAP, 1);
            gSave.stats.maxHealth = 24; gSave.stats.health = sCase < 22 ? 24 : 8;
            gSave.windcrests |= 0x1ffff;
            gSave.map_hints = sCase < 22 ? 0 : 0xffff;
            for (int id = 10; id <= 100; ++id) {
                WriteBit(gSave.kinstones.fusedKinstones, id);
                if (sCase < 22) WriteBit(gSave.kinstones.fusionUnmarked, id);
                else gSave.kinstones.fusionUnmarked[id / 8] &= ~(1 << (id & 7));
            }
            gPauseMenuOptions.screen = 4;
            Port_ReproBackground_Event("nav_open", sCase / 22);
            InitPauseMenu(); phase = 1;
        } else {
            int id = sHintIds[sCase - 44];
            gSave.stats.health = sCase % 3 == 0 ? 24 : sCase % 3 == 1 ? 15 : 8;
            gFuseInfo.kinstoneId = id;
            Port_ReproBackground_Event("nav_hint_request", id);
            MenuFadeIn(SUBTASK_LOCALMAPHINT, gKinstoneWorldEvents_eu[id].worldEventId);
            phase = 9;
        }
        ready = 0; return 0;
    }
    if (phase == 1 && pause && gPauseMenuOptions.screen == 4) {
        originalArea = gUI.roomControls.area; originalRoom = gUI.roomControls.room;
        phase = 2; since = frame;
    }
    if (phase == 2 && pause && gPauseMenuOptions.screen == 4) {
        const int recipe = sCase % 22;
        sTarget = recipe < 17 ? recipe : recipe == 17 ? 9 : recipe == 18 ? 11 : 14;
        ClearGlobalFlag(TATEKAKE_HOUSE);
        gSave.kinstones.fusedKinstones[KINSTONE_E / 8] &= ~(1 << (KINSTONE_E & 7));
        if (recipe == 17) SetGlobalFlag(TATEKAKE_HOUSE);
        if (recipe == 18) WriteBit(gSave.kinstones.fusedKinstones, KINSTONE_E);
        gUI.roomControls.area = recipe >= 19 ? 8 : originalArea;
        gUI.roomControls.room = recipe >= 19 ? recipe - 19 : originalRoom;
        if (gMenu.field_0x3 != sTarget) {
            ready = 0;
            if (frame >= since + 6) { *(vu16*)(gIoMem + 0x130) &= ~NextKey(gMenu.field_0x3, sTarget); since = frame; }
            return 0;
        }
        if (!ready) ready = frame;
        if (frame < ready + 16) return 0;
        Port_ReproBackground_Event("nav_world_ready", sCase);
        since = frame; phase = 3; return 1;
    }
    if (phase == 3 && frame >= since + 6) {
        Port_ReproBackground_Event("nav_enter_input", A_BUTTON);
        *(vu16*)(gIoMem + 0x130) &= ~A_BUTTON; ready = 0; phase = 4;
    }
    if (phase == 4 && pause && gPauseMenuOptions.screen == 6) {
        if (!ready) ready = frame;
        if (frame < ready + 16) return 0;
        Port_ReproBackground_Event("nav_detail_ready", sCase);
        since = frame; phase = 5; return 1;
    }
    if (phase == 5 && frame >= since + 6) {
        gUI.roomControls.area = originalArea; gUI.roomControls.room = originalRoom;
        Port_ReproBackground_Event("nav_back_input", B_BUTTON);
        *(vu16*)(gIoMem + 0x130) &= ~B_BUTTON; ready = 0; phase = 6;
    }
    if (phase == 6 && pause && gPauseMenuOptions.screen == 4) {
        if (!ready) ready = frame;
        if (frame < ready + 16) return 0;
        Port_ReproBackground_Event("nav_world_returned", sCase);
        phase = 13; return 1;
    }
    if (phase == 13) {
        ++sCase; ready = 0; since = frame; phase = sCase % 22 ? 2 : 7;
    }
    if (phase == 7 && frame >= since + 6) {
        Port_ReproBackground_Event("nav_exit_input", START_BUTTON);
        *(vu16*)(gIoMem + 0x130) &= ~START_BUTTON; ready = 0; phase = 8;
    }
    if (phase == 8 && gameplay) {
        Port_ReproBackground_Event("nav_game_restored", sCase / 22 - 1);
        phase = 0; return 1;
    }
    if (phase == 9 && hint) {
        if (!ready) ready = frame;
        if (frame < ready + 16) return 0;
        Port_ReproBackground_Event("nav_hint_ready", gFuseInfo.kinstoneId);
        since = frame; phase = 16; return 1;
    }
    if (phase == 16 && frame >= since + 4) {
        gSave.stats.health = gSave.stats.health == 24 ? 8 : 24;
        since = frame; phase = 10;
    }
    if (phase == 10 && frame >= since + 16) {
        Port_ReproBackground_Event("nav_hint_held", gFuseInfo.kinstoneId);
        since = frame; phase = 11; return 1;
    }
    if (phase == 11 && frame >= since + 6) {
        const u16 keys[] = {A_BUTTON, B_BUTTON, START_BUTTON};
        Port_ReproBackground_Event("nav_hint_exit_input", keys[sCase % 3]);
        *(vu16*)(gIoMem + 0x130) &= ~keys[sCase % 3]; phase = 12;
    }
    if (phase == 12 && gameplay) {
        Port_ReproBackground_Event("nav_hint_restored", gFuseInfo.kinstoneId);
        phase = 14; return 1;
    }
    if (phase == 14) { ++sCase; phase = 0; }
    return 0;
}
