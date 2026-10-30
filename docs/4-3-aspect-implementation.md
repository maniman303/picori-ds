# Plan: finish proper 4:3 (256 × 192)

Status: WP1 + WP2 implemented; release profile still gated by WP3–WP5  
Scope owner: code/geometry/streaming/validation/composition/tests  
Artwork: the user supplies 4:3 artwork. This plan does not create, edit or
export PNGs.

## Implementation rules (upstream-friendly, EU-first)

These rules apply to every work package. They are part of the plan, not
optional style advice.

- **Build on the current tree; inject rather than rewrite.** Prefer a new
  port-side module (`port/port_*.c/.h`) plus one or two small hook calls over
  moving, renaming or restructuring existing decomp/port code. Keep diffs
  localized enough to rebase onto upstream commits. Do not fork `src/` files or
  copy large engine functions to change one bound.
- **Keep native/wide/parity output byte-identical when 4:3 is off.** New paths
  must be inert at native and existing wide profiles, and must stay behind the
  release capability until the relevant work packages pass.
- **Inject at existing boundaries.** Room load, map fill, VRAM upload, scene
  publication, manager creation, entity creation, quicksave restore and present
  already have named port hooks. Extend those hooks instead of adding new
  engine-wide call sites.
- **Publish data, not profile constants.** Consumers read the published
  rectangles/capabilities; new code must not test for `256`, `192`, `4:3` or a
  specific profile id.
- **EU is the supplied and validated fixture.** Build, capture, acceptance and
  performance work use the EU ROM: `xmake f -y --game_version=EU ...`.
  EU/English is the region recorded in the acceptance document.
- **USA/JP are source-level best effort.** Regional table/offset review and
  synthetic tests are expected, but no USA/JP gameplay or capture claim is made
  without the real ROMs. Never substitute one region's data for another.
- **ROM-free tests run everywhere; ROM captures are evidence.** Geometry,
  policy, staging, validator and row-count tests must compile and run on a host
  without a ROM. EU captures and device runs are acceptance evidence, not a
  replacement for the host tests.
- **Preserve mergeability.** Keep public headers C-compatible, keep port-only
  code under `port/`, and avoid growing the vendored PPU API unless the
  composition cannot be expressed through presentation.

## Target definition

“Finished 4:3” means:

- **Live gameplay** uses the full 256 × 192 world aperture in normal field,
  town, dungeon, boss, Minish-path, weather and interior rooms. Towns and
  dungeons are not allowed to remain permanently on a native fallback.
- The same live-world implementation is profile-general. A future proper
  16:9 canvas (for example 288 × 162) should be addable as a profile/config
  and acceptance task without hardcoded 4:3/192 assumptions.
- **Fixed UI screens** — title, file select, inventory, quest, pieces,
  techniques, all map views, fast travel, hints, sleep/save, Game Over, credits,
  ending, fusion, figurines — keep their original 240 × 160 composition at
  `(8,16)` inside 256 × 192.
- **Context- and effect-aware artwork** fills the added margins behind fixed
  screens only. Gameplay never receives static artwork.
- Missing 4:3 artwork renders solid black; it never aliases another profile,
  family or health state. Supplying the actual art is the user's work.

This is one broad target, not a reduced “simple rooms only” release. The
implementation is complete only when normal towns, dungeons and the other
listed gameplay families work in 4:3. Classified transitions/effects may keep a
native aperture; normal rooms may not.

## Geometry model and future profiles

The live-world implementation must not hardcode 256 × 192. Treat 4:3 as the
first tall profile, not as a special renderer mode.

- A profile publishes a **canvas** `(W,H)`. Fixed screens use the native
  240 × 160 composition centered at `(floor((W-240)/2), floor((H-160)/2))`.
  For 256 × 192 that is `(8,16)`; for a future proper 16:9 288 × 162 it is
  `(24,1)`.
- A live room publishes a **world aperture** `(x,y,w,h)` bounded by the room
  and the canvas, centered when smaller than the canvas. Missing rows/columns
  use the existing procedural void/fill policy, never artwork.
- HUD/dialogue are positioned against the canvas, independently of the world
  aperture.
- All raster, map-row, camera, staging, layout, presentation, screenshot and
  HD-text decisions consume the published rectangles. No new code should test
  for `256`, `192`, or 4:3 specifically.
- `MODE1_GBA_HEIGHT` remains 160 for GBA state. Compile-time render capacity is
  a bound, not a profile: a 288 × 162 frame already fits the current 569-width /
  192-height capacity. Only a future profile taller than 192 would require a
  capacity bump.
- Parameterize tests across `240 × 160`, `285 × 160`, `256 × 192`, `288 × 162`
  and `374 × 160` so adding proper 16:9 later is a profile/config and acceptance
  task, not a second renderer implementation.
- Room heights of exactly 160 remain valid: the world aperture is 160 rows,
  centered in a 162-row canvas, with one fill row above and below.

## Current state

Already implemented and tested:

- 256 × 192 profile, enum/serialization, native-content offset `(8,16)`.
- 192-row PPU capacity, 27-row map fill/upload, camera/scroll math, shake
  coverage classification, right/bottom shadow rows.
- Fixed-screen native composition, 4:3 margin composition, title-cap bounds,
  palette/brightness/alpha effect inputs and missing-art black fallback.
- Tall HUD/dialogue layout, HD-text Y remapping, software and GPU shader remap.
- Runtime background catalog/cache/context adapter for `43`; missing art
  already fails closed to black.
- ROM-free tests for staging, geometry, camera, OAM, HUD/dialogue, presentation,
  context/effect composition and memory budgets.

Still deliberately disabled:

- `Port_Viewport_AspectSupported()` rejects tall profiles outside the
  `PORT_VIEWPORT_TEST_TALL` test build or a build that sets
  `PORT_VIEWPORT_RELEASE_TALL=1`. The production value remains `0` until
  WP3–WP5 pass.
- Tall world admission remains default-deny for managers, dynamic entities,
  unclassified shake/transitions and unverified OAM. Normal manager-backed
  rooms therefore still resolve to the safe aperture/fallback.
- GPU raster supports only 160 rows (CPU fallback exists; tall canvases use the
  CPU raster, wide 160-row canvases keep the GPU path).
- 4:3 artwork is incomplete by design; black fallback is the contract.

Implemented (WP1 + WP2):

- `Port_Viewport_Composition()` and the PPU live-world aperture rectangle make
  the live/fixed/fallback compositions explicit. Canvas HUD/dialogue, world
  BG/OBJ placement, margin fill, HD text, captures and presentation all agree.
- `port/port_tall_scene.c/.h` is the single, ROM-free policy for candidate
  aperture, required rows and the combined validation decision.
- `Port_Widescreen_MapUploaded()` is the production caller for
  `Port_Widescreen_SetTallValidated(1)`; validation is derived from the room
  key, camera tile origin, aperture height, staged fill/upload row count, map
  pixels, shadow rows, shake safety and manager/entity/OAM policy. It cannot be
  asserted by a caller.
- Manager creation, room load, quicksave restore and fixed-screen replacement
  invalidate the previous room's validation. Tests cover 4:3 and the 162-row
  synthetic extent, including unchanged native/wide output.

## Hard requirement

Do not expose the release profile until normal live gameplay can use 192 rows
in towns and dungeons, and fixed UI + artwork composition is verified at 4:3.
A native-aperture fallback is acceptable only for narrowly classified
transitions/effects, not as the final behavior for normal rooms.

## Work packages

### WP1 — Release enablement and geometry-general composition

1. Replace the `PORT_VIEWPORT_TEST_TALL`-only availability path with a release
   capability once WPs 2–5 pass. Availability is by published profile/capacity,
   not by a hardcoded 4:3 check.
2. Separate the two compositions explicitly:
   - **Live world:** the published world aperture inside the canvas,
     canvas-anchored HUD/dialogue. Full canvas when the room permits; centered
     world aperture plus fill when the room is smaller.
   - **Fixed screen:** one native 240 × 160 composition at the canvas-centered
     offset plus context/effect-aware margins.
3. Make `Port_Viewport_RasterHeight`, visible dimensions, layout gates, HD
   text, touch/capture consumers and presentation agree on which composition is
   active. Bottom HUD/dialogue must never fall outside the raster for any
   canvas size.
4. Add a forced-fallback composition for classified transition/effect cases
   that cannot use the full aperture. The fallback must be safe, centered and
   free of static gameplay artwork.
5. Remove profile-specific constants from world-aperture, map-row, camera,
   staging, layout and presentation code. Use the published rectangles and
   parameters throughout.
6. Tests: live-vs-fixed handoff, canvas/raster agreement, room-smaller-than-
   canvas centering, 288 × 162 synthetic extent, and unchanged native/wide
   outputs.

### WP2 — Production tall-scene validator

1. Add a real caller for `Port_Widescreen_SetTallValidated(1)` after room
   load/upload. Validation must require the same room key, tile origin,
   required row count for the **current aperture** and VRAM upload. Prefer a
   new pure `port/port_tall_scene.c/.h` policy module plus the existing
   `Port_Widescreen_MapUploaded()` hook; do not move the room loader.
2. Require installed map pixels, all aperture BG rows, bottom/right shadow
   rows, safe whole shake sequence, classified transition/effect state and
   manager/entity/OAM policy. The row count is derived from the published
   world aperture, not fixed at 192.
3. Clear validation on room change, reload, manager creation, quicksave
   restore and scene replacement.
4. Publish `TALL_VALIDATED`, `TALL_ROWS_READY`, `TALL_MANAGER_FREE`,
   `TALL_ENTITIES_EMPTY`/entity-safe and `TALL_OAM_VERIFIED` from one boundary.
5. Tests: positive/negative room keys, stale staging, late manager creation,
   transient entities, repeat presents and invalidation at 162, 192 and other
   supported aperture heights.

### WP3 — Towns, dungeons and manager/tileset residency

This is the main schedule risk. Normal town/dungeon rooms must become tall-safe.

1. Replace global manager denial with per-manager capabilities/allow-lists
   keyed by the published aperture size.
2. Audit and prove row residency for:
   - Hyrule Town normal/festival group swaps;
   - Minish Village eight-step group streaming and palette changes;
   - dungeon/area tileset managers;
   - horizontal and vertical Minish-path backgrounds;
   - animated backgrounds, rolling barrels, cloud/weather/parallax overlays;
   - special BG layouts and room-map variants.
3. Prove bottom rows never expose outgoing group tiles/palettes during a
   group switch or intermediate asynchronous update.
4. Keep only genuinely unverified/effect-owned managers on native fallback;
   list them explicitly.
5. Add manager-body tests at 162, 192 and other supported aperture heights
   plus EU captures for representative town, dungeon, Minish-path and weather
   rooms.
6. Cover every manager family that owns normal room graphics. Capturing every
   individual room is not required, but a normal room that still falls back is
   a failure of this plan, not a pass.

### WP4 — Entity and OAM coverage

Normal gameplay entities must work in tall rooms.

1. Replace `TALL_ENTITIES_EMPTY` as the only policy with a per-kind/class
   allow-list plus signed-OAM verification.
2. Audit Y visibility, spawn/park and draw clipping at rows above 160 for the
   active aperture (161 for 162-high, through 191 for 192-high) for:
   player, NPCs, common enemies, bosses, projectiles, drops, effects, cutscene
   actors and minigame actors.
3. Require valid signed metadata for every visible OAM slot used by tall
   admission.
4. Keep native-only special cases (for example the barrel spiderweb affine
   table) on native fallback.
5. Tests: spawn/cull at Y=160/191, negative Y, hidden/parked pieces, transient
   actors, OAM DMA publication and native byte preservation.
6. Cover every common gameplay entity class. Unique native-only edge cases may
   fall back, but they must not make ordinary towns/dungeons stay native.

### WP5 — Effects, windows, transitions and shake

1. Consume `TALL_SHAKE_UNSAFE` before camera work.
2. Classify iris/rolling/portal/window/HDMA effects:
   - extend only verified constant/fixed-source cases;
   - keep local lights, portal windows, triangles, ellipses and unclassified
     windows native.
3. Keep rolling/special transitions native until independently extended;
   they must not force normal steady rooms to native.
4. Add intermediate-effect fixtures at 162, 192 and native raster heights.
5. Ensure no static artwork is selected for gameplay effects.

### WP6 — Fixed UI and 4:3 artwork composition

This is code support only; artwork creation is excluded.

1. Verify every fixed screen uses one native 240 × 160 composition at the
   canvas-centered offset: `(8,16)` for 4:3, `(24,1)` for 288 × 162, etc.
2. Verify the existing context/effect-aware compositor fills only the added
   margins with the correct profile artwork or black fallback.
3. Verify palette fades, brightness, alpha, masked banks, forced blank,
   windows and title-cap completion affect margins once, correctly.
4. Verify missing profile files render black and never alias another profile,
   family, health state or context.
5. Add 4:3 fixed-screen captures with conspicuous artwork present and absent.
   The user will replace the artwork later; no code changes should be needed.

### WP7 — Renderer and presentation coverage

1. CPU software PPU remains the reference for every supported aperture height.
2. GPU raster stays on CPU fallback for non-160 heights; verify SDL_GPU
   presentation of 256 × 192 and 288 × 162.
3. Verify nearest/internal scale/xBRZ, color correction, persistence, filters,
   screenshots and shared framebuffer at 4:3 and the 288 × 162 extent.
4. Voxel view and custom `.glslp` presets keep their documented 2D/stock
   fallback.
5. Verify HD dialogue and touch/capture consumers use the canvas composition
   for any profile.

### WP8 — Acceptance and performance

1. Host captures at 256 × 192 / exact 4× 1024 × 768, plus synthetic
   288 × 162 geometry tests for the future proper-16:9 profile:
   - field, town, dungeon, boss, Minish path, weather and interior gameplay;
   - HUD and bottom dialogue;
   - fixed screens with artwork and missing-art black;
   - intermediate fades/effects;
   - profile switches while message/fade/menu is active.
2. Software/GPU base-framebuffer parity; document any presenter block-edge
   sampling.
3. Physical RG: 4:3 60 fps, frame-time distribution, PSS/RSS, 1/2/4 workers,
   thermal saturation. 4:3 adds ~28% pixels over 240 × 160.
4. EU is the tested region and the supplied fixture; run host tests and EU
   captures with `--game_version=EU`. USA/JP are source/table review only and
   must not be described as validated without their real ROMs.
5. Re-run all native/wide/PPU/background/title regressions.

## Artwork boundary

The user owns:

- creating `assets/override/43` artwork for all contexts/families/health states;
- contact sheets and artist-facing exports;
- deciding which fixed screens get custom 4:3 artwork.

The code must only guarantee the `43` contract, context/effect-aware margin
composition, black fallback and no cross-profile/family aliasing.

## Progress log

- **WP1 implemented.** `Port_Viewport_Composition()` is now the single
  composition decision (`NATIVE`, `LIVE_WORLD`, `FIXED_SCREEN`,
  `NATIVE_FALLBACK`). The software PPU has a live-world aperture rectangle:
  BG1–BG3 and world OBJ render into the published aperture, BG0 HUD/dialogue
  stay in canvas coordinates, and margins keep the backdrop. `OBJ` pieces
  emitted by `DrawUIElements` are tagged canvas so they are neither offset nor
  clipped by the world aperture. Raster height, visible width, layout,
  presentation, HD text and screenshots all read the same composition.
  Classified transitions/effects use a native 240 × 160 fallback composed with
  procedural/black margins and never receive static artwork. Release
  availability is profile/capacity based (`PORT_VIEWPORT_RELEASE_TALL`, still
  `0` until WP3–WP5 pass); the literal 192 in staging/camera math is gone.
- **WP2 implemented.** `port/port_tall_scene.c/.h` owns candidate aperture,
  required rows and the combined validation decision. `Port_Widescreen_MapUploaded()`
  is the production caller. Validation requires room key, camera tile origin,
  aperture height, staged fill/upload row count, installed map pixels, right
  shadow rows for the candidate width, whole shake sequence, map-live state,
  no classified native transition, manager-free, entity-empty and tagged OAM.
  Room load, manager creation, quicksave restore and fixed-screen replacement
  invalidate the commitment.
- **Tests.** Added `tools/tall_scene_regression_test.py`,
  `tools/viewport_composition_regression_test.py` (also compiled in a
  `PORT_VIEWPORT_RELEASE_TALL=1` variant) and
  `tools/tall_composition_regression_test.py` (4:3 and 288 × 162 aperture,
  canvas HUD, OBJ offset/clip, margin fill, byte-identical legacy output).
  Extended the staging/engine/presentation regressions for the native fallback
  and 162-row extent. EU host build and the ROM-free tall/wide/PPU suites pass.
- **EU capture.** A headless EU test build with `PORT_VIEWPORT_RELEASE_TALL=1`
  and `aspect_mode=4:3` captured area `0x03` room `0x08` at 256 × 192. The
  room's tall aperture was not yet validated (manager/entity policy), so the
  capture correctly shows the 256 × 160 live-world aperture centered with one
  16-row fill band above and below, canvas-anchored top and bottom HUD, and no
  artwork in the margins. Artifact:
  `.tmp/wp1-wp2/eu-field-area03-room08-256x192.png`.
- **Remaining.** WP3 manager/tileset residency, WP4 entity/OAM allow-lists,
  WP5 effect/window classification, then flip the release capability and run
  WP6–WP8 acceptance. Until then towns/dungeons/effects may still resolve to
  the safe native aperture/fallback.

## Sequencing

1. WP1 — composition boundary.
2. WP2 — validator.
3. WP3 — managers/tilesets (can start per family).
4. WP4 — entities/OAM (parallel with WP3 where possible).
5. WP5 — effects/transitions.
6. WP6 — fixed UI/art composition verification.
7. WP7 — renderer/presentation.
8. WP8 — acceptance/performance.

## Definition of done

- 4:3 selectable in release builds with capacity ≥256 × 192.
- Normal towns, dungeons, field, bosses, Minish paths and weather render true
  192-row world content; native fallback is limited to explicitly classified
  transitions/effects.
- The live-world path is profile-general: a future 288 × 162 proper-16:9
  profile can be added without a second renderer/composition implementation.
- HUD/dialogue are canvas-anchored and visible.
- Fixed screens remain native 240 × 160 at `(8,16)` with correct artwork/black
  margins and effects.
- No manager, entity, OAM or effect path can expose invalid rows.
- Software and GPU presentation are correct; optional paths fall back safely.
- RG holds native 60 fps with documented headroom.
- Missing 4:3 artwork is black only; artwork creation remains the user's work.

## Estimate

This plan has a single broad target, so there is no reduced “core only”
estimate:

- **Full broad 4:3:** roughly **11–17 focused work packages /
  3–4.5 working weeks**.

The schedule is dominated by WP3 (manager/tileset residency) and WP4
(entity/OAM validation), not by the rasterizer or UI layout. Adding a future
288 × 162 profile after this plan should be a profile/config and acceptance
task, not a second live-world implementation.
