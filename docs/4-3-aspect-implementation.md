# Plan: finish proper 4:3 (256 × 192)

Status: WP1–WP9 implemented and accepted. The Minish Village multi-group
residency is solved with host charset groups plus an atomic group switch, and
the release profile is enabled. WP8 now includes the physical RG 4:3
frame-time/thermal matrix and the remaining room-family captures (town,
dungeon, boss, Crenel rain path and interior). The tall message/HUD ownership
rule is final
and verified. Missing 4:3 artwork remains the supported final state.
Scope owner: code/geometry/streaming/validation/composition/tests  
Artwork: **optional**. Missing 4:3 artwork is intentional and must render solid
black; no work package depends on custom files being supplied.

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
- **EU is the primary fixture.** Build, capture and acceptance work use the EU
  ROM: `xmake f -y --game_version=EU ...`. EU/English is the region recorded in
  the main acceptance document.
- **USA and JP retail ROMs are now available and validated for this feature.**
  Both pass the same 4:3 presenter matrix, the village multi-group atlas matches
  a native single-group render pixel-for-pixel, and the sub-tile wrap captures
  have no shadow gaps. JP field dialogue renders Japanese text in the tested
  scene; broader JP text/menu layout audits remain separate. Never substitute
  one region's data for another.
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
  family or health state. Missing files are a supported final state, not an
  acceptance blocker; supplying artwork is optional user work.

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
- Full Minish Village multi-group residency: every group's charset/palette is
  available in host memory, other-group cells are selected per tilemap cell,
  and the selected group loads atomically with an immediate palette latch.
- Release capability on by default (`PORT_VIEWPORT_RELEASE_TALL=1`) with a
  `--tall_aspects=n` escape hatch; 4:3 is selectable in release builds.
- ROM-free tests for staging, geometry, camera, OAM, HUD/dialogue, presentation,
  context/effect composition, memory budgets, the margin fade, the village
  group grid and per-cell PPU charset/palette selection.
- Height-aware 4:3 live-gameplay presenter matrix (native/16:9/21:9/4:3,
  software; two 4:3 profile switches) and a 192-row snapshot guard in the RG
  performance probe.
- Physical RG 4:3: true 256 × 192 device captures (field at 1/2/4 workers,
  boss and tall town), a decoupled field-room frame-time matrix, a boss-scene
  check, a five-minute town thermal/PSS run, and a 4:3 60 fps target with
  measured headroom (see
  [the RG performance record](rg-performance-and-memory-2026-10-30.md)).
- Remaining room families: festival town (`0x15/0x00`, after its entry scene),
  normal dungeon (`0x48/0x00`), boss (`0x49/0x00`), Crenel Minish rain path
  (`0x12/0x02`) and interior (`0x22/0x12`) host captures at exact 4× (1024 ×
  768) in `screenshots/2026-10-31-eu-{town,dungeon,boss,crenel-rain,interior}-43-4x.png`.
  The festival entry camera pan stays full-height end to end (see
  `screenshots/2026-10-31-eu-festival-zelda-{castle,pan,town}-43-4x.png`), so
  it no longer toggles between native and tall around the scripted camera
  target. The interior is a 240 × 160 room centered in the 256 × 192 canvas;
  the margin fade extends its edge colors into the added rows/columns.

Still intentionally limited (not release blockers):

- The release capability is on (`PORT_VIEWPORT_RELEASE_TALL=1` by default);
  `--tall_aspects=n` is available for native-only bisects. Every room still
  passes the per-frame validator before the tall aperture is published.
- Tall world admission fails closed for explicitly denied manager families,
  unclassified shake/transitions, untagged OAM and failed uploads. Normal
  manager-backed rooms on the allow-list render tall when their residency
  proof passes.
- Minish Village views that intersect two region groups are fully supported:
  the selected group stays in VRAM, every other group is selected per tilemap
  cell from a host charset atlas, and the group switch is atomic so there is
  no eight-step aperture pop. Normal Hyrule Town keeps its single-group
  residency rule. Festival Town is now exempt from that proof: its two
  foreground region groups overlap the camera rectangles during the intro
  pan, but the loaded group covers the visible map cells. Forced-tall
  full-scene captures of the castle/town pan and the Zelda stage are clean,
  so the aperture stays tall for the whole festival scene instead of
  toggling native/tall at the camera target changes.
- Hardware windows, GBA affine modes and non-constant HDMA keep the native
  aperture; only a verified constant-brightness HDMA may extend into the tall
  canvas. Rolling/special transitions and the explicitly denied effect
  managers remain native.
- GPU raster supports only 160 rows; tall canvases use the CPU reference and
  reach the swapchain through the normal GPU presenter. Wide 160-row canvases
  keep the GPU path.
- Missing 4:3 artwork is an intentional black fallback. Custom artwork is
  optional and never a merge or release condition.
- The margin fade is limited to map-backed live gameplay. Fixed screens,
  classified native transitions/fallbacks and non-map overlays keep their
  existing composition. The fade replaces the old coloured/procedural margin
  fill with black beyond the five-pixel band.
- One village reveal-shadow edge (world `x=816..819, y=656..711` at a
  sub-tile offset) can show a 4-px black strip in the shadow column while the
  live/native view at the same world cell has content. The retaken sub-tile
  capture uses a nearby camera Y that avoids the affected cell; a shadow
  map/layer fallback for that edge is a remaining polish item.
- Tall message/HUD ownership is final. Standard top messages stay at the
  native top and standard bottom messages move to the canvas bottom inset,
  both horizontally centered. The rule uses the published height
  (`bottom shift = height - 160`), not a profile id. The lower-right HUD moves
  with the canvas and the rupee counter stays visible below a 4:3 bottom box.
  The key row shares BG0 cells with a bottom box and is overwritten by it
  exactly as on GBA; it is not recoverable after the box writes, and moving
  the box up only re-samples box pixels as HUD. Use the top position for the
  full HUD. Evidence:
  `screenshots/2026-10-31-eu-boss-43-dialogue-top-4x.png` (full key/rupee HUD
  with a 4:3 top message). Comparison captures:
  `screenshots/2026-10-31-eu-field-169-dialogue-{bottom,top}-4x.png` versus
  `screenshots/2026-10-31-eu-field-43-dialogue-{bottom,top}-4x.png`.

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

The release profile exposes 192-row live gameplay in normal towns, dungeons
and fields, including multi-group Minish Village views. A native-aperture
fallback is acceptable only for narrowly classified transitions/effects, not
as the final behavior for normal rooms. Fixed UI keeps its native composition;
missing artwork renders black and is a supported final state.

## Work packages

### WP1 — Release enablement and geometry-general composition

1. Replace the `PORT_VIEWPORT_TEST_TALL`-only availability path with a release
   capability. Availability is by published profile/capacity, not by a
   hardcoded 4:3 check; `--tall_aspects=n` remains a build-level escape hatch.
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
   - Minish Village eight-step group streaming and palette changes
     (solved with a host charset atlas and an atomic group switch, so two
     simultaneously visible groups no longer force a native fallback);
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
   a failure of this plan, not a pass. The Minish Village multi-group case is
   covered by the host charset atlas.

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
5. Add 4:3 fixed-screen captures with missing artwork (black margins). Custom
   artwork is optional; no code change or capture of supplied art is required.

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
   - fixed screens with missing-art black (custom artwork optional);
   - intermediate fades/effects;
   - profile switches while message/fade/menu is active.
2. Software/GPU base-framebuffer parity; document any presenter block-edge
   sampling.
3. Physical RG: 4:3 60 fps, frame-time distribution, PSS/RSS, 1/2/4 workers,
   thermal saturation. 4:3 adds ~28% pixels over 240 × 160.
4. EU is the primary fixture; run host tests and EU captures with
   `--game_version=EU`. USA and JP retail ROMs are available and pass the same
   4:3 presenter matrix and village atlas comparison; retain their evidence
   separately and never substitute one region's data for another.
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

Custom artwork is **optional**. Missing files are intentional and are the
supported final state for any context/family/health combination.

Optional user work:

- creating `assets/override/43` artwork for any context/family/health state;
- contact sheets and artist-facing exports;
- deciding which fixed screens or environments receive custom artwork.

The code guarantees the `43` fixed-screen contract, the context/effect-aware
margin composition, black fallback when a file is absent, no cross-profile/
family aliasing, and the live-gameplay margin fade. No test, build, merge or
release gate requires the artwork to exist.

## WP3/WP4 capability audit

Manager and entity admission is now an explicit, frame-evaluated policy in
`port/port_tall_scene.c`; `port/port_linked_stubs.c` scans the live engine
pools. A normal aperture always admits.

### Minish Village multi-group residency

The village streams one 32 KB tileset group (eight 4 KB chunks across two BG
char bases) through a single shared VRAM span. A 192-row aperture can intersect
two region groups at once; they cannot both live in VRAM, and both palettes
target the same 13 BG palette banks. The port therefore treats the host as the
residency authority:

- `port/port_village_groups.c/.h` keeps every group's complete charset
  (5 × 48 KB BSS) and captured group palette in host memory. The manager
  publishes its active region/chunk/palette tables; the port copies the chunks
  from the same `gGlobalGfxAndPalettes` source the engine uses.
- The selected group stays resident in VRAM, so native char animations and the
  normal GBA path keep working. Cells that belong to any *other* region group
  are published as a per-tilemap-cell group id; the PPU reads that group's
  charset and a faded palette LUT for the cell (`virtuappu_mode1_bind_bg_group_cells`).
  The LUT uses the same `MakeFadeBuff256` transform/brightness as VBlank, so
  fades stay consistent. Cells in the selected group or outside all regions
  keep entry `0xFF` (native VRAM + palette).
- The reveal shadow has its own per-cell group grid using the shadow's
  world-row/column mapping, so those columns select the correct group too. On
  tall canvases the shadow starts only at the 256-px sub-tile wrap
  (`256 - BGHOFS`), not at 240, so valid live VRAM columns are never replaced
  by redundant shadow columns; wider canvases use the same path for their
  extra columns.
- Under the tall profile the manager loads all eight chunks and the palette in
  one update (`Port_VillageGroups_AtomicStream`) and applies the palette to
  `gBgPltt` immediately (`Port_VillageGroups_ApplyLoadedPalette`). This removes
  both the staged mixed-charset frame and the one-frame palette lag, so region
  crossings have no aperture pop or colour flash. Native/wide builds keep the
  original eight-step GBA pacing.
- Tests: `tools/village_groups_regression_test.py` (region lookup, padding row,
  selected-group VRAM choice, scroll offsets),
  `tools/ppu_group_cells_regression_test.py` (per-cell charset/LUT selection,
  fallback), and the village manager test (staged + atomic + multi-group
  selections at 162/192). A clean EU 4:3 capture of the g2/g3 overlap matched
  the native single-group g3 render pixel-for-pixel in the atlas region.

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

Conditionally allowed (must prove residency):

- `HYRULE_TOWN_TILESET`: each region list must select at most one group.
- `MINISH_VILLAGE`: every group charset/palette must be resident in the host
  atlas and the selected group must be valid. The selected group loads
  atomically (one update) under the tall profile; the PPU selects any other
  group per tilemap cell. Multi-region intersections no longer fail closed.

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

EU headless 4:3 captures (256 × 192) are in `.tmp/wp3-wp4/` (historical) and
`screenshots/` (current):

- full-height tall: field (`0x03/0x08`), festival town (`0x15/0x00`), normal
  dungeon (`0x48/0x00`), boss (`0x49/0x00`), Minish path (`0x11/0x00`),
  Crenel rain path (`0x12/0x02`), Mount Crenel (`0x06/0x00`), Cloud Tops
  (`0x08/0x00`) and interior (`0x22/0x12`);
- Minish Village `0x01/0x00` is a multi-region view. The selected group stays
  in VRAM while every other group is read from the host charset atlas, so the
  view renders at 256 × 192 with no native fallback and no group-switch pop.
  The `screenshots/2026-10-30-eu-minish-village-43-multigroup*.png` captures
  show the multi-group view; a clean comparison against a native single-group
  capture matched pixel-for-pixel in the atlas region.

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
  availability is profile/capacity based; the literal 192 in staging/camera
  math is gone.
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
- **Minish Village seamless residency implemented.** `port/port_village_groups.c/.h`
  keeps every tileset group's 32 KB charset and palette in host memory, builds
  a per-tilemap-cell group grid from the manager's region table, and binds it
  to the PPU for the map BG layers. The selected group stays in VRAM (native
  animations preserved); other groups use the host atlas and the same faded
  palette banks. `virtuappu_mode1_bind_bg_group_cells()` selects charset and
  palette per cell; the reveal-shadow columns use their own group grid
  (`virtuappu_mode1_bind_bg_group_shadow`) with the shadow world mapping.
  Under the tall profile the manager
  loads the selected group and palette in one update
  (`Port_VillageGroups_AtomicStream`), so crossing a region boundary has no
  eight-step mixed-charset frame and no aperture pop. ROM-free tests:
  `tools/village_groups_regression_test.py`,
  `tools/ppu_group_cells_regression_test.py`; the manager test also covers the
  atomic path and a multi-group selection at 162/192.
- **Release enabled.** `PORT_VIEWPORT_RELEASE_TALL` defaults to `1`;
  `--tall_aspects=n` disables tall profiles for native-only builds. 4:3 is
  selectable in release builds with ≥256 × 192 capacity.
- **Acceptance tooling generalized for 192 rows.** `verify_eu_gameplay_presenters.py`
  now covers a 4:3 profile (256 × 192 / 1024 × 768), height-aware presentation
  sampling and dialogue bands, 4:3 profile switches, an ImageMagick-backed PNG
  reader (no PIL dependency) and a `--backends` selector. It passed 24 EU
  software captures at native/16:9/21:9/4:3, including two 4:3 live switches.
  `oam_position_regression_test.py` and `door_visibility_regression_test.py`
  links/stubs were repaired for the current PPU/viewport APIs;
  `screen_background.c` now follows the release tall capability.
- **USA/JP retail validation.** BZME and BZMJ ROMs matched their SHA-1s and
  were built with the multi-region and single-region configurations. Both pass
  the 24-capture 4:3 software presenter matrix (`.tmp/eu-gameplay-43-usa-final/`,
  `.tmp/eu-gameplay-43-jp-final/`); the EU-baseline multi-region binary also
  passes both matrices (`.tmp/eu-gameplay-43-{usa,jp}-eubase/`). The EU matrix
  was re-run after the generic message-top/HUD-anchor change
  (`.tmp/eu-gameplay-43-eu-final/`); USA/JP share the same generic code and were
  not re-run for that last change. The village g2/g3 atlas matches the native
  single-group render pixel-for-pixel on both; sub-tile-wrap captures have no
  shadow gap; JP field dialogue renders Japanese text. Evidence screenshots are
  under `screenshots/2026-10-30-{us,jp}-*`. Broader JP menu/text layout coverage
  remains a separate audit.
- **Message vertical policy generalized.** Standard top messages stay at the
  native top on every canvas; standard bottom messages move to the canvas
  bottom inset (`height - 160`), both horizontally centered. The right HUD
  anchor is also retained during messages even when `unk_10`/`unk_a` redraw
  flags are clear, so the rupee counter is anchored at the canvas bottom-right
  instead of being left at the native position under a 4:3 bottom box. 4:3 top
  dialogue appears at the top with the full counter; bottom dialogue keeps the
  counter visible below the box. The rule has no profile-specific case and is
  covered by `screen_background` (160/192/162) and the 4:3 presenter matrix.
  Native 240 output is unchanged; wide/tall message frames deliberately use the
  same canvas-edge anchor instead of leaving the right HUD at its native x.
- **Camera-tile staging.** Tall steady-state now refreshes a moved camera tile
  with a synchronous full fill + VRAM copy before the present instead of
  dropping to the native composition for a frame; partial scroll updates no
  longer clear a valid tall snapshot. This removes the walking jitter seen with
  rooms whose map animation uses modes 2-4 (notably the USA village).
- **Remaining (optional/separate).** Optional custom artwork, broader JP
  menu/text layout audits, and USA/JP re-runs of the final generic message
  change. Missing artwork is always black and is not a blocker.

- **WP8 physical RG 4:3 evidence.** The Anbernic RG DS Plus ran the EU field
  room at 256 × 192 with the release tall capability. Decoupled medians over
  23 steady windows: 1 worker game 0.514 / present 10.741 / render 8.463 ms
  (11.255 ms work, 5.41 ms headroom); 2 workers 0.515 / 7.825 / 5.227 ms
  (8.340 ms, 8.33 ms); 4 workers 0.726 / 6.605 / 3.677 ms (7.331 ms,
  9.34 ms). A 4-worker boss-room check measured 7.258 ms work (9.41 ms
  headroom); a five-minute tall-town run (156 windows) stayed at 6.505 ms work
  (10.16 ms headroom), 41.875 °C / 40.625 °C, and flat 101,480 KiB PSS. Device
  captures are true 256 × 192. See the updated
  [RG performance record](rg-performance-and-memory-2026-10-30.md).
- **Room-family matrix completed.** Festival town `0x15/0x00` (after its entry
  scene), normal dungeon `0x48/0x00`, boss `0x49/0x00`, Crenel Minish rain path
  `0x12/0x02` and interior `0x22/0x12` host captures at 1024 × 768 show full
  256 × 192 live worlds. The Festival Town entry camera pan stays full-height
  for the whole scene; the town's overlapping foreground groups are visually
  compatible and the manager proof is relaxed only for `AREA_FESTIVAL_TOWN`.
  The
  interior's 240 × 160 world is centered with the WP9 edge fade. The final
  24-capture EU software presenter matrix was re-run from
  `.tmp/eu-gameplay-43-final-2026-10-31/`.
- **Tall message/HUD ownership closed.** Top messages keep the full key/rupee
  HUD; a bottom box uses the canvas bottom inset and overwrites the BG0 key
  cells exactly as GBA does, so moving the box above them re-samples box
  pixels rather than restoring the HUD. The rule is documented in
  [the HUD audit](live-hud-layout-source-audit.md) and demonstrated by
  `screenshots/2026-10-31-eu-boss-43-dialogue-top-4x.png`.
- **Harness support for device windows.** `TMC_ROOMCAP_TIMEOUT` raises the
  abort limit and `TMC_ROOMCAP_HOLD=1` keeps the settled room live after a
  capture, so long profile/thermal windows no longer end at the old fixed
  5000-frame exit. `tools/roomcap_regression_test.py` covers the new timeout
  block.
- **Festival Town camera-pan aperture stability.** The Zelda intro pans the
  camera while `HYRULE_TOWN_TILESET` reports overlapping foreground region
  rectangles, which previously toggled Festival Town between native 160 and
  tall 192 at the start/end of the pan. Full-scene forced-tall captures of the
  castle-to-town pan and the Zelda stage show the loaded group covers the
  visible cells, so `HyruleTownRegionSelectionSafe()` now reports safe for
  `AREA_FESTIVAL_TOWN` only. The aperture stays tall for the whole scene, so
  there is no camera/world jump and no camera-scroll re-anchor. Normal Hyrule
  Town keeps the single-group proof. Evidence:
  `screenshots/2026-10-31-eu-festival-zelda-{castle,pan,town}-43-4x.png`.
- **Tall map-window upload ordering (16-px pan jump).** With the aperture
  stable, the pan still stepped one world tile (16 px) down and back at every
  camera tile crossing. The tall window was refilled and copied to VRAM from
  `Port_Widescreen_PublishScene()`'s present hook
  (`Port_Widescreen_BeforePresent`), which runs at the top of
  `VBlankIntrWait()` — *before* `VBlankIntr()`/`DispCtrlSet()` write the new
  `REG_BGxVOFS`. The new rows were therefore rendered with the previous
  vertical offset. The map buffer is now filled in the simulation phase
  (`Port_Widescreen_SyncMapWindow()`, called from `UpdateScroll()` right after
  `Scroll1`) and never copied to VRAM by the port: it only queues
  `gUpdateVisibleTiles`, so `WaitForNextFrame()`'s map DMA moves it after
  `DispCtrlSet()` in the same frame as the offsets. `PublishScene()` keeps the
  tall aperture while that DMA is pending (`Port_WidescreenMapWindowPending`).
  Verified with a 3000-frame 1-frame-step capture of the pan: every frame delta
  now equals the 1–2 px/frame scripted scroll, with no 16-px pairs. Evidence:
  `screenshots/2026-10-31-eu-festival-pan-{f180,f183-tile-crossing,f186}-4x3.png`
  and the crossing strip `2026-10-31-eu-festival-pan-tile-crossing-strip-4x3.png`.
- **Tall aperture across room load / area change.** `GAMEMAIN_CHANGEAREA`
  (and `GAMEMAIN_INITROOM`) forced the native 160-row composition even when
  the tall map window for the current room was already resident, so the
  Festival Town intro entered with 16-px letterbox bars and popped to 192
  rows one tile later. Residency (`sWsValidatedKey` plus a matching or
  pending map snapshot) now keeps the tall aperture through the load/show
  transition. Non-resident rooms still start native until their upload lands,
  and the black-fade arrival rule for `GAMEMAIN_CHANGEROOM` is unchanged.
  Device trace before: 160 rows from room frame 19 through 37; after: 192
  from frame 18 (only the single different-area flash frame stays native).
  Evidence:
  `screenshots/2026-10-31-eu-festival-load-160-vs-tall-4x3.png`.
- **Tall map fills always use the full mode-1 pass.** Native partial-scroll
  writers (`gUpdateVisibleTiles` 2–4) shift fixed 23-row windows and do not
  know about a taller aperture's extra rows. While the published view is
  taller than 160, `UpdateScrollVram()` promotes those updates to the full
  fill for the current camera tile, which is authoritative and only copies
  ~2 KB per layer.
- **Bottom void band keeps a pinned art set.** The extra bottom rows are below
  the GBA viewport and the room's bottom 32 px are the void border, whose map
  cells alias tiles that Hyrule Town uses for regional groups and BG
  animations. The first frames of the band therefore rendered the live
  group/frame instead of the border art, and only settled after the region
  swap. The port now captures the base group's static bank (group 4 at
  `0x2000`/`0xA000`, from the manager's own group load) and the creamiest
  BG-animation frame for `0x2800`, then pins those chars on the band's
  margin-only cells before each render. Visible cells always keep the live
  art, so streaming and animation of the gameplay area are untouched. The
  town manager also re-applies the current BG animation frame after each
  group load (`ReapplyBgAnimationsInRange`) and the room-load path re-applies
  every active frame, so the band looks correct from its first frame.
  Evidence: `screenshots/2026-11-01-rgdsplus-festival-pan-swap-border-4x3.png`
  and `screenshots/2026-11-01-rgdsplus-festival-pan-end-border-4x3.png`.
- **Scene starts tall from the first uploaded frame.** `GAMEMAIN_CHANGEROOM`
  is included in the residency-based tall admission, so the room's first
  visible frames after the map upload are 192 rows; only the frames before the
  upload (and a single different-area flash frame) stay native. Evidence:
  `screenshots/2026-11-01-rgdsplus-festival-load-tall-4x3.png`.

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

- 4:3 selectable in release builds with capacity ≥256 × 192. (Done.)
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
- RG holds 4:3 60 fps with documented headroom (1/2/4 workers, field and a
  five-minute town thermal run).
- Missing 4:3 artwork is black only. Missing files are intentional and
  supported; artwork creation is optional user work, never a release gate.
- Live gameplay has no hard black cutout: a five-pixel edge fade samples the
  original border and darkens to black, preserving black borders and HUD.

## Estimate

This plan has a single broad target, so there is no reduced “core only”
estimate:

- **Full broad 4:3:** roughly **11–17 focused work packages /
  3–4.5 working weeks**.

The schedule was dominated by WP3 (manager/tileset residency) and WP4
(entity/OAM validation), not by the rasterizer or UI layout; both are now
implemented, including the Minish Village host charset atlas, and the WP8
device/room-family evidence is recorded. Remaining work is optional custom
artwork and broader regional audits. Adding a future 288 × 162 profile after
this plan should be a profile/config and acceptance task, not a second
live-world implementation.
