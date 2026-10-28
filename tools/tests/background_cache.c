#include "port_background_cache.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static PortBackgroundCache cache;
_Static_assert(sizeof(cache) < 8u * 1024 * 1024, "cache exceeds steady budget");

static const uint8_t* current(int red) {
    int width = -1, height = -1;
    const uint8_t* pixels = Port_BackgroundCache_Current(&cache, &width, &height);
    assert(pixels && width == 285 && height == 160 && pixels[0] == red);
    return pixels;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    char plain[4096], saved[4096], missing[4096], fixture[4096], travel[4096];
    assert(snprintf(plain, sizeof(plain), "%s/override/169/menu_background_normal.png", argv[1]) < (int)sizeof(plain));
    assert(snprintf(saved, sizeof(saved), "%s/saved.png", argv[1]) < (int)sizeof(saved));
    assert(snprintf(missing, sizeof(missing), "%s/override/169/menu_background_damaged.png", argv[1]) < (int)sizeof(missing));
    assert(snprintf(fixture, sizeof(fixture), "%s/fixture.png", argv[1]) < (int)sizeof(fixture));
    assert(snprintf(travel, sizeof(travel), "%s/override/169/fast_travel_background.png", argv[1]) < (int)sizeof(travel));
    assert(Port_BackgroundCache_Init(&cache, argv[1]));
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
    Port_BackgroundCache_Invalidate(&cache);
    assert(Port_BackgroundCache_Stage(&cache, PORT_BACKGROUND_MENU_DAMAGED, PORT_ASPECT_WIDESCREEN_16_9, 1, false) == PORT_BACKGROUND_CACHE_READY);
    Port_BackgroundCache_Commit(&cache);
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
    puts("Background cache staging, failures, invalidation and no-I/O reads: PASS");
    return 0;
}
