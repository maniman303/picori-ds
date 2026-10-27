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
    { 0, 0, 240, 160 }, { 0, 0, 0 },
    240, 160, 240, 0, 0
};

PortViewportRect Port_Viewport_ResolveWorldRect(PortViewportRect canvas, PortViewportScene scene) {
    PortViewportRect world = { 0, 0, 240, 160 };
    const uint32_t wide = PORT_SCENE_WORLD | PORT_SCENE_WIDE_WORLD;
    const uint32_t tall = PORT_SCENE_WORLD | PORT_SCENE_MAP_LIVE | PORT_SCENE_TALL_MANAGER_FREE |
                          PORT_SCENE_TALL_VALIDATED | PORT_SCENE_TALL_ROWS_READY;
    const uint32_t native = PORT_SCENE_NATIVE_TRANSITION | PORT_SCENE_NATIVE_CONTENT;
    if (canvas.x != 0 || canvas.y != 0 || canvas.width < 240 || canvas.width > 569 ||
        canvas.height < 160 || canvas.height > 192)
        return world;
    if ((scene.capabilities & wide) == wide && !(scene.capabilities & native) &&
        scene.roomWidth > 240 && scene.roomHeight > 0)
        world.width = scene.roomWidth < canvas.width ? scene.roomWidth : canvas.width;
    if ((scene.capabilities & tall) == tall && !(scene.capabilities & (native | PORT_SCENE_TALL_SHAKE_UNSAFE)) &&
        scene.roomWidth >= 240 && scene.roomHeight > 160 && canvas.height > 160)
        world.height = scene.roomHeight < canvas.height ? scene.roomHeight : canvas.height;
    world.x = canvas.x + (canvas.width - world.width) / 2;
    world.y = canvas.y + (canvas.height - world.height) / 2;
    return world;
}

static void ResolveWorld(void) {
    sGeometry.world = Port_Viewport_ResolveWorldRect(sGeometry.canvas, sGeometry.scene);
}

void Port_Viewport_PublishScene(PortViewportScene scene) {
    if (sGeometry.scene.capabilities == scene.capabilities &&
        sGeometry.scene.roomWidth == scene.roomWidth && sGeometry.scene.roomHeight == scene.roomHeight)
        return;
    sGeometry.scene = scene;
    ResolveWorld();
    ++sGeometry.generation;
}

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
#ifdef TMC_N64
    return aspect == PORT_ASPECT_NATIVE_3_2 &&
           Port_Viewport_ProfileFits(profile, capacityWidth, capacityHeight);
#else
#ifdef PORT_VIEWPORT_TEST_TALL
    if (aspect == PORT_ASPECT_DS_4_3)
        return Port_Viewport_ProfileFits(profile, capacityWidth, capacityHeight);
#endif
    return Port_Viewport_ProfileFits(profile, capacityWidth, capacityHeight) &&
           profile->width <= 512 && profile->height <= 160;
#endif
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
    ResolveWorld();
    ++sGeometry.generation;
    return true;
}

const PortViewportGeometry* Port_Viewport_Get(void) {
    return &sGeometry;
}

int Port_Viewport_RasterHeight(const PortViewportGeometry* geometry) {
    if (!geometry || geometry->canvas.height < 160 || geometry->canvas.height > 192)
        return 160;
    if ((geometry->scene.capabilities & PORT_SCENE_WORLD) &&
        geometry->world.height >= 160 && geometry->world.height <= geometry->canvas.height)
        return geometry->world.height;
    if (geometry->scene.capabilities & PORT_SCENE_WORLD)
        return 160;
    return geometry->canvas.height;
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

/* Whole-sequence classification prevents a later negative/positive phase
 * from invalidating a tall aperture after camera and map-fill work. */
int Port_Viewport_ShakeSequenceRows(int viewHeight, const uint8_t* xyPairs) {
    int rows = Port_Viewport_MapTileRows(viewHeight);
    if (!xyPairs || !rows)
        return 0;
    for (int i = 0; i < 8; ++i) {
        unsigned value = xyPairs[2 * i + 1];
        int y = value < 128 ? (int)value : (int)value - 256;
        int phaseRows = Port_Viewport_MapTileRowsWithShake(viewHeight, y);
        if (!phaseRows)
            return 0;
        if (phaseRows > rows)
            rows = phaseRows;
    }
    return rows;
}

bool Port_Viewport_ShakeSequenceFits(int viewHeight, const uint8_t* xyPairs) {
    return Port_Viewport_ShakeSequenceRows(viewHeight, xyPairs) != 0;
}

void Port_Viewport_UploadReset(PortViewportUpload* u) {
    if (u) memset(u, 0, sizeof(*u));
}

static bool UploadMatches(const PortViewportUpload* u, uint64_t key, int x, int y, int rows) {
    return u && u->requested && u->roomKey == key && u->tileX == x && u->tileY == y && u->rows == rows;
}

bool Port_Viewport_UploadRequest(PortViewportUpload* u, uint64_t key, int x, int y, int height, int rows) {
    if (!u || height <= 160 || height > 192 || rows < Port_Viewport_MapTileRows(height) || rows > 32) {
        Port_Viewport_UploadReset(u);
        return false;
    }
    if (!UploadMatches(u, key, x, y, rows) || u->height != height) {
        u->roomKey = key; u->tileX = x; u->tileY = y; u->height = height; u->rows = rows;
        u->requested = true; u->filled = false; u->uploaded = false;
    }
    return u->uploaded;
}

void Port_Viewport_UploadFilled(PortViewportUpload* u, uint64_t key, int x, int y, int rows) {
    if (!u) return;
    u->filled = UploadMatches(u, key, x, y, rows);
    u->uploaded = false;
}

void Port_Viewport_UploadCompleted(PortViewportUpload* u, uint64_t key, int x, int y, int rows) {
    if (!u) return;
    u->uploaded = u->filled && UploadMatches(u, key, x, y, rows);
}
