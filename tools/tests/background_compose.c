#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "port_background_compose.h"

static uint32_t canvas[192 * 576];
static uint8_t rgba[192 * 569 * 4];
static PortBackgroundComposeRow rows[192], outside[192], right_rows[192];
static PortBackgroundComposeRow top_rows[240], bottom_rows[240];
static PortEffectSpan spans[192];
static uint8_t curve[3][256];

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
    for (int channel = 0; channel < 3; ++channel)
        for (int value = 0; value < 256; ++value) curve[channel][value] = value / 2;
    rows[3].brightness_curve = (const uint8_t (*)[256])curve;
    rows[2].forced_blank = true;
    rows[2].layer_effect = PORT_LAYER_EFFECT_INVALID; /* not needed for forced blank */
    assert(Port_Background_ComposeMargins(canvas,576,&c));
    assert(canvas[0] == 0xff9da7b1u);
    assert(canvas[576] == 0xff3c3c3cu);
    assert(canvas[1152] == 0xffffffffu);
    assert(canvas[1728] == 0xff1e2832u);
    /* Distinct actual second inputs on the two native edges. */
    for (int y = 0; y < 160; ++y) {
        rows[y] = (PortBackgroundComposeRow){0}; right_rows[y] = rows[y];
        rows[y].layer_effect = right_rows[y].layer_effect = PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT;
        rows[y].second_valid = right_rows[y].second_valid = true;
        rows[y].coefficient = right_rows[y].coefficient = 0x1000;
        rows[y].second_rgb[0] = 10; right_rows[y].second_rgb[0] = 200;
    }
    assert(Port_Background_ComposeEdgeMargins(canvas,576,&c,right_rows,160,NULL,0,NULL,0,NULL));
    assert(canvas[0] == 0xff00000a && canvas[284] == 0xff0000c8);
    canvas[0] = 0x12345678; right_rows[159].second_valid = false;
    assert(!Port_Background_ComposeEdgeMargins(canvas,576,&c,right_rows,160,NULL,0,NULL,0,NULL));
    assert(canvas[0] == 0x12345678);

    /* Top/bottom ownership varies across columns; corners extend the edge
     * endpoint, and no native pixel or pitch padding is changed. */
    for (int y = 0; y < 192; ++y) {
        rows[y] = right_rows[y] = (PortBackgroundComposeRow){0};
        rows[y].layer_effect = right_rows[y].layer_effect = PORT_LAYER_EFFECT_NONE;
    }
    for (int x = 0; x < 240; ++x) {
        top_rows[x] = bottom_rows[x] = (PortBackgroundComposeRow){0};
        top_rows[x].layer_effect = bottom_rows[x].layer_effect = PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT;
        top_rows[x].second_valid = bottom_rows[x].second_valid = true;
        top_rows[x].coefficient = bottom_rows[x].coefficient = 0x1000;
        top_rows[x].second_rgb[0] = (uint8_t)x;
        bottom_rows[x].second_rgb[0] = (uint8_t)(239 - x);
    }
    for (int j = 0; j < 192*576; ++j) canvas[j] = 0x12345678;
    PortBackgroundComposition tall = {{0,0,256,192},{8,16,240,160},NULL,0,{1,2,3},rows,192};
    assert(Port_Background_ComposePerimeter(canvas,576,&tall,right_rows,192,top_rows,bottom_rows,240,
                                             NULL,0,NULL,0,NULL));
    assert(canvas[0] == 0xff000000 && canvas[8] == 0xff000000 && canvas[247] == 0xff0000ef);
    assert(canvas[255] == 0xff0000ef && canvas[191*576] == 0xff0000ef && canvas[191*576+255] == 0xff000000);
    assert(canvas[16*576+8] == 0x12345678 && canvas[255+256] == 0x12345678);
    canvas[0] = 0x12345678; top_rows[239].second_valid = false;
    assert(!Port_Background_ComposePerimeter(canvas,576,&tall,right_rows,192,top_rows,bottom_rows,240,
                                              NULL,0,NULL,0,NULL));
    assert(canvas[0] == 0x12345678);

    /* Exercise the real published iris geometry at every native radius. */
    const uint8_t black[3] = {0, 0, 0};
    for (unsigned i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i) {
        int w = sizes[i][0], h = sizes[i][1], nx = (w-240)/2, ny = (h-160)/2;
        memset(rows, 0, sizeof(rows));
        memset(outside, 0, sizeof(outside));
        for (int y = 0; y < h; ++y) {
            rows[y].layer_effect = PORT_LAYER_EFFECT_NONE;
            outside[y].layer_effect = PORT_LAYER_EFFECT_NONE;
        }
        PortBackgroundComposition window = {{0,0,w,h},{nx,ny,240,160},NULL,0,{7,19,33},rows,h};
        for (int radius = 0; radius <= 150; ++radius) {
            Port_Effect_PublishFadeIris(120, 80, radius, 0x3f3f, 0);
            Port_Effect_CommitFadeIris();
            assert(Port_Effect_ResolveFadeIris(w,h,nx,ny,spans,192));
            for (int j = 0; j < 192*576; ++j) canvas[j] = 0x12345678;
            assert(Port_Background_ComposeWindowMargins(canvas,576,&window,spans,h,outside,h,black));
            for (int y = 0; y < h; ++y) for (int x = 0; x < 576; ++x) {
                int native = x >= nx && x < nx+240 && y >= ny && y < ny+160;
                uint32_t expected = x >= w || native ? 0x12345678u :
                    x >= spans[y].left && x < spans[y].right ? 0xff211307u : 0xff000000u;
                assert(canvas[y*576+x] == expected);
            }
        }
        /* Independent outside effect state is applied only outside the span. */
        for (int y = 0; y < h; ++y) { spans[y].left = 0; spans[y].right = w / 2; }
        outside[0].layer_effect = PORT_LAYER_EFFECT_BRIGHTEN;
        outside[0].coefficient = 16;
        assert(Port_Background_ComposeWindowMargins(canvas,576,&window,spans,h,outside,h,black));
        if (w > 240) assert(canvas[w-1] == 0xffffffffu);
        outside[0].layer_effect = PORT_LAYER_EFFECT_NONE;
        canvas[0] = 0x12345678;
        spans[h-1].right = w+1;
        assert(!Port_Background_ComposeWindowMargins(canvas,576,&window,spans,h,outside,h,black));
        assert(canvas[0] == 0x12345678);
        spans[h-1].right = spans[h-1].left = 0;
        outside[h-1].layer_effect = PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT;
        assert(!Port_Background_ComposeWindowMargins(canvas,576,&window,spans,h,outside,h,black));
        assert(canvas[0] == 0x12345678);
    }
    puts("Background margin composition: PASS");
    return 0;
}
