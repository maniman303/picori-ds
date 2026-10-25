# Menu background family revision and completed-task review

Revision date: 2026-10-25. This records the requested pieces, techniques and
top-left-title split in the [plan](aspect-ratio-and-background-overrides-plan.md).
It records implementation research and a review of all **27 previously checked
tasks**. The catalog/exporter migration has since been implemented; the
foreground title-frame fix remains open (see implementation status below).

## Revised artist contract

Each row has separate `_normal.png`, `_damaged.png` and `_critical.png` files:

| Family | Filename stem | Functional setting |
| --- | --- | --- |
| Plain menu | `menu_background` | Save/sleep prompts and other verified menu settings without the top-left title frame |
| Titled menu | `menu_titled_background` | Items, quest status, overworld/dungeon/detail maps when the top-left title frame is installed |
| Kinstone pieces | `kinstone_pieces_background` | Pause screen 7 and its own panel/heading arrangement |
| Sword techniques | `sword_techniques_background` | Pause screen 8 and its own panel/heading arrangement |
| Kinstone fusion | `kinstone_background` | Existing fusion setting, shared across its seven layouts; distinct from pieces |

There are **15 menu family/health files**, up from six. The entire catalog target
is **32 artwork names** (previously 23), with **33 native reference PNGs** when
the standalone `map_hint.png` swatch is included. Every ratio uses the same
names and the existing exact dimensions. No additional swatch, manifest,
per-language name or per-map name is introduced.

Keep the six existing menu/fusion filenames and their artist bytes. Add nine
names for titled menus, pieces and techniques; do not rename or copy the old
general-menu artwork automatically. A missing new-family file uses the existing
effect-aware black fallback, never a plain-menu file or another health state.
The code/exporter now implement the 32-name contract. The task list records
the completed migration and keeps runtime composition/title-frame work open.

## Source and pixel research

The source review follows
[the pause dispatch/configuration](../src/data/figurineMenuData.c),
[screen installation](../src/menu/figurineMenu.c),
[pause drawing/transitions](../src/menu/pauseMenu.c),
[detail-map loading](../src/menu/pauseMenuScreen6.c), and
[the legacy extension](../port/port_menu_background.c).
Local evidence includes the prior 106-case EU/English native audit and a fresh
ROM decode of the individual menu layers and title sprite pieces. Other
regions and languages have not been verified by this research.

| Setting | Verified source | Consequence |
| --- | --- | --- |
| Installed health | `sub_080A4D34` loads groups `0x56`/`0x57`/`0x58`, palette `0xb5`, before screen-specific content. The 106-case audit proves retention after health changes. | Keep this installed health separately from the active family. Changing family does not re-evaluate health. |
| Items / quest | Screens 1/2 use configuration rows 0/1: groups 90/91, palettes 182/183, BG2 control `0x1d03`. | Use the titled family while the top-left heading is installed. Native inventory slots and quest contents remain foreground. |
| Map screens | Screens 4/5/6 use rows 2/3/4; groups 94/92/129, palettes 185/184/185. Detail maps further select `0x5f`–`0x74` and per-line BG3 controls. | Ordinary pause maps use the titled family. Map variants and map contents do not create artwork names. |
| Pieces | Screen 7, row 5, group 125, palette 204; BG1 `0x1c03`, BG2 `0x1d03`. | Separate pieces family. BG2 contains the bag icon and a substantially different panel layout. |
| Techniques | Screen 8, row 6, group 126, palette 205; BG1 `0x1c02`, BG2 `0x1d03`. | Separate techniques family. BG2 contains the scroll icon and its distinct panel layout. |
| Heading pose | `sub_080A5128` draws frame 0 of sprite `0x1fa` on EU (`0x1fb` otherwise). Ordinary screens use `(64, 16 - 2 * unk16)`; screens 7/8 use `(96,24)`; screens 9/10/11 return without drawing it. | Do not infer a titled screen from BG3 or `TASK_GAME`. Track the installed heading layout and its animated pose. Pieces/techniques are not top-left-title variants. |
| Heading source | The EU frame has two 64 × 32 pieces, offsets `(-64,-16)` and `(0,-16)`, using tile/palette words `0x5040`/`0x5060` before command additions. Items/quest groups write OBJ VRAM at `0x6010800`; the BG2-only reconstruction omits the blue heading entirely. | The clipped/open edge is a foreground OBJ-frame problem, not proof that a different BG3 color or PNG alone can repair it. The full frame bounds start at native `(0,0)` in the stable pose. |
| Save / sleep | Screens 9/10/11 have no heading from `sub_080A5128`. EU configuration rows retain the previous palette. | Switch to the plain family after native prompt installation, keeping the installed health and the real entry-route palette. |
| Fast travel / hints | Fast travel runs a different subtask and does not call the pause heading helper. The two hint paths override or disable the decorative BG3. | Fast travel stays provisionally plain pending its native capture. Both verified hint fields remain procedural; do not infer a title/family from reuse of a map configuration row. |

The independent BG2 reconstruction confirms the pieces/techniques frame
differences. A BG3-only export contains their shared health decoration but
cannot describe those foreground geometries. Likewise, wrapping the BG2
tilemap into negative coordinates does not recover the title: it is drawn as
OBJ, and the inspected off-edge map cells are empty. Do not enable generic
GBA tile wrapping or move the native composition to conceal the cut edge.

Research artifacts are local under `.tmp/menu-family-research/`:
`layers-and-header-wrap.png`, `tile-observations.json` and
`header-sprite-pieces.json`. They are evidence, not approved artist references
or updated runtime assets.

## Implementation route

1. Extend the C catalog by appending nine image contexts; preserve existing enum
   values and filenames. Separate semantic family from installed health in the
   future state adapter. Update both C/C++ contracts and exporter agreement to
   the 32-name target, retaining all procedural/no-source/parity cases.
2. Publish `(family, installed health, heading style/pose)` after the incoming
   native screen is installed. Pause variant 4 calls `sub_080A4DB8` between
   outgoing and incoming fades; retain the outgoing descriptor until that
   boundary. A family change publishes a new descriptor even if viewport
   geometry and installed health are unchanged. Viewport geometry generation
   remains a geometry contract, not an artwork identity.
3. Export clean, unfaded setting/frame references for all five families and
   three health states. Pieces/techniques recipes must combine groups 125/126
   and palettes 204/205 with the installed decoration, selecting only verified
   static frame layers. Exclude live items, counters, cursor and localized
   heading lettering. The titled reference needs a blank heading alignment
   guide obtained from OBJ-frame geometry, not a captured Items/Map label.
   Do not assume every BG2 or OBJ pixel is decorative; audit the masks first.
4. Implement a bounded foreground completion for the top-left title frame in
   added margins. Use verified native border/corner pixels or an explicitly
   reconstructed frame cap; keep all native pixels, label placement and native
   content centering unchanged. The separate titled PNG supplies the matching
   setting, but renaming a background cannot repair an opaque native OBJ edge.
   The cap must work with missing/invalid PNGs as well as supplied artwork.
5. Clip completion strictly to available added-area pixels. In 4:3 anchor it to
   native origin `(8,16)`; wide 160-row profiles may have only side margins.
   Follow the native heading's vertical animation; a stationary cap during
   `unk16` transitions would leave a floating fragment. Resolve OBJ palette,
   alpha/window/fade ownership before applying effects, and avoid double fades.
   Native and Console-Parity output remain byte-identical with no completion.
6. Render the cap above artwork through the common compositor. Prefer bounded
   spans/native sprite data into the existing canvas, without a second
   full-frame layer or extra per-frame allocation. Preserve intentional black,
   software/GPU presentation, filters, persistence and screenshots. Keep
   unsupported raster/effect cases gated until their coverage is verified.
7. Cache by family/image ID plus installed health, ratio and renderer
   generation. Items ↔ quest ↔ pause maps retain titled artwork; save/sleep,
   pieces, techniques and fusion are distinct keys. Their switches may require
   a decode; they must not reuse a stale image. Keep the current/incoming byte
   budgets and evict outgoing state as already specified.

The exact animated cap pixels/mask and compositor integration remain pending.
Clean export recipes have since been implemented as recorded below. This
research identifies the frame source and constraints;
it does not claim a finished seam fix or regional equivalence.

## Review of every previously completed task

IDs below follow checked-task order before this revision. **Reopen** means the
current implementation no longer satisfies the revised contract. **Extend**
retains completed evidence but requires targeted follow-up. **Retain** means
the implementation contract does not need changing for this request; ordinary
checks must still run when subsequent code changes land.

| ID | Previously completed task | Decision and concrete follow-up |
| --- | --- | --- |
| R01 | C-compatible 23-name catalog | **Reopen.** `port_screen_background.{h,c}` needs nine appended contexts, five menu families and 32 artwork names. Preserve old IDs and artist filenames. |
| R02 | Image/live/unknown/solid/parity source policy | **Retain.** The new families are IMAGE sources; hint/logo/black/live rules do not change. Apply the same fail-closed/parity tests to new IDs in R03. |
| R03 | Catalog/exporter filename agreement | **Reopen.** `tools/tests/screen_background.c`, `screen_background_regression_test.py` and `tests/export_screen_backgrounds.py` currently require 23 names/six menu combinations. Require 32/15 and reject cross-family aliases. `tools/menu_background_test.c` remains a legacy BG3 decoder check, not new-family acceptance. |
| R04 | Five profile sizes, native centering and capacity rejection | **Retain.** Sizes, directory keys, ceiling widths and floor centering are unchanged. Complete a frame in margins, not by changing the native origin or PNG size. |
| R05 | RG build / ROM-free baseline checks | **Retain.** Baseline evidence remains valid. Rebuild/smoke and rerun relevant gates when implementation changes, rather than relabel old results as new-family support. |
| R06 | Native/wide named captures and separate goldens | **Extend.** Keep existing captures/goldens. Add all new families, title-cap seams, native-crop identity and family switches to enhanced captures; never overwrite native goldens to accept a cap regression. |
| R07 | File/prologue/credits/hint state-audit routes | **Extend.** Existing routes and local-hint correction remain valid. Add heading/family observations to the audit schema; keep unrelated routes and procedural hints unchanged. |
| R08 | 106-case pause health/map audit and inventory reference pixels | **Extend.** Thresholds, ten native decoration loads, 22 recipes and 6,240 pixel comparisons remain evidence. Add expected family sequence, pieces/techniques reference masks, title pose/cap and same-health cross-family transitions. Ten native loads are not a PNG decode budget. |
| R09 | Appended 4:3 enum / profile availability | **Retain.** No aspect enum change or profile enablement is justified. |
| R10 | Base geometry snapshot / simulation-boundary publication | **Retain.** Family changes use their own descriptor publication; do not alter canvas geometry or force geometry generations for artwork switches. |
| R11 | Scene capabilities and world aperture snapshot | **Retain.** Menu family classification belongs to the fixed-screen adapter, not world room caps. Live-world exclusions remain mandatory. |
| R12 | Snapshot-based width wrappers / resize behavior | **Retain.** Window resizing remains presentation-only; it does not change family or trigger image decode. |
| R13 | Removal of widescreen toggle / explicit profiles in tools | **Retain.** No toggle, JSON migration or width override is reintroduced. New captures must select explicit profiles. |
| R14 | Bounded PPU visible height / N64 native guard | **Retain.** Native frame hardware timing and 192-row capacity do not depend on image count or menu family. |
| R15 | PPU2 captures and PPU1 readers | **Retain.** Raw PPU serialization needs no version change. PPU2 alone does not record contextual PNGs/title completion; verify composed pixels with scene captures. |
| R16 | Height propagation through software, presentation, screenshots and OBJ supersampling | **Extend.** Keep the height plumbing. Audit the new cap through both presenters, screenshots, scaling/persistence and optional raster fallbacks; test signed coordinates outside the native rectangle. |
| R17 | Height scratch/storage audit | **Retain.** Family count needs no extra IO rows, taller affine scratch or enlarged filter pools. |
| R18 | Presentation allocation/path audit | **Extend.** Existing 569/192 formulas remain valid. Add bounded cap/descriptor storage and ensure 32 names do not become 32 resident images. Recheck optional-path behavior for the new foreground composition. |
| R19 | Build/tool capacity synchronization / native parity / N64 | **Retain.** Capacity and build flags remain unchanged. Native golden checks still apply after implementation; packaging must preserve new and existing custom files. |
| R20 | Hardware/render-row separation and HDMA bounds | **Retain.** Do not extend hardware callbacks to draw the cap. Its window/effect handling is additional compositor work. |
| R21 | Shared two-axis camera rest / vertical pacing | **Retain.** Fixed-screen artwork and heading completion do not affect world cameras. |
| R22 | Normal map row-count / seam plumbing | **Retain.** World tile streaming is independent of the menu map illustration/family. |
| R23 | Vertical streaming / manager audit | **Retain.** No new world rows or graphics-residency claim follows from this split. |
| R24 | Shake-aware map coverage and wrap classification | **Retain.** Do not use world shake/wrap rules as a way to extend a fixed-screen title. |
| R25 | Signed per-slot OBJ coordinates and lifetime | **Extend.** Preserve the existing metadata contract. Add tests for the actual direct title frame, its animated pose and clipping at native versus canvas edges. Avoid changing packed OAM or exposing parked sprites. |
| R26 | Entity/rectangle culling and generic piece clipping | **Retain.** World culling remains unchanged. Any frame-cap permission must be scoped to that fixed UI composition, not a global relaxed clip. |
| R27 | ROM-free exporter/catalog agreement gate | **Reopen.** Expand the existing gate and recipe fixtures to all five family/health sets; a green old 23-name gate does not certify the new contract. |

Result: **3 reopened, 6 requiring extensions, 18 retained**. Reopened catalog/
agreement tasks and explicit follow-ups are recorded in the task list. Historical
verification notes keep their original counts and are not retroactively treated
as 32-name or completed-title-frame evidence.

## Validation for this planning change

Inspect source and the local decoded evidence; check Markdown links, review all
27 task mappings and run `git diff --check`. No runtime code, exporter, artist
PNG, golden, build configuration or packaging is changed by this revision.


## Implementation status after the review

R01/R03/R27 are now updated: appended catalog IDs, an installed-health/family
lookup, 32-name agreement, 15 recipe combinations and safe missing-only export.
The exporter ran on EU/English, adding nine references and preserving existing
PNG hashes. Clean pieces/techniques BG2 layers match the earlier native lower
panels exactly; the blank title uses native OBJ pixels with lettering removed.
The exact animated margin-cap mask, renderer integration and new-family runtime
publication/loading remain pending. The decisions table above records the
review at revision time rather than claiming all follow-ups are implemented.
