#pragma once
#include <cstdint>

namespace PortHDText {
// Expanded canvases retain the original raster text until their shared
// dialogue/HUD layout is implemented. This is a layout gate, not storage.
inline bool SupportedFrame(int width, int height) {
    return width >= 240 && width <= 512 && height == 160;
}

// Locate the text using the same installed BG0 message band as the PPU.
// Fixed screens are composed as one native rectangle; live frames instead use
// the published message shift, which can be zero even on a wide frame.
inline bool ResolveCanvasOrigin(int frameWidth, int frameHeight, bool fixedCanvas,
                                int shift, int x0, int x1, int y0, int y1,
                                int width, int height, int& x, int& y) {
    if (frameWidth < 240 || frameWidth > 569 || frameHeight < 160 || frameHeight > 192 ||
        width <= 0 || width > 208 || height <= 0 || height > 32 ||
        x < 0 || x > 240 - width || y < 0 || y > 160 - height)
        return false;
    if (fixedCanvas) {
        x += (frameWidth - 240) / 2;
        y += (frameHeight - 160) / 2;
    } else if (shift != 0) {
        if (shift < 0 || shift > frameWidth - 240 ||
            x0 < 0 || x1 > 240 || x0 >= x1 || y0 < 0 || y1 > 160 || y0 >= y1 ||
            x < x0 || x + width > x1 || y < y0 || y + height > y1)
            return false;
        x += shift;
    }
    return x <= frameWidth - width && y <= frameHeight - height;
}
constexpr int kMaxPixels = 208 * 32;
constexpr int kMaxForeground = 2048;

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
