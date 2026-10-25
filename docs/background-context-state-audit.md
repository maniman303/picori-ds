# Background context/state audit

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
several. `--language` uses the public language preference (1 is English), not
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
No local-hint PNG or new swatch is exported. There are still 23 shared artwork
filenames; `PORT_BACKGROUND_LOCAL_MAP_HINT` appends a procedural catalog entry.

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
