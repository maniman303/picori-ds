#include "port_background_png.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    static uint8_t pixels[569 * 192 * 4];
    if (argc != 11) return 2;
    PortAspectMode aspect = (PortAspectMode)atoi(argv[2]);
    int family = atoi(argv[3]), health = atoi(argv[4]);
    PortBackgroundContext context = family == -1 ? PORT_BACKGROUND_FAST_TRAVEL :
        Port_Background_MenuContext((PortBackgroundMenuFamily)family, (PortBackgroundHealth)health);
    const PortViewportProfile* profile = Port_Viewport_ProfileForAspect(aspect);
    int expected = atoi(argv[5]), parity = atoi(argv[6]);
    int color[4] = {atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), atoi(argv[10])};
    memset(pixels, 0x5a, sizeof(pixels));
    int result = Port_Background_LoadPng(argv[1], context, aspect, parity != 0, pixels, sizeof(pixels));
    if (result != expected) return 3;
    if (!result) {
        for (size_t i = 0; i < sizeof(pixels); ++i) if (pixels[i] != 0x5a) return 4;
        return 0;
    }
    if (!profile) return 5;
    size_t size = (size_t)profile->width * profile->height * 4;
    for (size_t i = 0; i < size; ++i) if (pixels[i] != color[i % 4]) return 6;
    for (size_t i = size; i < sizeof(pixels); ++i) if (pixels[i] != 0x5a) return 7;
    memset(pixels, 0x5a, sizeof(pixels));
    if (Port_Background_LoadPng(argv[1], context, aspect, false, pixels, size - 1)) return 8;
    for (size_t i = 0; i < sizeof(pixels); ++i) if (pixels[i] != 0x5a) return 9;
    return 0;
}
