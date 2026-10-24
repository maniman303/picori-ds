#include "port_viewport.h"

#include <stddef.h>
#include <string.h>
#include <limits.h>

static const PortViewportProfile sProfiles[] = {
    { "3:2", "32", 240, 160 },
    { "4:3", "43", 256, 192 },
    { "16:9", "169", 285, 160 },
    { "21:9", "219", 374, 160 },
    { "32:9", "329", 569, 160 },
};

static PortViewportGeometry sGeometry = {
    PORT_ASPECT_NATIVE_3_2, PORT_ASPECT_NATIVE_3_2,
    { 0, 0, 240, 160 }, { 0, 0, 240, 160 }, { 0, 0, 240, 160 },
    240, 160, 240, 0, 0
};

const PortViewportProfile* Port_Viewport_ProfileForAspect(PortAspectMode aspect) {
    switch (aspect) {
        case PORT_ASPECT_NATIVE_3_2: return &sProfiles[0];
        case PORT_ASPECT_DS_4_3: return &sProfiles[1];
        case PORT_ASPECT_WIDESCREEN_16_9: return &sProfiles[2];
        case PORT_ASPECT_ULTRAWIDE_21_9: return &sProfiles[3];
        case PORT_ASPECT_SUPER_ULTRAWIDE_32_9: return &sProfiles[4];
        default: return NULL;
    }
}

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

bool Port_Viewport_AspectSupported(PortAspectMode aspect, int capacityWidth, int capacityHeight) {
    const PortViewportProfile* profile = Port_Viewport_ProfileForAspect(aspect);
    return Port_Viewport_ProfileFits(profile, capacityWidth, capacityHeight) &&
           profile->width <= 512 && profile->height <= 160;
}

bool Port_Viewport_Publish(PortAspectMode selected, bool consoleParity,
                           int capacityWidth, int capacityHeight, int pitchPixels) {
    if (capacityWidth < 240 || capacityHeight < 160 || pitchPixels < capacityWidth)
        return false;
    if (Port_Viewport_ProfileForAspect(selected) == NULL)
        selected = PORT_ASPECT_NATIVE_3_2;
    uint32_t flags = consoleParity ? PORT_VIEWPORT_PARITY : 0;
    if (!Port_Viewport_AspectSupported(selected, capacityWidth, capacityHeight))
        flags |= PORT_VIEWPORT_UNSUPPORTED;
    PortAspectMode effective = flags ? PORT_ASPECT_NATIVE_3_2 : selected;
    /* Generation changes only with geometry/policy, not every frame. */
    if (sGeometry.generation != 0 && sGeometry.selectedAspect == selected &&
        sGeometry.effectiveAspect == effective && sGeometry.flags == flags &&
        sGeometry.capacityWidth == capacityWidth && sGeometry.capacityHeight == capacityHeight &&
        sGeometry.pitchPixels == pitchPixels)
        return true;
    const PortViewportProfile* profile = Port_Viewport_ProfileForAspect(effective);
    sGeometry.selectedAspect = selected;
    sGeometry.effectiveAspect = effective;
    PortViewportRect canvas = { 0, 0, profile->width, profile->height };
    sGeometry.canvas = canvas;
    sGeometry.nativeContent = Port_Viewport_NativeContentRect(profile);
    sGeometry.uiSafe = sGeometry.canvas;
    sGeometry.capacityWidth = capacityWidth;
    sGeometry.capacityHeight = capacityHeight;
    sGeometry.pitchPixels = pitchPixels;
    sGeometry.flags = flags;
    ++sGeometry.generation;
    return true;
}

const PortViewportGeometry* Port_Viewport_Get(void) {
    return &sGeometry;
}

PortViewportRect Port_Viewport_NativeContentRect(const PortViewportProfile* profile) {
    PortViewportRect rect = { 0, 0, PORT_VIEWPORT_NATIVE_WIDTH, PORT_VIEWPORT_NATIVE_HEIGHT };
    if (profile != NULL && profile->width >= rect.width && profile->height >= rect.height) {
        rect.x = (profile->width - rect.width) / 2;
        rect.y = (profile->height - rect.height) / 2;
    }
    return rect;
}

int Port_Viewport_CameraRest(int target, int origin, int roomExtent, int viewExtent) {
    if (roomExtent <= 0 || viewExtent <= 0)
        return origin;
    int64_t hi = (int64_t)origin + roomExtent - viewExtent;
    if (hi < origin)
        hi = origin;
    int64_t rest = (int64_t)target - viewExtent / 2;
    if (rest > hi)
        rest = hi;
    if (rest < origin)
        rest = origin;
    return rest > INT_MAX ? INT_MAX : (int)rest;
}

int Port_Viewport_MapTileRows(int viewHeight) {
    return Port_Viewport_MapTileRowsWithShake(viewHeight, 0);
}

int Port_Viewport_MapTileRowsWithShake(int viewHeight, int shakeY) {
    if (viewHeight <= 0 || (int64_t)8 + shakeY < 0 || (int64_t)viewHeight + 22 + shakeY >= 256)
        return 0;
    /* Retain the unshaken window as a minimum so negative shake does not
     * shrink the backing rows between frames. Positive shake adds padding. */
    const int64_t rows = ((int64_t)viewHeight + 22 + (shakeY > 0 ? shakeY : 0)) / 8 + 1;
    return rows <= 32 ? (int)rows : 0;
}
