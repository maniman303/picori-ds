/* Expected-pixel regressions for the real CPU PPU; no ROM or SDL required. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cpu/mode1.h"
#include "virtuappu.h"
uint32_t virtuappu_frame_buffer[VIRTUAPPU_FRAME_BUFFER_SIZE];
static uint8_t io[0x400], vram[0x18000];
static uint16_t bgpal[256], objpal[256], oam[512];
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
    for (int i = 0; i < 128; ++i) oam[i * 4] = 0x200;
    bgpal[0] = bgpal[2] = 0x03e0;
    bgpal[1] = objpal[1] = 0x001f;
    memset(vram + 0x10000, 0x11, 32);
    uint16_t* screen = (uint16_t*)(vram + 0xf000);
    PPUMemory p = {0};
    p.frame_pitch = MODE1_GBA_WIDTH;
    /* Narrow view renders 212 columns at the native pitch. World/affine
     * backgrounds, sprites and window masks must match a centered crop. */
    static uint32_t native[240 * 160];
    virtuappu_mode1_ws_shadow[3] = NULL;
    memset(io, 0, sizeof(io));
    io[0] = 1; io[1] = 4 | 16 | 32; /* affine BG2, OBJ, WIN0 */
    io[0xc] = 4; io[0xd] = 0x3e; /* charblock 1, screenblock 30, wrap */
    io[0x21] = io[0x27] = 1; /* PA=PD=256 */
    io[0x40] = 224; io[0x41] = 8;
    io[0x44] = 160; io[0x45] = 0;
    io[0x48] = 0x14; /* BG2 and OBJ inside window, backdrop outside */
    for (int i = 0; i < 256; ++i) vram[0xf000 + i] = 1 + (i & 1);
    memset(vram + 0x4040, 1, 64);
    memset(vram + 0x4080, 2, 64);
    oam[0] = 16; oam[1] = 220; oam[2] = 0;
    p.mode = 2; p.frame_width = 240;
    virtuappu_mode1_render_frame(&p);
    for (int y = 0; y < 160; ++y)
        memcpy(native + y * 240, virtuappu_frame_buffer + y * MODE1_GBA_WIDTH, 240 * sizeof(uint32_t));
    p.frame_width = 212;
    for (int i = 0; i < MODE1_GBA_WIDTH * 160; ++i) virtuappu_frame_buffer[i] = 0x12345678;
    virtuappu_mode1_render_frame(&p);
    for (int y = 0; y < 160; ++y) {
        for (int x = 0; x < 212; ++x)
            expect("212px centered affine/world/sprite/window", virtuappu_frame_buffer[y * MODE1_GBA_WIDTH + x], native[y * 240 + x + 14]);
        expect("212px untouched stride padding", virtuappu_frame_buffer[y * MODE1_GBA_WIDTH + 212], 0x12345678);
    }
    /* Left hearts and right rupees retain their edges. Compact dialogue
     * rows sample the reflowed canvas rather than moving its right half. */
    memset(io, 0, sizeof(io));
    memset(vram + 0x4020, 0x11, 32);
    memset(vram + 0x4040, 0x22, 32);
    io[1] = 1; io[8] = 4; io[9] = 0x1e;
    for (int row = 0; row < 32; ++row) {
        for (int col = 0; col < 32; ++col) screen[row * 32 + col] = 0;
        screen[row * 32 + 1] = screen[row * 32 + 24] = 1;
        screen[row * 32 + 20] = 2;
    }
    virtuappu_mode1_narrow_hud = 1;
    virtuappu_mode1_narrow_msg_y0 = 120;
    virtuappu_mode1_narrow_msg_y1 = 152;
    p.mode = 1;
    virtuappu_mode1_render_frame(&p);
    expect("narrow hearts retain left margin", virtuappu_frame_buffer[8], 0xff0000f8);
    expect("narrow rupees move inward", virtuappu_frame_buffer[164], 0xff0000f8);
    expect("narrow dialogue keeps its own columns", virtuappu_frame_buffer[120 * MODE1_GBA_WIDTH + 164], 0xff00f800);
    virtuappu_mode1_narrow_hud = 0;
    virtuappu_mode1_narrow_msg_y0 = virtuappu_mode1_narrow_msg_y1 = 0;
    /* The 13-column keyboard moves whole 8px glyphs to 15px cells. */
    memset(io, 0, sizeof(io));
    io[1] = 2; io[0xa] = 4; io[0xb] = 0x1e;
    memset(screen, 0, 32 * 32 * sizeof(uint16_t));
    memset(vram + 0x4020, 0x21, 32);
    for (int col = 0; col < 13; ++col) screen[6 * 32 + 3 + col * 2] = 1;
    virtuappu_mode1_narrow_keyboard = 1;
    virtuappu_mode1_render_frame(&p);
    for (int col = 0; col < 13; ++col) for (int px = 0; px < 8; ++px)
        expect("narrow keyboard retains every glyph pixel",
               virtuappu_frame_buffer[48 * MODE1_GBA_WIDTH + 12 + col * 15 + px],
               px & 1 ? 0xff00f800 : 0xff0000f8);
    for (int x = 203; x < 212; ++x)
        expect("keyboard suppresses former last-column glyph", virtuappu_frame_buffer[48 * MODE1_GBA_WIDTH + x], 0xff00f800);
    virtuappu_mode1_narrow_keyboard = 0;
    if (!failures) puts("212x160 CPU PPU: PASS");
    return failures != 0;
}
