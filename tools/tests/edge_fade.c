/* ROM-free live-gameplay margin fade. */
#include "port_edge_fade.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t frame[PORT_VIEWPORT_MAX_WIDTH * PORT_VIEWPORT_MAX_HEIGHT];
static uint32_t original[PORT_VIEWPORT_MAX_WIDTH * PORT_VIEWPORT_MAX_HEIGHT];
static uint8_t ui_mask[PORT_VIEWPORT_MAX_WIDTH * PORT_VIEWPORT_MAX_HEIGHT];
static const int pitch = 260;
static const uint32_t fill_color = 0xff102030u;
static const uint32_t hud_color = 0xffff0000u;

static uint32_t pixel(int x, int y) {
    return 0xff000000u | ((uint32_t)(y * 3 + 7) << 16) |
           ((uint32_t)(x * 5 + 11) << 8) | (uint32_t)(x + y + 17);
}

static uint32_t fade(uint32_t src, int d) {
    const unsigned factor = (unsigned)(PORT_EDGE_FADE_PIXELS - d);
    return (src & 0xff000000u) |
        ((((src >> 16) & 0xffu) * factor / PORT_EDGE_FADE_PIXELS) << 16) |
        ((((src >> 8) & 0xffu) * factor / PORT_EDGE_FADE_PIXELS) << 8) |
        (((src & 0xffu) * factor / PORT_EDGE_FADE_PIXELS));
}

static void fill(int width, int height, PortViewportRect world) {
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            frame[(size_t)y * pitch + x] = fill_color;
    for (int y = world.y; y < world.y + world.height; ++y)
        for (int x = world.x; x < world.x + world.width; ++x)
            frame[(size_t)y * pitch + x] = pixel(x, y);
    memcpy(original, frame, sizeof(frame));
    memset(ui_mask, 0, sizeof(ui_mask));
}

static void expect_world(PortViewportRect world) {
    for (int y = world.y; y < world.y + world.height; ++y)
        for (int x = world.x; x < world.x + world.width; ++x)
            assert(frame[(size_t)y * pitch + x] == original[(size_t)y * pitch + x]);
}

int main(void) {
    /* 4:3 live world with side bars only. */
    PortViewportRect world = { 8, 0, 240, 192 };
    fill(256, 192, world);
    assert(Port_EdgeFade_Apply(frame, pitch, 256, 192, world, NULL, 0));
    expect_world(world);
    for (int d = 1; d <= PORT_EDGE_FADE_PIXELS; ++d) {
        const int x = world.x - d;
        const uint32_t expected = d == PORT_EDGE_FADE_PIXELS ? 0xff000000u :
            fade(pixel(world.x, 42), d);
        assert(frame[(size_t)42 * pitch + x] == expected);
        const int rx = world.x + world.width - 1 + d;
        const uint32_t rexpected = d == PORT_EDGE_FADE_PIXELS ? 0xff000000u :
            fade(pixel(world.x + world.width - 1, 42), d);
        assert(frame[(size_t)42 * pitch + rx] == rexpected);
    }
    assert(frame[(size_t)42 * pitch + 0] == 0xff000000u);
    assert(frame[(size_t)42 * pitch + 255] == 0xff000000u);

    /* Canvas UI is preserved by the mask, including black UI pixels that
     * happen to equal the background. */
    fill(256, 192, world);
    frame[(size_t)42 * pitch + 1] = hud_color;
    frame[(size_t)42 * pitch + 2] = 0xff000000u;
    ui_mask[(size_t)42 * pitch + 1] = 1;
    ui_mask[(size_t)42 * pitch + 2] = 1;
    memcpy(original, frame, sizeof(frame));
    assert(Port_EdgeFade_Apply(frame, pitch, 256, 192, world, ui_mask, pitch));
    assert(frame[(size_t)42 * pitch + 1] == hud_color);
    assert(frame[(size_t)42 * pitch + 2] == 0xff000000u);
    assert(frame[(size_t)42 * pitch + 7] == fade(pixel(world.x, 42), 1));

    /* A HUD pixel on the world edge must not bleed into the margin; the fade
     * uses the adjacent visible map pixel instead. */
    fill(256, 192, world);
    ui_mask[(size_t)42 * pitch + world.x] = 1;
    ui_mask[(size_t)42 * pitch + world.x + world.width - 1] = 1;
    assert(Port_EdgeFade_Apply(frame, pitch, 256, 192, world, ui_mask, pitch));
    assert(frame[(size_t)42 * pitch + 7] == fade(pixel(world.x + 1, 42), 1));
    assert(frame[(size_t)42 * pitch + 6] == fade(pixel(world.x + 1, 42), 2));
    assert(frame[(size_t)42 * pitch + world.x] == pixel(world.x, 42));

    /* 4:3 world with top and bottom margins: vertical fade plus corners. */
    world = (PortViewportRect){ 8, 16, 240, 160 };
    fill(256, 192, world);
    assert(Port_EdgeFade_Apply(frame, pitch, 256, 192, world, NULL, 0));
    expect_world(world);
    assert(frame[(size_t)15 * pitch + 120] == fade(pixel(120, 16), 1));
    assert(frame[(size_t)11 * pitch + 120] == 0xff000000u);
    assert(frame[(size_t)176 * pitch + 120] == fade(pixel(120, 175), 1));
    assert(frame[(size_t)180 * pitch + 120] == 0xff000000u);
    /* Corner uses the nearest corner pixel; the fade is a uniform 5px frame. */
    assert(frame[(size_t)15 * pitch + 7] == fade(pixel(8, 16), 1));
    assert(frame[(size_t)15 * pitch + 3] == 0xff000000u);
    /* Rounded corners: diagonal distance, not a square Chebyshev band. */
    assert(frame[(size_t)14 * pitch + 6] == fade(pixel(8, 16), 3));
    assert(frame[(size_t)14 * pitch + 7] == fade(pixel(8, 16), 2));
    assert(frame[(size_t)13 * pitch + 5] == fade(pixel(8, 16), 4));
    assert(frame[(size_t)12 * pitch + 4] == 0xff000000u);
    assert(frame[(size_t)176 * pitch + 248] == fade(pixel(247, 175), 1));
    assert(frame[(size_t)176 * pitch + 252] == 0xff000000u);
    assert(frame[(size_t)0 * pitch + 0] == 0xff000000u);

    /* Black-border room: the edge color is black, so the fade is black and
     * the original border is preserved. */
    world = (PortViewportRect){ 8, 16, 240, 160 };
    fill(256, 192, world);
    for (int x = world.x; x < world.x + world.width; ++x) {
        frame[(size_t)16 * pitch + x] = 0xff000000u;
        frame[(size_t)175 * pitch + x] = 0xff000000u;
    }
    for (int y = world.y; y < world.y + world.height; ++y) {
        frame[(size_t)y * pitch + world.x] = 0xff000000u;
        frame[(size_t)y * pitch + world.x + world.width - 1] = 0xff000000u;
    }
    assert(Port_EdgeFade_Apply(frame, pitch, 256, 192, world, NULL, 0));
    assert(frame[(size_t)15 * pitch + 120] == 0xff000000u);
    assert(frame[(size_t)176 * pitch + 120] == 0xff000000u);
    assert(frame[(size_t)42 * pitch + 7] == 0xff000000u);
    assert(frame[(size_t)42 * pitch + 248] == 0xff000000u);

    /* Full-canvas world: nothing to paint, no writes. */
    world = (PortViewportRect){ 0, 0, 256, 192 };
    fill(256, 192, world);
    assert(!Port_EdgeFade_Apply(frame, pitch, 256, 192, world, NULL, 0));
    assert(memcmp(frame, original, sizeof(frame)) == 0);

    /* Invalid inputs fail closed without touching the frame. */
    fill(240, 160, (PortViewportRect){ 0, 0, 240, 160 });
    assert(!Port_EdgeFade_Apply(frame, 239, 240, 160, (PortViewportRect){ 0, 0, 240, 160 }, NULL, 0));
    assert(!Port_EdgeFade_Apply(NULL, pitch, 240, 160, (PortViewportRect){ 0, 0, 240, 160 }, NULL, 0));
    assert(!Port_EdgeFade_Apply(frame, pitch, 240, 160, (PortViewportRect){ -1, 0, 240, 160 }, NULL, 0));
    assert(!Port_EdgeFade_Apply(frame, pitch, 240, 160, (PortViewportRect){ 0, 0, 241, 160 }, NULL, 0));
    assert(!Port_EdgeFade_Apply(frame, pitch, 240, 160, (PortViewportRect){ 0, 0, 0, 160 }, NULL, 0));
    assert(!Port_EdgeFade_Apply(frame, pitch, 240, 160, (PortViewportRect){ 0, 0, 239, 160 }, ui_mask, 10));
    assert(memcmp(frame, original, sizeof(frame)) == 0);

    puts("Live gameplay edge fade: PASS");
    return 0;
}
