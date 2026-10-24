#pragma once
#include <cstdint>

namespace PortHDText {
constexpr int kMaxPixels = 208 * 32;
constexpr int kMaxForeground = 2048;

struct CanvasViewport {
    float x, y, width, height, scaleX, scaleY;
};

// The viewport is the inset game frame, not its surrounding aspect-ratio
// stage. Draw in logical coordinates, but judge available detail in pixels.
inline bool MapCanvasViewport(int x, int y, int width, int height, int frameWidth, int frameHeight,
                              float viewportX, float viewportY, float viewportWidth, float viewportHeight,
                              float framebufferScaleX, float framebufferScaleY, CanvasViewport& result) {
    if (frameWidth <= 0 || frameHeight <= 0 || width <= 0 || height <= 0 ||
        x < 0 || y < 0 || x > frameWidth - width || y > frameHeight - height ||
        viewportWidth <= 0 || viewportHeight <= 0)
        return false;
    const float sx = viewportWidth / frameWidth, sy = viewportHeight / frameHeight;
    if (sx * framebufferScaleX < 2.0f || sy * framebufferScaleY < 2.0f)
        return false;
    result = {viewportX + x * sx, viewportY + y * sy, width * sx, height * sy, sx, sy};
    return true;
}

// A palette index must have one displayed color throughout the canvas. This
// preserves uniform fades and refuses sprite overlaps or other compositing.
inline bool MatchCanvas(const uint8_t* pixels, int width, int height, const uint32_t* frame,
                        int frameWidth, int frameHeight, int pitch, int x, int y, uint32_t colors[16]) {
    if (!pixels || !frame || width <= 0 || height <= 0 || width > 208 || height > 32 ||
        pitch < frameWidth || x < 0 || y < 0 || x > frameWidth - width || y > frameHeight - height)
        return false;
    unsigned seen = 0;
    int foreground = 0;
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const unsigned index = pixels[row * width + col];
            if (index == 0 || index > 15)
                return false;
            if (index != 15 && ++foreground > kMaxForeground)
                return false;
            const uint32_t color = frame[(y + row) * pitch + x + col];
            if ((seen & (1u << index)) && colors[index] != color)
                return false;
            colors[index] = color;
            seen |= 1u << index;
        }
    }
    return (seen & (1u << 15)) != 0;
}

inline bool Background(const uint8_t* pixels, int width, int height, int x, int y) {
    return x < 0 || y < 0 || x >= width || y >= height || pixels[y * width + x] == 15;
}

// Cut only exposed corners; shared edges and neighboring shadow colors retain
// their original coverage so letters never develop seams between cells.
inline float CornerInset(const uint8_t* pixels, int width, int height, int x, int y, int dx, int dy) {
    return Background(pixels, width, height, x + dx, y) &&
                   Background(pixels, width, height, x, y + dy) &&
                   Background(pixels, width, height, x + dx, y + dy) ? 0.25f : 0.0f;
}
}
