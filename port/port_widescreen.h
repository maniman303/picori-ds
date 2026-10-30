#ifndef PORT_WIDESCREEN_H
#define PORT_WIDESCREEN_H

#include "port_viewport.h"

/*
 * port_widescreen.h — single source of truth for the rendered viewport
 * width, shared by the ViruaPPU renderer and the engine-side on-screen /
 * culling tests in the port.
 *
 * Why this exists (see docs/widescreen-*.md):
 *   The GBA decomp hardcodes a 240-px playfield into its visibility math
 *   (CheckOnScreen's 0x16E, CheckRectOnScreen's 0xF0, per-enemy
 *   `scroll_x + 0x108` bounds, ...). When the PPU renders a wider frame
 *   but the engine still culls/parks against 240, entities the engine
 *   treats as "off-screen" (parked, or AI-gated past col 240) leak into
 *   the revealed area — the sprite-edge flicker that has blocked Phase 2.
 *
 *   The two reference GBA ports that actually ship widescreen both fix
 *   this the same way — by keying every cull/camera bound off one
 *   display-width constant instead of a literal 240:
 *     - SAT-R/sa2:      include/gba/defines.h (DISPLAY_WIDTH) +
 *                       include/game/shared/stage/camera.h (IS_OUT_OF_RANGE,
 *                       IS_OUT_OF_DISPLAY_RANGE, CAM_BOUND_X).
 *     - pret/pokeemerald: src/event_object_movement.c cull at
 *                       `x >= DISPLAY_WIDTH + 16`; src/sprite.c parks
 *                       off-screen sprites at `DISPLAY_WIDTH + 64`.
 *
 * MODE1_GBA_WIDTH / PORT_VIEW_WIDTH describe allocation capacity, injected
 * by the legacy `--widescreen_width=N` option (default 240). Runtime callers
 * use the helpers below for profile/room-capped camera and culling bounds.
 */

/* Mirror of the ViruaPPU compile-time width. Kept #ifndef-guarded so
 * this header is self-contained when included by a TU that does not pull
 * in <cpu/mode1.h>; xmake always defines MODE1_GBA_WIDTH for tmc_pc, so
 * in a real build the injected value wins and this fallback is inert. */
#ifndef MODE1_GBA_WIDTH
#define MODE1_GBA_WIDTH 240
#endif

/* Legacy capacity macros. Taller rendering remains disabled until raster
 * state, streaming and sprite metadata support it; hardware timing stays 160. */
#define PORT_VIEW_WIDTH (MODE1_GBA_WIDTH)
#define PORT_VIEW_HEIGHT 160

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime state for a wide-width build. These collapse to native no-ops when
 * MODE1_GBA_WIDTH == 240.
 *
 * FallbackNative == 1 means this scene renders the GBA-native 240px canvas
 * and the presenter fits it at its correct 3:2 aspect (pillarbox bars) —
 * fixed canvases are NEVER stretched to the widescreen target.
 *
 * EffectiveViewWidth is additionally capped by the current room's width:
 * rooms narrower than the selected profile target but wider than 240 (256 and
 * 272px rooms — most dungeons/interiors with any extra world) run true
 * widescreen at their full room width, pillarboxing only the remainder. */
/* Engine boundaries only: call after loading a room and when entering or
 * completing a transition. Repeated presentation reads the published snapshot. */
void Port_Widescreen_PublishScene(void);
void Port_Widescreen_RoomLoaded(void);
/* Scene validation is derived, not asserted: the map-upload boundary calls
 * SetTallValidated(1), which honours the request only when the staged fill and
 * VRAM upload match the current room key, camera tile origin, candidate
 * aperture height and required row count. Any other caller can only clear it. */
void Port_Widescreen_SetTallValidated(int validated);
/* Stage taller map rows before publishing the larger world aperture. */
void Port_Widescreen_MapFilled(unsigned mode);
void Port_Widescreen_MapUploaded(void);
void Port_Widescreen_BeforePresent(void);
void Port_Widescreen_ManagerCreated(void);
void Port_Widescreen_EntityCreated(void);
int Port_Widescreen_FallbackNative(void);
int Port_Widescreen_IsActive(void);
/* Canvas-wide live-gameplay HUD/dialogue layout, independent of room width.
 * Fixed screens and native handoffs return false. */
int Port_Widescreen_LayoutActive(void);
int Port_Widescreen_LayoutWidth(void);
int Port_Widescreen_LayoutHeight(void);
int Port_Widescreen_EffectiveViewWidth(void);
/* Vertical world reveal remains native until streaming, OAM and raster
 * capabilities are published together. Camera/tile callers use this accessor
 * so future validated taller apertures do not require more engine edits. */
int Port_Widescreen_EffectiveViewHeight(void);
unsigned Port_Widescreen_MapBufferSize(void);
int Port_Widescreen_HudRightAnchor(void);
/* True while the map BGs are what the PPU renders (>=1 shadow registered);
 * overlay screens (storybook, pause) drop this to 0 -> present native 240. */
int Port_Widescreen_ShadowsLive(void);

/* Effective logical canvas width from the immutable profile snapshot. The
 * window size has no effect on camera/culling geometry. Use EffectiveViewWidth
 * for room-capped camera bounds; TargetViewWidth is the selected canvas extent. */
int Port_Widescreen_TargetViewWidth(void);

/* Camera rest position for a target at absolute world x (origin included):
 * target centered in the live view width, clamped so the view stays inside
 * the room. THE single formula for "where the camera stops" — used by the
 * follow camera (Scroll1), the snap cameras (InitializeCamera,
 * sub_08080974, sub_080809D4) and the script wait
 * (WaitForCameraTouchRoomBorder, which busy-waits for scroll_x EQUALITY:
 * any drift between producers and that check deadlocks a cutscene).
 * Reduces to the GBA `x - 120` clamped to [origin, origin+width-240] at an
 * effective view width of 240. */
int Port_Widescreen_CameraRestX(int target_x);
int Port_Widescreen_CameraRestY(int target_y);

#ifdef __cplusplus
}
#endif

#endif /* PORT_WIDESCREEN_H */
