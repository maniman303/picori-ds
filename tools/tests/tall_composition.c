/* Live-world aperture inside a taller canvas: world BG/OBJ placement, canvas
 * HUD, margin fill and byte-identical legacy behavior when disabled. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cpu/mode1.h"
#include "virtuappu.h"

uint32_t virtuappu_frame_buffer[VIRTUAPPU_FRAME_BUFFER_SIZE];
static uint8_t io[0x400], vram[0x18000];
static uint16_t bgpal[256], objpal[256], oam[512];
static int failures;

static void expect(const char* name, int x, int y, uint32_t want) {
    const uint32_t got = virtuappu_frame_buffer[y * MODE1_GBA_WIDTH + x];
    if (got != want) {
        fprintf(stderr, "%s at (%d,%d): got %08x, expected %08x\n", name, x, y, got, want);
        ++failures;
    }
}

static uint32_t abgr(uint16_t color) {
    return virtuappu_mode1_rgb555_to_abgr8888(color);
}

static void set_oam_sprite(int slot, int x, int y, unsigned tile) {
    oam[slot * 4 + 0] = (uint16_t)y;     /* attr0: 8x8, 4bpp */
    oam[slot * 4 + 1] = (uint16_t)x;     /* attr1: 8x8, no flip */
    oam[slot * 4 + 2] = (uint16_t)tile;  /* attr2: palette 0 */
    oam[slot * 4 + 3] = 0;
}

static void reset(void) {
    memset(io, 0, sizeof(io));
    memset(vram, 0, sizeof(vram));
    memset(bgpal, 0, sizeof(bgpal));
    memset(objpal, 0, sizeof(objpal));
    memset(oam, 0, sizeof(oam));
    for (int i = 0; i < 128; ++i)
        oam[i * 4] = 0x200;
    for (size_t i = 0; i < VIRTUAPPU_FRAME_BUFFER_SIZE; ++i)
        virtuappu_frame_buffer[i] = 0x12345678;
    virtuappu_mode1_pre_line_callback = NULL;
    virtuappu_mode1_continue_constant_hdma = false;
    virtuappu_mode1_bg2x_hdma_strobe = false;
    virtuappu_mode1_bg2y_hdma_strobe = false;

    /* Palette: transparent/black, red BG, green BG, blue OBJ. */
    bgpal[0] = 0x0000;
    bgpal[1] = 0x001f;
    bgpal[2] = 0x03e0;
    objpal[3] = 0x7c00;

    /* BG0 (canvas HUD) screen base 31, one green tile at (0,0). */
    memset(vram + 0x4020, 0x11, 32); /* tile 1 = red */
    memset(vram + 0x4040, 0x22, 32); /* tile 2 = green */
    io[0x08] = 0x04; io[0x09] = 0x1f; /* BG0 char base 1, screen base 31 */
    uint16_t* bg0screen = (uint16_t*)(vram + 0xf800);
    for (int i = 0; i < 32 * 32; ++i)
        bg0screen[i] = 0;
    bg0screen[0] = 2;

    /* BG1 (world) screen base 30, all red. */
    io[0x0a] = 0x04; io[0x0b] = 0x1e; /* BG1 char base 1, screen base 30 */
    uint16_t* bg1screen = (uint16_t*)(vram + 0xf000);
    for (int i = 0; i < 32 * 32; ++i)
        bg1screen[i] = 1;

    /* OBJ tile 3: every pixel is color index 3 (blue). */
    memset(vram + 0x10000 + 3 * 32, 0x33, 32);

    /* GBA mode 0, BG0 + BG1 + OBJ (DISPCNT bits 8/9/12). */
    io[0] = 0x01;
    io[1] = 0x13; /* BG0 on, BG1 on, OBJ on */
}

static PPUMemory canvas_geometry(int width, int height) {
    PPUMemory p = {0};
    p.frame_width = (uint16_t)width;
    p.frame_pitch = (uint16_t)MODE1_GBA_WIDTH;
    p.frame_height = (uint16_t)height;
    p.mode = 1;
    return p;
}

int main(void) {
    VirtuaPPUMode1GbaMemory mem = {io, vram, bgpal, objpal, oam};
    virtuappu_mode1_bind_gba_memory(&mem);
    reset();

    /* Native/wide reference: no world rectangle, then a full-canvas rectangle.
     * Both must be byte-identical to the legacy path. */
    PPUMemory p = canvas_geometry(384, 160);
    virtuappu_mode1_clear_world_rect();
    virtuappu_mode1_render_frame(&p);
    static uint32_t legacy[384 * 160];
    memcpy(legacy, virtuappu_frame_buffer, sizeof(legacy));
    virtuappu_mode1_set_world_rect(0, 0, 384, 160);
    virtuappu_mode1_render_frame(&p);
    if (memcmp(legacy, virtuappu_frame_buffer, sizeof(legacy)) != 0) {
        fprintf(stderr, "full-canvas world rect changed legacy output\n");
        ++failures;
    }
    virtuappu_mode1_clear_world_rect();

    /* 4:3 canvas with a 240x160 world aperture centered at (8,16). */
    reset();
    p = canvas_geometry(256, 192);
    virtuappu_mode1_set_world_rect(8, 16, 240, 160);
    virtuappu_mode1_render_frame(&p);

    /* Margins keep the backdrop; the world is centered inside the aperture. */
    expect("top margin", 100, 0, abgr(bgpal[0]));
    expect("left margin", 0, 100, abgr(bgpal[0]));
    expect("right margin", 248, 100, abgr(bgpal[0]));
    expect("bottom margin", 100, 176, abgr(bgpal[0]));
    expect("world top-left", 8, 16, abgr(bgpal[1]));
    expect("world bottom-right", 247, 175, abgr(bgpal[1]));
    expect("world just outside left", 7, 16, abgr(bgpal[0]));
    expect("world just outside right", 248, 16, abgr(bgpal[0]));

    /* BG0 is canvas-relative even though BG1 is offset. */
    expect("canvas HUD stays at origin", 0, 0, abgr(bgpal[2]));
    expect("world does not leak into HUD cell", 8, 16, abgr(bgpal[1]));

    /* OBJ: world sprites follow the aperture, canvas sprites stay absolute. */
    static VirtuaPPUObjPosition positions[MODE1_GBA_OAM_COUNT];
    memset(positions, 0, sizeof(positions));
    set_oam_sprite(0, 10, 20, 3);
    positions[0].x = 10;
    positions[0].y = 20;
    positions[0].attr0 = oam[0];
    positions[0].attr1 = oam[1];
    positions[0].flags = VIRTUAPPU_OBJ_POSITION_VALID;
    set_oam_sprite(1, 10, 180, 3);
    positions[1].x = 10;
    positions[1].y = 180;
    positions[1].attr0 = oam[4];
    positions[1].attr1 = oam[5];
    positions[1].flags = VIRTUAPPU_OBJ_POSITION_VALID | VIRTUAPPU_OBJ_POSITION_CANVAS;
    virtuappu_mode1_bind_obj_positions(positions);
    virtuappu_mode1_render_frame(&p);
    expect("world sprite follows aperture", 18, 36, abgr(objpal[3]));
    expect("world sprite top-left is shifted", 8, 16, abgr(bgpal[1]));
    expect("canvas sprite outside aperture", 10, 180, abgr(objpal[3]));

    /* A world sprite is clipped to the aperture; a canvas sprite is not. */
    set_oam_sprite(2, 236, 20, 3); /* screen x 244..251, aperture ends 248 */
    positions[2].x = 236;
    positions[2].y = 20;
    positions[2].attr0 = oam[8];
    positions[2].attr1 = oam[9];
    positions[2].flags = VIRTUAPPU_OBJ_POSITION_VALID;
    set_oam_sprite(3, 250, 20, 3); /* canvas x 250..257, frame ends 256 */
    positions[3].x = 250;
    positions[3].y = 20;
    positions[3].attr0 = oam[12];
    positions[3].attr1 = oam[13];
    positions[3].flags = VIRTUAPPU_OBJ_POSITION_VALID | VIRTUAPPU_OBJ_POSITION_CANVAS;
    virtuappu_mode1_render_frame(&p);
    expect("world sprite drawn inside aperture", 247, 36, abgr(objpal[3]));
    expect("world sprite clipped at aperture edge", 248, 36, abgr(bgpal[0]));
    expect("canvas sprite drawn past aperture", 250, 20, abgr(objpal[3]));
    expect("canvas sprite clipped at frame edge", 255, 20, abgr(objpal[3]));

    /* A world sprite below the aperture draws nothing. */
    set_oam_sprite(4, 10, 160, 3); /* screen y 176, aperture ends at 176 */
    positions[4].x = 10;
    positions[4].y = 160;
    positions[4].attr0 = oam[16];
    positions[4].attr1 = oam[17];
    positions[4].flags = VIRTUAPPU_OBJ_POSITION_VALID;
    virtuappu_mode1_render_frame(&p);
    expect("world sprite below aperture clipped", 18, 176, abgr(bgpal[0]));

    /* 288x162 (future proper 16:9): one fill row above and below. */
    reset();
    p = canvas_geometry(288, 162);
    virtuappu_mode1_set_world_rect(24, 1, 240, 160);
    virtuappu_mode1_render_frame(&p);
    expect("16:9 top fill row", 100, 0, abgr(bgpal[0]));
    expect("16:9 world starts at y=1", 24, 1, abgr(bgpal[1]));
    expect("16:9 world ends at y=160", 263, 160, abgr(bgpal[1]));
    expect("16:9 bottom fill row", 100, 161, abgr(bgpal[0]));

    if (!failures)
        puts("tall live-world composition: aperture, HUD, OBJ and fill PASS");
    return failures != 0;
}
