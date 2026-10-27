# Vertical world streaming audit

Source audit: 2026-10-24. This separates verified tile addressing from graphics
residency and scene support. **4:3 remains disabled**; the runtime world-height
accessor returns 160. Tests inject 192 into the production camera/copy/render
paths without advertising unsupported gameplay.

## Camera contract

`Port_Viewport_CameraRest` implements the same bounded formula on either axis:

```text
clamp(target - viewExtent/2, origin, max(origin, origin + roomExtent - viewExtent))
```

Arithmetic widens before clamping. Small rooms pin to their origin. The port
X/Y wrappers supply room coordinates and effective world extents. Snap cameras,
`InitializeCamera`, and `WaitForCameraTouchRoomBorder` use the same goals,
including the room-relative-to-absolute conversion during initialization.

`Scroll1` retains native vertical movement/flags and tile-refresh pacing at
height 160. The taller branch uses the shared rest target and its matching
bottom limit, so a camera parked beyond a newly reduced boundary can pan back
instead of leaving script completion waiting forever. This step does not
change rolling/iris durations, Q-format movement, entity layouts or GBA timing.

## Normal tile window

Evidence: `src/game.c` updates scroll, then calls `UpdateScrollVram`;
`port/port_linked_stubs.c` dispatches copy modes; `src/interrupts.c` uploads
the BG buffers after display/resource updates.

| Part | Verified source behavior | Implementation / remaining work |
| --- | --- | --- |
| BG source | `LoadRoomGfx` in `src/playerUtils.c` clears two 128 × 128 subtile maps and loads the room map. These maps contain tile entries, not proof of resident character graphics. | Preserve the maps and existing VRAM layout. |
| Normal copy mode 1 | `ram_sub_080B197C_c` previously filled 32 × 23 tile entries. It duplicates the first padding row at the top for relative Y < 8. | Row count now comes from the effective world height. Index checks happen before pointer creation, preserving the native copy footprint and skipped rows. Invalid newly exposed rows are cleared. |
| Vertical register offset | `UpdateScreenShake` starts with `(relativeScrollY & 15) + 8`. Without shake, the maximum visible source row is `floor((height - 1 + 23)/8)`. | `Port_Viewport_MapTileRows` supplies 23 rows at 160 lines, 27 at 192. Larger requests that cannot fit 32 rows return zero. |
| VRAM upload | `WaitForNextFrame` previously copied `0x5c0` bytes from each enabled map BG buffer. | PC copies now use the same effective-height byte count as the fill: `0x5c0` at 160, `0x6c0` at 192. Non-PC behavior retains `0x5c0`. |
| Right-hand shadow | `Port_WidescreenShadow_Populate` already fills all 32 rows, using the native VRAM row origin and independent world-column indexing. | Production-path tests verify bottom/right pixels at rows 159/160/191 and columns 239/240/255 through all 16 × 16 fractional scroll phases. No new shadow allocation or whole-room copy is needed for this normal path. |
| Screen shake | Actual offsets add `gShakeOffsets` to the base offset. The native 23-row footprint is retained. | The coverage classifier now accounts for a signed vertical offset across all fractional phases. At height 192, offsets -8…41 fit the current origin; +3/+8 require 28 rows. Unsupported wrap (-9/+42 and beyond) requires native aperture selection or remapping before enablement. |

`Port_Viewport_MapTileRowsWithShake` returns zero for unsupported coverage. The
taller backing-copy wrapper pads supported positive shake and bounds an invalid
case to a full 32-row copy, never a zero-length DMA. That bound protects memory;
it does **not** approve the invalid camera/effect mapping. Scene-aperture
publication must reject unsupported coverage before taller rendering is enabled.
The PC scene adapter now publishes `PORT_SCENE_TALL_SHAKE_UNSAFE` when any
of the active magnitude’s eight XY pairs fails the 192-row envelope; checking
the full sequence before camera work prevents a later phase from invalidating
a previously accepted aperture. This flag is informational while world height
is fixed at 160; a future tall-scene policy must consume it before enablement.
Native-height fill/upload still uses 23 rows even during shake.

The normal copy uses the original linear map-span bounds. It does not establish
room-aperture clipping or convert every special BG into a room tilemap. Those
remain separate scene capabilities. A 27-row window fitting in a screenblock
does not establish character or palette residency.

## Graphics and special-screen gates

| Source | Evidence | Gate before taller reveal |
| --- | --- | --- |
| `CheckRegionOnScreen` / `CheckRegionsOnScreen` in `src/main.c` | Region intersection already follows effective X extent; Y now uses the height accessor. The region-list dispatcher chooses the first matching entry. | The runtime accessor remains 160. Expanded-region intersection is tested, but simultaneous region/bank compatibility is not established. |
| `src/manager/hyruleTownTileSetManager.c` | Normal/festival region tables select graphics groups; `HyruleTownTileSetManager_LoadGfxGroup` asynchronously replaces two graphics spans at data-selected destinations. | Validate town/festival boundaries and simultaneous visible regions, bank conflicts, and upload timing for each available region. Do not simply load every intersecting group into shared destinations. |
| `src/manager/minishVillageTileSetManager.c` | EU/USA region tables select one group. Updates stream eight 0x1000-byte chunks over a timer and change palettes; entry loads can copy the complete group. | Verify group boundaries and every intermediate loading state. Confirm that added bottom rows never expose graphics from an outgoing group. |
| `src/screenTileMap.c`, copy modes 2/3/4 | Rolling and special transitions have fixed native row/column loops and buffer offsets. | Retain native transition apertures; adapt each transition only with completion, tile residency and effect tests. The normal-fill change does not generalize these modes. |
| `horizontalMinishPathBackgroundManager.c` / `verticalMinishPathBackgroundManager.c` | They use slices of `gMapDataTopSpecial`, independent layer scroll rates, and bounded PC pointer remapping. | Treat them as special BG layouts, not normal room maps. Validate slice coverage and scroll ratios at all extra rows. |
| `animatedBackgroundManager.c` / `rollingBarrelManager.c` | These install/reload special graphics or directly own BG/raster behavior. | Preserve their live effects; require explicit raster/streaming policy. Static contextual artwork must never fill these gameplay regions. |

Remaining enablement also needs entity/OBJ culling and parked-Y semantics,
scene/world-aperture publication, fixed-screen composition, and the requested
HUD/dialogue layout. Camera readiness alone does not enable 4:3.

## Verification

`ASAN_OPTIONS=detect_leaks=0 python3 tools/world_view_regression_test.py` compiles
production camera, initialization, script-wait, region and map-copy bodies with
the real viewport and software PPU modules at capacities 240 and 384. Only
external engine operations and aperture selection are controlled by the fixture.
Address/undefined-behavior checks remain active; leak scanning is disabled for
the restricted process environment.

Checks include native/tall goals, odd widths, nonzero origins, extreme integer
inputs, capped step sizes, script completion, small-room clamps, native edge-step
flags, recovery from a prior aperture's boundary, map-buffer canaries, deep and
negative scrolls, and actual two-axis rendered pixels with offsets -8/-3/0/+3/+8
through all fractional scroll phases. Synthetic graphics are
fully resident; these tests are not regional tileset validation.

The RG build, EU native parity gate and local wide field captures preserve
existing output. Device measurements and additional regional/manager/effect
fixtures remain open. No golden hashes or ROM-derived fixtures are changed.

## Aperture publication and manager fallback (2026-10-27)

The current runtime still selects native height. A future tall scene must
explicitly pass the separate scene/entity/presentation gate, have a normal live
map and no manager activity in the room, and have safe current and whole-sequence
shake offsets. Every manager is conservatively unverified; creation marks the
room unsafe even if that manager is removed before the next presentation. The
room stays unsafe until a fresh room load. Special backgrounds, streaming
managers and transitions therefore cannot silently expose unverified rows.

For an approved prospective tall scene, map fill runs at the target height
while the published aperture remains native. The fill and subsequent real VRAM
upload must match room identity, camera tile origin and row count before the
aperture expands. Stale fills restart staging. The PPU raster uses the world
aperture, so a rejected room does not render extra rows. Shake setup computes
the maximum footprint across all eight phases, refreshes backing rows when
needed, and immediately republishes an unsafe fallback.

Production-body tests inject a test-only tall profile and verify publication,
fill/upload ordering, stale origins, pending resource queues, transient managers,
unsafe shake and room changes. Manager tests execute the real village and town
loaders using the PC immediate-copy implementation: all five EU village groups
stream over eight updates; town/festival groups each replace two 0x1000-byte
spans before return. Native versus taller region intersections can select
different village groups, so a completed timer does not prove simultaneous
residency. USA/JP manager control flow is tested with synthetic tables only.
All managers remain gated; these tests do not approve taller manager scenes.

PC village loading now handles a viewport with no matching region by retaining
the current valid group, or waiting if none has been installed, instead of
using an uninitialized/sentinel index. No non-PC manager code was changed.
The production profile remains unavailable until the separate sprite, scene,
effect and composition tasks are complete.
