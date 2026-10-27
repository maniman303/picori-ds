/* Integration contracts for actual production bodies; no ROM/SDL required. */
#include "global.h"
#include "main.h"
#include "player.h"
#include "room.h"
#include "screen.h"
#include "scroll.h"
#include "script.h"
#include "port_widescreen.h"
#include "cpu/mode1.h"
#include "virtuappu.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

RoomControls gRoomControls;
PlayerEntity gPlayerEntity;
PlayerState gPlayerState;
RoomTransition gRoomTransition;
Screen gScreen;
ActiveScriptInfo gActiveScriptInfo;
u8 gUpdateVisibleTiles;
u8 gShakeOffsets[256];
static int viewWidth = 240, viewHeight = 160;
static int loads, shakes, shakeFills;
static int priorityClears;
#if MODE1_GBA_WIDTH > 240
static int sWsStageHeight;
#endif
void UpdateScrollVram(void) { ++shakeFills; }
void ClearEventPriority(void) { ++priorityClears; }
void MemFill32(u32 value, void* dest, u32 size) {
    u32* words = dest;
    for (u32 i = 0; i < size / sizeof(*words); ++i) words[i] = value;
}

/* Explicit test-only aperture injection. The runtime height gate stays 160
 * while the independent camera/map paths are checked at future DS geometry. */
int Port_Widescreen_EffectiveViewWidth(void) { return viewWidth; }
int Port_Widescreen_EffectiveViewHeight(void) { return viewHeight; }
void Port_Widescreen_PublishScene(void) {}
void Port_Widescreen_BeforePresent(void) {}
void Port_Widescreen_RoomLoaded(void) {}
void LoadRoomTileSet(void) { ++loads; }
void LoadRoomGfx(void) { ++loads; }
void UpdateScreenShake(void) { ++shakes; }
void UpdateSpriteForCollisionLayer(Entity* entity) { (void)entity; }
void sub_080809D4(void);
void sub_0807FEC8(RoomControls* controls);
#include "world_view_functions.inc"

static void check_helpers(void) {
    assert(Port_Viewport_CameraRest(400, 100, 800, 192) == 304);
    assert(Port_Viewport_CameraRest(1000, 100, 800, 192) == 708);
    assert(Port_Viewport_CameraRest(0, 100, 800, 192) == 100);
    assert(Port_Viewport_CameraRest(400, 100, 160, 192) == 100);
    assert(Port_Viewport_CameraRest(400, 100, 192, 192) == 100);
    assert(Port_Viewport_CameraRest(500, -300, 1024, 285) == 358);
    assert(Port_Viewport_CameraRest(INT_MIN, INT_MIN, INT_MAX, 192) == INT_MIN);
    assert(Port_Viewport_CameraRest(INT_MAX, INT_MAX, INT_MAX, 192) == INT_MAX);
    assert(Port_Viewport_CameraRest(INT_MAX, 0, INT_MAX, 160) == INT_MAX - 160);
    assert(Port_Viewport_CameraRest(200, 100, 0, 192) == 100);
    assert(Port_Viewport_CameraRest(200, 100, 800, 0) == 100);
    assert(Port_Viewport_MapTileRows(160) == 23);
    assert(Port_Viewport_MapTileRows(192) == 27);
    assert(Port_Viewport_MapTileRows(232) == 32);
    assert(Port_Viewport_MapTileRows(233) == 32);
    assert(Port_Viewport_MapTileRows(234) == 0);
    assert(Port_Viewport_MapTileRows(INT_MAX) == 0);
    assert(Port_Viewport_MapTileRows(-1) == 0);
    assert(Port_Viewport_MapTileRowsWithShake(192, -8) == 27);
    assert(Port_Viewport_MapTileRowsWithShake(192, -9) == 0);
    assert(Port_Viewport_MapTileRowsWithShake(192, 3) == 28);
    assert(Port_Viewport_MapTileRowsWithShake(192, 8) == 28);
    assert(Port_Viewport_MapTileRowsWithShake(192, 41) == 32);
    assert(Port_Viewport_MapTileRowsWithShake(192, 42) == 0);
    assert(Port_Viewport_MapTileRowsWithShake(192, INT_MIN) == 0);
    assert(Port_Viewport_MapTileRowsWithShake(192, INT_MAX) == 0);
    uint8_t shake[16] = {0};
    assert(Port_Viewport_ShakeSequenceFits(192, shake));
    shake[15] = (uint8_t)-8;
    assert(Port_Viewport_ShakeSequenceFits(192, shake));
    shake[15] = (uint8_t)-9;
    assert(!Port_Viewport_ShakeSequenceFits(192, shake));
    shake[15] = 41;
    assert(Port_Viewport_ShakeSequenceRows(192, shake) == 32);
    assert(Port_Viewport_ShakeSequenceFits(192, shake));
    shake[15] = 42;
    assert(!Port_Viewport_ShakeSequenceFits(192, shake));
    assert(!Port_Viewport_ShakeSequenceFits(192, NULL));
    memset(gShakeOffsets, 0, sizeof(gShakeOffsets));
    gShakeOffsets[15] = 8;
    viewHeight = 192;
    gRoomControls.shake_duration = 1;
    assert(Port_Widescreen_MapBufferSize() == 28 * 32 * sizeof(u16));
    gRoomControls.shake_duration = 0;
}

static void check_shake_start(void) {
    gRoomControls.scrollAction = 1;
    viewHeight = 160; gUpdateVisibleTiles = 0; shakeFills = 0;
    InitScreenShake(12, 3);
    assert(gRoomControls.shake_duration == 12 && gRoomControls.shake_magnitude == 3);
    assert(gUpdateVisibleTiles == 0 && shakeFills == 0);
    viewHeight = 192;
    InitScreenShake(12, 3);
    assert(gUpdateVisibleTiles == 1 && shakeFills == 1);
    gRoomControls.scrollAction = 2; gUpdateVisibleTiles = 2;
    InitScreenShake(12, 3);
    assert(gUpdateVisibleTiles == 2 && shakeFills == 1);
    gRoomControls.shake_duration = 0;
}

static void check_cameras(void) {
    const int widths[] = { 240, 256, 285, 374 };
    const int heights[] = { 160, 192 };
    for (unsigned w = 0; w < sizeof(widths) / sizeof(widths[0]); ++w) {
        viewWidth = widths[w];
        if (viewWidth > MODE1_GBA_WIDTH) continue;
        for (unsigned h = 0; h < sizeof(heights) / sizeof(heights[0]); ++h) {
            viewHeight = heights[h];
            const int targets[][2] = { { 100, 200 }, { 500, 600 }, { 1100, 900 } };
            for (unsigned t = 0; t < sizeof(targets) / sizeof(targets[0]); ++t) {
                memset(&gRoomControls, 0, sizeof(gRoomControls));
                gRoomControls.origin_x = 100;
                gRoomControls.origin_y = 200;
                gRoomControls.width = 1024;
                gRoomControls.height = 768;
                gRoomControls.camera_target = &gPlayerEntity.base;
                gRoomControls.scrollSpeed = 4;
                gPlayerEntity.base.x.HALF.HI = targets[t][0];
                gPlayerEntity.base.y.HALF.HI = targets[t][1];
                gRoomControls.scroll_x = 300;
                gRoomControls.scroll_y = 400;
                const int restX = Port_Widescreen_CameraRestX(targets[t][0]);
                const int restY = Port_Widescreen_CameraRestY(targets[t][1]);
                for (int frame = 0; frame < 512; ++frame) {
                    int x = gRoomControls.scroll_x, y = gRoomControls.scroll_y;
                    gRoomControls.scroll_flags &= ~4;
                    Scroll1(&gRoomControls);
                    assert(gRoomControls.scroll_x - x <= 4 && x - gRoomControls.scroll_x <= 4);
                    assert(gRoomControls.scroll_y - y <= 4 && y - gRoomControls.scroll_y <= 4);
                }
                assert(gRoomControls.scroll_x == restX && gRoomControls.scroll_y == restY);
                ScriptExecutionContext context = {0};
                gActiveScriptInfo.flags = 0;
                WaitForCameraTouchRoomBorder(NULL, &context);
                assert(gActiveScriptInfo.flags & 1);
                --gRoomControls.scroll_y;
                gActiveScriptInfo.flags = 0;
                gActiveScriptInfo.commandSize = 7;
                WaitForCameraTouchRoomBorder(NULL, &context);
                assert(!(gActiveScriptInfo.flags & 1) && gActiveScriptInfo.commandSize == 0);
                sub_080809D4();
                assert(gRoomControls.scroll_x == restX && gRoomControls.scroll_y == restY);
                sub_08080974(targets[t][0], targets[t][1]);
                assert(gRoomControls.scroll_x == restX && gRoomControls.scroll_y == restY);
            }
            // Room initialization starts with room-relative target coordinates.
            gPlayerEntity.base.x.HALF.HI = 300;
            gPlayerEntity.base.y.HALF.HI = 400;
            loads = shakes = 0;
            InitializeCamera();
            assert(loads == 2 && shakes == 1);
            assert(gPlayerEntity.base.x.HALF.HI == 400 && gPlayerEntity.base.y.HALF.HI == 600);
            assert(gRoomControls.scroll_x == Port_Widescreen_CameraRestX(400));
            assert(gRoomControls.scroll_y == Port_Widescreen_CameraRestY(600));
        }
    }
    // Preserve native edge-step flags and tile refresh timing.
    viewWidth = 240;
    viewHeight = 160;
    gRoomControls.scroll_x = Port_Widescreen_CameraRestX(gPlayerEntity.base.x.HALF.HI);
    gRoomControls.scroll_y = gRoomControls.origin_y + 2;
    gPlayerEntity.base.y.HALF.HI = gRoomControls.origin_y;
    gRoomControls.scroll_flags = 0;
    gUpdateVisibleTiles = 0;
    Scroll1(&gRoomControls);
    assert(gRoomControls.scroll_y == gRoomControls.origin_y);
    assert((gRoomControls.scroll_flags & 4) && gUpdateVisibleTiles == 1);
    // A taller aperture must recover from the old, lower native rest limit.
    viewHeight = 192;
    gPlayerEntity.base.y.HALF.HI = 900;
    gRoomControls.scroll_y = 808;
    for (int frame = 0; frame < 16; ++frame) Scroll1(&gRoomControls);
    assert(gRoomControls.scroll_y == 776);

    // A target in a room smaller than the aperture must pin to its origin;
    // script completion must agree rather than waiting for a negative limit.
    viewWidth = 240;
    viewHeight = 192;
    gRoomControls.width = 224;
    gRoomControls.height = 160;
    sub_080809D4();
    assert(gRoomControls.scroll_x == gRoomControls.origin_x);
    assert(gRoomControls.scroll_y == gRoomControls.origin_y);
    gActiveScriptInfo.flags = 0;
    ScriptExecutionContext context = {0};
    WaitForCameraTouchRoomBorder(NULL, &context);
    assert(gActiveScriptInfo.flags & 1);
}

static void check_scroll_transition_completion(void) {
    for (int scenario = 0; scenario < (MODE1_GBA_WIDTH > 240 ? 2 : 1); ++scenario) {
        for (int direction = 0; direction < 4; ++direction) {
            memset(&gRoomControls, 0, sizeof(gRoomControls));
            memset(&gPlayerState, 0, sizeof(gPlayerState));
            memset(&gPlayerEntity, 0, sizeof(gPlayerEntity));
            gRoomControls.origin_x = 100;
            gRoomControls.origin_y = 200;
            gRoomControls.width = 1024;
            gRoomControls.height = 768;
            gRoomControls.scrollAction = 2;
            gRoomControls.scroll_direction = direction;
            gRoomControls.camera_target = &gPlayerEntity.base;
            gPlayerEntity.base.x.HALF.HI = 500;
            gPlayerEntity.base.y.HALF.HI = 600;
            viewWidth = MODE1_GBA_WIDTH > 240 && scenario == 0 ? 285 : 240;
            viewHeight = scenario == 1 ? 192 : 160;
            const int frames = direction % 2 ? 60 : 40;
            for (int frame = 0; frame < frames; ++frame) {
                Scroll2Sub2(&gRoomControls);
                assert(gRoomControls.scrollAction == (frame + 1 == frames ? 0 : 2));
            }
            assert(gPlayerState.startPosX == gPlayerEntity.base.x.HALF.HI);
            assert(gPlayerState.startPosY == gPlayerEntity.base.y.HALF.HI);
            if (viewWidth > 240 || viewHeight > 160) {
                assert(gRoomControls.scroll_x == Port_Widescreen_CameraRestX(gPlayerEntity.base.x.HALF.HI));
                assert(gRoomControls.scroll_y == Port_Widescreen_CameraRestY(gPlayerEntity.base.y.HALF.HI));
                assert(gUpdateVisibleTiles == 1);
            }
        }
    }
}

/* Once the black-fade arrival has published its staged height, the real
 * camera follow and script wait must agree on the new two-axis rest point. */
static void check_black_fade_arrival_camera(void) {
    memset(&gRoomControls, 0, sizeof(gRoomControls));
    memset(&gPlayerEntity, 0, sizeof(gPlayerEntity));
    gRoomTransition.type = TRANSITION_FADE_BLACK;
    gRoomControls.origin_x = 100;
    gRoomControls.origin_y = 200;
    gRoomControls.width = 1024;
    gRoomControls.height = 768;
    gRoomControls.scrollAction = 1;
    gRoomControls.scrollSpeed = 4;
    gRoomControls.camera_target = &gPlayerEntity.base;
    gPlayerEntity.base.x.HALF.HI = 500;
    gPlayerEntity.base.y.HALF.HI = 900;
    viewWidth = MODE1_GBA_WIDTH > 240 ? 256 : 240;
    viewHeight = 192;
    gRoomControls.scroll_x = 380;
    gRoomControls.scroll_y = 808; /* old 160-row bottom clamp */
    const int targetX = Port_Widescreen_CameraRestX(gPlayerEntity.base.x.HALF.HI);
    const int targetY = Port_Widescreen_CameraRestY(gPlayerEntity.base.y.HALF.HI);
    ScriptExecutionContext context = {0};
    gActiveScriptInfo.flags = 0;
    WaitForCameraTouchRoomBorder(NULL, &context);
    assert(!(gActiveScriptInfo.flags & 1));
    for (int i = 0; i < 64; ++i) Scroll1(&gRoomControls);
    assert(gRoomControls.scroll_x == targetX && gRoomControls.scroll_y == targetY);
    gActiveScriptInfo.flags = 0;
    WaitForCameraTouchRoomBorder(NULL, &context);
    assert(gActiveScriptInfo.flags & 1);
}

static void check_scroll4_transition_completion(void) {
    for (int scenario = 0; scenario < (MODE1_GBA_WIDTH > 240 ? 2 : 1); ++scenario) {
        memset(&gRoomControls, 0, sizeof(gRoomControls));
        memset(&gPlayerEntity, 0, sizeof(gPlayerEntity));
        gRoomControls.origin_x = 100;
        gRoomControls.origin_y = 200;
        gRoomControls.width = 1024;
        gRoomControls.height = 768;
        gRoomControls.scrollAction = 4;
        gRoomControls.camera_target = &gPlayerEntity.base;
        gPlayerEntity.base.x.HALF.HI = 500;
        gPlayerEntity.base.y.HALF.HI = 600;
        viewWidth = MODE1_GBA_WIDTH > 240 && scenario == 0 ? 285 : 240;
        viewHeight = scenario == 1 ? 192 : 160;
        priorityClears = 0;
        for (int frame = 0; frame < 40; ++frame) {
            gRoomTransition.frameCount = frame;
            Scroll4Sub1(&gRoomControls);
            assert(gRoomControls.scrollAction == (frame >= 38 ? 0 : 4));
            assert(priorityClears == (frame >= 38 ? 1 : 0));
            if (frame == 38) break;
        }
        if (viewWidth > 240 || viewHeight > 160) {
            assert(gRoomControls.scroll_x == Port_Widescreen_CameraRestX(gPlayerEntity.base.x.HALF.HI));
            assert(gRoomControls.scroll_y == Port_Widescreen_CameraRestY(gPlayerEntity.base.y.HALF.HI));
            assert(gUpdateVisibleTiles == 1);
        }
    }
}

static void check_scroll5_transition_completion(void) {
    for (int scenario = 0; scenario < (MODE1_GBA_WIDTH > 240 ? 2 : 1); ++scenario) {
        memset(&gRoomControls, 0, sizeof(gRoomControls));
        memset(&gPlayerEntity, 0, sizeof(gPlayerEntity));
        memset(&gScreen, 0, sizeof(gScreen));
        gRoomControls.origin_x = 100;
        gRoomControls.origin_y = 200;
        gRoomControls.width = 1024;
        gRoomControls.height = 768;
        gRoomControls.scrollAction = 5;
        gRoomControls.camera_target = &gPlayerEntity.base;
        gPlayerEntity.base.x.HALF.HI = 500;
        gPlayerEntity.base.y.HALF.HI = 600;
        gScreen.lcd.displayControl = DISPCNT_WIN1_ON;
        viewWidth = MODE1_GBA_WIDTH > 240 && scenario == 0 ? 285 : 240;
        viewHeight = scenario == 1 ? 192 : 160;
        priorityClears = 0;
        Scroll5Sub5(&gRoomControls);
        assert(gRoomControls.scrollAction == 0 && priorityClears == 1);
        assert(!(gScreen.lcd.displayControl & DISPCNT_WIN1_ON));
        if (viewWidth > 240 || viewHeight > 160) {
            assert(gRoomControls.scroll_x == Port_Widescreen_CameraRestX(gPlayerEntity.base.x.HALF.HI));
            assert(gRoomControls.scroll_y == Port_Widescreen_CameraRestY(gPlayerEntity.base.y.HALF.HI));
            assert(gUpdateVisibleTiles == 1);
        }
    }
}

static void check_world_pixels(void) {
    static u16 map[128 * 128], buffer[32 * 32];
    static u16 shadow[MODE1_WS_SHADOW_ROWS * MODE1_WS_SHADOW_COLS];
    static u8 io[0x400], vram[0x18000];
    static u16 bgpal[256], objpal[256], oam[512];
    const uint32_t colors[3] = { 0xff0000f8, 0xff00f800, 0xfff80000 };
    VirtuaPPUMode1GbaMemory memory = { io, vram, bgpal, objpal, oam };
    virtuappu_mode1_bind_gba_memory(&memory);
    virtuappu_mode1_pre_line_callback = NULL;
    io[1] = 2; // BG1, screenbase 30, charbase 0
    io[0xb] = 0x1e;
    bgpal[1] = 0x1f;
    bgpal[2] = 0x3e0;
    bgpal[3] = 0x7c00;
    for (int tile = 1; tile <= 3; ++tile) memset(vram + tile * 32, tile * 0x11, 32);
    for (int row = 0; row < 128; ++row)
        for (int col = 0; col < 128; ++col)
            map[row * 128 + col] = (u16)(1 + (row + col) % 3);
    gRoomControls.origin_x = gRoomControls.origin_y = 0;
    gRoomControls.width = gRoomControls.height = 1024;
    viewHeight = 192;
    PPUMemory p = {0};
    p.frame_width = MODE1_GBA_WIDTH >= 256 ? 256 : 240;
    p.frame_height = 192;
    p.frame_pitch = MODE1_GBA_WIDTH;
    p.mode = 1;
    // Normal VRAM columns and bottom/right shadow columns must agree with
    // independent room-pixel coordinates for every fractional camera phase.
    const int shakeOffsets[] = { -8, -3, 0, 3, 8 };
    for (unsigned shake = 0; shake < sizeof(shakeOffsets) / sizeof(shakeOffsets[0]); ++shake) {
        gRoomControls.aff_y = shakeOffsets[shake];
        for (int sy = 16; sy < 32; ++sy) {
            for (int sx = 16; sx < 32; ++sx) {
                gRoomControls.scroll_x = sx;
                gRoomControls.scroll_y = sy;
                ram_sub_080B197C_c(map, buffer + 32);
                memcpy(vram + 0xf000, buffer, Port_Widescreen_MapBufferSize());
                Port_WidescreenShadow_Populate(1, map, shadow);
                io[0x14] = sx & 15;
                io[0x16] = (sy & 15) + 8 + shakeOffsets[shake];
                virtuappu_mode1_render_frame(&p);
                const int lines[] = { 0, 159, 160, 191 };
                const int columns[] = { 0, 239, 240, 255 };
                for (unsigned y = 0; y < sizeof(lines) / sizeof(lines[0]); ++y) {
                    for (unsigned x = 0; x < sizeof(columns) / sizeof(columns[0]); ++x) {
                        if (columns[x] >= p.frame_width) continue;
                        int color = ((sy + lines[y] + shakeOffsets[shake]) / 8 + (sx + columns[x]) / 8) % 3;
                        assert(virtuappu_frame_buffer[lines[y] * MODE1_GBA_WIDTH + columns[x]] == colors[color]);
                    }
                }
            }
        }
    }
    gRoomControls.aff_y = 42;
    assert(Port_Widescreen_MapBufferSize() == 0x800); // bounded backing, not supported coverage
    viewHeight = 160;
    assert(Port_Widescreen_MapBufferSize() == 0x5c0); // preserve native footprint even during shake
    gRoomControls.aff_y = 0;
}

static void check_regions(void) {
    viewWidth = 240;
    gRoomControls.origin_x = 100;
    gRoomControls.origin_y = 200;
    gRoomControls.scroll_x = 132;
    gRoomControls.scroll_y = 264;
    viewHeight = 160;
    assert(!CheckRegionOnScreen(48, 240, 8, 8));
    viewHeight = 192;
    assert(CheckRegionOnScreen(48, 240, 8, 8));
    assert(!CheckRegionOnScreen(48, 272, 8, 8));
}

static void check_map_fill(void) {
    static u16 map[128 * 128], buffer[32 * 32];
    for (unsigned i = 0; i < sizeof(map) / sizeof(map[0]); ++i) map[i] = (u16)(i + 1);
    gRoomControls.origin_x = gRoomControls.origin_y = 0;
    for (int height = 160; height <= 192; height += 32) {
        viewHeight = height;
        const unsigned rows = height == 160 ? 23 : 27;
        assert(Port_Widescreen_MapBufferSize() == rows * 64);
        // All fractional scroll phases and the top-edge duplicated pad row.
        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 32; ++x) {
                for (unsigned i = 0; i < 32 * 32; ++i) buffer[i] = 0xbeef;
                gRoomControls.scroll_x = x;
                gRoomControls.scroll_y = y;
                ram_sub_080B197C_c(map, buffer + 32);
                for (unsigned row = 0; row < rows; ++row) {
                    int mapRow = (y / 16) * 2 + (int)row - 1;
                    if (row == 0 && y < 8) mapRow = 0;
                    for (int col = 0; col < 32; ++col) {
                        u16 expected = mapRow < 0 ? 0xbeef : map[mapRow * 128 + (x / 16) * 2 + col];
                        assert(buffer[row * 32 + col] == expected);
                    }
                }
                assert(buffer[rows * 32] == 0xbeef);
            }
        }
        // Deep and negative transition positions cannot read outside the map.
        const int positions[] = { 1016, -16 };
        for (unsigned p = 0; p < sizeof(positions) / sizeof(positions[0]); ++p) {
            for (unsigned i = 0; i < 32 * 32; ++i) buffer[i] = 0xbeef;
            gRoomControls.scroll_y = positions[p];
            ram_sub_080B197C_c(map, buffer + 32);
            if (height == 192) assert(buffer[26 * 32] == 0);
            assert(buffer[rows * 32] == 0xbeef);
        }
    }
}

int main(void) {
    check_helpers();
    check_shake_start();
    check_cameras();
    check_scroll_transition_completion();
    check_black_fade_arrival_camera();
    check_scroll4_transition_completion();
    check_scroll5_transition_completion();
    check_regions();
    check_map_fill();
    check_world_pixels();
    puts("World camera goals, completion, region bounds and 2D streamed pixels: PASS");
    return 0;
}
