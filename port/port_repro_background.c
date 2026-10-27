/* Native fixed-screen audit. Only compiled with repro_harness=y.
 * TMC_BACKGROUND_AUDIT=credits|prologue|intro|files|local_hint|menus|travel|figurines|hints|dungeons|travel_actions|figurine_actions|detail_maps|sleep|save|inventory_save|gameover|fusion_actions|ending_actions|file_writes|map_navigation|file_errors|gameover_returns;
 * TMC_BACKGROUND_AUDIT_OUT=<directory>.
 * The driver stages a synthetic save in memory, then runs native task/subtask
 * code at normal simulation speed. No timer, graphics or fade is skipped.
 * Events describe installation, never infer installed art from a text command.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "area.h"
#include "fade.h"
#include "fileselect.h"
#include "flags.h"
#include "kinstone.h"
#include "pauseMenu.h"
#include "game.h"
#include "main.h"
#include "menu.h"
#include "player.h"
#include "message.h"
#include "item.h"
#include "screen.h"
#include "subtask.h"
#include "port_gba_mem.h"
#include "port_repro.h"

extern u32 GetAmountInKinstoneBag(KinstoneId id);

extern int Port_CaptureBaseFramebufferPNG(const char* path);
extern void SetActiveSave(u32 idx);

static FILE* sTrace;
static const char* sDirectory;
static unsigned sFrame, sEvent, sLastEventFrame;
static int sInstalled = -1; /* observation only; not a runtime descriptor */
static int sFinished;
static int sDecoration = -1, sPauseScreen = -1, sMapGroup = -1;

/* Observation schema v2: semantic identity follows native installation. Heading
 * pose is sampled at the actual DrawDirect call, including transition frames.
 * This instrumentation is deliberately independent of the future adapter. */
static const char* sFamily = "unknown";
static int sFusionLayout = -1;
static int sTravelInstalled, sFigurineInstalled, sHintInstalled;
static int sHeading, sHeadingX, sHeadingY, sHeadingTransition;
static unsigned sHeadingGeneration;

void Port_ReproBackground_Heading(int style, int x, int y, int transition) {
    if (style != sHeading || x != sHeadingX || y != sHeadingY || transition != sHeadingTransition)
        ++sHeadingGeneration;
    sHeading = style;
    sHeadingX = x;
    sHeadingY = y;
    sHeadingTransition = transition;
}

static void WriteDescriptor(void) {
    /* Observe the resource slot, not the requested config language. EU slots
     * are offset from the public language enum; a fallback must fail the audit. */
    fprintf(sTrace, ",\"language_slot\":%u", gSaveHeader->language);
    fprintf(sTrace, ",\"schema\":2,\"family\":\"%s\",\"heading_style\":%d,\"heading_x\":%d,"
                    "\"heading_y\":%d,\"heading_transition\":%d", sFamily, sHeading,
                    sHeadingX, sHeadingY, sHeadingTransition);
    fprintf(sTrace, ",\"nav_case\":%d,\"nav_target\":%d,\"nav_hints\":%d,\"nav_cursor\":%u,\"nav_markers\":%u,"
                    "\"nav_keys\":%u,\"nav_task\":%u,\"nav_substate\":%u,\"nav_ui\":%u,\"nav_next\":%u,\"nav_fade\":%u,"
                    "\"nav_id\":%u,\"nav_icon\":%u,\"nav_timer\":%u,\"writes_delete_checked\":1",
            Port_ReproBackground_MapNavigationCase(), Port_ReproBackground_MapNavigationTarget(), Port_ReproBackground_MapNavigationHints(),
            gMenu.field_0x3, gGenericMenu.unk2d, gInput.newKeys, gMain.task, gMain.substate, gUI.lastState, gUI.nextToLoad,
            gFadeControl.active, gFuseInfo.kinstoneId, gFuseInfo.kinstoneId <= 117 ? gKinstoneWorldEvents_eu[gFuseInfo.kinstoneId].mapMarkerIcon : 0,
            gMenu.transitionTimer);
    fprintf(sTrace, ",\"ending_case\":%d,\"file_write_case\":%d,\"writes_task\":%u,\"writes_state\":%u,"
                    "\"writes_ui\":%u,\"writes_menu\":%u,\"writes_overlay\":%u,\"writes_timer\":%u,"
                    "\"writes_keys\":%u,\"writes_fade\":%u,\"writes_column\":%u,\"writes_row\":%u,\"writes_dest\":%u,"
                    "\"writes_slot0\":%d,\"writes_slot1\":%d,\"writes_slot2\":%d,\"writes_name\":%u,\"writes_name_len\":%u,"
                    "\"writes_clear\":%u,\"writes_staffroll\":%u,\"writes_root_state\":%u",
            Port_ReproBackground_EndingCase(), Port_ReproBackground_FileWriteCase(), gMain.task, gMain.state,
            gUI.lastState, gMenu.menuType, gMenu.overlayType, gMenu.transitionTimer, gInput.newKeys, gFadeControl.active,
            gMenu.column_idx, gFileSelectState.unk6, gFileSelectState.unk7,
            gFileSelectState.saveStatus[0], gFileSelectState.saveStatus[1], gFileSelectState.saveStatus[2],
            gSave.name[0], gGenericMenu.unk10.a[3], (unsigned)CheckGlobalFlag(GAMECLEAR), gSave.saw_staffroll,
            gChooseFileState.state);
    fprintf(sTrace, ",\"fusion_action_case\":%d,\"fusion_task\":%u,\"fusion_substate\":%u,\"fusion_ui\":%u,\"fusion_next\":%u,"
                    "\"fusion_menu\":%u,\"fusion_overlay\":%u,\"fusion_column\":%u,\"fusion_cursor\":%u,\"fusion_selected\":%u,"
                    "\"fusion_timer\":%u,\"fusion_keys\":%u,\"fusion_fade\":%u,\"fusion_brightness\":%u,"
                    "\"fusion_id\":%u,\"fusion_result\":%u,\"fusion_count\":%u,\"fusion_owned\":%u,"
                    "\"fusion_piece109\":%u,\"fusion_piece110\":%u",
            Port_ReproBackground_FusionActionCase(), gMain.task, gMain.substate, gUI.lastState, gUI.nextToLoad,
            gMenu.menuType, gMenu.overlayType, gMenu.column_idx, gGenericMenu.unk28, gGenericMenu.unk2a,
            gMenu.transitionTimer, gInput.newKeys, gFadeControl.active, gScreen.controls.layerBrightness,
            gFuseInfo.kinstoneId, gFuseInfo.fusionState, gSave.kinstones.fusedCount, CheckKinstoneFused(gFuseInfo.kinstoneId),
            GetAmountInKinstoneBag(109), GetAmountInKinstoneBag(110));
    fprintf(sTrace, ",\"fusion_layout\":%d", strcmp(sFamily, "fusion") == 0 ? sFusionLayout : -1);
    fprintf(sTrace, ",\"travel_case\":%d,\"travel_field\":%d,\"travel_destination\":%u,\"travel_unlocks\":%u,\"display_control\":%u",
            Port_ReproBackground_TravelCase(), sTravelInstalled, gMenu.field_0x3, (unsigned)(gSave.windcrests >> 24),
            gScreen.lcd.displayControl);
    fprintf(sTrace, ",\"figurine_case\":%d,\"figurine_field\":%d,\"figurine_index\":%u,"
                    "\"figurine_owned\":%u,\"staffroll\":%u,\"palette_zero\":%u,\"palette_251\":%u,"
                    "\"detail_scroll\":%d,\"detail_limit\":%u",
            Port_ReproBackground_FigurineCase(), sFigurineInstalled, gFigurineMenu.figure_idx,
            (unsigned)ReadBit(gSave.figurines, gFigurineMenu.figure_idx), gSave.saw_staffroll,
            gPaletteBuffer[0], gPaletteBuffer[251], gFigurineMenu.unk1f, gFigurineMenu.unk1e);
    fprintf(sTrace, ",\"figurine_action_case\":%d,\"figurine_variant\":%u,\"figurine_menu\":%u,"
                    "\"figurine_counter\":%u,\"figurine_duplicate\":%u,\"figurine_brightness\":%u,"
                    "\"figurine_blend\":%u,\"figurine_bg1_y\":%d,\"figurine_input\":%u",
            Port_ReproBackground_FigurineActionCase(), gUI.field_0x3, gMenu.menuType,
            gFigurineMenu.unk20, gFigurineMenu.duplicate, gScreen.controls.layerBrightness,
            gScreen.controls.layerFXControl, (s16)gScreen.bg1.yOffset, gInput.menuScrollKeys);
    fprintf(sTrace, ",\"hint_case\":%d,\"hint_field\":%d,\"hint_variant\":%u,\"hint_timer\":%u,"
                    "\"hint_mask\":%u,\"hint_pending\":%u,\"saved_hints\":%u,\"palette_81\":%u",
            Port_ReproBackground_HintCase(), sHintInstalled, gUI.field_0x3, gMenu.transitionTimer,
            gGenericMenu.unk10.h[0], gGenericMenu.unk10.h[1], gSave.map_hints, gPaletteBuffer[81]);
    fprintf(sTrace, ",\"dungeon_case\":%d,\"dungeon_index\":%u,\"dungeon_floor\":%u,\"dungeon_items\":%u,\"dungeon_area\":%u",
            Port_ReproBackground_DungeonCase(), gArea.dungeon_idx, gMenu.field_0x3,
            gArea.dungeon_idx < 16 ? gSave.dungeonItems[gArea.dungeon_idx] : 0,
            gMain.substate == GAMEMAIN_SUBTASK ? gUI.roomControls.area : gRoomControls.area);
    const int detailArea = gMenu.field_0x3;
    const struct_gUnk_08128E94* detail = &gUnk_08128E94[detailArea < 17 ? detailArea : 0];
    fprintf(sTrace, ",\"detail_map_case\":%d,\"detail_map_area\":%u,\"detail_map_scroll\":%u,"
                    "\"detail_map_limit\":%u,\"detail_bg1_y\":%d,\"detail_bg2_y\":%d,"
                    "\"detail_input\":%u,\"detail_pause_phase\":%u,\"detail_snapshot_area\":%u,"
                    "\"detail_snapshot_room\":%u,\"detail_house\":%u,\"detail_fused\":%u,\"detail_has_map\":%u",
            Port_ReproBackground_DetailMapCase(), detailArea, gMenu.field_0xa,
            detail->unk2, (s16)gScreen.bg1.yOffset, (s16)gScreen.bg2.yOffset,
            gInput.heldKeys, gPauseMenuOptions.unk11, gUI.roomControls.area, gUI.roomControls.room,
            (unsigned)CheckGlobalFlag(TATEKAKE_HOUSE), (unsigned)CheckKinstoneFused(KINSTONE_E), (unsigned)GetInventoryValue(ITEM_MAP));
    fprintf(sTrace, ",\"gameover_task\":%u,\"gameover_state\":%u,\"gameover_substate\":%u,\"gameover_fade_active\":%u",
            gMain.task, gMain.state, gMain.substate, gFadeControl.active);
    fprintf(sTrace, ",\"gameover_case\":%d,\"gameover_menu\":%u,\"gameover_cursor\":%u,\"gameover_timer\":%u,\"gameover_keys\":%u,\"gameover_y\":%u,\"death_flag\":%u",
            Port_ReproBackground_GameOverCase(), gMenu.menuType, gMenu.field_0x3,
            gMenu.transitionTimer, gInput.newKeys, gGenericMenu.unk10.a[0], gRoomTransition.field_0x4[1]);
    fprintf(sTrace, ",\"save_case\":%d,\"save_timer\":%u,\"save_stage\":%u",
            Port_ReproBackground_SaveCase(), gMenu.transitionTimer, gMenu.storyPanelIndex);
    fprintf(sTrace, ",\"save_cursor\":%u,\"save_menu\":%u,\"save_pause_phase\":%u,\"save_new_keys\":%u",
            gMenu.field_0x3, gMenu.menuType, gPauseMenuOptions.unk11, gInput.newKeys);
    fprintf(sTrace, ",\"sleep_case\":%d,\"sleep_status\":%u,\"sleep_ticks\":%u,"
                    "\"sleep_cursor\":%u,\"sleep_menu\":%u,\"sleep_pause_phase\":%u,"
                    "\"sleep_new_keys\":%u,\"sleep_raw_keys\":%u,\"sleep_raw_display\":%u",
            Port_ReproBackground_SleepCase(), gMain.sleepStatus, gMain.ticks,
            gMenu.field_0x3, gMenu.menuType, gPauseMenuOptions.unk11, gInput.newKeys,
            *(vu16*)(gIoMem + 0x130), *(vu16*)gIoMem);
    fprintf(sTrace, ",\"travel_text_status\":%u,\"travel_first_warp\":%u",
            gTextRender.renderStatus, (unsigned)CheckGlobalFlag(WARP_1ST));
    fprintf(sTrace, ",\"travel_story_progress\":%u,\"travel_ocarina\":%u",
            gSave.global_progress, (unsigned)GetInventoryValue(ITEM_OCARINA));
    fprintf(sTrace, ",\"travel_story_flags\":%u",
            (CheckGlobalFlag(EZERO_1ST) ? 1u : 0u) |
            (CheckLocalFlagByBankB(FLAG_BANK_1, SOUGEN_01_ZELDA) ? 2u : 0u) |
            (CheckLocalFlagByBankB(FLAG_BANK_1, MORI_ENTRANCE_1ST) ? 4u : 0u) |
            (CheckLocalFlagByBankB(FLAG_BANK_1, MORI_00_KOBITO) ? 8u : 0u) |
            (CheckGlobalFlag(TABIDACHI) ? 16u : 0u));
    fprintf(sTrace, ",\"travel_action_case\":%d,\"travel_phase\":%u,\"travel_choice\":%d,\"travel_message\":%u,"
                    "\"travel_subtask\":%u,\"travel_game_state\":%u,\"travel_ui\":%u,"
                    "\"travel_fade_active\":%u,\"travel_fade_type\":%u,\"travel_fade_progress\":%u,\"travel_fade_mask\":%u",
            Port_ReproBackground_TravelActionCase(), gMenu.menuType, Port_ReproBackground_MessageChoice(),
            gMessage.state, gMain.substate, gMain.state, gUI.lastState,
            gFadeControl.active, gFadeControl.type, gFadeControl.progress, gFadeControl.mask);
    fprintf(sTrace, ",\"world_area\":%u,\"world_room\":%u,\"player_layer\":%u,\"player_action\":%u,\"player_control\":%u,"
                    "\"target_area\":%u,\"target_room\":%u,\"target_x\":%u,\"target_y\":%u,\"target_layer\":%u",
            gRoomControls.area, gRoomControls.room, gPlayerEntity.base.collisionLayer,
            gPlayerEntity.base.action, gPlayerState.controlMode,
            gRoomTransition.player_status.area_next, gRoomTransition.player_status.room_next,
            (unsigned)(u16)gRoomTransition.player_status.start_pos_x, (unsigned)(u16)gRoomTransition.player_status.start_pos_y,
            gRoomTransition.player_status.layer);
}

void Port_ReproBackground_Event(const char* event, unsigned int value) {
    if (!sTrace)
        return;
    if (strcmp(event, "menu_decoration") == 0)
        sDecoration = value;
    if (strcmp(event, "pause_screen") == 0) {
        sPauseScreen = value;
        sFamily = value == 7 ? "pieces" : value == 8 ? "techniques" :
                  value >= 9 && value <= 11 ? "plain" :
                  value == 1 || value == 2 || (value >= 4 && value <= 6) ? "titled" : "unknown";
        /* Map hints/figurines/fast travel reuse this initializer. Its screen
         * argument alone is not evidence of an installed pause family. */
        if (gUI.lastState != SUBTASK_PAUSEMENU)
            sFamily = "unknown";
    }
    if (strcmp(event, "fast_travel_screen") == 0) {
        /* Group 93 replaced the health decoration with its fixed BG3 map.
         * It has its own fixed-image catalog context; do not alias a pause family. */
        sTravelInstalled = 1;
        Port_ReproBackground_Heading(0, 0, 0, 0);
    }
    if (strcmp(event, "fast_travel_handoff") == 0)
        sTravelInstalled = 0;
    if (strcmp(event, "hint_screen") == 0) {
        sHintInstalled = 1;
        Port_ReproBackground_Heading(0, 0, 0, 0);
    }
    if (strcmp(event, "figurine_screen") == 0) {
        sFigurineInstalled = 1;
        Port_ReproBackground_Heading(0, 0, 0, 0);
    }
    if (strcmp(event, "fusion_world_event_install") == 0 || strcmp(event, "fusion_local_hint_install") == 0) {
        sFamily = "unknown";
        Port_ReproBackground_Heading(0, 0, 0, 0);
    }
    if (strcmp(event, "fusion_screen") == 0) {
        sPauseScreen = -1;
        sFamily = "fusion";
        sFusionLayout = value;
    }
    if (strcmp(event, "map_detail") == 0)
        sMapGroup = value;
    if (strcmp(event, "credits_install") == 0 || strcmp(event, "prologue_install") == 0)
        sInstalled = (int)value;
    if (strcmp(event, "credits_clear") == 0 || strcmp(event, "credits_save") == 0 ||
        strcmp(event, "prologue_clear") == 0 || strcmp(event, "prologue_world") == 0)
        sInstalled = -1;
    fprintf(sTrace, "{\"event\":\"%s\",\"value\":%u,\"frame\":%u,\"installed\":%d,"
            "\"case\":%d,\"decoration\":%d,\"pause_screen\":%d,\"map_group\":%d,\"health\":%u,\"max_health\":%u",
            event, value, sFrame, sInstalled, Port_ReproBackground_MenuCase(),
            sDecoration, sPauseScreen, sMapGroup, gSave.stats.health, gSave.stats.maxHealth);
    WriteDescriptor();
    fputs("}\n", sTrace);
    sEvent++;
    sLastEventFrame = sFrame;
    if ((strcmp(event, "credits_save") == 0 && Port_ReproBackground_EndingCase() < 0) || strcmp(event, "prologue_clear") == 0 ||
        strcmp(event, "prologue_world") == 0)
        sFinished = 1;
}

void Port_ReproBackground_Tick(unsigned int frame) {
    static int mode = -1, stage;
    static unsigned readyFrame, finishFrame, lastCapturedEvent, lastHeadingGeneration, fileStart, hintReady;
    if (mode == -1) {
        const char* value = getenv("TMC_BACKGROUND_AUDIT");
        mode = 0;
        if (!value || !*value)
            return;
        mode = strcmp(value, "credits") == 0 ? 1 : strcmp(value, "prologue") == 0 ? 2 :
               strcmp(value, "files") == 0 ? 3 : strcmp(value, "intro") == 0 ? 4 :
               strcmp(value, "local_hint") == 0 ? 5 : strcmp(value, "menus") == 0 ? 6 : strcmp(value, "travel") == 0 ? 7 : strcmp(value, "figurines") == 0 ? 8 : strcmp(value, "hints") == 0 ? 9 : strcmp(value, "dungeons") == 0 ? 10 : strcmp(value, "travel_actions") == 0 ? 11 : strcmp(value, "figurine_actions") == 0 ? 12 : strcmp(value, "detail_maps") == 0 ? 13 : strcmp(value, "sleep") == 0 ? 14 : strcmp(value, "save") == 0 ? 15 : strcmp(value, "inventory_save") == 0 ? 16 : strcmp(value, "gameover") == 0 ? 17 : strcmp(value, "fusion_actions") == 0 ? 18 : strcmp(value, "ending_actions") == 0 ? 19 : strcmp(value, "file_writes") == 0 ? 20 : strcmp(value, "map_navigation") == 0 ? 21 : strcmp(value, "file_errors") == 0 ? 22 : strcmp(value, "gameover_returns") == 0 ? 23 : 0;
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
    if (gMain.task != TASK_GAME || gMain.substate != GAMEMAIN_SUBTASK ||
        (gUI.lastState != SUBTASK_PAUSEMENU && gUI.lastState != SUBTASK_KINSTONEMENU)) {
        sFamily = "unknown";
        Port_ReproBackground_Heading(0, 0, 0, 0);
    }
    if (gMain.task != TASK_GAME || gMain.substate != GAMEMAIN_SUBTASK || gUI.lastState != SUBTASK_FASTTRAVEL)
        sTravelInstalled = 0;
    if (gMain.task != TASK_GAME || gMain.substate != GAMEMAIN_SUBTASK || gUI.lastState != SUBTASK_FIGURINEMENU)
        sFigurineInstalled = 0;
    if (gMain.task != TASK_GAME || gMain.substate != GAMEMAIN_SUBTASK || gUI.lastState != SUBTASK_MAPHINT)
        sHintInstalled = 0;
    const int navigationResult = mode == 21 ? Port_ReproBackground_MapNavigationTick(frame) : 0;
    const int writesResult = mode == 19 ? Port_ReproBackground_EndingTick(frame) : ((mode == 20 || mode == 22) && stage) ? Port_ReproBackground_FileWritesTick(frame) : 0;
    const int fusionActionResult = mode == 18 ? Port_ReproBackground_FusionActionsTick(frame) : 0;
    const int gameOverResult = (mode == 17 || mode == 23) ? Port_ReproBackground_GameOverTick(frame) : 0;
    const int saveResult = (mode == 15 || mode == 16) ? Port_ReproBackground_SaveTick(frame) : 0;
    const int sleepResult = mode == 14 ? Port_ReproBackground_SleepTick(frame) : 0;
    const int detailMapResult = mode == 13 ? Port_ReproBackground_DetailMapsTick(frame) : 0;
    const int figurineActionResult = mode == 12 ? Port_ReproBackground_FigurineActionsTick(frame) : 0;
    const int travelActionResult = mode == 11 ? Port_ReproBackground_TravelActionsTick(frame) : 0;
    const int dungeonResult = mode == 10 ? Port_ReproBackground_DungeonsTick(frame) : 0;
    const int hintResult = mode == 9 ? Port_ReproBackground_HintsTick(frame) : 0;
    const int figurineResult = mode == 8 ? Port_ReproBackground_FigurinesTick(frame) : 0;
    const int menuResult = mode == 6 ? Port_ReproBackground_MenusTick(frame, sDecoration, sPauseScreen, sMapGroup) : 0;
    const int travelResult = mode == 7 ? Port_ReproBackground_TravelTick(frame, sDecoration) : 0;
    if ((navigationResult | writesResult | fusionActionResult | gameOverResult | menuResult | travelResult | figurineResult | hintResult | dungeonResult | travelActionResult | figurineActionResult | detailMapResult | sleepResult | saveResult) & 2)
        sFinished = 1;
    if (frame > ((mode == 11 || mode == 12 || mode == 13) ? 60000u : 30000u)) {
        fprintf(stderr, "[background-audit] timed out\n");
        exit(2);
    }

    /* Trace every simulation boundary. PNGs sample the completed framebuffer;
     * mutable engine state can lead that image by one tick at an installation.
     * Also capture two ticks after events, after the VBlank upload/present. */
    const int prizeBlink = mode == 12 && gMain.substate == GAMEMAIN_SUBTASK &&
        gUI.lastState == SUBTASK_FIGURINEMENU && gMenu.menuType == 2 &&
        gFigurineMenu.unk20 > 0 && gFigurineMenu.unk20 < 128 && frame % 4 == 0;
    const int capture = prizeBlink || sHeadingGeneration != lastHeadingGeneration || ((navigationResult | writesResult | fusionActionResult | gameOverResult | menuResult | travelResult | figurineResult | hintResult | dungeonResult | travelActionResult | figurineActionResult | detailMapResult | sleepResult | saveResult) & 1) || frame % 60 == 0 || (gFadeControl.active && frame % 8 == 0) ||
        (sEvent != lastCapturedEvent && frame >= sLastEventFrame + 2);
    fprintf(sTrace, "{\"frame\":%u,\"task\":%u,\"state\":%u,\"substate\":%u,"
            "\"ui\":%u,\"menu\":%u,\"overlay\":%u,\"installed\":%d,"
            "\"fade_active\":%u,\"fade_type\":%u,\"fade_progress\":%u,\"fade_mask\":%u,"
            "\"bldcnt\":%u,\"alpha\":%u,\"case\":%d,\"decoration\":%d,\"pause_screen\":%d,"
            "\"map_group\":%d,\"health\":%u,\"max_health\":%u,\"capture\":%s",
            frame, gMain.task, gMain.state, gMain.substate, gUI.lastState, gMenu.menuType,
            gMenu.overlayType, sInstalled, gFadeControl.active, gFadeControl.type,
            gFadeControl.progress, gFadeControl.mask, gScreen.controls.layerFXControl,
            gScreen.controls.alphaBlend, Port_ReproBackground_MenuCase(), sDecoration, sPauseScreen,
            sMapGroup, gSave.stats.health, gSave.stats.maxHealth, capture ? "true" : "false");
    WriteDescriptor();
    fputs("}\n", sTrace);
    if (capture) {
        char path[4096];
        if (snprintf(path, sizeof(path), "%s/%06u.png", sDirectory, frame) >= (int)sizeof(path) ||
            !Port_CaptureBaseFramebufferPNG(path)) {
            fprintf(stderr, "[background-audit] capture failed at %u\n", frame);
            exit(2);
        }
        lastCapturedEvent = sEvent;
        lastHeadingGeneration = sHeadingGeneration;
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
        if (mode == 3 || mode == 20 || mode == 22) {
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
