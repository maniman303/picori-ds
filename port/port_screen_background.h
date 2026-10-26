#ifndef PORT_SCREEN_BACKGROUND_H
#define PORT_SCREEN_BACKGROUND_H

#include <stdbool.h>
#include <stdint.h>

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
    /* Fusion's local reveal replaces the installed decoration with a solid
     * BG3 field. It is distinct from the standalone map hint's green field. */
    PORT_BACKGROUND_LOCAL_MAP_HINT,
    PORT_BACKGROUND_MENU_TITLED_NORMAL,
    PORT_BACKGROUND_MENU_TITLED_DAMAGED,
    PORT_BACKGROUND_MENU_TITLED_CRITICAL,
    PORT_BACKGROUND_KINSTONE_PIECES_NORMAL,
    PORT_BACKGROUND_KINSTONE_PIECES_DAMAGED,
    PORT_BACKGROUND_KINSTONE_PIECES_CRITICAL,
    PORT_BACKGROUND_SWORD_TECHNIQUES_NORMAL,
    PORT_BACKGROUND_SWORD_TECHNIQUES_DAMAGED,
    PORT_BACKGROUND_SWORD_TECHNIQUES_CRITICAL,
    /* Fixed blue map decoration, installed after the health preload is
     * overwritten. One image for all destinations, unlock masks and health. */
    PORT_BACKGROUND_FAST_TRAVEL,
    PORT_BACKGROUND_COUNT
} PortBackgroundContext;

typedef enum {
    PORT_BACKGROUND_MENU_PLAIN,
    PORT_BACKGROUND_MENU_TITLED,
    PORT_BACKGROUND_MENU_PIECES,
    PORT_BACKGROUND_MENU_TECHNIQUES,
    PORT_BACKGROUND_MENU_FUSION,
    PORT_BACKGROUND_MENU_FAMILY_COUNT
} PortBackgroundMenuFamily;

typedef enum {
    PORT_BACKGROUND_HEALTH_NORMAL,
    PORT_BACKGROUND_HEALTH_DAMAGED,
    PORT_BACKGROUND_HEALTH_CRITICAL
} PortBackgroundHealth;

typedef enum {
    PORT_BACKGROUND_HEADING_NONE = 0,
    PORT_BACKGROUND_HEADING_TITLE_PLATE = 1,
    PORT_BACKGROUND_HEADING_CENTERED = 2
} PortBackgroundHeadingStyle;

/* Bounded snapshot published at menu/scene installation boundaries.
 * Zero heap allocation. Describes the semantic family, installed health,
 * and animated heading pose for title-cap and background composition. */
typedef struct {
    PortBackgroundMenuFamily family;
    PortBackgroundHealth installedHealth;
    PortBackgroundHeadingStyle headingStyle;
    int16_t headingX;
    int16_t headingY;
    int16_t headingTransition;
    uint32_t generation;
} PortMenuBackgroundDescriptor;

/* The adapter supplies installed health, not health polled at presentation.
 * Invalid inputs fail closed. This function never infers game/task state. */
PortBackgroundContext Port_Background_MenuContext(PortBackgroundMenuFamily family, PortBackgroundHealth installedHealth);

typedef enum {
    PORT_BACKGROUND_SOURCE_NONE,
    PORT_BACKGROUND_SOURCE_IMAGE,
    PORT_BACKGROUND_SOURCE_BLACK,
    /* The white native logo field, with its native layer/effect target.
     * This is not an instruction to set the GBA backdrop color to white. */
    PORT_BACKGROUND_SOURCE_LOGO_FIELD,
    /* Resolve from the active palette-zero field, including palette fades.
     * Figurines copy entry 251 to zero; map hint copies entry 81 to zero.
     * Local fusion hints set zero and entry 95 together (native 0x475f).
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
