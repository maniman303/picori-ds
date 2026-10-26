# Background context/state audit

The [menu-family revision](menu-background-family-revision.md) supersedes the
original shared-menu classification below. The catalog/exporter now have 33 artwork names (including one fixed fast-travel image)
and five menu families, each with three health variants. This audit's native
captures, installed-health observations and pixel matches remain evidence;
they do not verify game-state publication, loading or title-frame completion.
Clean reference recipes and the pure catalog lookup have since been added;
the family/heading audit extension is recorded below; runtime publication,
loading and title-frame completion remain open.

This audit supports the first open visual-catalog gate in the
[task list](aspect-ratio-and-background-overrides-tasks.md). It records native
screen evidence before runtime context selection or margin composition exists.
The full visual catalog remains open: the routes below cover the previously
missing file-operation, prologue and retained-credits states, rather than every
functional menu, health variant, error path and region.

Current remaining scope is tracked in the
[closure checklist](background-context-closure-checklist.md). It supersedes the
historical remaining-coverage summaries below.

## Reproduce

Build the current configuration with `repro_harness=y`. For the RG cross-build,
retain its original architecture, toolchain, dependency and capacity options
when configuring; restore `repro_harness=n` afterward. The capture driver is
absent from normal release builds. With Pillow installed:

```sh
python3 tools/capture_background_audit.py \
  --binary /path/to/repro-enabled/tmc_pc \
  --rom /path/to/local/rom.gba \
  --output .tmp/background-state-audit
```

For an ARM64 binary on a desktop, `--binary` can name an executable wrapper
that runs `qemu-aarch64 -L /usr/aarch64-linux-gnu /absolute/path/to/tmc_pc "$@"`.
The reported executable hash then identifies the wrapper; retain the actual
binary separately when comparing runs. `--scenario credits`, `prologue`,
`intro`, `files`, `local_hint`, `menus`, `travel`, `figurines`, `hints` or
`dungeons` restricts the run. The EU-only `travel_actions` route adds native
confirmation/decline and arrival coverage. Repeat the flag to select
several. `--menu-baseline /path/to/old/menus` compares all 212 ready/held
checkpoints from the original 106-case matrix without modifying that baseline.
`--language` uses the public language preference (1 is English), not
the EU save-header slot. The game still validates the supplied ROM.

Each scenario gets a fresh temporary runtime/config/save directory. Existing
output directories are rejected, preserving earlier evidence and preventing
stale captures from satisfying a failed run. Native geometry, software
presentation, scale 1 and neutral colors are explicit. ImGui menu art,
HD dialogue, persistence, autosaving and render decoupling are disabled. No
reference image or golden hash is approved or rewritten.

The driver stages a synthetic in-memory save after file-screen initialization.
Credits enter the real staffroll task; the two story routes enter the real
auxiliary cutscene subtasks. This verifies those rendering sequences, not their
normal quest prerequisites. The file route uses native input for options/back,
view/copy/cancel, view/erase/cancel, name-entry/cancel and EU language selection.
It does not commit copy/erase operations or inject save failures. The local-hint
route uses the default fusion-event fixture; other event/area variants remain
to be reviewed.

`trace.jsonl` distinguishes installation/clear events from commands and records
task/UI/menu state, installed illustration, fade mask/progress and blend
controls at each simulation boundary. PNGs sample every 60 frames, every eight
frames during fades, and two ticks after events to include VBlank upload and
presentation. Engine state may lead the completed framebuffer by one tick;
do not interpret an event's immediate screenshot as its uploaded pixels.
The trace keeps every state boundary; periodic PNGs do not cover every frame.

`summary.json` records capture hashes and required route coverage. Contact
sheets select examples by task/UI/menu/illustration/fade state; inspect the
individual PNGs and adjacent trace rows for transition details. Black frames
are valid observations. Missing/incorrect-size captures and incomplete routes
fail the run. The local-hint check independently requires both exposed lower
corners to match native `0x475f` with neutral colors.

## Installed-state findings

| Context | Installation/lifetime evidence | Source policy |
| --- | --- | --- |
| Boot/title/file screens | `TitleTask` installs the logo field, then the title forest; `HandleFileScreenEnter` installs file art. New, options, language, view, copy and erase redraw foreground UI while retaining the file setting. | Logo field, title image and file-select image remain separate contexts. |
| Opening story | Auxiliary cutscene 1 dispatches `sub_080536D4`; `gUnk_080FCCFC` loads page indices 0–4. `sub_08053974` resets the display and returns to live scene content. | Five of the six shared prologue images; invalidate at world handoff. |
| Sanctuary retelling | Auxiliary cutscene 3 dispatches `sub_0805373C`; `gUnk_080FCD40` loads indices 0–5. `sub_080539F4` resets the display after the sixth page. | Six shared prologue images, followed by a cleared field. The sixth image cannot be verified by capturing only the opening. |
| Credits artwork | `StaffrollTask_State1MenuType1` loads the selected entry from `gUnk_08127CEC` only after the outgoing fade is inactive. | Entries 0–7 select `credits_01.png`–`credits_08.png`; entry 8 selects `ending_screen.png`. |
| Credits text/waits/fades | `sub_080A3954` overwrites `gfxEntry` for each command, including text. Type 2 clears/redraws BG1 text and changes its alpha; it leaves the BG2 illustration installed. Types 0/3/4/5 also do not install another illustration. | Hold the last installed image across commands and fades. Never reinterpret a text index as an illustration ID or classify text commands as cleared credits. |
| Cleared credits | Type 6 waits for the fade, then explicitly clears BG1 and BG2. | Generated black after the clear; retain the outgoing image until then. |
| Post-credits save | State 2 resets the display, installs prompt graphics and explicitly sets palette zero black. | Generated black; save/decline/error interaction still needs capture coverage. |
| Fast-travel selection/cancel | Native screen 13/group 93 overwrites the health-specific BG3 map; the 2026-10-26 route verifies all destinations and sparse unlocks across health preloads. | Fixed blue field; select the independent `fast_travel_background.png`, without health variants or plain-menu aliases. Runtime publication/composition remains pending. |
| Dungeon maps | The new native room/floor route enters all six map-bearing dungeons and installs screen 5 through the pause transition. | Titled menu family; retain the installed health decoration across floor changes and item overlays. |
| Standalone map hints | Native hint initialization installs screen 4, disables BG3 and copies palette 81 into zero; the new route covers all six timer-driven reveals and A/B/START exits. | Procedural live palette-zero field, separate from the local fusion hint and all menu images. |
| Figurine collection | Native screen 3 copies palette entry 251 into zero, retaining its field across owned/missing entries and native navigation. | Procedural live palette-zero field; collection/reveal foreground remains native. |
| Fast-travel enquiry/travel | The native enquiry retains group 93; accepted travel fades out, then `sub_080A71F4` replaces the scene before the real bird arrival/tutorial path. EU decline exits to gameplay. | Retain the fixed image through confirmation/outgoing fade; clear at scene handoff or native cancellation restoration. Stale menu type 4 is not source evidence. |
| Local fusion map reveal | `sub_080A6B04` initially calls `sub_080A4D34`, then fills BG3 with `0x5001`. Its character 1 is uniformly index 15 in the local EU graphics. Palette bank 5 entry 15 (95) and palette zero are set to `0x475f`. | A separate procedural context using live palette zero. The earlier health decoration is no longer the field. Never load `menu_background_*.png` for this state. |

The local fusion-hint border is RGB (248,208,136), `#F8D088`, in the neutral
native EU capture. This is evidence, not a new runtime RGB constant. The
standalone map hint copies palette entry 81 into zero and has the distinct
green artist-reference swatch. The local hint's bottom-center pixel belongs
to its map label; sampling it would incorrectly classify the field as white.
No local-hint PNG or new swatch is exported. At that audit step the implementation
had 23 shared artwork filenames; `PORT_BACKGROUND_LOCAL_MAP_HINT` appended a
procedural catalog entry. The later family revision adds nine planned artwork
names without changing this hint rule.

This separates source/color ownership only. A BG3 field and the GBA backdrop
can have different blend targets even when their colors match. Resolve those
targets, palette-mask fades and foreground coverage before composing margins.

## Local verification (2026-10-25)

Available fixture: retail EU ROM, English preference, ARM64 RG build at width
capacity 569, QEMU and dummy SDL video/audio. Captures are native 240 × 160.
Local artifacts are separate from committed goldens under:

- `.tmp/background-state-audit-eu-files/`
- `.tmp/background-state-audit-eu-prologue/`
- `.tmp/background-state-audit-eu-intro/`
- `.tmp/background-state-audit-eu-hint/`
- `.tmp/background-state-audit-eu-en-v2/` (credits)

The failed initial title-input experiment in `.tmp/background-state-audit-eu-en/`
is incomplete evidence and must not be used as a reference. The driver now
supplies repeated title START edges rather than relying on the BIOS's short
automatic-input window.

The complete opening and retelling, file-screen route, local fusion-hint field,
and retained credits illustrations are reviewed using native captures and
source installation boundaries: 188 opening, 182 retelling, 75 file-route,
43 local-hint and 390 credits PNGs (878 total, including boot/setup frames).
The credits trace observes all nine installed illustrations, the explicit
clear at frame 12706 and the post-credits prompt at frame 13314. Full
intermediate-effect correctness is a separate gate; these captures do not validate enhanced margins, GPU execution,
fixed-screen centering or taller gameplay.

Remaining visual coverage includes dungeon menus, seven fusion layouts and
their health variants, standalone map hints, fast travel, figurines,
detail-map scrolling, save/sleep/game-over choices, save/copy/erase success and
error paths, post-credits save/decline/error, additional fusion-event hints and
other enabled languages. USA/JP ROM fixtures and a physical RG device were
unavailable. Keep the full visual-catalog, effect-resolution and device-budget
checkboxes open.

Verification commands:

```sh
python3 tools/screen_background_regression_test.py
python3 tools/tests/export_screen_backgrounds.py
python3 tools/widescreen_engine_regression_test.py
python3 tools/widescreen_scroll_regression_test.py
python3 tools/widescreen_regression_test.py
python3 tools/tests/capture_background_audit.py  # requires Pillow
xmake build -y tmc_pc
```

All pass. The existing EU native parity gate also passes the original Tier A/B
logo/title hashes. Headless EU startup reaches
`Port layer initialized. Entering AgbMain...`. These are local cross-build/QEMU
checks, with no asset staging change, device deployment or golden update.

## Pause-menu health and map matrix (2026-10-25)

This matrix predates implementation of the five-family split. Its ten native
decoration loads are not an expected PNG decode count for the revised design.
The title/pieces/techniques family sequence and foreground-cap seams must be
added as separate assertions; the existing checkpoints remain native baselines.

`--scenario menus` adds 106 staged native cases on the same EU/English fixture.
The run records 691 PNGs, including transitions and setup, under
`.tmp/background-menu-audit/eu-en/`. The copied repro binary, QEMU wrapper and
actual binary SHA-256 are retained in `.tmp/background-menu-audit/`, separately
from the original audit artifacts and committed goldens.

The first seven cases open inventory with the following game health values:

| Health / maximum | Installed decoration | Boundary exercised |
| --- | --- | --- |
| 24 / 24 | Normal | Full health |
| 16 / 24 | Normal | Exactly eight health units missing |
| 15 / 24 | Damaged | Nine health units missing |
| 9 / 24 | Damaged | Just above the critical threshold |
| 8 / 24 | Critical | One heart remaining |
| 1 / 24 | Critical | Lowest positive health |
| 8 / 8 | Critical | Critical takes precedence even at full maximum health |

Three more menu openings install normal, damaged and critical decorations.
Within each opening the driver runs 33 views, without reinstalling the field:

- Inventory, its save prompt (screen 11), and inventory again.
- Quest status, its save prompt (screen 10), and quest status again.
- Sleep confirmation, followed by quest status.
- Overworld map, kinstone pieces and sword techniques.
- All 17 base detail maps (graphics groups `0x5f`–`0x6f`), the built-house and
  fused-kinstone variants (`0x70`/`0x71`), and the three saved-interior variants
  (`0x72`–`0x74`).

The EU save/sleep configuration rows retain the preceding palette. Requests
therefore preserve the real source-screen order: inventory before screen 11,
quest before screen 10 or sleep. These are prompt/layout checks; no save or
sleep choice is confirmed. The dungeon-map screen requires a dungeon fixture
and is not inferred from an overworld menu.

For each case, the driver waits for the native transition and initializer to
finish, then holds neutral geometry for 24 more ticks before capturing. It
changes live health to another decoration category and captures again after
12 ticks. The altered health remains in place while changing screens, exposing
accidental selection from current health rather than installed content. The
full-one-heart case also stages a heart-container/heal before checking retention.
Every one of the 106 ready and 106 held checkpoints keeps the expected installed
decoration. There are exactly ten decoration loads for ten menu openings, with
no extra load at a screen change or health mutation.

The driver requests the native pause transition phases directly for screen and
same-screen map changes; it does not skip those phases or replace their graphics.
House/fusion flags and saved room identity are controlled fixtures, not evidence
that the corresponding quest prerequisites or button navigation were exercised.
The original saved room identity is restored before leaving the menu. The map
checks use their native initial scroll position and do not establish full
scrolling/marker/visit-state coverage.

Source observations come from `sub_080A4D34` after decoration installation,
`sub_080A4DB8` after screen initialization, and `sub_080A67C4` after detail-map
graphics selection. These events are audit instrumentation. Runtime background
selection remains unimplemented; nothing classifies all `TASK_GAME` frames as
menus or enables an override during gameplay.

The checker independently decodes the three native menu backgrounds from the
verified ROM using the exporter, without opening or overwriting artist files.
It compares the exposed inventory bottom row at 26 ready/held checkpoints:
6,240 exact RGB pixel comparisons pass. All three reference rows have different
hashes, so a stale or wrong health variant fails the comparison. Other screens
can cover that row with foreground frames or labels, and are reviewed using
their complete native captures instead of applying an invalid common pixel mask.
JP exporter recipes remain unavailable and are reported separately if requested.

`summary.json` records named case coverage, captured-frame hashes and the ROM
pixel checks. `menus-01.png`–`menus-03.png` show the neutral matrix; `contact-*.png`
also include the transition and health-mutated states. Validation rejects missing,
reordered or faded checkpoints, a wrong screen/map recipe, unexpected decoration
reloads, blank neutral menus and wrong inventory pixels. The ROM-free checker
tests exercise coverage, state, recipe and pixel mismatch failures.
Existing catalog/exporter and engine/scroll/
PPU regression checks pass. Full context, effect and regional acceptance remains
open, including the fusion family's independent decoration/foreground layouts.

The repro and restored RG release builds pass. Native EU logo/title Tier A/B
parity retains the original hashes, and the restored release reaches
`Port layer initialized. Entering AgbMain...` under QEMU/dummy video/audio.
The original release configuration has `repro_harness=n`; no device deployment,
asset staging or golden approval occurred.


## Semantic family and heading audit extension (2026-10-25)

The repro-only trace schema is now version 2. `family` observes the installed
`plain`, `titled`, `pieces`, `techniques` or `fusion` setting independently of
`decoration` (installed health). The pause initializer publishes its observation
only after loading native layout graphics; a pending screen request retains the
outgoing identity. Reused hint/figurine/fast-travel initialization does not prove
a pause family. Non-menu audit routes must keep this observation `unknown`.
These are audit observations, not the future runtime background adapter.

`heading_style` is 0 for absent, 1 for the top-left plate and 2 for the centered
pieces/techniques heading. `heading_x`, `heading_y` and `heading_transition`
are sampled at the native heading draw/skip call. Stable ordinary headings are
at `(64,16)`, pieces/techniques at `(96,24)` and prompts have no heading. Ordinary
pause animation uses `y = 16 - 2 * transition`; fusion uses
`y = 16 - transition`. Changed poses trigger additional captures. Pose fields
refer to the last draw, so an installation event can still carry the outgoing
pose; completed-frame checkpoints and the per-tick trace establish the incoming
pose. The checker validates the pose formula and requires fully exposed,
intermediate and fully withdrawn ordinary heading poses.

The original 106 pause cases retain their order, threshold fixtures, 22 map
recipes and ready/held checkpoints. Three appended cases use the native fusion
initializer, with a staged Tingle fuser identity and a representative fusion
setting at each installed health. No fusion, save or sleep is confirmed. This
is family-setting coverage, not coverage of all seven fusion layouts, their
interactions or normal quest prerequisites.

The checker requires all 15 family/health settings and, for each installed
health, titled → plain → titled, titled → pieces → techniques → titled switches.
Live health mutations continue across pause-family changes; native decoration
reloads remain forbidden inside a pause opening. There are ten original pause
openings plus three independent fusion openings. These 13 native decoration
loads say nothing about PNG identity, cache hits or decode counts: a same-health
family change selects a different artwork identity in the planned loader.

Independent ROM references now also check pieces/techniques rows 64–159, where
the static BG2 panel is unobscured. The mask excludes the upper localized
heading and live OBJ/BG0 content. This retains inventory's exposed bottom-row
check; identical native panel pixels across health states do not merge their
three artist filenames. Existing native captures and committed goldens remain
separate from the new local artifacts.

The local EU/English matrix under `.tmp/background-family-audit/eu-en/` has
2,082 captures, 109 cases, all 15 combinations, 15 required same-health switches
and all nine ordinary transition poses (0,2,…,16). The 212 original ready/held
checkpoints are byte-identical to `.tmp/background-menu-audit/eu-en/menus/`.
Inventory retains 6,240 ROM pixel comparisons; the 12 pieces/techniques
checkpoints add 276,480 comparisons. `summary.json` records this coverage,
baseline comparison and native capture hashes. Original artifacts stay intact.

The 75-frame file route is retained under `eu-routes/`; the final map-initializer
guard passes a separate 44-frame local-hint run under `eu-hint-final/`. Copied
matrix/final repro binaries and SHA-256 files are in the audit root. The final
guard excludes reused non-pause initializers without changing the menu-matrix
branch. Both EU native logo/title parity scenes pass their original Tier A/B
hashes with the final repro binary. The RG repro and restored release builds,
final EU startup under QEMU/dummy video/audio, 16 ROM-free checker tests and
catalog/exporter/engine/scroll/CPU PPU regressions pass. Release configuration
is restored to `repro_harness=n`.

The foreground title cap, enhanced margin seam/crop checks, GPU execution,
USA/JP/language coverage and device budgets remain separate open gates.


## Seven-layout fusion audit extension (2026-10-26)

The repro-only menu route now appends 21 cases: all seven native fusion layout
indices at each installed health decoration. Original case IDs 0–108 stay in
their original order, including the three representative Tingle cases. The
complete route contains 130 cases, each with ready and held checkpoints; live
health changes after installation still must leave the decoration installed.

Partner fixtures come from the active ROM's bounded NPC fuser table. The native
text renderer measures partner names into a private 768-byte scratch buffer
using the fusion menu's font/color/width settings. Discovery neither writes
VRAM nor borrows the global text pool, and does not advance simulation. Each
selected real NPC identity survives the entity-pool clear. The native fusion
initializer independently measures that name and chooses graphics group
`0x76 + layout`; the harness never forces that index or supplies replacement
layout graphics. A language whose partner names do not cover all seven choices
fails early with its observed layout mask rather than claiming full coverage.

The additive schema-v2 `fusion_layout` observation follows the existing native
`fusion_screen` installation event and is `-1` outside the fusion family. The
checker requires the full installation sequence and agreement at both event
and PNG checkpoints, all 21 layout/health settings, installed-decoration
retention and the existing family/heading contracts. Retained pixel-baseline
comparison accepts either the original 106 pause cases or all 109 family cases.
The dummy window is now native size to reduce QEMU presentation overhead;
captures remain native 240 × 160 pixels.

EU/English discovery selects these native partner identities (decimal
`NPC id / type / type2`); indices are zero-based, matching the installation event:

| Layout | NPC identity | Rendered name width | Graphics group |
| --- | --- | --- | --- |
| 0 | 34 / 0 / 0 | 26 | `0x76` |
| 1 | 48 / 2 / 0 | 53 | `0x77` |
| 2 | 56 / 2 / 18 | 58 | `0x78` |
| 3 | 6 / 15 / 0 | 68 | `0x79` |
| 4 | 45 / 0 / 7 | 79 | `0x7a` |
| 5 | 78 / 1 / 0 | 87 | `0x7b` |
| 6 | 83 / 0 / 0 | 90 | `0x7c` |

Verification: all 130 EU/English cases pass under QEMU/dummy video/audio,
producing 2,348 PNGs. All 218 ready/held checkpoints from the retained 109-case
family audit are byte-identical. All 15 family/health settings, all 21 fusion
layout/health settings, the 15 required same-health family switches and all nine
ordinary heading poses pass. Inventory retains 6,240 independently decoded ROM
pixel comparisons; pieces/techniques retain 276,480 lower-panel comparisons.
The final fusion contact sheet was inspected, including each partner portrait
and differently sized name plate. There are 34 native decoration installations
(ten pause and 24 fusion), not PNG decodes or cache publications.

The complete artifacts are under `.tmp/background-fusion-audit/eu-en-scale1/`;
`summary.json` records comparisons and decoded-pixel hashes, and `binary.sha256`
identifies the copied capture binary. The earlier `eu-en/` and `eu-en-final/`
runs were interrupted while reducing dummy-window overhead and are incomplete
evidence. A separate `final-startup/` check verifies all seven name fixtures
with the final explicit scratch-alignment/missing-ROM guards; it is not a full
matrix capture. Original references, artwork files and committed goldens remain
unchanged. No new goldens were approved.

The 19 ROM-free checker tests, nine exporter tests, C11/C++17 catalog/exporter
agreement, engine/scroll regressions, ARM64 repro and restored release builds
pass. Release EU startup under QEMU reaches
`Port layer initialized. Entering AgbMain...`. The original release options are
restored, including `repro_harness=n`; no device deployment or package staging
was performed.

This extension covers neutral native fusion layouts and installed health, not
fusion confirmation/mismatch/cancel effects or quest prerequisites. Dungeon
menus, standalone hints, fast travel, figurines, interaction/error paths,
additional languages/regions, compositor effects and device budgets still keep
the full context/state task open. Runtime background loading/composition and
title-frame completion remain their own implementation tasks.


## Native fast-travel selection/cancel audit (2026-10-26)

The separate `--scenario travel` route covers all eight selectable destinations
with all windcrests unlocked, plus a sparse mask unlocking only destination 7,
at each of the three initial health fixtures (24/24, 15/24, 8/24). There are
27 cases and 54 neutral ready/held checkpoints. After native initialization,
real right-key edges select the destination; no cursor assignment or layout
patching is used. Live health changes after the ready checkpoint, then B follows
the native cancellation path and restores gameplay before the next opening.
The route does not confirm travel or modify a real save file.

**Correction to the provisional plain-family mapping:**
`Subtask_FastTravel_0` calls `sub_080A4D34`, loading health graphics group
0x56/0x57/0x58 into BG3. Screen 13 then loads group 93, which writes another
0x500-byte tilemap to the same GBA EWRAM address `0x02001a40`. This replaces the
entire visible health-specific tilemap. The native field is the fixed blue map
border, with palette 185; it is neither the plain menu's installed health
variant nor a solid palette-zero field. Independently decoding BG3 after groups
`health, 93, 129` and palettes `181, 185` yields byte-identical full native BG3
references for all three preload paths.

Fast travel retains menu `family=unknown` because it is not one of the five
menu health families. The completed catalog/exporter now assigns its independent
fixed `PORT_BACKGROUND_FAST_TRAVEL` / `fast_travel_background.png` context.
There are 33 artwork names and the same 15 menu family/health combinations.
The earlier interim no-source decision left this image missing; the dedicated
fixed-image addition supersedes it. A future runtime adapter selects it after
the observed field installation, without choosing plain-menu health art from
the overwritten preload.
The trace's `decoration` value here records that preload event, not an installed
semantic health background after group 93.

Native `fast_travel_screen`, `fast_travel_choice` and `fast_travel_cancel`
events identify installation, input-driven selection and cancellation.
Additive schema-v2 fields `travel_case`, `travel_field`, `travel_destination`,
`travel_unlocks` and `display_control` provide neutral checkpoint evidence.
`travel_field` becomes 1 only after the native fixed field is installed and
returns to 0 on gameplay restoration. Reusing screen 13 must never publish a
pause-menu family, and no heading is drawn. The checker requires the complete
request/install/ready/mutate/held/cancel/restore sequence, native cursor events,
correct unlock masks, BG3 enablement, no extra decoration loads and unchanged
fixed-field pixels after health mutation.

The EU/English run passes all 27 cases, 84 native cursor changes, 27 native
cancellations and 27 gameplay restorations. It records 482 PNGs. The exposed
bottom row (x=0–239, y=159) matches the independent fixed-BG3 ROM reference at
all 54 checkpoints: 12,960 runtime pixel comparisons. Live map, marker, frame
and cursor content outside that mask is excluded from the pixel claim. The
27-case neutral contact sheet was inspected, including the eight destinations
and sparse-mask cases at each starting health.

Authoritative local artifacts are `.tmp/background-travel-audit/eu-en-fixed/`;
`summary.json` records coverage, decoded pixel hashes and reference checks.
`binary.sha256` beside it identifies the copied capture binary. The earlier
`eu-en/` run is exploratory evidence with the superseded plain-family
assumption; use the corrected run for classification. No artist file or
committed golden was changed or approved.

Verification passes: 26 ROM-free checker tests, ten exporter tests, C11/C++17
catalog/exporter agreement, compatibility with the retained 130-case
pause/fusion trace, ARM64 repro/release builds and release startup under
QEMU/dummy video/audio reaching `Port layer initialized. Entering AgbMain...`.
The original release configuration is restored with `repro_harness=n`. No
staging or device deployment was performed.

Remaining fast-travel coverage includes confirmation/decline, the actual travel
transition, wraparound/other sparse masks, local effects and other languages or
regional ROMs. The full context/state task also retains dungeon, figurine,
standalone-hint and functional/error gaps. Margin composition/effect lifetime,
GPU execution and physical RG budgets are separate open gates.


## Fixed fast-travel reference export (2026-10-26)

The independent catalog context and native recipe complete the previously
missing artist reference. `fast_travel_background.png` contains only BG3, using
groups 86/93/129 and palettes 181/185. No native map/frame/markers/cursor or
localized text is baked into it. It is one image for all health preloads,
destinations and unlock masks, with the existing five exact aspect sizes.
The full decoded PNG is compared against all three independent native BG3
reconstructions, and its exposed bottom row against the retained 54 native
checkpoints. The extractor rerun and artist preservation evidence are recorded
in the task list. Runtime source publication, effect masks and confirmation/
travel-transition lifetime remain implementation/acceptance follow-ups.

## Native figurine collection audit (2026-10-26)

`--scenario figurines` extends the first open context/state gate with 27
EU/English collection views across six openings: normal/damaged/critical health
preloads, each before and after credits. Synthetic in-memory ownership marks
entries 1, 130 and 136 owned; entries 2 and 7 are missing. Real DOWN/R input
visits 1, 2, 7 and 130 before credits, and also 136 afterward. No selected index,
native graphics, timers or fades are patched. B exits the pre-credits openings;
START exits the post-credits openings. Every opening restores native gameplay.
This stages access to the viewer, not the normal quest prerequisites.

`FigurineMenu_080A4608` calls the shared health initializer, installs screen 3
(graphics 89 / palette 206), then copies palette entry 251 into palette zero
and clears BG3 before native list text is drawn. This is the catalog's existing
procedural figurine field, not any of the five menu-health image families.
Palette group 12, loaded by `LoadGfxGroups`, supplies entry 251 in the fixture;
decoding only palettes 181/206 would incorrectly report its value as zero.
The neutral fixture's value is `0x318e`, RGB (112,96,96); runtime composition
must use the live source rather than hard-code this observation.

Additive schema-v2 fields record `figurine_case`, `figurine_field`, selected
index/ownership, staffroll status, palette zero/251 and description scroll/limit.
Native `figurine_screen`, `figurine_choice` and `figurine_cancel` events record
installation, input-driven navigation and exit. The field is retained through
exit until gameplay restoration; reusing screen 3 never publishes a pause
family or heading. The checker requires all 27 ordered ready/held pairs,
six native installations/restorations, exact navigation events, health mutation
and no extra health-decoration installation. All 54 checkpoints pass.

The exposed samples at (100,40) and (220,100) match live palette zero at all
54 checkpoints (108 pixel comparisons). Independent ROM decoding verifies
BG2/BG3 transparency there after the native graphics/clear sequence, and the
palette source agrees for all three preloads. BG1's description window begins
at row 120. Frame corners are excluded: their opaque BG2 pixels have a different
color. List lettering, figure sprites, selection bars and description text are
outside the pixel claim. The neutral 27-view contact sheet was inspected.

Reproduce with a repro-enabled binary and the standard capture driver:

```sh
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --scenario figurines \
  --output .tmp/background-figurine-context-audit/new-run
python3 tools/tests/background_figurine_audit.py
```

Authoritative local artifacts are
`.tmp/background-figurine-context-audit/eu-en-verified/`; `summary.json` records
coverage and decoded capture hashes. The adjacent `binary.sha256` identifies
the copied ARM64 binary. The earlier `eu-en/` and `eu-en-final/` runs were exploratory: their initial
checkers used opaque frame corners or omitted base palette group 12. Both were
corrected using ROM source coverage.
Neither these runs nor the unrelated older `.tmp/background-figurine-audit/`
replace committed goldens. No artist image was touched.

The seven new failure-oriented checker tests, retained 26 audit-checker tests,
11 exporter tests, C11/C++/UBSan catalog agreement, engine/scroll/CPU PPU checks,
ARM64 `xmake build -y tmc_pc` and EU startup under QEMU/dummy video/audio pass.
The startup reaches `Port layer initialized. Entering AgbMain...`. Existing
130-case menu/fusion and 27-case travel traces still validate. Incoming build
options (including `repro_harness=y`) are preserved; no staging or device
deployment was performed.

The collection view is a completed slice, not completion of the full gate.
Prize drawing/reveal/duplicate effects, description scrolling, other ownership
patterns, dungeon menus, standalone hints, travel confirmation/effects,
remaining functional/error paths and other available languages still need
coverage. USA/JP ROM fixtures and physical RG budgets remain unavailable.
Runtime state publication, compositor effect targets and margin pixels remain
separate implementation/acceptance tasks.

## Standalone hint reveal and exit audit (2026-10-26)

`--scenario hints` runs 36 cases: six native `gUnk_08128F4C` variants, first
and repeated reveals, at normal/damaged/critical health preloads. The fixture
stages all visited-area bits and either clears saved hints or preinstalls all
hint bits. Native `SUBTASK_MAPHINT` initialization and the 60-tick reveal timer
are unchanged. Each case records a neutral checkpoint before the reveal, after
it and after changing live health. Real A/B/START edges exit the hint, followed
by native gameplay restoration. These are synthetic access/save fixtures, not
quest-prerequisite verification.

Native `hint_screen`, `hint_reveal` and `hint_cancel` events distinguish field
installation, the timer-30 reveal and input-driven exit. Additive schema-v2
fields record installed field lifetime, case/variant, remaining timer, visible
and pending masks, saved hints and live palette entry 81. The checker requires
all 108 ordered neutral checkpoints, all 36 native reveal/exit/restore paths,
exact exit keys, first/repeat save fixtures and no unexpected decoration load.
The pre-reveal visible mask is zero; after the timer-30 event it is the installed
pending mask. Requesting or leaving the subtask alone never publishes/clears a
field ahead of its native installation/gameplay restoration.

Screen 4/group 94 replaces the health preload, and initialization disables
BG3 and copies palette entry 81 to zero. This confirms the existing standalone
hint procedural context, with no menu-health image or runtime PNG. Independent
ROM decoding verifies the six masks (1,30,32,64,128,256), source palette and clear
BG2 border at the two sampled bottom corners. The EU fixture source is `0x4b99`,
RGB (200,224,144); runtime composition must consume the live value. Each native
checkpoint's corners match that source, including after live health changes.
This validates source identity and neutral pixels, not resolved fade targets.

The initial EU/English run passes 36 cases, 108 checkpoints and 216 pixel
comparisons, with 603 captures. Artifacts are
`.tmp/background-hint-context-audit/eu-en/`; `summary.json` records coverage and
capture hashes and adjacent `binary.sha256` identifies the copied ARM64 binary.
The neutral matrix was inspected. Additional language coverage is recorded with
the dungeon/functional extension below. Original captures, committed goldens,
artist images and exporter names are retained.

## Native dungeon room and floor audit (2026-10-26)

`--scenario dungeons` enters real room 0 of Deepwood Shrine, Cave of Flames,
Fortress of Winds, Temple of Droplets, Palace of Winds and Dark Hyrule Castle
using `DoExitTransition`. Native room loading establishes `gArea.dungeon_idx`;
the driver does not assign a dungeon ID, floor metadata or layout table to an
overworld room. Each entry waits for gameplay and verifies the actual room and
dungeon before opening inventory and requesting the native screen-5 transition.
Spawn coordinates use the room's center. Access and item availability are staged
in memory, independently of the normal quest prerequisites.

Each dungeon has six openings: three health preloads, with map/compass/big-key
bits either all absent or all present (0/7). Real UP/DOWN edges first reach
floor zero, then visit every native floor. The six dungeons have 3/4/3/3/5/5
floors: 138 cases and 276 neutral ready/held checkpoints. Live health changes
after each ready checkpoint. START follows the native pause exit and restores
gameplay before the next opening; no direct `Subtask_Exit` shortcut is used.
Mixed item-bit combinations, alternate visited-room/quest flags and normal
entrance prerequisites are not covered by this matrix.

Native `dungeon_screen` and `dungeon_floor` events distinguish layout
installation from floor redraws. Additive schema-v2 fields record case, native
dungeon index, saved room area, selected floor and installed item bits. The
checker requires six native room entries, 36 ordered openings/restorations,
the exact floor-input sequence at each opening, titled-family/heading identity
at all neutral checkpoints, and no extra health-decoration installation.
The family appears at the native screen installation and clears on gameplay
restoration. Foreground map, labels, item icons and rotating palette entries
remain outside the artwork identity.

The bottom row (x=0..239, y=159) exposes the installed health BG3 field. It is
compared with independently ROM-decoded native health references at all 276
checkpoints (66,240 pixel comparisons), including after live health changes.
This validates installed native decoration, not custom PNG identity, margin
composition, fade targets or expanded rendering. No dungeon artwork filename
is added: the existing titled-menu family supplies its three health identities.

The completed EU/English dungeon run validates all 138 cases, 276 checkpoints,
36 native exits/restorations and 66,240 independent ROM pixel comparisons.
It records 1,492 captures; the four neutral matrix sheets were inspected.
Authoritative local artifacts are
`.tmp/background-dungeon-context-audit/eu-en-v2/`; the adjacent
`binary-v2.sha256` identifies the copied capture binary. `summary.json` records
capture hashes and coverage. This complete native run was revalidated after
correcting the checker's initial confusion between `numFloors` and
`highestFloor`, and its assumption that a pending request already had a saved
room snapshot. Native ready/held checkpoints require the actual saved room;
a request precedes `Subtask_FadeIn`'s snapshot and its `dungeon_area` observation
can still be zero. The earlier `eu-en/` run was interrupted after exposing the
driver's retained-tab setup error and is not complete evidence.

## EU hint and figurine language extension (2026-10-26)

The same final ARM64 binary ran `--scenario hints --scenario figurines` with
public language preferences 2/3/4/5 (French/German/Spanish/Italian). Each language
passes 36 hint cases / 108 checkpoints and 27 collection cases / 54 checkpoints,
including the independent ROM palette checks and native exits/restorations.
There are 603 hint captures and 388 figurine captures per language. Local
artifacts are `.tmp/background-dungeon-context-audit/eu-language-{2,3,4,5}/`,
with per-run summaries, hashes and neutral matrices. Representative localized
collection names/descriptions were inspected alongside the neutral English
hint and dungeon matrices. English retains its original verified hint and
figurine runs; none of those native references was overwritten.

Together with retained English evidence, these two routes cover all five EU
languages: 180 hint cases / 540 checkpoints / 1,080 pixel comparisons and
135 figurine cases / 270 checkpoints / 540 pixel comparisons. These narrow
source masks do not claim full localized text/description-layout correctness.
Dungeon floor coverage in this extension remains English-only; other screen
families also retain their previous language limits.

Reproduce the new routes using a repro-enabled binary (or QEMU wrapper):

```sh
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --scenario hints --scenario dungeons \
  --output .tmp/background-functional-new
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --language 2 --scenario hints --scenario figurines \
  --output .tmp/background-functional-fr-new
python3 tools/tests/background_functional_audit.py
```

Verification: 13 new failure-oriented functional-checker tests, retained 26
capture-checker and seven figurine-checker tests, 11 exporter tests, C11/C++/UBSan
catalog agreement, engine/scroll/CPU PPU regressions, ARM64 build and EU startup
under QEMU/dummy SDL pass. The existing 130-case menu/fusion, 27-case travel,
English figurine and standalone-hint traces still validate. The final binary
also preserves all five existing profile gates; no renderer pixel loop,
artwork file, committed golden or runtime packaging changes. Incoming xmake
options, including `repro_harness=y`, are preserved. No device deployment or
physical performance measurement was performed.

The full context/state task remains open for fusion interactions, travel
confirmation/decline/actual travel, figurine prize reveal/duplicate effects and
description scrolling, detail-map scrolling, save/sleep/game-over choices,
file copy/erase success/error and post-credits save/decline/error. Alternate
quest/room/item combinations and broader language/regional captures also
remain. USA/JP ROM fixtures were unavailable. Compositor effect resolution,
runtime source publication, PNG loading and margin rendering are separate
implementation gates; this audit does not enable them.

## Native fast-travel confirmation and scene handoff (2026-10-26)

`--scenario travel_actions` extends the retained selection/cancel route without
renumbering its 27 cases. The new EU-only matrix requests both native answers
at all eight destinations and the single-unlock destination-7 fixture, at three
health preloads: 54 cases, 27 declines and 27 accepted trips. Requests stage
post-Fortress story flags (`TABIDACHI`, `LV1_CLEAR`–`LV3_CLEAR`), derive progress 5
with `UpdateGlobalProgress`, and supply the ocarina in memory. Completed intro
flags include `EZERO_1ST`, `SOUGEN_01_ZELDA`, `MORI_ENTRANCE_1ST` and
`MORI_00_KOBITO`; bank-B access preserves the regional ordinal remapping. The
checker requires the staged story flags, progress and item at requests and all
neutral checkpoints. This avoids the
fresh-save festival redirect at Hyrule Town; it does not verify normal quest or
windcrest-unlock prerequisites. All destinations are visited through native
travel code, not a room assignment or forced player state.

The driver uses real RIGHT edges to select destinations, A/START to open the
enquiry, native choice navigation to select Yes/No, then A/START to submit it.
Read-only helpers observe when the text renderer has a ready two-choice enquiry
or a completed page awaiting input. No message result, selected cursor, fade,
player action or player-control state is assigned. Live health changes between
the ready and held checkpoints; the enquiry adds a third neutral checkpoint.
Each result waits for normal player action/control and a further 24 stable ticks
before capturing gameplay. Result case identity remains fixed through that
simulation boundary and advances on the next tick.

The first accepted arrival leaves `WARP_1ST` unset until the native bird/script
path sets it. The driver advances queued story-hint and first-use ocarina
pages with real A edges, including the final close prompt. Subsequent trips
retain the native flag. Per-case timeouts allow these dialogues and the player
leave animation to complete. Source lifetime is therefore checked across the
first-use tutorial as well as repeated travel, rather than merely observing the
requested destination fields before arrival.

Native `fast_travel_prompt`, `fast_travel_answer`, `fast_travel_fade`,
`fast_travel_depart` and `fast_travel_handoff` events identify confirmation,
answer consumption, outgoing fade and native scene replacement. Additive
schema-v2 fields record case, native menu/text/choice state, story/ocarina and
first-use state, fade controls, actual room/player control and the requested
native destination tuple. The checker requires exact native lifecycle order,
input-driven selection, no extra decoration load, all destination handoffs,
normal gameplay return and agreement between completed result events/captures.

The fixed fast-travel source remains installed through enquiry and outgoing
fade. Decline retains it through cancellation until native gameplay restoration.
Accepted travel waits for the outgoing fade to finish, then `sub_080A71F4`
replaces the scene; the observation clears the installed field at that explicit
handoff. `gMenu.menuType` can still contain 4 in gameplay and must not be used to
infer that the travel background remains installed. This is audit observation,
not implementation of the runtime adapter or image cache.

The two exposed top corners, (0,0) and (239,0), are compared against independent
ROM-decoded fixed BG3 pixels before/after health mutation and with the enquiry
open. Decoding checks BG1/BG2 transparency there using their native -4 scroll.
The bottom row used by the earlier selection audit is excluded because dialogue
can cover it. The field references are identical for all three health preloads;
the neutral corner color is RGB (168,216,248). Native map/frame/OBJ/dialogue
pixels stay outside this source mask. These neutral checks and recorded fade
boundaries do not resolve unfaded-source effect coverage or certify intermediate
fade pixels, custom artwork, enhanced margins or renderer/cache behavior.

This route currently rejects non-EU ROMs explicitly: their decline path returns
to destination selection, whereas EU exits the subtask. The existing generic
selection/cancel route remains available independently. Other regions require
their own interaction expectations and native fixtures.

The completed EU/English run passes all 54 cases: 54 native enquiries/answers,
27 native decline restorations and 27 outgoing fades/handoffs/arrivals. All
162 neutral ready/held/enquiry checkpoints match the independent fixed-BG3
reference (324 pixel comparisons); all 54 completed gameplay captures are
nonblank and agree with their native case/room/control events. The first-use
path consumes eight native page/final-close inputs, then retains `WARP_1ST`
for subsequent travel. The run records 1,506 captures; both enquiry matrices
and both completed-gameplay matrices were inspected.

Authoritative artifacts are
`.tmp/background-travel-actions-audit/eu-en-story/`; `summary.json` records
coverage, ROM/wrapper hashes and decoded capture hashes. Adjacent
`binary-story.sha256` identifies the copied ARM64 executable. Earlier runs in
that parent directory are incomplete development evidence: they exposed queued
arrival pages, too-short per-case timeouts and missing completed-intro flags.
The `eu-en-current/` run was deliberately interrupted while updating the
fixture. In particular, fresh-save festival redirection and the South Hyrule
Field white intro are not accepted destination results; the checker fails those
cases instead of assigning a room, cancelling a fade or forcing player control.

A separate final-binary `--scenario travel` run is retained at
`.tmp/background-travel-actions-audit/eu-en-selection-final/`. All 54 original
ready/held checkpoints have byte-identical decoded RGBA pixels compared with
`.tmp/background-travel-audit/eu-en-fixed/travel/`; `baseline-comparison.json`
records that comparison. Original capture files and committed goldens are not
rewritten. Neither native decoration load counts nor these tests claim PNG
cache/decode behavior.

Reproduce with the current repro-enabled binary and a verified EU ROM:

```sh
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --scenario travel_actions \
  --output .tmp/background-travel-actions-new
python3 tools/tests/background_travel_actions_audit.py
```

The 12 new failure-oriented checker tests and 46 retained capture/figurine/
functional-checker tests pass, along with the 11 exporter tests, C11/C++/UBSan
catalog agreement and engine/scroll/CPU PPU regressions. ARM64
`xmake build -y tmc_pc` and final EU startup under QEMU/dummy SDL pass, reaching
`Port layer initialized. Entering AgbMain...`. The retained menu/fusion, travel,
figurine, hint and dungeon traces still validate. Incoming xmake options,
including `repro_harness=y`, are preserved. No artist files, renderer pixel
loops, profile gates, runtime packaging, approved goldens or physical device
state change.

This completes the EU/English native confirmation/decline/arrival slice.
Remaining travel coverage includes other sparse masks/wraparound, other
languages/regions and resolved effect-source/intermediate-fade pixels. The
full context/state task also retains fusion interactions, figurine prize/
duplicate/scrolling behavior, detail-map scrolling, save/sleep/game-over and
file/post-credits success/error paths. Runtime context publication, image
loading, margin/native-crop checks and both-presenter/cache acceptance remain
their separate open tasks.

## Native figurine prizes, duplicates and description scrolling (2026-10-26)

`--scenario figurine_actions` adds a separate native matrix, retaining the old
27-case collection route. Each language has 54 cases: 48 prizes covering four
capsule types, first/duplicate ownership, three health preloads and pre-/post-
credits state, plus six collection-description cases. Native EU type-table
representatives are figurines 47/1/68 and 130 before credits or 136 afterward;
the latter two also provide scrollable collection descriptions. The driver
checks the active regional type table before entering a prize. Eligibility,
lottery selection and shell spending are staged rather than exercised: this
route begins at the native prize/viewer subtask with synthetic in-memory save
ownership, not at the figurine-device interaction in the live world.

The capsule object runs its real bounce and animation, enables the native reveal
timer and creates its explosion effect. The native menu sets ownership at
counter 64, distinguishes a new award from a duplicate, then finishes the
brightness sequence at counter 128. `WriteBit` returns the previous bit mask,
not a normalized boolean: duplicate values can be 128/2/16/4/1 for these IDs.
The checker preserves those values and requires the ownership bit to change
only at the native award boundary. No capsule frame, timer, ownership result,
selected index, scroll offset or effect control is forced by the driver.

Read-only native events identify capsule initialization/opening, award and
reveal completion. Additive schema-v2 observations record action case, native
prize/collection variant and menu, reveal counter, raw duplicate mask,
brightness/BLDCNT, description offset/limit, BG1 Y and input. Every prize must
retain every engine-state counter from 1 through 127, with brightness
`counter / 4` rising to 16 and folding down as `32 - counter / 4`, and BLDCNT
`0xbf`. Completion must clear both controls. Frames are sampled every four
simulation ticks during that sequence. These are native control observations;
engine state can lead the completed raster by a tick, so they do not certify
intermediate flash pixels or resolved source/window effect coverage.

After neutral reveal/viewer readiness, the driver mutates live health, then
uses real RIGHT/LEFT edges to scroll to the description limit and back to zero.
An extra edge at each end checks both clamps, including zero-length descriptions.
Collection cases reach their target using native R/DOWN navigation. Native B
and START exits restore gameplay; each result retains its case ID through the
capture boundary before the next opening starts.

Four neutral checkpoints per case (ready, held, bottom, top) require the same
installed procedural palette-zero field, no menu family/heading and no leftover
brightness controls. BG1 Y must equal `scroll - 112`. The independently decoded
ROM palette and exposed source samples remain those of the original figurine
field; neither health mutation nor prize/collection mode selects a menu image.
A separate native description crop, x=16..223 / y=120..143, excludes figure,
frame and edge arrows: held and returned-top pixels must equal the ready text;
scrollable bottom pixels must differ, while a zero-limit crop stays unchanged.
This checks rendered text movement as well as engine scroll registers. It does
not assert every localized glyph or enhanced/HD-text layout.

Restoration here means the native game task/update state, original room,
retained award ownership and cleared figurine source. The staged route can
return with the player in the native scroll-wait action; it does not establish
normal player-control readiness or a full live-world figurine-device round trip.

Local evidence is in `.tmp/background-figurine-actions-audit/`.
`verified-coverage.json` combines disjoint canonical ranges for each language;
`coverage-language-{1..5}.json` and each segment's verification report retain
trace/ROM hashes, every retained PNG's decoded RGBA hash and checkpoint paths.
The original five runs hit the former 1200-second process timeout. English
recorded all 54 cases and the native terminal marker before timing out during
the final capture tail. French/German/Spanish/Italian completed prefixes of
50/46/51/51 cases. Explicit offline recovery retained those completed receipts;
separate suffix runs starting at 50/46/51/51 completed successfully under the
new 2400-second budget. Original raw traces, including incomplete EOF fragments,
remain unchanged. `capture-outcome.json` sidecars record the timeouts separately
from successful suffix process exits. No interrupted process is reported as a
successful full run.

Across the five languages the verified union contains exactly 270 cases:
240 native prize lifecycles (120 first awards and 120 duplicates) and 30
collection cases. It checks 30,480 native brightness counter values, 1,080
neutral field checkpoints / 2,160 ROM-derived source-pixel comparisons and 810
description-crop comparisons. Contact matrices retain ready/bottom/top images;
`localized-scroll-review.png` provides visually inspected pre-/post-credits
scroll samples in all five languages. The final range-enabled binary also
reran the original English collection route in `eu-en-viewer-final/`; all 54
ready/held checkpoints remain byte-identical in decoded RGBA to
`.tmp/background-figurine-context-audit/eu-en-verified/figurines/`.

Reproduce a full language run with a repro-enabled binary and a verified EU ROM
(the default scenario budget is now 2400 seconds):

```sh
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --scenario figurine_actions --language 1 \
  --output .tmp/background-figurine-actions-new
python3 tools/tests/background_figurine_actions_audit.py
```

An interrupted capture can be inspected explicitly, without declaring it a
successful process or silently accepting an incomplete normal capture:

```sh
python3 tools/verify_figurine_actions.py \
  --capture /path/to/interrupted/figurine_actions --rom /path/to/EU.gba \
  --language 2 --completed-prefix --output /tmp/figurine-prefix.json
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --scenario figurine_actions --language 2 \
  --figurine-start-case 50 --output .tmp/background-figurine-suffix
```

The start value must match the first missing canonical case; a suffix stages
each independent save fixture and executes its complete native lifecycle.
`combine_coverage()` rejects duplicate IDs, gaps and mixed languages/ROMs.
Normal capture validation still requires a native terminal marker and a
successful process exit. Recovery rejects corruption before EOF and validates
only cases with matching restoration/capture receipts.

The 16 new failure-oriented checker tests and 58 retained audit tests pass,
along with 11 exporter tests. C11/C++/UBSan catalog checks and engine/scroll/CPU
PPU regressions pass. ARM64 `xmake build -y tmc_pc` and the final EU QEMU/dummy-SDL
viewer run pass, reaching `Port layer initialized. Entering AgbMain...`.
Incoming build options remain intact. This audit does not implement the runtime
image adapter/compositor or change approved goldens. Other regions, live-device
lottery/shell flows, normal player-control readiness and resolved intermediate
effect pixels remain open, alongside the full catalog's remaining interaction
and error paths.

## Native detail-map scrolling and return paths (2026-10-26)

`--scenario detail_maps` exercises all 22 native detail-map recipes at three
installed health decorations on the EU/English fixture: 66 cases across three
pause-menu openings. Each opening installs its health field once. The route
requests native detail-screen transitions, staging the same house/fusion flags
and interior room snapshots as the retained static matrix; it does not navigate
the world-map cursor or enter those 22 areas in the live world. All fades,
initializers, map uploads, held-key scrolling and back/exit transitions run
normally. The route never assigns the map scroll offset or patches its artwork.

The trace observes case, area, scroll/limit, BG1/BG2 Y, held input, pause phase,
room snapshot and conditional recipe flags. Native `detail_scroll` events are
emitted after the map update applies offsets. The checker requires the exact
2-pixel-per-held-tick sequence from the initial midpoint to the top, then bottom,
then top again, including two extra held ticks at each bound. Twelve cases have
zero scroll range and must still exercise both direction clamps. The independent
ROM table confirms each area's limit and origin; BG1 stays at its native origin
while BG2 follows `scroll - origin`.

Five neutral checkpoints per case retain the same installed titled family,
health decoration and top-left heading: initial, after live-health mutation,
top, bottom and returned top. Native B returns from each detail screen to the
world-map page; START exits after each 22-case group. The checker observes the
actual B/START input boundary and requires return to the original game task and
room with the audit family cleared. It does not claim normal player-control
readiness or live-world navigation. World-map item ownership is an explicit
in-memory prerequisite: without it, the native menu redirects B to Quest Status.
The initial diagnostic run demonstrating that redirect is retained separately
in `eu-en/`; it was interrupted and is not successful audit evidence.

Source comparisons independently reconstruct native BG2 tiles and palettes
from the verified ROM, using a fixed 16-point grid and excluding transparent
samples. Each checkpoint must expose at least four opaque map samples. The
background comparison uses x=0..7 and x=232..239 at y=159; those 16 pixels still
distinguish all three ROM health fields. Some visited areas display a native
area-name box over the central bottom row, so comparing the entire row would
incorrectly include foreground UI. A separate x=24..215 / y=40..119 crop checks
unchanged pixels after health mutation, movement between top/bottom on scrollable
maps and exact returned-top pixels. All five crops must match for immovable
maps. This is a native-layout audit; it does not cover HD text, alternate
languages/regions, all world-event marker combinations or intermediate effects.

The complete native run and final verification are in
`.tmp/background-detail-map-actions-audit/eu-en-fixed/`: all 66 cases, 330 neutral
checkpoints, 5,280 exposed decoration-pixel comparisons and 4,020 opaque scrolled
map-pixel comparisons pass. Crops verify 66 health-retention pairs, 66 returned-
top pairs, 54 moving maps and 12 immovable maps. `verification.json` retains the
complete checked event sequence, decoded PNG hashes, source trace/ROM/binary
hashes and exact source masks. Ready/top/bottom/returned-top contact matrices and
`detail-scroll-review.png` retain visual evidence; the latter was inspected for
scrolling, zero-range, area-name and conditional-recipe examples.

The native process recorded its completion marker and exited successfully at
frame 17786. Its capture command had loaded an earlier checker that incorrectly
expected the pause UI subtask to remain 1 after exit; native code clears it to
0. That command's validation failure is retained in `capture-outcome.json`.
The final checker requires UI subtask 0 and validates the complete unchanged
trace and every capture, including the refined exposed-field mask. No partial
or interrupted cases are used for this final 66-case evidence.

The same binary reran all 130 pause/fusion cases in `eu-en-menus/`. All 260
ready/held checkpoint images remain byte-identical in decoded RGBA to
`.tmp/background-fusion-audit/eu-en-scale1/menus/`; `baseline-comparison.json`
records the comparison. Approved goldens are unchanged.

Reproduce using a repro-enabled binary and verified EU ROM:

```sh
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --scenario detail_maps --language 1 \
  --output .tmp/background-detail-maps-new
python3 tools/tests/background_detail_map_audit.py
```

The scenario defaults to a 2400-second capture budget. Twelve new checker tests
and 85 retained audit/exporter tests pass; catalog/exporter/profile geometry
and C/C++ ABI regression checks pass. The ARM64 `xmake build -y tmc_pc` and EU
QEMU/dummy-SDL startup pass, reaching
`Port layer initialized. Entering AgbMain...`. Existing build settings are
preserved. Native changes are read-only repro observations; runtime image
publication, compositor/effect resolution, approved art and renderer pixel
loops are unchanged by this slice.

The broad catalog remains open for fusion interactions, save/sleep/game-over,
file/post-credits success/error paths and the remaining regional/live-world
gaps. The separate effect-source, presenter/cache and physical-device acceptance
tasks remain open.

## Native sleep confirmation and wake-up (2026-10-26)

`--scenario sleep` adds a separate 15-case EU/English matrix: five input paths
at each of the three health preloads. It accepts with A, accepts after RIGHT /
LEFT, declines with RIGHT / A, declines with B / B and declines with RIGHT / B.
B at the initial Yes selection moves to No; a second B confirms that refusal.
The native cursor/answer events and sampled input distinguish that behavior
from immediate cancellation. No sleep status, menu result, fade timer, selected
cursor or graphics is assigned by the driver.

Each case starts a native pause opening at inventory, uses R to enter Quest
Status, then follows real LEFT / UP / LEFT input to the Sleep button. Quest
Status remembers its preceding cursor across openings: subsequent cases first
navigate from 4 through 8 / 3 / 1 to 0 with LEFT / UP / UP / LEFT, then repeat
the path to Sleep. The driver stages only health and the initial pause page;
it neither installs a sleep prompt directly nor overwrites the remembered
quest cursor. Native screen initializers preserve the real palette order:
inventory, quest, sleep prompt, then quest again.

The route waits for neutral prompt readiness, mutates live health, retains the
installed decoration and confirms its selected answer with real key edges.
Accepted cases execute `sub_080A5BB8`, arm native sleep and run `SetSleepMode`.
That function writes raw DISPCNT `0x80`, waits for all keys to be released and
then waits for a new press. The driver leaves the inputs released for 24 ticks
before a sleeping checkpoint and another four ticks before injecting raw A.
Read-only events observe entry and wake; additive schema-v2 fields record
sleep status, game ticks, cursor, menu/pause phase, new keys and raw KEYINPUT /
DISPCNT. Every boundary in each 29-frame entry-to-wake interval must be present,
with an unchanged game-tick count. Raw KEYINPUT must be `0x3ff` before the wake
request and `0x3fe` at wake. Game ticks must resume before the returned quest
checkpoint. This exercises the current PC sleep implementation's any-key wake
behavior; it does not test the prompt's printed SELECT/L/R chord or device/OS
suspend behavior.

The installed audit family remains plain while the native forced-blank bit
suppresses its image. All six sleeping captures must be completely RGB
`255/255/255`, matching the software PPU's explicit forced-blank fill. This
checks both register state and completed native pixels; it does not implement
future compositor effect resolution or certify intermediate fades. Declines
must never arm or enter sleep. All 15 cases return through the native quest
transition and START exit, preserving their case IDs and original game room,
clearing sleep status and the audit family on game restoration. Restoration
checks task/update/UI state; it does not assert normal player-control readiness.

Four neutral source checkpoints per case (quest ready, prompt ready, held after
health mutation and quest returned) compare exposed pixels against independently
ROM-decoded health fields. The source mask uses x=0..7 and x=232..239 at y=159;
all three health identities must differ at these actual sample points. The
central bottom row is excluded because returning with Sleep selected draws the
native "Go to Sleep Screen" tooltip there. Prompt checkpoints require the plain
family without a heading; quest checkpoints require the titled family and
original top-left heading. Only one decoration installation is allowed per
opening, including the live-health mutation and sleep/wake interval.

Complete successful command artifacts are in
`.tmp/background-sleep-actions-audit/eu-en-verified/`. Both native process and
capture command exit zero. `summary.json` retains the full checked event list
and decoded RGBA hashes for all 1,188 captures. `verification.json` records
trace/ROM/native-binary hashes, all six frozen intervals and final checker/pixel
results. All 15 cases pass: six accepted sleep/wake cycles, nine declines, 60
neutral source checkpoints / 960 field-pixel comparisons and six whole-frame
white checks / 230,400 sleeping pixels. Ready/held/returned-quest contact sheets
and the inspected `sleep-review.png` retain representative acceptance, B refusal
and all three health preloads.

Earlier diagnostic artifacts remain separate: `eu-en/` fails when the first
driver assumes the remembered quest cursor resets; `eu-en-final/` completes
natively but its then-loaded pixel checker rejects the foreground tooltip.
Their `capture-outcome.json` files retain those failures. The complete latter
trace also passes final offline verification without rewriting raw evidence;
neither diagnostic is substituted for the successful `eu-en-verified/` command.

With the new observation hooks, the retained pause/fusion matrix was rerun in
`eu-en-menus/`. All 260 ready/held checkpoints across 130 cases remain
byte-identical in decoded RGBA to
`.tmp/background-fusion-audit/eu-en-scale1/menus/`; `baseline-comparison.json`
records the comparison and its binary hash. Approved goldens are unchanged.

Reproduce with a repro-enabled binary and a verified EU ROM:

```sh
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --scenario sleep --language 1 \
  --output .tmp/background-sleep-new
python3 tools/tests/background_sleep_audit.py
```

The capture runner restricts this interaction route to EU ROMs and uses the
usual 1200-second budget. Runtime configuration and any startup save-header
writes stay in its temporary directory; this route does not perform a game
save. Fourteen new failure-oriented checker tests and 97 retained audit/exporter
tests pass, together with catalog/exporter/profile geometry and C/C++ ABI
checks. The final ARM64 `xmake build -y tmc_pc` and EU QEMU/dummy-SDL startup
pass, reaching `Port layer initialized. Entering AgbMain...`; incoming build
options remain intact. Native game changes are read-only repro observations.

This completes the EU/English sleep-choice/wake slice. Other languages/regions,
hardware sleep, save success/error handling, game-over choices, fusion
interactions and remaining file/post-credits paths keep the full context/state
catalog open. Runtime image publication, cache/presenter acceptance, resolved
intermediate effects and physical-device measurements remain separate tasks.


## Native Quest Status save choices and write failure (2026-10-26)

The next context/state slice covers native pause Save interactions on the
available EU/English fixture. `port_repro_background_save.c` adds 18 cases:
three installed health decorations times six input sequences. The driver
opens inventory, presses R to reach Quest Status, then LEFT / UP / A to open
Save through quest cursor 5. Later cases navigate the remembered cursor back
through 4 / 8 / 3 / 1 / 0 with real key edges before repeating that path.
Only health and the initial pause page are staged; graphics, cursor, timers,
save results and return transitions remain native.

The six sequences are A acceptance, DOWN / UP / A acceptance, DOWN / A refusal,
B / B refusal, DOWN / B refusal and A acceptance with EEPROM write failure.
B first selects Don't Save, and confirms only when it is already selected.
Read-only events in the native pause handlers record prompt, choice, answer,
success and error. Additive schema-v2 fields record save case, cursor, menu,
pause phase, input, transition timer and save-handler stage.

The fault is compiled only with `TMC_REPRO_HARNESS` and is inactive outside
this route's three error cases. It returns the EEPROM write error before
changing emulated storage. Both copies and native retries fail through the
normal `HandleSave` path; the driver never injects `SAVE_ERROR` or sets a menu
result. These are emulated-device write failures, not host ENOSPC/EIO or atomic
flush failures. Save persistence regressions separately exercise the production
storage layer; this route does not certify every host filesystem failure mode.

Six successful saves return automatically to Quest Status without a dismissal
key. Three errors display "Save was not performed correctly.", retain the
plain family and wait the native 60 ticks before allowing A dismissal. Because
trace boundaries precede native work, samples immediately following the error
event must show timer values 60 through 0 on 61 consecutive boundaries. The
driver waits another 24 ticks for a neutral result checkpoint. Nine refusals
return without saving. Native `ReadSaveFile` validates the successful record's
checksum and mutated health; refusals/errors compare the entire prior record
and status. Error fixtures follow successful saves, so preservation is checked
against a real valid prior record.

All cases verify quest/prompt source installation, heading identity, real input,
live-health mutation without decoration reload and START restoration to the
original game room. Four neutral source checkpoints per case cover quest ready,
prompt ready, prompt held and quest return; accepted cases add a result check.
Successful result/quest-return events share the same framebuffer. Thus 81 source
checks use 75 distinct neutral field frames, comparing 1,296 pixels at x=0..7
and 232..239, y=159 with independently ROM-decoded health fields. Error result
and prompt/held contact sheets were inspected. Restoration checks task/update/UI
and room identity; it does not assert normal player-control readiness.

Local evidence is under `.tmp/background-save-actions-audit/`:

- `tmc_pc-repro`, `tmc-qemu` and the build log preserve the ARM64 binary/wrapper.
- `eu-en/` retains the first driver failure: it incorrectly waited for a
  successful-save result screen instead of the native automatic quest return.
- `eu-en-corrected/` retains the remembered-cursor diagnostic failure at cursor 4.
- `eu-en-verified/save/` retains the final binary's complete raw trace and 1,442
  captures. The native process exits zero at frame 7,219. Its enclosing capture
  command reports failure because it loaded the older checker before the
  trace-boundary timing correction. `offline-summary.json` and `verification.json`
  record successful final validation of these unchanged raw artifacts; the
  command failure remains in `capture-verified.log`.
- `eu-en-menus/menus/` retains a successful native 130-case replay, completing at
  frame 10,033. Its capture command rejects the 130-case baseline with the old
  106/109-case-only helper. The helper now accepts 130 cases and has a new
  failure-oriented regression. Final `offline-summary.json` and
  `baseline-comparison.json` record all 260 ready/held RGBA comparisons passing
  against `.tmp/background-fusion-audit/eu-en-scale1/menus/`. Original traces,
  artist files and approved goldens remain unchanged.

Reproduce fresh captures with the final tools and a repro-enabled binary:

```sh
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --scenario save --language 1 \
  --output .tmp/background-save-new
python3 tools/tests/background_save_audit.py
```

The runner restricts this route to EU and isolates all real saves/configuration
in a temporary runtime directory. Twelve new save-checker tests plus the new
130-case baseline test and 111 retained audit/exporter tests pass. Catalog/
profile C/C++ agreement, camera/scroll checks and CPU PPU ASan/UBSan checks pass;
LeakSanitizer is disabled because the execution sandbox cannot run its tracer.
The persistence suite passes with the cached SDL headers supplied through `CC`.
The ARM64 `xmake build -y tmc_pc` and native EU startup under QEMU/dummy SDL pass,
reaching `Port layer initialized. Entering AgbMain...`. Incoming build options,
including `repro_harness=y`, are not changed.

This completes the EU/English Quest Status save slice. Inventory-entry save
interactions, host flush errors, other languages/regions, game-over choices,
fusion interactions and remaining file/post-credits paths keep the full
context/state task open. Runtime image publication, cache/composition, resolved
intermediate effects and physical-device measurements remain separate work.


## Native inventory-entry save choices and write failure (2026-10-26)

The `inventory_save` route extends the Quest Status save audit with a separate
18-case EU/English matrix: the same six acceptance/refusal/write-error inputs
at three installed health decorations. The existing `save` route and its case
IDs remain unchanged. Both routes share native save/readback/error handling;
entry and return checks distinguish inventory screen 1 / prompt 11 from Quest
Status screen 2 / prompt 10.

Only health and the initial inventory page are staged. Real UP / LEFT / A
input moves from inventory cursor 0 through bottle slot 12 to Save slot 16 and
opens the prompt. Subsequent openings reset the remembered cursor using RIGHT /
UP / UP / UP (12 / 8 / 4 / 0) before repeating that path. A read-only
`inventory_choice` event observes native navigation; no cursor, result, timer,
graphics or return transition is assigned by the driver. Successful saves and
refusals return automatically to inventory with Save selected; write errors
retain the prompt until the native 60-tick wait and A dismissal finish.

The checker requires the native 1 / 11 / 1 installation sequence, titled/plain/
titled family changes, heading removal/restoration, installed-health retention
across live-health mutation, checksum readback and unchanged prior records on
refusal/failure. START must restore the original game room and clear the audit
family. Restoration checks game task/update/UI state, not normal player-control
readiness. The EEPROM fault remains repro-only; host flush failures are outside
this route. Runtime saves/configuration are isolated in a temporary directory.

Reproduce with a repro-enabled binary and verified EU ROM:

```sh
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --scenario inventory_save --language 1 \
  --output .tmp/background-inventory-save-new
python3 tools/tests/background_save_audit.py
```

Final evidence is under `.tmp/background-inventory-save-audit/eu-en-final/`.
Both native process and capture command exit zero at frame 6,859. Its summary
and `verification.json` retain the complete 18-case validation, binary/trace
hashes and 1,131 captures: six successful saves, nine unchanged refusals, three
unchanged write failures and 18 original-room restorations. The 81 source
checkpoints compare 1,296 independent ROM-derived field pixels. Successful
result/returned-inventory events share a framebuffer. The prompt and error
dialog were inspected, and generated ready/held/returned contact sheets remain
beside the raw captures.

The first `eu-en/` attempt was externally terminated (exit 143) after frame
6,235 with an incomplete final JSON line and without the last case. Its raw
artifacts and `capture-outcome.json` remain separate; no complete-run success
is inferred from it. The final run uses the final rebuilt binary and completes
independently. `tmc_pc-repro`, `tmc-qemu` and `final-binary.json` identify that
successful capture executable.

All 126 audit-checker tests (13 new inventory checks), 11 exporter tests,
catalog/profile C/C++ checks, engine/scroll regressions and ASan/UBSan CPU PPU
checks pass (LeakSanitizer disabled for the sandbox). The final ARM64
`xmake build -y tmc_pc` and isolated EU QEMU/dummy-SDL smoke reach
`Port layer initialized. Entering AgbMain...`. Retained Quest Status and
130-case menu/fusion raw traces still pass the final tools. They were not
recaptured for this extension, so this records trace compatibility rather than
new native framebuffer parity. Incoming build options, original goldens and
artist files remain unchanged.

The shared checker tests now run against both routes, including wrong-entry
rejection, missing native events, navigation/answer input, error timing,
readback, source/heading retention, return-room checks and ROM-field mismatch.
The broad context/state catalog remains open for other languages/regions,
host flush errors, game-over/fusion interactions and remaining file/post-credits
paths. Runtime adapter, artwork loading/composition and intermediate effect
resolution remain separate tasks.


## Native Game Over save, retry, Continue and Quit (2026-10-26)

The repro-only `gameover` route stages zero saved and entity health while the
player has normal control in a real room, then lets native death animation,
room exit and `GameOverTask` run. It never assigns a player action, task,
menu result, cursor, timer, graphics or fade to enter Game Over. The existing
synthetic save/start-room bootstrap remains the fixture boundary.

Five cases cover A save/Continue, DOWN/UP/A save/Continue, DOWN/A refusal with
DOWN/UP/A Continue navigation, EEPROM write failure followed by A dismissal
and a successful native retry/Continue, and DOWN/A refusal followed by DOWN/A
Quit. Native save success proceeds automatically to the Continue prompt. The
error path waits 60 ticks, dismisses with A, reinstalls the Save prompt with
its own 30-tick input wait, then retries successfully. Three checksum-valid
readbacks verify saved health 24; refusals and the failed write compare the
entire prior record. The first successful save provides a valid prior record
for the later error case. The fault is repro-only and fails before EEPROM
mutation; it does not simulate a host flush failure.

Read-only hooks record Game Over installation, prompt, navigation, answer,
save result and Continue/Quit dispatch. Additive schema-v2 fields record case,
task/state/substate, fade state, cursor/menu/timer, real input, letter Y and
the native death flag. The checker requires actual death/exit before native
installation, real DPAD/A input, complete error timing and retry lifecycle,
four returns to normal player control in the original room, and the final
native soft reset back to the title task. Game Over resets health to 24,
independently of pause decoration thresholds.

The shared screen initializer still loads the critical decoration once per
opening (five `menu_decoration=2` events, because entry health is zero). This
does not classify the installed visible source: BG3 stays disabled (neutral
DISPCNT `0x1740`) and palette zero is black. The audit family remains unknown
with no heading, matching the catalog's generated-black policy. Neutral Save,
Continue, error and retry captures check every outer
8-pixel strip (6,144 pixels per frame), with central native foreground required
to remain visible. These samples certify the neutral field, not intermediate
masked-fade targets or a compositor implementation.

Reproduce with a repro-enabled binary and verified EU ROM:

```sh
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --scenario gameover --language 1 \
  --output .tmp/background-gameover-new
python3 tools/tests/background_gameover_audit.py
```

The final evidence is in `.tmp/background-gameover-audit/eu-en-final/`.
Both the native process and capture command exit zero; the process completes
at frame 4,153 with 260 captures. `verification.json` records binary/trace
hashes and final offline validation against the unchanged raw trace. All five
cases pass, including four native player-control returns, one native title
return and 12 neutral field checkpoints / 73,728 black-pixel checks. The native
Save and write-error frames were inspected. Generic contact sheets retain the
other captured animation/fade/return frames for review.

The first `eu-en/` attempt is retained as a failure: saved health alone did not
change entity health, so death never began and case 0 timed out. Its native
process exits 2 and the capture command exits 1. `capture-outcome.json` records
that fixture mistake; the corrected route sets both health values, with no
native death or menu action bypass. Its `tmc_pc-repro` binary is separate from
the successful `tmc_pc-final` / `tmc-qemu-final` artifacts.

Eleven new failure-oriented checker tests bring the audit suite to 137 tests;
all pass alongside 11 exporter tests, catalog/profile C/C++ agreement,
engine/scroll regressions and CPU PPU ASan/UBSan checks (LeakSanitizer disabled
for the sandbox). The final ARM64 `xmake build -y tmc_pc` passes and its isolated
EU QEMU/dummy-SDL capture reaches `Port layer initialized. Entering AgbMain...`.
Retained inventory-entry Save, Quest Status Save and 130-case menu/fusion traces
still validate with the final tools. They were not recaptured, so this is trace
compatibility rather than a fresh native framebuffer comparison. Incoming
build options, approved goldens and artist files remain unchanged.

Broader health/quest-state fixtures, save-and-Quit combinations, alternate
error dismissals, ignored keys, other languages/regions and intermediate effect
pixels remain open. The full context/state catalog, runtime adapter, PNG cache,
composition and device measurements remain separate tasks.


## Native fusion cancellation, mismatch and success handoffs (2026-10-26)

The separate `fusion_actions` route covers six paths at each of three installed
health decorations (18 cases): B, START and L cancellation; attempting an
incompatible piece, waiting for native recovery and cancelling with B; matching
with a direct return; and matching with a world-event/local-hint handoff.
This closes C1 in the [closure checklist](background-context-closure-checklist.md).

Each independent fixture resets its kinstone save block, gives two each of
pieces 109/110 through the native bag helpers, stages health 24/15/8 of 24 and
initializes a stable Tingle partner as in the retained layout matrix. Native
`MenuFadeIn(SUBTASK_KINSTONEMENU)` installs the screen and sorts the bag. This
stages the fusion subtask, not the initiating NPC conversation. No menu cursor,
result, animation timer, graphics or fade is assigned by the driver. Matching
SOURCE_FLOW uses piece 109 (shape 9); the other cases target KINSTONE_A (shape
11), so piece 109 mismatches and real DOWN navigation selects matching piece
110. Both shape decisions are made by the native regional event table reader.

Live health changes after readiness without replacing the installed decoration.
Ready/held and mismatch-return checkpoints retain the fusion family and heading
at (64,16). Native B/START/L input exits with fusion state 5 and unchanged bag/
count/ownership. A mismatch executes the native bounce/return before B exit.
Matches execute the local brightness progression 0..14 then 0, award one fused
bit/count and consume exactly one matching piece. The success message, 120-tick
animation and spark sequence run natively. The driver advances the native
message with released A key edges; it does not clear message state directly.

SOURCE_FLOW has no outgoing effect subtask and returns directly. KINSTONE_A
natively dispatches world event 20, then installs its local map hint. The
fusion-family observation clears at world-event installation and stays unknown
through the procedural hint and original-room restoration. The hint's native
shared loader installs an overwritten decoration at the mutated health; that
load must not reactivate a fusion-family image. Reward/global flags outside the
per-case kinstone block are not reset between cases; repeated native event
execution and restoration are covered, not every reward/quest-state fixture.
Restoration checks game task/update state and original room, not normal player
control or completion of an initiating NPC dialogue.

Read-only events observe choices, attempts, matches/mismatches, award,
cancellation and outgoing native subtask. Additional schema-v2 fields record
case, task/subtask, selection, native menu/overlay/column/timer, input, brightness,
target/result, fused count/bit and both bag amounts. The checker requires the
full event order, key edges, installed-health lifetime, flash progression,
consumption invariants and all 18 restorations. Existing seven-layout/health
installation captures remain separate evidence rather than repeating every
interaction against all seven names.

The pixel check independently decodes the three exported native fusion recipes
without opening artist artwork. It compares the complete static bottom row at
39 ready/held/mismatch-return checkpoints (9,360 pixels). The stone frame masks
both lower corners, so those corners alone cannot distinguish installed health.
The complete ROM-decoded rows differ across all three decorations and match all
retained native samples, including after live-health mutation.

Evidence is in `.tmp/background-fusion-actions-audit/eu-en/`. The native process
completes successfully at frame 7,835, with 434 captures, 12 cancellations,
three mismatches, six awards and three world-event/local-hint handoffs. The
capture command exits 1 because its then-loaded checker rejects the original
corner-only mask. `offline-summary.json` and `verification.json` record final
successful trace/pixel validation against the unchanged raw artifacts, binary/
trace hashes and the original failure status. No native failure or incomplete
run is substituted for success. Ready/held contact sheets and representative
native views were inspected; original traces, goldens and artist files remain
unchanged.

Reproduce fresh captures with the final checker:

```sh
python3 tools/capture_background_audit.py --binary /path/to/tmc-qemu \
  --rom /path/to/EU.gba --scenario fusion_actions --language 1 \
  --output .tmp/background-fusion-actions-new
python3 tools/tests/background_fusion_actions_audit.py
```

Eleven new failure-oriented tests bring the audit suite to 148 passing tests;
11 exporter tests, catalog/profile C/C++ checks, engine/scroll regressions and
CPU PPU ASan/UBSan checks pass (LeakSanitizer disabled for the sandbox). The
ARM64 `xmake build -y tmc_pc` and isolated EU QEMU/dummy-SDL capture reach
`Port layer initialized. Entering AgbMain...`. Retained Game Over, inventory/
quest Save and 130-case menu/fusion raw traces still validate with the final
tools; they were not recaptured for a fresh native pixel comparison. Incoming
build options and profile availability stay unchanged. C2–C7 in the closure
checklist remain; intermediate effect pixels and runtime artwork publication/
loading/composition stay with their own parent tasks.
