// g++ -std=c++17 tools/hd_text_regression_test.cpp -o /tmp/hd_text_test && /tmp/hd_text_test
#include "../port/port_hd_text_geometry.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
    using namespace PortHDText;
    CanvasViewport canvas;
    // Native frames can be inset; dialogue must track their actual origin.
    for (int y : {8, 120}) {
        assert(MapCanvasViewport(16, y, 208, 32, 240, 160,
                                 80, 80, 480, 320, 1, 1, canvas));
        assert(canvas.x == 112 && canvas.y == 80 + y * 2);
        assert(canvas.width == 416 && canvas.height == 64);
        assert(canvas.scaleX == 2 && canvas.scaleY == 2);
        assert(canvas.x + canvas.width <= 560 && canvas.y + canvas.height <= 400);
        // Identical physical output when ImGui coordinates use half-size units.
        assert(MapCanvasViewport(16, y, 208, 32, 240, 160,
                                 40, 40, 240, 160, 2, 2, canvas));
        assert(canvas.x * 2 == 112 && canvas.y * 2 == 80 + y * 2);
        assert(canvas.width * 2 == 416 && canvas.height * 2 == 64);
    }
    // A larger output supports 3x base pixels without stretching glyphs.
    assert(MapCanvasViewport(16, 120, 208, 32, 240, 160,
                             40, 60, 720, 480, 1, 1, canvas));
    assert(canvas.x == 88 && canvas.y == 420 && canvas.width == 624 && canvas.height == 96);
    assert(!MapCanvasViewport(16, 120, 208, 32, 240, 160,
                              0, 0, 240, 160, 1, 1, canvas));
    assert(!MapCanvasViewport(40, 120, 208, 32, 240, 160,
                              80, 80, 480, 320, 1, 1, canvas));
    assert(!MapCanvasViewport(16, 140, 208, 32, 240, 160,
                              80, 80, 480, 320, 1, 1, canvas));
    // Actual 212x160 4:3 frame at 640x480: 3x pixels with a 2px margin.
    for (int y : {8, 120}) {
        assert(MapCanvasViewport(8, y, 192, 32, 212, 160,
                                 2, 0, 636, 480, 1, 1, canvas));
        assert(canvas.x == 26 && canvas.y == y * 3);
        assert(canvas.width == 576 && canvas.height == 96);
        assert(canvas.x + canvas.width <= 638 && canvas.y + canvas.height <= 480);
        assert(MapCanvasViewport(8, y, 192, 32, 212, 160,
                                 1, 0, 318, 240, 2, 2, canvas));
        assert(canvas.x * 2 == 26 && canvas.y * 2 == y * 3);
    }
    const uint8_t pixels[] = {15, 1, 15, 1};
    uint32_t frame[] = {0xff002244, 0xff88aaff, 0xff002244, 0xff88aaff};
    uint32_t colors[16] = {};
    assert(MatchCanvas(pixels, 2, 2, frame, 2, 2, 2, 0, 0, colors));
    assert(colors[1] == 0xff88aaff); // ABGR colors remain unchanged, including fades.
    frame[3] = 0xff123456; // A sprite/effect obscures one glyph: leave original frame.
    assert(!MatchCanvas(pixels, 2, 2, frame, 2, 2, 2, 0, 0, colors));
    assert(!MatchCanvas(pixels, 2, 2, frame, 2, 2, 2, 1, 0, colors));
    assert(!MatchCanvas(pixels, 2, 2, frame, 2, 2, 1, 0, 0, colors));
    const uint8_t transparent[] = {0};
    assert(!MatchCanvas(transparent, 1, 1, frame, 2, 2, 2, 0, 0, colors));
    const uint8_t solid[] = {1, 1, 1, 1};
    assert(CornerInset(solid, 2, 2, 0, 0, -1, -1) == 0.25f);
    assert(CornerInset(solid, 2, 2, 0, 0, 1, 1) == 0.0f);
    assert(CornerInset(solid, 2, 2, 0, 0, -1, 1) == 0.0f);
    const uint8_t coloredNeighbor[] = {1, 2, 15, 15};
    assert(CornerInset(coloredNeighbor, 2, 2, 0, 0, 1, -1) == 0.0f);
    const uint8_t diagonal[] = {1, 15, 15, 1};
    assert(CornerInset(diagonal, 2, 2, 0, 0, 1, 1) == 0.0f); // Keep diagonal strokes connected.
    uint8_t crowded[kMaxPixels];
    uint32_t crowdedFrame[kMaxPixels];
    for (int i = 0; i < kMaxPixels; ++i) { crowded[i] = 1; crowdedFrame[i] = 0xffabcdef; }
    assert(!MatchCanvas(crowded, 208, 32, crowdedFrame, 208, 32, 208, 0, 0, colors));
    std::puts("HD dialogue geometry: passed");
}
