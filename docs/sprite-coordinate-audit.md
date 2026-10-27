# Port sprite coordinates and vertical visibility

Implementation/audit date: 2026-10-24. This prepares signed OBJ positions and
generic vertical culling for taller/wider frames. **4:3 and 32:9 remain gated**
until scene apertures, graphics residency, raster effects and layout are ready.

## Coordinate and lifetime contract

The packed GBA OAM layout stays unchanged. Its Y byte cannot distinguish a
real piece at Y=160 from native off-screen encoding, and its 9-bit X cannot
represent a piece at X=530. Changing the global Y wrap threshold to 192 would
also reveal sprites the engine deliberately parks at Y=160.

`port/port_oam.{h,c}` instead records signed coordinates beside OAM. It reuses
the existing common piece emitter and voxel-tag DMA boundary. The software PPU
consumes a small C-compatible `VirtuaPPUObjPosition` descriptor; it does not
read entities, game tasks, config or voxel state in its sprite loops.

| Boundary | Behavior |
| --- | --- |
| `FlushSprites` | Clears the **build** position table along with the build count. The published table remains unchanged. |
| `RenderSpritePieces` | Records each emitted piece's signed X/Y, final packed attr0/attr1 stamps and hidden/parked status. Both direct UI and entity sprites use this path. Existing voxel anchors remain intact. |
| OAM DMA in `UpdateDisplayControls` | Publishes positions with the OAM they describe, beside the existing voxel-tag latch. Slots beyond the emitted count lose validity. Workers and repeat presents read this immutable latched table. |
| PPU normal/supersampled OBJ render | A valid entry with matching raw attributes supplies signed coordinates. Mismatches/untagged slots use legacy decode; hardware disabled entries and prohibited shapes still take precedence. Explicit parked/hidden metadata never renders in 2D. |
| Native/Console-Parity presentation | Metadata binding stays off for currently supported dimensions and for Console-Parity. Existing packed decode and golden pixels are preserved. |
| Extended-profile raster fallback | Binding follows the logical canvas requirement, even if the raster aperture temporarily returns to 160. Otherwise a real positive Y could wrap onto the top during fallback. |
| GPU raster | Uses software fallback while signed metadata is bound; its shaders do not consume this contract yet. SDL_GPU presentation can still present the software frame. |
| Reset / `ClearOAM` | Invalidates the tables with OAM reset/clearing. `virtuappu_reset` also drops its optional metadata binding. |
| Quicksave restore | Current quicksaves restore simulation regions, not OAM. Existing latched metadata remains paired with the unchanged OAM until the next build/DMA. No pointer-bearing metadata or new regions are serialized. |

Attribute stamps prevent a later raw OAM edit from reusing unrelated signed
positions. Graphics compaction may update attr2 without invalidating a piece's
unchanged location. Unrecorded entries above the 512-pixel encoding limit use
native X decode when metadata is bound, rather than interpreting a negative
native X as a far-right positive coordinate.

The normal OBJ path and affine supersampling share one resolver. Optional
metadata owns no heap memory: two 128-entry tables occupy 2,560 bytes on the
tested ABI, plus one PPU binding pointer. There is no per-frame file access,
allocation, game-state parsing or saved-structure change. This is a source-size
calculation, not measured device memory or frame-time evidence.

## Visibility audit

Generic `CheckOnScreen`, `CheckRectOnScreen`, draw-only visibility and sprite
piece clipping now take their Y extent from the same effective-world-height
accessor as the camera. The native 63-pixel margins and clipping rules are
retained. Runtime height remains 160; tests inject 192 into production bodies.

This does **not** complete every entity's update/parking audit. Individual
enemies, projectiles, transitions, weather and special BG managers may carry
their own native screen assumptions. Those require focused fixtures before
scene capability selection can expose the extra rows. Signed OBJ coordinates
also do not establish tile/palette residency or full 32:9 support.

## Verification

```sh
ASAN_OPTIONS=detect_leaks=0 python3 tools/oam_position_regression_test.py
```

The harness uses the actual piece emitter, `FlushSprites`, visibility helpers,
metadata module and software OBJ rasterizer. It runs with AddressSanitizer and
UBSan at capacities 384/640, covering:

- Real direct-UI/entity pieces at Y=160/191, bottom/right clipping and negative X.
- A signed piece at X=530 in a 569-pixel fixture, preserving packed low bits.
- Untagged/native decode, stale attribute rejection, count shrink and oversized
  API inputs, hidden/disabled entries, and voxel-parked pieces.
- Build versus DMA publication, reset invalidation and immutable repeat reads.
- Affine, double-size affine and supersampled pixels at extra rows.
- A temporary 160-line raster aperture where raw Y wrapping would otherwise
  expose the top of a sprite that actually starts below the aperture.
- Height-aware generic entity/rectangle culling while native limits remain exact.

The RG build, original EU native parity scenes, and local 16:9/21:9 field captures
preserve existing output. GPU shader execution, complete entity-specific
coverage, other regional ROM fixtures and device timing remain open.

## Entity, effects and OAM consumer gate (2026-10-27)

The admission policy is intentionally conservative. A taller world requires a
room-specific scene approval, no active or previously created manager, no
dynamic entity or auxiliary player entity since room load, and signed metadata
matching every visible OAM entry. A non-player entity created and deleted
between presentation boundaries still blocks the room until reload. This is
an exhaustive default-deny decision for entity kinds, not a claim that all
individual AI or effect behavior has been adapted. No production scene
currently grants the separate validation bit; 4:3 remains disabled.

The source-level screen-relative cases resolve as follows:

| Consumer | Decision |
| --- | --- |
| Guard line-of-sight projectile / guard, Dampé, Cucco | Viewport-edge culling now uses effective world dimensions. Cucco's bottom spawn and side distribution follow height; native positions and thresholds remain identical. |
| Gyorg child, Gyorg boss spawn, Gyorg male and enemy 64 | Bottom entry, departure and draw bounds follow the world height; Gyorg's right entry uses world width. Native values are unchanged. |
| Projectile utility | `IsProjectileOffScreen` checks the room, not a 160-row screen, and is retained. Generic `CheckOnScreen` / rectangle users use the height-aware helpers already tested. |
| Barrel spiderweb | The native 160-row range indexes a ten-row affine table. It must not simply be extended: the table has no entries for rows 160–191. Its entity/manager scene remains barred. |
| Mazaal particles, rainfall, cutscene random placement and parallax-room centering | Native distribution, timing and parallax semantics are retained. Their dynamic entity or manager scenes are barred until separately validated. |
| Big Goron, light door/ray, portal closeup, Temple of Droplets and other window effects | Native window bounds/HDMA stay intact; unclassified windows cap the raster at 160. Dynamic entities/managers also bar taller world admission. |
| Other entity types and special effects | The default entity-pool/creation gate denies taller admission, including unreviewed and transient actors. This is a fallback, not behavioral verification for these entities. |

The direct OAM consumers have these explicit contracts:

- The common emitter records signed coordinates; FlushSprites and OAM DMA
  maintain separate build/latched lifetimes. ClearOAM, reset and disabled slots
  retain native hiding. Tile-only and affine-matrix updates do not invalidate
  location; raw 16/32-bit writes touching attr0/attr1 now invalidate the affected
  latched slot even if the value is byte-identical. Untagged or stale visible
  slots prevent tall-scene approval.
- The software and supersampled renderers use the checked position table. The
  GPU rasterizer and voxel presenter keep their tested software/2D fallbacks
  when signed metadata or unsupported geometry is active. Raw OAM inspectors
  and debugging tools continue to show hardware bytes, not logical positions.
- PPU2 does not serialize the side table. Live perfcap now rejects such a
  capture before touching the output file; use a live framebuffer screenshot.
  Existing native PPU1/PPU2 captures remain compatible.
- The optional shared-framebuffer publisher sets OAM count to zero when signed
  metadata is bound. Its in-tree Vulkan-RT consumer then skips raw sprite quads
  and uses the composite frame. Older/out-of-tree readers that ignore the
  count are not certified for extended coordinates.

Production-body entity tests exercise native/tall boundaries and Cucco spawns;
separate ASan/UBSan checks cover raw writes, signed/parked OBJ, upload staging,
shared-framebuffer fallback and presentation backend gates. The ARM64 build,
EU logo/title PPU parity and EU dummy-SDL startup pass. The remaining 4:3 scene,
entity/effect implementation, transition and compositor acceptance tasks must
not interpret the default-deny audit as visual gameplay acceptance.
