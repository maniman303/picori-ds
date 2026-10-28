#include "port_title_cap.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t canvas[569 * 192], before[569 * 192];
static uint8_t vram[0x18000], io[0x400];
static uint8_t lineio[2][0x400];
static uint16_t palette[256];
static const uint8_t frame[] = {2, 0xc0, 0xf0, 0x71, 0x40, 0x50, 0, 0xf0, 0x71, 0x60, 0x50};
static void io16(int i, unsigned v) { io[i] = v; io[i + 1] = v >> 8; }
static void reset(void) {
    for (unsigned i = 0; i < 569u * 192; ++i) canvas[i] = 0xff183048;
    memcpy(before, canvas, sizeof(canvas));
    memset(vram, 0, sizeof(vram)); memset(io, 0, sizeof(io)); memset(palette, 0, sizeof(palette));
    memset(vram + 0x10800, 0x11, 0x1000);
    palette[81] = 0x001f;
    io16(0, 0x1040);
}
static bool draw(int w, int h, int nx, int ny, int hy) {
    return Port_TitleCap_Draw(canvas, w, h, 569, nx, ny, 64, hy, frame, sizeof(frame),
                              vram, sizeof(vram), palette, io, NULL);
}
static const uint8_t* io_line(int line) { return lineio[line >= 8]; }
static void native_untouched(int nx, int ny) {
    for (int y = ny; y < ny + 160; ++y)
        for (int x = nx; x < nx + 240; ++x)
            assert(canvas[y * 569 + x] == before[y * 569 + x]);
}
int main(void) {
    const int profiles[][4] = {{240,160,0,0}, {256,192,8,16}, {285,160,22,0},
                               {374,160,67,0}, {569,160,164,0}};
    for (unsigned i = 0; i < sizeof(profiles) / sizeof(profiles[0]); ++i) {
        reset();
        assert(draw(profiles[i][0], profiles[i][1], profiles[i][2], profiles[i][3], 16));
        native_untouched(profiles[i][2], profiles[i][3]);
        if (profiles[i][2] > 0)
            assert(canvas[profiles[i][2] - 1] != before[profiles[i][2] - 1] || profiles[i][3] > 0);
    }
    reset(); assert(draw(285,160,22,0,16)); native_untouched(22,0);
    assert(canvas[22 - 8] == 0xff0000f8 && canvas[22 - 9] == before[22 - 9]);
    assert(canvas[31 * 569 + 21] == 0xff0000f8 && canvas[32 * 569 + 21] == before[32 * 569 + 21]);
    reset();
    for (int row = 0; row < 4; ++row)
        memset(vram + 0x10000 + (96 + row * 8 + 6) * 32, 0, 64);
    assert(draw(285,160,22,0,16)); native_untouched(22,0);
    assert(canvas[14] == 0xff0000f8 && canvas[21] == 0xff0000f8);
    reset(); assert(draw(256,192,8,16,0)); native_untouched(8,16);
    assert(canvas[8] == 0xff0000f8 && canvas[15 * 569 + 8] == 0xff0000f8);
    assert(canvas[16 * 569 + 8] == before[16 * 569 + 8]);
    reset(); io16(0x50, 0xd0); io16(0x54, 8); assert(draw(285,160,22,0,16));
    assert(canvas[21] == 0xff000080); /* 5-bit OBJ darkening */
    reset(); io16(0x50, 0x90); io16(0x54, 8); assert(draw(285,160,22,0,16));
    assert(canvas[21] == 0xff7878f8); /* brighten each five-bit channel */
    reset(); io16(0x50, 0x2050); io16(0x52, 0x0808); assert(draw(285,160,22,0,16));
    assert(canvas[21] == 0xff0818a0); /* blend OBJ against existing backdrop */
    reset(); io16(0,0x3040); io16(0x4a,0x10); io16(0x50,0xd0); io16(0x54,8);
    assert(draw(285,160,22,0,16)); assert(canvas[21] == 0xff0000f8); /* window disables effects */
    reset(); memcpy(lineio[0],io,sizeof(io)); memcpy(lineio[1],io,sizeof(io));
    lineio[1][0x50] = 0xd0; lineio[1][0x54] = 8;
    assert(Port_TitleCap_Draw(canvas,285,160,569,22,0,64,16,frame,sizeof(frame),
                             vram,sizeof(vram),palette,io,io_line));
    assert(canvas[21] == 0xff0000f8 && canvas[8 * 569 + 21] == 0xff000080);
    reset(); io16(0,0x3040); io16(0x4a,0); assert(draw(285,160,22,0,16));
    assert(canvas[21] == before[21]); /* outside-window OBJ disabled */
    reset(); io16(0,0x9040); assert(!draw(285,160,22,0,16));
    assert(memcmp(canvas,before,sizeof(canvas)) == 0);
    reset(); uint8_t bad[sizeof(frame)]; memcpy(bad,frame,sizeof(frame)); bad[5] = 0;
    assert(!Port_TitleCap_Draw(canvas,285,160,569,22,0,64,16,bad,sizeof(bad),vram,sizeof(vram),palette,io,NULL));
    assert(memcmp(canvas,before,sizeof(canvas)) == 0);
    puts("Title cap source, margin mask, animation and OBJ effects: PASS");
    return 0;
}
