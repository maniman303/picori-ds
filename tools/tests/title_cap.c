#include "port_title_cap.h"
#include "port_background_compose.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t canvas[569 * 192], before[569 * 192];
static uint32_t artwork[569 * 192];
static uint8_t vram[0x18000], io[0x400];
static uint8_t rgba[569 * 192 * 4];
static PortBackgroundComposeRow effects[192];
static PortEffectSpan iris[192];
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
                              vram, sizeof(vram), palette, io, 5, NULL);
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
        int w = profiles[i][0], h = profiles[i][1], nx = profiles[i][2], ny = profiles[i][3];
        for (int y = 0; y < 192; ++y) for (int x = 0; x < 569; ++x) {
            canvas[y * 569 + x] = 0xff000000u | (unsigned)(y * 569 + x);
            artwork[y * 569 + x] = 0xff800000u | (unsigned)(y * 569 + x);
        }
        memcpy(before, canvas, sizeof(canvas));
        assert(Port_TitleCap_ExpandFrame(canvas, w, h, 569, nx, ny, artwork, 569, 0));
        for (int y = 0; y < h; ++y) for (int x = 0; x < 569; ++x) {
            uint32_t expected = x >= w ? before[y * 569 + x] :
                (x >= nx && x < nx + 240 && y >= ny && y < ny + 160 ?
                 before[(y - ny) * 569 + x - nx] : artwork[y * 569 + x]);
            assert(canvas[y * 569 + x] == expected);
        }
        memcpy(canvas, before, sizeof(canvas));
        assert(Port_TitleCap_ExpandFrame(canvas, w, h, 569, nx, ny, NULL, 0, 0xff000000));
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
            uint32_t expected = x >= nx && x < nx + 240 && y >= ny && y < ny + 160 ?
                before[(y - ny) * 569 + x - nx] : 0xff000000;
            assert(canvas[y * 569 + x] == expected);
        }
        reset();
        assert(draw(profiles[i][0], profiles[i][1], profiles[i][2], profiles[i][3], 16));
        native_untouched(profiles[i][2], profiles[i][3]);
        if (profiles[i][2] > 0)
            assert(canvas[profiles[i][2] - 1] != before[profiles[i][2] - 1] || profiles[i][3] > 0);
    }
    /* Exercise the actual margin kernel followed by the cap: source-over
     * alpha and palette fade are resolved before OBJ alpha blending. */
    reset();
    for (int y = 0; y < 160; ++y) {
        effects[y].layer_effect = PORT_LAYER_EFFECT_NONE;
        effects[y].apply_palette_fade = true;
        effects[y].palette_fade = (PortPaletteFade){128, 248};
        for (int x = 0; x < 285; ++x) {
            unsigned p = (y * 285 + x) * 4;
            rgba[p] = 200; rgba[p+1] = 100; rgba[p+2] = 50; rgba[p+3] = 128;
        }
    }
    assert(Port_TitleCap_ExpandFrame(canvas,285,160,569,22,0,NULL,0,0xff000000));
    PortBackgroundComposition composed = {{0,0,285,160},{22,0,240,160},rgba,285*4,{0,0,0},effects,160};
    assert(Port_Background_ComposeMargins(canvas,569,&composed));
    memcpy(before,canvas,sizeof(canvas));
    assert(canvas[21] == 0xff8c98b1);
    io16(0x50, 0x0850); io16(0x52, 0x0808);
    assert(Port_TitleCap_Draw(canvas,285,160,569,22,0,64,16,frame,sizeof(frame),
                             vram,sizeof(vram),palette,io,3,NULL));
    assert(canvas[21] == 0xff464cd4);
    native_untouched(22,0);
    assert(canvas[13] == before[13]);

    /* Extreme animation poses may clip against either canvas edge.  They
     * must never touch row padding, rows below the canvas, or native pixels. */
    for (unsigned i = 0; i < sizeof(profiles) / sizeof(profiles[0]); ++i) {
        int w = profiles[i][0], h = profiles[i][1], nx = profiles[i][2], ny = profiles[i][3];
        for (int hy = -64; hy <= 80; ++hy) {
            reset();
            assert(draw(w, h, nx, ny, hy));
            native_untouched(nx, ny);
            for (int y = 0; y < 192; ++y)
                for (int x = 0; x < 569; ++x)
                    if (x >= w || y >= h)
                        assert(canvas[y * 569 + x] == before[y * 569 + x]);
        }
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
    /* A moving heading must carry the side cap with it and leave the
     * underlying margin untouched above and below the sprite. */
    reset(); assert(draw(285,160,22,0,24)); native_untouched(22,0);
    assert(canvas[7 * 569 + 21] == before[7 * 569 + 21]);
    assert(canvas[8 * 569 + 21] == 0xff0000f8);
    assert(canvas[39 * 569 + 21] == 0xff0000f8);
    assert(canvas[40 * 569 + 21] == before[40 * 569 + 21]);
    /* Transparent OBJ pixels must preserve existing margin artwork. */
    reset(); memset(vram + 0x10800, 0, 0x1000);
    assert(draw(285,160,22,0,16));
    assert(memcmp(canvas, before, sizeof(canvas)) == 0);
    reset(); io16(0x50, 0xd0); io16(0x54, 8); assert(draw(285,160,22,0,16));
    assert(canvas[21] == 0xff000080); /* 5-bit OBJ darkening */
    reset(); io16(0x50, 0x90); io16(0x54, 8); assert(draw(285,160,22,0,16));
    assert(canvas[21] == 0xff7878f8); /* brighten each five-bit channel */
    reset(); io16(0x50, 0x2050); io16(0x52, 0x0808); assert(draw(285,160,22,0,16));
    assert(canvas[21] == 0xff0c18a0); /* blend OBJ against existing backdrop */
    reset(); canvas[21] = 0xff19314b; io16(0x50, 0x0850); io16(0x52, 0x0808);
    assert(Port_TitleCap_Draw(canvas,285,160,569,22,0,64,16,frame,sizeof(frame),
                             vram,sizeof(vram),palette,io,3,NULL));
    assert(canvas[21] == 0xff0c18a1); /* BG3 artwork keeps its eight-bit channels */
    reset(); io16(0x50, 0x0850); io16(0x52, 0x0808); assert(draw(285,160,22,0,16));
    assert(canvas[21] == 0xff0000f8); /* backdrop is not a selected second target */
    reset(); assert(!Port_TitleCap_Draw(canvas,285,160,569,22,0,64,16,frame,sizeof(frame),
                                      vram,sizeof(vram),palette,io,4,NULL));
    assert(memcmp(canvas,before,sizeof(canvas)) == 0);
    reset(); io16(0,0x3040); io16(0x4a,0x10); io16(0x50,0xd0); io16(0x54,8);
    assert(draw(285,160,22,0,16)); assert(canvas[21] == 0xff0000f8); /* window disables effects */
    reset(); memcpy(lineio[0],io,sizeof(io)); memcpy(lineio[1],io,sizeof(io));
    lineio[1][0x50] = 0xd0; lineio[1][0x54] = 8;
    assert(Port_TitleCap_Draw(canvas,285,160,569,22,0,64,16,frame,sizeof(frame),
                             vram,sizeof(vram),palette,io,5,io_line));
    assert(canvas[21] == 0xff0000f8 && canvas[8 * 569 + 21] == 0xff000080);
    reset(); io16(0,0x3040); io16(0x4a,0); assert(draw(285,160,22,0,16));
    assert(canvas[21] == before[21]); /* outside-window OBJ disabled */
    reset(); io16(0,0x3040); io16(0x48,0x3f); io16(0x4a,0);
    for (int y = 0; y < 160; ++y) iris[y] = (PortEffectSpan){18, 285};
    assert(Port_TitleCap_DrawIris(canvas,285,160,569,22,0,64,16,frame,sizeof(frame),
                                 vram,sizeof(vram),palette,io,5,NULL,iris,160));
    assert(canvas[17] == before[17] && canvas[18] == 0xff0000f8 && canvas[21] == 0xff0000f8);
    native_untouched(22,0);
    reset(); iris[159].right = 286;
    assert(!Port_TitleCap_DrawIris(canvas,285,160,569,22,0,64,16,frame,sizeof(frame),
                                  vram,sizeof(vram),palette,io,5,NULL,iris,160));
    assert(memcmp(canvas,before,sizeof(canvas)) == 0);
    reset(); io16(0,0x9040); assert(!draw(285,160,22,0,16));
    assert(memcmp(canvas,before,sizeof(canvas)) == 0);
    reset(); uint8_t bad[sizeof(frame)]; memcpy(bad,frame,sizeof(frame)); bad[5] = 0;
    assert(!Port_TitleCap_Draw(canvas,285,160,569,22,0,64,16,bad,sizeof(bad),vram,sizeof(vram),palette,io,5,NULL));
    assert(memcmp(canvas,before,sizeof(canvas)) == 0);
    puts("Title cap source, margin mask, animation and OBJ effects: PASS");
    return 0;
}
