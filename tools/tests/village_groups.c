/* Pure Minish Village group-grid policy: region lookup and VRAM-cell layout.
 * The PPU consumes this grid; see ppu_group_cells_regression_test.py for the
 * rendered result. No ROM, SDL or engine globals required. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "port_village_groups.h"

enum { FALLBACK = 0xFF };

static const uint16_t regions[][5] = {
    {0, 0, 0, 16, 16},  /* group 0: world x/y 0..15 */
    {1, 16, 0, 16, 16}, /* group 1: world x 16..31, y 0..15 */
};

static int region_at(int x, int y, int fallback) {
    return Port_VillageGroups_RegionAt(&regions[0][0], 2, x, y, fallback);
}

/* A group may own several disjoint rectangles; region count is not group
 * count. The fourth entry must still be considered. */
static const uint16_t repeated[][5] = {
    {0, 0, 0, 8, 8},
    {1, 8, 0, 8, 8},
    {0, 16, 0, 8, 8},
};

static void check_regions(void) {
    assert(region_at(0, 0, FALLBACK) == 0);
    assert(region_at(15, 15, FALLBACK) == 0);
    assert(region_at(16, 0, FALLBACK) == 1);
    assert(region_at(31, 15, FALLBACK) == 1);
    assert(region_at(16, 16, FALLBACK) == FALLBACK);
    assert(region_at(-1, 0, FALLBACK) == FALLBACK);
    /* Out-of-range group ids fall back through the caller. */
    assert(Port_VillageGroups_RegionAt(&regions[0][0], 0, 0, 0, 4) == 4);
    assert(Port_VillageGroups_RegionAt(NULL, 2, 0, 0, 4) == 4);
    assert(Port_VillageGroups_RegionAt(&repeated[0][0], 3, 20, 4, 4) == 0);
    assert(Port_VillageGroups_RegionAt(&repeated[0][0], 3, 12, 4, 4) == 1);
}

static void check_cells(int selected) {
    uint8_t cells[8 * 4];
    memset(cells, 0xEE, sizeof(cells));
    Port_VillageGroups_BuildCells(&regions[0][0], 2, 2, selected, 0, 0, 8, 4, cells);
    /* Row 0 duplicates world row 0 while ydiff < 8. Columns 0..1 are group 0,
     * columns 2..3 are group 1, the rest are outside both regions. The
     * selected group stays on VRAM (0xFF). */
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 8; ++c) {
            int group = c < 2 ? 0 : (c < 4 ? 1 : -1);
            uint8_t want = FALLBACK;
            if (group >= 0 && group != selected)
                want = (uint8_t)group;
            assert(cells[r * 8 + c] == want);
        }
    }
    /* Row 3 is world row 2: outside both regions -> selected/native. */
    for (int c = 0; c < 8; ++c)
        assert(cells[3 * 8 + c] == FALLBACK);
}

static void check_scroll_offsets(void) {
    uint8_t cells[8 * 4];
    /* ydiff 8: no duplicate row; buffer row 0 is the padding row (-1). */
    Port_VillageGroups_BuildCells(&regions[0][0], 2, 2, 0, 0, 8, 8, 4, cells);
    for (int c = 0; c < 8; ++c) {
        assert(cells[0 * 8 + c] == FALLBACK);
        assert(cells[1 * 8 + c] == (c == 2 || c == 3 ? 1 : FALLBACK));
    }
    /* xdiff 16 starts at world column 2, the first group-1 column. */
    Port_VillageGroups_BuildCells(&regions[0][0], 2, 2, 0, 16, 0, 8, 4, cells);
    assert(cells[1 * 8 + 0] == 1);
    assert(cells[1 * 8 + 1] == 1);
    assert(cells[1 * 8 + 2] == FALLBACK); /* world col 4: no region */
}

static void check_no_selection(void) {
    uint8_t cells[8 * 4];
    Port_VillageGroups_BuildCells(&regions[0][0], 2, 2, FALLBACK, 0, 0, 8, 4, cells);
    for (int i = 0; i < 8 * 4; ++i)
        assert(cells[i] == FALLBACK);
}

int main(void) {
    check_regions();
    check_cells(0);
    check_cells(1);
    check_scroll_offsets();
    check_no_selection();
    puts("village group grid: region lookup, padding row and VRAM selection PASS");
    return 0;
}
