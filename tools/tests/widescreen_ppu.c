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
static uint32_t abgr(uint16_t color) {
    return virtuappu_mode1_rgb555_to_abgr8888(color);
}
int main(void) {
    VirtuaPPUMode1GbaMemory mem = {io, vram, bgpal, objpal, oam};
    virtuappu_mode1_bind_gba_memory(&mem);
    for (int i = 0; i < 128; i++) oam[i * 4] = 0x200;
    io[1] = 0x10; /* OBJ enabled; BGs transparent. */
    bgpal[0] = 0x03e0;
    objpal[1] = 0x001f;
    memset(vram + 0x10000, 0x11, 32);
    oam[0] = 16; oam[1] = 236; oam[2] = 0;
    PPUMemory p = {0};
    p.mode = 1; p.frame_width = 284; p.frame_pitch = MODE1_GBA_WIDTH;
    virtuappu_mode1_render_frame(&p);
    expect("sprite left of seam", virtuappu_frame_buffer[16 * MODE1_GBA_WIDTH + 239], 0xff0000f8);
    expect("sprite right of seam", virtuappu_frame_buffer[16 * MODE1_GBA_WIDTH + 240], 0xff0000f8);
    expect("backdrop right of seam", virtuappu_frame_buffer[240], 0xff00f800);
    /* Window-hidden layers must still reveal the real backdrop. */
    io[1] |= 0x20; /* WIN0 enabled, zero window/outside controls. */
    virtuappu_mode1_render_frame(&p);
    expect("window masks sprite", virtuappu_frame_buffer[16 * MODE1_GBA_WIDTH + 240], 0xff00f800);
    /* Native fallback must not touch padding in the wider-pitch buffer. */
    p.frame_width = 240;
    for (int i = 0; i < MODE1_GBA_WIDTH * 160; i++) virtuappu_frame_buffer[i] = 0x12345678;
    virtuappu_mode1_render_frame(&p);
    expect("native fallback padding", virtuappu_frame_buffer[240], 0x12345678);
    /* Woods fog is a repeating screenblock, including scanline-varying HOFS.
     * Its shadow must represent every tile column, not just the camera reveal. */
    static uint16_t overlay[32 * MODE1_WS_SHADOW_COLS];
    memset(io, 0, sizeof(io));
    io[1] = 8; /* BG3 only. */
    io[0xe] = 4; io[0xf] = 0x1e; /* char base 1, screen base 30 */
    bgpal[1] = 0x001f; bgpal[2] = 0x03e0;
    memset(vram + 0x4020, 0x11, 32);
    memset(vram + 0x4040, 0x22, 32);
    uint16_t* screen = (uint16_t*)(vram + 0xf000);
    for (int row = 0; row < 32; ++row) {
        for (int col = 0; col < 32; ++col) {
            uint16_t entry = 1 + (col & 1);
            screen[row * 32 + col] = entry;
            if (col < MODE1_WS_SHADOW_COLS)
                overlay[row * MODE1_WS_SHADOW_COLS + col] = entry;
        }
    }
    virtuappu_mode1_ws_shadow[3] = overlay;
    virtuappu_mode1_ws_shadow_base_tile[3] = 0;
    p.frame_width = 384;
    for (int offset = 0; offset < 256; offset += 7) {
        io[0x1c] = offset;
        virtuappu_mode1_render_frame(&p);
        for (int x = 240; x < 384; ++x) {
            uint32_t color = (((x + offset) / 8) & 1) ? 0xff00f800 : 0xff0000f8;
            expect("repeating fog past native edge", virtuappu_frame_buffer[x], color);
            if (failures) break;
        }
        if (failures) break;
    }
    /* The right HUD remap owns only the normal key/rupee cells. Fill BG0
     * with opaque pixels so accidental movement of unrelated UI is visible. */
    virtuappu_mode1_ws_shadow[3] = NULL;
    memset(io, 0, sizeof(io));
    io[1] = 1; /* BG0 only */
    io[8] = 4; io[9] = 0x1e; /* char base 1, screen base 30 */
    for (int i = 0; i < 32 * 32; ++i) screen[i] = 1;
    virtuappu_mode1_ws_hud_right_anchor = MODE1_WS_HUD_KEYS | MODE1_WS_HUD_RUPEES;
    p.frame_width = 284;
    virtuappu_mode1_render_frame(&p);
    expect("non-HUD upper right stays native", virtuappu_frame_buffer[80 * MODE1_GBA_WIDTH + 180], 0xff0000f8);
    expect("non-HUD lower right stays native", virtuappu_frame_buffer[136 * MODE1_GBA_WIDTH + 192], 0xff0000f8);
    expect("key original suppressed", virtuappu_frame_buffer[136 * MODE1_GBA_WIDTH + 200], 0xff00f800);
    expect("key moved", virtuappu_frame_buffer[136 * MODE1_GBA_WIDTH + 244], 0xff0000f8);
    expect("rupee original suppressed", virtuappu_frame_buffer[152 * MODE1_GBA_WIDTH + 192], 0xff00f800);
    expect("rupee moved", virtuappu_frame_buffer[152 * MODE1_GBA_WIDTH + 236], 0xff0000f8);
    expect("right margin remains clear", virtuappu_frame_buffer[152 * MODE1_GBA_WIDTH + 278], 0xff00f800);
    virtuappu_mode1_ws_hud_right_anchor = MODE1_WS_HUD_KEYS;
    virtuappu_mode1_render_frame(&p);
    expect("visible key still moved", virtuappu_frame_buffer[136 * MODE1_GBA_WIDTH + 244], 0xff0000f8);
    expect("unowned rupee cells stay native", virtuappu_frame_buffer[152 * MODE1_GBA_WIDTH + 192], 0xff0000f8);
    expect("unowned rupee cells not copied", virtuappu_frame_buffer[152 * MODE1_GBA_WIDTH + 244], 0xff00f800);
    virtuappu_mode1_ws_hud_right_anchor = MODE1_WS_HUD_KEYS | MODE1_WS_HUD_RUPEES;
    /* Message lines take ownership of BG0 before the HUD rectangle remap. */
    virtuappu_mode1_ws_msg_shift = 22;
    virtuappu_mode1_ws_msg_x0 = 8; virtuappu_mode1_ws_msg_x1 = 224;
    virtuappu_mode1_ws_msg_y0 = 128; virtuappu_mode1_ws_msg_y1 = 160;
    virtuappu_mode1_render_frame(&p);
    expect("message overlap takes priority", virtuappu_frame_buffer[136 * MODE1_GBA_WIDTH + 210], 0xff0000f8);
    expect("HUD absent on message line", virtuappu_frame_buffer[136 * MODE1_GBA_WIDTH + 260], 0xff00f800);
    virtuappu_mode1_ws_msg_shift = 0;
    /* An unknown scrolling BG0 must fail closed rather than move arbitrary cells. */
    io[0x10] = 1;
    virtuappu_mode1_render_frame(&p);
    expect("scrolled BG0 stays native", virtuappu_frame_buffer[136 * MODE1_GBA_WIDTH + 200], 0xff0000f8);
    expect("scrolled BG0 is clipped", virtuappu_frame_buffer[136 * MODE1_GBA_WIDTH + 244], 0xff00f800);

    /* Tall canvas: message and lower-right HUD keep native tiles but move to
     * the canvas corners. Each two-row band uses a separate palette bank. */
    memset(io, 0, sizeof(io));
    io[1] = 1; /* BG0 only */
    io[8] = 4; io[9] = 0x1e; /* char base 1, screen base 30 */
    for (int bank = 2; bank < 16; ++bank) {
        bgpal[bank * 16 + 1] = (uint16_t)(0x001f | ((bank & 0x1f) << 5));
        bgpal[bank * 16 + 2] = (uint16_t)(0x03e0 | ((bank & 0x1f) << 10));
    }
    memset(vram + 0x4020, 0x11, 32); /* tile 1: every pixel is color index 1 */
    for (int row = 0; row < 32; ++row) {
        uint16_t entry = (uint16_t)(1u | ((uint16_t)(2 + (row / 2) % 14) << 12));
        for (int col = 0; col < 32; ++col) screen[row * 32 + col] = entry;
    }
    p.frame_width = 256; p.frame_height = 192;
    virtuappu_mode1_ws_hud_right_anchor = 0;
    virtuappu_mode1_ws_hud_shift_y = 0;
    virtuappu_mode1_ws_msg_shift = 8;
    virtuappu_mode1_ws_msg_shift_y = 32;
    virtuappu_mode1_ws_msg_x0 = 8;
    virtuappu_mode1_ws_msg_x1 = 232;
    virtuappu_mode1_ws_msg_y0 = 96;
    virtuappu_mode1_ws_msg_y1 = 144;
    virtuappu_mode1_render_frame(&p);
    /* Source row 100 (bank 8) -> destination row 132; x 100 -> 108. */
    expect("tall message moved down/right", virtuappu_frame_buffer[132 * MODE1_GBA_WIDTH + 108],
           abgr(bgpal[8 * 16 + 1]));
    expect("tall native message band suppressed", virtuappu_frame_buffer[100 * MODE1_GBA_WIDTH + 100],
           abgr(bgpal[0]));
    expect("tall BG0 outside message band stays native", virtuappu_frame_buffer[100 * MODE1_GBA_WIDTH + 4],
           abgr(bgpal[8 * 16 + 1]));
    /* A top-requested window uses the same final bottom anchor. */
    virtuappu_mode1_ws_msg_shift_y = 120;
    virtuappu_mode1_ws_msg_y0 = 8;
    virtuappu_mode1_ws_msg_y1 = 56;
    virtuappu_mode1_render_frame(&p);
    expect("tall top message shares bottom anchor", virtuappu_frame_buffer[132 * MODE1_GBA_WIDTH + 108],
           abgr(bgpal[2 * 16 + 1]));
    expect("tall top source band suppressed", virtuappu_frame_buffer[12 * MODE1_GBA_WIDTH + 100],
           abgr(bgpal[0]));

    virtuappu_mode1_ws_msg_shift = 0;
    virtuappu_mode1_ws_msg_shift_y = 0;
    virtuappu_mode1_ws_msg_x0 = 0;
    virtuappu_mode1_ws_msg_x1 = 0;
    virtuappu_mode1_ws_msg_y0 = 0;
    virtuappu_mode1_ws_msg_y1 = 0;
    virtuappu_mode1_ws_hud_right_anchor = MODE1_WS_HUD_KEYS | MODE1_WS_HUD_RUPEES;
    virtuappu_mode1_ws_hud_shift_y = 32;
    virtuappu_mode1_render_frame(&p);
    expect("tall key moved down/right", virtuappu_frame_buffer[168 * MODE1_GBA_WIDTH + 216],
           abgr(bgpal[10 * 16 + 1]));
    expect("tall native key cells suppressed", virtuappu_frame_buffer[136 * MODE1_GBA_WIDTH + 200],
           abgr(bgpal[0]));
    expect("tall rupee moved down/right", virtuappu_frame_buffer[184 * MODE1_GBA_WIDTH + 208],
           abgr(bgpal[11 * 16 + 1]));
    /* An uninstalled counter keeps its native cells instead of blanking them. */
    virtuappu_mode1_ws_hud_right_anchor = MODE1_WS_HUD_KEYS;
    virtuappu_mode1_render_frame(&p);
    expect("tall uninstalled rupee stays native", virtuappu_frame_buffer[152 * MODE1_GBA_WIDTH + 192],
           abgr(bgpal[11 * 16 + 1]));

    if (!failures) puts("widescreen CPU PPU: PASS");
    return failures != 0;
}
