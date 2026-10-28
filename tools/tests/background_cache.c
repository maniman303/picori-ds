#include "port_background_cache.h"
#include "port_title_cap.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static PortBackgroundCache cache;
static uint32_t canvas[374 * 160];
_Static_assert(sizeof(cache) < 8u * 1024 * 1024, "cache exceeds steady budget");

static const uint8_t* current(int red) {
    int width = -1, height = -1;
    const uint8_t* pixels = Port_BackgroundCache_Current(&cache, &width, &height);
    assert(pixels && width == 285 && height == 160 && pixels[0] == red);
    return pixels;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    char plain[4096], saved[4096], missing[4096], fixture[4096], travel[4096], edited[4096], corrupt[4096];
    assert(snprintf(plain, sizeof(plain), "%s/override/169/menu_background_normal.png", argv[1]) < (int)sizeof(plain));
    assert(snprintf(saved, sizeof(saved), "%s/saved.png", argv[1]) < (int)sizeof(saved));
    assert(snprintf(missing, sizeof(missing), "%s/override/169/menu_background_damaged.png", argv[1]) < (int)sizeof(missing));
    assert(snprintf(fixture, sizeof(fixture), "%s/fixture.png", argv[1]) < (int)sizeof(fixture));
    assert(snprintf(travel, sizeof(travel), "%s/override/169/fast_travel_background.png", argv[1]) < (int)sizeof(travel));
    assert(snprintf(edited, sizeof(edited), "%s/edited.png", argv[1]) < (int)sizeof(edited));
    assert(snprintf(corrupt, sizeof(corrupt), "%s/corrupt.png", argv[1]) < (int)sizeof(corrupt));
    assert(Port_BackgroundCache_Init(&cache, argv[1]));
    static const char* families[] = {"menu_background", "menu_titled_background", "kinstone_pieces_background",
                                     "sword_techniques_background", "kinstone_background"};
    for (int family = 0; family < 5; ++family) {
        for (int health = 0; health < 3; ++health) {
            assert(Port_BackgroundCache_StageMenu(&cache, (PortBackgroundMenuFamily)family,
                                                  (PortBackgroundHealth)health,
                                                  PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_READY);
            Port_BackgroundCache_Commit(&cache);
            current(19 + family * 14 + health * 3);
        }
    }
    /* With the old plain image still present, none of the other families may
     * alias it. Commit the failure and exercise the actual black-margin path. */
    for (int family = 1; family < 5; ++family) {
        char path[4096], temp[4096];
        assert(snprintf(path, sizeof(path), "%s/override/169/%s_normal.png", argv[1], families[family]) < (int)sizeof(path));
        assert(snprintf(temp, sizeof(temp), "%s/hidden.png", argv[1]) < (int)sizeof(temp));
        assert(rename(path, temp) == 0);
        Port_BackgroundCache_Invalidate(&cache);
        assert(Port_BackgroundCache_StageMenu(&cache, (PortBackgroundMenuFamily)family,
                                              PORT_BACKGROUND_HEALTH_NORMAL,
                                              PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_FAILED);
        Port_BackgroundCache_Commit(&cache);
        assert(!Port_BackgroundCache_Current(&cache, NULL, NULL));
        for (int i = 0; i < 374 * 160; ++i) canvas[i] = 0xffabcdef;
        assert(Port_TitleCap_ExpandFrame(canvas, 285, 160, 374, 22, 0, NULL, 0, 0xff000000));
        assert(canvas[0] == 0xff000000 && canvas[21] == 0xff000000 && canvas[22] == 0xffabcdef);
        assert(rename(temp, path) == 0);
    }
    Port_BackgroundCache_Invalidate(&cache);
    assert(Port_BackgroundCache_StageMenu(&cache, PORT_BACKGROUND_MENU_PLAIN, PORT_BACKGROUND_HEALTH_NORMAL,
                                          PORT_ASPECT_ULTRAWIDE_21_9, 1, false) == PORT_BACKGROUND_CACHE_READY);
    Port_BackgroundCache_Commit(&cache);
    int wide_width, wide_height;
    const uint8_t* wide = Port_BackgroundCache_Current(&cache, &wide_width, &wide_height);
    assert(wide && wide_width == 374 && wide_height == 160 && wide[0] == 76);
    assert(Port_BackgroundCache_StageMenu(&cache, PORT_BACKGROUND_MENU_PLAIN, PORT_BACKGROUND_HEALTH_NORMAL,
                                          PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_READY);
    Port_BackgroundCache_Commit(&cache);
    current(19);
    Port_BackgroundCache_Invalidate(&cache);
    assert(remove(missing) == 0);
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_MENU_NORMAL, PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_READY);
    assert(!Port_BackgroundCache_Current(&cache, NULL, NULL));
    Port_BackgroundCache_Commit(&cache);
    const uint8_t* first = current(19);
    assert(rename(plain, saved) == 0);
    /* Repeated presentation and staging of the same key cannot reopen it. */
    for (int i = 0; i < 1000; ++i) assert(current(19) == first);
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_MENU_NORMAL, PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_READY);
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_MENU_TITLED_NORMAL, PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_READY);
    assert(current(19) == first); /* staging cannot overwrite the current slot */
    Port_BackgroundCache_Commit(&cache);
    current(33);
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_MENU_NORMAL, PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_READY);
    Port_BackgroundCache_Commit(&cache);
    current(19); /* outgoing slot remains cached */
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_MENU_DAMAGED, PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_FAILED);
    assert(current(19) == first);
    Port_BackgroundCache_Commit(&cache);
    assert(!Port_BackgroundCache_Current(&cache, NULL, NULL));
    assert(rename(fixture, missing) == 0);
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_MENU_DAMAGED, PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_FAILED);
    Port_BackgroundCache_Commit(&cache);
    assert(!Port_BackgroundCache_Current(&cache, NULL, NULL));
    assert(Port_BackgroundCache_Reload(&cache) == PORT_BACKGROUND_CACHE_READY);
    current(44);
    /* A new generation cannot reuse an old success after its file disappears. */
    assert(rename(missing, fixture) == 0);
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_MENU_DAMAGED, PORT_ASPECT_WIDESCREEN_16_9, 2, false) == PORT_BACKGROUND_CACHE_FAILED);
    Port_BackgroundCache_Commit(&cache);
    assert(!Port_BackgroundCache_Current(&cache, NULL, NULL));
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_MENU_NORMAL, PORT_ASPECT_WIDESCREEN_16_9, 1, true) == PORT_BACKGROUND_CACHE_EMPTY);
    Port_BackgroundCache_Commit(&cache);
    assert(!Port_BackgroundCache_Current(&cache, NULL, NULL));
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_FAST_TRAVEL, PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_READY);
    Port_BackgroundCache_Commit(&cache);
    current(55);
    assert(remove(fixture) == 0);
    assert(rename(travel, fixture) == 0);
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_FAST_TRAVEL, PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_READY);
    Port_BackgroundCache_Commit(&cache);
    current(55);
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_FAST_TRAVEL, PORT_ASPECT_ULTRAWIDE_21_9, 1, false) == PORT_BACKGROUND_CACHE_FAILED);
    Port_BackgroundCache_Commit(&cache);
    assert(!Port_BackgroundCache_Current(&cache, NULL, NULL));
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_LIVE_WORLD, PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_EMPTY);
    Port_BackgroundCache_Commit(&cache);
    assert(!Port_BackgroundCache_Current(&cache, NULL, NULL));
    assert(Port_BackgroundCache_Reload(&cache) == PORT_BACKGROUND_CACHE_EMPTY);
    assert(rename(saved, plain) == 0);
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_MENU_NORMAL, PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_READY);
    Port_BackgroundCache_Commit(&cache);
    current(19);
    assert(rename(plain, saved) == 0 && rename(edited, plain) == 0);
    assert(Port_BackgroundCache_Reload(&cache) == PORT_BACKGROUND_CACHE_READY);
    const uint8_t* reloaded = current(88);
    assert(reloaded[1] == 17 && reloaded[2] == 23 && reloaded[3] == 128);
    assert(remove(plain) == 0 && rename(corrupt, plain) == 0);
    assert(Port_BackgroundCache_Reload(&cache) == PORT_BACKGROUND_CACHE_FAILED);
    assert(!Port_BackgroundCache_Current(&cache, NULL, NULL));
    puts("Background cache staging, failures, invalidation and no-I/O reads: PASS");
    return 0;
}
