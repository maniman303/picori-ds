#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "port_background_runtime.h"

int main(int argc, char** argv) {
    assert(argc == 2);
    assert(Port_BackgroundRuntime_Init(argv[1]));
    PortViewportGeometry g = {0};
    g.canvas = (PortViewportRect){0,0,285,160};
    g.nativeContent = (PortViewportRect){22,0,240,160};
    g.effectiveAspect = PORT_ASPECT_WIDESCREEN_16_9;
    Port_Background_InvalidatePaletteFades();
    Port_Background_RecordPaletteFade(5, (PortPaletteFade){64, 248}, 1);
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_FAST_TRAVEL, &g, 1, true);
    const PortBackgroundRuntimeSnapshot* snap = Port_BackgroundRuntime_Get();
    assert(snap->source == PORT_BACKGROUND_SOURCE_IMAGE && snap->width == 285 && snap->height == 160);
    assert(snap->effect_policy_valid && snap->effect_policy.layers == 8 && snap->effect_policy.banks == 0x20);
    assert(snap->rgba && snap->rgba[0] == 55 && snap->native.x == 22);
    assert(snap->valid_palette_fades == (1u << 5) && snap->palette_fades[5].intensity == 64);
    PortBackgroundComposeRow resolved;
    const uint8_t backdrop[3] = {1, 2, 3};
    assert(!Port_BackgroundRuntime_ResolveEffect(snap, 8, 0x20, 0x3648, 0x0f01, 0, 1, -1, NULL, &resolved));
    assert(Port_BackgroundRuntime_ResolveEffect(snap, 8, 0x20, 0x3648, 0x0f01, 0, 1, 5, backdrop, &resolved));
    assert(resolved.second_valid && resolved.second_rgb[2] == 3 && resolved.palette_fade.intensity == 64);
    Port_Background_RecordPaletteFade(5, (PortPaletteFade){128, 248}, 1);
    assert(snap->palette_fades[5].intensity == 64);
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_FAST_TRAVEL, &g, 1, true);
    assert(snap->palette_fades[5].intensity == 128);
    char path[4096];
    assert(snprintf(path, sizeof(path), "%s/override/169/fast_travel_background.png", argv[1]) > 0);
    assert(remove(path) == 0);
    for (int i = 0; i < 100; ++i) {
        Port_BackgroundRuntime_Publish(PORT_BACKGROUND_FAST_TRAVEL, &g, 1, true);
        assert(snap->rgba && snap->rgba[0] == 55);
    }
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_MENU_NORMAL, &g, 1, true);
    assert(snap->rgba && snap->rgba[0] == 11 && snap->width == 285);
    g.canvas.width = 374; g.nativeContent.x = 67;
    g.effectiveAspect = PORT_ASPECT_ULTRAWIDE_21_9;
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_MENU_NORMAL, &g, 1, true);
    assert(snap->rgba && snap->rgba[0] == 77 && snap->width == 374 && snap->native.x == 67);
    g.canvas.width = 285; g.nativeContent.x = 22;
    g.effectiveAspect = PORT_ASPECT_WIDESCREEN_16_9;
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_FAST_TRAVEL, &g, 1, true);
    /* The two-slot cache may have evicted this image after profile switches. */
    assert(!snap->rgba);
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_LIVE_WORLD, &g, 1, true);
    assert(snap->source == PORT_BACKGROUND_SOURCE_NONE && !snap->rgba && !snap->width && !snap->effect_policy_valid);
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_GAME_OVER, &g, 1, true);
    assert(snap->source == PORT_BACKGROUND_SOURCE_BLACK && !snap->rgba);
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_FAST_TRAVEL, &g, 1, true);
    assert(!snap->rgba); /* failed key remains cached after scene change */
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_FAST_TRAVEL, &g, 2, true);
    assert(snap->source == PORT_BACKGROUND_SOURCE_IMAGE && !snap->rgba); /* failed new key */
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_FAST_TRAVEL, &g, 2, true);
    assert(!snap->rgba);
    g.flags = PORT_VIEWPORT_PARITY;
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_FAST_TRAVEL, &g, 2, true);
    assert(snap->source == PORT_BACKGROUND_SOURCE_NONE && !snap->rgba);
    g.flags = 0; g.effectiveAspect = PORT_ASPECT_NATIVE_3_2;
    g.canvas.width = 240; g.nativeContent.x = 0;
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_FAST_TRAVEL, &g, 2, true);
    assert(snap->source == PORT_BACKGROUND_SOURCE_NONE && !snap->rgba);
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_GAME_OVER, &g, 2, false);
    assert(!snap->effect_policy_valid); /* no unverified regional assumption */
    Port_BackgroundRuntime_Reset();
    assert(!snap->rgba && snap->source == PORT_BACKGROUND_SOURCE_NONE);
    assert(!Port_BackgroundRuntime_Init(NULL));
    g.canvas.width = 285; g.effectiveAspect = PORT_ASPECT_WIDESCREEN_16_9;
    Port_BackgroundRuntime_Publish(PORT_BACKGROUND_GAME_OVER, &g, 2, true);
    assert(snap->source == PORT_BACKGROUND_SOURCE_BLACK && !snap->rgba);
    puts("Background boundary cache publication: PASS");
    return 0;
}
