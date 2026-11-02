/* Fixed-menu OBJ margin overlay: sprites that slide past the native border
 * keep drawing into the added margins, blend with the composed background by
 * the port opacity, and never touch the native rectangle. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cpu/mode1.h"
#include "virtuappu.h"

uint32_t virtuappu_frame_buffer[VIRTUAPPU_FRAME_BUFFER_SIZE];
static uint8_t io[0x400], vram[0x18000];
static uint16_t bgpal[256], objpal[256], oam[512];
static int failures;

#define CANVAS_W 288
#define CANVAS_H 162
#define NATIVE_X 24
#define NATIVE_Y 1
static uint32_t canvas[CANVAS_W * CANVAS_H];

static void check(const char* name, uint32_t got, uint32_t want) {
    if (got != want) {
        fprintf(stderr, "%s: got %08x, expected %08x\n", name, got, want);
        ++failures;
    }
}

static uint32_t blend5(uint32_t top, uint32_t bottom, int eva, int evb) {
    int tr = (int)((top >> 0u) & 0xFFu) >> 3;
    int tg = (int)((top >> 8u) & 0xFFu) >> 3;
    int tb = (int)((top >> 16u) & 0xFFu) >> 3;
    int br = (int)((bottom >> 0u) & 0xFFu) >> 3;
    int bg = (int)((bottom >> 8u) & 0xFFu) >> 3;
    int bb = (int)((bottom >> 16u) & 0xFFu) >> 3;
    int r = (tr * eva + br * evb) >> 4;
    int g = (tg * eva + bg * evb) >> 4;
    int b = (tb * eva + bb * evb) >> 4;
    if (r > 31) r = 31;
    if (g > 31) g = 31;
    if (b > 31) b = 31;
    return 0xFF000000u | ((uint32_t)(b << 3) << 16u) | ((uint32_t)(g << 3) << 8u) | (uint32_t)(r << 3);
}

static uint32_t pixel(int x, int y) { return canvas[(size_t)y * CANVAS_W + (size_t)x]; }

static void reset(void) {
    memset(io, 0, sizeof(io));
    memset(vram, 0, sizeof(vram));
    memset(bgpal, 0, sizeof(bgpal));
    memset(objpal, 0, sizeof(objpal));
    memset(oam, 0, sizeof(oam));
    for (size_t i = 0; i < sizeof(canvas) / sizeof(canvas[0]); ++i)
        canvas[i] = 0xff404040u;
    /* One opaque 8x8 4bpp sprite tile: palette index 1 everywhere. */
    bgpal[0] = 0x0000;
    objpal[1] = 0x001f; /* red */
    memset(vram + 0x10000, 0x11, 32);
    /* 8x8 OBJ at OAM slot 0, normal mode. attr0 y=0, attr1 x=0, tile 0.
     * Every other slot is hardware-disabled so it can't fall back to x=0. */
    oam[0] = 0x0000;
    oam[1] = 0x0000;
    oam[2] = 0x0000;
    for (int i = 1; i < MODE1_GBA_OAM_COUNT; ++i)
        oam[i * 4] = 0x0200;
    io[0x01] = 0x10; /* DISPCNT OBJ on */
}

static void render_reference(void) {
    PPUMemory p = {0};
    p.frame_width = 240;
    p.frame_pitch = 240;
    p.frame_height = 160;
    p.mode = 0;
    virtuappu_mode1_render_frame(&p);
}

int main(void) {
    VirtuaPPUMode1GbaMemory mem = {io, vram, bgpal, objpal, oam};
    static VirtuaPPUObjPosition positions[MODE1_GBA_OAM_COUNT];
    static uint8_t opacity[MODE1_GBA_OAM_COUNT];
    uint32_t red;
    const uint32_t background = 0xff404040u;

    virtuappu_mode1_bind_gba_memory(&mem);
    reset();
    render_reference();
    red = virtuappu_mode1_rgb555_to_abgr8888(objpal[1]);

    /* Sprite native x=-8 crosses the left border; native_y=1 puts its rows at
     * canvas y=1..8. It must appear at canvas x=16..23 and nowhere else. */
    memset(positions, 0, sizeof(positions));
    positions[0].attr0 = oam[0];
    positions[0].attr1 = oam[1];
    positions[0].x = -8;
    positions[0].y = 0;
    positions[0].flags = VIRTUAPPU_OBJ_POSITION_VALID;
    for (int i = 0; i < MODE1_GBA_OAM_COUNT; ++i) opacity[i] = 255;

    check("opaque overlay draws margins",
          virtuappu_mode1_overlay_obj_margins(canvas, CANVAS_W, CANVAS_W, CANVAS_H,
                                              NATIVE_X, NATIVE_Y, positions, opacity) ? 1u : 0u, 1u);
    check("opaque sprite colour", pixel(16, 1), red);
    check("sprite extends to border", pixel(23, 8), red);
    check("native rectangle untouched", pixel(24, 1), background);
    check("outside sprite untouched", pixel(15, 1), background);
    check("row below sprite untouched", pixel(16, 9), background);

    /* Half opacity blends with the composed margin background using the GBA
     * 5-bit alpha path. */
    reset();
    opacity[0] = 128;
    virtuappu_mode1_overlay_obj_margins(canvas, CANVAS_W, CANVAS_W, CANVAS_H,
                                        NATIVE_X, NATIVE_Y, positions, opacity);
    check("half-opacity blend", pixel(16, 1), blend5(red, background, 8, 8));

    /* Zero opacity leaves the background intact and reports nothing drawn. */
    reset();
    opacity[0] = 0;
    check("zero opacity draws nothing",
          virtuappu_mode1_overlay_obj_margins(canvas, CANVAS_W, CANVAS_W, CANVAS_H,
                                              NATIVE_X, NATIVE_Y, positions, opacity) ? 1u : 0u, 0u);
    check("zero opacity pixel", pixel(16, 1), background);

    /* Sprites fully inside the native rectangle never reach the margins. */
    reset();
    positions[0].x = 100;
    opacity[0] = 255;
    check("inside sprite draws no margins",
          virtuappu_mode1_overlay_obj_margins(canvas, CANVAS_W, CANVAS_W, CANVAS_H,
                                              NATIVE_X, NATIVE_Y, positions, opacity) ? 1u : 0u, 0u);
    check("inside sprite leaves margin", pixel(16, 1), background);

    /* Canvas-tagged HUD sprites use the same native-space coordinates; the tag
     * only suppresses the live-world aperture offset. A right-margin sprite is
     * shifted once into canvas space. */
    reset();
    positions[0].x = 250;
    positions[0].y = 5;
    positions[0].flags = VIRTUAPPU_OBJ_POSITION_VALID | VIRTUAPPU_OBJ_POSITION_CANVAS;
    opacity[0] = 255;
    virtuappu_mode1_overlay_obj_margins(canvas, CANVAS_W, CANVAS_W, CANVAS_H,
                                        NATIVE_X, NATIVE_Y, positions, opacity);
    check("canvas-tagged sprite shifted once", pixel(274, 6), red);

    /* Native 240-wide canvases are left to the legacy path. */
    check("native canvas rejected",
          virtuappu_mode1_overlay_obj_margins(canvas, CANVAS_W, 240, 160, 0, 0,
                                              positions, opacity) ? 1u : 0u, 0u);

    if (!failures)
        puts("fixed-menu OBJ margin overlay: border crossing, opacity and native isolation PASS");
    return failures != 0;
}
