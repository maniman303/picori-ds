/* Exercise the production room publication and two-phase map staging bodies. */
#include "global.h"
#include "main.h"
#include "game.h"
#include "room.h"
#include "map.h"
#include "screen.h"
#include "tileMap.h"
#include "vram.h"
#include "port_widescreen.h"
#include "port_oam.h"
#undef static_assert
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

Main gMain;
RoomControls gRoomControls;
MapLayer gMapBottom, gMapTop;
Screen gScreen;
u8 gUpdateVisibleTiles, gManagerCount, gUnk_03003DE0;
u8 gShakeOffsets[256];
u16 gOamMem[512];
GenericEntity gEntities[MAX_ENTITIES], gAuxPlayerEntities[MAX_AUX_PLAYER_ENTITIES];
u16 gMapDataBottomSpecial[0x4000], gMapDataTopSpecial[0x4000];
u8 gEwram[0x40000];
static BgSettings bg;
static uint64_t sWsContentKey = UINT64_MAX;
static s32 sWsContentPx;
static PortViewportUpload sWsUpload;
static int sWsStageHeight;
static int sWsManagerDirty, sWsEntityDirty;
static uint64_t sWsValidatedKey = UINT64_MAX;
static int Port_WidescreenScanContentPx(void) { return 0; }
static int Port_WidescreenPpuBgForControl(u32 c) { (void)c; return 1; }
static int Port_WidescreenRepeatingOverlayLive(void) { return 0; }
int Port_Widescreen_TargetViewWidth(void) { return Port_Viewport_Get()->canvas.width; }
void sub_0807D280(u16* a, u16* b) { (void)a; (void)b; }
void sub_0807D46C(u16* a, u16* b) { (void)a; (void)b; }
void sub_0807D6D8(u16* a, u16* b) { (void)a; (void)b; }
#include "tall_aperture_functions.inc"

int main(void) {
    assert(Port_Viewport_Publish(PORT_ASPECT_DS_4_3, false, 569, 192, 569));
    for (int i = 0; i < 128; ++i) gOamMem[i * 4] = 0x200;
    gMain.task = TASK_GAME; gMain.state = GAMETASK_MAIN; gMain.substate = GAMEMAIN_UPDATE;
    gRoomControls.width = 1024; gRoomControls.height = 1024; gRoomControls.scrollAction = 1;
    gRoomControls.scroll_x = 32; gRoomControls.scroll_y = 32;
    gMapBottom.bgSettings = &bg; bg.control = 0;
    for (unsigned i = 0; i < 0x4000; ++i) gMapDataBottomSpecial[i] = (u16)(i + 1);
    Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 160); /* no scene certification */
    Port_Widescreen_SetTallValidated(1);
    Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 160 && sWsUpload.filled && !sWsUpload.uploaded);
    assert(Port_Viewport_RasterHeight(Port_Viewport_Get()) == 160);
    assert(Port_Widescreen_MapBufferSize() == 27 * 64);
    assert(gBG1Buffer[26 * 32] != 0); /* bottom staging row was filled */
    Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 160); /* no upload yet */
    u8 fakeVram[32 * 32 * sizeof(u16)] = {0};
    assert(fakeVram[26 * 64] == 0);
    memcpy(fakeVram, gBG1Buffer, Port_Widescreen_MapBufferSize());
    Port_Widescreen_MapUploaded();
    assert(memcmp(fakeVram, gBG1Buffer, 27 * 64) == 0);
    Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 192);
    assert(Port_Viewport_RasterHeight(Port_Viewport_Get()) == 192);
    gOamMem[0] = 0; /* a raw visible OAM slot cannot establish signed coordinates */
    Port_Widescreen_BeforePresent();
    assert(Port_Viewport_Get()->world.height == 160);
    Port_Oam_Record(0, gOamMem[0], gOamMem[1], 0, 0, false);
    Port_Oam_Latch(1);
    Port_Widescreen_PublishScene();
    assert(sWsUpload.filled && !sWsUpload.uploaded);
    Port_Widescreen_MapUploaded(); Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 192);
    Port_Oam_InvalidateLatchedRange(0, 2); /* even a byte-identical raw write */
    Port_Widescreen_BeforePresent();
    assert(Port_Viewport_Get()->world.height == 160);
    gOamMem[0] = 0x200; /* hardware-disabled slot is safe */
    Port_Widescreen_PublishScene(); Port_Widescreen_MapUploaded(); Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 192);
    gUnk_03003DE0 = 1; /* unknown pending native upload is not resident proof */
    Port_Widescreen_BeforePresent();
    assert(Port_Viewport_Get()->world.height == 160);
    gUnk_03003DE0 = 0;
    Port_Widescreen_PublishScene();
    assert(sWsUpload.filled && !sWsUpload.uploaded);
    Port_Widescreen_MapUploaded(); Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 192);
    gManagerCount = 1;
    Port_Widescreen_BeforePresent();
    assert(Port_Viewport_Get()->world.height == 160 && !sWsUpload.requested);
    assert(Port_Viewport_RasterHeight(Port_Viewport_Get()) == 160);
    gManagerCount = 0;
    Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 160); /* deleted manager still blocks */
    sWsManagerDirty = 0; /* emulate a fresh manager-free room load */
    Port_Widescreen_PublishScene();
    assert(sWsUpload.filled && !sWsUpload.uploaded);
    gRoomControls.scroll_y += 16; /* stale fill must not authorize new camera */
    Port_Widescreen_MapUploaded();
    assert(!sWsUpload.uploaded);
    Port_Widescreen_PublishScene();
    assert(sWsUpload.filled && !sWsUpload.uploaded && Port_Viewport_Get()->world.height == 160);
    Port_Widescreen_MapUploaded(); Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 192);
    gShakeOffsets[15] = (u8)-9;
    InitScreenShake(1, 0);
    assert(Port_Viewport_Get()->world.height == 160);
    assert(gUpdateVisibleTiles == 1 && Port_Widescreen_MapBufferSize() == 23 * 64);
    gShakeOffsets[15] = 8;
    InitScreenShake(1, 0);
    assert(sWsUpload.filled && !sWsUpload.uploaded && Port_Widescreen_MapBufferSize() == 28 * 64);
    Port_Widescreen_MapUploaded(); Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 192);
    gEntities[0].base.next = (Entity*)&gEntities[0]; /* live unclassified entity */
    Port_Widescreen_BeforePresent();
    assert(Port_Viewport_Get()->world.height == 160);
    gEntities[0].base.next = NULL;
    Port_Widescreen_BeforePresent();
    assert(Port_Viewport_Get()->world.height == 160); /* sticky after deletion */
    sWsEntityDirty = 0;
    Port_Widescreen_PublishScene(); Port_Widescreen_MapUploaded(); Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 192);
    gAuxPlayerEntities[0].base.next = (Entity*)&gAuxPlayerEntities[0];
    Port_Widescreen_BeforePresent();
    assert(Port_Viewport_Get()->world.height == 160);
    gAuxPlayerEntities[0].base.next = NULL; sWsEntityDirty = 0;
    Port_Widescreen_PublishScene(); Port_Widescreen_MapUploaded(); Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 192);
    Port_Widescreen_EntityCreated(); /* appeared and vanished within this tick */
    Port_Widescreen_BeforePresent();
    assert(Port_Viewport_Get()->world.height == 160);
    sWsEntityDirty = 0; /* emulate a fresh empty room */
    Port_Widescreen_PublishScene(); Port_Widescreen_MapUploaded(); Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 192);
    Port_Widescreen_ManagerCreated(); /* appeared and vanished within this tick */
    Port_Widescreen_BeforePresent();
    assert(Port_Viewport_Get()->world.height == 160);
    ++gRoomControls.room;
    Port_Widescreen_PublishScene();
    assert(Port_Viewport_Get()->world.height == 160); /* certification is room-specific */
    puts("Room aperture waits for matching map fill and upload: PASS");
    return 0;
}
