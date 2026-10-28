#include "global.h"
#include "entity.h"
#include "main.h"
#include "player.h"
#include "structures.h"
#include "port_oam.h"
#include "port_voxel.h"
#include "port_widescreen.h"
#include "virtuappu.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

RoomControls gRoomControls;
PlayerEntity gPlayerEntity;
PlayerState gPlayerState;
OAMControls gOAMControls;
u32 gFrameObjLists[50016];
PortVoxelOamTag gPortVoxelOamTagsBuild[128];
static PortVoxelOamTag sVoxelCtx;
static int sRenderingPlayer;
static u8 sSizeTable[240];
static int viewWidth = 256, viewHeight = 192, voxel;
static u8 io[0x400], vram[0x18000];
static u16 bgpal[256], objpal[256], oam[512];
static const uint32_t green = 0xff00f800, red = 0xff0000f8;

int Port_Widescreen_EffectiveViewWidth(void) { return viewWidth; }
int Port_Widescreen_EffectiveViewHeight(void) { return viewHeight; }
bool Port_Voxel_IsDrawing(void) { return voxel != 0; }
u32 Port_FrameObjListsSizeForRegion(void) { return sizeof(gFrameObjLists); }
#include "oam_functions.inc"

static void reset(void) {
    Port_Oam_Invalidate();
    memset(&gOAMControls, 0, sizeof(gOAMControls));
    memset(io, 0, sizeof(io));
    memset(vram, 0, sizeof(vram));
    memset(oam, 0, sizeof(oam));
    memset(&gRoomControls, 0, sizeof(gRoomControls));
    memset(&gPlayerState, 0, sizeof(gPlayerState));
    memset(&sVoxelCtx, 0, sizeof(sVoxelCtx));
    for (int i = 0; i < 128; ++i) {
        ((u16*)&gOAMControls.oam[i])[0] = 0x2a0;
        oam[i * 4] = 0x2a0;
    }
    io[1] = 0x10; // OBJ enabled
    io[0] = 0x40; // one-dimensional OBJ character mapping
    bgpal[0] = 0x3e0;
    objpal[1] = 0x1f;
    memset(vram + 0x10000, 0x11, 0x8000);
    virtuappu_mode1_bind_obj_positions(NULL);
    virtuappu_mode1_pre_line_callback = NULL;
    viewWidth = 256;
    viewHeight = 192;
    voxel = 0;
    FlushSprites();
}

static void piece(int x, int y, unsigned flags, u8 shape) {
    u8* data = (u8*)gFrameObjLists;
    data[0] = 1;
    data[1] = data[2] = data[4] = data[5] = 0;
    data[3] = shape;
    RenderSpritePieces(data, (s16)x, (s16)y, flags, 0);
}

static void latch(void) {
    // Mirror the actual OAM DMA boundary; matrices/attributes are packed GBA
    // data, while position metadata remains separate and immutable to workers.
    memcpy(oam, gOAMControls.oam, sizeof(oam));
    Port_Oam_Latch(gOAMControls.updated);
}

static void render(bool metadata) {
    PPUMemory p = {0};
    p.frame_width = (u16)viewWidth;
    p.frame_height = (u16)viewHeight;
    p.frame_pitch = MODE1_GBA_WIDTH;
    p.mode = 1;
    virtuappu_mode1_bind_obj_positions(metadata ? Port_Oam_Positions() : NULL);
    virtuappu_mode1_render_frame(&p);
}

static uint32_t pixel(int x, int y) { return virtuappu_frame_buffer[y * MODE1_GBA_WIDTH + x]; }

static void positions(void) {
    reset();
    piece(10, 160, 0, 0);
    assert(gOAMControls.updated == 1);
    assert(((u16*)&gOAMControls.oam[0])[0] == 160); // hardware layout unchanged
    latch();
    render(false);
    assert(pixel(10, 160) == green); // untagged/native decode keeps parked Y semantics
    render(true);
    assert(pixel(10, 160) == red);
    assert(pixel(10, 159) == green);
    // The same emitter records world entities and direct HUD pieces.
    reset();
    sVoxelCtx.kind = PORT_VOXEL_OAM_ENTITY;
    piece(10, 170, 0, 0);
    latch();
    render(true);
    assert(pixel(10, 170) == red && gPortVoxelOamTagsBuild[0].trueY == 170);
    // Raster fallback must retain the metadata: this positive Y must not wrap
    // a tall affine sprite onto the top of a temporary native-height aperture.
    reset();
    piece(10, 170, 0x300, 0x30); // double-size 64x64 in 128x128 bounds
    latch();
    viewHeight = 160;
    render(true);
    assert(pixel(10, 0) == green);
    render(false);
    assert(pixel(10, 0) == red);
    reset();
    piece(255, 191, 0, 0);
    latch();
    render(true);
    assert(pixel(255, 191) == red && pixel(254, 191) == green);
    assert(pixel(0, 0) == green);

    reset();
    viewHeight = 160;
    piece(10, 160, 0, 0);
    assert(gOAMControls.updated == 0); // native piece clipping retained
    viewHeight = 192;
    piece(10, 192, 0, 0);
    assert(gOAMControls.updated == 0);

    reset();
    piece(-4, 170, 0, 0);
    latch();
    render(true);
    assert(pixel(0, 170) == red && pixel(3, 170) == red && pixel(4, 170) == green);

    if (MODE1_GBA_WIDTH >= 569) {
        reset();
        viewWidth = 569;
        piece(530, 10, 0, 0);
        latch();
        assert((oam[1] & 511) == 18);
        render(true);
        assert(pixel(530, 10) == red && pixel(18, 10) == green);
        render(false);
        assert(pixel(18, 10) == red); // baseline exposes the encoding ambiguity
        reset();
        viewWidth = 569;
        oam[0] = 10;
        oam[1] = 508; // untagged native X=-4 must not become a sprite at X=508
        oam[2] = 0;
        render(true);
        assert(pixel(0, 10) == red && pixel(508, 10) == green);
    }
}

static void validity(void) {
    reset();
    piece(10, 170, 0, 0);
    latch();
    const VirtuaPPUObjPosition before = Port_Oam_Positions()[0];
    FlushSprites();
    piece(40, 180, 0, 0);
    assert(memcmp(&before, &Port_Oam_Positions()[0], sizeof(before)) == 0);
    render(true);
    assert(pixel(10, 170) == red && pixel(40, 180) == green);
    latch();
    render(true);
    assert(pixel(10, 170) == green && pixel(40, 180) == red);
    // An out-of-band raw OAM edit invalidates the coordinate override.
    oam[0] = 10;
    render(true);
    assert(pixel(40, 10) == red && pixel(40, 180) == green);

    reset();
    piece(10, 170, 0, 0);
    latch();
    Port_Oam_Invalidate();
    render(true);
    assert(pixel(10, 170) == green);
    assert(Port_Oam_Positions()[0].flags == 0);
    Port_Oam_Record(0, 170, 10, 10, 170, true);
    Port_Oam_Latch(1);
    render(true);
    assert(pixel(10, 170) == green);
    Port_Oam_Record(0, 170, 10, INT_MAX, 170, false);
    Port_Oam_Latch(1);
    assert(Port_Oam_Positions()[0].flags & VIRTUAPPU_OBJ_POSITION_HIDDEN);
    Port_Oam_Record(128, 0, 0, 0, 0, false);
    Port_Oam_Latch(999); // bounded count
    Port_Oam_Latch(0);
    for (unsigned i = 0; i < 128; ++i) assert(Port_Oam_Positions()[i].flags == 0);

    reset();
    voxel = 1;
    piece(270, 10, 0, 0);
    assert(gOAMControls.updated == 1);
    latch();
    assert(Port_Oam_Positions()[0].flags & VIRTUAPPU_OBJ_POSITION_HIDDEN);
    render(true);
    assert(pixel(0, 0) == green && pixel(14, 10) == green);
    // Metadata must never bypass the hardware disabled bit.
    oam[0] = 0x2a0;
    Port_Oam_Record(0, oam[0], oam[1], 10, 170, false);
    Port_Oam_Latch(1);
    render(true);
    assert(pixel(10, 170) == green);
    virtuappu_reset();
    assert(!virtuappu_mode1_has_obj_positions());
}

static void affine(void) {
    reset();
    piece(10, 170, 0x100, 0x10); // affine 16x16
    ((u16*)gOAMControls.oam)[3] = ((u16*)gOAMControls.oam)[15] = 0x100;
    latch();
    render(true);
    assert(pixel(16, 176) == red);
    static uint32_t scaled[MODE1_GBA_WIDTH * 2 * 192 * 2];
    memset(scaled, 0, sizeof(scaled));
    virtuappu_mode1_render_affine_obj_overlay(scaled, viewWidth * 2, 192 * 2, 2);
    assert(scaled[(176 * 2) * (viewWidth * 2) + 16 * 2] == red);
    reset();
    piece(10, 170, 0x300, 0); // double-size affine 8x8 in 16x16 bounds
    ((u16*)gOAMControls.oam)[3] = ((u16*)gOAMControls.oam)[15] = 0x100;
    latch();
    render(true);
    assert(pixel(16, 176) == red && pixel(10, 170) == green);
}

static void culling(void) {
    reset();
    Entity entity = {0};
    entity.y.HALF.HI = 240;
    viewHeight = 160;
    assert(!CheckOnScreen(&entity));
    assert(!CheckRectOnScreen(16, 176, 4, 4));
    viewHeight = 192;
    assert(CheckOnScreen(&entity));
    assert(CheckRectOnScreen(16, 176, 4, 4));
    entity.y.HALF.HI = -64;
    assert(!CheckOnScreen(&entity));
}

static void heading_lifetime(void) {
    reset();
    assert(!Port_Oam_MenuHeading()->valid);
    Port_Oam_RecordMenuHeading(64, -16, 0x1fa);
    assert(!Port_Oam_MenuHeading()->valid); /* before the OAM latch */
    Port_Oam_Latch(0);
    assert(Port_Oam_MenuHeading()->valid && Port_Oam_MenuHeading()->x == 64);
    assert(Port_Oam_MenuHeading()->y == -16 && Port_Oam_MenuHeading()->sprite == 0x1fa);
    Port_Oam_BeginFrame();
    assert(Port_Oam_MenuHeading()->valid); /* repeat presents stay latched */
    Port_Oam_Latch(0);
    assert(!Port_Oam_MenuHeading()->valid);
    Port_Oam_RecordMenuHeading(INT_MAX, 0, 0x1fa);
    Port_Oam_Latch(0);
    assert(!Port_Oam_MenuHeading()->valid);
    Port_Oam_RecordMenuHeading(64, 16, 0x1fb);
    Port_Oam_Latch(0);
    Port_Oam_InvalidateLatchedRange(4, 2);
    assert(!Port_Oam_MenuHeading()->valid);
    Port_Oam_Latch(0);
    Port_Oam_Invalidate();
    assert(!Port_Oam_MenuHeading()->valid);
}

int main(void) {
    static const u8 widths[12] = { 8,16,32,64,16,32,32,64,8,8,16,32 };
    static const u8 heights[12] = { 8,16,32,64,8,8,16,32,16,32,32,64 };
    for (unsigned table = 0; table < 5; ++table)
        for (unsigned i = 0; i < 12; ++i) {
            sSizeTable[table * 48 + i * 4 + 2] = widths[i] * (table == 4 ? 2 : 1);
            sSizeTable[table * 48 + i * 4 + 3] = heights[i] * (table == 4 ? 2 : 1);
        }
    VirtuaPPUMode1GbaMemory memory = { io, vram, bgpal, objpal, oam };
    virtuappu_mode1_bind_gba_memory(&memory);
    positions();
    validity();
    affine();
    culling();
    heading_lifetime();
    puts("Signed OAM emission, latching, raw fallback, affine pixels and culling: PASS");
    return 0;
}
