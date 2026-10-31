/* Per-cell charset group selection in the production software PPU.
 * A taller Minish Village view can show two streamed tileset groups that
 * cannot both live in GBA char VRAM; the port binds a host charset/LUT for
 * the non-resident group. This verifies the tile fetch and palette selection. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cpu/mode1.h"
#include "virtuappu.h"

uint32_t virtuappu_frame_buffer[VIRTUAPPU_FRAME_BUFFER_SIZE];

static uint8_t io[0x400], vram[0x18000];
static uint16_t bgpal[256], objpal[256], oam[512];
static uint8_t chars[2 * 0xC000];
static uint32_t luts[2 * MODE1_PALETTE_COLORS];
static uint8_t cells[32 * 32];
static int failures;

static void expect(const char* name, uint32_t got, uint32_t want) {
    if (got != want) {
        fprintf(stderr, "%s: got %08x, expected %08x\n", name, got, want);
        failures++;
    }
}

int main(void) {
    VirtuaPPUMode1GbaMemory mem = {io, vram, bgpal, objpal, oam};
    virtuappu_mode1_bind_gba_memory(&mem);
    PPUMemory p = {0};
    p.mode = 1;
    p.frame_width = 240;
    p.frame_pitch = MODE1_GBA_WIDTH;
    p.frame_height = 160;

    /* BG1 on, 4bpp, char base 0, screen base 30 (0xF000), 32x32 map. */
    io[1] = 0x02;
    io[10] = 0x00;
    io[11] = 0x1E;
    /* Native tile 1 = colour 1; host group tile 1 = colour 2. */
    memset(vram + 0x20, 0x11, 32);
    memset(chars + 0x20, 0x22, 32);
    bgpal[1] = 0x001F; /* red */
    bgpal[2] = 0x03E0; /* green */
    luts[2] = virtuappu_mode1_rgb555_to_abgr8888(0x7C00); /* blue */
    uint16_t* screen = (uint16_t*)(vram + 0xF000);
    for (int i = 0; i < 32 * 32; ++i)
        screen[i] = 1;

    memset(cells, 0xFF, sizeof(cells));
    virtuappu_mode1_bind_bg_group_cells(cells, 32, 32, luts, 2, chars, 0xC000, 1u << 1);
    virtuappu_mode1_render_frame(&p);
    expect("selected cell uses VRAM palette", virtuappu_frame_buffer[0], 0xff0000f8);

    /* Tile column 1 uses group 0 (host chars + LUT). */
    for (int row = 0; row < 32; ++row)
        cells[row * 32 + 1] = 0;
    virtuappu_mode1_render_frame(&p);
    expect("group cell uses host charset", virtuappu_frame_buffer[8], 0xfff80000);
    expect("neighbour stays native", virtuappu_frame_buffer[0], 0xff0000f8);

    /* A group id past the published count falls back to native VRAM. */
    for (int row = 0; row < 32; ++row)
        cells[row * 32 + 1] = 2;
    virtuappu_mode1_render_frame(&p);
    expect("unknown group falls back to VRAM", virtuappu_frame_buffer[8], 0xff0000f8);

    /* Clear restores the native path. */
    virtuappu_mode1_bind_bg_group_cells(cells, 32, 32, luts, 2, chars, 0xC000, 1u << 1);
    virtuappu_mode1_clear_bg_group_cells();
    virtuappu_mode1_render_frame(&p);
    expect("cleared binding uses VRAM", virtuappu_frame_buffer[8], 0xff0000f8);

    /* Reveal-shadow columns (x >= 240) use their own group grid. */
    static uint16_t shadow_map[32 * MODE1_WS_SHADOW_COLS];
    for (int i = 0; i < 32 * MODE1_WS_SHADOW_COLS; ++i)
        shadow_map[i] = 1;
    static uint8_t shadow_cells[32 * 32];
    memset(shadow_cells, 0xFF, sizeof(shadow_cells));
    for (int row = 0; row < 32; ++row)
        shadow_cells[row * 32 + 1] = 0;
    virtuappu_mode1_ws_shadow[1] = shadow_map;
    virtuappu_mode1_ws_shadow_base_tile[1] = 29;
    p.frame_width = 256;
    virtuappu_mode1_bind_bg_group_cells(cells, 32, 32, luts, 2, chars, 0xC000, 1u << 1);
    virtuappu_mode1_bind_bg_group_shadow(shadow_cells, 32, 32);
    /* Before the 256-px wrap the live VRAM columns are authoritative. */
    virtuappu_mode1_ws_shadow_start_x[1] = 256;
    virtuappu_mode1_render_frame(&p);
    expect("pre-wrap cell uses live VRAM", virtuappu_frame_buffer[240], 0xff0000f8);
    virtuappu_mode1_ws_shadow_start_x[1] = 240;
    virtuappu_mode1_render_frame(&p);
    expect("shadow cell uses host charset", virtuappu_frame_buffer[240], 0xfff80000);
    expect("native cell stays VRAM", virtuappu_frame_buffer[200], 0xff0000f8);
    virtuappu_mode1_clear_bg_group_shadow();
    virtuappu_mode1_render_frame(&p);
    expect("cleared shadow grid uses VRAM", virtuappu_frame_buffer[240], 0xff0000f8);
    virtuappu_mode1_ws_shadow[1] = NULL;
    virtuappu_mode1_ws_shadow_start_x[1] = 240;
    p.frame_width = 240;

    /* Invalid input fails closed. */
    virtuappu_mode1_bind_bg_group_cells(NULL, 32, 32, luts, 2, chars, 0xC000, 2);
    virtuappu_mode1_render_frame(&p);
    expect("null cells fail closed", virtuappu_frame_buffer[8], 0xff0000f8);

    if (failures) {
        fprintf(stderr, "ppu group cells: %d failure(s)\n", failures);
        return 1;
    }
    puts("ppu group cells: per-cell charset and palette selection PASS");
    return 0;
}
