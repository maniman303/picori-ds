#ifndef PORT_SCREEN_BACKGROUND_H
#define PORT_SCREEN_BACKGROUND_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Semantic settings, not GBA BG planes or individual menu layouts. The game
 * state adapter must select these only after the native screen is installed.
 * In particular, a credits text command does not imply CREDITS_CLEARED: an
 * earlier illustration can still be visible beneath it. Menu/kinstone health
 * contexts describe the installed variant from sub_080A4D34 (gfx 86/87/88),
 * not current health polled during presentation. Each family has three images.
 *
 * This catalog does not read game globals, decode images, or resolve effects.
 * Native foreground content always remains a complete 240x160 composition. */
typedef enum {
    PORT_BACKGROUND_UNCLASSIFIED,
    PORT_BACKGROUND_LIVE_WORLD,
    PORT_BACKGROUND_BOOT_LOGOS,
    PORT_BACKGROUND_TITLE,
    PORT_BACKGROUND_FILE_SELECT,
    PORT_BACKGROUND_MENU_NORMAL,
    PORT_BACKGROUND_KINSTONE_NORMAL,
    PORT_BACKGROUND_PROLOGUE_01,
    PORT_BACKGROUND_PROLOGUE_02,
    PORT_BACKGROUND_PROLOGUE_03,
    PORT_BACKGROUND_PROLOGUE_04,
    PORT_BACKGROUND_PROLOGUE_05,
    PORT_BACKGROUND_PROLOGUE_06,
    PORT_BACKGROUND_CREDITS_01,
    PORT_BACKGROUND_CREDITS_02,
    PORT_BACKGROUND_CREDITS_03,
    PORT_BACKGROUND_CREDITS_04,
    PORT_BACKGROUND_CREDITS_05,
    PORT_BACKGROUND_CREDITS_06,
    PORT_BACKGROUND_CREDITS_07,
    PORT_BACKGROUND_CREDITS_08,
    PORT_BACKGROUND_ENDING,
    PORT_BACKGROUND_GAME_OVER,
    PORT_BACKGROUND_CREDITS_CLEARED,
    PORT_BACKGROUND_ENDING_SAVE,
    PORT_BACKGROUND_FIGURINES,
    PORT_BACKGROUND_MAP_HINT,
    PORT_BACKGROUND_MENU_DAMAGED,
    PORT_BACKGROUND_MENU_CRITICAL,
    PORT_BACKGROUND_KINSTONE_DAMAGED,
    PORT_BACKGROUND_KINSTONE_CRITICAL,
    PORT_BACKGROUND_COUNT
} PortBackgroundContext;

typedef enum {
    PORT_BACKGROUND_SOURCE_NONE,
    PORT_BACKGROUND_SOURCE_IMAGE,
    PORT_BACKGROUND_SOURCE_BLACK,
    /* The white native logo field, with its native layer/effect target.
     * This is not an instruction to set the GBA backdrop color to white. */
    PORT_BACKGROUND_SOURCE_LOGO_FIELD,
    /* Resolve from the active palette-zero field, including palette fades.
     * Figurines copy entry 251 to zero; map hint copies entry 81 to zero.
     * Do not use the unfaded EU reference swatch as a runtime RGB constant. */
    PORT_BACKGROUND_SOURCE_PALETTE_ZERO
} PortBackgroundSource;

typedef struct {
    PortBackgroundSource source;
    /* A basename only for IMAGE; NULL otherwise, so solids/world/parity can
     * never cause file lookup. Missing/invalid expected images use black. */
    const char* filename;
} PortScreenBackground;

/* Immutable, process-lifetime descriptors. Unknown contexts and Console-Parity
 * return NONE. Effects belong to the active screen state, not the image cache;
 * resolve them separately before composing any margins. */
const PortScreenBackground* Port_Background_Get(PortBackgroundContext context, bool consoleParity);

#ifdef __cplusplus
}
#endif

#endif
