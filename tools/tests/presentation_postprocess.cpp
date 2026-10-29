#include "port_background_compose.h"
#include "port_title_cap.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

static constexpr int pitch = 576, maxHeight = 192;
#define VIRTUAPPU_FRAME_BUFFER_SIZE (pitch * maxHeight)
static uint32_t virtuappu_frame_buffer[VIRTUAPPU_FRAME_BUFFER_SIZE];
static int sComposedWidth, sComposedHeight;
static int virtuappu_mode1_frame_width() { return 240; }
static int virtuappu_mode1_frame_height() { return 160; }
static int virtuappu_mode1_frame_pitch() { return pitch; }
static bool sColorCorrectEnabled = false, sColorLutReady = false;
static uint8_t sColorLut[256];
static bool sPersistEnabled = false, sPersistAccumValid = false, sPresentFirstOfTick = true;
static float sPersistRho = 0.5f;
static uint32_t sPersistAccum[VIRTUAPPU_FRAME_BUFFER_SIZE];
#include "postprocess_functions.inc"

static void compose(int width, int height, int nx, int ny, uint8_t value) {
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            virtuappu_frame_buffer[y * pitch + x] = 0xff202020;
    assert(Port_TitleCap_ExpandFrame(virtuappu_frame_buffer, width, height, pitch, nx, ny,
                                     nullptr, 0, 0xff000000));
    PortBackgroundComposeRow rows[maxHeight] = {};
    for (int y = 0; y < height; ++y) rows[y].layer_effect = PORT_LAYER_EFFECT_NONE;
    PortBackgroundComposition bg = {{0, 0, width, height}, {nx, ny, 240, 160},
                                    nullptr, 0, {value, value, value}, rows, height};
    assert(Port_Background_ComposeMargins(virtuappu_frame_buffer, pitch, &bg));
    sComposedWidth = width;
    sComposedHeight = height;
}

int main() {
    for (int i = 0; i < 256; ++i) sColorLut[i] = static_cast<uint8_t>(255 - i);
    sColorLutReady = sColorCorrectEnabled = true;
    Port_PPU_SetPersistence(true, 0.5f);
    compose(285, 160, 22, 0, 16);
    Port_PPU_ApplyDisplayPostProcess(pitch * 160);
    assert(virtuappu_frame_buffer[0] == 0xffefefef);
    assert(virtuappu_frame_buffer[22] == 0xffdfdfdf);
    int w, h, bytes;
    assert(Port_PPU_SelectPresentFrame(&w, &h, &bytes) == virtuappu_frame_buffer);
    assert(w == 285 && h == 160 && bytes == pitch * 4);
    compose(285, 160, 22, 0, 80);
    sPresentFirstOfTick = false;
    Port_PPU_ApplyDisplayPostProcess(pitch * 160);
    assert(virtuappu_frame_buffer[0] == 0xffcfcfcf);
    compose(285, 160, 22, 0, 80);
    sPresentFirstOfTick = true;
    Port_PPU_ApplyDisplayPostProcess(pitch * 160);
    assert(virtuappu_frame_buffer[0] == 0xffcfcfcf);
    compose(285, 160, 22, 0, 80);
    Port_PPU_ApplyDisplayPostProcess(pitch * 160);
    assert(virtuappu_frame_buffer[0] == 0xffbfbfbf);
    // Changing dimensions invalidates old persistence history, including top margins.
    compose(256, 192, 8, 16, 80);
    Port_PPU_ApplyDisplayPostProcess(pitch * 192);
    assert(virtuappu_frame_buffer[0] == 0xffafafaf);
    assert(virtuappu_frame_buffer[16 * pitch + 8] == 0xffdfdfdf);
    Port_PPU_SelectPresentFrame(&w, &h, &bytes);
    assert(w == 256 && h == 192 && bytes == pitch * 4);
    Port_PPU_SetPersistence(false, 0.5f);
    compose(256, 192, 8, 16, 16);
    Port_PPU_ApplyDisplayPostProcess(pitch * 192);
    assert(virtuappu_frame_buffer[0] == 0xffefefef);
    std::puts("Composed margin post-processing and presentation frame: PASS");
}
