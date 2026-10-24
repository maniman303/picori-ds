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
