# Aspect ratio and background implementation tasks

Tracks implementation of [the design](aspect-ratio-and-background-overrides-plan.md).
Work proceeds in the order below. Checked items describe implemented contracts
or completed checks, not a claim that the renderer supports the new profiles.

## 1. Catalog and baseline

- [x] Add a port-owned, C-compatible catalog for the 19 shared image names.
- [x] Distinguish live/unknown screens, image families, generated black,
  the logo field, and live palette-zero fields. Disable every source in parity.
- [x] Test actual catalog filenames against the standalone exporter's public
  names. The map-hint reference swatch must never become a runtime image.
- [x] Define version 1 of the five profile sizes/directory keys and native
  centering. Except for 4:3 at 256 × 192, keep height 160 and ceiling-round
  fractional widths. Reject insufficient capacity rather than truncate geometry.
- [x] Verify the existing RG cross-build and existing ROM-free renderer/engine
  regressions before changing rendering behavior.
- [ ] Capture native/wide named scenes with a repro-enabled binary; retain
  original goldens and local before/after captures separately.
- [ ] Visually verify the full context/state catalog across available regions,
  especially file-operation transitions, prologue and retained credits pages.
- [ ] Resolve per-context effect targets from unfaded source coverage, including
  white logo BG versus backdrop, title local alpha, and masked palette fades.
- [ ] Measure RG scale-1 frame times, PSS/RSS and driver memory on device.

Implementation: `port/port_screen_background.{h,c}` owns semantic source policy;
`port/port_viewport.{h,c}` owns logical profile contracts. Neither reads game
globals, allocates, or opens files. Camera/shadow/presentation callers now
consume the published profile geometry. Catalog selection is
explicit: the future state adapter must identify installed content, rather
than infer an image from `TASK_GAME` or a transient credits command. The catalog
groups shared settings; effect policies remain specific to the active screen.
Unknown screens return no source until their state mapping is verified.

The profile registry now drives runtime target widths through a shared geometry
snapshot. Builds with capacity 384 support native, 16:9 and 21:9. The settings
list also shows 4:3 and 32:9 as unavailable until taller raster rendering and
wide signed OBJ coordinates are implemented. Configs retain those preferences
but render effective native geometry. Framebuffer capacities remain unchanged.

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
- [ ] Extend that snapshot to own scene capability flags and world-aperture
  decisions at room/transition boundaries. The complete planned snapshot must
  include selected/effective profile, capacity/pitch, canvas, native content,
  world aperture, UI safe rectangle, capability flags and generation.
- [x] Migrate target-width wrappers to consume that snapshot. Window
  resizing must affect presentation only. Preserve room/transition safety.
- [x] Remove widescreen toggle state, config entry/API, UI and indirect gates;
  erase the legacy JSON key on save and pin capture tools to explicit profiles.
- [ ] Add visible height to the PPU contract and version snapshot formats;
  audit allocations before increasing build capacity to 569 by 192.
- [ ] Update `build.py`, RG builds and tools together. Preserve N64 native-only
  constraints and verify original native golden pixels before taller rendering.

## 3. World rendering

- [ ] Separate the 160 hardware register rows from bounded rendered rows;
  classify HDMA continuations without reading beyond original tables.
- [ ] Add two-axis camera-rest and room-aperture helpers, then audit streaming
  managers/tile residency before exposing rows 160–191.
- [ ] Audit culling, hidden OAM and signed port coordinates, including X > 511.
- [ ] Extend transitions one validated case at a time; test camera completion.

## 4. Layout

- [ ] Translate each complete fixed-screen composition once using the native
  content rectangle; include HD text and any hit testing.
- [ ] Resolve functional live gameplay separately from pause/map/fusion states.
- [ ] Implement semantic corner HUD anchors and bottom dialogue geometry shared
  by BG/OAM, portrait, choices, cursor, arrows and HD text; keep banners separate.

## 5. Margin composition

- [ ] Add a game-state adapter selecting catalog contexts at scene boundaries;
  track the installed prologue/credits illustration across text commands/fades.
- [ ] Publish source, native coverage and resolved effect data to a compositor
  with no task IDs or config parsing in its pixel loops.
- [ ] Implement procedural fields and black fallback with correct global effects;
  preserve every native foreground pixel and exclude gameplay completely.
- [ ] Retire the old ImGui menu side-art path as its supported screens migrate;
  verify software/GPU presentation and raster fallbacks, filters and persistence.

## 6. Artwork loading

- [ ] Decode exact-size ordinary PNGs with libpng limits and safe error cleanup;
  resolve only the selected ratio and shared family filename. Validate width
  and height against the table above, including ceiling-rounded widths.
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
capacity/pitch, parity/unsupported flags and generation. Scene capability and
room/transition aperture publication remain a separate unfinished subtask;
the old room safety helpers still make those decisions.

Verification completed:

- RG ARM64 multi-region build with the repro harness enabled for captures;
  original release configuration restored afterward with the harness disabled.
- `tools/ppu_parity_check.sh` using the local EU ROM and a QEMU wrapper:
  both committed scenes (`intro_logo`, `title`) match original Tier A and B
  hashes. Use the oracle's default native width 240; compiling the static
  oracle at capacity 384 renders extra pixels and is not a native parity test.
  Committed golden hashes remain unchanged.
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
