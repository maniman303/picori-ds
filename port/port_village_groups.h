#ifndef PORT_VILLAGE_GROUPS_H
#define PORT_VILLAGE_GROUPS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PORT_VILLAGE_GROUP_LIMIT 5

/* View-dependent charset groups used by the Minish Village tileset manager.
 * The manager streams one group at a time through the same eight VRAM spans,
 * so a taller aperture that shows two groups at once cannot be rendered from
 * VRAM alone. The port keeps every group's complete charset in host memory
 * and selects it per tilemap cell; the engine's VRAM stream stays untouched.
 *
 * `regions` is the manager's region table: five u16 per entry
 * {group, x, y, width, height}, terminated by a 0xFF group.
 * `chunks` is the 8-source/8-destination group table (16 u32 per group):
 * source offsets are relative to gGlobalGfxAndPalettes, destinations are GBA
 * char addresses (0x06000000 based).
 * `palette_ids` is the per-group global palette group id. */
void Port_VillageGroups_Publish(const uint16_t* regions, const uint32_t* chunks,
                                const uint8_t* palette_ids, int group_count);
/* Forget the live room. The host charsets stay cached for a same-room reload. */
void Port_VillageGroups_Reset(void);

/* True when the complete charsets/palettes for the live room are available and
 * the manager has a valid selected group. Tall admission uses this instead of
 * requiring one group to cover the aperture. */
bool Port_VillageGroups_ResidencyProven(void);

/* True when the active canvas wants the tall composition. The manager then
 * loads the selected group in one frame instead of the GBA eight-step stream:
 * the port renders every group from host memory, so waiting for the staged
 * upload would only produce a visible aperture pop. */
bool Port_VillageGroups_AtomicStream(void);

/* Apply the group palette banks just loaded by LoadPaletteGroup() to palette
 * RAM immediately. The engine's VBlank latch normally costs one frame; the
 * atomic stream changes the selected group before that present, so without
 * this a group switch would show the previous palette for one frame. */
void Port_VillageGroups_ApplyLoadedPalette(void);

/* Per-frame, after the scene is published and before the PPU render. Builds
 * the VRAM-cell -> group grid and the faded per-group palette LUTs, then binds
 * them for the BG layers that own the live map. `bg_mask` is a bit per BG. */
void Port_VillageGroups_Update(unsigned bg_mask);

/* Pure helpers (ROM-free tests). `world_x/world_y` are room pixels.
 * `region_count` is the number of table entries (the table can repeat a group
 * for disjoint rectangles); `group_count` is the distinct charset count. */
int Port_VillageGroups_RegionAt(const uint16_t* regions, int region_count,
                                int world_x, int world_y, int fallback);
void Port_VillageGroups_BuildCells(const uint16_t* regions, int region_count,
                                   int group_count, int selected, int xdiff, int ydiff,
                                   int cols, int rows, uint8_t* cells);

#ifdef __cplusplus
}
#endif

#endif
