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
}

int main(void) {
    CheckProfiles();
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
    assert(images == 19);
    assert(strcmp(Port_Background_Get(PORT_BACKGROUND_TITLE, false)->filename, "title_screen.png") == 0);
    assert(strcmp(Port_Background_Get(PORT_BACKGROUND_FILE_SELECT, false)->filename, "file_select.png") == 0);
    assert(strcmp(Port_Background_Get(PORT_BACKGROUND_MENU, false)->filename, "menu_background.png") == 0);
    assert(strcmp(Port_Background_Get(PORT_BACKGROUND_KINSTONE, false)->filename, "kinstone_background.png") == 0);
    return 0;
}
