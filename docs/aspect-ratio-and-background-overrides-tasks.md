# Aspect ratio and background implementation tasks

Tracks implementation of [the design](aspect-ratio-and-background-overrides-plan.md).
Work proceeds in the order below. Checked items describe implemented contracts
or completed checks, not a claim that the renderer supports the new profiles.

## 1. Catalog and baseline

- [x] Add a port-owned, C-compatible catalog for the 23 shared image names (including six menu health/family variants).
- [x] Distinguish live/unknown screens, image families, generated black,
  the logo field, and live palette-zero fields. Disable every source in parity.
- [x] Test actual catalog filenames against the standalone exporter's public
  names. The map-hint reference swatch must never become a runtime image.
- [x] Define version 1 of the five profile sizes/directory keys and native
  centering. Except for 4:3 at 256 × 192, keep height 160 and ceiling-round
  fractional widths. Reject insufficient capacity rather than truncate geometry.
- [x] Verify the existing RG cross-build and existing ROM-free renderer/engine
  regressions before changing rendering behavior.
- [x] Capture native/wide named scenes with a repro-enabled binary; retain
  original goldens and local before/after captures separately.
- [x] Add native state-audit capture routes for file operations, opening/retold
  prologue, retained credits and the local fusion map reveal. Correct the local
  hint's source classification without adding an artwork filename.
- [x] Audit pause-menu decoration thresholds and installed-state retention across
  functional screens and all 22 detail-map recipes on the available EU fixture;
  compare exposed inventory pixels against independently decoded ROM references.
- [ ] Finish the full context/state catalog across available regions, including
  dungeon/fusion/fast-travel/figurine screens and interaction/error paths;
  retain the completed file-operation, prologue, credits and pause-menu evidence.
- [ ] Resolve per-context effect targets from unfaded source coverage, including
  white logo BG versus backdrop, title local alpha, and masked palette fades.
- [ ] Measure RG scale-1 frame times, PSS/RSS and driver memory on device.

The [context/state audit](background-context-state-audit.md) records the available
EU evidence and the remaining functional/regional gaps. Capture route coverage
is distinct from completing the full visual catalog or compositor effect audit.

Implementation: `port/port_screen_background.{h,c}` owns semantic source policy;
`port/port_viewport.{h,c}` owns logical profile contracts. Neither reads game
globals, allocates, or opens files. Camera/shadow/presentation callers now
consume the published profile geometry. Catalog selection is
explicit: the future state adapter must identify installed content, rather
than infer an image from `TASK_GAME` or a transient credits command. The catalog
groups shared settings; effect policies remain specific to the active screen.
Unknown screens return no source until their state mapping is verified.

The profile registry now drives runtime target widths through a shared geometry
snapshot. Release helpers now request width capacity 569 and publish the
bounded 192-row raster capacity. Native, 16:9 and 21:9 remain available; 4:3
and 32:9 stay unavailable pending scene, streaming, effect, individual-sprite
and layout validation. Configs retain those preferences but render effective
native geometry. N64 remains native-only. Optional upscale pools retain their
`capacityWidth * 160` source-pixel budgets.

Image resolution validation must use the **selected aspect ratio**, independent
of window dimensions and internal scale:

| Aspect ratio | Directory | Required PNG dimensions |
| --- | --- | --- |
| 3:2 | `32` | 240 × 160 |
| 4:3 | `43` | 256 × 192 |
| 16:9 | `169` | 285 × 160 |
| 21:9 | `219` | 374 × 160 |
| 32:9 | `329` | 569 × 160 |

For every ratio other than 4:3, width is `ceil(160 * ratioWidth / ratioHeight)`.
Round fractional widths upward to an integer pixel; do not force an even width
or tile alignment. Native-content centering continues to use floor offsets.

## 2. Geometry and settings

- [x] Append the 4:3 aspect enum without changing existing serialized IDs;
  integrate canonical profile lookup and skip unsupported modes visibly.
- [x] Publish a base geometry snapshot at startup and simulation boundaries:
  selected/effective profile, capacity/pitch, canvas, native content, UI safe
  rectangle, parity/unsupported flags and geometry generation. Repeat presents
  consume the same snapshot; pending config changes wait for publication.
- [x] Extend that snapshot to own scene capability flags and world-aperture
  decisions at room/transition boundaries. The complete planned snapshot must
  include selected/effective profile, capacity/pitch, canvas, native content,
  world aperture, UI safe rectangle, capability flags and generation.
- [x] Migrate target-width wrappers to consume that snapshot. Window
  resizing must affect presentation only. Preserve room/transition safety.
- [x] Remove widescreen toggle state, config entry/API, UI and indirect gates;
  erase the legacy JSON key on save and pin capture tools to explicit profiles.
- [x] Add bounded visible height to `PPUMemory` separately from hardware timing,
  with zero retaining the native default and N64 remaining capped at 160.
- [x] Version static captures as PPU2 with explicit width/height/pitch; update
  both readers and retain native PPU1 compatibility. Validate malformed inputs.
- [x] Carry normalized height through software rendering, post-processing,
  presentation, screenshots, overlay geometry and affine OBJ supersampling;
  keep unsupported taller GPU rasterization on software fallback.
- [x] Audit initial height-related scratch/storage changes: retain the existing
  160-row IO snapshots and upscale pools; bound affine scratch at 192 rows.
- [x] Finish allocation/path audits before increasing build capacity to 569 by
  192, including optional filter/voxel/HD-text behavior and scene composition.
  See the [presentation allocation audit](presentation-allocation-audit.md).
- [x] Update `build.py`, RG builds and tools together. Preserve N64 native-only
  constraints and verify original native golden pixels before taller rendering.

## 3. World rendering

- [x] Separate the original hardware register sequence from bounded rendered
  rows. Guard HDMA against extra/negative callback lines; stable-state scenes
  can render extra rows and continue affine references without extra IO copies.
- [ ] Classify HDMA continuations and implement full-canvas effect geometry.
  Until then, active callbacks cap the effective aperture at 160. This is an
  interim safety gate while 4:3 remains disabled, not completed 4:3 behavior.
- [x] Add a shared two-axis camera-rest formula and connect port camera snaps,
  initialization, expanded-height follow and script completion. Preserve native
  vertical pacing and fix small-room clamp agreement.
- [x] Share normal map fill/upload row counts by effective world height: 23 rows
  at 160 and 27 at 192 for the unshaken scroll offset. Validate real normal/shadow
  pixels at the bottom/right seam without enabling taller gameplay.
- [x] Record the [vertical streaming audit](vertical-world-streaming-audit.md),
  including the town/village graphics loaders and special BG/transition paths.
- [x] Add shake-aware tile-row coverage/padding with bounded backing copies;
  test positive/negative offsets at every fractional camera phase. Keep native
  uploads unchanged and expose unsupported-wrap classification to scene selection.
- [ ] Finish room-aperture publication, unsupported-shake fallback selection,
  per-manager graphics residency and upload timing before exposing rows 160–191.
- [x] Add signed per-slot OBJ positions, attribute validation, build/DMA lifetime
  and explicit hidden/parked semantics to the software/supersampled paths.
  Verify real Y=160/191 pieces and X > 511 without changing packed OAM.
- [x] Migrate generic entity/rectangle culling and piece clipping to effective
  world height. Record the [sprite visibility audit](sprite-coordinate-audit.md).
- [ ] Complete individual entity/AI/parking and special-effect audits; verify
  all direct OAM consumers and scene capability decisions before enablement.
- [ ] Extend transitions one validated case at a time; test camera completion.

## 4. Layout

- [ ] Translate each complete fixed-screen composition once using the native
  content rectangle; include HD text and any hit testing.
- [ ] Resolve functional live gameplay separately from pause/map/fusion states.
- [ ] Implement semantic corner HUD anchors and bottom dialogue geometry shared
  by BG/OAM, portrait, choices, cursor, arrows and HD text; keep banners separate.

## 5. Margin composition

- [ ] Add a game-state adapter selecting catalog contexts at scene boundaries;
  track the installed prologue/credits illustration across text commands/fades
  and the installed normal/damaged/critical menu or kinstone decoration.
- [ ] Publish source, native coverage and resolved effect data to a compositor
  with no task IDs or config parsing in its pixel loops.
- [ ] Implement procedural fields and black fallback with correct global effects;
  preserve every native foreground pixel and exclude gameplay completely.
- [ ] Retire the old ImGui menu side-art path as its supported screens migrate;
  verify software/GPU presentation and raster fallbacks, filters and persistence.

## 6. Artwork loading

- [ ] Decode exact-size ordinary PNGs with libpng limits and safe error cleanup;
  resolve only the selected ratio and exact shared family/health filename. Never
  substitute another health state or menu family. Validate width and height
  against the table above, including ceiling-rounded widths.
- [ ] Test accepting each selected profile's exact size and rejecting a file
  sized for another ratio, floor-rounded widths (284/373/568), and wrong heights.
- [ ] Cache current/incoming image bytes within 8 MiB steady / 16 MiB peak;
  cache failures, clear stale images, and perform no hot-path I/O/allocation.
- [ ] Add explicit reload, corruption/size/alpha fixtures and packaging checks
  that preserve custom artwork. Keep raw asset mods independent.

## 7. Export synchronization

- [x] Add a ROM-free runtime-catalog/exporter agreement gate.
- [ ] Validate all native export recipes against available regional ROMs after
  context/effect auditing; preserve artist edits and existing export names.

## 8. Acceptance and budgets

- [ ] Complete the design's regional, functional-screen, world, HUD/dialogue,
  renderer, intermediate-fade, PNG failure and profile-switch coverage.
- [ ] Measure RG performance, memory/cache limits and 1/2/4-thread behavior;
  record unsupported optional-path fallbacks and unavailable fixtures explicitly.
- [ ] Run full staging/RG packaging checks when runtime asset packaging changes.

## Initial foundation verification (2026-10-24)

Available configuration is the existing Linux ARM64 RG cross-build, multi-region,
width capacity 384, GPU presentation enabled, repro harness disabled. The local
ROM fixture is EU. A baseline `xmake build -y tmc_pc` passed. The EU binary reached
`Port layer initialized. Entering AgbMain...` under `qemu-aarch64` with dummy
video/audio, `--no-audio`, a temporary runtime directory and the explicit ROM
path. Emulation is a startup check, not a device performance measurement.

ROM-free checks:

```sh
python3 tools/screen_background_regression_test.py
python3 tools/tests/export_screen_backgrounds.py
python3 tools/widescreen_engine_regression_test.py
python3 tools/widescreen_scroll_regression_test.py
python3 tools/widescreen_regression_test.py
```

All passed. The new contract harness compiles the production modules as C and
links both C11 and C++17 consumers with warnings as errors and UBSan. It checks
all five exact geometries, odd-width centering, capacity rejection, fail-closed
unknown/parity contexts, procedural sources with no filenames and exporter
agreement. It does not validate automatic game-state selection or pixels.

The initial parity invocation could not find the default `baserom.gba`.
Subsequent discovery located the EU fixture under `roms/`, but the current
binary has the repro harness disabled, so deterministic scene/PPU captures
remain pending. No golden hashes were changed. USA/JP fixtures, visual/effect
audits and physical RG measurements remain open. No device deployment occurred.

## Geometry/settings verification (2026-10-24)

The widescreen toggle and window-pixel publication API are removed. Selected
profiles now determine target world width; existing room-content caps,
fixed-screen fallbacks and transition safety checks remain. Startup window
width uses the effective profile rather than maximum allocation capacity.
The ImGui selector lists unsupported profiles disabled and explains effective
native mode under Console-Parity or an unsupported saved preference. Legacy
controller-menu cycling skips unsupported profiles. The retired
`TMC_WS_VIEW_WIDTH` environment variable is ignored.

Publication happens at startup after CLI overrides and in `VBlankIntrWait`
after completed-frame presentations/input, before VBlank shadow construction
and the next engine camera update. No engine source files were changed.
The snapshot owns selected/effective aspect, canvas/native/UI geometry,
capacity/pitch, parity/unsupported flags and generation. At that stage, scene capability and
room/transition aperture publication remained separate; see the subsequent
scene/aperture verification below.

Verification completed:

- RG ARM64 multi-region build with the repro harness enabled for captures;
  original release configuration restored afterward with the harness disabled.
- `tools/ppu_parity_check.sh` using the local EU ROM and a QEMU wrapper:
  both committed scenes (`intro_logo`, `title`) match original Tier A and B
  hashes. At that stage the PPU1 reader required the oracle's default width
  240; a capacity-384 build rendered extra pixels. The subsequent PPU2 reader
  now honors stored geometry independently of compiled capacity. Committed
  golden hashes remain unchanged.
- Production C/C++ geometry tests and UBSan config regressions: enum stability,
  selected/effective profiles, unsupported dimensions, snapshot publication,
  unchanged generation for unchanged geometry, invalid capacity/pitch,
  legacy toggle removal, alias parsing, 4:3 serialization, parity restoration,
  and skipping unavailable modes.
- Existing engine, scroll and CPU PPU regression tools; all four scene-tool
  regressions using Pillow in a temporary Python environment.
- EU field captures under QEMU/software presentation: 16:9 produces 285 × 160
  in both 640 × 640 and 960 × 540 windows with byte-identical base pixels;
  21:9 produces 374 × 160 in a 480 × 640 window. Deliberately setting the
  retired width override to 240 does not change either selected profile.
- An EU native field capture reaches the requested room and produces pixels,
  but the scene baseline command reports a missing reference: the committed
  presentation hashes cover USA only. This is not a verified EU field pixel
  comparison. The local artifact remains separate; no baseline was approved.

True 256 × 192 rendering, full 32:9, fixed-screen canvas composition, GPU
presentation execution, other regional fixtures and device profiling remain
pending. Current image-size contracts already describe their target dimensions;
they do not imply those renderer paths are enabled.

## PPU height/capture verification (2026-10-24)

`PPUMemory.frame_height` now distinguishes render rows from the native hardware
constant. The software renderer normalizes height to at most 192 on PC / 160
on N64. Stable IO supports tiled/affine rendering through row 191. Unknown
scanline effects retain a 160-line effective aperture; their callbacks never
read an extra table row. Native window and OAM Y interpretation stays intact.
Gameplay profile availability is unchanged: camera/streaming and layout are
still prerequisites for enabling 4:3.

Presentation uses the normalized extent, and persistence history resets when
geometry changes. Existing upscale workspaces retain their sizes and their
capacity checks; taller dimensions do not multiply maximum-scale BSS in this
step. The surface fallback wraps the existing framebuffer through the bounded
render height. The GPU raster path rejects non-native height before touching
its native-sized register buffers; SDL_GPU presentation can still receive a
software-rendered frame. HD text now receives actual height and retains its
existing non-native-height rejection until shared layout work lands. Shared
memory BG/OBJ planes explicitly retain their native 240 × 160 geometry.

New runtime memory is bounded: `PPUMemory` grows by two bytes; software
scanline-control/affine scratch capacity grows by 704 bytes per frame on PC
(384 bytes in the separate prepare pass). The 160 KiB IO snapshot array and
the large upscale/persistence pools are unchanged. N64 scratch bounds stay
native. These are source-size calculations, not RSS/PSS/device measurements.

Captures use [PPU2](ppu-snapshot-format.md), storing effective width, height and
pitch in explicit little-endian fields. Both standalone readers validate
metadata/payload sizes and retain native PPU1 support. Static capture limits
(no HDMA replay, shadow maps or port sprite metadata) remain explicit.

Verification:

```sh
ASAN_OPTIONS=detect_leaks=0 python3 tools/ppu_geometry_regression_test.py
python3 tools/widescreen_regression_test.py
python3 tools/widescreen_engine_regression_test.py
python3 tools/widescreen_scroll_regression_test.py
python3 tools/screen_background_regression_test.py
```

All passed. The new harness runs AddressSanitizer/UBSan at width capacities
240/384 and checks the N64 native-height guard. Leak detection is disabled for
the restricted process environment; address/undefined-behavior checks remain
active. Independent expectations cover distinct tile/affine pixels at rows
159/160/191, pitch/bottom canaries, native/zero/oversized height, forced blank,
hidden OAM Y, supersampled sprites, a cropped frame's full hardware sequence,
and direct HDMA bounds. Capture tests cover PPU2 round trips, PPU1 defaults,
malformed/truncated inputs, and real oracle/benchmark checksums for native and
256 × 192 fixtures with padded pitch. An additional two-thread OpenMP run and
the existing affine-latch tests also passed.

The RG cross-build and restored-release EU startup smoke under QEMU passed,
reaching `Port layer initialized. Entering AgbMain...`.
Both EU native parity scenes retain their original
Tier A/B hashes with PPU2 captures. Local 16:9 square/wide-window and 21:9
tall-window field captures are byte-identical to the previous geometry step.
No golden hashes or ROM-derived fixtures were committed. The normal RG release
configuration is restored after capture checks; no device deployment occurs.

Remaining gates: tall gameplay streaming/camera/culling, semantic HUD/dialogue
layout, fixed-screen centering, classified raster effects, optional backend
execution and device budgets. These checks establish the height interface and
preservation of existing output, not completion of true 4:3 gameplay.

## World camera/normal-streaming verification (2026-10-24)

Both camera axes now have port wrappers backed by the same widened clamp
helper. Native height remains explicitly gated at 160. Normal map copy and
VRAM upload use one row-count contract, preparing the 192-line case without
expanding GBA constants or rewriting rolling-transition code. The source audit
distinguishes tilemap addressing from character/palette residency and records
remaining special-manager/shake gates.

New verification command:

```sh
ASAN_OPTIONS=detect_leaks=0 python3 tools/world_view_regression_test.py
```

It passes at capacities 240 and 384 with real camera/copy/render bodies and
controlled 160/192-line apertures. Tested follow/snap/initialization/wait goals
agree; native edge-step behavior and 4-pixel pacing remain intact. Tests cover
odd-width centers, small rooms, prior-boundary recovery, relative origins,
region intersection, 23/27-row footprints and bounded map reads. Rendered pixels
match independent world coordinates through all 16 × 16 fractional camera
phases at native/bottom/right seams. This is synthetic-resident-graphics proof,
not complete manager residency validation.

Existing engine, scroll, CPU PPU and C/C++ profile regressions pass. The RG build
and restored-release EU startup smoke under QEMU pass; both EU native parity
scenes retain their golden Tier A/B hashes.
The 16:9 square/wide-window and 21:9 tall-window field captures remain
byte-identical to the earlier implementation steps. Release configuration is
restored after capture checks; no device deployment or golden update occurs.

## Signed OBJ / shake-padding verification (2026-10-24)

`port_oam` adds bounded build/latched coordinate tables (2,560 bytes on the tested
ABI) beside packed hardware OAM, sharing the existing voxel-tag publication
boundary. The PPU consumes only stamped signed positions and hidden flags;
normal and supersampled sprites share the resolver. Native/parity binding stays
off, and extended canvases keep binding during temporary native raster fallback.
GPU rasterization falls back to software while metadata is bound. Reset/clear
hooks invalidate metadata; quicksave simulation restores keep unchanged OAM and
its matching latched metadata together. Saved formats and entity layouts stay
unchanged.

The normal streaming test now checks offsets -8/-3/0/+3/+8 through every
fractional camera phase. At height 192, positive +3/+8 pads to 28 rows. The
coverage classifier rejects offset ranges that wrap past the current map origin;
the backing-copy limit is always at most 32 rows and never sends zero-length
DMA. Runtime height remains gated at 160, whose 23-row upload is preserved.

```sh
ASAN_OPTIONS=detect_leaks=0 python3 tools/oam_position_regression_test.py
ASAN_OPTIONS=detect_leaks=0 python3 tools/world_view_regression_test.py
ASAN_OPTIONS=detect_leaks=0 python3 tools/ppu_geometry_regression_test.py
```

All passed, including expected pixels for emitted direct/entity sprites, signed
Y/X limits, affine/double-size/supersampled sprites, disabled/parked entries,
stale stamps, publication/reset, native-aperture recovery and generic culling.
Existing scroll/profile/PPU and regional sprite regressions also pass. The RG
build, restored-release EU startup smoke and native Tier A/B parity pass; the three local 16:9/21:9 field captures
remain byte-identical. The original release configuration is restored after
capture checks, with no golden update or device deployment.

This completes generic sprite-coordinate and normal shake-padding plumbing.
It does not enable 4:3 or 32:9: remaining work includes scene/world-aperture
publication, individual entity/special-manager coverage, graphics residency,
classified raster effects, fixed-screen composition and gameplay UI layout.


## Scene/aperture snapshot verification (2026-10-25)

The geometry snapshot now includes a centered world rectangle and explicit
horizontal-world, installed-map, native-transition and native-content policy.
The pure C viewport module resolves the aperture from those inputs without
reading game state. Room caps leave the selected canvas, native-content
rectangle and UI safe rectangle unchanged. Native/parity/unsupported profiles
still resolve to 240 × 160; vertical world reveal remains disabled.

The engine adapter publishes after `LoadRoomGfx` (including reloads of the same
room), before scroll/camera snap work, on transition completion before camera
restoration, and before VBlank shadow construction. Painted content width is
sampled at room load and held throughout that room rather than rescanned and
ratcheted every VBlank. Room keys retain both origins independently, avoiding
the previous XOR collisions. Camera/culling wrappers consume the published
world dimensions. Presentation consumes the published installed-map capability
and world width, so repeated presents do not query mutable task/room globals.
Known repeating overlay eligibility uses the same predicate as shadow creation.

Verification completed:

- Production C11/C++17 geometry contracts under UBSan: centered room aperture,
  unchanged canvas/UI, scene generation, unchanged repeated publication,
  profile/parity changes, invalid room bounds and the retained height gate.
- Production engine/renderer helper regressions: frozen presentation after
  mutable task/room changes, narrow rooms, installed maps versus fixed overlays,
  held content decisions and explicit same-room reloads, all three native
  transition actions and wide camera recovery, plus existing shadow, dialogue
  and repeating-overlay checks.
- `ASAN_OPTIONS=detect_leaks=0 python3 tools/world_view_regression_test.py`
  at width capacities 240/384, and the existing scroll and CPU PPU regressions.
- `xmake build -y tmc_pc` with the existing RG ARM64 release configuration;
  EU startup under QEMU/dummy video/audio reaches
  `Port layer initialized. Entering AgbMain...`.

No profiles were enabled, PPU pixel loops changed, golden hashes updated or
assets packaged. This step does not classify individual managers, vertical
residency, shake fallbacks or raster continuations. Regional scene captures,
native pixel comparisons for this step, GPU execution and physical RG budgets
remain unverified; the repro harness remains disabled in the release config.


## Presentation allocation/path verification (2026-10-25)

The [allocation audit](presentation-allocation-audit.md) records storage formulas,
width-384/569 totals, optional-path fallback decisions and the pending compositor
boundary. Upscale pools retain their pixel-count budgets, which at capacity 569
fit every v1 profile without reserving the full 569 × 192 Cartesian envelope at
every scale. Arithmetic/pitch checks now reject invalid inputs before copying.
Voxel, custom-preset and HD-text gates keep unsupported layouts on their normal
2D/stock-filter/original-text fallbacks. Both SDL presenters and overlays now use
one viewport fitting function consuming the published effective aspect.

Verification:

```sh
ASAN_OPTIONS=detect_leaks=0 python3 tools/presentation_geometry_regression_test.py
ASAN_OPTIONS=detect_leaks=0 python3 tools/ppu_geometry_regression_test.py
bash tools/hd_text_regression_test.sh
python3 tools/screen_background_regression_test.py
python3 tools/widescreen_engine_regression_test.py
python3 tools/widescreen_scroll_regression_test.py
python3 tools/widescreen_regression_test.py
xmake build -y tmc_pc
```

All pass, plus EU startup under QEMU/dummy video/audio. The new sanitizer harness
runs production presentation helpers, upscaling and all CPU filters at capacities
240/384/569, including padded pitch and exact/over-capacity storage. The existing
PPU geometry harness now also checks a full synthetic 569 × 192 frame and retains
its N64 native guard. No PPU source, golden hash, build capacity or profile
availability was changed. RG device budgets, GPU execution and regional native
pixel captures remain unverified. Build/tool capacity synchronization is next.


## Menu health/family split restored (2026-10-25)

General-menu and kinstone backgrounds each have explicit normal, damaged and
critical images: six filenames, 23 catalog images total, plus the reference-only
map-hint swatch in native exports. The exporter uses graphics groups 0x56/0x57/
0x58 from `sub_080A4D34` for each family's health decoration and retains the
representative fusion layout across its seven variants. The old unsuffixed
filenames are retired. Runtime selection must follow the installed decoration;
missing art uses black rather than another state/family. Full state-adapter,
PNG loading and composition implementation remains pending.


Verification: C11/C++17 catalog/exporter agreement and UBSan checks pass for all
23 image names; exporter recipe tests cover all six exact graphics-group/layer
combinations. The local EU export yields 24 PNGs with three distinct pixel
outputs in each menu family. Existing normal reference/artwork files in `32/`
and `43/` were renamed to the explicit `_normal` names with their bytes
preserved. Damaged/critical native references and the contact sheet/README were
regenerated; custom 4:3 damaged/critical artwork remains to be supplied. The RG
ARM64 build and EU startup smoke under QEMU pass. Regional captures and runtime
state-adapter/compositor integration remain pending.


## Build-capacity synchronization and named captures (2026-10-25)

`build.py`, the RG helper and Android CI now request width capacity 569.
Direct xmake builds retain the native-width default. Runtime config publishes
`MODE1_MAX_RENDER_HEIGHT` (192 on PC) as capacity, while selected/effective
profiles and world apertures still obey all existing readiness gates. No 4:3
or 32:9 gameplay is enabled. N64's profile registry now explicitly accepts
only native, even if callers supply wider/taller capacities; its raster and
framebuffer remain 240 × 160.

The repro-enabled ARM64 builds at capacities 384 and 569 were compared using
the local EU ROM, QEMU, dummy video/audio and software presentation:

- Both native PPU golden scenes (`intro_logo`, `title`) pass their original
  Tier A and B hashes at both capacities.
- All five named scene cases were captured before and after: file selection
  (two images), Hyrule Field, inventory, bottom dialogue and top dialogue.
  Each was run with both `original` (240 × 160 base pixels) and `widescreen`
  (960 × 540 presentation, 16:9 logical profile). All 12 comparison pairs have
  identical decoded pixels/hashes. The local contact sheet was inspected.
- Before/after captures and logs are separate under
  `.tmp/aspect-capacity-batch/before/` and `after/`; local reference hashes are
  `before-original.json` / `before-widescreen.json`. `summary.json` records the
  comparisons. These local references are not new approved release goldens.
  Committed PPU and presentation golden files retain their original hashes.
- World streaming and signed-OBJ tests now include exact capacity 569 alongside
  prior capacities; config tests run at 384/569 and verify published height,
  pitch, and unsupported saved-profile fallback. Standalone PPU readers render
  native, tall and 569-wide PPU2 fixtures, including a synthetic 569 × 192
  extent. Presentation workspace tests and the N64 native guards pass under
  ASan/UBSan. Catalog, engine, scroll and scene-tool regressions pass.

The RG release helper stages the local slim package at `dist/rgdsplus/` with
capacity 569 and the repro harness disabled. Build/package verification and EU
startup under QEMU are local checks; there is no device deployment. The memory
increase follows the prior allocation audit, not a device PSS/RSS measurement.
Android CI configuration was synchronized but Android execution, GPU execution,
USA/JP fixtures and physical RG budgets were unavailable. Full visual context/
effect auditing and final regional/platform acceptance remain open.
