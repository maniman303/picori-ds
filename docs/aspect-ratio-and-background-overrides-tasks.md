# Aspect ratio and background implementation tasks

Tracks implementation of [the design](aspect-ratio-and-background-overrides-plan.md).
Work proceeds in the order below. Checked items describe implemented contracts
or completed checks, not a claim that the renderer supports the new profiles.

The [2026-10-25 menu-family revision](menu-background-family-revision.md)
reviews every one of the 27 previously checked tasks. The active target is
33 artwork names / 15 menu family-health combinations, including one fixed
fast-travel image; catalog and exporter now implement 33 / 15. The three
reopened catalog/agreement tasks are completed below;
completed tasks retain their evidence; outstanding targeted extensions are
tracked below.
Historical verification sections retain their original 23- or 32-name
results; those results do not certify the revised catalog or title-frame fix.

The current remaining-work count and finite context-audit closure groups are in
[the closure checklist](background-context-closure-checklist.md): 21 unchecked
parent items across this list. Historical verification notes below are not
additional automatically reopened tasks.

## 1. Catalog and baseline

- [x] Research pieces/techniques/top-left-title separation and review all 27
  completed tasks for impact; record retained evidence and required follow-ups.
- [x] **Updated (R01):** extend the C-compatible catalog from 23 to 33 artwork
  names: plain, titled, pieces, techniques and fusion each have three health
  variants; fast travel has one independent fixed image. Append enum IDs and
  preserve existing names/artist files.
- [x] Distinguish live/unknown screens, image families, generated black,
  the logo field, and live palette-zero fields. Disable every source in parity.
- [x] **Updated (R03):** update C/C++ filename/source contracts and exporter
  comparison to all 33 names / 15 menu combinations. Test exact new-family
  names, no aliases and procedural/parity exclusions. The map-hint swatch must
  never become a runtime image. The former 23-name check is historical evidence.
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
- [x] Extend capture/state audits (R06–R08) with semantic family, heading style
  and pose, all 15 family/health settings and same-health family transitions.
  Retain old native captures and health-load evidence; native decoration loads
  do not imply unchanged PNG identity or decode counts across different families.
- [x] Finish the context/state catalog on the EU fixture, including
  dungeon/fusion/fast-travel/figurine screens and interaction/error paths;
  retain the completed file-operation, prologue, credits and pause-menu evidence.
  USA/JP support is best effort based on source and regional data review;
  those regions are untested and are not a fixture or acceptance gate.
- [x] Audit the native figurine collection viewer on EU/English before/after
  credits at three health preloads: owned/missing entries, native navigation,
  B/START cancellation and gameplay restoration. Verify the live palette-zero
  field survives health changes without selecting a menu-family image. Other
  regions remain open; the prize/scroll extension is recorded below.
- [x] Audit native figurine prizes, duplicates and both description viewers in
  all five EU languages: four capsule types, three health preloads and pre-/post-
  credits ownership, 270 cases total. Verify native award/reveal timing,
  brightness controls, scroll input/clamps, rendered description movement and
  game-task/room restoration. Retain interrupted captures explicitly and verify
  disjoint completed prefixes/suffixes; preserve 54 original viewer checkpoints.
- [x] Audit all six native map-bearing dungeons on EU/English, entering real
  rooms before screen installation. Cover every floor at three health preloads
  with map/compass/big-key bits absent/present: 138 cases, native navigation and
  START restoration. Retain original pause/fusion case IDs and captures.
- [x] Audit native detail-map scrolling on EU/English for all 22 recipes at
  three installed health decorations: 66 cases, midpoint/top/bottom/top inputs,
  both clamps, live-health mutation, B return to the world map and START exit.
  Compare ROM-derived field/map samples and native crop movement/return;
  preserve all 260 retained pause/fusion checkpoint pixels. Other languages,
  world-map cursor entry and additional marker combinations remain open.
- [x] Audit native sleep confirmation on EU/English at all three health
  preloads: 15 cases covering A acceptance, RIGHT/LEFT acceptance, A refusal,
  B/B refusal and RIGHT/B refusal. Verify real quest navigation, native cursor
  retention, six released-key sleep/wake loops with frozen game ticks and white
  forced-blank output, source retention and quest/game restoration. Other
  languages/regions, hardware sleep and save/game-over paths remain open.
- [x] Audit native Quest Status save confirmation on EU/English at three
  installed health decorations: 18 cases covering acceptance, DOWN/UP
  acceptance, DOWN/A refusal, B/B refusal, DOWN/B refusal and injected EEPROM
  write failure. Verify checksum readback, unchanged prior records on refusal/
  failure, the native error wait/dismissal, installed-family retention and
  quest/game restoration. Inventory-entry saves, host flush failures and other
  languages/regions remain open.
- [x] Audit native inventory-entry save confirmation on EU/English at three
  installed health decorations: 18 acceptance/refusal/EEPROM-failure cases.
  Verify native cursor navigation, screen 11 installation, checksum readback,
  error timing/dismissal, unchanged records on refusal/failure and inventory/game
  restoration. Retain the Quest Status evidence; host flush failures and other
  languages/regions remain open.
- [x] Audit native Game Over save/refusal, EEPROM failure/retry, Continue and
  Quit on EU/English: five cases entered through real death/room exit. Verify
  checksum readback, unchanged prior records, native input/error timing, four
  player-control returns and a title soft reset. Confirm the generated-black
  field despite the shared decoration load. Additional combinations, fixtures,
  languages/regions and intermediate effect pixels remain open.
- [x] Audit all six standalone map hints, first/repeated reveals, at three health
  preloads in all five EU languages. Verify native timer-30 reveal, A/B/START
  exits and the live palette-zero field before/after health mutation.
- [x] Extend the 27-case figurine collection route to French/German/Spanish/
  Italian, retaining English evidence and the same source/lifetime checks.
- [x] Extend the native fusion audit to all seven layouts at each installed
  health on the available EU/English fixture; retain all 109 previous case IDs
  and native checkpoint pixels. Interaction/effect and regional/language gaps
  remain with the full context/state task above.
- [x] Audit native fusion interactions on EU/English at three installed health
  decorations: B/START/L cancellation, mismatch/recovery/cancel, successful direct
  return and successful world-event/local-hint handoff, 18 cases total. Verify
  real input, installed-family retention, native flash cadence, piece consumption,
  fused bit/count and original-room restoration. Close group C1 in the explicit
  context-audit checklist; remaining groups and effect resolution stay separate.
- [x] Audit committing file operations on EU/English: create/name and copy
  success, erase acceptance/refusal, and EEPROM failures in all three operations.
  Verify checksum/whole-record readback, untouched unrelated slots, native error
  waits, input, file-list restoration and ROM-derived background samples. Record
  failed-erase UI/storage disagreement under C5; close coverage group C2.
- [x] Audit post-credits Save on EU/English: acceptance, refusal, failure/retry
  and failure/refusal from the native post-credits initializer. Verify prior
  record retention, checksum readback, error waits, B/START dismissal, generated
  black and native title return. Retain full credits evidence; close group C3.
- [x] Close C4 map/navigation boundaries on EU/English: 44 real cursor/A
  detail-map entries across the 22 recipes with markers hidden/visible, plus
  all 49 distinct reachable local-hint map-area/icon combinations. Verify native
  B return, retained cursor, source lifetime, health mutation and room restoration.
- [x] Close C5 error/return paths on EU/English: propagate durable PC erase
  failure without clearing its slot or leaving a pending deletion; verify a real
  blocked atomic-write path, full durable-file retention and successful recovery.
  Extend native Game Over with B/START error dismissal, retry refusal, recovery
  and Save-and-Quit; preserve historical failure evidence and GBA behavior.
- [x] Close C6 EU localization: extend the 14 remaining menu/map/travel,
  sleep/save/Game Over and ending/file routes to French/German/Spanish/Italian.
  Verify live resource slots, 2,029 cases plus four file-browsing flows and
  56,196 captures. Preserve canonical IDs and explicitly record the three
  unavailable Spanish fusion-width cases; retain English and five-language
  figurine/hint evidence. C7 is resolved by the best-effort regional policy below.
- [x] Audit native fast-travel selection/cancel on EU/English: all eight
  destinations and a single-unlock mask at three health preload states.
  Verify group 93 replaces the health decoration with a fixed BG3 field;
  replace its provisional plain-family alias with the single dedicated
  `fast_travel_background.png` catalog/export contract. Confirmation/travel
  effects remain open.
- [x] Extend native fast-travel coverage to enquiry/decline/accepted travel on
  EU/English: both answers at eight destinations plus the single-unlock fixture,
  at three health preloads. Verify all 54 cases, first-use tutorial, fixed-field
  lifetime through outgoing fade/scene replacement and normal player-control
  return. Preserve all 54 previous selection/cancel checkpoint pixels. Broader
  masks, languages/regions and resolved intermediate effects remain open.
- [x] Resolve per-context effect targets from unfaded source coverage for the
  available EU fixture, including white logo BG versus backdrop, title local
  alpha, and masked palette fades. Review USA/JP source differences on a
  best-effort basis; publish the per-edge policy in the later compositor work.
- [x] Audit EU unfaded image/edge ownership and palette banks in all five
  languages; distinguish logo backdrop from opaque white BG2 and correct the
  local fusion hint to live BG3 palette entry 95. Add full-precision palette,
  brightness and explicit-second-input alpha helpers, with real `FadeMain` mask
  checks. Unobserved intermediate effects and publication remain open in the
  [effect-target audit](background-effect-target-audit.md).
- [x] Measure RG scale-1 frame times, PSS/RSS and driver memory on device.
  Record title and instrumented gameplay/menu samples with their limitations
  below; multi-scene, percentile and thread-count acceptance remains separate.

The [context/state audit](background-context-state-audit.md) records the available
EU evidence and the untested regional limitations. Capture route coverage
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
- [x] Extend the allocation/path audit (R18) for bounded title-cap/descriptor
  storage and new-family switches; keep current/incoming cache budgets and
  avoid resident buffers for all 33 images; fast travel reuses the same two-slot bound. Existing capacity totals remain valid.
- [x] Update `build.py`, RG builds and tools together. Preserve N64 native-only
  constraints and verify original native golden pixels before taller rendering.

## 3. World rendering

- [x] Separate the original hardware register sequence from bounded rendered
  rows. Guard HDMA against extra/negative callback lines; stable-state scenes
  can render extra rows and continue affine references without extra IO copies.
- [x] Classify HDMA continuations and implement full-canvas effect geometry.
  Unclassified or varying callbacks cap the effective aperture at 160; the
  verified fixed-brightness case can extend. Static unclassified windows also
  cap height. This is an interim safety gate while 4:3 remains disabled, not
  completed 4:3 behavior.
- [x] Classify the current HDMA destinations and fail closed for unsupported
  continuations. Extend only verified fixed-source BLDY without windows; retain
  160 hardware callbacks and snapshot storage. Add bounded full-canvas iris
  spans checked against the native circle. The later follow-up below publishes
  fade-iris geometry; other raster/window continuations remain native-height
  fallbacks in the [HDMA audit](background-effect-target-audit.md#hdma-classification-and-geometry).
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
- [x] Finish room-aperture publication, unsupported-shake fallback selection,
  per-manager graphics residency and upload timing before exposing rows 160–191.
  Taller rows are staged and uploaded before publication; all manager-occupied
  rooms conservatively fall back to native height until separately validated.
  Other scene, sprite and layout approvals still keep 4:3 disabled.
- [x] Add signed per-slot OBJ positions, attribute validation, build/DMA lifetime
  and explicit hidden/parked semantics to the software/supersampled paths.
  Verify real Y=160/191 pieces and X > 511 without changing packed OAM.
- [x] Migrate generic entity/rectangle culling and piece clipping to effective
  world height. Record the [sprite visibility audit](sprite-coordinate-audit.md).
- [x] Complete individual entity/AI/parking and special-effect audits; verify
  all direct OAM consumers and scene capability decisions before enablement.
  Unreviewed dynamic entities and effects are explicitly denied tall-scene
  admission; this is not a claim that their 4:3 behavior has been validated.
- [x] Extend transitions one validated case at a time; test camera completion.
  A palette-only black-fade room arrival now admits staged tall geometry when
  the room, sprite, manager and upload gates pass. Windows, HDMA, mosaic and
  other transitions retain native-height fallback. The production staging test
  verifies admission and withdrawal, the PPU regression checks the palette fade
  across all 192 rows, and the real camera/script-wait regression checks
  arrival completion. These are ROM-free fixtures; 4:3 remains disabled until
  the separate scene and acceptance gates are complete.
  The rolling-scroll completion regression covers all four directions and
  verifies the existing wide-camera snap and tile refresh after the native
  transition. The separate timed Scroll4 and iris Scroll5 completion paths are
  also checked, including camera snap and refresh with injected wide and tall
  apertures. Completion now restores the camera for height-only expansion too.
  Rolling entry also reframes a height-only expanded view before replacing its
  room. Rolling transitions retain their native aperture until their mixed-room
  tile streaming is separately validated.

## 4. Layout

- [x] Translate each complete fixed-screen composition once using the native
  content rectangle; include HD text and any hit testing.
  The shared SDL/GPU/surface viewport now projects the published native
  rectangle exactly, including odd margins, and the HD text overlay follows
  that viewport. Installed menu and story-page scenes are held to native raster
  dimensions, including menu-to-menu handoffs. File-select presentation captures
  at 16:9 and 21:9 have byte-identical 240x160 crops against native at 4x scale;
  fixed screens use button input and have no screen-coordinate hit testing.
  Added artwork, margin effects and unsupported-profile enablement remain
  separate tasks.
- [ ] Implement top-left title-frame completion strictly in added margins,
  preserving native pixels and label placement. Verify native OBJ source/mask,
  local effects and animated heading pose; missing PNGs must not leave the cut
  frame edge. Do not use tile wrapping or move the centered native composition.
  Pause and fusion draw sites record the signed pose and region sprite ID with
  the OAM build/latch lifetime, even when native pieces are clipped. The
  compositor reconstructs the cap from live OBJ tiles and palette, honors the
  animated pose and per-line OBJ effects, and preserves the native rectangle.
  EU software captures verify the missing-image fallback and an unchanged
  native crop. OBJ-window scenes still fail closed; runtime artwork composition
  and other regions and presenters require validation before closing this task.
- [ ] Extend direct-OBJ and presentation tests (R16/R25) for that frame/cap at
  native/canvas edges, all profiles, both presenters, screenshots and optional
  paths. Keep native/parity goldens and parked-OBJ semantics unchanged.
- [ ] Resolve functional live gameplay separately from pause/map/fusion states.
  Include fixed fast-travel BG3 artwork behind its centered native map/frame/OBJ;
  it has no health variant or top-left heading-cap requirement.
- [ ] Implement semantic corner HUD anchors and bottom dialogue geometry shared
  by BG/OAM, portrait, choices, cursor, arrows and HD text; keep banners separate.

## 5. Margin composition

- [ ] Add a game-state adapter selecting catalog contexts at scene boundaries;
  track the installed prologue/credits illustration across text commands/fades
  and the installed normal/damaged/critical menu or kinstone decoration.
  Publish the fixed FAST_TRAVEL context after screen 13 installation; ignore
  overwritten health preloads and retain the image across destination changes;
  invalidate it on native gameplay restoration/scene replacement.
- [ ] Track semantic menu family independently of installed health. Publish
  titled/plain/pieces/techniques changes after native layout installation and
  before incoming fade; do not infer family from BG3, current health or task ID.
- [ ] Resolve fast-travel BG3 effect targets and source lifetime across selection,
  confirmation/decline, cancel and travel transitions before compositor publication.
- [ ] Publish source, native coverage and resolved effect data to a compositor
  with no task IDs or config parsing in its pixel loops.
- [ ] Include the verified title-frame completion mask/pose in foreground
  composition above artwork, with no task IDs or live health reads in pixel loops.
- [ ] Implement procedural fields and black fallback with correct global effects;
  preserve every native foreground pixel and exclude gameplay completely.
- [ ] Retire the old ImGui menu side-art path as its supported screens migrate;
  verify software/GPU presentation and raster fallbacks, filters and persistence.

## 6. Artwork loading

- [ ] Decode exact-size ordinary PNGs with libpng limits and safe error cleanup;
  resolve only the selected ratio and exact shared family/health filename. Never
  substitute another health state or menu family. Validate width and height
  against the table above, including ceiling-rounded widths.
- [ ] Resolve all five menu families independently; key cache/publication by
  family/image ID plus installed health, ratio and renderer generation. Test
  missing new files with old plain-menu art present: black, never a family alias.
  Include the fixed fast-travel image key without health or destination variants;
  navigation must not decode again. Missing art must not select a plain-menu image.
- [ ] Test accepting each selected profile's exact size and rejecting a file
  sized for another ratio, floor-rounded widths (284/373/568), and wrong heights.
- [ ] Cache current/incoming image bytes within 8 MiB steady / 16 MiB peak;
  cache failures, clear stale images, and perform no hot-path I/O/allocation.
- [ ] Add explicit reload, corruption/size/alpha fixtures and packaging checks
  that preserve custom artwork. Keep raw asset mods independent.

## 7. Export synchronization

- [x] **Updated (R27):** expand the ROM-free runtime-catalog/exporter agreement
  gate and recipe fixtures to 33 artwork names / 15 menu combinations. Keep
  all existing procedural/reference exclusions and exact profile dimensions.
- [x] Add nine clean native reference recipes for titled menus, pieces and
  techniques, with blank-heading/static-frame masks and no live/localized UI.
  Update README/contact sheet; export 33 artwork references plus one swatch, including fixed fast travel.
  Preserve existing artist files and require explicit overwrite as before.
- [x] Add the fixed blue fast-travel BG3 recipe, omit foreground map/frame/OBJ,
  verify all three health preload paths agree, append its catalog ID, and rerun
  the missing-only extractor to refresh the native folder/README/contact sheet.
- [ ] Validate all native export recipes against the EU ROM after context/effect
  auditing; review regional code paths best-effort and preserve artist edits
  and existing export names.

## 8. Acceptance and budgets

- [ ] Verify fixed fast-travel margins/native crop, screenshots, profile switches,
  missing-image fallback and no-redecode navigation on both presenters.
- [ ] Complete the design's EU functional-screen, world, HUD/dialogue,
  renderer, intermediate-fade, PNG failure and profile-switch coverage; review
  USA/JP source differences on a best-effort basis without regional ROM testing.
- [ ] Measure RG performance, memory/cache limits and 1/2/4-thread behavior;
  record unsupported optional-path fallbacks and untested regional behavior explicitly.
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


## Five-family catalog and exporter implementation (2026-10-25)

The C catalog appends nine contexts without changing old IDs. A pure family /
installed-health lookup covers all 15 combinations and rejects invalid values.
The exporter separates plain, titled, pieces, techniques and fusion roles, with
32 artwork names plus the reference-only map-hint swatch. The titled recipe
decodes the native OBJ plate and removes lettering; pieces/techniques combine
their static BG2 frame/icon with the installed decoration and omit live OBJ/BG0.

The local EU/English exporter ran with `--add-missing`: nine new native PNGs
were added in `assets/override/32/`, with 33 PNGs total. All 40 pre-existing PNGs
in `32/` and `43/` retained their SHA-256 hashes. Generated README/contact sheet
were refreshed. Native pieces/techniques frames fully cover the decoration,
so their three health references have identical pixels; their files and runtime
catalog identities remain separate for independent custom artwork.

C11/C++17/UBSan catalog/exporter checks, nine exporter tests and the RG release
build pass. Six pieces/techniques lower-panel comparisons against the earlier
EU native captures are byte-exact (rows 64–159). EU startup under QEMU reaches
`Port layer initialized. Entering AgbMain...`. No artist file was overwritten.
Runtime PNG loading/composition and animated title-frame completion remain
their explicit open implementation tasks; the new native reference plate is
not a claim that the in-game margin seam is already repaired.


## Family/heading capture and state-audit extension (2026-10-25)

The [state audit](background-context-state-audit.md#semantic-family-and-heading-audit-extension-2026-10-25)
now records schema-v2 semantic family and heading style/position/transition
observations at native installation and draw boundaries. Pending pause requests
retain the outgoing family, while independent openings clear it after gameplay
restoration. Reused map-hint/figurine/fast-travel initialization cannot classify
a pause-family image. This remains repro-only observation, not runtime artwork
selection or compositor publication.

The extended EU/English matrix captures 109 cases: the retained 106 pause cases
and three representative fusion settings. All 15 family/health combinations,
15 required same-health pause-family switches and all nine ordinary heading
poses (transition 0,2,…,16) pass. All 212 original ready/held native checkpoints
are byte-identical to the retained audit. Inventory retains 6,240 independent
ROM pixel comparisons; pieces/techniques add 276,480 comparisons over 12 lower
panel checkpoints. There are ten pause and three fusion decoration loads;
these are not PNG decode counts or evidence of cross-family cache reuse.

Local artifacts, copied repro binaries and their hashes are separate under
`.tmp/background-family-audit/`: `eu-en/` has 2,082 menu captures and its summary;
`eu-routes/` retains the 75-frame file route; `eu-hint-final/` has the final
44-frame procedural hint check. Original native/wide captures, artist files and
committed goldens remain unchanged. The final hint guard is verified separately
from the menu matrix; it leaves the pause/fusion observations unchanged.

Verification passes: 16 ROM-free audit-checker tests, catalog/C/C++/UBSan and
exporter checks, engine/scroll/CPU PPU regressions, RG ARM64 repro and restored
release builds, and final release startup under QEMU/dummy video/audio reaching
`Port layer initialized. Entering AgbMain...`. Both native EU logo/title scenes
retain their original Tier A/B hashes. The original release configuration is
restored with `repro_harness=n`; no device deployment or packaging was performed.
The title-cap seam, enhanced margin/native-crop checks, full seven-layout fusion
coverage, additional regions/languages, GPU execution and RG device budgets
remain with their explicit open tasks.


## Presentation allocation and new-family cache extension (2026-10-25)

The [allocation audit](presentation-allocation-audit.md#scene-composition-boundary-and-menu-family-extension-2026-10-25)
now records storage bounds for title-frame completion in added margins, the
family/heading descriptor contract, and resident image cache budgets across all
five menu families:

- **Descriptor bounds**: Struct `PortMenuBackgroundDescriptor` in
  `port_screen_background.h` is a 24-byte POD record holding family, installed
  health, heading style, heading pose (`x, y, transition`), and generation.
  Double-buffered snapshot publication is zero-allocation and static.
- **Title-cap storage**: In 4:3, the top-left heading cap is strictly bounded by
  added margins outside native content (at most 1,408 pixels, 5,632 bytes RGBA).
  Wide 160-row profiles add no top margin and at most side margin pixels (`cx * 32`).
  Composition blits directly into the existing primary canvas or uses bounded
  static scratch (<= 16 KiB); no second full-frame 569 × 192 buffer (436,992 bytes
  raw or 43.7 MiB at 10×) is allocated.
- **Cache budgeting**: 32 artwork names must not become 32 resident images in RAM
  (a naive cache would consume 11.65 MiB unscaled, exceeding the 8 MiB steady limit).
  The runtime cache is strictly bounded to two slots (current active + incoming
  transition), consuming at most 728,320 bytes (~711 KiB at 32:9) unscaled, well
  within the 8 MiB steady / 16 MiB peak budget. All 30 inactive images remain on
  disk. Family switches evict outgoing images on completion; same-family navigation
  preserves the active cache slot with zero decodes.
- **Existing capacities**: Build capacity 569, raster height 192, and upscale pools
  (43.7 MiB) remain valid. Optional path gates (Voxel, GLSL presets, HD text) continue
  to reject expanded canvas geometries.

Verification passes: `tools/presentation_geometry_regression_test.py` under ASan/UBSan
at capacities 240, 384, and 569, checking descriptor POD layout, title-cap margin
bounds, 2-slot resident memory vs. 32-image naive rejection, and family switch state
machine; `tools/screen_background_regression_test.py` across C11 and C++17; and
existing engine/scroll/PPU regression suites.


## Seven-layout fusion context coverage (2026-10-26)

The first open context/state task now has a completed EU/English fusion-layout
slice. The [state audit](background-context-state-audit.md#seven-layout-fusion-audit-extension-2026-10-26)
records 21 appended native layout/installed-health cases, discovered from real
ROM partner names without forcing layout graphics. All 130 menu cases pass;
all 218 retained 109-case checkpoints are byte-identical. The checker requires
native installation and ready/held layout agreement and keeps the previous
family, heading and independently decoded ROM pixel checks.

The 19 checker tests, nine exporter tests, C/C++ catalog agreement, engine/scroll
regressions, ARM64 audit/release builds and release EU startup under QEMU pass.
The original release configuration is restored. Fusion interactions/effects,
dungeon menus, standalone hints, travel, figurines, remaining functional/error
paths and other regions/languages keep the full context/state task open. This
slice does not implement the runtime adapter, artwork loader or compositor.


## Fast-travel context correction and native cancel coverage (2026-10-26)

The [native travel audit](background-context-state-audit.md#native-fast-travel-selectioncancel-audit-2026-10-26)
adds 27 cases: eight destinations plus a single-unlock mask at three initial
health states, with 54 neutral ready/held checkpoints and native B cancellation
back to gameplay. Group 93 replaces the health-specific BG3 tilemap. The
provisional plain-family mapping is withdrawn from the design/exporter;
that audit initially left fast travel unclassified. The fixed-image completion
below supersedes that interim source decision and adds the missing reference;
its native capture evidence remains valid.

All 27 cases, 84 native cursor changes and 27 cancellations/restorations pass.
The 54 exposed bottom rows match an independent ROM-decoded fixed-BG3 reference
(12,960 runtime pixel comparisons). The neutral contact sheet was inspected.
The 26 checker tests, ten exporter tests, C/C++ catalog agreement, retained
130-case trace compatibility, ARM64 audit/release builds and release EU startup
under QEMU pass. The release configuration is restored. Confirmation/travel
transitions, additional masks/languages/regions, remaining contexts and effect
resolution keep their broader tasks open.


## Fixed fast-travel image and extractor completion (2026-10-26)

The interim no-source decision above left the verified blue field out of the
exporter. It is superseded by `fast_travel_background.png`, a single fixed
BG3-only reference for all destinations/unlock masks and health values. The
catalog appends an ID without renumbering; there are now 33 artwork names and
34 native PNGs including the map-hint swatch. No new health variants are added.

The [completed-task review](menu-background-family-revision.md#fixed-fast-travel-artwork-task-review-2026-10-26)
revisits all R01–R27 decisions: catalog/agreement/export and allocation counts
are updated; state-adapter, cache, compositor/effect and packaging acceptance
follow-ups are explicit. Existing native capture evidence and title/world/
geometry gates are retained. The full context/state task stays open.


Extractor verification for the fixed-image completion:

```sh
python3 tools/export_screen_backgrounds.py --rom /path/to/EU.gba --language 1 --add-missing
```

The local EU/English run exports 34 PNGs (33 artwork references plus the swatch)
and adds only `assets/override/32/fast_travel_background.png` at 240 × 160.
All 55 pre-existing artwork/reference PNGs across ratio folders retain their
SHA-256 hashes; the generated README/contact sheet are refreshed. The new PNG
was inspected and matches all three independently decoded full native BG3
preload results, plus the exposed row in all 54 retained travel checkpoints
(12,960 runtime pixels). Its BG1/BG2 map/frame and OBJ content are excluded.
Local preservation/hashes are recorded in `.tmp/fast-travel-export/verification.json`.
Native `32/` exports remain Git-ignored; rerunning the exporter creates the new
reference in fresh workspaces without distributing ROM-derived artwork.

The 33-name C11/C++17/UBSan agreement gate, 11 exporter tests, 26 audit-checker
tests and ASan/UBSan presentation-budget checks at capacities 240/384/569 pass.
The ARM64 release build and EU startup smoke under QEMU/dummy video/audio pass;
release options stay unchanged with `repro_harness=n`. Native/artist files and
committed goldens are not replaced; no packaging, device or extra-region
acceptance is claimed. Runtime adapter/PNG loading/composition remain pending.


## Figurine collection context coverage (2026-10-26)

The first open context/state task gains a completed EU/English collection-viewer
slice. The [figurine audit](background-context-state-audit.md#native-figurine-collection-audit-2026-10-26)
records 27 views / 54 neutral checkpoints across three health preloads and
pre-/post-credits collection limits. Native input visits owned/missing entries;
six B/START exits restore gameplay. The palette-zero field remains independent
of the overwritten health decoration and no pause-family image is selected.
ROM decoding verifies the two exposed source samples and palette identity;
all 108 checkpoint pixel comparisons pass. No new artwork filename is added.

Seven new checker tests, the retained 26 audit-checker and 11 exporter tests,
C/C++ catalog agreement, engine/scroll/CPU PPU regressions, ARM64 build and EU
QEMU startup pass. The existing 130-case menu/fusion and 27-case travel traces
still validate. The incoming build configuration is unchanged. Captures and
binary hashes are local under `.tmp/background-figurine-context-audit/`, separate
from original evidence and goldens; no artist files, packaging or profiles change.

Dungeon menus, standalone hints, figurine prize effects/description scrolling,
travel confirmation/effects, other interaction/error paths and regional/language
coverage keep the full task open. Runtime adapter/loading/composition and device
measurements remain their explicit later tasks.


## Native dungeon, standalone hint and EU language coverage (2026-10-26)

The full context/state task gains two completed routes. The
[dungeon room/floor audit](background-context-state-audit.md#native-dungeon-room-and-floor-audit-2026-10-26)
enters real rooms in all six map-bearing dungeons and exercises every native
floor with three health preloads and map/compass/big-key bits absent/present.
All 138 cases / 276 neutral checkpoints, 36 native START exits/restorations and
66,240 independently decoded BG3 pixel comparisons pass on EU/English.

The [standalone-hint audit](background-context-state-audit.md#standalone-hint-reveal-and-exit-audit-2026-10-26)
adds six first/repeated native hints at three health preloads, checking the
native timer-30 reveal, A/B/START exits, installed source retention and health
mutation. The hint and prior figurine routes now pass in all five EU languages:
180 hint cases / 540 checkpoints and 135 collection cases / 270 checkpoints.
Source checks compare 1,080 hint and 540 figurine pixels with live palette zero
and independently decoded ROM palettes. Existing English evidence is retained.

The 13 new functional-checker tests, retained 33 capture/figurine-checker tests,
11 exporter tests, C/C++ catalog agreement, engine/scroll/CPU PPU regressions,
ARM64 build and EU startup under QEMU pass. Original pause/fusion/travel traces
still validate. Local artifacts and exact verification/coverage limits are in
the audit. Build options, profiles, artist files and committed goldens remain
unchanged; no runtime PNG, compositor, packaging or device work is claimed.

The full task stays open for the interaction/error paths enumerated in the
audit, other dungeon languages and broader regional/quest-state evidence.


## Native fast-travel interaction coverage (2026-10-26)

The [confirmation/scene-handoff audit](background-context-state-audit.md#native-fast-travel-confirmation-and-scene-handoff-2026-10-26)
completes the next native interaction slice: both answers at all eight travel
destinations plus a single-unlock fixture, at three health preloads. All 54
cases pass, including 27 declines, 27 actual native arrivals, first-use tutorial
completion and normal player-control return. The synthetic post-Fortress save
includes completed intro flags; the native room/arrival code is not bypassed.

The fixed field remains installed through enquiry/outgoing fade and clears at
native scene replacement or cancellation restoration. All 162 neutral source
checkpoints match independent ROM pixels (324 comparisons). The final binary
also preserves decoded pixels from all 54 prior selection/cancel checkpoints
byte for byte.
New observations and input driving are repro-only; no artwork filename or
runtime adapter/compositor/cache behavior is added.

Twelve new checker tests and 46 retained audit-checker tests, 11 exporter tests,
C/C++ catalog agreement, engine/scroll/CPU PPU regressions, ARM64 build and EU
QEMU startup pass. Exact artifacts, staged fixture flags and remaining coverage
are recorded in the audit. Original goldens, artist files, build options and
profile availability remain unchanged. Other travel masks/languages/regions,
resolved effect pixels and the full task's other interaction/error paths stay
open; no packaging or device acceptance is claimed.

## Native figurine prize and description interaction coverage (2026-10-26)

Added a repro-only 54-case route per EU language for all four native capsule
types, new/duplicate awards, health/credits preloads and description scrolling
in prize/collection views. Read-only events and trace fields verify native
award counter 64, reveal completion 128, raw duplicate masks, the full brightness
control timeline, ownership lifetime, native navigation/exit and scroll clamps.
Independent ROM source points and native text crops verify neutral field and
rendered description behavior. Runtime image publication remains a separate task.

The five-language union verifies 270 canonical cases, 1,080 neutral checkpoints
and 810 description comparisons. Initial process timeouts are retained as
failures; explicit completed-prefix verification plus successful independent
suffix runs provides complete disjoint case coverage. No full-run success is
inferred from those timeouts. The final binary preserves all 54 retained English
collection checkpoint pixels. Sixteen new and 58 retained audit tests, 11
exporter tests, the ARM64 build and EU startup pass. See the
[native figurine audit](background-context-state-audit.md#native-figurine-prizes-duplicates-and-description-scrolling-2026-10-26)
for exact artifact ranges, recovery commands and limits, including game-task
restoration without a claim of normal player-control readiness. The broad
context/state catalog and effect-source audit remain open.

## Native detail-map scrolling coverage (2026-10-26)

Added a separate 66-case EU/English route for all 22 detail-map recipes at three
installed health decorations. Native held UP/DOWN input exercises the initial
midpoint, both bounds, exact 2-pixel cadence and zero-range clamps. B returns to
the world-map page; START exits each health group to the original game room.
Map item ownership and conditional house/fusion/interior fixtures are explicit.

Final verification passes 330 neutral checkpoints, 5,280 ROM-derived decoration
samples and 4,020 independently decoded scrolled-map samples. Native crops check
health retention, movement and exact top return. The native process completes
successfully; its complete raw trace was revalidated after correcting the
checker's exit-UI assumption and excluding native area-name UI from the field
mask. All 260 retained pause/fusion checkpoints remain pixel-identical. Twelve
new checker tests and 85 retained tests, catalog/profile checks, ARM64 build and
EU startup pass. See the
[detail-map audit](background-context-state-audit.md#native-detail-map-scrolling-and-return-paths-2026-10-26)
for process outcomes, artifacts, reproduction and remaining scope. The full
context/state catalog and runtime adapter/compositor work remain open.

## Native sleep confirmation and wake coverage (2026-10-26)

Added a repro-only 15-case EU/English route covering acceptance, refusal and
B selection/confirmation at three installed health decorations. Real input
navigates the remembered quest cursor and opens Sleep; accepted cases execute
the native released-key sleep loop and wake with raw A. The checker requires
frozen game ticks at every sleeping boundary, raw forced blank and completely
white captures, resumed ticks, source retention across health mutation and
quest/game restoration.

The successful capture command verifies six sleep/wake cycles, nine declines,
60 neutral checkpoints / 960 ROM-derived field samples and six whole white
frames. All 260 retained pause/fusion checkpoint pixels remain identical.
Fourteen new and 97 retained audit/exporter tests, catalog/profile checks,
ARM64 build and EU startup pass. The
[sleep audit](background-context-state-audit.md#native-sleep-confirmation-and-wake-up-2026-10-26)
records exact artifacts, earlier diagnostic failures, source masks and scope.
Current PC any-key wake is covered; hardware sleep and the displayed SELECT/L/R
chord are not. Save/error, game-over and fusion interactions remain with the
open context/state task; no runtime adapter/compositor work is marked complete.

## Native Quest Status save interaction coverage (2026-10-26)

Added a repro-only 18-case EU/English route using native inventory-to-quest
navigation and the Save button. Six successful saves return automatically;
nine refusals preserve the prior record; three injected EEPROM write failures
execute the native retry/error path, wait the full 60 ticks and require real A
dismissal. Native checksum reads verify saved health or unchanged prior records.
The audit never assigns a menu result, save result, cursor or timer.

The native process completes all 18 cases. Final offline validation passes the
full lifecycle/input/source checks, 81 source checkpoints / 1,296 ROM-derived
field comparisons and original-room restoration. Success result/quest-return
checks share six framebuffer captures. All 260 retained pause/fusion checkpoint
pixels remain identical. The baseline helper now accepts the established
130-case fusion matrix alongside historical 106/109-case evidence.

Twelve new save-checker tests, one new 130-case baseline test and 111 retained
audit/exporter tests pass, together with catalog/profile agreement, engine,
scroll, CPU PPU and save/quicksave persistence checks. The ARM64 build and EU
QEMU/dummy-SDL startup pass; incoming build configuration stays unchanged.
The [save audit](background-context-state-audit.md#native-quest-status-save-choices-and-write-failure-2026-10-26)
records raw artifacts, native success versus capture-command checker failures,
final offline verification and remaining scope. This completes the Quest Status
save slice; the broad context/state task and runtime adapter/compositor remain
open.


## Native inventory-entry save interaction coverage (2026-10-26)

Added the separate `inventory_save` route using real UP / LEFT / A input to
open screen 11, native remembered-cursor navigation and native inventory return.
All 18 EU/English cases pass: six successful saves, nine refusals and three
EEPROM write errors across three installed health decorations. The final native
process and capture command both exit zero at frame 6,859. Validation covers
81 source checkpoints / 1,296 ROM-derived field samples, error wait/dismissal,
checksum readback, installed-family/heading lifetime and original-room restoration.
The earlier externally terminated capture is retained and is not counted as
completed evidence.

Thirteen new inventory-checker tests bring the shared audit suite to 126 tests;
11 exporter tests, catalog/profile agreement, engine/scroll and CPU PPU checks,
the ARM64 incremental build and isolated EU startup smoke all pass. Retained
Quest Status and 130-case menu/fusion traces still validate with the final tools;
this is trace compatibility, not a fresh native pixel replay. See the
[inventory save audit](background-context-state-audit.md#native-inventory-entry-save-choices-and-write-failure-2026-10-26)
for artifacts and reproduction. Build options and existing goldens/artwork remain
unchanged. The full context/state catalog, runtime adapter/compositor and broader
regional/effect/device acceptance remain open.


## Native Game Over interaction coverage (2026-10-26)

Added a repro-only `gameover` route with five native-death cases covering Save,
Don't Save, cursor navigation, EEPROM error/dismissal/successful retry, Continue
and Quit. Both native process and capture command exit zero at frame 4,153,
retaining 260 captures. All cases pass: three saved records, two refusals, four
normal player-control returns and one title soft reset. Twelve neutral
checkpoints verify 73,728 black field pixels with native foreground present.
The shared critical-decoration loads do not select a pause-family image: BG3
is disabled and the visible source remains generated black.

Eleven new checker tests bring the audit suite to 137 passing tests, alongside
11 exporter tests, catalog/profile agreement, engine/scroll and CPU PPU checks,
the ARM64 build and EU QEMU/dummy-SDL startup. Retained inventory/quest Save and
130-case menu/fusion traces remain compatible; no fresh native pixel replay of
those routes is claimed. The initial saved-health-only fixture failure is
retained separately from the successful corrected run. See the
[Game Over audit](background-context-state-audit.md#native-game-over-save-retry-continue-and-quit-2026-10-26)
for artifacts, reproduction and limits. Other combinations, quest-state/region/
language coverage and intermediate effects keep the full context/state task
open; runtime adapter/loading/composition and device budgets remain separate.


## Native fusion interactions and explicit remaining scope (2026-10-26)

Added an 18-case EU/English native fusion route: six interaction paths at three
installed health decorations. All cases validate, including 12 cancellations,
three mismatch recoveries, six awards and three world-event/local-map-hint
handoffs. Native ownership/count/piece consumption, input, brightness cadence,
source clearing and original-room restoration pass. Independently ROM-decoded
bottom rows match 39 checkpoints / 9,360 pixel comparisons. The native process
exits zero at frame 7,835. The initial capture-command checker rejected a
corner-only mask hidden by the fixed frame; final offline validation passes the
unchanged complete trace/captures using the independent full-row reference.
The failed command outcome is retained rather than relabeled successful.

Eleven new checker tests bring the audit suite to 148 passing tests, alongside
11 exporter tests, catalog/profile agreement, engine/scroll and CPU PPU checks,
the ARM64 build and isolated EU QEMU/dummy-SDL startup. Retained Game Over,
inventory/quest Save and menu/fusion traces still validate. Details and artifacts
are in the [fusion interaction audit](background-context-state-audit.md#native-fusion-cancellation-mismatch-and-success-handoffs-2026-10-26).

The [closure checklist](background-context-closure-checklist.md) now replaces
open-ended historical remaining-coverage paragraphs: the first context/state
parent task has seven named coverage groups, with C1 complete and six remaining.
The implementation list has 29 unchecked parent items; these new checked audit
slices subdivide one parent and do not reduce that total. Additional ROMs and
physical-device acceptance are explicitly distinguished from available EU work.


## File commits and post-credits Save coverage (2026-10-26)

Completed closure groups C2 and C3 with seven file-operation cases and four
post-credits Save cases on EU/English. The final combined capture command and
both native processes exit zero: ending completes at frame 2,179 (156 captures),
files at frame 2,827 (162 captures). All 36 source checkpoints pass: 49,152
black-field pixels for the ending and 448 ROM-derived file-field samples with
coherent legal animated palette frames. Native input, error waits, record
preservation, successful checksums and screen/title restoration pass.

The failed-erase fixture exposes native behavior rather than a new error
screen: deletion write results are discarded, the UI slot becomes empty and
the prior valid stored record survives. This is explicitly verified and remains
under existing C5 failure-propagation work. Earlier bootstrap/checker failures
are preserved separately from the successful final command. Exact fixtures,
limitations and reproduction are in the
[write interaction audit](background-context-state-audit.md#file-commit-and-post-credits-save-interactions-2026-10-26).

Thirteen new checker tests bring the audit suite to 161 passing tests; 11
exporter tests, catalog/profile agreement, engine/scroll and CPU PPU checks,
ARM64 incremental build and isolated EU QEMU/dummy-SDL startup pass. Retained
fusion, Game Over, inventory/quest Save and menu traces remain compatible.
Build options, artwork and goldens remain unchanged. The
[closure checklist](background-context-closure-checklist.md) has three completed
and four remaining catalog groups; the implementation list still has 29
unchecked parent items because the broad context/state parent remains open.


## Map navigation and remaining error/return coverage (2026-10-26)

Completed closure groups C4/C5. The native map route completes at frame 12,186
with 2,402 captures: 44 cursor/A entries and all 49 distinct reachable local
hint area/icon combinations. Final validation passes all 230 source checkpoints
(3,680 pixels), retained cursor/room, marker visibility and hint health mutation.
Existing 22-recipe scroll and conditional-map evidence is retained.

PC file erasure now commits both status copies durably before clearing the
visible slot. Device/host failure rolls back the EEPROM RAM snapshot and shows
the native save-error dialog instead of a false success. Ordinary save dirty-
retry behavior and non-PC deletion remain unchanged. Nine file cases verify
the corrected error path, a real blocked `.tmp` write, byte-identical durable
storage on failure and later successful erasure (38 field checkpoints). Six
extended Game Over cases cover B/START error dismissal, refusal after failure,
retry and Save-and-Quit (16 black-field checkpoints).

All native routes and final offline checks pass; the separate final Game Over
capture command exits zero. Diagnostic parent-command interruption/observer
failure and the exact distinct binaries remain documented in the
[C4/C5 audit](background-context-state-audit.md#native-map-navigation-and-remaining-error-paths-2026-10-26),
without claiming those enclosing commands succeeded. Fifteen new checker tests
bring the audit suite to 176 passing tests, plus 11 exporter tests and 20 storage/
quicksave/ownership cases. Catalog/profile, engine/scroll, CPU PPU, ARM64 build
and isolated EU startup checks pass. Original artwork, goldens and build options
are preserved. The [closure checklist](background-context-closure-checklist.md)
has five completed groups and only C6 localization / C7 regional fixtures left;
the broad context/state parent is still one of 29 unchecked parent tasks.

## Remaining EU localization closure (2026-10-27)

Completed C6 with 14 native routes in French/German/Spanish/Italian: 2,029
canonical cases plus four file-browsing flows and 56,196 checked captures.
Live save-header resource slots certify the requested language through source
installation and returns. Spanish's missing 57–64-pixel fusion-name width is
recorded explicitly at three canonical IDs; the 127 reachable cases retain all
15 family/health combinations. Other languages retain all 130 cases / 21 fusion
layout/health combinations. No synthetic name or substitute layout is accepted.

All 56 routes pass final lifecycle/language/hash verification. Initial Spanish
discovery/checker failures remain recorded separately; a fresh final Spanish
capture command passes and matches 254 ARM64 checkpoints. The native verification
build also preserves all 260 retained English scale-1 checkpoints and matches
a fresh ARM64 replay. The 185 audit tests, 11 exporter tests, catalog/profile,
engine/scroll and CPU PPU checks, final ARM64 build and isolated EU startup pass.
Exact command outcomes, binary/trace hashes, recovery evidence and reproduction
are in the [localization audit](background-context-state-audit.md#remaining-eu-localization-coverage-2026-10-27).

The closure checklist now has six completed groups and only C7 regional ROM
fixtures outstanding. The broad context/state parent remains open, and the
implementation list still has 29 unchecked parent items. Runtime adapter,
composition, artwork loading, effect resolution and device acceptance retain
their separate tasks.

## EU effect targets and initial HDMA continuation (2026-10-27)

Resolved the available EU source targets using 33 independently decoded image
recipes, all native edge owners and all five languages. The logo's outer white
field is backdrop while some opaque white logo pixels are BG2. The local fusion
hint's solid field is BG3 palette entry 95, not palette zero: their equal neutral
colors can diverge under masked fades. The catalog now distinguishes those sources.
Native title, pause, prologue, fusion, figurine, Game Over, credits and travel
effect timelines, real staff-roll/FadeMain/FadeVBlank mask checks, and bounded
RGB888 effect helpers are recorded in the [effect audit](background-effect-target-audit.md).
Regional fixtures, native intermediate screenshot acceptance and compositor
publication remain separate; this does not claim image-margin rendering.

Classified the existing HDMA producers and implemented safe fixed-source BLDY
continuation without additional hardware callbacks or a larger IO snapshot
pool. Varying/unknown channels and window effects retain the native-height
fallback. Full-canvas iris spans match the real native circle generator for all
five profiles, but scene-wide radius policy and publication are still pending;
the HDMA/full-canvas parent task remains open. CPU PPU geometry/sanitizer tests,
EU logo/title byte-exact parity, catalog/exporter agreement, the ARM64 build and
isolated EU startup pass. The implementation list now has 28 unchecked parent
items; C7 remains open for missing USA/JP ROM fixtures.

## Regional policy revision (2026-10-27)

EU is the tested regional fixture. USA and JP are supported on a best-effort
basis through source and regional-data review; no USA/JP ROM testing or regional
acceptance is claimed or required. Historical notes retain their original
evidence and prior scope. The context/state catalog task is closed under this
revised scope; later feature and effect work must preserve regional code paths.

## Initial RG device measurements (2026-10-27)

On the physical RG DS Plus, a freshly built multi-region ARM64 release
(`repro_harness=n`, width capacity 569; SHA256
`776155a7a9409fb484f36a600fadb79f7e141ad000d8612161f5a2c0e31b4edd`)
ran against the installed EU ROM and assets from an isolated temporary directory.
The configuration used native 3:2, nearest internal scale 1, four OpenMP threads,
legacy pacing, no FPS cap and disabled configured VSync. SDL used the Wayland
`opengles2` renderer. It reached AgbMain; the initial title sequence produced
six 120-frame profiler windows. Game averages were 0.498–0.598 ms, presentation
15.987–16.121 ms, and rasterization 1.784–3.883 ms per frame. The compositor
appears to pace presentation near 60 Hz despite the uncapped configuration;
these are not uncapped throughput figures or frame-time percentiles.

Samples from `/proc/<pid>/smaps_rollup` at seconds 8–15 peaked at 123,672 kB
RSS and 106,591 kB PSS. The Mali `gpu_memory` debug counter reported a total
of 1,848 and three context values 1,413, 430 and 5, with a game-PID context
present in debugfs. Units and correspondence between the opaque counter entries
and contexts were not established; these values must not be interpreted as
bytes or attributed to the game. They omit any driver allocations not exposed
by that counter. An earlier repro-enabled run covered the title and file-select
transition, with observed peaks of 124,876 kB RSS and 107,795 kB PSS; its
stripped executable SHA256 was
`70cf16719dd90cd397bda8958d983174ddd0ddcc6c62c8558662f19834624490`.
The frontend was restored, and the installed package and saves were preserved.

These initial title measurements are supplemented by the gameplay fixture and
process-specific allocation measurement below. Frame-time distributions and
multi-scene, profile and thread-count acceptance remain separate.

## RG baseline follow-up (2026-10-27)

A repro-enabled ARM64 build (stripped SHA256
`70cf16719dd90cd397bda8958d983174ddd0ddcc6c62c8558662f19834624490`)
ran the existing EU synthetic menu-audit bootstrap on the physical device at
native 3:2, nearest internal scale 1, four OpenMP threads and Wayland/OpenGL ES.
It entered `TASK_GAME` at frame 515 and captured a mode-0 PPU snapshot after
300 game ticks. The audit writes traces and PNGs, so game times include
instrumentation and this is not an uninstrumented gameplay benchmark. After
entry, 120-frame windows averaged 2.093–5.729 ms game, 11.316–14.526 ms
present and 2.989–3.347 ms rasterization, excluding the transition window
(which had a 697.19 ms game maximum). Presentation is compositor-paced near
60 Hz; these are averages, not p50/p95/p99 or uncapped throughput. Sampled
RSS/PSS peaked at 136,784 / 119,699 kB over seconds 8–30.

The game-PID Mali `mem_allocs` table at second 15 reported 5,730,304 bytes
of native committed allocations and 9,584,640 bytes of imported UMM
allocations. These figures can overlap other memory accounting and must not
be summed with PSS. The corresponding device-wide `gpu_memory` sample was
1,844 units; a Mali driver implementation with the same output format reports
[used pages](https://chromium.googlesource.com/chromiumos/third_party/kernel/+/2316e798a647ceff30fcd9b014fafa931b33555b/drivers/gpu/arm/bifrost/mali_kbase_gpu_memory_debugfs.c),
which suggests 7.20 MiB at the device's 4 KiB page size, but the exact running
driver source was not verified. The per-context allocation table is the
process-specific evidence; opaque context labels alone do not establish PID
attribution. Full multi-scene and percentile acceptance remains in section 8.

Raw logs and the device-side sampling script are retained locally under
`.tmp/rg-device-baseline-2026-10-27/`. A separate attempt to load the device's
older copied quicksave reported restore and then segfaulted; it is excluded from
the baseline. The synthetic fixture succeeded and the frontend was restored.

## Static-window and shake safety follow-up (2026-10-27)

Static WIN0, WIN1 and OBJWIN now fail closed to a 160-row raster when
requesting a taller frame, even if no HDMA callback is installed. This preserves
the existing native output and does not supply extended window geometry.

The wide-scene adapter publishes a tall-shake-unsafe capability if any of the
active shake magnitude’s eight XY phases exceeds the 192-row tile-window
envelope. The complete sequence is checked before camera work so a later phase
cannot surprise the eventual aperture selector. The current world remains at
160 rows; the selector has not yet been extended to consume this flag. Manager
graphics residency, upload timing, full-canvas iris policy/publication and
other unclassified effects remain open.

The height/window and world-view regressions pass with ASan/UBSan (leak
scanning disabled for the sandbox), as does catalog/profile publication. The
ARM64 repro build succeeds, EU logo/title PPU parity matches the retained
byte-exact goldens, and an isolated EU dummy-SDL smoke reaches AgbMain. The
release xmake configuration is restored after capture verification.

## HDMA and scene-wide iris geometry completion (2026-10-27)

The continuation classifier retains 160 hardware callbacks and rejects varying
or unsupported channels. Only a verified fixed-source halfword write to BLDY
with windows disabled may continue to additional rows; static or unclassified
windows remain native-height. These are interim safety gates, not 4:3 enablement.

The native scene-wide fade-iris producer now publishes a bounded descriptor.
Its full-canvas margin spans use the native midpoint circle and a radius that
scales from zero to the minimum discrete radius covering the canvas as the
native radius advances from 0 to 150. A consumer must clip the spans to added
margins; the native 240 × 160 window and its pixels stay authoritative. Local
lights, portal windows, triangles, ellipses, unsupported window controls and
offscreen centers do not publish this scene-wide policy. Their render aperture
continues to fail closed.

Pending ownership is replaced whenever native window DMA is replaced and is
latched with the actual VBlank DMA snapshot, so repeated presents see the same
shape as the current native window. DMA cancellation, fade initialization and
quicksave restore clear host-only state. The descriptor is ready for the
separately tracked margin compositor; no PNG margin drawing, additional raster
continuation, or 4:3/32:9 availability is claimed here.

The native fade/DMA lifecycle regression tests opening, closing, held-closed,
replacement, cancellation and the VBlank publication boundary. Native circle
agreement covers all five profiles and edge centers; independent ASan/UBSan
checks exercise all 151 radii across profile edges. PPU geometry, catalog,
world-view and persistence regressions, the ARM64 build, EU byte-exact logo/title
parity and EU dummy-SDL startup pass.

## Room aperture and streaming gate (2026-10-27)

The viewport now resolves a bounded room aperture and rejects taller geometry
for unsafe shake, native transitions, unverified scenes, rooms with managers,
missing live maps or incomplete map upload. A future scene validator must
explicitly opt in for each room; no production caller enables that option and
the 4:3 profile remains disabled. The world raster height consumes the room
aperture rather than the canvas height.

Taller map rows are filled while the native aperture is still active. Publication
requires the same room, tile origin and row count at fill and actual VRAM upload;
a changed room, camera or shake footprint restarts staging. A manager created
and removed between boundaries still blocks the room until reload. The
pre-presentation check catches late manager changes. The whole shake sequence
determines required padding, and starting a shake on a tall aperture refreshes
its backing rows before upload. Native-height fill/upload sizes are unchanged.

The conservative per-manager policy rejects all managers rather than certifying
unreviewed graphics residency. The town and festival loaders replace two shared
VRAM spans immediately on PC; the village loader replaces eight chunks over
eight updates and may switch groups mid-stream. Synthetic tests exercise those
actual manager bodies, all EU village groups, town/festival groups and region
boundaries, with USA/JP source paths tested against synthetic data only. A PC
village no-region case now retains its current valid group or waits for a first
group instead of using an uninitialized/sentinel index. Additional manager
residency, scene, sprite, effect and layout work remains necessary to expose 4:3
in manager-occupied rooms; no ROM-based USA/JP acceptance is claimed.

Production-body staging, native/tall world pixel, manager upload, widescreen
engine and catalog regressions pass; staging and manager tests run under
ASan/UBSan. ARM64 build, EU logo/title PPU parity and EU dummy-SDL startup pass.
The test-only tall profile used for staging does not change release availability.

## Entity and OAM consumer safety gate (2026-10-27)

Updated identified guard, Dampé, Cucco and Gyorg screen-edge behavior to use
the effective world aperture, retaining native thresholds and spawn positions.
Special-window and fixed-table effects remain on native fallbacks. The
[entity/OAM audit](sprite-coordinate-audit.md#entity-effects-and-oam-consumer-gate-2026-10-27)
records the source-level decisions and deliberately denies unreviewed dynamic
entities, auxiliary entities and transient creations from tall-scene admission.
Signed metadata is now invalidated on raw attr0/attr1 writes, and all visible
OAM slots must be tagged before a prospective tall scene can be approved.

PPU2 perfcap refuses to serialize a frame with signed OBJ metadata. The
shared-framebuffer publisher marks raw OAM unavailable in that case, and its
in-tree Vulkan-RT consumer falls back to the composite image. These guards do
not add 4:3 support or certify out-of-tree OAM readers. Entity-boundary, OAM
write, shared-framebuffer, snapshot, staging, presentation and signed-OBJ
regressions pass, alongside the ARM64 build, EU native PPU parity and startup.

## Fixed-screen layout and black-fade arrival

Fixed-screen classification is published with the viewport at simulation
boundaries. The native raster is projected into the selected canvas using the
published native rectangle in the SDL renderer, SDL_GPU and surface paths;
the HD-text overlay consumes the same frame rectangle. The native composition
is retained during menu-to-menu handoffs until the incoming screen is installed.
Opening and retelling story pages are classified only while they replace the
auxiliary room, and gameplay is restored when the room maps return.

EU file-select presentation captures at scale 4 compare byte-for-byte inside
the native crop at 3:2, 16:9 and 21:9, including two animation frames. The
16:9 black margins are 88 and 92 output pixels. In a 16:9 native-raster replay,
all 151 opening and 141 retelling story captures from image installation to
world/clear handoff match their retained 3:2 PNGs. These are SDL software
captures; GPU display and unsupported profiles remain part of acceptance.
Artifacts are under `.tmp/fixed-screen-presentation-check/` and
`.tmp/fixed-screen-lifecycle/`.

An initial 16:9 menu replay hit its 300-second limit before completing the
130-case route. Among its first 106 canonical cases, 622 installed-screen
captures with retained matching frame IDs are byte-identical to the earlier
native menu audit. All 212 ready/held checkpoints from the original 106-case
menu audit also match by case ID, as do 46 available fusion-audit checkpoints.
The interrupted command is not a completed route. A fresh 16:9 replay then
completed all 130 cases at frame 10,033, and all 260 ready/held checkpoints
match the retained complete 3:2 fusion audit byte-for-byte.

One transition continuation is admitted for a palette-only black-fade arrival
after the destination map has been filled and uploaded, with no windows or
active HDMA and all existing room, manager and signed-OBJ gates satisfied.
The staging, 192-row palette and camera/script-completion fixtures verify that
case and reject unrelated fades. No production caller currently certifies a
tall room and 4:3 remains unavailable; rolling, mosaic, iris and unclassified
transition continuations retain their native-height safety fallback.
