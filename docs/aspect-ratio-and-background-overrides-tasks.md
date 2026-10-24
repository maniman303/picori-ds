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
globals or allocates, opens files, or alters rendering. Catalog selection is
explicit: the future state adapter must identify installed content, rather
than infer an image from `TASK_GAME` or a transient credits command. The catalog
groups shared settings; effect policies remain specific to the active screen.
Unknown screens return no source until their state mapping is verified.

The profile registry describes target asset dimensions only. It does not add
4:3 to settings, advertise 32:9 renderer support, change framebuffer capacities,
or publish the per-frame geometry snapshot. Those belong to the next task.

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

- [ ] Append the 4:3 aspect enum without changing existing serialized IDs;
  integrate canonical profile lookup and skip unsupported modes visibly.
- [ ] Publish one immutable geometry snapshot before simulation/layout/shadows;
  include selected/effective profile, capacity/pitch, canvas, native content,
  world aperture, UI safe rectangle, capability flags and generation.
- [ ] Migrate `port_widescreen.h` wrappers to consume that snapshot. Window
  resizing must affect presentation only. Preserve room/transition safety.
- [ ] Remove widescreen toggle state, config entry/API, UI and indirect gates;
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

## Verification record (2026-10-24)

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
