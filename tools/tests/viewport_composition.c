/* Profile-general viewport composition: live aperture centering, fallback and
 * canvas/raster agreement at 4:3 and the synthetic 288x162 extent. */
#include "port_viewport.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const unsigned kTall =
    PORT_SCENE_WORLD | PORT_SCENE_WIDE_WORLD | PORT_SCENE_MAP_LIVE |
    PORT_SCENE_TALL_MANAGER_FREE |
    PORT_SCENE_TALL_VALIDATED | PORT_SCENE_TALL_ROWS_READY |
    PORT_SCENE_TALL_ENTITIES_EMPTY | PORT_SCENE_TALL_OAM_VERIFIED;

static void expect_rect(PortViewportRect r, int x, int y, int w, int h) {
    assert(r.x == x && r.y == y && r.width == w && r.height == h);
}

int main(void) {
#if PORT_VIEWPORT_RELEASE_TALL
    /* The release path is profile/capacity based: tall profiles need the
     * 569x192 capacity, and the OBJ-safe width bound still rejects 32:9. */
    assert(Port_Viewport_AspectSupported(PORT_ASPECT_DS_4_3, 569, 192));
    assert(!Port_Viewport_AspectSupported(PORT_ASPECT_DS_4_3, 240, 192));
    assert(!Port_Viewport_AspectSupported(PORT_ASPECT_SUPER_ULTRAWIDE_32_9, 569, 192));
#endif
    /* Published 4:3 canvas: a native world aperture is centered. */
    assert(Port_Viewport_Publish(PORT_ASPECT_DS_4_3, false, 569, 192, 569));
    const PortViewportGeometry* g = Port_Viewport_Get();
    expect_rect(g->canvas, 0, 0, 256, 192);
    expect_rect(g->nativeContent, 8, 16, 240, 160);

    PortViewportScene scene = { PORT_SCENE_WORLD | PORT_SCENE_WIDE_WORLD | PORT_SCENE_MAP_LIVE, 240, 160 };
    Port_Viewport_PublishScene(scene);
    expect_rect(g->world, 8, 16, 240, 160);
    assert(Port_Viewport_Composition(g) == PORT_COMPOSITION_LIVE_WORLD);
    assert(Port_Viewport_RasterHeight(g) == 192);

    scene.roomWidth = 256;
    Port_Viewport_PublishScene(scene);
    expect_rect(g->world, 0, 16, 256, 160);
    assert(Port_Viewport_RasterHeight(g) == 192);

    scene.capabilities = kTall;
    scene.roomWidth = 240;
    scene.roomHeight = 192;
    Port_Viewport_PublishScene(scene);
    expect_rect(g->world, 8, 0, 240, 192);
    assert(Port_Viewport_RasterHeight(g) == 192);

    scene.roomWidth = 256;
    scene.roomHeight = 192;
    Port_Viewport_PublishScene(scene);
    expect_rect(g->world, 0, 0, 256, 192);
    assert(Port_Viewport_RasterHeight(g) == 192);

    /* A classified fallback is one native composition centered in the canvas. */
    scene.capabilities = kTall | PORT_SCENE_NATIVE_FALLBACK;
    Port_Viewport_PublishScene(scene);
    expect_rect(g->world, 8, 16, 240, 160);
    assert(Port_Viewport_Composition(g) == PORT_COMPOSITION_NATIVE_FALLBACK);
    assert(Port_Viewport_RasterHeight(g) == 160);

    scene.capabilities = PORT_SCENE_WORLD | PORT_SCENE_FIXED_SCREEN;
    Port_Viewport_PublishScene(scene);
    assert(Port_Viewport_Composition(g) == PORT_COMPOSITION_FIXED_SCREEN);
    assert(Port_Viewport_RasterHeight(g) == 160);

    scene.capabilities = 0;
    Port_Viewport_PublishScene(scene);
    assert(Port_Viewport_Composition(g) == PORT_COMPOSITION_NATIVE);

    /* Native/wide canvases keep the existing direct path. */
    scene.capabilities = PORT_SCENE_WORLD | PORT_SCENE_WIDE_WORLD | PORT_SCENE_MAP_LIVE;
    Port_Viewport_PublishScene(scene);
    assert(Port_Viewport_Publish(PORT_ASPECT_NATIVE_3_2, false, 569, 192, 569));
    expect_rect(g->canvas, 0, 0, 240, 160);
    scene.roomWidth = 240;
    scene.roomHeight = 160;
    Port_Viewport_PublishScene(scene);
    expect_rect(g->world, 0, 0, 240, 160);
    assert(Port_Viewport_RasterHeight(g) == 160);
    assert(Port_Viewport_Publish(PORT_ASPECT_WIDESCREEN_16_9, false, 569, 192, 569));
    expect_rect(g->canvas, 0, 0, 285, 160);
    scene.roomWidth = 1024;
    Port_Viewport_PublishScene(scene);
    expect_rect(g->world, 0, 0, 285, 160);
    assert(Port_Viewport_RasterHeight(g) == 160);

    /* Synthetic proper-16:9 extent (288x162): profile-general, no 192/4:3
     * branch. A 160-row room gets one fill row above and below. */
    PortViewportRect canvas = { 0, 0, 288, 162 };
    PortViewportScene synthetic = { PORT_SCENE_WORLD | PORT_SCENE_WIDE_WORLD, 240, 160 };
    expect_rect(Port_Viewport_ResolveWorldRect(canvas, synthetic), 24, 1, 240, 160);
    synthetic.roomWidth = 288;
    expect_rect(Port_Viewport_ResolveWorldRect(canvas, synthetic), 0, 1, 288, 160);
    synthetic.capabilities = PORT_SCENE_WORLD | PORT_SCENE_WIDE_WORLD | PORT_SCENE_MAP_LIVE |
                             PORT_SCENE_TALL_MANAGER_FREE | PORT_SCENE_TALL_VALIDATED |
                             PORT_SCENE_TALL_ROWS_READY | PORT_SCENE_TALL_ENTITIES_EMPTY |
                             PORT_SCENE_TALL_OAM_VERIFIED;
    synthetic.roomWidth = 288;
    synthetic.roomHeight = 192;
    expect_rect(Port_Viewport_ResolveWorldRect(canvas, synthetic), 0, 0, 288, 162);
    synthetic.roomHeight = 160;
    expect_rect(Port_Viewport_ResolveWorldRect(canvas, synthetic), 0, 1, 288, 160);

    PortViewportGeometry tallSynthetic;
    memset(&tallSynthetic, 0, sizeof(tallSynthetic));
    tallSynthetic.canvas = canvas;
    tallSynthetic.scene.capabilities = PORT_SCENE_WORLD | PORT_SCENE_MAP_LIVE;
    tallSynthetic.world = (PortViewportRect){ 24, 1, 240, 160 };
    assert(Port_Viewport_RasterHeight(&tallSynthetic) == 162);
    tallSynthetic.scene.capabilities = PORT_SCENE_WORLD | PORT_SCENE_NATIVE_FALLBACK;
    assert(Port_Viewport_RasterHeight(&tallSynthetic) == 160);

    /* UI anchors are canvas-based on any published extent. */
    PortUITranslation top = Port_Viewport_UITranslation(PORT_UI_TOP_CENTER, 288, 162);
    assert(top.x == 24 && top.y == 0);
    PortUITranslation bottomLeft = Port_Viewport_UITranslation(PORT_UI_BOTTOM_LEFT, 288, 162);
    assert(bottomLeft.x == 0 && bottomLeft.y == 2);
    PortUITranslation bottomRight = Port_Viewport_UITranslation(PORT_UI_BOTTOM_RIGHT, 256, 192);
    assert(bottomRight.x == 16 && bottomRight.y == 32);

    /* Two-phase staging accepts the 162-row extent with its own row count. */
    PortViewportUpload upload;
    Port_Viewport_UploadReset(&upload);
    assert(!Port_Viewport_UploadRequest(&upload, 1, 0, 0, 162, Port_Viewport_MapTileRows(162)));
    Port_Viewport_UploadFilled(&upload, 1, 0, 0, Port_Viewport_MapTileRows(162));
    Port_Viewport_UploadCompleted(&upload, 1, 0, 0, Port_Viewport_MapTileRows(162));
    assert(Port_Viewport_UploadRequest(&upload, 1, 0, 0, 162, Port_Viewport_MapTileRows(162)));
    assert(Port_Viewport_MapTileRows(162) == 24);
    assert(Port_Viewport_MapTileRows(192) == 27);

    puts("viewport composition: live aperture, fallback, 4:3 and 162 extent PASS");
    return 0;
}
