#include "port_viewport.h"

#include <stddef.h>
#include <string.h>

static const PortViewportProfile sProfiles[] = {
    { "3:2", "32", 240, 160 },
    { "4:3", "43", 256, 192 },
    { "16:9", "169", 285, 160 },
    { "21:9", "219", 374, 160 },
    { "32:9", "329", 569, 160 },
};

const PortViewportProfile* Port_Viewport_FindProfile(const char* directory) {
    if (directory != NULL) {
        for (size_t i = 0; i < sizeof(sProfiles) / sizeof(sProfiles[0]); ++i) {
            if (strcmp(directory, sProfiles[i].directory) == 0)
                return &sProfiles[i];
        }
    }
    return NULL;
}

bool Port_Viewport_ProfileFits(const PortViewportProfile* profile, int capacityWidth, int capacityHeight) {
    return profile != NULL && profile->width >= PORT_VIEWPORT_NATIVE_WIDTH &&
           profile->height >= PORT_VIEWPORT_NATIVE_HEIGHT && profile->width <= capacityWidth &&
           profile->height <= capacityHeight;
}

PortViewportRect Port_Viewport_NativeContentRect(const PortViewportProfile* profile) {
    PortViewportRect rect = { 0, 0, PORT_VIEWPORT_NATIVE_WIDTH, PORT_VIEWPORT_NATIVE_HEIGHT };
    if (profile != NULL && profile->width >= rect.width && profile->height >= rect.height) {
        rect.x = (profile->width - rect.width) / 2;
        rect.y = (profile->height - rect.height) / 2;
    }
    return rect;
}
