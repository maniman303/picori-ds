/* ROM-free contracts for screen sources and logical profile geometry. */
#include "port_screen_background.h"
#include "port_viewport.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void CheckSource(PortBackgroundContext context, PortBackgroundSource source) {
    const PortScreenBackground* background = Port_Background_Get(context, false);
    assert(background->source == source);
    assert(background->filename == NULL);
}

static void CheckProfiles(void) {
    static const struct {
        const char* directory;
        const char* aspect;
        int width, height, x, y;
    } cases[] = {
        { "32", "3:2", 240, 160, 0, 0 },
        { "43", "4:3", 256, 192, 8, 16 },
        { "169", "16:9", 285, 160, 22, 0 },
        { "219", "21:9", 374, 160, 67, 0 },
        { "329", "32:9", 569, 160, 164, 0 },
    };
    assert(PORT_VIEWPORT_PROFILE_VERSION == 1);
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const PortViewportProfile* profile = Port_Viewport_FindProfile(cases[i].directory);
        assert(profile != NULL);
        assert(strcmp(profile->aspect, cases[i].aspect) == 0);
        assert(profile->width == cases[i].width && profile->height == cases[i].height);
        if (strcmp(profile->directory, "43") != 0) {
            unsigned numerator = 0, denominator = 0;
            assert(sscanf(profile->aspect, "%u:%u", &numerator, &denominator) == 2);
            assert(denominator != 0);
            assert(profile->height == 160);
            assert((unsigned)profile->width == (160 * numerator + denominator - 1) / denominator);
        }
        PortViewportRect rect = Port_Viewport_NativeContentRect(profile);
        assert(rect.x == cases[i].x && rect.y == cases[i].y);
        assert(rect.width == 240 && rect.height == 160);
        PortUITranslation tl = Port_Viewport_UITranslation(PORT_UI_TOP_LEFT, profile->width, profile->height);
        PortUITranslation tr = Port_Viewport_UITranslation(PORT_UI_TOP_RIGHT, profile->width, profile->height);
        PortUITranslation bl = Port_Viewport_UITranslation(PORT_UI_BOTTOM_LEFT, profile->width, profile->height);
        PortUITranslation br = Port_Viewport_UITranslation(PORT_UI_BOTTOM_RIGHT, profile->width, profile->height);
        PortUITranslation tc = Port_Viewport_UITranslation(PORT_UI_TOP_CENTER, profile->width, profile->height);
        assert(tl.x == 0 && tl.y == 0);
        assert(tr.x == profile->width - 240 && tr.y == 0);
        assert(bl.x == 0 && bl.y == profile->height - 160);
        assert(br.x == tr.x && br.y == bl.y);
        assert(tc.x == rect.x && tc.y == 0);
        assert(rect.x + rect.width <= profile->width && rect.y + rect.height <= profile->height);
        assert(Port_Viewport_ProfileFits(profile, profile->width, profile->height));
        assert(!Port_Viewport_ProfileFits(profile, profile->width - 1, profile->height));
        assert(!Port_Viewport_ProfileFits(profile, profile->width, profile->height - 1));
        assert(!Port_Viewport_ProfileFits(profile, -1, -1));
    }
    /* Capacity alone must not turn a DS frame into 256x160 or truncate 32:9. */
    assert(!Port_Viewport_ProfileFits(Port_Viewport_FindProfile("43"), 384, 160));
    assert(!Port_Viewport_ProfileFits(Port_Viewport_FindProfile("329"), 384, 192));
    assert(Port_Viewport_ProfileFits(Port_Viewport_FindProfile("32"), 240, 160));
    assert(Port_Viewport_FindProfile(NULL) == NULL);
    assert(Port_Viewport_FindProfile("") == NULL);
    assert(Port_Viewport_FindProfile("4:3") == NULL);
    assert(Port_Viewport_FindProfile("../43") == NULL);
    assert(!Port_Viewport_ProfileFits(NULL, 569, 192));
    PortViewportRect native = Port_Viewport_NativeContentRect(NULL);
    assert(native.x == 0 && native.y == 0 && native.width == 240 && native.height == 160);
    PortUITranslation tallTopRight = Port_Viewport_UITranslation(PORT_UI_TOP_RIGHT, 256, 192);
    PortUITranslation tallBottomRight = Port_Viewport_UITranslation(PORT_UI_BOTTOM_RIGHT, 256, 192);
    PortUITranslation tallBottomLeft = Port_Viewport_UITranslation(PORT_UI_BOTTOM_LEFT, 256, 192);
    PortUITranslation tallTopCenter = Port_Viewport_UITranslation(PORT_UI_TOP_CENTER, 256, 192);
    assert(tallTopRight.x == 16 && tallTopRight.y == 0);
    assert(tallBottomRight.x == 16 && tallBottomRight.y == 32);
    assert(tallBottomLeft.x == 0 && tallBottomLeft.y == 32);
    assert(tallTopCenter.x == 8 && tallTopCenter.y == 0);
    PortUITranslation invalid = Port_Viewport_UITranslation(PORT_UI_BOTTOM_RIGHT, 239, 192);
    assert(invalid.x == 0 && invalid.y == 0);
    invalid = Port_Viewport_UITranslation((PortUIAnchor)-1, 256, 192);
    assert(invalid.x == 0 && invalid.y == 0);
}

static void CheckStandardMessageGeometry(void) {
    const PortViewportRect top = { 8, 8, 224, 48 };
    const PortViewportRect bottom = { 8, 96, 224, 48 };
    PortUITranslation t = { -1, -1 };
    assert(Port_Viewport_StandardMessageTranslation(top, 256, 192, &t));
    assert(t.x == 8 && t.y == 0); /* top stays at the native top */
    /* Every animation frame uses this same offset, including a smaller
     * rectangle centered inside the fully open native window. */
    assert(24 + t.y == 24 && 16 + t.y == 16);
    assert(Port_Viewport_StandardMessageTranslation(bottom, 256, 192, &t));
    assert(t.x == 8 && t.y == 32); /* bottom follows the canvas bottom */
    assert(Port_Viewport_StandardMessageTranslation(top, 285, 160, &t));
    assert(t.x == 22 && t.y == 0);
    assert(Port_Viewport_StandardMessageTranslation(bottom, 374, 160, &t));
    assert(t.x == 67 && t.y == 0);
    /* Generic height rule: top always 0, bottom always height - 160. */
    assert(Port_Viewport_StandardMessageTranslation(top, 288, 162, &t));
    assert(t.x == 24 && t.y == 0);
    assert(Port_Viewport_StandardMessageTranslation(bottom, 288, 162, &t));
    assert(t.x == 24 && t.y == 2);
    PortViewportRect custom = top;
    custom.y = 16;
    t.x = 7; t.y = 9;
    assert(!Port_Viewport_StandardMessageTranslation(custom, 256, 192, &t));
    assert(t.x == 7 && t.y == 9);
    assert(!Port_Viewport_StandardMessageTranslation(top, 239, 192, &t));
    assert(!Port_Viewport_StandardMessageTranslation(top, 256, 193, &t));
    assert(!Port_Viewport_StandardMessageTranslation(top, 256, 192, NULL));
}

static void CheckPublication(void) {
    /* Existing enum values are persisted; the DS profile is appended. */
    assert(PORT_ASPECT_NATIVE_3_2 == 0 && PORT_ASPECT_WIDESCREEN_16_9 == 1);
    assert(PORT_ASPECT_ULTRAWIDE_21_9 == 2 && PORT_ASPECT_SUPER_ULTRAWIDE_32_9 == 3);
    assert(PORT_ASPECT_DS_4_3 == 4);
    assert(Port_Viewport_ProfileForAspect((PortAspectMode)-1) == NULL);
    assert(Port_Viewport_ProfileForAspect(PORT_ASPECT_DS_4_3) == Port_Viewport_FindProfile("43"));
    assert(!Port_Viewport_AspectSupported(PORT_ASPECT_WIDESCREEN_16_9, 240, 160));
    assert(Port_Viewport_AspectSupported(PORT_ASPECT_ULTRAWIDE_21_9, 384, 160));
#if PORT_VIEWPORT_RELEASE_TALL
    /* Tall canvases are a release capability; capacity alone still gates. */
    assert(Port_Viewport_AspectSupported(PORT_ASPECT_DS_4_3, 569, 192));
    assert(!Port_Viewport_AspectSupported(PORT_ASPECT_DS_4_3, 384, 160));
#else
    assert(!Port_Viewport_AspectSupported(PORT_ASPECT_DS_4_3, 569, 192));
#endif
    assert(!Port_Viewport_AspectSupported(PORT_ASPECT_SUPER_ULTRAWIDE_32_9, 569, 192));

    const PortViewportGeometry* geometry = Port_Viewport_Get();
    assert(geometry->canvas.width == 240 && geometry->generation == 0);
    assert(Port_Viewport_Publish(PORT_ASPECT_WIDESCREEN_16_9, false, 384, 160, 384));
    assert(geometry == Port_Viewport_Get());
    assert(geometry->selectedAspect == PORT_ASPECT_WIDESCREEN_16_9);
    assert(geometry->effectiveAspect == PORT_ASPECT_WIDESCREEN_16_9);
    assert(geometry->canvas.width == 285 && geometry->canvas.height == 160);
    assert(geometry->nativeContent.x == 22 && geometry->nativeContent.width == 240);
    assert(geometry->uiSafe.width == 285);
    assert(geometry->pitchPixels == 384 && geometry->capacityWidth == 384);
    uint32_t generation = geometry->generation;
    assert(Port_Viewport_Publish(PORT_ASPECT_WIDESCREEN_16_9, false, 384, 160, 384));
    assert(geometry->generation == generation);
    assert(!Port_Viewport_Publish(PORT_ASPECT_NATIVE_3_2, false, 384, 160, 240));
    assert(!Port_Viewport_Publish(PORT_ASPECT_NATIVE_3_2, false, 239, 160, 384));
    assert(!Port_Viewport_Publish(PORT_ASPECT_NATIVE_3_2, false, 384, 159, 384));
    assert(geometry->generation == generation && geometry->canvas.width == 285);

    assert(Port_Viewport_Publish(PORT_ASPECT_WIDESCREEN_16_9, true, 384, 160, 384));
    assert(geometry->generation != generation);
    assert(geometry->flags == PORT_VIEWPORT_PARITY);
    assert(geometry->canvas.width == 240 && geometry->effectiveAspect == PORT_ASPECT_NATIVE_3_2);
    assert(geometry->selectedAspect == PORT_ASPECT_WIDESCREEN_16_9);
    assert(Port_Viewport_Publish(geometry->selectedAspect, false, 384, 160, 384));
    assert(geometry->canvas.width == 285 && geometry->flags == 0);
    assert(Port_Viewport_Publish(PORT_ASPECT_ULTRAWIDE_21_9, false, 384, 160, 400));
    assert(geometry->canvas.width == 374 && geometry->nativeContent.x == 67);
    assert(geometry->pitchPixels == 400);
    assert(Port_Viewport_Publish(PORT_ASPECT_DS_4_3, false, 384, 160, 384));
    assert(geometry->selectedAspect == PORT_ASPECT_DS_4_3);
    assert(geometry->canvas.width == 240 && geometry->flags == PORT_VIEWPORT_UNSUPPORTED);
    assert(Port_Viewport_Publish(PORT_ASPECT_ULTRAWIDE_21_9, false, 240, 160, 240));
    assert(geometry->selectedAspect == PORT_ASPECT_ULTRAWIDE_21_9);
    assert(geometry->canvas.width == 240 && geometry->flags == PORT_VIEWPORT_UNSUPPORTED);
    assert(Port_Viewport_Publish(PORT_ASPECT_SUPER_ULTRAWIDE_32_9, false, 569, 192, 569));
    assert(geometry->canvas.width == 240 && geometry->flags == PORT_VIEWPORT_UNSUPPORTED);
}

static void CheckFutureRoomAperture(void) {
    const PortViewportRect canvas = {0, 0, 256, 192};
    PortViewportGeometry fixed = *Port_Viewport_Get();
    fixed.canvas = canvas;
    fixed.scene.capabilities = PORT_SCENE_FIXED_SCREEN;
    assert(Port_Viewport_RasterHeight(&fixed) == 160);
    PortViewportScene s = {PORT_SCENE_WORLD | PORT_SCENE_WIDE_WORLD | PORT_SCENE_MAP_LIVE |
                           PORT_SCENE_TALL_MANAGER_FREE | PORT_SCENE_TALL_VALIDATED | PORT_SCENE_TALL_ROWS_READY |
                           PORT_SCENE_TALL_ENTITIES_EMPTY | PORT_SCENE_TALL_OAM_VERIFIED, 1024, 1024};
    PortViewportRect w = Port_Viewport_ResolveWorldRect(canvas, s);
    assert(w.width == 256 && w.height == 192 && w.x == 0 && w.y == 0);
    s.roomHeight = 176;
    w = Port_Viewport_ResolveWorldRect(canvas, s);
    assert(w.height == 176 && w.y == 8);
    s.roomHeight = 159;
    w = Port_Viewport_ResolveWorldRect(canvas, s);
    assert(w.height == 160 && w.y == 16);
    s.roomHeight = 1024;
    const unsigned gates[] = {PORT_SCENE_TALL_MANAGER_FREE, PORT_SCENE_TALL_VALIDATED, PORT_SCENE_TALL_ROWS_READY, PORT_SCENE_TALL_ENTITIES_EMPTY, PORT_SCENE_TALL_OAM_VERIFIED, PORT_SCENE_MAP_LIVE};
    for (unsigned i = 0; i < sizeof(gates) / sizeof(gates[0]); ++i) {
        s.capabilities &= ~gates[i];
        w = Port_Viewport_ResolveWorldRect(canvas, s);
        assert(w.height == 160 && w.y == 16);
        s.capabilities |= gates[i];
    }
    const unsigned blockers[] = {PORT_SCENE_TALL_SHAKE_UNSAFE, PORT_SCENE_NATIVE_TRANSITION,
                                 PORT_SCENE_NATIVE_CONTENT};
    for (unsigned i = 0; i < sizeof(blockers) / sizeof(blockers[0]); ++i) {
        s.capabilities |= blockers[i];
        w = Port_Viewport_ResolveWorldRect(canvas, s);
        assert(w.height == 160 && w.y == 16);
        s.capabilities &= ~blockers[i];
    }
    s.roomWidth = 239;
    w = Port_Viewport_ResolveWorldRect(canvas, s);
    assert(w.height == 160 && w.width == 240);
    PortViewportUpload upload = {0, 0, 0, 0, 0, false, false, false};
    assert(!Port_Viewport_UploadRequest(&upload, 1, 2, 3, 192, 23));
    assert(!upload.requested);
    assert(!Port_Viewport_UploadRequest(&upload, 1, 2, 3, 192, 28));
    Port_Viewport_UploadCompleted(&upload, 1, 2, 3, 28); /* no fill yet */
    assert(!upload.uploaded);
    Port_Viewport_UploadFilled(&upload, 1, 2, 3, 28);
    Port_Viewport_UploadCompleted(&upload, 1, 2, 3, 28);
    assert(Port_Viewport_UploadRequest(&upload, 1, 2, 3, 192, 28));
    assert(!Port_Viewport_UploadRequest(&upload, 1, 2, 4, 192, 28));
    Port_Viewport_UploadFilled(&upload, 1, 2, 4, 28);
    Port_Viewport_UploadCompleted(&upload, 1, 2, 5, 28); /* camera changed */
    assert(!upload.uploaded);
    assert(!Port_Viewport_UploadRequest(&upload, 2, 2, 4, 192, 28)); /* room changed */
    Port_Viewport_UploadFilled(&upload, 2, 2, 4, 28);
    Port_Viewport_UploadCompleted(&upload, 2, 2, 4, 28);
    assert(!Port_Viewport_UploadRequest(&upload, 2, 2, 4, 192, 29)); /* shake grew */
    Port_Viewport_UploadReset(&upload);
    assert(!upload.requested && !upload.uploaded);
    PortViewportRect bad = {0, 0, 256, 193};
    w = Port_Viewport_ResolveWorldRect(bad, s);
    assert(w.height == 160 && w.width == 240 && w.x == 0 && w.y == 0);
}

static void CheckScenePublication(void) {
    PortViewportScene scene = { PORT_SCENE_WORLD | PORT_SCENE_WIDE_WORLD | PORT_SCENE_MAP_LIVE, 1024, 1024 };
    assert(Port_Viewport_Publish(PORT_ASPECT_WIDESCREEN_16_9, false, 384, 160, 384));
    Port_Viewport_PublishScene(scene);
    const PortViewportGeometry* geometry = Port_Viewport_Get();
    assert(geometry->world.width == 285 && geometry->world.height == 160);
    assert(geometry->world.x == 0 && geometry->canvas.width == 285);
    uint32_t generation = geometry->generation;
    Port_Viewport_PublishScene(scene);
    assert(geometry->generation == generation);
    /* Unsafe vertical shake is published without narrowing the current
     * native-height horizontal aperture or changing the selected profile. */
    scene.capabilities |= PORT_SCENE_TALL_SHAKE_UNSAFE;
    Port_Viewport_PublishScene(scene);
    assert(geometry->scene.capabilities & PORT_SCENE_TALL_SHAKE_UNSAFE);
    assert(geometry->world.width == 285 && geometry->world.height == 160);
    generation = geometry->generation;
    Port_Viewport_PublishScene(scene);
    assert(geometry->generation == generation);
    scene.capabilities &= ~PORT_SCENE_TALL_SHAKE_UNSAFE;
    scene.roomWidth = 256;
    Port_Viewport_PublishScene(scene);
    assert(geometry->world.width == 256 && geometry->world.x == 14);
    assert(geometry->canvas.width == 285 && geometry->uiSafe.width == 285);
    assert(geometry->nativeContent.x == 22 && geometry->generation != generation);
    scene.capabilities |= PORT_SCENE_NATIVE_TRANSITION;
    Port_Viewport_PublishScene(scene);
    assert(geometry->world.width == 240 && geometry->world.x == 22);
    scene.capabilities &= ~PORT_SCENE_NATIVE_TRANSITION;
    Port_Viewport_PublishScene(scene);
    assert(geometry->world.width == 256);
    assert(Port_Viewport_Publish(PORT_ASPECT_WIDESCREEN_16_9, true, 384, 160, 384));
    assert(geometry->world.width == 240 && geometry->world.x == 0);
    assert(Port_Viewport_Publish(PORT_ASPECT_ULTRAWIDE_21_9, false, 384, 160, 400));
    assert(geometry->world.width == 256 && geometry->world.x == 59);
    scene.roomWidth = -1;
    Port_Viewport_PublishScene(scene);
    assert(geometry->world.width == 240 && geometry->world.x == 67);
    scene.roomWidth = 1024;
    scene.roomHeight = 0;
    Port_Viewport_PublishScene(scene);
    assert(geometry->world.width == 240);
    scene.roomHeight = 1024;
    scene.capabilities = 0;
    Port_Viewport_PublishScene(scene);
    assert(geometry->world.width == 240 && geometry->world.height == 160);
}

int main(void) {
    CheckProfiles();
    CheckStandardMessageGeometry();
    CheckPublication();
    CheckScenePublication();
    CheckFutureRoomAperture();
    CheckSource(PORT_BACKGROUND_UNCLASSIFIED, PORT_BACKGROUND_SOURCE_NONE);
    CheckSource(PORT_BACKGROUND_LIVE_WORLD, PORT_BACKGROUND_SOURCE_NONE);
    CheckSource((PortBackgroundContext)-1, PORT_BACKGROUND_SOURCE_NONE);
    CheckSource(PORT_BACKGROUND_COUNT, PORT_BACKGROUND_SOURCE_NONE);
    CheckSource((PortBackgroundContext)9999, PORT_BACKGROUND_SOURCE_NONE);
    CheckSource(PORT_BACKGROUND_BOOT_LOGOS, PORT_BACKGROUND_SOURCE_LOGO_FIELD);
    CheckSource(PORT_BACKGROUND_GAME_OVER, PORT_BACKGROUND_SOURCE_BLACK);
    CheckSource(PORT_BACKGROUND_CREDITS_CLEARED, PORT_BACKGROUND_SOURCE_BLACK);
    CheckSource(PORT_BACKGROUND_ENDING_SAVE, PORT_BACKGROUND_SOURCE_BLACK);
    CheckSource(PORT_BACKGROUND_FIGURINES, PORT_BACKGROUND_SOURCE_PALETTE_ZERO);
    CheckSource(PORT_BACKGROUND_MAP_HINT, PORT_BACKGROUND_SOURCE_PALETTE_ZERO);
    CheckSource(PORT_BACKGROUND_LOCAL_MAP_HINT, PORT_BACKGROUND_SOURCE_BG3_PALETTE_95);
    assert(PORT_BACKGROUND_SOURCE_PALETTE_ZERO == 4 && PORT_BACKGROUND_SOURCE_BG3_PALETTE_95 == 5);

    unsigned images = 0;
    for (unsigned i = 0; i < PORT_BACKGROUND_COUNT; ++i) {
        PortBackgroundContext context = (PortBackgroundContext)i;
        const PortScreenBackground* background = Port_Background_Get(context, false);
        assert(background == Port_Background_Get(context, false));
        const PortScreenBackground* parity = Port_Background_Get(context, true);
        assert(parity->source == PORT_BACKGROUND_SOURCE_NONE && parity->filename == NULL);
        if (background->source == PORT_BACKGROUND_SOURCE_IMAGE) {
            assert(background->filename != NULL);
            /* The Python runner compares the real catalog against the exporter,
             * including duplicate names and accidental solid reference images. */
            puts(background->filename);
            ++images;
        } else {
            assert(background->filename == NULL);
        }
    }
    assert(images == 33);
    assert(PORT_BACKGROUND_SWORD_TECHNIQUES_CRITICAL == 40 && PORT_BACKGROUND_FAST_TRAVEL == 41);
    assert(strcmp(Port_Background_Get(PORT_BACKGROUND_FAST_TRAVEL, false)->filename, "fast_travel_background.png") == 0);
    assert(strcmp(Port_Background_Get(PORT_BACKGROUND_TITLE, false)->filename, "title_screen.png") == 0);
    assert(strcmp(Port_Background_Get(PORT_BACKGROUND_FILE_SELECT, false)->filename, "file_select.png") == 0);
    static const struct { PortBackgroundContext context; const char* filename; } menus[] = {
        { PORT_BACKGROUND_MENU_NORMAL, "menu_background_normal.png" },
        { PORT_BACKGROUND_MENU_DAMAGED, "menu_background_damaged.png" },
        { PORT_BACKGROUND_MENU_CRITICAL, "menu_background_critical.png" },
        { PORT_BACKGROUND_KINSTONE_NORMAL, "kinstone_background_normal.png" },
        { PORT_BACKGROUND_KINSTONE_DAMAGED, "kinstone_background_damaged.png" },
        { PORT_BACKGROUND_KINSTONE_CRITICAL, "kinstone_background_critical.png" },
    };
    for (unsigned i = 0; i < sizeof(menus) / sizeof(menus[0]); ++i)
        assert(strcmp(Port_Background_Get(menus[i].context, false)->filename, menus[i].filename) == 0);
    /* Existing IDs are unchanged; every family/installed-health pair is unique. */
    assert(PORT_BACKGROUND_LOCAL_MAP_HINT == 31 && PORT_BACKGROUND_MENU_TITLED_NORMAL == 32);
    static const char* const stems[] = { "menu_background", "menu_titled_background", "kinstone_pieces_background",
                                         "sword_techniques_background", "kinstone_background" };
    static const char* const health[] = { "normal", "damaged", "critical" };
    for (unsigned family = 0; family < PORT_BACKGROUND_MENU_FAMILY_COUNT; ++family) {
        for (unsigned state = 0; state < 3; ++state) {
            char expected[80];
            snprintf(expected, sizeof(expected), "%s_%s.png", stems[family], health[state]);
            PortBackgroundContext context = Port_Background_MenuContext((PortBackgroundMenuFamily)family, (PortBackgroundHealth)state);
            assert(strcmp(Port_Background_Get(context, false)->filename, expected) == 0);
        }
    }
    assert(Port_Background_MenuContext((PortBackgroundMenuFamily)-1, PORT_BACKGROUND_HEALTH_NORMAL) == PORT_BACKGROUND_UNCLASSIFIED);
    assert(Port_Background_MenuContext(PORT_BACKGROUND_MENU_FAMILY_COUNT, PORT_BACKGROUND_HEALTH_NORMAL) == PORT_BACKGROUND_UNCLASSIFIED);
    assert(Port_Background_MenuContext(PORT_BACKGROUND_MENU_PLAIN, (PortBackgroundHealth)-1) == PORT_BACKGROUND_UNCLASSIFIED);
    assert(Port_Background_MenuContext(PORT_BACKGROUND_MENU_PLAIN, (PortBackgroundHealth)3) == PORT_BACKGROUND_UNCLASSIFIED);
    assert(sizeof(PortMenuBackgroundDescriptor) <= 32);
    assert(PORT_BACKGROUND_HEADING_NONE == 0);
    assert(PORT_BACKGROUND_HEADING_TITLE_PLATE == 1);
    assert(PORT_BACKGROUND_HEADING_CENTERED == 2);
    return 0;
}
