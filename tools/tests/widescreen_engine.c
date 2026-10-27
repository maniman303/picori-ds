/* Small fixture for the unchanged production helper bodies extracted by the runner. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "port_viewport.h"
typedef uint8_t u8;
typedef int16_t s16;
typedef int32_t s32;
typedef uint16_t u16;
typedef uint32_t u32;
#define MODE1_GBA_WIDTH 384
#define MODE1_GBA_BG_CLIP_X 240
#define MODE1_WS_SHADOW_COLS 32
#define MODE1_WS_SHADOW_ROWS 32
static struct { int scroll_x, scroll_y, origin_x, origin_y, width, height, scrollAction, area, room;
                int aff_y, shake_duration, shake_magnitude, reload_flags; } gRoomControls;
static struct { unsigned active, xPos, yPos, width, height; } gCurrentWindow;
static int virtuappu_mode1_ws_shadow_base_tile[4];
static u16* virtuappu_mode1_ws_shadow[4];
static uint64_t sWsContentKey = UINT64_MAX;
static int sWsContentPx, sWsStageHeight, sWsManagerDirty, sWsEntityDirty;
static u16 gOamMem[512];
static int Port_Widescreen_HasDynamicEntities(void) { return 0; }
static bool Port_Oam_VisibleSlotsTagged(const u16* p) { (void)p; return true; }
static uint64_t sWsValidatedKey = UINT64_MAX;
static PortViewportUpload sWsUpload;
static u8 gShakeOffsets[256], gManagerCount, gUnk_03003DE0, gUpdateVisibleTiles;
static int Port_Widescreen_MapTileX(void) { return (u16)(gRoomControls.scroll_x - gRoomControls.origin_x) >> 4; }
static int Port_Widescreen_MapTileY(void) { return (u16)(gRoomControls.scroll_y - gRoomControls.origin_y) >> 4; }
static unsigned Port_Widescreen_MapBufferSize(void) { return 27 * 64; }
static void UpdateScrollVram(void) {}
void Port_Widescreen_PublishScene(void);
int Port_Widescreen_EffectiveViewWidth(void);
/* Only unrelated task/HUD dependencies are stubbed; shadow publication runs
 * the production UpdateShadows body, including its area and BG gates. */
enum { TASK_GAME = 2, GAMETASK_MAIN = 2, GAMEMAIN_UPDATE = 2, AREA_MINISH_WOODS = 0x0b, AREA_HYRULE_FIELD = 0x0a,
       AREA_CAVE_OF_FLAMES = 0x60, MANAGER = 9, ENTER_ROOM_TEXTBOX_MANAGER = 1 };
#define MODE1_GBA_BG_COUNT 4
#define DISPCNT_BG3_ON 0x800
static struct { int task, state, substate; } gMain;
typedef struct { u32 control; int yOffset; } TestBg;
static struct { TestBg bg0, bg1, bg2, bg3; struct { int displayControl; } lcd; } gScreen;
static struct { TestBg* bgSettings; } gMapBottom, gMapTop;
static u16 gMapDataBottomSpecial[128*128], gMapDataTopSpecial[128*128];
static u16 sWsShadowBG1[32*32], sWsShadowBG2[32*32], sWsShadowOverlay[32*32];
static u16 vram[0x18000/2];
#define gVram ((unsigned char*)vram)
static int virtuappu_mode1_ws_hud_right_anchor, virtuappu_mode1_ws_msg_shift;
static int virtuappu_mode1_ws_msg_x0, virtuappu_mode1_ws_msg_x1;
static int virtuappu_mode1_ws_msg_y0, virtuappu_mode1_ws_msg_y1;
static int Port_Widescreen_HudRightAnchor(void) { return 0; }
typedef struct { int action; } Entity;
static Entity* FindEntityByID(int kind, int id, int list) { (void)kind; (void)id; (void)list; return NULL; }
#include "widescreen_helpers.h"
static int errors;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); ++errors; } } while (0)
int main(void) {
    gMain.task = TASK_GAME;
    Port_Viewport_Publish(PORT_ASPECT_ULTRAWIDE_21_9, false, 384, 160, 384);
    gRoomControls.width = 1024;
    gRoomControls.height = 1024;
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
    CHECK(Port_PPU_VisibleFrameWidth() == 256);
    uint32_t generation = geometry->generation;
    Port_Widescreen_PublishScene();
    CHECK(geometry->generation == generation);
    gRoomControls.width = 1024;
    gMain.task = 0;
    CHECK(Port_PPU_VisibleFrameWidth() == 256);
    CHECK(Port_Widescreen_EffectiveViewWidth() == 256);
    CHECK(geometry->generation == generation);
    Port_Widescreen_PublishScene();
    CHECK(Port_PPU_VisibleFrameWidth() == 240 && geometry->world.width == 240);
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
    /* Task=GAME alone does not prove that map pixels are installed. */
    gMapBottom.bgSettings = NULL;
    Port_Widescreen_UpdateShadows();
    CHECK(geometry->world.width == 374 && Port_PPU_VisibleFrameWidth() == 240);
    CHECK(!(geometry->scene.capabilities & PORT_SCENE_MAP_LIVE));
    gMapBottom.bgSettings = &gScreen.bg1;
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
    if (!errors) puts("widescreen engine: scene publication, frozen presentation, room aperture, transitions and shadows passed");
    return errors != 0;
}
