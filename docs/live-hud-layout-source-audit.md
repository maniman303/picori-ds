# Live HUD layout: source audit

This is the source-level ownership record for the semantic HUD and dialogue
layout. Coordinates below are native BG0 tile coordinates unless stated
otherwise. Production 4:3 is still disabled; the tall path is exercised through
the test-only geometry rather than a released profile. BG0 has a 32-tile
stride, so an index is `(row * 32 + column)`.

| Owner in `src/ui.c` | Native footprint or position | Implemented anchor / constraints |
| --- | --- | --- |
| `DrawHearts` / `EraseHearts` | Row 1, columns 0–10; row 2 when max health exceeds 10 hearts. Erasure clears 12 columns. | Top left, unchanged. The animated heart OBJ is separate and stays with these tiles. |
| `DrawChargeBar` / `EraseChargeBar` | Columns 0–11, row 2 or 3 depending on max health. | Top left, unchanged; the row still changes with the second heart row and the hide flag. |
| `DrawKeys` | Rows 16–17, columns 25–28. | Bottom right; the BG0 rectangle moves by `(canvasW-240, canvasH-160)` only when installed and visible. |
| `DrawRupees` | Rows 18–19, columns 24–28. | Bottom right; wallet icon and three digit tiles move together under the same installed/hide gates. |
| A/B/R button elements | Initial centers A (208,28), B (184,28), R (216,14); animated vertically when hidden or a message is active. | Top right; X comes from the logical canvas even when the room aperture falls back to 240. Y animation is untouched. |
| Item and action-text elements | `ItemUIElement` and `TextUIElement` copy the corresponding button element's position. | Follow the button once; no second translation. Item-count graphics remain part of the item sprite's tile data. |
| Heart OBJ | `HeartUIElement` derives its position from health, with a second-row adjustment above ten hearts. | Top left, unchanged; health-dependent positioning and message/hide behavior are retained. |
| Ezlo nag OBJ | Starts at (16,144) and changes type during its animation. | Bottom left; the OAM draw adds `(0, canvasH-160)` for both nag element types without mutating animation coordinates. |

The top-right rule preserves the native edge distance rather than a constant
display-pixel gap. At 16:9 (285 px) the A/B/R cluster is shifted by +45 px, so
the R sprite's right edge remains 12 native px from the canvas edge, exactly as
in the 240 px game. The visual gap grows with presentation scale; it is not an
under-shifted anchor. EU field captures verified native 240 × 160, 16:9 and
4:3-effective-native placements in `.tmp/button-anchor-2026-10-30/`.

These footprints identify the normal HUD writers, not exclusive ownership of
the cells. Messages and other UI can overwrite BG0; position alone cannot
identify the active semantic owner. In particular, the existing mode-1 wide
path now moves only the installed key and rupee rectangles, except on published
message/banner lines. Each counter has its own installed-state and hide-flag
gate; nonzero BG0 scroll fails closed to native placement. A bottom-anchored
layout still needs vertical relocation and verified handling of other writers.

`ChangeWindowSize` in `src/message.c` derives the animated `gNewWindow` from
the final message dimensions and position, and `CreateWindow` copies it to
`gCurrentWindow` after drawing the frame, text and continuation cursor into
BG0. `Message_GetWindowRect` exposes that current animated rectangle, not the
intended final rectangle. A stable destination for opening and closing needs
both the final message geometry and the current rectangle; anchoring the latter
independently each frame would move its center. `DeleteWindow` clears the
original BG0 cells and calls `RecoverUI` / `RefreshUI`, so those writes must
remain in native coordinates.

The VBlank publication now gates on `Port_Widescreen_LayoutActive`, which uses
the published functional scene and canvas rather than the room aperture. Fixed
screens and native handoffs publish no anchors. `Port_Widescreen_UpdateShadows`
publishes the live BG0 rectangle, the final-rect translation, both shift axes,
and the independent lower-right HUD shift. Banners continue to publish only
their top-center X shift and keep `shift_y == 0`.

`port/ppu/src/mode1.c` and `port/shaders/ppu_core.glsl` both sample the moved
message band and key/rupee rectangles from their native scanlines, suppress the
native copies, and leave unrelated BG0 cells on the same line alone. The choice
cursor, continuation arrows and choice text are BG0 pixels inside the same
window rectangle, so they follow that transform. HD text consumes the same X
and Y translation and validates the text canvas against the source band before
drawing. There are no portrait OAM parts in this engine's normal dialogue path.

ROM-free coverage renders a 256 × 192 canvas with distinct per-row palette
banks: top and bottom message requests, opening/closing publication, key and
rupee movement, uninstalled counter behavior, message/HUD overlap, banner
separation, fixed-screen clearing, and 160-line preservation. Software, Vulkan
and GLES agree byte-for-byte in `tools/ppu_gpu_parity.cpp`; the CPU expectations
are asserted independently in `tools/tests/widescreen_ppu.c`. Production 4:3
and 32:9 remain disabled by their independent world, scene and profile gates.
Item-get/special-minigame visual captures and physical-device acceptance remain
open; touch overlays do not consume HUD coordinates.
