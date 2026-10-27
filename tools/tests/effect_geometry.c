/* Compare canvas iris geometry against the real native window generator. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "port_effect_geometry.h"

uint16_t gUnk_02017AA0[0xa00];
int16_t gUnk_02018EE0[0x800];
uint8_t gUnk_03003DE4[0xc];
void sub_0801E24C(int32_t radius, int32_t offset);
void sub_0801E290(uint32_t x, uint32_t y, uint32_t radius);

static void compare_native(int width, int height, int nx, int ny, int cx, int cy, unsigned radius) {
    PortEffectSpan rows[192];
    memset(gUnk_02017AA0, 0, sizeof(gUnk_02017AA0));
    sub_0801E24C((int32_t)radius, 0);
    sub_0801E290((uint32_t)cx, (uint32_t)cy, radius);
    assert(Port_Effect_BuildIrisRows(width, height, nx, ny, cx, cy, radius, rows, 192));
    const uint8_t* native = (const uint8_t*)gUnk_02017AA0;
    for (int y = 0; y < 160; ++y) {
        int left = rows[y + ny].left, right = rows[y + ny].right;
        if (left < nx) left = nx;
        if (right > nx + 240) right = nx + 240;
        if (left >= right) left = right = nx;
        assert(left - nx == native[y * 2 + 1]);
        assert(right - nx == native[y * 2]);
    }
}

int main(void) {
    const int centers_x[] = {0, 50, 120, 239};
    const int centers_y[] = {0, 80, 159};
    for (unsigned r = 0; r <= 150; ++r)
        for (unsigned x = 0; x < sizeof(centers_x) / sizeof(*centers_x); ++x)
            for (unsigned y = 0; y < sizeof(centers_y) / sizeof(*centers_y); ++y) {
                compare_native(240, 160, 0, 0, centers_x[x], centers_y[y], r);
                compare_native(256, 192, 8, 16, centers_x[x], centers_y[y], r);
                compare_native(285, 160, 22, 0, centers_x[x], centers_y[y], r);
                compare_native(374, 160, 67, 0, centers_x[x], centers_y[y], r);
                compare_native(569, 160, 164, 0, centers_x[x], centers_y[y], r);
            }
    PortEffectSpan rows[192];
    assert(Port_Effect_BuildIrisRows(256, 192, 8, 16, 120, 80, 20, rows, 192));
    assert(rows[75].left == 0 && rows[75].right == 0);
    assert(rows[96].left == 108 && rows[96].right == 148);
    assert(rows[115].left < rows[115].right && rows[116].left == rows[116].right);
    assert(Port_Effect_BuildIrisRows(569, 160, 164, 0, 10, 80, 200, rows, 192));
    assert(rows[80].left == 0 && rows[80].right == 374);
    memset(rows, 0xa5, sizeof(rows));
    assert(!Port_Effect_BuildIrisRows(256, 192, 8, 16, 120, 80, 1025, rows, 192));
    assert(rows[0].left == 0xa5a5 && rows[191].right == 0xa5a5);
    assert(!Port_Effect_BuildIrisRows(256, 192, 8, 16, 120, 80, 20, rows, 191));
    assert(!Port_Effect_BuildIrisRows(256, 192, -1, 16, 120, 80, 20, rows, 192));
    assert(!Port_Effect_BuildIrisRows(256, 192, 8, 16, 240, 80, 20, rows, 192));
    assert(!Port_Effect_BuildIrisRows(256, 192, 8, 16, 120, 80, 20, NULL, 192));
    assert(rows[0].left == 0xa5a5 && rows[191].right == 0xa5a5);
    puts("Full-canvas iris geometry agrees with native circle: PASS");
    return 0;
}
