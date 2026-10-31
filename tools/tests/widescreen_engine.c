/* Small fixture for the unchanged production helper bodies extracted by the runner. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "port_viewport.h"
#include "port_tall_scene.h"
#include "port_village_groups.h"
typedef uint8_t u8;
typedef int16_t s16;
typedef int32_t s32;
typedef uint16_t u16;
typedef uint32_t u32;
#define MODE1_GBA_WIDTH 384
#define PC_PORT 1
#define MODE1_GBA_BG_CLIP_X 240
#define MODE1_WS_SHADOW_COLS 32
#define MODE1_WS_SHADOW_ROWS 32
#define MODE1_WS_HUD_KEYS 1
#define MODE1_WS_HUD_RUPEES 2
#define HUD_HIDE_KEYS 0x80
#define HUD_HIDE_RUPEES 0x40
static struct { int scroll_x, scroll_y, origin_x, origin_y, width, height, scrollAction, area, room;
                int aff_y, shake_duration, shake_magnitude, reload_flags; } gRoomControls;
static struct { unsigned active, xPos, yPos, width, height; } gCurrentWindow;
static struct { s16 buttonX[3]; unsigned hideFlags, unk_10, unk_a; } gHUD;
static struct { struct { unsigned textWindowWidth, textWindowHeight, textWindowPosX, textWindowPosY; } message; } gTextRender;
static int virtuappu_mode1_ws_shadow_base_tile[4];
static int virtuappu_mode1_ws_shadow_start_x[4] = {240, 240, 240, 240};
static u16* virtuappu_mode1_ws_shadow[4];
static uint64_t sWsContentKey = UINT64_MAX;
static int sWsContentPx, sWsStageHeight, sWsFixedInstalled;
static u16 gOamMem[512];
static bool Port_Oam_VisibleSlotsTagged(const u16* p) { (void)p; return true; }
void Port_VillageGroups_Update(unsigned bg_mask) { (void)bg_mask; }
void Port_VillageGroups_Reset(void) {}
static uint64_t sWsValidatedKey = UINT64_MAX;
static PortViewportUpload sWsUpload;
static u8 gShakeOffsets[256], gManagerCount, gUnk_03003DE0, gUpdateVisibleTiles;
static int Port_Widescreen_MapTileX(void) { return (u16)(gRoomControls.scroll_x - gRoomControls.origin_x) >> 4; }
static int Port_Widescreen_MapTileY(void) { return (u16)(gRoomControls.scroll_y - gRoomControls.origin_y) >> 4; }
static unsigned Port_Widescreen_MapBufferSize(void) { return 27 * 64; }
static void UpdateScrollVram(void) {}
void Port_Widescreen_PublishScene(void);
void Port_Widescreen_BeforePresent(void);
int Port_Widescreen_EffectiveViewWidth(void);
/* Only unrelated task/HUD dependencies are stubbed; shadow publication runs
 * the production UpdateShadows body, including its area and BG gates. */
enum { TASK_TITLE = 0, TASK_FILE_SELECT = 1, TASK_GAME = 2, TASK_GAMEOVER = 3, TASK_STAFFROLL = 4,
       GAMETASK_MAIN = 2, GAMEMAIN_CHANGEROOM = 1, GAMEMAIN_UPDATE = 2,
       TRANSITION_FADE_BLACK = 5, FADE_INSTANT = 4, AREA_MINISH_WOODS = 0x0b, AREA_HYRULE_FIELD = 0x0a,
       AREA_CAVE_OF_FLAMES = 0x60, MANAGER = 9, ENTER_ROOM_TEXTBOX_MANAGER = 1 };
enum { GAMEMAIN_SUBTASK = 7, SUBTASK_PAUSEMENU = 1, SUBTASK_MAPHINT = 3, SUBTASK_KINSTONEMENU = 4,
       SUBTASK_AUXCUTSCENE = 5,
       SUBTASK_FIGURINEMENU = 7, SUBTASK_FASTTRAVEL = 9, SUBTASK_LOCALMAPHINT = 10 };
#define MODE1_GBA_BG_COUNT 4
#define DISPCNT_BG3_ON 0x800
#define DISPCNT_WIN0_ON 0x2000
#define DISPCNT_WIN1_ON 0x4000
#define DISPCNT_OBJWIN_ON 0x8000
static int port_hdma_has_active_channels(void) { return 0; }
static int port_hdma_can_continue_brightness(const void* io, size_t size) { (void)io; (void)size; return 0; }
static u8 gIoMem[0x400];
static struct { int task, state, substate; } gMain;
static struct { int type; } gRoomTransition;
static struct { int active, type; } gFadeControl;
static struct { int nextToLoad, lastState; } gUI;
static struct { int menuType, field_0x0, field_0x3; } gMenu;
typedef struct { u32 control; int yOffset; } TestBg;
static struct { TestBg bg0, bg1, bg2, bg3; struct { int displayControl; } lcd; } gScreen;
static struct { TestBg* bgSettings; } gMapBottom, gMapTop;
static u16 gMapDataBottomSpecial[128*128], gMapDataTopSpecial[128*128];
static u16 sWsShadowBG1[32*32], sWsShadowBG2[32*32], sWsShadowOverlay[32*32];
static u16 vram[0x18000/2];
static u16 gBG1Buffer[0x400], gBG2Buffer[0x400];
#define gVram ((unsigned char*)vram)
static int virtuappu_mode1_ws_hud_right_anchor, virtuappu_mode1_ws_msg_shift;
static int virtuappu_mode1_ws_hud_shift_y, virtuappu_mode1_ws_msg_shift_y;
static int virtuappu_mode1_ws_msg_x0, virtuappu_mode1_ws_msg_x1;
static int virtuappu_mode1_ws_msg_y0, virtuappu_mode1_ws_msg_y1;
typedef struct Entity { struct Entity* next; u8 kind, id, type, type2, action; } Entity;
typedef struct GenericEntity { Entity base; } GenericEntity;
typedef struct Manager { struct Manager* prev; struct Manager* next; u8 kind, id, type, type2; } Manager;
typedef Manager Temp;
static GenericEntity gEntities[72], gAuxPlayerEntities[7];
static u8 gUnk_02033290[32 * 0x80];
#define MANAGER 9
#define MAX_ENTITIES 72
#define MAX_AUX_PLAYER_ENTITIES 7
static Entity* testBanner;
static Entity* FindEntityByID(int kind, int id, int list) { (void)kind; (void)id; (void)list; return testBanner; }
bool Message_GetWindowRect(int* x, int* y, int* width, int* height);
static int Port_WidescreenTallRows(void);
static int Port_WidescreenTallAperture(void);
static int Port_WidescreenCandidateWidth(void);
static bool Port_WidescreenMapSnapshotMatches(int height);
static void Port_WidescreenUploadMapBuffers(void);
void Port_Widescreen_MapUploaded(void);
#include "widescreen_helpers.h"
static int errors;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); ++errors; } } while (0)
int main(void) {
    gMain.task = TASK_GAME;
    Port_Viewport_Publish(PORT_ASPECT_ULTRAWIDE_21_9, false, 384, 160, 384);
    gRoomControls.width = 1024;
    gRoomControls.height = 1024;
    PortViewportScene live = { PORT_SCENE_WORLD | PORT_SCENE_WIDE_WORLD, 1024, 1024 };
    Port_Viewport_PublishScene(live);
    gHUD.unk_10 = 2; gHUD.unk_a = 2;
    CHECK(Port_Widescreen_HudRightAnchor() == (MODE1_WS_HUD_KEYS | MODE1_WS_HUD_RUPEES));
    gHUD.hideFlags = HUD_HIDE_KEYS;
    CHECK(Port_Widescreen_HudRightAnchor() == MODE1_WS_HUD_RUPEES);
    gHUD.hideFlags = HUD_HIDE_RUPEES;
    CHECK(Port_Widescreen_HudRightAnchor() == MODE1_WS_HUD_KEYS);
    gHUD.hideFlags = 1; /* unrelated button hide flag */
    CHECK(Port_Widescreen_HudRightAnchor() == (MODE1_WS_HUD_KEYS | MODE1_WS_HUD_RUPEES));
    gHUD.hideFlags = 0; gHUD.unk_10 = 0;
    CHECK(Port_Widescreen_HudRightAnchor() == MODE1_WS_HUD_RUPEES);
    gHUD.buttonX[0] = 208;
    CHECK(HudButtonTargetX(0) == 342);
    /* Hiding a widget animates it vertically; it must not also drift left. */
    gHUD.hideFlags = 255;
    CHECK(HudButtonTargetX(0) == 342);
    gHUD.buttonX[0] = -16;
    CHECK(HudButtonTargetX(0) == -16);
    live.capabilities = PORT_SCENE_FIXED_SCREEN;
    Port_Viewport_PublishScene(live);
    gHUD.buttonX[0] = 208;
    CHECK(HudButtonTargetX(0) == 208);
    gHUD.hideFlags = 0;
    static u16 map[128*128], shadow[MODE1_WS_SHADOW_COLS*32];
    for (int i=0; i<128*128; ++i) map[i] = (u16)(i+1);
    gRoomControls.scroll_y = 16;
    for (int phase=0; phase<16; ++phase) {
        gRoomControls.scroll_x = 64 + phase;
        Port_WidescreenShadow_Populate(1, map, shadow);
        for (int shake=-3; shake<=3; ++shake) {
            for (int x=240; x<384; ++x) {
                int col = ((x+phase+shake)&255)/8;
                int index = (col-virtuappu_mode1_ws_shadow_base_tile[1]+32)%32;
                int expectedCol = (64+phase+shake+x)/8;
                if (index >= MODE1_WS_SHADOW_COLS || shadow[index] != map[128+expectedCol]) {
                    fprintf(stderr,"FAIL shadow phase=%d shake=%d x=%d index=%d\n",phase,shake,x,index);
                    ++errors; break;
                }
            }
        }
    }
    for (int action=0; action<=5; ++action) {
        gRoomControls.scrollAction = action;
        Port_Widescreen_PublishScene();
        CHECK(Port_Widescreen_EffectiveViewWidth() == ((action==2 || action==4 || action==5) ? 240 : 374));
        CHECK(Port_Widescreen_CameraRestX(1000) == ((action==2 || action==4 || action==5) ? 784 : 650));
    }
    /* At the top edge the native fill duplicates map row zero into its
     * leading padding row. Upward shake exposes that row at the seam. */
    gRoomControls.scroll_x = 0;
    for (int y = 0; y < 8; ++y) {
        gRoomControls.scroll_y = y;
        Port_WidescreenShadow_Populate(1, map, shadow);
        for (int shake = -3; shake < 0; ++shake) {
            int row = (8 + y + shake) / 8;
            CHECK(shadow[row * 32 + 1] == map[30]);
        }
    }
    gRoomControls.scroll_y = 16;
    /* Room-edge padding stays transparent even when the backing map is stale. */
    gRoomControls.scroll_x = 0;
    gRoomControls.width = 256;
    Port_WidescreenShadow_Populate(1, map, shadow);
    CHECK(shadow[2] == map[128+31]);
    CHECK(shadow[3] == 0);
    gRoomControls.scrollAction = 1;
    Port_Widescreen_PublishScene();
    CHECK(Port_Widescreen_EffectiveViewWidth() == 256);
    Port_Viewport_Publish(PORT_ASPECT_NATIVE_3_2, false, 384, 160, 384);
    CHECK(Port_Widescreen_EffectiveViewWidth() == 240);
    Port_Viewport_Publish(PORT_ASPECT_ULTRAWIDE_21_9, false, 384, 160, 384);
    gRoomControls.width = 240;
    Port_Widescreen_PublishScene();
    CHECK(Port_Widescreen_EffectiveViewWidth() == 240);
    /* Overlay is a complete wrapping screenblock, independent of camera. */
    static u16 screen[32 * 32];
    for (int i = 0; i < 32 * 32; ++i) screen[i] = (u16)(i + 7);
    Port_WidescreenShadow_PopulateOverlay(screen, shadow);
    CHECK(virtuappu_mode1_ws_shadow[3] == shadow);
    CHECK(virtuappu_mode1_ws_shadow_base_tile[3] == 0);
    for (int row = 0; row < 32; ++row)
        for (int col = 0; col < MODE1_WS_SHADOW_COLS; ++col)
            CHECK(shadow[row * MODE1_WS_SHADOW_COLS + col] == screen[row * 32 + (col & 31)]);
    /* All three repeating overlays must publish the complete screenblock;
     * disabled overlays, unrelated canvases and native mode must not. */
    memcpy(gVram + 0xf000, screen, sizeof(screen));
    gMain.task = TASK_GAME;
    gRoomControls.width = 1024;
    const struct { int area, control, enabled, wide, expected; } overlays[] = {
        { AREA_MINISH_WOODS, 0x1e04, 1, 1, 1 },
        { AREA_HYRULE_FIELD, 0x1e05, 1, 1, 1 },
        { AREA_CAVE_OF_FLAMES, 0x1e04, 1, 1, 1 },
        { AREA_HYRULE_FIELD, 0x1e05, 0, 1, 0 },
        { AREA_HYRULE_FIELD, 0x1e05, 1, 0, 0 },
        { AREA_HYRULE_FIELD, 0x1d05, 1, 1, 0 },
        { 0, 0x1e04, 1, 1, 0 },
    };
    for (unsigned i = 0; i < sizeof(overlays)/sizeof(overlays[0]); ++i) {
        gRoomControls.area = overlays[i].area;
        gScreen.bg3.control = overlays[i].control;
        gScreen.lcd.displayControl = overlays[i].enabled ? DISPCNT_BG3_ON : 0;
        Port_Viewport_Publish(overlays[i].wide ? PORT_ASPECT_ULTRAWIDE_21_9 : PORT_ASPECT_NATIVE_3_2,
                              false, 384, 160, 384);
        Port_Widescreen_UpdateShadows();
        CHECK((virtuappu_mode1_ws_shadow[3] != NULL) == overlays[i].expected);
        if (overlays[i].expected && virtuappu_mode1_ws_shadow[3])
            CHECK(memcmp(virtuappu_mode1_ws_shadow[3], screen, sizeof(screen)) == 0);
    }
    Port_Viewport_Publish(PORT_ASPECT_ULTRAWIDE_21_9, false, 384, 160, 384);
    int x,y,w,h;
    gCurrentWindow.active=0;
    CHECK(!Message_GetWindowRect(&x,&y,&w,&h));
    gTextRender.message.textWindowWidth = 26;
    gTextRender.message.textWindowHeight = 4;
    gTextRender.message.textWindowPosX = 1;
    gTextRender.message.textWindowPosY = 12;
    for (int scale = 1; scale <= 8; ++scale) {
        int innerWidth = (scale * 26) / 8;
        int innerHeight = (scale * 4) / 8;
        innerWidth += innerWidth & 1;
        innerHeight += innerHeight & 1;
        gCurrentWindow.active = 1;
        gCurrentWindow.xPos = 14 - innerWidth / 2;
        gCurrentWindow.yPos = 14 - innerHeight / 2;
        gCurrentWindow.width = innerWidth;
        gCurrentWindow.height = innerHeight;
        CHECK(Message_GetFinalWindowRect(&x, &y, &w, &h));
        CHECK(x == 8 && y == 96 && w == 224 && h == 48);
    }
    gCurrentWindow.active = 1; gCurrentWindow.xPos = 13; gCurrentWindow.yPos = 14;
    gCurrentWindow.width = 2; gCurrentWindow.height = 0;
    CHECK(Message_GetFinalWindowRect(&x, &y, &w, &h));
    CHECK(x == 8 && y == 96 && w == 224 && h == 48);
    gTextRender.message.textWindowPosY = 1;
    CHECK(!Message_GetFinalWindowRect(&x, &y, &w, &h));
    gCurrentWindow.yPos = 3;
    CHECK(Message_GetFinalWindowRect(&x, &y, &w, &h));
    CHECK(y == 8);
    CHECK(!Message_GetFinalWindowRect(NULL, &y, &w, &h));
    gCurrentWindow.active = 0;
    gCurrentWindow.active=1; gCurrentWindow.xPos=5; gCurrentWindow.yPos=1;
    gCurrentWindow.width=18; gCurrentWindow.height=2;
    CHECK(Message_GetWindowRect(&x,&y,&w,&h));
    CHECK(x==40 && y==8 && w==160 && h==32);
    /* Closing animation uses the smaller currently drawn frame. */
    gCurrentWindow.xPos=13; gCurrentWindow.yPos=3;
    gCurrentWindow.width=2; gCurrentWindow.height=0;
    CHECK(Message_GetWindowRect(&x,&y,&w,&h));
    CHECK(x==104 && y==24 && w==32 && h==16);
    gCurrentWindow.active=0;
    CHECK(!Message_GetWindowRect(&x,&y,&w,&h));
    /* Published canvas and UI are independent of the room aperture. A
     * repeated presentation must ignore subsequent mutable game state. */
    gMain.task = TASK_GAME;
    gRoomControls.area = 1;
    gRoomControls.scrollAction = 1;
    gRoomControls.width = 256;
    gMapBottom.bgSettings = &gScreen.bg1;
    gScreen.bg1.control = 0x1c00;
    gMapDataBottomSpecial[31] = 1;
    Port_Widescreen_RoomLoaded();
    const PortViewportGeometry* geometry = Port_Viewport_Get();
    CHECK(geometry->world.width == 256 && geometry->world.x == 59);
    CHECK(geometry->canvas.width == 374 && geometry->uiSafe.width == 374);
    CHECK(geometry->nativeContent.x == 67 && geometry->world.height == 160);
    CHECK(Port_PPU_VisibleFrameWidth() == 374);
    uint32_t generation = geometry->generation;
    Port_Widescreen_PublishScene();
    CHECK(geometry->generation == generation);
    gRoomControls.width = 1024;
    gMain.task = 0;
    CHECK(Port_PPU_VisibleFrameWidth() == 374);
    CHECK(Port_Widescreen_EffectiveViewWidth() == 256);
    CHECK(geometry->generation == generation);
    Port_Widescreen_PublishScene();
    CHECK(Port_PPU_VisibleFrameWidth() == 240 && geometry->world.width == 240);
    CHECK(geometry->scene.capabilities & PORT_SCENE_FIXED_SCREEN);
    CHECK(geometry->canvas.width == 374 && geometry->generation != generation);

    /* Painted-width classification is held for the room. Loading it again
     * (including the same room ID) explicitly resamples after graphics load. */
    gMain.task = TASK_GAME;
    Port_Widescreen_RoomLoaded();
    CHECK(geometry->scene.capabilities & PORT_SCENE_NATIVE_CONTENT);
    CHECK(geometry->world.width == 240);
    gMapDataBottomSpecial[127] = 1;
    generation = geometry->generation;
    Port_Widescreen_UpdateShadows();
    CHECK(geometry->world.width == 240 && geometry->generation == generation);
    Port_Widescreen_RoomLoaded();
    CHECK(geometry->world.width == 374 && Port_PPU_VisibleFrameWidth() == 374);
    CHECK(!(geometry->scene.capabilities & PORT_SCENE_NATIVE_CONTENT));
    /* Banner rows wrap with the 256-pixel BG0 map, then clip to the native
     * raster. Offscreen bands must not reach the GPU as negative bounds. */
    Entity banner = { 0 };
    banner.action = 1;
    testBanner = &banner;
    gCurrentWindow.active = 0;
    const struct { int scroll, y0, y1; } bannerCases[] = {
        { 0, 40, 56 }, { 48, 0, 8 }, { 56, 0, 0 },
        { 200, 96, 112 }, { 256, 40, 56 }, { 511, 41, 57 },
        { 136, 160, 160 },
    };
    for (unsigned i = 0; i < sizeof(bannerCases) / sizeof(bannerCases[0]); ++i) {
        gScreen.bg0.yOffset = bannerCases[i].scroll;
        Port_Widescreen_UpdateShadows();
        if (bannerCases[i].y0 < bannerCases[i].y1) {
            CHECK(virtuappu_mode1_ws_msg_shift == 67);
            CHECK(virtuappu_mode1_ws_msg_y0 == bannerCases[i].y0);
            CHECK(virtuappu_mode1_ws_msg_y1 == bannerCases[i].y1);
        } else {
            CHECK(virtuappu_mode1_ws_msg_shift == 0);
        }
    }
    testBanner = NULL;
    gScreen.bg0.yOffset = 0;
    /* Task=GAME alone does not prove that map pixels are installed. */
    gMapBottom.bgSettings = NULL;
    Port_Widescreen_UpdateShadows();
    CHECK(geometry->world.width == 374 && Port_PPU_VisibleFrameWidth() == 240);
    CHECK(!(geometry->scene.capabilities & PORT_SCENE_MAP_LIVE));
    gMapBottom.bgSettings = &gScreen.bg1;
    /* An installed fixed-screen subtask must not inherit a map-backed wide
     * raster merely because its BG controls happen to match the old room. */
    const int fixedSubtasks[] = {SUBTASK_PAUSEMENU, SUBTASK_MAPHINT, SUBTASK_KINSTONEMENU,
                                 SUBTASK_FIGURINEMENU, SUBTASK_FASTTRAVEL, SUBTASK_LOCALMAPHINT};
    gMain.state = GAMETASK_MAIN;
    gMain.substate = GAMEMAIN_SUBTASK;
    for (unsigned i = 0; i < sizeof(fixedSubtasks) / sizeof(fixedSubtasks[0]); ++i) {
        gUI.lastState = fixedSubtasks[i];
        for (int phase = 0; phase <= 4; ++phase) {
            gUI.nextToLoad = phase;
            Port_Widescreen_PublishScene();
            CHECK(geometry->world.width == ((phase == 2 || phase == 3) ? 240 : 374));
            CHECK(Port_PPU_VisibleFrameWidth() == ((phase == 2 || phase == 3) ? 240 : 374));
            CHECK(!!(geometry->scene.capabilities & PORT_SCENE_FIXED_SCREEN) == (phase == 2 || phase == 3));
        }
    }
    /* Subtask installation and exit can occur while room scrolling is idle. */
    gUI.lastState = SUBTASK_FIGURINEMENU;
    gUI.nextToLoad = 2;
    Port_Widescreen_BeforePresent();
    CHECK(geometry->scene.capabilities & PORT_SCENE_FIXED_SCREEN);
    CHECK(Port_PPU_VisibleFrameWidth() == 240);
    gMain.substate = GAMEMAIN_UPDATE;
    Port_Widescreen_BeforePresent();
    CHECK(!(geometry->scene.capabilities & PORT_SCENE_FIXED_SCREEN));
    gMain.substate = GAMEMAIN_SUBTASK;
    gUI.lastState = SUBTASK_PAUSEMENU;
    gUI.nextToLoad = 2;
    Port_Widescreen_PublishScene();
    gUI.lastState = SUBTASK_AUXCUTSCENE;
    gUI.nextToLoad = 1;
    Port_Widescreen_PublishScene();
    CHECK(geometry->scene.capabilities & PORT_SCENE_FIXED_SCREEN);
    gUI.nextToLoad = 2;
    Port_Widescreen_PublishScene();
    CHECK(!(geometry->scene.capabilities & PORT_SCENE_FIXED_SCREEN));
    gUI.lastState = SUBTASK_AUXCUTSCENE;
    gUI.nextToLoad = 2;
    gMenu.menuType = gMenu.field_0x0 = 1;
    gMenu.field_0x3 = 0;
    gMapBottom.bgSettings = NULL;
    Port_Widescreen_PublishScene();
    CHECK(geometry->scene.capabilities & PORT_SCENE_FIXED_SCREEN);
    gMenu.field_0x3 = 2;
    Port_Widescreen_PublishScene();
    CHECK(geometry->scene.capabilities & PORT_SCENE_FIXED_SCREEN);
    gMapBottom.bgSettings = &gScreen.bg1;
    Port_Widescreen_PublishScene();
    CHECK(!(geometry->scene.capabilities & PORT_SCENE_FIXED_SCREEN));
    gMain.substate = GAMEMAIN_UPDATE;
    gUI.nextToLoad = 0;
    for (int action = 2; action <= 5; ++action) {
        if (action == 3) continue;
        gRoomControls.scrollAction = action;
        Port_Widescreen_UpdateShadows();
        CHECK(geometry->scene.capabilities & PORT_SCENE_NATIVE_TRANSITION);
        CHECK(geometry->world.width == 240 && Port_PPU_VisibleFrameWidth() == 240);
        gRoomControls.scrollAction = 0;
        Port_Widescreen_PublishScene();
        CHECK(geometry->world.width == 374);
        CHECK(Port_Widescreen_CameraRestX(1000) == 650);
    }
    /* Origins that collided under the previous XOR room key are distinct. */
    gRoomControls.origin_x = 0; gRoomControls.origin_y = 2;
    uint64_t roomKey = Port_WidescreenRoomKey();
    gRoomControls.origin_x = 4; gRoomControls.origin_y = 0;
    CHECK(Port_WidescreenRoomKey() != roomKey);

    /* Tall live canvas: layout follows the canvas even when the room aperture
     * stays 240x160, and standard dialogue is published with a bottom anchor. */
    CHECK(Port_Viewport_Publish(PORT_ASPECT_DS_4_3, false, 569, 192, 569));
    CHECK(geometry->canvas.width == 256 && geometry->canvas.height == 192);
    gMain.task = TASK_GAME;
    gMain.substate = GAMEMAIN_UPDATE;
    gUI.nextToLoad = 0;
    sWsContentPx = 0;
    gRoomControls.width = 240;
    gRoomControls.height = 160;
    gMapBottom.bgSettings = NULL;
    PortViewportScene tallLive = { PORT_SCENE_WORLD | PORT_SCENE_WIDE_WORLD | PORT_SCENE_MAP_LIVE, 240, 160 };
    Port_Viewport_PublishScene(tallLive);
    CHECK(geometry->world.width == 240 && geometry->world.height == 160);
    CHECK(Port_Widescreen_LayoutActive());
    CHECK(Port_Widescreen_LayoutWidth() == 256 && Port_Widescreen_LayoutHeight() == 192);
    /* A narrow room's native world fallback does not shrink the corner layout. */
    PortViewportScene nativeContent = { PORT_SCENE_WORLD | PORT_SCENE_NATIVE_CONTENT | PORT_SCENE_MAP_LIVE, 240, 160 };
    Port_Viewport_PublishScene(nativeContent);
    CHECK(Port_Widescreen_LayoutActive());
    Port_Viewport_PublishScene(tallLive);
    CHECK(Port_Widescreen_LayoutActive());
    /* A classified effect/window uses one native composition, not the canvas
     * HUD layout: no half-rendered world rows and no artwork in the margins. */
    gScreen.lcd.displayControl |= DISPCNT_WIN0_ON;
    Port_Widescreen_PublishScene();
    CHECK((geometry->scene.capabilities & PORT_SCENE_NATIVE_FALLBACK) != 0);
    CHECK(!Port_Widescreen_LayoutActive());
    CHECK(Port_Viewport_RasterHeight(geometry) == 160);
    gScreen.lcd.displayControl &= ~DISPCNT_WIN0_ON;
    gMapBottom.bgSettings = &gScreen.bg1; /* publish a map-backed live scene */
    Port_Widescreen_PublishScene();
    CHECK(!(geometry->scene.capabilities & PORT_SCENE_NATIVE_FALLBACK));
    CHECK(Port_Widescreen_LayoutActive());
    gHUD.hideFlags = 0; gHUD.unk_10 = 2; gHUD.unk_a = 2;
    CHECK(Port_Widescreen_HudRightAnchor() == (MODE1_WS_HUD_KEYS | MODE1_WS_HUD_RUPEES));
    gHUD.buttonX[0] = 208;
    CHECK(HudButtonTargetX(0) == 224);
    gTextRender.message.textWindowWidth = 26;
    gTextRender.message.textWindowHeight = 4;
    gTextRender.message.textWindowPosX = 1;
    gTextRender.message.textWindowPosY = 12;
    gCurrentWindow.active = 1;
    gCurrentWindow.xPos = 1; gCurrentWindow.yPos = 12;
    gCurrentWindow.width = 26; gCurrentWindow.height = 4;
    Port_Widescreen_UpdateShadows();
    CHECK(virtuappu_mode1_ws_msg_x0 == 8 && virtuappu_mode1_ws_msg_x1 == 232);
    CHECK(virtuappu_mode1_ws_msg_y0 == 96 && virtuappu_mode1_ws_msg_y1 == 144);
    CHECK(virtuappu_mode1_ws_msg_shift == 8 && virtuappu_mode1_ws_msg_shift_y == 32);
    CHECK(virtuappu_mode1_ws_hud_shift_y == 32);
    /* Top-requested dialogue stays at the native top on every canvas. */
    gTextRender.message.textWindowPosY = 1;
    gCurrentWindow.yPos = 1;
    Port_Widescreen_UpdateShadows();
    CHECK(virtuappu_mode1_ws_msg_y0 == 8 && virtuappu_mode1_ws_msg_y1 == 56);
    CHECK(virtuappu_mode1_ws_msg_shift_y == 0);
    /* Custom window geometry keeps only the tested horizontal policy. */
    gTextRender.message.textWindowPosY = 2;
    gCurrentWindow.yPos = 2;
    Port_Widescreen_UpdateShadows();
    CHECK(virtuappu_mode1_ws_msg_shift == 8 && virtuappu_mode1_ws_msg_shift_y == 0);
    /* Opening/closing publishes the current animated rect with the final
     * translation, so the destination center stays fixed. */
    gTextRender.message.textWindowPosY = 12;
    gCurrentWindow.xPos = 13; gCurrentWindow.yPos = 14;
    gCurrentWindow.width = 2; gCurrentWindow.height = 0;
    Port_Widescreen_UpdateShadows();
    CHECK(virtuappu_mode1_ws_msg_y0 == 112 && virtuappu_mode1_ws_msg_y1 == 128);
    CHECK(virtuappu_mode1_ws_msg_shift_y == 32);
    CHECK(virtuappu_mode1_ws_msg_y0 + virtuappu_mode1_ws_msg_shift_y == 144);
    CHECK(virtuappu_mode1_ws_msg_y1 + virtuappu_mode1_ws_msg_shift_y == 160);
    /* Location banners stay on their independent top-center rule. */
    gCurrentWindow.active = 0;
    testBanner = (Entity*)&(Entity){ .action = 1 };
    gScreen.bg0.yOffset = 0;
    Port_Widescreen_UpdateShadows();
    CHECK(virtuappu_mode1_ws_msg_y0 == 40 && virtuappu_mode1_ws_msg_y1 == 56);
    CHECK(virtuappu_mode1_ws_msg_shift == 8 && virtuappu_mode1_ws_msg_shift_y == 0);
    testBanner = NULL;
    /* Fixed screens retain their native composition and publish no anchors. */
    gMain.state = GAMETASK_MAIN;
    gMain.substate = GAMEMAIN_SUBTASK;
    gUI.lastState = SUBTASK_PAUSEMENU;
    gUI.nextToLoad = 2;
    Port_Widescreen_UpdateShadows();
    CHECK(geometry->scene.capabilities & PORT_SCENE_FIXED_SCREEN);
    CHECK(!Port_Widescreen_LayoutActive());
    gCurrentWindow.active = 1;
    gTextRender.message.textWindowPosY = 12;
    gCurrentWindow.yPos = 12;
    Port_Widescreen_UpdateShadows();
    CHECK(virtuappu_mode1_ws_msg_shift == 0 && virtuappu_mode1_ws_msg_shift_y == 0);
    CHECK(virtuappu_mode1_ws_hud_shift_y == 0 && virtuappu_mode1_ws_hud_right_anchor == 0);
    gCurrentWindow.active = 0;
    if (!errors) puts("widescreen engine: scene publication, frozen presentation, room aperture, transitions and shadows passed");
    return errors != 0;
}
