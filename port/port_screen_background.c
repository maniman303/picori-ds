#include "port_screen_background.h"

#include <stddef.h>

/* Shared names are the public artist contract in export_screen_backgrounds.py.
 * Plain prompts, titled menus, pieces, techniques and fusion are distinct
 * families, each with three installed health decorations. Local hints replace
 * that decoration with a solid field. Fusion retains its seven shared layouts.
 * Original frames, maps, widgets and text are foreground. */
static const PortScreenBackground sBackgrounds[PORT_BACKGROUND_COUNT] = {
    [PORT_BACKGROUND_UNCLASSIFIED] = { PORT_BACKGROUND_SOURCE_NONE, NULL },
    [PORT_BACKGROUND_LIVE_WORLD] = { PORT_BACKGROUND_SOURCE_NONE, NULL },
    [PORT_BACKGROUND_BOOT_LOGOS] = { PORT_BACKGROUND_SOURCE_LOGO_FIELD, NULL },
    [PORT_BACKGROUND_TITLE] = { PORT_BACKGROUND_SOURCE_IMAGE, "title_screen.png" },
    [PORT_BACKGROUND_FAST_TRAVEL] = { PORT_BACKGROUND_SOURCE_IMAGE, "fast_travel_background.png" },
    [PORT_BACKGROUND_FILE_SELECT] = { PORT_BACKGROUND_SOURCE_IMAGE, "file_select.png" },
    [PORT_BACKGROUND_MENU_NORMAL] = { PORT_BACKGROUND_SOURCE_IMAGE, "menu_background_normal.png" },
    [PORT_BACKGROUND_MENU_DAMAGED] = { PORT_BACKGROUND_SOURCE_IMAGE, "menu_background_damaged.png" },
    [PORT_BACKGROUND_MENU_CRITICAL] = { PORT_BACKGROUND_SOURCE_IMAGE, "menu_background_critical.png" },
    [PORT_BACKGROUND_KINSTONE_NORMAL] = { PORT_BACKGROUND_SOURCE_IMAGE, "kinstone_background_normal.png" },
    [PORT_BACKGROUND_KINSTONE_DAMAGED] = { PORT_BACKGROUND_SOURCE_IMAGE, "kinstone_background_damaged.png" },
    [PORT_BACKGROUND_KINSTONE_CRITICAL] = { PORT_BACKGROUND_SOURCE_IMAGE, "kinstone_background_critical.png" },
    [PORT_BACKGROUND_PROLOGUE_01] = { PORT_BACKGROUND_SOURCE_IMAGE, "prologue_01.png" },
    [PORT_BACKGROUND_PROLOGUE_02] = { PORT_BACKGROUND_SOURCE_IMAGE, "prologue_02.png" },
    [PORT_BACKGROUND_PROLOGUE_03] = { PORT_BACKGROUND_SOURCE_IMAGE, "prologue_03.png" },
    [PORT_BACKGROUND_PROLOGUE_04] = { PORT_BACKGROUND_SOURCE_IMAGE, "prologue_04.png" },
    [PORT_BACKGROUND_PROLOGUE_05] = { PORT_BACKGROUND_SOURCE_IMAGE, "prologue_05.png" },
    [PORT_BACKGROUND_PROLOGUE_06] = { PORT_BACKGROUND_SOURCE_IMAGE, "prologue_06.png" },
    [PORT_BACKGROUND_CREDITS_01] = { PORT_BACKGROUND_SOURCE_IMAGE, "credits_01.png" },
    [PORT_BACKGROUND_CREDITS_02] = { PORT_BACKGROUND_SOURCE_IMAGE, "credits_02.png" },
    [PORT_BACKGROUND_CREDITS_03] = { PORT_BACKGROUND_SOURCE_IMAGE, "credits_03.png" },
    [PORT_BACKGROUND_CREDITS_04] = { PORT_BACKGROUND_SOURCE_IMAGE, "credits_04.png" },
    [PORT_BACKGROUND_CREDITS_05] = { PORT_BACKGROUND_SOURCE_IMAGE, "credits_05.png" },
    [PORT_BACKGROUND_CREDITS_06] = { PORT_BACKGROUND_SOURCE_IMAGE, "credits_06.png" },
    [PORT_BACKGROUND_CREDITS_07] = { PORT_BACKGROUND_SOURCE_IMAGE, "credits_07.png" },
    [PORT_BACKGROUND_CREDITS_08] = { PORT_BACKGROUND_SOURCE_IMAGE, "credits_08.png" },
    [PORT_BACKGROUND_ENDING] = { PORT_BACKGROUND_SOURCE_IMAGE, "ending_screen.png" },
    [PORT_BACKGROUND_GAME_OVER] = { PORT_BACKGROUND_SOURCE_BLACK, NULL },
    [PORT_BACKGROUND_CREDITS_CLEARED] = { PORT_BACKGROUND_SOURCE_BLACK, NULL },
    [PORT_BACKGROUND_ENDING_SAVE] = { PORT_BACKGROUND_SOURCE_BLACK, NULL },
    [PORT_BACKGROUND_FIGURINES] = { PORT_BACKGROUND_SOURCE_PALETTE_ZERO, NULL },
    [PORT_BACKGROUND_MAP_HINT] = { PORT_BACKGROUND_SOURCE_PALETTE_ZERO, NULL },
    [PORT_BACKGROUND_LOCAL_MAP_HINT] = { PORT_BACKGROUND_SOURCE_BG3_PALETTE_95, NULL },
    [PORT_BACKGROUND_MENU_TITLED_NORMAL] = { PORT_BACKGROUND_SOURCE_IMAGE, "menu_titled_background_normal.png" },
    [PORT_BACKGROUND_MENU_TITLED_DAMAGED] = { PORT_BACKGROUND_SOURCE_IMAGE, "menu_titled_background_damaged.png" },
    [PORT_BACKGROUND_MENU_TITLED_CRITICAL] = { PORT_BACKGROUND_SOURCE_IMAGE, "menu_titled_background_critical.png" },
    [PORT_BACKGROUND_KINSTONE_PIECES_NORMAL] = { PORT_BACKGROUND_SOURCE_IMAGE, "kinstone_pieces_background_normal.png" },
    [PORT_BACKGROUND_KINSTONE_PIECES_DAMAGED] = { PORT_BACKGROUND_SOURCE_IMAGE, "kinstone_pieces_background_damaged.png" },
    [PORT_BACKGROUND_KINSTONE_PIECES_CRITICAL] = { PORT_BACKGROUND_SOURCE_IMAGE, "kinstone_pieces_background_critical.png" },
    [PORT_BACKGROUND_SWORD_TECHNIQUES_NORMAL] = { PORT_BACKGROUND_SOURCE_IMAGE, "sword_techniques_background_normal.png" },
    [PORT_BACKGROUND_SWORD_TECHNIQUES_DAMAGED] = { PORT_BACKGROUND_SOURCE_IMAGE, "sword_techniques_background_damaged.png" },
    [PORT_BACKGROUND_SWORD_TECHNIQUES_CRITICAL] = { PORT_BACKGROUND_SOURCE_IMAGE, "sword_techniques_background_critical.png" },
};

PortBackgroundContext Port_Background_MenuContext(PortBackgroundMenuFamily family, PortBackgroundHealth installedHealth) {
    static const PortBackgroundContext contexts[PORT_BACKGROUND_MENU_FAMILY_COUNT][3] = {
        { PORT_BACKGROUND_MENU_NORMAL, PORT_BACKGROUND_MENU_DAMAGED, PORT_BACKGROUND_MENU_CRITICAL },
        { PORT_BACKGROUND_MENU_TITLED_NORMAL, PORT_BACKGROUND_MENU_TITLED_DAMAGED, PORT_BACKGROUND_MENU_TITLED_CRITICAL },
        { PORT_BACKGROUND_KINSTONE_PIECES_NORMAL, PORT_BACKGROUND_KINSTONE_PIECES_DAMAGED, PORT_BACKGROUND_KINSTONE_PIECES_CRITICAL },
        { PORT_BACKGROUND_SWORD_TECHNIQUES_NORMAL, PORT_BACKGROUND_SWORD_TECHNIQUES_DAMAGED, PORT_BACKGROUND_SWORD_TECHNIQUES_CRITICAL },
        { PORT_BACKGROUND_KINSTONE_NORMAL, PORT_BACKGROUND_KINSTONE_DAMAGED, PORT_BACKGROUND_KINSTONE_CRITICAL },
    };
    if ((unsigned)family >= PORT_BACKGROUND_MENU_FAMILY_COUNT || (unsigned)installedHealth > PORT_BACKGROUND_HEALTH_CRITICAL)
        return PORT_BACKGROUND_UNCLASSIFIED;
    return contexts[family][installedHealth];
}

const PortScreenBackground* Port_Background_Get(PortBackgroundContext context, bool consoleParity) {
    if (consoleParity || (unsigned)context >= PORT_BACKGROUND_COUNT)
        context = PORT_BACKGROUND_UNCLASSIFIED;
    return &sBackgrounds[context];
}

static PortBackgroundContext sInstalled, sPublished;
static PortBackgroundScope sInstalledScope;
static PortBackgroundHealth sHealth;
static bool sHealthValid;

void Port_Background_StageEvent(PortBackgroundEvent event, unsigned value) {
    PortBackgroundContext context = PORT_BACKGROUND_UNCLASSIFIED;
    PortBackgroundScope scope = PORT_BG_SCOPE_UNKNOWN;
    if (event == PORT_BG_EVENT_HEALTH) {
        sHealthValid = value <= PORT_BACKGROUND_HEALTH_CRITICAL;
        if (sHealthValid) sHealth = (PortBackgroundHealth)value;
        return;
    }
    switch (event) {
        case PORT_BG_EVENT_PAUSE: {
            PortBackgroundMenuFamily family;
            if (value == 7) family = PORT_BACKGROUND_MENU_PIECES;
            else if (value == 8) family = PORT_BACKGROUND_MENU_TECHNIQUES;
            else if (value >= 9 && value <= 11) family = PORT_BACKGROUND_MENU_PLAIN;
            else if (value == 1 || value == 2 || (value >= 4 && value <= 6)) family = PORT_BACKGROUND_MENU_TITLED;
            else break;
            if (sHealthValid) context = Port_Background_MenuContext(family, sHealth);
            scope = PORT_BG_SCOPE_PAUSE;
            break;
        }
        case PORT_BG_EVENT_FUSION:
            if (sHealthValid) context = Port_Background_MenuContext(PORT_BACKGROUND_MENU_FUSION, sHealth);
            scope = PORT_BG_SCOPE_FUSION; break;
        case PORT_BG_EVENT_TRAVEL: context = PORT_BACKGROUND_FAST_TRAVEL; scope = PORT_BG_SCOPE_TRAVEL; break;
        case PORT_BG_EVENT_FIGURINE: context = PORT_BACKGROUND_FIGURINES; scope = PORT_BG_SCOPE_FIGURINE; break;
        case PORT_BG_EVENT_HINT: context = PORT_BACKGROUND_MAP_HINT; scope = PORT_BG_SCOPE_HINT; break;
        case PORT_BG_EVENT_LOCAL_HINT: context = PORT_BACKGROUND_LOCAL_MAP_HINT; scope = PORT_BG_SCOPE_LOCAL_HINT; break;
        case PORT_BG_EVENT_PROLOGUE:
            if (value < 6) context = (PortBackgroundContext)(PORT_BACKGROUND_PROLOGUE_01 + value);
            scope = PORT_BG_SCOPE_STORY; break;
        case PORT_BG_EVENT_CREDITS:
            if (value < 8) context = (PortBackgroundContext)(PORT_BACKGROUND_CREDITS_01 + value);
            else if (value == 8) context = PORT_BACKGROUND_ENDING;
            scope = PORT_BG_SCOPE_CREDITS; break;
        case PORT_BG_EVENT_CREDITS_CLEAR: context = PORT_BACKGROUND_CREDITS_CLEARED; scope = PORT_BG_SCOPE_CREDITS; break;
        case PORT_BG_EVENT_ENDING_SAVE: context = PORT_BACKGROUND_ENDING_SAVE; scope = PORT_BG_SCOPE_CREDITS; break;
        case PORT_BG_EVENT_LOGOS: context = PORT_BACKGROUND_BOOT_LOGOS; scope = PORT_BG_SCOPE_STORY; break;
        case PORT_BG_EVENT_TITLE: context = PORT_BACKGROUND_TITLE; scope = PORT_BG_SCOPE_STORY; break;
        case PORT_BG_EVENT_FILE: context = PORT_BACKGROUND_FILE_SELECT; scope = PORT_BG_SCOPE_FILE; break;
        case PORT_BG_EVENT_GAME_OVER: context = PORT_BACKGROUND_GAME_OVER; scope = PORT_BG_SCOPE_GAME_OVER; break;
        case PORT_BG_EVENT_CLEAR: sHealthValid = false; break;
        default: return;
    }
    sInstalled = context;
    sInstalledScope = scope;
}

void Port_Background_Publish(PortBackgroundScope scope) {
    if (scope == PORT_BG_SCOPE_WORLD) {
        sInstalled = PORT_BACKGROUND_UNCLASSIFIED;
        sInstalledScope = PORT_BG_SCOPE_UNKNOWN;
        sHealthValid = false;
        sPublished = PORT_BACKGROUND_LIVE_WORLD;
    } else {
        sPublished = scope != PORT_BG_SCOPE_UNKNOWN && scope == sInstalledScope
                         ? sInstalled : PORT_BACKGROUND_UNCLASSIFIED;
        if (scope != sInstalledScope) {
            sInstalled = PORT_BACKGROUND_UNCLASSIFIED;
            sInstalledScope = PORT_BG_SCOPE_UNKNOWN;
        }
    }
}

PortBackgroundContext Port_Background_Published(void) { return sPublished; }

void Port_Background_Reset(void) {
    sInstalled = sPublished = PORT_BACKGROUND_UNCLASSIFIED;
    sInstalledScope = PORT_BG_SCOPE_UNKNOWN;
    sHealthValid = false;
}
