# Plan: finish proper 4:3 (256 × 192)

Status: WP1–WP7 implemented; live-gameplay margin fade implemented; WP8
acceptance/performance and the multi-group Minish Village residency design
remain; release profile still gated by those items
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
- Live-gameplay margin fade: every map-backed live-world composition paints
  its published canvas, and `port/port_edge_fade.c` fills the area outside the
  world aperture with a five-pixel edge-color fade into black. Original world
  pixels and canvas-anchored HUD/dialogue are preserved; a black source border
  simply fades black to black.
- Presentation coverage for tall canvases: the existing width-569 xBRZ pools
  cover 4:3 and 288 × 162, the CPU reference handles 162/192 rows, and the GPU
  presenter accepts the composed tall frame. The over-budget 569 × 192
  synthetic Cartesian case still falls back as audited.
- ROM-free tests for staging, geometry, camera, OAM, HUD/dialogue, presentation,
  context/effect composition, memory budgets and the margin fade.

Still deliberately disabled:

- `Port_Viewport_AspectSupported()` rejects tall profiles outside the
  `PORT_VIEWPORT_TEST_TALL` test build or a build that sets
  `PORT_VIEWPORT_RELEASE_TALL=1`. The production value remains `0` until the
  multi-group Minish Village residency design and WP8 acceptance pass.
- Tall world admission still fails closed for the explicitly denied manager
  families, multi-group Minish Village views, unclassified shake/transitions
  and untagged OAM. Normal manager-backed rooms on the allow-list now render
  tall when their residency proof passes.
- Hardware windows, GBA affine modes and non-constant HDMA keep the native
  aperture; only a verified constant-brightness HDMA may extend into the tall
  canvas. Rolling/special transitions and the explicitly denied effect
  managers remain native.
- GPU raster supports only 160 rows; tall canvases use the CPU reference and
  reach the swapchain through the normal GPU presenter. Wide 160-row canvases
  keep the GPU path.
- 4:3 artwork is incomplete by design; black fallback is the contract.
- The margin fade is limited to map-backed live gameplay. Fixed screens,
  classified native transitions/fallbacks and non-map overlays keep their
  existing composition. The fade replaces the old coloured/procedural margin
  fill with black beyond the five-pixel band.

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

### WP9 — Live-gameplay margin fade

Every map-backed live gameplay composition publishes a canvas that can be
larger than the room's world aperture (side bars, and top/bottom bands when the
room is shorter than the canvas). Instead of a hard black cutout, extend the
map visually by five pixels on every side: repeat the nearest world-edge pixel
color and darken it in five steps so the fifth pixel is black. The original
world rectangle stays untouched, including any black border art it already has;
black fades to black.

1. **Geometry.** The fade band is `PORT_EDGE_FADE_PIXELS` (5) pixels outside the
   published world rectangle. Straight edges use exact five-pixel steps; corners
   use rounded Euclidean distance so the fade outline curves instead of forming
   a square.
2. **Composition.** The area outside the world is black. The PPU publishes a
   per-pixel UI mask for BG0 and canvas-tagged OBJ; the fade skips those pixels
   and, where UI covers the world edge, uses the nearest visible map pixel
   instead of the UI colour or black. UI stays completely separate from the
   world fade. Fixed screens and classified native
   transitions/fallbacks never receive the fade.
3. **Presentation.** Normal map-backed live gameplay renders at the published
   canvas width (including wide profiles), so the fade is inside the frame and
   follows the same presentation/screenshot path. Margin canvases bind signed
   host OAM positions, and canvas-tagged OAM is culled against the full canvas,
   so HUD sprites and right-edge button hints keep their canvas coordinates.
4. **Implementation.** `port/port_edge_fade.c/.h` is a pure, allocation-free
   post-pass over the composed frame, called once after the software/GPU raster
   and before display post-processing. It is a no-op when the world already
   fills the canvas.
5. **Tests and captures.**
   - ROM-free: side bands, top/bottom bands, corners, exact five-step factors,
     black-border preservation, HUD preservation, full-canvas no-op and
     invalid-input guards.
   - EU captures at 4:3 and 16:9 for a narrow room and a full-canvas field.
   - Native/wide/PPU/background regressions must keep their original world
     pixels; only the newly published margin pixels change.

## Artwork boundary

The user owns:

- creating `assets/override/43` artwork for all contexts/families/health states;
- contact sheets and artist-facing exports;
- deciding which fixed screens or environments receive custom artwork.

The code must only guarantee the `43` fixed-screen contract, the
context/effect-aware margin composition, black fallback, no cross-profile/family
aliasing, and the live-gameplay margin fade.

## WP3/WP4 capability audit

Manager and entity admission is now an explicit, frame-evaluated policy in
`port/port_tall_scene.c`; `port/port_linked_stubs.c` scans the live engine
pools. A normal aperture always admits.

### Managers

Allowed (logic, entity spawning, HUD text, audio, tile edits, BG3 overlays and
parallax backgrounds whose source spans the aperture):

`MINISH_PORTAL`, `DIGGING_CAVE_ENTRANCE`, `BRIDGE`, `SPECIAL_WARP`, `EZLO_HINT`,
`FIGHT`, `TILE_CHANGE_OBSERVE`, `ENTITY_SPAWN`, `MISC`, `WEATHER_CHANGE`,
`FLAG_AND_OPERATOR`, `HOUSE_SIGN`, `DELAYED_ENTITY_LOAD`, `FALLING_ITEM`,
`HOLE`, `REGION_TRIGGER`, `RAIL_INTERSECTION`, `MOVEABLE_OBJECT`,
`MINISH_SIZED_ENTRANCE`, `BOMBABLE_WALL`, `FLAME`, `PUSHABLE_FURNITURE`,
`ARMOS_INTERIOR`, `ENEMY_INTERACTION`, `MANAGER_29`,
`DESTRUCTIBLE_TILE_OBSERVE`, `ANGRY_STATUE`, `WATERFALL_BOTTOM`, `SECRET`,
`TILE_PUZZLE`, `GORON_MERCHANT_SHOP`, `VAATI_APPARATE`, `HYRULE_TOWN_BELL`,
`CAMERA_TARGET`, `REPEATED_SOUND`, `FLOATING_PLATFORM`, `ENTER_ROOM_TEXTBOX`,
`CLOUD_OVERLAY`, `STEAM_OVERLAY`, `RAINFALL`, `ANIMATED_BACKGROUND`,
`STATIC_BACKGROUND`, `POW_BACKGROUND`, `HORIZONTAL_MINISH_PATH_BACKGROUND`,
`VERTICAL_MINISH_PATH_BACKGROUND`, `MINISH_RAFTERS_BACKGROUND`.

Conditionally allowed (must prove one region group covers the candidate view):

- `HYRULE_TOWN_TILESET`: each region list must select at most one group.
- `MINISH_VILLAGE`: one region group **and** the eight-step streaming sequence
  must be complete (`timer >= 8`) before the tall aperture is admitted.

Explicitly native-only (unverified effects/streamers):

`LIGHT_RAY`, `LIGHT`, `LIGHT_LEVEL_SET`, `TEMPLE_OF_DROPLETS`,
`ROLLING_BARREL`, `CLOUD_STAIRCASE_TRANSITION`, `VAATI3_BACKGROUND`,
`VAATI3_INSIDE_ARM`, `VAATI3_START`, and any unknown manager id.

The Minish path/rafter backgrounds generate 32-row (256 px) maps from the full
room source; their parallax normalization now uses the candidate aperture
(`Port_Widescreen_RegionViewWidth/Height`) instead of the native 160/240 extent.

### Entities

Allowed with signed OAM metadata: `PLAYER`, `NPC`, `ENEMY`, `PROJECTILE`,
`OBJECT`, `PLAYER_ITEM` (players/projectiles/drops/common objects/actors).
Native-only exceptions: `BARREL_SPIDERWEB`, `PARALLAX_ROOM_VIEW`,
`MINISH_PORTAL_CLOSEUP` objects and the `SPIDER_WEB` projectile. Unknown kinds
fail closed. Every visible OAM slot must be signed/tagged
(`Port_Oam_VisibleSlotsTagged`) before tall admission.

### Evidence

EU headless 4:3 test-build captures (256 × 192) are in `.tmp/wp3-wp4/`:

- full-height tall: field (`0x03/0x08`), festival town (`0x15/0x00`), dungeon
  (`0x49/0x00`), Minish path (`0x11/0x00`), Mount Crenel (`0x06/0x00`), Cloud
  Tops (`0x08/0x00`);
- Minish Village `0x01/0x00` is a multi-region view: the manager cannot keep
  two groups resident in the shared eight-span stream, so the policy keeps that
  view native/centered. This is the remaining known normal-room fallback and is
  why the release capability stays off.

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
- **WP3/WP4 implemented.** Manager/entity admission is a per-id/class policy.
  Town and village region groups are proven against the candidate aperture;
  village streaming completes before admission. Minish path/rafter parallax
  normalizes against the rendered aperture. Common entity classes are allowed;
  special affine/portal painters and the spiderweb remain native. EU captures
  cover field, town, dungeon, Minish path, Mount Crenel and Cloud Tops at
  256 × 192. See the capability audit above.
- **WP5 implemented.** `Port_TallScene_EffectAllowsTall()` classifies live
  effects: hardware windows, GBA affine modes and non-constant HDMA keep the
  native aperture; a verified constant-brightness HDMA may continue into the
  tall raster. Transitions/rolling frames remain native. The shake envelope is
  still consumed before camera work. Intermediate 162/192 fixtures cover
  windows, unclassified callbacks and constant brightness.
- **WP6 verified.** The 4:3 fixed-screen cache test now stages a real 256 × 192
  `43` artwork set, removes it to prove black-margin failure, and proves the
  missing 43 image never aliases the 16:9 slot. Native centering at `(8,16)`,
  margin-only composition, palette/brightness/alpha/masked-bank/forced-blank
  and title-cap paths remain covered by the background/title tests. A 4:3
  fixed-screen audit captured the native pause/title composition under
  `screenshots/` (git-ignored per AGENTS.md). That live context renders its
  margins from the procedural palette field, so its absent-art run is visually
  identical; the missing-`43` black-margin and no-alias contract is proven by
  the ROM-free 4:3 cache test.
- **Live-gameplay margin fade implemented.** Map-backed live gameplay now
  renders the published canvas width, including wide profiles, and
  `port/port_edge_fade.c/.h` fills the outside-world area once per frame after
  the CPU/GPU raster. The original world rectangle is untouched. A five-pixel
  rounded-Euclidean band samples the nearest edge pixel and darkens it in five
  steps to black; corners curve rather than forming a square. The PPU publishes
  a per-pixel UI mask (BG0 and canvas-tagged OBJ) so HUD/dialogue pixels are
  never faded; if UI covers the world edge, the fade uses the nearest visible
  map pixel rather than the UI colour or black. Margin canvases bind signed host OAM positions, and canvas-tagged
  OAM is culled against the full canvas rather than the room aperture, so
  right/bottom button hints (A/B/R) stay visible. A pre-existing black border
  simply fades black-to-black. Fixed screens and classified native
  transitions/fallbacks keep their previous composition and aperture. The
  post-pass is pure, allocation-free and a no-op when the world already fills
  the canvas. Tests:
  `tools/edge_fade_regression_test.py` covers side/top/bottom bands, corners,
  exact factors, black-border preservation, UI-mask preservation including
  adjacent-map edge-source replacement, full-canvas no-op and invalid inputs; `presentation_geometry_regression_test.py` covers
  the signed-OAM margin decision; the wide/tall engine suites cover the new
  published frame width. EU captures are under
  `screenshots/2026-10-30-eu-edge-fade/`: true 4:3 768 × 576 Minish Path and
  narrow Palace of Winds captures, a 16:9 855 × 480 narrow-room capture, and a
  full-canvas field with no fade.
- **WP7 implemented.** The audited width-569 xBRZ pools already cover the
  full v1 profile set at 4× (4:3 is 49,152 px, the 288 × 162 extent is
  46,656 px, budget is 91,040 px); the 569 × 192 synthetic Cartesian case is
  intentionally over budget and still falls back. `Port_PPU_GpuRasterHeightSupported()`
  documents and locks the native-only GPU raster decision: tall frames use the
  CPU reference and the normal GPU/Renderer presenter. Presentation geometry,
  nearest/internal-scale, xBRZ, CPU filters, colour correction, persistence,
  screenshots, the shared composed-frame accessor and the 288 × 162 extent are
  covered by `presentation_geometry_regression_test.py`,
  `presentation_postprocess_regression_test.py` and
  `screenshot_composition_regression_test.py`. Voxel and custom `.glslp`
  presets remain legacy-160-only and fall back by the existing
  `ScreenGeometrySupported`/`Port_GPU_PresetGeometrySupported` gates. HD
  dialogue geometry is checked at 4:3 and the synthetic 288 × 162 extent. EU
  4:3 software captures at 768 × 576 verify nearest and xBRZ presentation, and
  an SDL_GPU/Vulkan presentation capture of the same frame is byte-identical to
  the software presenter. GPU device/performance acceptance remains WP8.
- **WP2 follow-up.** `Port_Viewport_ResolveWorldRect()` now requires
  `TALL_ROWS_READY` in the tall gate, matching the published validator
  contract and the existing `screen_background_regression_test.py` expectation.
- **Remaining.** Add a residency design for multi-group Minish Village views if
  they must become tall, then flip the release capability and run WP8
  acceptance/performance.

## Sequencing

1. WP1 — composition boundary.
2. WP2 — validator.
3. WP3 — managers/tilesets (can start per family).
4. WP4 — entities/OAM (parallel with WP3 where possible).
5. WP5 — effects/transitions.
6. WP6 — fixed UI/art composition verification.
7. WP9 — live-gameplay margin fade (all map-backed live canvases).
8. WP7 — renderer/presentation.
9. WP8 — acceptance/performance.

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
- Live gameplay has no hard black cutout: a five-pixel edge fade samples the
  original border and darkens to black, preserving black borders and HUD.

## Estimate

This plan has a single broad target, so there is no reduced “core only”
estimate:

- **Full broad 4:3:** roughly **11–17 focused work packages /
  3–4.5 working weeks**.

The schedule is dominated by WP3 (manager/tileset residency) and WP4
(entity/OAM validation), not by the rasterizer or UI layout. Adding a future
288 × 162 profile after this plan should be a profile/config and acceptance
task, not a second live-world implementation.
