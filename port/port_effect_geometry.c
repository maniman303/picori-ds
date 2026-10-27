#include "port_effect_geometry.h"

#define MAX_CANVAS_WIDTH 569
#define MAX_CANVAS_HEIGHT 192
#define MAX_IRIS_RADIUS 1024

bool Port_Effect_BuildIrisRows(int canvas_width, int canvas_height, int native_x, int native_y,
                               int center_x, int center_y, unsigned radius,
                               PortEffectSpan* rows, size_t row_capacity) {
    uint16_t half_width[MAX_IRIS_RADIUS + 1] = {0};
    int x = 0, y;
    if (!rows || canvas_width < 240 || canvas_width > MAX_CANVAS_WIDTH ||
        canvas_height < 160 || canvas_height > MAX_CANVAS_HEIGHT ||
        native_x < 0 || native_x > canvas_width - 240 ||
        native_y < 0 || native_y > canvas_height - 160 ||
        center_x < 0 || center_x >= 240 || center_y < 0 || center_y >= 160 ||
        radius > MAX_IRIS_RADIUS || row_capacity < (size_t)canvas_height)
        return false;

    y = (int)radius;
    int decision = 3 - 2 * y;
    while (x <= y) {
        half_width[x] = (uint16_t)y;
        half_width[y] = (uint16_t)x;
        if (decision < 0) {
            decision += 6 + x++ * 4;
        } else {
            decision += 10 + (x++ - y--) * 4;
        }
    }
    const int cx = native_x + center_x, cy = native_y + center_y;
    for (int line = 0; line < canvas_height; ++line) {
        int distance = line > cy ? line - cy : cy - line;
        int left = 0, right = 0;
        /* The native writer iterates radius entries, omitting distance=radius. */
        if ((unsigned)distance < radius) {
            left = cx - half_width[distance];
            right = cx + half_width[distance];
            if (left < 0) left = 0;
            if (right > canvas_width) right = canvas_width;
            if (left >= right) left = right = 0;
        }
        rows[line].left = (uint16_t)left;
        rows[line].right = (uint16_t)right;
    }
    return true;
}

/* Only the fade producer opts in. Generic circular lights, portal windows,
 * and other WIN0 tables must not inherit scene-wide margin behavior. */
typedef struct { int active, x, y, radius; } FadeIris;
static FadeIris sFadeIris, sPendingFadeIris;

void Port_Effect_ClearFadeIris(void) {
    sFadeIris.active = 0;
    sPendingFadeIris.active = 0;
}

void Port_Effect_ClearPendingFadeIris(void) { sPendingFadeIris.active = 0; }
void Port_Effect_CommitFadeIris(void) { sFadeIris = sPendingFadeIris; }

void Port_Effect_PublishFadeIris(int x, int y, int radius, unsigned inside, unsigned outside) {
    Port_Effect_ClearPendingFadeIris();
    if (x < 0 || x >= 240 || y < 0 || y >= 160 || radius < 0 || radius > 150 ||
        inside != 0x3f3f || outside != 0)
        return;
    sPendingFadeIris.x = x; sPendingFadeIris.y = y;
    sPendingFadeIris.radius = radius; sPendingFadeIris.active = 1;
}

/* The full-cover radius is derived from the same discrete midpoint raster as
 * the native shape, not from a floating-point approximation. The margin
 * radius advances linearly from zero to that cover radius over native 0..150.
 * Consumers clip these spans to margins and preserve native WIN0 pixels. */
bool Port_Effect_ResolveFadeIris(int width, int height, int nx, int ny,
                                PortEffectSpan* rows, size_t capacity) {
    PortEffectSpan scratch[MAX_CANVAS_HEIGHT];
    if (!sFadeIris.active || !rows || capacity < (size_t)height ||
        !Port_Effect_BuildIrisRows(width, height, nx, ny, sFadeIris.x, sFadeIris.y,
                                   0, scratch, MAX_CANVAS_HEIGHT))
        return false;
    unsigned lo = 0, hi = MAX_IRIS_RADIUS;
    while (lo < hi) {
        unsigned mid = lo + (hi - lo) / 2;
        bool covers = true;
        if (!Port_Effect_BuildIrisRows(width, height, nx, ny, sFadeIris.x, sFadeIris.y,
                                       mid, scratch, MAX_CANVAS_HEIGHT))
            return false;
        for (int y = 0; y < height; ++y)
            if (scratch[y].left != 0 || scratch[y].right != width) { covers = false; break; }
        if (covers) hi = mid; else lo = mid + 1;
    }
    unsigned radius = ((unsigned)sFadeIris.radius * lo + 149u) / 150u;
    return Port_Effect_BuildIrisRows(width, height, nx, ny, sFadeIris.x, sFadeIris.y,
                                     radius, rows, capacity);
}
