#include "port_title_cap.h"
#include <string.h>

bool Port_TitleCap_ExpandFrame(uint32_t* canvas, int width, int height, int pitch,
                               int nx, int ny, const uint32_t* margins,
                               int margins_pitch, uint32_t fill) {
    if (!canvas || width < 240 || width > 569 || height < 160 || height > 192 ||
        pitch < width || nx < 0 || ny < 0 || nx > width - 240 || ny > height - 160 ||
        (margins && margins_pitch < width))
        return false;
    for (int y = 159; y >= 0; --y)
        memmove(canvas + (size_t)(y + ny) * pitch + nx,
                canvas + (size_t)y * pitch, 240 * sizeof(*canvas));
    for (int y = 0; y < height; ++y) {
        uint32_t* row = canvas + (size_t)y * pitch;
        const uint32_t* source = margins ? margins + (size_t)y * margins_pitch : NULL;
        int left = (y >= ny && y < ny + 160) ? nx : width;
        int right = (y >= ny && y < ny + 160) ? nx + 240 : width;
        if (source) {
            memcpy(row, source, (size_t)left * sizeof(*row));
            memcpy(row + right, source + right, (size_t)(width - right) * sizeof(*row));
        } else {
            for (int x = 0; x < left; ++x) row[x] = fill;
            for (int x = right; x < width; ++x) row[x] = fill;
        }
    }
    return true;
}

static uint16_t read16(const uint8_t* p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }

static unsigned title_index(const uint8_t* vram, unsigned display, unsigned sx, unsigned sy) {
    unsigned piece = sx / 64, px = sx % 64;
    unsigned tile = 64 + piece * 32 + (sy / 8) * ((display & 0x40) ? 8 : 32) + px / 8;
    size_t address = 0x10000u + tile * 32u + (sy % 8) * 4u + (px % 8) / 2u;
    return (vram[address] >> ((px & 1) * 4)) & 15;
}

/* A mirrored copy of the native right corner completes the clipped left
 * corner. Above native y=0, sample the real heading pieces at their animated
 * pose; the native rectangle itself remains authoritative. */
bool Port_TitleCap_CanDraw(int width, int height, int pitch, int nx, int ny, int hx, int hy,
                           const uint8_t* frame, size_t frame_size, size_t vram_size, const uint8_t* io) {
    static const uint8_t expected[] = {2, 0xc0, 0xf0, 0x71, 0x40, 0x50,
                                      0x00, 0xf0, 0x71, 0x60, 0x50};
    if (!frame || !io || frame_size < sizeof(expected) ||
        width < 240 || width > 569 || height < 160 || height > 192 || pitch < width ||
        nx < 0 || ny < 0 || nx > width - 240 || ny > height - 160 ||
        hx != 64 || hy < -64 || hy > 80 || vram_size < 0x18000)
        return false;
    for (size_t i = 0; i < sizeof(expected); ++i)
        if (frame[i] != expected[i]) return false;
    return true;
}

static bool draw_cap(uint32_t* canvas, int width, int height, int pitch,
                        int nx, int ny, int hx, int hy,
                        const uint8_t* frame, size_t frame_size,
                        const uint8_t* vram, size_t vram_size,
                        const uint16_t* palette, const uint8_t* io, unsigned background_layer,
                        const uint8_t* (*io_for_line)(int), const PortEffectSpan* spans, int span_count,
                        const uint8_t* background_layers, int layer_count, const uint8_t* top_layers) {
    if (!canvas || !vram || !palette || (background_layer > 3 && background_layer != 5) ||
        !Port_TitleCap_CanDraw(width, height, pitch, nx, ny, hx, hy, frame, frame_size, vram_size, io))
        return false;
    if (background_layers) {
        if (layer_count < height) return false;
        for (int y = 0; y < height; ++y)
            if (background_layers[y] > 3 && background_layers[y] != 5) return false;
    }
    if (top_layers) for (int x = 0; x < 240; ++x)
        if (top_layers[x] > 3 && top_layers[x] != 5) return false;
    if (spans) {
        if (span_count < height) return false;
        for (int y = 0; y < height; ++y)
            if (spans[y].left > spans[y].right || spans[y].right > width) return false;
    }
    /* Sprite pieces include transparent padding after the right corner. Find
     * the actual outer edge in the live OBJ tile data instead of assuming
     * that the 128-pixel piece bounds are opaque. */
    int right[2] = {-1, -1};
    for (int mapping = 0; mapping < 2; ++mapping)
        for (int sx = 127; sx >= 0 && right[mapping] < 0; --sx)
            for (unsigned sy = 0; sy < 32; ++sy)
                if (title_index(vram, mapping ? 0x40 : 0, (unsigned)sx, sy)) {
                    right[mapping] = sx; break;
                }
    const int top = hy - 16;
    int first_y = ny + top;
    int last_y = first_y + 32;
    if (first_y < 0) first_y = 0;
    if (last_y > height) last_y = height;
    int first_x = nx - 8;
    int last_x = nx + 128;
    if (first_x < 0) first_x = 0;
    if (last_x > width) last_x = width;
    for (int y = first_y; y < last_y; ++y) {
        const unsigned row_layer = background_layers ? background_layers[y] : background_layer;
        const int sy = y - ny - top;
        if (sy < 0 || sy >= 32) continue;
        const uint8_t* line_io = io_for_line ? io_for_line(y - ny) : io;
        if (!line_io) continue;
        const uint16_t display = read16(line_io);
        if (!(display & 0x1000) || (display & 0x80)) continue;
        unsigned window = 0x3f;
        if (display & 0xe000) window = read16(line_io + (spans ? 0x48 : 0x4a)) & 0x3f;
        if (!(window & 0x10)) continue;
        const int right_edge = right[(display & 0x40) != 0];
        if (right_edge < 7) continue;
        const unsigned blend = read16(line_io + 0x50);
        unsigned amount = read16(line_io + 0x54) & 31;
        if (amount > 16) amount = 16;
        unsigned eva = read16(line_io + 0x52) & 31, evb = (read16(line_io + 0x52) >> 8) & 31;
        if (eva > 16) eva = 16;
        if (evb > 16) evb = 16;
        for (int x = first_x; x < last_x; ++x) {
            if (spans && (x < spans[y].left || x >= spans[y].right)) continue;
            unsigned pixel_layer = row_layer;
            if (top_layers && y < ny) {
                int edge_x = x - nx;
                if (edge_x < 0) edge_x = 0;
                if (edge_x > 239) edge_x = 239;
                pixel_layer = top_layers[edge_x];
            }
            int sx = x - nx;
            if (sx >= 0 && sx < 240 && y >= ny && y < ny + 160) continue;
            if (sx < 0) {
                if (sx < -8) continue;
                sx = right_edge - 8 - sx; /* mirrored right cap, x=-8..-1 */
            } else if (sx >= 128 || y >= ny) {
                continue;
            }
            unsigned index = title_index(vram, display, (unsigned)sx, (unsigned)sy);
            if (!index) continue;
            uint16_t color = palette[80 + index];
            unsigned c[3] = {color & 31u, (color >> 5) & 31u, (color >> 10) & 31u};
            uint32_t back = canvas[(size_t)y * pitch + x];
            if ((window & 0x20) && (blend & 0x10)) {
                unsigned effect = (blend >> 6) & 3;
                for (int ch = 0; ch < 3; ++ch) {
                    if (effect == 2) c[ch] += ((31 - c[ch]) * amount) >> 4;
                    if (effect == 3) c[ch] -= (c[ch] * amount) >> 4;
                }
            }
            uint32_t result = 0xff000000u;
            for (int ch = 0; ch < 3; ++ch) {
                unsigned value = c[ch] << 3;
                if ((window & 0x20) && (blend & 0x10) && ((blend >> 6) & 3) == 1 &&
                    (blend & (1u << (8 + pixel_layer)))) {
                    value = (value * eva + ((back >> (ch * 8)) & 255u) * evb) >> 4;
                    if (value > 255) value = 255;
                }
                result |= value << (ch * 8);
            }
            canvas[(size_t)y * pitch + x] = result;
        }
    }
    return true;
}


bool Port_TitleCap_Draw(uint32_t* canvas, int width, int height, int pitch, int nx, int ny, int hx, int hy,
                        const uint8_t* frame, size_t frame_size, const uint8_t* vram, size_t vram_size,
                        const uint16_t* palette, const uint8_t* io, unsigned background_layer,
                        const uint8_t* (*io_for_line)(int)) {
    return draw_cap(canvas, width, height, pitch, nx, ny, hx, hy, frame, frame_size, vram, vram_size,
                    palette, io, background_layer, io_for_line, NULL, 0, NULL, 0, NULL);
}

bool Port_TitleCap_DrawIris(uint32_t* canvas, int width, int height, int pitch, int nx, int ny, int hx, int hy,
                            const uint8_t* frame, size_t frame_size, const uint8_t* vram, size_t vram_size,
                            const uint16_t* palette, const uint8_t* io, unsigned background_layer,
                            const uint8_t* (*io_for_line)(int), const PortEffectSpan* spans, int span_count) {
    if (!spans) return false;
    return draw_cap(canvas, width, height, pitch, nx, ny, hx, hy, frame, frame_size, vram, vram_size,
                    palette, io, background_layer, io_for_line, spans, span_count, NULL, 0, NULL);
}

bool Port_TitleCap_DrawResolved(uint32_t* canvas, int width, int height, int pitch, int nx, int ny, int hx, int hy,
                                const uint8_t* frame, size_t frame_size, const uint8_t* vram, size_t vram_size,
                                const uint16_t* palette, const uint8_t* io, const uint8_t* background_layers,
                                int layer_count, const uint8_t* top_layers, const uint8_t* (*io_for_line)(int),
                                const PortEffectSpan* spans, int span_count) {
    if (!background_layers) return false;
    return draw_cap(canvas, width, height, pitch, nx, ny, hx, hy, frame, frame_size, vram, vram_size,
                    palette, io, 5, io_for_line, spans, span_count, background_layers, layer_count, top_layers);
}
