/* Exercise real manager upload sequences against the PC immediate-copy path. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "manager/minishVillageTileSetManager.h"
#include "manager/hyruleTownTileSetManager.h"
#include "room.h"
#include "structures.h"
#include "region.h"
#include "area.h"
#include "assets/gfx_offsets.h"
#undef static_assert
#include <assert.h>

RoomControls gRoomControls;
RoomVars gRoomVars;
PauseMenuOptions gPauseMenuOptions;
int gActiveRegion = TMC_REGION_EU;
static u8 source[0x200000];
static u8 vram[0x18000];
const u8* gGlobalGfxAndPalettes = source;
const u8 gUnk_086E8460[1] = {0};
static unsigned selection, townSelections[3], selectionIndex, palettes;
static int actualRegions, viewHeight = 160;
int Port_Widescreen_EffectiveViewWidth(void) { return 240; }
int Port_Widescreen_EffectiveViewHeight(void) { return viewHeight; }
int Port_Widescreen_RegionViewWidth(void) { return 240; }
int Port_Widescreen_RegionViewHeight(void) { return viewHeight; }
static int lastManagerSafe = -1;
void Port_Widescreen_NoteManagerResidency(unsigned id, int safe) { (void)id; lastManagerSafe = safe; }
static int atomicStream;
static int residencyProven = 1;
bool Port_VillageGroups_AtomicStream(void) { return atomicStream != 0; }
void Port_VillageGroups_Publish(const uint16_t* regions, const uint32_t* chunks,
                                const uint8_t* palette_ids, int group_count) {
    (void)regions; (void)chunks; (void)palette_ids; (void)group_count;
}
bool Port_VillageGroups_ResidencyProven(void) { return residencyProven != 0; }
static unsigned palettesApplied;
void Port_VillageGroups_ApplyLoadedPalette(void) { palettesApplied++; }
#include "region_intersection.inc"
extern const u16 gUnk_08108050_eu[];

extern const u32 gUnk_081080A4_eu[];
extern const u32 gUnk_081080A4[];
void MinishVillageTileSetManager_Main(MinishVillageTileSetManager*);
void sub_08057E7C(u32);
void HyruleTownTileSetManager_UpdateLoadGfxGroups(HyruleTownTileSetManager*);
void HyruleTownTileSetManager_LoadGfxGroup(u32, u32);

void* port_resolve_addr(uintptr_t a) {
    if (a >= 0x06000000 && a < 0x06000000 + sizeof(vram)) return vram + (a - 0x06000000);
    return (void*)a;
}
#include "immediate_resource.inc"
u32 CheckRegionsOnScreen(const u16* regions) {
    if (actualRegions) {
        for (; *regions != 0xff; regions += 5)
            if (CheckRegionOnScreen(regions[1], regions[2], regions[3], regions[4])) return *regions;
        return 0xff;
    }
    if (gRoomControls.area == AREA_HYRULE_TOWN || gRoomControls.area == AREA_FESTIVAL_TOWN)
        return townSelections[selectionIndex++];
    return selection;
}
void RegisterTransitionHandler(void* mgr, void (*onEnter)(), void (*onExit)()) {
    (void)mgr; (void)onEnter; (void)onExit;
}
void SetEntityPriority(Entity* e, u32 priority) { (void)e; (void)priority; }
void LoadPaletteGroup(u32 group) { palettes++; (void)group; }
bool32 CheckGlobalFlag(u32 flag) { (void)flag; return FALSE; }
void SetTileByIndex(u32 tile, u32 pos, u32 layer) { (void)tile; (void)pos; (void)layer; }
u32 Port_RemapGfxOffset(u32 off) { return off; }
u32 Port_RemapCompiledGfxOffset(u32 off) { return off; }
u32 Port_IsLoadedAssetBytes(const void* ptr, u32 size) { (void)ptr; (void)size; return 0; }

static void check_village(void) {
    MinishVillageTileSetManager m = {0};
    m.base.action = 1; m.base.timer = 8; m.unk_20 = 0xff;
    gRoomControls.area = 0xff;
    atomicStream = 0;
    residencyProven = 0; /* native/wide staged path: tall admission stays off */
    for (selection = 0; selection < 5; ++selection) {
        memset(vram, 0xee, sizeof(vram));
        lastManagerSafe = -1;
        for (int phase = 0; phase < 8; ++phase) {
            MinishVillageTileSetManager_Main(&m);
            assert(m.base.timer == phase + 1);
            /* The staged GBA stream is only used when the tall composition is
             * not active; it must not claim residency itself. */
            assert(lastManagerSafe == 0);
            for (int chunk = 0; chunk < 8; ++chunk) {
                u32 dest = gUnk_081080A4_eu[selection * 16 + chunk * 2 + 1];
                u8* actual = port_resolve_addr(dest);
                if (chunk <= phase) assert(memcmp(actual, source + gUnk_081080A4_eu[selection * 16 + chunk * 2], 0x1000) == 0);
                else for (int i = 0; i < 0x1000; ++i) assert(actual[i] == 0xee);
            }
        }
        assert(lastManagerSafe == 0);
        assert(palettes == selection + 1);
    }
    /* A region switch in the middle of streaming starts the new sequence. */
    selection = 1;
    MinishVillageTileSetManager_Main(&m);
    MinishVillageTileSetManager_Main(&m);
    selection = 2;
    MinishVillageTileSetManager_Main(&m);
    assert(m.base.timer == 1 && palettes == 7);
    assert(memcmp(vram, source + gUnk_081080A4_eu[32], 0x1000) == 0);
    selection = 0xff; /* no intersecting region: complete the installed group */
    MinishVillageTileSetManager_Main(&m);
    assert(m.base.timer == 2 && m.unk_20 == 2);
    assert(memcmp(vram + 0x1000, source + gUnk_081080A4_eu[34], 0x1000) == 0);
    memset(&m, 0, sizeof(m)); m.base.action = 1; m.base.timer = 8; m.unk_20 = 0xff;
    memset(vram, 0xee, sizeof(vram));
    MinishVillageTileSetManager_Main(&m);
    assert(m.base.timer == 8 && palettes == 7);
    for (size_t i = 0; i < sizeof(vram); ++i) assert(vram[i] == 0xee);
}

static void check_atomic_village(void) {
    MinishVillageTileSetManager m = {0};
    gRoomControls.area = 0xff;
    atomicStream = 1;
    residencyProven = 1;
    palettes = 0;
    palettesApplied = 0;
    for (selection = 0; selection < 5; ++selection) {
        memset(vram, 0xee, sizeof(vram));
        m.base.action = 1; m.base.timer = 8; m.unk_20 = 0xff;
        MinishVillageTileSetManager_Main(&m);
        /* One update installs the complete group and its palette: no staged
         * mixed-charset frame and therefore no aperture pop. */
        assert(m.base.timer == 8 && m.unk_20 == selection);
        assert(lastManagerSafe == 1);
        assert(palettes == selection + 1);
        assert(palettesApplied == selection + 1);
        for (int chunk = 0; chunk < 8; ++chunk) {
            u32 dest = gUnk_081080A4_eu[selection * 16 + chunk * 2 + 1];
            assert(memcmp(port_resolve_addr(dest),
                          source + gUnk_081080A4_eu[selection * 16 + chunk * 2], 0x1000) == 0);
        }
        /* A second update with the same group is a no-op. */
        MinishVillageTileSetManager_Main(&m);
        assert(palettes == selection + 1 && palettesApplied == selection + 1);
    }
    atomicStream = 0;
    residencyProven = 1;
}
static void check_other_region_tables(void) {
    const int regions[] = {TMC_REGION_USA, TMC_REGION_JP};
    for (unsigned r = 0; r < sizeof(regions) / sizeof(regions[0]); ++r) {
        MinishVillageTileSetManager m = {0};
        m.base.action = 1; m.base.timer = 8; m.unk_20 = 0xff;
        gActiveRegion = regions[r]; gRoomControls.area = 0xff; selection = 0xff;
        MinishVillageTileSetManager_Main(&m); /* no group must fail closed */
        assert(m.base.timer == 8 && m.unk_20 == 0xff);
        selection = 3; memset(vram, 0xee, sizeof(vram));
        for (int phase = 0; phase < 8; ++phase) {
            MinishVillageTileSetManager_Main(&m);
            assert(m.base.timer == phase + 1);
            u32 dest = gUnk_081080A4[48 + phase * 2 + 1];
            assert(memcmp(port_resolve_addr(dest), source + gUnk_081080A4[48 + phase * 2], 0x1000) == 0);
        }
        selection = 0xff;
        MinishVillageTileSetManager_Main(&m);
        assert(m.base.timer == 9 && m.unk_20 == 3);
    }
    gActiveRegion = TMC_REGION_EU;
}
static void check_region_boundaries(void) {
    unsigned differences = 0;
    actualRegions = 1;
    gRoomControls.origin_x = gRoomControls.origin_y = 0;
    for (int x = 0; x < 1024; x += 16) for (int y = 0; y < 1024; y += 16) {
        gRoomControls.scroll_x = x; gRoomControls.scroll_y = y;
        viewHeight = 160; u32 native = CheckRegionsOnScreen(gUnk_08108050_eu);
        viewHeight = 192; u32 tall = CheckRegionsOnScreen(gUnk_08108050_eu);
        if (native != tall) ++differences;
    }
    assert(differences > 0); /* A taller rectangle can select a different bank. */
    actualRegions = 0; viewHeight = 160;
}
static void check_town_groups(void) {
    static const unsigned normal1[] = {0x8000,0xb000,0x9000,0xc000,0xa000,0xd000};
    static const unsigned normal2[] = {0xe000,0x11000,0xf000,0x12000,0x10000,0x13000};
    static const unsigned festival1[] = {0x0800,0x3800,0x1800,0x4800,0x2800,0x5800};
    static const unsigned festival2[] = {0x6800,0x9800,0x7800,0xa800,0x8800,0xb800};
    for (int festival = 0; festival < 2; ++festival) {
        gRoomControls.area = festival ? AREA_FESTIVAL_TOWN : AREA_HYRULE_TOWN;
        for (unsigned group = 0; group < 6; ++group) {
            unsigned base = festival ? offset_gUnk_086E8460 : offset_gUnk_086D4460;
            unsigned a = festival ? festival1[group] : normal1[group];
            unsigned b = festival ? festival2[group] : normal2[group];
            unsigned dest = (group / 2) * 0x1000;
            memset(vram, 0xee, sizeof(vram));
            HyruleTownTileSetManager_LoadGfxGroup(group / 2, group);
            assert(memcmp(vram + dest, source + base + a, 0x1000) == 0);
            assert(memcmp(vram + dest + 0x8000, source + base + b, 0x1000) == 0);
        }
    }
}
static void check_town(void) {
    HyruleTownTileSetManager m = {0};
    m.base.action = 1;
    gRoomControls.area = AREA_HYRULE_TOWN;
    memset(gRoomVars.graphicsGroups, 0xff, sizeof(gRoomVars.graphicsGroups));
    townSelections[0] = 0; townSelections[1] = 3; townSelections[2] = 4;
    selectionIndex = 0;
    memset(vram, 0xee, sizeof(vram));
    HyruleTownTileSetManager_UpdateLoadGfxGroups(&m);
    assert(selectionIndex == 3);
    assert(gRoomVars.graphicsGroups[0] == 0 && gRoomVars.graphicsGroups[1] == 3 && gRoomVars.graphicsGroups[2] == 4);
    /* Each pair is copied synchronously, before UpdateLoadGfxGroups returns. */
    u8 copy[sizeof(vram)]; memcpy(copy, vram, sizeof(vram));
    selectionIndex = 0;
    HyruleTownTileSetManager_UpdateLoadGfxGroups(&m);
    assert(memcmp(copy, vram, sizeof(vram)) == 0);
    townSelections[0] = 1; selectionIndex = 0;
    HyruleTownTileSetManager_UpdateLoadGfxGroups(&m);
    assert(gRoomVars.graphicsGroups[0] == 1 && memcmp(copy, vram, sizeof(vram)) != 0);
}
static void check_tall_group_selection(int height) {
    actualRegions = 1;
    gRoomControls.origin_x = gRoomControls.origin_y = 0;
    int found = 0;
    for (int x = 0; x < 1024 && !found; x += 16) {
        for (int y = 0; y < 1024 && !found; y += 16) {
            gRoomControls.scroll_x = x; gRoomControls.scroll_y = y;
            viewHeight = height;
            u32 tall = CheckRegionsOnScreen(gUnk_08108050_eu);
            const u32 mask = CheckRegionsOnScreenMask(gUnk_08108050_eu);
            /* Exercise a view that intersects two regions: the selected group
             * still loads its chunks, while the host charset atlas covers the
             * other group in the tall aperture. */
            if (tall == 0xff || (mask & (mask - 1)) == 0)
                continue;
            MinishVillageTileSetManager m = {0};
            m.base.action = 1; m.base.timer = 8; m.unk_20 = 0xff;
            palettes = 0;
            memset(vram, 0xee, sizeof(vram));
            for (int phase = 0; phase < 8; ++phase) {
                MinishVillageTileSetManager_Main(&m);
                assert(m.base.timer == phase + 1);
                for (int chunk = 0; chunk <= phase; ++chunk) {
                    u32 dest = gUnk_081080A4_eu[tall * 16 + chunk * 2 + 1];
                    assert(memcmp(port_resolve_addr(dest),
                                  source + gUnk_081080A4_eu[tall * 16 + chunk * 2], 0x1000) == 0);
                }
            }
            assert(m.unk_20 == tall);
            assert(lastManagerSafe == 1);
            found = 1;
        }
    }
    assert(found); /* a single-group tall selection is resident and safe */
    actualRegions = 0;
    viewHeight = 160;
}

int main(void) {
    for (size_t i = 0; i < sizeof(source); ++i) source[i] = (u8)((i >> 12) ^ (i >> 20));
    check_village(); check_atomic_village(); check_other_region_tables(); check_town_groups(); check_town();
    check_region_boundaries(); check_tall_group_selection(162); check_tall_group_selection(192);
    puts("Town and village manager immediate upload timing: PASS");
    return 0;
}
