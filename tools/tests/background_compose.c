#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "port_background_compose.h"

static uint32_t canvas[192 * 576];
static uint8_t rgba[192 * 569 * 4];
static PortBackgroundComposeRow rows[192];

int main(void) {
    const int sizes[][2] = {{240,160},{256,192},{285,160},{374,160},{569,160}};
    for (unsigned i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i) {
        int w = sizes[i][0], h = sizes[i][1], nx = (w-240)/2, ny = (h-160)/2;
        for (int j = 0; j < 192*576; ++j) canvas[j] = 0x12345678;
        for (int j = 0; j < 192*569; ++j) {
            rgba[j*4] = 201; rgba[j*4+1] = 101; rgba[j*4+2] = 51; rgba[j*4+3] = 128;
        }
        memset(rows, 0, sizeof(rows));
        for (int y = 0; y < h; ++y) rows[y].layer_effect = PORT_LAYER_EFFECT_NONE;
        PortBackgroundComposition c = {{0,0,w,h},{nx,ny,240,160},rgba,w*4,{0,0,0},rows,h};
        assert(Port_Background_ComposeMargins(canvas,576,&c));
        for (int y = 0; y < h; ++y) for (int x = 0; x < 576; ++x) {
            int native = x >= nx && x < nx+240 && y >= ny && y < ny+160;
            assert(canvas[y*576+x] == (x >= w || native ? 0x12345678u : 0xff1a3365u));
        }
        rows[0].layer_effect = PORT_LAYER_EFFECT_MIXED;
        canvas[0] = 0x12345678;
        assert(!Port_Background_ComposeMargins(canvas,576,&c) && canvas[0] == 0x12345678);
    }
    memset(rows, 0, sizeof(rows));
    for (int y = 0; y < 160; ++y) rows[y].layer_effect = PORT_LAYER_EFFECT_NONE;
    PortBackgroundComposition c = {{0,0,285,160},{22,0,240,160},NULL,0,{100,80,60},rows,160};
    rows[0].apply_palette_fade = true; rows[0].palette_fade = (PortPaletteFade){128,248};
    rows[1].layer_effect = PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT; rows[1].coefficient = 0x0808;
    assert(!Port_Background_ComposeMargins(canvas,576,&c));
    rows[1].second_valid = true;
    rows[1].second_rgb[0]=20; rows[1].second_rgb[1]=40; rows[1].second_rgb[2]=60;
    rows[2].forced_blank = true;
    assert(Port_Background_ComposeMargins(canvas,576,&c));
    assert(canvas[0] == 0xff9da7b1u);
    assert(canvas[576] == 0xff3c3c3cu);
    assert(canvas[1152] == 0xffffffffu);
    puts("Background margin composition: PASS");
    return 0;
}
