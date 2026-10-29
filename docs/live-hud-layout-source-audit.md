# Live HUD layout: source audit

This is a source-level starting point for the outstanding semantic HUD and
dialogue task. Coordinates below are native BG0 tile coordinates unless stated
otherwise; they are not evidence of rendered 4:3 behavior. BG0 has a 32-tile
stride, so an index is `(row * 32 + column)`.

| Owner in `src/ui.c` | Native footprint or position | Proposed anchor / constraints |
| --- | --- | --- |
| `DrawHearts` / `EraseHearts` | Row 1, columns 0–10; row 2 when max health exceeds 10 hearts. Erasure clears 12 columns. | Top left. The animated heart OBJ is separate and must stay with these tiles. |
| `DrawChargeBar` / `EraseChargeBar` | Columns 0–11, row 2 or 3 depending on max health. | Top left; the row changes with the second heart row and the hide flag. |
| `DrawKeys` | Rows 16–17, columns 25–28. | Bottom right; present only when keys are allowed and the area has keys. |
| `DrawRupees` | Rows 18–19, columns 24–28. | Bottom right; wallet icon and three digit tiles move together. |
| A/B/R button elements | Initial centers A (208,28), B (184,28), R (216,14); animated vertically when hidden or a message is active. | Top right. Apply a common translation to the button and its dependent elements without changing the animation. |
| Item and action-text elements | `ItemUIElement` and `TextUIElement` copy the corresponding button element's position. | Follow the button once; do not apply the translation a second time. Item-count graphics are part of the item sprite's tile data. |
| Heart OBJ | `HeartUIElement` derives its position from health, with a second-row adjustment above ten hearts. | Top left; retain health-dependent positioning and message/hide behavior. |
| Ezlo nag OBJ | Starts at (16,144) and changes type during its animation. | Candidate bottom-left widget, requiring visual and lifecycle validation before assigning a translation. |

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

The existing VBlank shadow publication identifies the live message rectangle
and separately centers the room banner's BG0 rows 5–6. It currently publishes
only a horizontal shift. `Port_Widescreen_IsActive` requires a world scene and
a world width greater than 240; it is therefore insufficient by itself as the
gate for a 256 × 192 canvas whose room aperture has fallen back to native
width. The layout needs the published functional scene and canvas geometry,
with fixed-screen handoffs kept on their native composition.

Before marking the task complete, trace and validate special minigame and
item-get writers, script-positioned windows, portraits and choice indicators,
HD text and touch/capture consumers. Exercise native and wide profiles as well
as a test-only taller profile, including hide flags, two heart rows, animation,
dialogue overlap and fixed-screen transitions. The production 4:3 safety gates
must remain in force until their independent acceptance work is complete.
