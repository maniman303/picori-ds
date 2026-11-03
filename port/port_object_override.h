#ifndef PORT_OBJECT_OVERRIDE_H
#define PORT_OBJECT_OVERRIDE_H

#include <stdbool.h>
#include <stdint.h>

/* Logical names used by the shipped object-override hooks. Keep these in
 * `assets/override/objects/<name>[_<language>].png`. */
#define PORT_OBJECT_FILE_SELECT_HEADER "file_select_header"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fixed-screen object overrides.
 *
 * Artists can replace a logical on-screen object with a PNG placed at:
 *
 *   <assets>/override/objects/<name>.png
 *   <assets>/override/objects/<name>_<language>.png
 *
 * The language-specific file is preferred; the generic name covers every
 * language. If neither file exists, the game draws its own object unchanged.
 *
 * The PNG may be the same size as the original object or larger. Its centre
 * is aligned with the centre of the original object's on-screen bounds, so
 * extra transparent padding naturally extends the artwork in any direction.
 * The override only replaces pixels; the object keeps its normal position,
 * slide/fade timing and logic.
 *
 * To add a new object, call Port_ObjectOverride_Submit() from its draw site
 * with its native-screen bounds. When it returns true, skip the original
 * sprite for that frame (for example set spriteSettings.draw = 0). Object
 * bounds for an entity sprite can be measured with
 * Port_Draw_MeasureFrame().
 */

/* asset_root is the directory containing override/ (normally .../assets). */
void Port_ObjectOverride_Init(const char* asset_root);

/* Forget cached PNGs so newly added/edited files are picked up. */
void Port_ObjectOverride_RequestReload(void);

/* Start a new frame's request list. Called from FlushSprites. */
void Port_ObjectOverride_BeginFrame(void);

/* Submit an override for one logical object. Coordinates are native-screen
 * pixels (the 240x160 GBA frame), not canvas/margin coordinates. language is
 * gSaveHeader->language, or -1 for no language-specific lookup. obj_palette
 * is the original OBJ palette bank (0..15) used to follow the engine's fade,
 * or -1 to draw the PNG colours untouched. Returns true when an override image
 * exists; the caller must skip its own rendering. */
bool Port_ObjectOverride_Submit(const char* name, int language,
                                int x, int y, int width, int height, int obj_palette);

/* Overlay variant: records the same centred PNG draw but does NOT ask the
 * caller to skip its original object. Use it for a shared frame with a
 * transparent window (e.g. a heading border whose text stays game-drawn). */
bool Port_ObjectOverride_SubmitOverlay(const char* name, int language,
                                       int x, int y, int width, int height, int obj_palette);

/* Draw pending overrides on the composed canvas. native_x/native_y are the
 * canvas position of the native 240x160 rectangle (0,0 for native 3:2). */
void Port_ObjectOverride_Draw(uint32_t* canvas, int width, int height, int pitch,
                              int native_x, int native_y);

/* Measure a sprite frame's drawn bounds relative to its entity origin. flags
 * is the normal OAM command flags word (gOamCmd._4 | gOamCmd._6 << 16); pass 0
 * for an unflipped normal sprite. Returns false for missing/corrupt frames. */
bool Port_Draw_MeasureFrame(uint32_t sprite_index, uint32_t frame_index, uint32_t flags,
                            int32_t* out_x, int32_t* out_y, int32_t* out_width, int32_t* out_height);

#ifdef __cplusplus
}
#endif
#endif
