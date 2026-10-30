#include "port_title_cap.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <png.h>

static constexpr int kFrameW = 569, pitch = 576;
static uint32_t virtuappu_frame_buffer[pitch * 192];
static int sComposedWidth, sComposedHeight;
static int virtuappu_mode1_frame_width() { return 240; }
static int virtuappu_mode1_frame_height() { return 160; }
static int virtuappu_mode1_frame_pitch() { return pitch; }
#include "screenshot_functions.inc"

int main(int argc, char** argv) {
    assert(argc == 2);
    const int profiles[][4] = {{240,160,0,0}, {256,192,8,16}, {285,160,22,0},
                               {374,160,67,0}, {569,160,164,0}, {288,162,24,1}};
    static uint8_t vram[0x18000], io[0x400];
    static uint16_t palette[256];
    const uint8_t frame[] = {2, 0xc0, 0xf0, 0x71, 0x40, 0x50, 0, 0xf0, 0x71, 0x60, 0x50};
    for (int i = 0x10800; i < 0x11800; ++i) vram[i] = 0x11;
    palette[81] = 0x001f;
    // The fixed-screen OBJ window is limited to the native rectangle; the
    // added margins use WINOUT, which enables OBJ and effects here.
    io[0] = 0x40; io[1] = 0x90; io[0x4a] = 0x30;
    for (const auto& p : profiles) {
        const int w = p[0], h = p[1], nx = p[2], ny = p[3];
        for (int y = 0; y < 160; ++y)
            for (int x = 0; x < 240; ++x)
                virtuappu_frame_buffer[y * pitch + x] = 0xff490000u | (y << 8) | x;
        assert(Port_TitleCap_ExpandFrame(virtuappu_frame_buffer, w, h, pitch, nx, ny,
                                        nullptr, 0, 0xff070503));
        /* The 288x162 synthetic extent has no cap pieces in the added margins;
         * that profile checks expansion/screenshot geometry only. */
        if (w != 288)
            assert(Port_TitleCap_Draw(virtuappu_frame_buffer, w, h, pitch, nx, ny, 64, 16,
                                      frame, sizeof(frame), vram, sizeof(vram), palette, io, 5, nullptr));
        sComposedWidth = w; sComposedHeight = h;
        assert(WriteScreenshotPNG(std::filesystem::path(argv[1]) / (std::to_string(w) + ".png")));
    }
}
