#include "port_village_groups.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fade.h"
#include "gfx.h"
#include "main.h"
#include "room.h"
#include "screen.h"
#include "structures.h"

#include "cpu/mode1.h"
#include "port_fade.h"
#include "port_gba_mem.h"
#include "port_viewport.h"

#define VILLAGE_MAX_GROUPS PORT_VILLAGE_GROUP_LIMIT
#define VILLAGE_CHARS_PER_GROUP 0xC000
#define VILLAGE_CHUNK_BYTES 0x1000
#define VILLAGE_GRID_COLS 32
#define VILLAGE_GRID_ROWS 32

/* The village charsets live in GBA BG char VRAM at 0x0000-0x3FFF and
 * 0x8000-0xBFFF (two 16 KB BG char bases). Keep the same 0xC000 layout in host
 * memory so the PPU can reuse the original char offset unchanged. */
static uint8_t sChars[VILLAGE_MAX_GROUPS][VILLAGE_CHARS_PER_GROUP];
static uint16_t sRawPal[VILLAGE_MAX_GROUPS][MODE1_PALETTE_COLORS];
static uint16_t sPal[VILLAGE_MAX_GROUPS][MODE1_PALETTE_COLORS];
static uint32_t sLut[VILLAGE_MAX_GROUPS][MODE1_PALETTE_COLORS];
static uint8_t sCells[VILLAGE_GRID_COLS * VILLAGE_GRID_ROWS];
/* Reveal-shadow columns (x >= 240): the port shadow tilemap mirrors the room
 * map, so it needs its own group grid with the shadow's world-row/col mapping. */
static uint8_t sShadowCells[MODE1_WS_SHADOW_ROWS][MODE1_WS_SHADOW_COLS];

static const uint16_t* sRegions;
static const uint32_t* sChunks;
static const uint8_t* sPaletteIds;
static int sGroupCount;
static int sRegionCount;
static bool sActive;
static bool sReady;

static bool VillageAssetsWanted(void) {
    return Port_Viewport_Get()->canvas.height > PORT_VIEWPORT_NATIVE_HEIGHT;
}

int Port_VillageGroups_RegionAt(const uint16_t* regions, int region_count,
                                int world_x, int world_y, int fallback) {
    if (regions == NULL || region_count < 0)
        return fallback;
    const int limit = region_count > 32 ? 32 : region_count;
    for (int i = 0; i < limit; ++i) {
        const uint16_t* row = regions + i * 5;
        const int group = row[0];
        if (group == 0xFF)
            break;
        const int x = row[1];
        const int y = row[2];
        const int w = row[3];
        const int h = row[4];
        if (w <= 0 || h <= 0)
            continue;
        if (world_x >= x && world_x < x + w && world_y >= y && world_y < y + h)
            return group;
    }
    return fallback;
}

void Port_VillageGroups_BuildCells(const uint16_t* regions, int region_count,
                                   int group_count, int selected, int xdiff, int ydiff,
                                   int cols, int rows, uint8_t* cells) {
    if (cells == NULL || cols <= 0 || rows <= 0)
        return;
    if (selected < 0 || selected >= group_count) {
        /* No valid resident group: keep every cell on native VRAM. Tall
         * admission already refuses this state. */
        memset(cells, 0xFF, (size_t)cols * (size_t)rows);
        return;
    }
    const unsigned uxdiff = (unsigned)xdiff & 0xFFFFu;
    const unsigned uydiff = (unsigned)ydiff & 0xFFFFu;
    /* ram_sub_080B197C_c starts the buffer at an even tile column and one
     * padding row above the camera row. The first row duplicates row zero
     * while ydiff < 8 (native top-padding contract). */
    const int col0 = (int)((uxdiff >> 3) & ~1u);
    const int row0 = (int)((uydiff >> 3) & ~1u);
    const int fallback = (selected >= 0 && selected < group_count) ? selected : 0xFF;
    for (int r = 0; r < rows; ++r) {
        int world_row = row0 + r - 1;
        if (uydiff < 8u && r == 0)
            world_row = row0;
        for (int c = 0; c < cols; ++c) {
            const int world_col = col0 + c;
            int group = Port_VillageGroups_RegionAt(regions, region_count,
                                                    world_col * 8 + 4, world_row * 8 + 4,
                                                    fallback);
            if (group < 0 || group >= group_count)
                group = fallback;
            /* The selected group is already resident in VRAM (including any
             * animated char/palette updates). Use the host charset only for
             * the other groups that cannot coexist in the shared span. */
            if (group == selected)
                group = 0xFF;
            cells[r * cols + c] = (uint8_t)group;
        }
    }
}

static int VillageCountRegions(const uint16_t* regions) {
    int count = 0;
    if (regions != NULL) {
        while (count < 32 && regions[count * 5] != 0xFF)
            ++count;
    }
    return count;
}

static bool VillageLoadAssets(void) {
    if (sReady)
        return true;
    if (sChunks == NULL || sPaletteIds == NULL || sGroupCount <= 0 || sGroupCount > VILLAGE_MAX_GROUPS)
        return false;
    if (gGlobalGfxAndPalettes == NULL)
        return false;

    for (int g = 0; g < sGroupCount; ++g) {
        for (int chunk = 0; chunk < 8; ++chunk) {
            const uint32_t src = sChunks[g * 16 + chunk * 2];
            const uint32_t dest = sChunks[g * 16 + chunk * 2 + 1];
            if (dest < 0x06000000u)
                return false;
            const uint32_t offset = dest - 0x06000000u;
            if (offset + VILLAGE_CHUNK_BYTES > VILLAGE_CHARS_PER_GROUP)
                return false;
            memcpy(&sChars[g][offset], gGlobalGfxAndPalettes + src, VILLAGE_CHUNK_BYTES);
        }
    }

    /* Capture exactly what LoadPaletteGroup installs for each group (ROM or
     * asset override), without disturbing the room's live palette. */
    uint16_t saved[0x200];
    memcpy(saved, gPaletteBuffer, sizeof(saved));
    const u32 savedUsed = gUsedPalettes;
    for (int g = 0; g < sGroupCount; ++g) {
        LoadPaletteGroup(sPaletteIds[g]);
        memcpy(sRawPal[g], gPaletteBuffer, sizeof(sRawPal[g]));
    }
    memcpy(gPaletteBuffer, saved, sizeof(saved));
    gUsedPalettes = savedUsed;
    sReady = true;
    return true;
}

void Port_VillageGroups_Publish(const uint16_t* regions, const uint32_t* chunks,
                                const uint8_t* palette_ids, int group_count) {
    if (regions == NULL || chunks == NULL || palette_ids == NULL ||
        group_count <= 0 || group_count > VILLAGE_MAX_GROUPS) {
        Port_VillageGroups_Reset();
        return;
    }
    if (regions != sRegions || chunks != sChunks || palette_ids != sPaletteIds ||
        group_count != sGroupCount)
        sReady = false;
    sRegions = regions;
    sChunks = chunks;
    sPaletteIds = palette_ids;
    sGroupCount = group_count;
    sRegionCount = VillageCountRegions(regions);
    if (sRegionCount <= 0) {
        Port_VillageGroups_Reset();
        return;
    }
    sActive = true;
    if (VillageAssetsWanted())
        (void)VillageLoadAssets();
}

void Port_VillageGroups_Reset(void) {
    sActive = false;
    virtuappu_mode1_clear_bg_group_cells();
}

bool Port_VillageGroups_ResidencyProven(void) {
    if (!sActive)
        return false;
    if (!sReady && (!VillageAssetsWanted() || !VillageLoadAssets()))
        return false;
    return gRoomVars.graphicsGroups[0] < (u8)sGroupCount;
}

bool Port_VillageGroups_AtomicStream(void) {
    /* Only the tall profile needs the atomic switch; native/wide keep the
     * GBA eight-step pacing and therefore byte-identical output. */
    return Port_Viewport_Get()->canvas.height > PORT_VIEWPORT_NATIVE_HEIGHT;
}

void Port_VillageGroups_ApplyLoadedPalette(void) {
    /* The village palette groups install 13 banks at destination 2. Apply the
     * same transform FadeVBlank would use, without waiting for the VBlank
     * latch that happens after the next present. */
    for (int bank = 2; bank <= 14; ++bank)
        Port_Fade_TransformPaletteBank(&gPaletteBuffer[bank * 16], &gBgPltt[bank * 16],
                                       (unsigned)bank);
}

static void VillageBuildLuts(unsigned selected) {
    const bool hasSelected = selected < (unsigned)sGroupCount;
    for (int g = 0; g < sGroupCount; ++g) {
        for (int bank = 0; bank < 16; ++bank) {
            uint16_t* dst = &sPal[g][bank * 16];
            const bool groupBank = bank >= 2 && bank <= 14;
            if (groupBank && (!hasSelected ||
                              memcmp(&sRawPal[g][bank * 16], &sRawPal[selected][bank * 16],
                                     sizeof(uint16_t) * 16) != 0)) {
                Port_Fade_TransformPaletteBank(&sRawPal[g][bank * 16], dst, (unsigned)bank);
            } else {
                memcpy(dst, &gBgPltt[bank * 16], sizeof(uint16_t) * 16);
            }
        }
        for (int i = 0; i < MODE1_PALETTE_COLORS; ++i)
            sLut[g][i] = virtuappu_mode1_rgb555_to_abgr8888(sPal[g][i]);
    }
}

static void VillageBuildShadowCells(int xdiff, int ydiff, int selected) {
    const int16_t sxd = (int16_t)xdiff;
    const int16_t syd = (int16_t)ydiff;
    const int row16 = syd >> 4;
    const int base_world_col = (sxd >> 3) + (MODE1_GBA_BG_CLIP_X / 8) - 1;
    for (int r = 0; r < MODE1_WS_SHADOW_ROWS; ++r) {
        int world_row = 2 * row16 - 1 + r;
        if (syd >= 0 && syd < 8 && r == 0)
            world_row = 0;
        for (int c = 0; c < MODE1_WS_SHADOW_COLS; ++c) {
            const int world_col = base_world_col + c;
            int group = Port_VillageGroups_RegionAt(sRegions, sRegionCount,
                                                    world_col * 8 + 4, world_row * 8 + 4,
                                                    selected);
            if (group < 0 || group >= sGroupCount || group == selected)
                group = 0xFF;
            sShadowCells[r][c] = (uint8_t)group;
        }
    }
}

void Port_VillageGroups_Update(unsigned bg_mask) {
    const PortViewportGeometry* geometry = Port_Viewport_Get();
    const unsigned selected = gRoomVars.graphicsGroups[0];
    const bool live = sActive && sReady && bg_mask != 0 &&
                      geometry->canvas.height > PORT_VIEWPORT_NATIVE_HEIGHT &&
                      geometry->world.height > PORT_VIEWPORT_NATIVE_HEIGHT &&
                      Port_Viewport_Composition(geometry) == PORT_COMPOSITION_LIVE_WORLD &&
                      selected < (unsigned)sGroupCount;
    if (!live) {
        virtuappu_mode1_clear_bg_group_cells();
        return;
    }

    const int xdiff = (int)(u16)(gRoomControls.scroll_x - gRoomControls.origin_x);
    const int ydiff = (int)(u16)(gRoomControls.scroll_y - gRoomControls.origin_y);
    Port_VillageGroups_BuildCells(sRegions, sRegionCount, sGroupCount, (int)selected,
                                  xdiff, ydiff, VILLAGE_GRID_COLS, VILLAGE_GRID_ROWS, sCells);
    VillageBuildShadowCells(xdiff, ydiff, (int)selected);
    VillageBuildLuts(selected);
    virtuappu_mode1_bind_bg_group_cells(sCells, VILLAGE_GRID_COLS, VILLAGE_GRID_ROWS,
                                        &sLut[0][0], sGroupCount, &sChars[0][0],
                                        VILLAGE_CHARS_PER_GROUP, bg_mask);
    virtuappu_mode1_bind_bg_group_shadow(&sShadowCells[0][0], MODE1_WS_SHADOW_COLS,
                                         MODE1_WS_SHADOW_ROWS);

    static int trace = -1;
    if (trace < 0)
        trace = getenv("TMC_VILLAGE_TRACE") != NULL;
    if (trace) {
        static unsigned tick;
        int counts[VILLAGE_MAX_GROUPS] = {0};
        int native = 0;
        int minr = VILLAGE_GRID_ROWS, maxr = -1, minc = VILLAGE_GRID_COLS, maxc = -1;
        for (int r = 0; r < VILLAGE_GRID_ROWS; ++r) {
            for (int c = 0; c < VILLAGE_GRID_COLS; ++c) {
                const uint8_t cell = sCells[r * VILLAGE_GRID_COLS + c];
                if (cell < VILLAGE_MAX_GROUPS) {
                    counts[cell]++;
                    if (r < minr) minr = r;
                    if (r > maxr) maxr = r;
                    if (c < minc) minc = c;
                    if (c > maxc) maxc = c;
                } else {
                    native++;
                }
            }
        }
        int shadowGrouped = 0;
        int shMinR = MODE1_WS_SHADOW_ROWS, shMaxR = -1, shMinC = MODE1_WS_SHADOW_COLS, shMaxC = -1;
        for (int r = 0; r < MODE1_WS_SHADOW_ROWS; ++r) {
            for (int c = 0; c < MODE1_WS_SHADOW_COLS; ++c) {
                if (sShadowCells[r][c] < VILLAGE_MAX_GROUPS) {
                    shadowGrouped++;
                    if (r < shMinR) shMinR = r;
                    if (r > shMaxR) shMaxR = r;
                    if (c < shMinC) shMinC = c;
                    if (c > shMaxC) shMaxC = c;
                }
            }
        }
        if ((++tick % 60) == 0) {
            fprintf(stderr,
                    "[village] sel=%u mask=0x%x cells: g0=%d g1=%d g2=%d g3=%d g4=%d vram=%d "
                    "bounds r%d..%d c%d..%d scroll=%d,%d shadow=%d r%d..%d c%d..%d\n",
                    selected, bg_mask, counts[0], counts[1], counts[2], counts[3], counts[4],
                    native, minr, maxr, minc, maxc, xdiff, ydiff, shadowGrouped,
                    shMinR, shMaxR, shMinC, shMaxC);
        }
    }
}
