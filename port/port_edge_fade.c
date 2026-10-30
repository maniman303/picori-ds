#include "port_edge_fade.h"

#include <stddef.h>

static bool valid_world(PortViewportRect world, int width, int height) {
    return world.x >= 0 && world.y >= 0 && world.width > 0 && world.height > 0 &&
           world.x <= width - world.width && world.y <= height - world.height;
}

/* Rounded corner distance: straight edges keep their exact 1..5 pixel steps,
 * while diagonal corner pixels use rounded Euclidean distance so the fade
 * outline curves instead of forming a square. Values beyond the band collapse
 * to the same black result. Integer-only: no libm in the frame path. */
static int rounded_band_distance(int dx, int dy) {
    const int d2 = dx * dx + dy * dy;
    if (d2 <= 2) return 1;   /* sqrt(2.25) */
    if (d2 <= 6) return 2;   /* sqrt(6.25) */
    if (d2 <= 12) return 3;  /* sqrt(12.25) */
    if (d2 <= 20) return 4;  /* sqrt(20.25) */
    return 5;                /* 30 is sqrt(30.25); anything larger is black */
}

typedef struct {
    uint32_t left[PORT_VIEWPORT_MAX_HEIGHT];
    uint32_t right[PORT_VIEWPORT_MAX_HEIGHT];
    uint32_t top[PORT_VIEWPORT_MAX_WIDTH];
    uint32_t bottom[PORT_VIEWPORT_MAX_WIDTH];
    uint32_t corners[4]; /* tl, tr, bl, br */
} EdgeSources;

/* Find the closest non-UI map pixel along an edge. HUD pixels are opaque in
 * the composed frame, so the map colour underneath them is not recoverable;
 * the adjacent visible map pixel is the correct visual continuation and avoids
 * both the old HUD-colour bleed and a black notch. */
static void build_sources(const uint32_t* frame, int pitch, PortViewportRect world,
                          const uint8_t* ui_mask, int mask_pitch, EdgeSources* s) {
    const int right = world.x + world.width;
    const int bottom = world.y + world.height;
    for (int y = world.y; y < bottom; ++y) {
        const int row = y - world.y;
        const uint8_t* mask_row = ui_mask + (size_t)y * (size_t)mask_pitch;
        s->left[row] = 0xff000000u;
        s->right[row] = 0xff000000u;
        for (int x = world.x; x < right; ++x) {
            if (!mask_row[x]) { s->left[row] = frame[(size_t)y * (size_t)pitch + (size_t)x]; break; }
        }
        for (int x = right - 1; x >= world.x; --x) {
            if (!mask_row[x]) { s->right[row] = frame[(size_t)y * (size_t)pitch + (size_t)x]; break; }
        }
    }
    for (int x = world.x; x < right; ++x) {
        const int col = x - world.x;
        s->top[col] = 0xff000000u;
        s->bottom[col] = 0xff000000u;
        for (int y = world.y; y < bottom; ++y) {
            if (!ui_mask[(size_t)y * (size_t)mask_pitch + (size_t)x]) {
                s->top[col] = frame[(size_t)y * (size_t)pitch + (size_t)x];
                break;
            }
        }
        for (int y = bottom - 1; y >= world.y; --y) {
            if (!ui_mask[(size_t)y * (size_t)mask_pitch + (size_t)x]) {
                s->bottom[col] = frame[(size_t)y * (size_t)pitch + (size_t)x];
                break;
            }
        }
    }
    /* Diagonal scan for rounded corners. */
    const int max_d = world.width < world.height ? world.width : world.height;
    for (int corner = 0; corner < 4; ++corner) {
        const int base_x = (corner & 1) ? right - 1 : world.x;
        const int base_y = (corner & 2) ? bottom - 1 : world.y;
        const int step_x = (corner & 1) ? -1 : 1;
        const int step_y = (corner & 2) ? -1 : 1;
        s->corners[corner] = 0xff000000u;
        for (int d = 0; d < max_d; ++d) {
            const int x = base_x + d * step_x;
            const int y = base_y + d * step_y;
            if (x < world.x || x >= right || y < world.y || y >= bottom)
                break;
            if (!ui_mask[(size_t)y * (size_t)mask_pitch + (size_t)x]) {
                s->corners[corner] = frame[(size_t)y * (size_t)pitch + (size_t)x];
                break;
            }
        }
    }
}

bool Port_EdgeFade_Apply(uint32_t* frame, int pitch, int width, int height,
                         PortViewportRect world, const uint8_t* ui_mask,
                         int ui_mask_pitch) {
    if (!frame || width <= 0 || height <= 0 || pitch < width ||
        width > PORT_VIEWPORT_MAX_WIDTH || height > PORT_VIEWPORT_MAX_HEIGHT ||
        !valid_world(world, width, height) ||
        (ui_mask != NULL && ui_mask_pitch < width))
        return false;
    const int right = world.x + world.width;
    const int bottom = world.y + world.height;
    if (world.x == 0 && world.y == 0 && right == width && bottom == height)
        return false;

    EdgeSources sources;
    if (ui_mask != NULL)
        build_sources(frame, pitch, world, ui_mask, ui_mask_pitch, &sources);

    const int band = PORT_EDGE_FADE_PIXELS;
    for (int y = 0; y < height; ++y) {
        const bool row_inside = y >= world.y && y < bottom;
        const int dy = y < world.y ? world.y - y : (y >= bottom ? y - (bottom - 1) : 0);
        uint32_t* row = frame + (size_t)y * (size_t)pitch;
        const uint8_t* mask_row = ui_mask != NULL
            ? ui_mask + (size_t)y * (size_t)ui_mask_pitch : NULL;
        for (int x = 0; x < width; ++x) {
            if (row_inside && x >= world.x && x < right)
                continue;
            if (mask_row != NULL && mask_row[x])
                continue;
            const int dx = x < world.x ? world.x - x : (x >= right ? x - (right - 1) : 0);
            const int d = rounded_band_distance(dx, dy);
            uint32_t src = 0xff000000u;
            if (d > 0 && d < band) {
                if (ui_mask == NULL) {
                    const int sx = x < world.x ? world.x : (x >= right ? right - 1 : x);
                    const int sy = y < world.y ? world.y : (y >= bottom ? bottom - 1 : y);
                    src = frame[(size_t)sy * (size_t)pitch + (size_t)sx];
                } else if (dx > 0 && dy > 0) {
                    const int corner = (x < world.x ? 0 : 1) | (y < world.y ? 0 : 2);
                    src = sources.corners[corner];
                } else if (dx > 0) {
                    src = x < world.x ? sources.left[y - world.y] : sources.right[y - world.y];
                } else {
                    src = y < world.y ? sources.top[x - world.x] : sources.bottom[x - world.x];
                }
                const unsigned factor = (unsigned)(band - d);
                src = (src & 0xff000000u) |
                    ((((src >> 16) & 0xffu) * factor / (unsigned)band) << 16) |
                    ((((src >> 8) & 0xffu) * factor / (unsigned)band) << 8) |
                    (((src & 0xffu) * factor / (unsigned)band));
            }
            row[x] = src;
        }
    }
    return true;
}
