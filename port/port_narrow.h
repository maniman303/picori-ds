#ifndef PORT_NARROW_H
#define PORT_NARROW_H
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
bool Port_Narrow_IsActive(void);
bool Port_Narrow_KeyboardActive(void);
bool Port_Narrow_HudActive(void);
#ifdef __cplusplus
}
#endif

#define PORT_NARROW_WIDTH 212
#define PORT_NARROW_CROP 14
#define PORT_NARROW_TEXT_TILES 24
#define PORT_NARROW_TEXT_WIDTH (PORT_NARROW_TEXT_TILES * 8)
#define PORT_TEXT_LINE_STRIDE 208

static inline int Port_Narrow_KeyboardCursorX(int column) {
    /* Native OAM coordinates; PPU subtracts PORT_NARROW_CROP. */
    return 30 + column * 15;
}

/* Existing glyph storage retains its 208px stride. Only the visible line
 * width changes; a pending glyph moves intact to the next line/page. */
static inline int Port_Narrow_TextOverflow(int line, int cursor, int advance, int width) {
    const int start = line ? PORT_TEXT_LINE_STRIDE : 0;
    return cursor > start && cursor + advance > start + width;
}

#endif
