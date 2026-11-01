/* Fixed-screen title effect margins: live non-art BG layers composite over
 * caller-provided artwork rows across the full canvas, and native pixels are
 * never produced by this path. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cpu/mode1.h"
#include "virtuappu.h"

uint32_t virtuappu_frame_buffer[VIRTUAPPU_FRAME_BUFFER_SIZE];
static uint8_t io[0x400], vram[0x18000];
static uint16_t bgpal[256], objpal[256], oam[512];
static int failures;

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

static void render_reference(int mode) {
    PPUMemory p = {0};
    p.frame_width = 240;
    p.frame_pitch = 240;
    p.frame_height = 160;
    p.mode = (uint8_t)mode;
    virtuappu_mode1_render_frame(&p);
}

static void reset(void) {
    memset(io, 0, sizeof(io));
    memset(vram, 0, sizeof(vram));
    memset(bgpal, 0, sizeof(bgpal));
    memset(objpal, 0, sizeof(objpal));
    memset(oam, 0, sizeof(oam));
    memset(virtuappu_frame_buffer, 0x53, sizeof(virtuappu_frame_buffer));
}

static void fill_row(uint32_t* row, int width) {
    for (int x = 0; x < width; ++x) row[x] = 0xff404040u;
}

int main(void) {
    VirtuaPPUMode1GbaMemory mem = {io, vram, bgpal, objpal, oam};
    virtuappu_mode1_bind_gba_memory(&mem);

    /* BG0 text rays: their tilemap is only 32 tiles wide, but the compositor
     * must sample columns past 240 so 4:3/16:9 margins receive the overlay.
     * Alpha blending uses the artwork row as BG1, the BLDCNT second target. */
    reset();
    bgpal[0] = 0x0000;
    bgpal[1] = 0x001f; /* red */
    bgpal[2] = 0x03e0; /* green */
    memset(vram + 0x20, 1, 32);
    {
        uint16_t* bg0map = (uint16_t*)(vram + 29 * 0x800);
        for (int i = 0; i < 32 * 32; ++i) bg0map[i] = 0;
        bg0map[31] = 1; /* x=248..255 */
        bg0map[27] = 1; /* x=216..223, for the top-margin x-shift check */
    }
    io[0x08] = 0x02; io[0x09] = 0x1d; /* BG0 priority 2, screen base 29 */
    io[0x0a] = 0x03; io[0x0b] = 0x1e; /* BG1 priority 3, screen base 30 */
    io[0x01] = 0x03;                   /* BG0 + BG1 on */
    io[0x50] = 0x41; io[0x51] = 0x02;  /* alpha blend: BG0 over BG1 */
    io[0x52] = 0x08; io[0x53] = 0x08;
    render_reference(1);

    static uint32_t background[MODE1_GBA_WIDTH], output[MODE1_GBA_WIDTH];
    fill_row(background, 256);
    check("text overlay changed", virtuappu_mode1_composite_overlay_line(0, 0, 256, background, output, 1, 1),
          1u);
    const uint32_t red = virtuappu_mode1_rgb555_to_abgr8888(bgpal[1]);
    check("text ray extends past 240", output[250], blend5(red, background[250], 8, 8));
    check("text transparent column untouched", output[247], background[247]);

    /* Rays extend only to the right; the left margin must stay clean. */
    fill_row(background, 288);
    virtuappu_mode1_composite_overlay_line(0, -8, 288, background, output, 1, 1);
    check("left margin has no rays", output[0], background[0]);
    check("native-aligned column resumes at 0", output[8], background[8]);
    check("right source overflow stays clear", output[280], background[280]);

    /* Top/bottom margins reuse the nearest native text line instead of
     * unrelated wrapped map rows. */
    fill_row(background, 256);
    virtuappu_mode1_composite_overlay_line(-8, 0, 256, background, output, 1, 1);
    check("top border extrapolates the shaft diagonal", output[224], blend5(red, background[224], 8, 8));

    /* A live layer below the opaque artwork layer must not cover it. */
    io[0x08] = 0x03; /* BG0 priority 3 */
    io[0x0a] = 0x02; /* BG1 priority 2 (artwork above) */
    fill_row(background, 256);
    virtuappu_mode1_composite_overlay_line(0, 0, 256, background, output, 1, 1);
    check("lower-priority ray hidden", output[250], background[250]);

    /* Affine BG2 (the sword scale layer): sample beyond 240, but do not wrap
     * source texels into the margin (a centred sword would duplicate). */
    reset();
    bgpal[0] = 0x0000;
    bgpal[2] = 0x03e0; /* green */
    memset(vram + 2 * 64, 2, 64);
    vram[28 * 0x800 + 31] = 2; /* tile 2 at source x=248..255, y=0..7 */
    io[0x0a] = 0x03; io[0x0b] = 0x1e; /* BG1 priority 3 (artwork) */
    io[0x0c] = 0x01; io[0x0d] = 0x7c; /* BG2 priority 1, screen base 28, wrap, 256 */
    io[0x01] = 0x06;                   /* BG1 + BG2 on */
    io[0x20] = 0x00; io[0x21] = 0x01; /* PA = 0x100 */
    io[0x24] = 0x00; io[0x25] = 0x00; /* PC = 0 */
    io[0x28] = io[0x29] = io[0x2a] = io[0x2b] = 0; /* BG2X */
    io[0x2c] = io[0x2d] = io[0x2e] = io[0x2f] = 0; /* BG2Y */
    render_reference(2);

    fill_row(background, 288);
    check("affine overlay changed", virtuappu_mode1_composite_overlay_line(0, 0, 288, background, output, 4, 1),
          1u);
    check("sword extends past 240", output[250], virtuappu_mode1_rgb555_to_abgr8888(bgpal[2]));
    check("affine source wrap suppressed", output[270], background[270]);

    io[0x0c] = 0x03; io[0x0d] = 0x7c; /* BG2 below artwork */
    io[0x0a] = 0x02; io[0x0b] = 0x1e;
    fill_row(background, 288);
    virtuappu_mode1_composite_overlay_line(0, 0, 288, background, output, 4, 1);
    check("sword below artwork hidden", output[250], background[250]);

    if (!failures)
        puts("title effect margin overlay: rays, affine sword and priority PASS");
    return failures != 0;
}
