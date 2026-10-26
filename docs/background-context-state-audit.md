# Background context/state audit

The [menu-family revision](menu-background-family-revision.md) supersedes the
original shared-menu classification below. The catalog/exporter now have 32 artwork names
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
`intro`, `files`, `local_hint` or `menus` restricts the run; repeat the flag to select
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
