# Native 4:3 rendering and background overrides

Research date: 2026-10-24. Status: runtime implementation planned; initial local artwork export completed.

Implementation has started with the port-owned catalog and profile contracts;
rendering integration remains pending. See the ordered
[implementation tasks and verification record](aspect-ratio-and-background-overrides-tasks.md).

This document plans the requested removal of the widescreen toggle, true
256 × 192 rendering for 4:3, corner-anchored live-gameplay UI, bottom gameplay dialogue,
effect-aware splash extension, named contextual PNG background artwork for every
supported ratio, and a complete local export of original backgrounds into
`assets/override/32/`. Runtime rendering and configuration changes remain planned.
The initial standalone artwork export is now available; see section 12.

Scope clarified by the user: “background” means a screen's visual setting by
function, such as the title or save/load screen. It does not mean a hardware
BG plane. The preferred first implementation keeps those screens rendering
at their original 240 × 160 layout and places one static, context-selected image behind their content to
fill added canvas space. World gameplay always uses extended tilemap rendering;
no contextual static images fill gameplay world gaps. Full replacement of
existing screen artwork is deferred rather than required for this approach.

The design prioritizes the RG DS Plus with the user's stated 1 GB RAM limit,
leaving room for the OS, audio, assets, graphics driver, and future second-screen
support. Memory figures below are estimates or proposed budgets, not device
measurements. Source inspection, rather than older design notes, establishes
current behavior. No claim of an exhaustive visually verified image catalog or
on-device performance is made yet.

## 1. Findings in the current tree

| Area | Evidence | Consequence |
| --- | --- | --- |
| Aspect selector | `port/port_runtime_config.h`, `PortAspectMode`; `port/port_ppu.cpp`, `Port_PPU_ComputeViewportRects` | Native, 16:9, 21:9, and 32:9 select a presentation **stage**, not a fixed internal resolution. Native currently means unconstrained stage, not strictly 3:2. There is no 4:3 mode. |
| Actual world width | `port/port_linked_stubs.c`, `Port_Widescreen_TargetViewWidth` | Width is `floor(windowW * 160 / windowH)`, clamped to [240, compile-time capacity], independent of the selected stage ratio. Comments mentioning tile rounding disagree with the actual pixel-based calculation. |
| Toggle | `port/port_imgui_display_tab.inc`; `port/port_runtime_config.cpp`, `Port_Config_WidescreenEnabled` | The toggle does affect rendering, HUD, culling, and menu artwork when capacity exceeds 240. It is forced off by Console-Parity. It is ineffective in native-capacity builds or fallback scenes. Removing only the checkbox would leave the conflicting state behind. |
| Build differences | `xmake.lua`, `widescreen_width`; `build.py` configure command; `scripts/build-rgdsplus.sh` | Direct xmake defaults to capacity 240. The user build and RG DS Plus build request 384. Thus neither consistent 4:3 nor full 32:9 world rendering currently follows from the menu labels. |
| Scene fallback | `Port_Widescreen_FallbackNative`, `Port_PPU_VisibleFrameWidth`, `Port_Widescreen_ShadowsLive` | Fixed canvases, narrow/underpainted rooms, overlays, and scroll actions 2/4/5 can render native width. Fallback prevents invalid world reveal; preserve its safety purpose. |
| Height | `port/port_widescreen.h`; `port/ppu/include/cpu/mode1.h`; `port/ppu/include/ppu_memory.h` | Height is 160; `PPUMemory` exposes width and pitch only. Existing allocation capacity alone cannot make a 192-line frame work. |
| Software renderer | `port/ppu/src/mode1.c`, geometry setup, text BG renderer, OBJ renderer, affine precompute, frame renderer | Rendering, IO snapshots, affine arrays, window limits, and OBJ Y interpretation depend on 160. Display modes 1 and 2 are handled in this file; there is no separate current `mode2.c`. |
| Camera and streaming | `src/scroll.c`, `Scroll1`, snap cameras, transition helpers; `port/port_linked_stubs.c`, shadow population | Horizontal camera adaptation exists. Vertical follow uses 80, limits use `DISPLAY_HEIGHT`, and snap cameras subtract 80. World shadows extend horizontally; vertical reveal is not a completed feature. |
| HUD | `src/ui.c`, PC sprite X adjustment; PPU `MODE1_WS_HUD_RIGHT_NATIVE_X = 176` | Right-side OBJ widgets and BG0 columns move horizontally. No general Y anchoring exists. Moving BG0 indiscriminately also moves dialogue, banners, and menu content. |
| Dialogue | `src/message.c`, `Message_GetWindowRect`; `Port_Widescreen_UpdateShadows` | The live animated window rectangle is already available. Current PPU relocation centers it horizontally only and clamps source coordinates to 240 × 160. Location banners have a separate band. |
| HD dialogue | `port/port_hd_text.cpp`; `port/port_imgui_menu.cpp` | HD text rejects heights other than 160, independently computes X shift, and is called with literal height 160. It must share the final dialogue geometry. |
| Splash and title | `src/title.c`, `HandleNintendoCapcomLogos`, `HandleTitlescreen`, `UpdateSwordBgAffineData` | Splash uses gfx groups 16/1 and palette groups 1/2. Title uses group 2 and language-dependent palettes/layers; group 3 supplies localized OBJ artwork. Title contains affine sword animation, light rays, alpha blending, and foreground sprites. A screenshot is not the title's replaceable background. |
| PC startup splash | `port/port_asset_bootstrap.cpp`, `Port_PaintBootSplash`; `port/port_gpu_renderer.cpp`, `Port_GPU_PaintBootSplash` | This is a separate procedural dark-teal loading screen before `AgbMain`, not the game's white logo screen. It has no GBA fade state or ROM background image to export. |
| Existing menu extension | `port/port_menu_background.c`, `port/port_menu_background_imgui.cpp`, `port/port_imgui_menu.cpp` | Selected file/pause menus decode 256 × 160 BG3 and repeat the visible 240 columns in side bars using ImGui rectangles. This is gated off for several effects/backends/settings and cannot supply general 4:3 or effect-correct PNG replacement. |
| Asset loading | `src/common.c`, `LoadGfxGroup`; `port/port_asset_loader.cpp`, `Port_LoadGfxGroupFromAssets` | Groups load many raw tile/map/palette components, sometimes into EWRAM. The asset-backed gfx group route currently returns false outside USA, falling back to ROM. Putting the new feature exclusively there would miss other regions and ROM fallback. |
| Existing mods | `port/port_asset_loader_mods.inc`; `mods/README.md` | Existing replacements operate on raw asset paths. Keep that contract; a screen-sized PNG is not a valid raw VRAM replacement. |
| Extraction | `tools/src/assets_extractor/assets_extractor.hpp`; `data/gfx/gfx_groups.s`; `assets/{gfx_groups,palette_groups,palettes}.json` | Component descriptions and palette conversion exist. Reconstructing images needs map, tiles, palette, transparency, and scene state together. A gfx sheet export is insufficient. |
| Capture and testing | `tools/render_scenes.tsv`, `tools/render_scene_check.py`, `tools/ppu_parity_check.sh` | Existing named scenes cover file select, inventory, field, and top/bottom messages. PPU parity's committed corpus is only two boot frames; passing it does not establish gameplay correctness. |
| Other consumers | `port/port_bugreport.cpp`, GPU renderer/raster paths, shared framebuffer/voxel paths | Several exports and overlays assume 160 or native planes. Geometry must be carried through explicitly, not inferred from buffer capacity. |

Older `docs/widescreen-status-2026-05-30.md` and
`docs/widescreen-phase2-design.md` include historical/reverted approaches.
`docs/WIDESCREEN_SWARM_2026-09-12.md` and
`docs/REGIONAL_WIDESCREEN_FIXES_2026-09-13.md` explain later fixes. Do not recreate
old stretch paths or old submodule patches from those documents.

## 2. One aspect setting and explicit frame geometry

Introduce a small C-compatible port geometry API, preferably in
`port/port_viewport.{h,c}`. Keep `port_widescreen.h` wrappers temporarily so engine
call sites can migrate without a large upstream-facing rename. Publish one
immutable snapshot before camera/layout updates and PPU shadow construction;
render and present consume that same snapshot. Window sizes published only at
presentation time must not make simulation and rendering disagree for a frame.

Snapshot fields: selected ratio ID, logical canvas W/H, pitch/capacity, world
rectangle, native content rectangle, UI safe rectangle, scene capability flags,
and a generation number. Distinguish screen canvas from available world area.
Do not use a room-width cap to shrink the HUD or change override dimensions.

Proposed fixed profiles, before optional internal supersampling:

| Setting | Override directory | Logical canvas | Notes |
| --- | --- | --- | --- |
| 3:2 / GBA | `32` | 240 × 160 | Explicit native geometry. |
| 4:3 / DS | `43` | **256 × 192** | Exact DS pixel dimensions, not a resized GBA frame. |
| 16:9 | `169` | 285 × 160 | Ceiling of 160 × 16/9. |
| 21:9 | `219` | 374 × 160 | Ceiling of 160 × 21/9. |
| 32:9 | `329` | 569 × 160 | Ceiling of 160 × 32/9; exceeds the currently shipped 384 capacity and native OBJ X range assumptions. |

Except for the explicit 4:3 / DS profile at 256 × 192, every profile keeps a
height of 160 pixels. Calculate width as `ceil(160 * ratioWidth / ratioHeight)`:
when the exact ratio requires a fractional width, always round upward to the
next integer pixel, not down or to the nearest even/tile-aligned width. Thus
16:9 requires 285 × 160, not 284 × 160 or 286 × 160. This ceiling rule applies
to canvas dimensions; the native-content centering offsets still use floor.
Preserve square source pixels and aspect-fit the integer canvas inside the
requested stage; accept the subpixel-rounding remainder as bars/fill. Document
these sizes in the settings and override guide. Every backend and PNG validator
must use the same selected profile. Profile sizes become a versioned public
asset contract.

Make profile selection determine the renderer target even when the window has
another aspect. Resizing only changes presentation rectangles. This makes
`assets/override/169/title_screen.png` well-defined and eliminates decoding on
every window resize. The current unconstrained native stage can remain a
presentation policy, but native **rendering** becomes explicitly 240 × 160.
Do not add arbitrary automatic ratios in the first implementation; supporting
them requires an explicit resolution/file selection contract.

Remove `widescreen_enabled` storage, load/save table entry, setter/toggler,
checkbox, tooltips, and remaining gates in renderer/menu/config/tests. Existing
configs keep `aspect_mode`; ignore and remove the obsolete key on the next
normal config save. Preserve existing enum numeric values by appending 4:3,
and serialize it as `4:3`. Parse legacy aliases already supported.

Console-Parity remains an explicit effective native profile, with the user's
selected aspect retained for restoration. Explain the effective native mode
in settings. Tests that used `widescreen_enabled=false` to pin native behavior
must now select native geometry explicitly. Overrides and enhanced layout are
disabled in parity mode.

Capacity must support the published profiles. Prefer a bounded maximum width
of at least 569 and height of 192, with runtime **visible** dimensions; capacity
must not enlarge every loop. Retain native-only N64 constraints. Keep the old
build option as a deprecated capacity alias initially; update `build.py`, the
RG script, tools, and documentation together. A capacity-limited build must
disable unsupported profiles visibly, never accept a 569-pixel override and
then present only 384 pixels.

## 3. True 256 × 192 gameplay

### Renderer height and GBA timing are different concepts

Keep 240/160 constants for emulated hardware contracts, timing, packed data,
native scene source coordinates, and original HDMA tables. Add visible height
to `PPUMemory`/frame geometry and route it through software loops, affine
reference arrays, clipping, frame copies, uploads, texture dimensions,
screenshots, and overlays. Audit all struct initializers and serialized PPU
snapshots when extending `PPUMemory`; version capture formats rather than
misreading older snapshots. Keep `MODE1_GBA_HEIGHT` as the native hardware
constant and introduce a separate bounded render height.

The existing generic PPU buffer is 1280 × 360 on PC, so base frame storage
already has capacity; this does not fix height-dependent scratch arrays.
Do not change VBlank timing or simply call the existing pre-line callback
192 times. `port/port_hdma.c` advances source tables per line without an asset
length contract. Blindly reading 32 extra entries can overrun game tables.

Build the original 160-line register sequence once. For normal scenes extend
the frame's stable state to new lines. Classify each HDMA/raster effect and
provide an explicit continuation/translation rule: constant brightness can
extend; affine references may continue mathematically; iris/window geometry
needs port-side full-canvas coordinates. Effects with no established extension
keep their native world aperture inside the 256 × 192 canvas, with defined
effect-aware margins. Track these as QA cases, not permanent unexamined
fallbacks. Hardware line count and game update cadence remain unchanged.

### World streaming and camera

Add shared camera-rest helpers for both axes:
`clamp(target - worldExtent/2, origin, max(origin, origin + roomExtent - worldExtent))`.
Use them in follow, initialization, snap/script cameras, and camera-border wait
checks (`src/scroll.c`, associated script commands). Retain original pacing and
step limits; replace only viewport-derived limits, not unrelated 80/160 values.

For a large room, render and feed 256 × 192 world pixels. Extend the shadow
map sampler in two dimensions, mapping output pixels to room coordinates and
the current VRAM ring origin. Audit `ram_sub_080B197C_c`, tile DMA/update paths,
and tileset managers for bottom/right reveal residency. A 32 × 32 tilemap has
space for 24 visible rows plus scroll margin, but that does **not** prove its
tiles or rolling updates are resident. Reuse streamed graphics where possible;
do not expand GBA VRAM or copy whole rooms on every frame.

Rooms smaller than the logical canvas keep a clamped world aperture and a
stable centered content offset. A native 240 × 160 scene occupies (8,16) in
4:3. For gameplay, unavailable world area stays a defined procedural void or
existing presentation fill; a contextual PNG must never supply missing world.
For fixed title/menu screens, contextual artwork may fill the added area. HUD stays
anchored to the 256 × 192 canvas. No stretched world and no accidental access
beyond room maps. Camera motion must not oscillate between two view sizes as
content detection changes; decide aperture at scene/transition boundaries.

Transitions must keep source/destination mapping coherent. Generalize both
axes' viewport-derived step counts and buffer refresh extents, while preserving
script completion checks. Test stairs, doors, screen rolls, digging-cave iris,
barrel rotation, portals, falling, and scripted camera pans. Retain narrowly
scoped native-aperture transitions until their extension is validated.

### Entity visibility and OAM

Audit `CheckOnScreen`, `CheckRectOnScreen`, sprite-piece clipping, region-based
AI/tile streaming, and per-entity vertical parking/culling. Rows 160–191 become
visible; treating y=160 as an invisible parking location is now wrong.
`port_draw.c` and voxel OAM tagging explicitly rely on this today.

The PPU currently sign-wraps OBJ Y at native height and X at frame width.
Increasing height can reinterpret sprites parked at y=160 as visible. Wider
than 512 also cannot encode every screen X in native 9-bit OAM. Audit the
existing extra sprite metadata/tagging mechanisms first. Use bounded port-side
signed screen coordinates/visibility metadata where native OAM is ambiguous,
preserving the packed OAM format and native decode in parity mode. Cover
direct UI sprites and entity sprites; a wider framebuffer alone cannot make
32:9 correct. Do not globally move disabled OAM entries without distinguishing
their original disabled/hidden semantics.

## 4. Preserve fixed-screen UI; adapt live-gameplay HUD only

Every identified fixed screen keeps its entire original **240 × 160** layout:
title, boot logos, file selection and operations, inventory, quest status, all
map views and hints, fast travel, fusion, figurines, sleep/save prompts, game
over, prologue, credits, and ending. Render its original UI, text, cursors,
windows, sprites, maps, animations, and transitions as one native canvas, then
center that canvas in the selected logical frame. At 4:3 its top-left is
**(8,16)** in 256 × 192. No individual fixed-screen widget is reanchored,
resized, reflowed, or moved into a margin. Internal map scrolling still works
inside the original screen. Apply one translation to the complete native
composition, rather than modifying engine UI coordinates.

This is separate from live gameplay, which extends the world tilemaps and
uses the requested corner HUD and bottom dialogue. Gate those layout changes
on the functional live-gameplay context, not merely `TASK_GAME`: that task
also hosts pause/map/fusion screens. Popup dialogue inside a fixed menu stays
at its original native position. HD text or touch hit-testing must use the
same canvas transform and retain the same original fixed-screen layout.

For other ratios center the same native canvas with integer offsets
`(floor((W-240)/2), floor((H-160)/2))`; a one-pixel asymmetry for odd widths is
intentional. Added artwork or procedural fill is behind this canvas and visible
only in its margins. The native 3:2 screen covers the full canvas.

The following anchor changes apply **only to live gameplay**.

Create a small table of semantic anchors, native rectangles, and translation
rules in the port layer. Reuse original artwork, glyphs, tile buffers, counters,
and game-side UI state. Avoid spreading ratio checks throughout engine files.

For 4:3, use the original distances from the relevant canvas edge:

| Element | Anchor | Translation from 240 × 160 coordinates |
| --- | --- | --- |
| Hearts and charge HUD | Top left | (0,0) |
| A/B/R buttons, labels, item icons | Top right | (+16,0) |
| Rupees, keys, lower-right counters | Bottom right | (+16,+32) |
| Any verified lower-left widget | Bottom left | (0,+32) |
| Location banner | Top center | (+8,0), independent of dialogue |

Confirm individual widget membership from `src/ui.c` before freezing the
table: BG0 tiles and OBJ parts must move together. Use widget rectangles/
identities, not a generic `x >= 176` rule for every BG0 row. Preserve hide flags,
special minigame HUD, item-get UI, multi-row hearts, and animation offsets.
Translate signed coordinates before OAM packing.

Keep the existing native dialogue composition and text wrapping. Extend the
live-window remap to both X and Y in the BG0 sampling path. For 4:3, standard
dialogue is centered horizontally and anchored to the bottom with its original
bottom inset. For a fully open native bottom window this gives (+8,+32).
For top-requested standard dialogue, derive its destination from the final
bottom anchor rather than adding 32 to its original top position.

Use one stable destination anchor for opening/closing animations: move the
window's animated source rectangle relative to its intended final rectangle
so it grows in place. Do not recompute its bottom anchor from shrinking height
each frame. Choice cursors, continuation arrows, portraits, and any OBJ parts
must follow the same transform. Explicitly classify unusual script-positioned
windows; default ordinary messages to the requested bottom position in 4:3,
and record necessary special-window exceptions for review.

Share the resolved transform and actual frame height with `port_hd_text.cpp`,
touch overlays where relevant, and capture tools. Recover/erase BG0 in native
coordinates so closing dialogue restores HUD correctly. Maintain current wide
horizontal centering in other ratios unless intentionally changed.

## 5. Hardcoded screen families and shared filenames

“Background” means a screen's visual setting by function, not its hardware BG
planes. The original walkable world and its parallax/weather remain live
rendering. No static filler image is selected for gameplay, including dialogue
over the world, narrow rooms, bosses, or gameplay transitions.

Use a small **hardcoded port catalog** mapping the semantic screen context to
an image filename or a procedural color rule. Keep it in code, shared by all
aspect ratios. Append only the selected ratio directory when resolving an
image. There is no `inventory.json`, per-ratio manifest, artist metadata file,
required hash table, or runtime lookup by graphics digest. Task/subtask/menu
state determines context; source gfx/palette IDs are research evidence and
exporter details, not artist requirements. The exporter hardcodes its shared
names in `tools/export_screen_backgrounds.py`; the future runtime catalog must
use those same literal names.

| Screen family | Override filename or generated field |
| --- | --- |
| Title forest | `title_screen.png` |
| File selection, new/name/language/options/copy/erase screens | `file_select.png` |
| Inventory, quest status, overworld/dungeon/detail maps, **all 22 detail-map region/state variants**, fast-travel map, kinstone-pieces and sword-techniques tabs, save and sleep confirmations | **`menu_background.png`** |
| All seven kinstone fusion variants | **`kinstone_background.png`** |
| Prologue illustrations | `prologue_01.png` … `prologue_06.png` |
| Credits illustrations | `credits_01.png` … `credits_08.png` |
| Final illustrated card | `ending_screen.png` |
| Boot logo screen | Generated white, **no override**; logos remain normal native foreground |
| Game over, cleared/text-only credits, post-credits saving | Generated black; original native letters/prompts unchanged |
| Figurine viewer/details | Generated live field color (palette-zero copied from entry 251); native frame/figurine UI unchanged |
| Standalone map-hint presentation with BG3 disabled | Generated color matching native bottom-center pixel (120,159): **#C8E090 / RGB (200,224,144)** in the EU reference. Keep `map_hint.png` as a solid reference swatch only; original map/markers unchanged |
| Live world, dialogue over world, room-based cutscenes | No contextual image; extend original world/effect rendering |

The many inventory/map/confirmation screenshots are native **UI over one
setting**, so they are not separate overrides. `menu_background.png` is the
clean common hat-pattern reference, without slots, maps, frames, or prompts.
Health-dependent native menu variations may keep animating/changing inside
the original screen while their added margins share this one image. The
fusion variants similarly share the stone-table setting in
`kinstone_background.png`; only their original native content varies.

Solid cases never inspect an override path, even if an artist happens to
leave an old `boot_logos.png` or similarly named file in the directory. An
image-family context whose expected file is absent/invalid uses **solid black**
in its margins, not another image, another ratio, repeated tiles, blur, a
previous context's image, or a substituted source color. Black fallback still
participates in applicable screen-wide effects (e.g. a fade to white).

### Research evidence: menus, maps, death, ending, and credits

The audit follows the task dispatch in `src/main.c`, the complete subtask enum
in `include/subtask.h`/dispatch in `port/port_stubs.c`, and the 20-entry pause
screen dispatch in `src/data/figurineMenuData.c`. “Solid” below describes the
decorative field, not whether foreground lettering, icons, or maps are visible.
Setting palette entry zero alone does not prove an entire screen setting is
solid; enabled artwork can still cover it. Distinguish proven solid phases,
artwork-bearing screens, and cases requiring reconstructed-image verification.

| Functional context / proposed ID | Source evidence and actual behavior | Planned fill/export treatment |
| --- | --- | --- |
| Inventory / `pause_inventory` | Pause screen 1 dispatches `PauseMenu_ItemMenu`; configuration row 0 selects palette 182 and gfx group 90. Existing `Port_MenuBackground_GetPixels` recognizes its decorative BG3. | Use `menu_background.png` in added margins. Preserve the complete original 240 × 160 layout and all live map/UI content in the centered native canvas. No per-screen or per-region image. |
| Quest status / `pause_quest_status` | Screen 2 dispatches `PauseMenu2`; row 1 selects palette 183/group 91. Existing BG3 extension recognizes it. | Use `menu_background.png` in added margins. Preserve the complete original 240 × 160 layout and all live map/UI content in the centered native canvas. No per-screen or per-region image. |
| Kinstone pieces / `pause_kinstone_pieces` | Screen 7 uses row 5, palette 204/group 125 and `PauseMenu_Screen_7`; existing BG3 extension recognizes it. | Use `menu_background.png` in added margins. Preserve the complete original 240 × 160 layout and all live map/UI content in the centered native canvas. No per-screen or per-region image. |
| Sword techniques / `pause_sword_techniques` | Screen 8 uses row 6, palette 205/group 126 and `PauseMenu_Screen_8`; existing BG3 extension recognizes it. | Use `menu_background.png` in added margins. Preserve the complete original 240 × 160 layout and all live map/UI content in the centered native canvas. No per-screen or per-region image. |
| Overworld map / `pause_map` | Screen 4 uses row 2, palette 185/group 94; `sub_080A6290` also loads group 0x81 and sets map offsets. | Use `menu_background.png` in added margins. Preserve the complete original 240 × 160 layout and all live map/UI content in the centered native canvas. No per-screen or per-region image. |
| Dungeon map / `pause_dungeon_map` | Screen 5 uses row 3, palette 184/group 92. `DrawDungeonMapActually` draws map data; its handler rotates eight palette colors every eight ticks. | Use `menu_background.png` in added margins. Preserve the complete original 240 × 160 layout and all live map/UI content in the centered native canvas. No per-screen or per-region image. |
| Map detail / `pause_map_detail` | Screen 6 uses row 4, palette 185/group 129; `sub_080A67C4` loads area palette 0xba + area and gfx 0x5f + area, with flag-dependent variants 0x70–0x74 and per-line BG control. | Use `menu_background.png` in added margins. Preserve the complete original 240 × 160 layout and all live map/UI content in the centered native canvas. No per-screen or per-region image. |
| Fast-travel selection / `fast_travel_map` | `SUBTASK_FASTTRAVEL`, `Subtask_FastTravel_0`, initializes screen 13 (row 7, palette 185/group 93) and uses shared overworld-map setup. | Use `menu_background.png` in added margins. Preserve the complete original 240 × 160 layout and all live map/UI content in the centered native canvas. No per-screen or per-region image. |
| Fusion map reveal / `map_hint` | `SUBTASK_MAPHINT`, `Subtask_MapHint_0`, initializes map screen 4, disables BG3, and copies `gPaletteBuffer[81]` into color zero. | Generate the border color sampled at native (120,159), #C8E090 in the verified EU export. Keep a solid `map_hint.png` reference swatch; do not load it as a runtime override. Preserve the original map/hint UI. |
| Local fusion map reveal / `local_map_hint` | `SUBTASK_LOCALMAPHINT`, `sub_080A6B04`, initializes map-detail screen 6 and chooses an area from the fusion event. | Use `menu_background.png` in added margins. Preserve the complete original 240 × 160 layout and all live map/UI content in the centered native canvas. No per-screen or per-region image. |
| Sleep confirmation / `sleep_confirmation` | Screen 9 uses row 9, palette 207/group 128. `sub_080A5BB8` explicitly sets `gMain.sleepStatus = SLEEP`. The TTS name “Save” in `sub_080A4DB8` does not describe this action correctly. | Use `menu_background.png` in added margins. Preserve the complete original 240 × 160 layout and all live map/UI content in the centered native canvas. No per-screen or per-region image. |
| Pause save confirmation/result/error / `save_confirmation` | Screens 10/11 share row 10/group 127. `PauseMenu_Screen_10` leads to `HandleSave`, `CreateDialogBox(8/9,...)`, and returns to quest status/items. EU rows 9/10 retain existing palettes (`paletteGroup = 0`). | Use `menu_background.png` in added margins. Preserve the complete original 240 × 160 layout and all live map/UI content in the centered native canvas. No per-screen or per-region image. |
| Game over / `game_over` | `TASK_GAMEOVER`, `GameOver_Init` calls `sub_08052010` in `src/subtask/subtaskAuxCutscene.c`: reset display, clear BG1/BG2 maps and tile zero, load palette 0xA/group 4, set color zero to black. `DrawGameOverText` draws moving OBJ letters. Later states enable popup BGs and save/continue choices. | The cleared black field is procedural, not a `game_over.png`. Keep letters and popup content above it. Register intro, save/error, and continue/quit phases separately for effect masks; all retain the procedural field and original centered native UI. |
| Credits image pages / `credits_01` … | `TASK_STAFFROLL`, state 1/menu type 1 loads a palette/gfx pair from `gUnk_08127CEC`. Its ROM stub is 20 bytes; `StaffrollGfxEntry` is two packed bytes, giving **10 source records**, not necessarily 10 distinct visible settings. | Nine nonzero source records reconstruct eight credits illustrations plus `ending_screen.png`. Retain native credits text, timing, and positions; use one file per distinct illustration. |
| Credits text-only/blank phases / `credits_text` | Staffroll initialization sets color zero black; menu type 6 clears both BG1/BG2 maps; menu type 2 writes text onto BG1 and animates its alpha. A page's underlying BG2 can persist until explicitly cleared. | Procedural black only when source/state proves the image is absent. Do not treat all credits as solid merely because initialization uses black. Respect text alpha versus full-screen fades, including mask `0xffff7fff`. |
| Post-credits save/result / `ending_save` | `StaffrollTask_State2` resets display, loads palette 10/group 4, clears tile zero, sets color zero black, then displays save choices/errors; state 3 resets the game after the fade. | Procedural black field with normal popup foreground. No separate PNG for that field; cover save accepted/declined/error and final fade. |
| Ending illustration / `ending_screen` | Ending world scripts under `data/scripts/cutscene/outro/` lead through `script_Npc4EMinishPortalClosing.inc` to `sub_0807FB94`, which selects `TASK_STAFFROLL`. There is no separately named ending task in the task enum. The end card is represented inside the data-backed staffroll pages. | Regional export resolves nine nonzero palette/gfx pairs plus a zero sentinel. The ninth illustration is the final stained-glass card, exported as `ending_screen.png`; original ending room cutscenes remain live-world rendering. |
| File-operation screens | `FileSelectState` in `src/fileselect.c` includes new file, language, options, view, copy, erase, start, and PC randomizer configuration. | Audit each state's setting, including copy/erase/save failures and prompts; share artwork where genuinely identical. Plain-color phases use generated fill. Native PC configuration overlays remain port UI rather than ROM background exports. |
The remaining pause indices are not additional proven user screens: 0/3 and
12–19 dispatch initialization/alias paths, screen 14 is used for menu exit,
and selector 15 resolves to overworld/dungeon map in `sub_080A50A0`. Follow their
actual destinations rather than generating twenty anonymous PNGs.

The figurine viewer exposes a procedural field: `FigurineMenu_080A4608`
copies palette entry 251 into color zero and clears BG3. Its frame, list,
figurine sprites, and description area are native foreground UI. Likewise,
`Subtask_MapHint_0` explicitly disables BG3 and sets color zero from entry 81.
These fields need generated color, not exports of the overlaid screen content.
For map hint, the restored `map_hint.png` is a deliberate solid-color reference:
sample native pixel (120,159), not the map interior or a resized image. The
verified unfaded EU border is uniformly RGB (200,224,144), hex #C8E090. All
240 pixels in its bottom row have that color. The reference contains no map/UI.
`sub_080A4D34` loads the same common menu decoration before menu-specific content
and health-dependent variants; `Subtask_KinstoneMenu` chooses seven variant maps
on top of a shared fusion setting. These are the source reasons to share files.

`TASK_DEBUG` is inaccessible during normal play and is not part of the artist
catalog. Its `src/debug.c` color-zero value 0x1144 can use the same procedural
mechanism if its viewport is extended. Room-based auxiliary/world-event,
portal, and beanstalk sequences remain excluded unless a specific phase is
verified to be a fixed presentation screen.

### Completeness and maintainability

Keep the task/subtask/pause-state audit above as the coverage checklist. The
current regional export verifies the menu setting, prologue pictures, and nine
credits/ending illustrations. Add a context only when a distinct functional
setting is demonstrated. Regional/area/save-state differences in original UI
do not create new override names. Regional ROM fixtures validate native
rendering and context classification, not palette compatibility of artist PNGs.

File stems are short lowercase ASCII `a-z0-9_`, avoid Windows reserved names,
and describe the setting. A single code catalog supplies the context-to-file
mapping and procedural rules for all ratio folders. Missing context coverage
must not be “fixed” by displaying a generic static image during live gameplay.
Temporary tracing can help audit ambiguous states, but it is opt-in and has no
per-frame hashing/filesystem work. No generated manifest is shipped or loaded.

## 6. Ordinary PNG contract and deterministic lookup

Artist-facing examples:

```text
assets/override/32/menu_background.png      # 240 x 160 native reference
assets/override/43/menu_background.png      # 256 x 192
assets/override/169/menu_background.png     # 285 x 160
assets/override/219/menu_background.png     # 374 x 160
assets/override/329/menu_background.png     # 569 x 160
assets/override/43/kinstone_background.png  # 256 x 192, all seven variants
```

Names are hardcoded once; ratios change only the directory and required pixel
size. There is no JSON or sidecar in any folder. Use the runtime asset root
(`./assets/override/...` under the existing launch convention), independently
of raw asset mods and `.pak` files. Preserve loose artist files during staging,
self-extraction cleanup, and application updates.

These are **normal, palette-free, full-color PNGs**. Artists can use any RGB
colors, ordinary RGBA transparency including fractional alpha, and their usual
image editor. No GBA palette matching, fixed color count, BGR555 conversion,
tile alignment, or palette metadata is required. Decode standard RGB, RGBA,
grayscale, grayscale+alpha, and indexed PNG encodings into RGBA8 as necessary;
indexed input is optional convenience, not a required palette format. Handle
standard PNG bit depths and interlacing with libpng's normal normalization.
The art is not quantized to GBA colors. The only content contract is the exact
logical canvas size for the selected aspect ratio, before supersampling:
240 × 160 for 3:2, 256 × 192 for 4:3, 285 × 160 for 16:9, 374 × 160 for
21:9, and 569 × 160 for 32:9. Validate both PNG dimensions against that
profile, not against a universal 256 × 192 size or the current window size.

Resolve the hardcoded context before lookup:

1. Live gameplay: no static image; render extended world tilemaps/effects.
2. Procedural fixed screen: generate its specified color with its effect rule;
   perform no PNG lookup. Boot logos always have generated white behind them.
3. Image-family screen: try exactly `<ratio>/<hardcoded_name>.png`. On missing,
   corrupt, wrong-size, or failed decode, use opaque black in the added margins.

Always load the original native screen normally. Only the margin source is
selected by the PNG lookup; no asset-group interception or background-layer
replacement is needed. Cache by `(shared image ID, ratio, renderer generation)`
so switching between inventory and map screens can reuse one decoded image.
Effect state belongs to the active context, not to the shared image cache.

Use the existing libpng dependency. Check signature/IHDR and exact dimensions
before image allocation; bound compressed input, arithmetic, and decoder
resource usage without imposing an artist-facing color restriction.
`png_set_user_limits` and chunk allocation/cache limits are documented in the
[official libpng manual](https://www.libpng.org/pub/png/libpng-manual.txt).
Handle libpng errors/longjmp without leaking buffers or crossing C++ destructors.
A failed expected image logs once and selects black. Never show a previous
screen's cached artwork after a context/ratio change or failed reload.

Decode only on image activation, ratio change, or explicit artist reload.
Cache missing/invalid results until invalidation. Do not stat/open/decode/hash,
allocate, or parse anything in the frame/scanline hot loops. Publish immutable
buffers before parallel rendering and release only after workers finish.

## 7. Contextual behind-image composition and procedural solids

Preferred implementation: one immutable screen-art descriptor with context ID,
pixel pointer, stride, canvas W/H, original content rectangle, explicit gap
coverage, and effect policy. It is port-owned and is not a fifth GBA BG layer
or a replacement for BG0–BG3. Only the selected image changes with screen
context; sample its pixels at fixed logical canvas coordinates with no camera
offset, wrapping, or animation. At 4:3 a centered native 240 × 160 screen occupies
(8,16)–(248,176), leaving four margin strips for a 256 × 192 image.

Keep the full image so artists can use its central area as an alignment guide,
but initially display only samples in the explicit added-area mask. Preserve
all pixels of the original screen, including opaque black/white fields, and
retain its foreground sprites, UI and animations above the contextual artwork.
A black pixel is not a hole. Neither palette index zero nor a transparent BG
tile proves the final screen is uncovered: another layer or the original
backdrop may own that pixel. Do not color-key the final frame or replace its
backdrop globally. Interior cutouts require a deliberate per-context mask and
are deferred unless a verified screen actually needs them.

This explicit mask is necessary because the current PPU always resolves a
backdrop color: putting a texture underneath its opaque final framebuffer would
hide the texture everywhere. Render the original content as today, then write
only the owned margin strips into the logical frame, before color correction,
persistence, filtering, upscaling, and presentation. Fixed-screen foreground
never expands into those margins: its layout and clipping remain native. This
uses the same conceptual order as drawing artwork behind the centered scene
without adding full-screen alpha buffers or replacing original hardware layers.

### Matching the current screen's effects

An image copied into the margins after the game's fade calculation would stay
bright. Give each eligible context an explicit effect policy that uses the
same live fade state as the original screen's decorative setting, applied
at full RGB precision to the external art. `mode1.c` tracks the composited
top/bottom layer
and consults BLDCNT target bits; arbitrary external pixels do not automatically
inherit these rules. A per-context policy identifies the original effect owner
(BG or backdrop) without making it the artist-facing image identity.

For each margin scanline, apply the scene's relevant brightness/fade state to
the static source exactly once. Use the frame's prepared register snapshots;
follow the bounded raster continuation rules in section 3 for new rows. Reuse
the PPU's fade/blend coefficients through a small RGB888 effect helper,
rather than inferring a fade from the final frame or applying unrelated SDL
alpha. Keep the original content untouched so it does not receive the fade twice. Force blank and
scene-wide black/white transitions cover the full logical canvas.

There are **two distinct effect stages** to account for. `src/fade.c`,
`FadeVBlank`/`Port_MakeFadeBuff256`, applies palette-bank-specific fade and
brightness transforms from `gPaletteBuffer` into live palette RAM before PPU
sampling; BLDCNT/BLDY/alpha/window effects occur later in rendering. Use the
context's selected palette-bank fade state (including `gFadeControl.mask`)
to derive full-precision effect coefficients, then apply the applicable renderer
effects once. This is engine-owned effect state, not a palette requirement
for the PNG. If matching the game brightness preference is required, evaluate
a full 8-bit transfer curve or interpolate its native curve without reducing
source colors to 32 levels; neutral brightness must preserve all source colors. A solid sourced from the already faded live palette
must skip the palette-fade transform and only receive applicable later effects.
Reading only BLDCNT or a global fade progress value would miss palette fades
or incorrectly fade excluded banks. Game over and credits exercise these cases.

For alpha fades or window/iris/wipe transitions, identify the actual scene-wide
transition and define its extension/mask in full-canvas coordinates, including
its second color/input where needed. Hardware BLDCNT blend mode alone does not
mean the whole screen is fading: the title uses alpha blending for its animated
layers, and that local animation should not pulse the static margins. Likewise,
a local window or mosaic on a foreground object should not distort unrelated
margin artwork. Extend screen-wide effects; retain local effects on their
original content. Record and verify the policy for every supported context.

Preserve full RGB888 values through external-art effects and composition.
At neutral effect settings an opaque PNG sample must retain its exact 8-bit
channels before the user's optional global color correction/filtering. For
fractional alpha, composite the PNG over the black margin base using ordinary
source-over arithmetic, then apply the screen-wide fade/brightness exactly
once. Full alpha is supported from the first implementation.

Port the relevant native effect coefficients to RGB888 arithmetic, not the
native color storage format. For example, a fade-to-white moves each channel
toward 255, and a fade-to-black toward zero; retain original effect timing,
mask selection, and rounding consistently. Do not run arbitrary PNG colors
through a 32-entry channel LUT or drop their low three bits. The original
center canvas still uses the untouched native renderer. External-art color
correction, persistence, and filtering then run once over the assembled frame.
Palette cycling inside the original UI continues there; only explicit
screen-wide recolor/fade policies affect the static margin image.

Splash: identify the solid white field from its source layer/palette, not from
the faded final frame. Extend that field over the full logical canvas in every
profile while centering the logo composition. Source the color from the same
live palette/backdrop state, with the same BG/backdrop blend target and per-line
brightness/window rules. If white belongs to a BG rather than palette-zero
backdrop, extend that BG's solid source; changing the global backdrop alone
would apply the wrong fade target. Forced blank and transition black must still
cover the entire logical canvas. Check intermediate fade values, not only white
and black endpoints. Outside the aspect-fitted presentation stage remains the
user's normal outer-window fill policy.

Solid detection happens on the decoded unfaded source, respecting opaque/
transparent coverage. A white layer beneath colored logo tiles is a solid
layer plus foreground, not a single solid composite. Store the procedural source/color rule in the hardcoded catalog, generate
on demand, and export no PNG for it. In particular, do not export the boot
logos as a background: the logos are native foreground over generated white. Never classify a background as
solid merely because a fade currently makes the whole framebuffer black.

For **every** eligible screen and substate, classify the decorative setting as
`solid`, `image`, or `none/live_world`. A solid setting always uses a generated
in-memory fill, with no PNG lookup, decode, or export, regardless of whether a
same-named file happens to exist. Store a color source (live palette entry or
explicit procedural color) and effect policy rather than a fixed RGB screenshot.
This includes black fields behind game-over text and post-credits save prompts,
colored map/menu fields when verified, and white splash fields. Foreground
text/icons/maps remain normal rendering and do not disqualify a solid setting.

Prefer a packed color plus row/span fill into the existing canvas; there is no
need to allocate a second full-size solid image. If a backend needs a texture,
use a reusable 1 × 1 texture or the existing canvas upload, with the same resolved
color/effect state. At a solid-to-image or image-to-world transition, publish the
new descriptor and release the outgoing PNG cache; no stale artwork may persist.

Retire the duplicated ImGui side-art path once its supported scenes use the
shared contextual artwork, required procedural colors, or missing-image black.
Do not preserve the old repeating/blurred menu-art fallback for missing images.
Prevent both paths from painting the same background. Required
background support must also work with ImGui hidden, nearest presentation,
software renderer, SDL_GPU, filters, and LCD persistence. Audit GPU raster
capabilities; use the same margin composer after either PPU output path. If a
GPU raster path does not expose the effect/foreground-coverage data required
for a particular context, use software PPU rendering for that context until
equivalent composition exists. SDL_GPU presentation can still present the
result. Never activate the behind-image descriptor for gameplay, including
narrow rooms, dialogue, transitions, boss arenas, and weather/parallax scenes.

## 8. Shared native references and artist workflow

`tools/export_screen_backgrounds.py` is the standalone stdlib-only exporter.
It validates a local USA/EU ROM and reconstructs screen artwork from tiles,
maps, and source palettes; this source decoding does not constrain later PNG
artwork. It uses retail offsets from `port/port_rom.c` and regional symbol
segments/patches from `port/port_rom_stubs.c`, avoiding unrelocated passthrough
data for staffroll tables.

The public exports are the **19 shared image names** in section 5: title,
file select, common menu backdrop, common fusion setting, six prologue images,
eight credits images, and ending. An additional `map_hint.png` is a **solid reference swatch**, bringing the
local folder to 20 PNGs. It is not an extra runtime image family. No boot logo
image, per-screen menu frame, map-detail variant, duplicate fusion image,
health-specific margin image, figurine UI screenshot, map-hint UI screenshot,
or JSON is produced.

`menu_background.png` is the clean hat-pattern setting, not an arbitrarily
selected inventory/map screenshot. Original slots, map data, stone frames,
text, prompts, and other screen-specific UI continue rendering on the native
240 × 160 foreground. `kinstone_background.png` is the single representative
stone-table setting shared by all fusion variants. Boot/logo, game-over,
cleared-credits, post-credits-save, figurine-field, and standalone-map-hint
solid cases use code-owned procedural rules. The `map_hint.png` swatch is the
explicit artist-reference exception: it records the bottom-center border color,
while the runtime generates that same color without opening a PNG. In native
unfaded EU rendering this is #C8E090. During fades, follow the corresponding
live border/field state; do not display a permanently unfaded swatch or apply
the native fade twice. Other ratio folders need no copy of this reference.

Artists copy the relevant ordinary PNG into the desired ratio folder and
resize/repaint it to that ratio's logical pixel dimensions. Its central
240 × 160 area is an alignment reference covered by the original native
screen. In 4:3, added artwork occupies the margins around (8,16)–(248,176).
No palettes or technical metadata need editing. If a file is not supplied,
the corresponding margins are solid black with the same screen-wide effects.

The folder README describes these few shared roles in plain language.
`assets/override/background_contact_sheet.png` is an overview outside the
runtime lookup folders. Do not generate an inventory sidecar in any ratio.
The exporter preserves existing outputs unless `--overwrite` is explicit;
normal runtime/packaging must never overwrite artist edits. Only the generated
`assets/override/32/` folder and `assets/override/background_contact_sheet.png`
remain Git-ignored. Other override folders, including `assets/override/43/`,
are custom source artwork and must be trackable. Code and documentation are reviewable
source changes; runtime loading/composition remains a future implementation.

## 9. Performance and memory budget

Base pixel work grows from 38,400 to 49,152 pixels at 4:3: **28% more pixels**,
not a forecast of 28% more total frame time. Nearest presentation with internal
scale 1 should be the initial handheld measurement profile. Do not force higher
scales or filters just to make 4:3 work.

| Buffer/work | 256 × 192 estimate | Policy |
| --- | --- | --- |
| One RGBA8 canvas/layer | 192 KiB | Original 240 × 160 is 150 KiB. |
| Current and incoming contextual RGBA8 images | 384 KiB | At most one per context; coexist only during a scene transition. |
| One 4× RGBA8 canvas | 3 MiB | Allocate/touch only for selected upscale path. |
| One 10× RGBA8 canvas | 18.75 MiB | Existing worst-case scaling pattern; avoid default activation. |
| 569 × 192 × 10² RGBA8 maximum-capacity pool | About 41.7 MiB | Expanding compile-time capacity can enlarge BSS even though only one profile is active. Audit before changing macros. |
| Additional full IO snapshot rows | 32 KiB per 32 rows | Prefer shared stable-state rows when no raster effect needs unique state. |

`port_ppu.cpp` already reserves maximum-scale BSS plus 2×/4× xBRZ pools.
Linux lazy backing helps untouched pages, but does not justify multiplying
maximum-capacity pools for every image/backend/screen. Track virtual size,
RSS/PSS, driver allocations, and actual touched memory separately. Keep a
single reusable upscale workspace or allocate exact profile/scale buffers at
settings/scene boundaries, outside the render hot path. Avoid an unrelated
allocator rewrite unless measurement shows the existing pools exceed budget.

Proposed new-feature budget: at most **8 MiB steady / 16 MiB transition peak**
for contextual image cache, lookup metadata, scratch, and additional geometry state,
excluding already-selected optional supersampling/filter workspaces. Enforce
the limit across current/incoming images and pending decode, with no cache of all PNGs,
ratios, regions, or languages. A current and incoming scene may coexist briefly;
evict outgoing state immediately after publication. Size the cache by bytes,
not just image count. Decode directly into final storage where practical.

Reserve an initial process-level target below **256 MiB PSS** for the basic
scale-1 handheld profile as a measurement goal, not a claim about current
usage. Record the baseline first; if baseline already exceeds it, identify
actual contributors rather than increasing the feature budget toward 1 GB.
OS/driver/audio/future-screen reserves are not available PNG cache space.
Keep per-view source ownership separable from the game's global simulation;
do not allocate a second screen or duplicate asset packs in this work.

Measure on the RG build: cold load, scene switches, ratio switches, file select,
busy field, dungeon, credits, raster effects; frame-time p50/p95/p99, PPU time,
PNG decode stall, RSS/PSS peak, GPU/texture memory where exposed, and allocation/
filesystem activity after warm-up. Target the existing ~59.7275 Hz cadence
(~16.74 ms per frame). New override sampling should add no disk traffic or
heap work after scene activation. Investigate an added steady overhead over
roughly 0.5 ms at fixed 256 × 192 before accepting it; this is a proposed gate,
not measured evidence.

The RG launcher currently sets `OMP_NUM_THREADS` to 4 by default. Benchmark
1/2/4 threads; more threads and larger buffers are not automatic improvements
on a small shared-memory device. Existing menu rectangle submission and repeated
content-width scanning are measurement candidates, but optimize only with data.

## 10. Implementation sequence and review boundaries

1. **Baseline and catalog:** capture current native/wide scenes; identify every
   eligible fixed-screen context and explicitly exclude live-world rendering.
   Finalize hardcoded shared-image IDs, ratio dimensions, export recipes,
   effect policies, and native/Console-Parity semantics. Save measured RG baseline.
2. **Geometry/settings foundation:** add profile snapshot, height/pitch contract,
   config migration, remove the toggle and its indirect gates, update build
   capacities, callers, snapshot formats, and native pinning in tools. Verify
   native parity before enabling taller world rendering.
3. **4:3 world:** implement vertical geometry, bounded raster-state extension,
   two-axis camera/streaming, culling/OAM handling, room apertures, transitions.
   Include wide OBJ-coordinate handling before claiming full 32:9 support.
4. **UI layout:** keep every fixed-screen layout native and apply one canvas
   centering transform. Apply corner HUD/bottom dialogue transforms only in live
   gameplay, including choices/portraits/arrows, banners, and HD text.
5. **Contextual behind-image composition:** introduce a screen descriptor, gap
   masks, shared effect math, and procedural solids. Make splash white/fades
   correct in every profile; migrate menu art from the ImGui extension. Prove
   original content remains intact and gameplay never acquires a static image.
6. **Artwork loader:** add strict-size PNG loading, semantic context resolution,
   shared family filenames, full-color PNG decode, black fallback, bounded cache,
   and packaging rules. Leave raw asset loading and normal BG rendering intact.
7. **Shared native export:** keep the hardcoded exporter synchronized with the
   runtime names; verify the consolidated `32` folder, README and contact sheet.
   No JSON, native UI screenshots, or redundant ratio metadata is generated.
8. **QA and budgets:** finish all regional/background/effect coverage, run device
   profiling and staging checks, document supported sizes and artist workflow.

Keep these independently reviewable. Most new code should live under `port/`
and `tools/`. Engine changes should be a few `PC_PORT` hooks and viewport helper
calls; do not alter packed saves/entities, generated ROM tables, GBA constants
globally, or asset formats unrelated to this feature. Edit vendored PPU in place
and use its parity gate. Define one small port-owned code table for shared
image names and procedural rules, with no runtime manifest dependency.
The margin composer should consume pixel data,
geometry, coverage, and resolved effect state; the context resolver owns game
task IDs. No game-state or JSON parsing belongs in scanline composition.

The behind-image approach is the first implementation, rather than a mandatory
intermediate step toward full artwork replacement. Replacing a title's central
artwork, exposing its hardware layers as separate PNGs, or adapting gameplay
parallax to static PNGs is not part of this plan. Consider full replacement only
as a separately requested extension if a concrete screen cannot be served by
the contextual image plus its normally rendered original content.

## 11. Verification and acceptance

Documentation-only validation for this task: review source references and
Markdown links, run `git diff --check`, and confirm only this plan changed.
No build or game run is needed to validate this documentation change.

Implementation verification must include:

- `xmake build -y tmc_pc` and a bounded headless smoke run reaching
  `Port layer initialized. Entering AgbMain...`; `xmake test` is not applicable.
  Run full `build.py --usa`/`--eur` packaging checks for asset staging changes,
  and build the RG package. Do not deploy without an implementation task
  authorizing device work.
- `tools/ppu_parity_check.sh` in native/parity mode with overrides disabled.
  Keep old golden pixels; add deliberate enhanced-profile baselines separately.
  Broaden local corpus for gameplay because the committed parity corpus is tiny.
- Existing widescreen engine/scroll/PPU regression tools, menu-background tests,
  runtime config checks, and `tools/render_scene_check.py` profiles. Extend
  existing focused harnesses with independent geometry/layout assertions.
- 4:3 captures **256 × 192** at scale 1 on software and SDL_GPU presentation.
  Verify unique world content, no global stretch, stable room fallback, HUD
  corners, bottom dialogue from both top/bottom requests, opening/closing,
  choices, long localized text, hiding HUD, and inventory transitions.
- For every fixed-screen family, compare the centered 240 × 160 crop against
  the original native screen at equivalent state/frame. Require identical
  layout, cursors, map position, line wrapping, popup placement and animation.
  Verify the offset is exactly (8,16) in 4:3 and no native widget enters the
  margins. Live-gameplay HUD/dialogue tests are a separate context.
- Change between inventory, all map-detail variants and save/sleep without
  re-decoding `menu_background.png`; change between all seven fusion variants
  without re-decoding `kinstone_background.png`. Effects remain context-specific.
- Every profile in square, tall, native, and wide windows; aspect changes while
  a message/fade/menu is active; unsupported capacity behavior; no 1-frame
  camera/render mismatch. Explicit 32:9 testing beyond X=511.
- Native export reconstruction checks for all named screen settings and
  regions/languages; native output unchanged with a fully covered behind-image;
  extended-profile fixtures proving visible gap composition. Check correct
  filenames and directory keys; missing/wrong-size/corrupt/truncated PNGs,
  large IHDR/chunks, indexed/grayscale conversion, full fractional alpha,
  reload, and repeated ratio/scene changes. Verify arbitrary non-GBA colors
  survive neutral effects exactly, and every invalid/missing expected image
  produces black rather than a previous image or fallback from another ratio.
- Splash and solid fields at fade endpoints **and intermediate values**; title
  sword/rays/foreground, palette animation, alpha/window/mosaic, forced blank,
  barrel affine/HDMA, transitions, credits, and parallax overlays.
- Context transitions title → file select → gameplay → pause menu → gameplay
  and region/language/ratio changes. A conspicuous test image for every context
  must never appear in overworld, dungeon, boss, Minish-path, dialogue-over-world,
  or gameplay-transition captures. Inspect data beyond native edges to verify
  extended gameplay tiles remain real world content. Intentional black areas
  inside a fixed screen must remain opaque; screen-wide fades affect both
  original content and margins once, while local title alpha effects leave
  static artwork stable. Missing art uses only solid-black fallback.
- All functional screen families in the additional audit: inventory, quest
  status, pieces/techniques, all three map views, fast travel, both hint paths,
  sleep/resume, both pause save entry routes and errors, game-over intro and
  choices, every distinct credits setting, text-only/cleared credits phases,
  the ending card, and post-credits save/decline/error. Test solid contexts
  with deliberately conspicuous same-named PNGs present: they must still render
  generated fill and perform no image decode. Verify both palette-based fades
  (including masked banks) and subsequent renderer effects at intermediate values.
- Nearest, internal scale, xBRZ, color correction, persistence, filter/preset,
  screenshots, HD dialogue, voxel mode, and ImGui-hidden behavior. Either full
  support or a documented safe renderer fallback for each optional path.
- USA/EU/JP and any other enabled region validated against available local
  ROMs. Record unavailable fixtures as gaps; the context catalog must not silently
  infer equivalence from USA or asset-backed loading alone.
- Device memory and frame-time targets, no per-frame override I/O/allocation,
  bounded cache growth over a long scene-switch run, and room for future views.

Remaining implementation research gates are explicit: complete the visual
catalog and functional screen names; verify vertical tile residency for each
streaming manager; classify scene-wide versus local effects and margin coverage;
establish wide signed OBJ metadata;
and measure the device baseline. These are required early work packages, not
reasons to weaken the requested 4:3 or all-background acceptance criteria.


## 12. Current local exports (2026-10-24)

The consolidated EU/English references contain **20 ordinary RGB PNGs**
(19 artwork references plus the restored `map_hint.png` color swatch), each
240 × 160, in `assets/override/32/`. They use shared, hardcoded filenames with
no `inventory.json`. `menu_background.png` replaces all per-screen map/menu
exports and the health-dependent backdrop copies; `kinstone_background.png`
replaces all seven fusion exports. Boot logos have no exported background:
the future runtime generates white and keeps the original logos unchanged.
The distinct title/file-select, prologue, credits and ending artwork remains.
`map_hint.png` is uniformly #C8E090 (RGB 200,224,144), sampled from the original
map-hint screen’s bottom-center border pixel (120,159); it is reference-only.
The planned runtime generates that field color without loading the file.

Example regeneration (use an actual local ROM path):

```sh
python3 tools/export_screen_backgrounds.py --rom /path/to/local/rom.gba
python3 tools/tests/export_screen_backgrounds.py
```

Existing files require explicit `--overwrite`. `README.md` explains the shared
roles; the overview is `assets/override/background_contact_sheet.png`. Exports
in `32/` and the contact sheet remain Git-ignored by `assets/.gitignore`;
custom ratio folders such as `43/` are trackable. Runtime PNG lookup, RGB888 effects,
fixed-screen centering, black fallback, and procedural fields are specified
here but are not implemented by the export/documentation work.
