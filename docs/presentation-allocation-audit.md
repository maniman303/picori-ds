# Presentation allocation and path audit

Audited 2026-10-25 for the five version-1 profiles and the prospective PC
capacity envelope 569 × 192. The audit baseline was width 384; the subsequent
build-synchronization batch switched release helpers to width 569 and verified
unchanged native/widescreen captures. 4:3 and 32:9 remain unavailable. This is a source/storage audit with synthetic
sanitizer coverage, not a device memory or performance measurement.

## Storage policy

Keep `MODE1_GBA_HEIGHT` at 160. `MODE1_MAX_RENDER_HEIGHT` bounds PC raster
scratch at 192, while N64 retains 160. Width capacity does not select a
profile. The largest profile by pixel count is 569 × 160 (91,040 pixels),
larger than 4:3 at 256 × 192 (49,152 pixels). No profile requests 569 × 192.

The existing optional pools in [port_ppu.cpp](../port/port_ppu.cpp) can therefore
retain their `capacityWidth * 160` pixel budget. At width capacity 569 all five
profiles fit at scales 1–10, including xBRZ's two workspaces. Do not replace
160 with 192 in these pool definitions when increasing raster capacity.

| Fixed CPU storage, bytes | Capacity 384 | Prospective capacity 569 |
| --- | ---: | ---: |
| Internal-scale/GPU-supersample shared workspace, 10× maximum | 24,576,000 | 36,416,000 |
| xBRZ 2× intermediate | 983,040 | 1,456,640 |
| xBRZ 4× output | 3,932,160 | 5,826,560 |
| All three optional pools | 29,491,200 (28.125 MiB) | 43,699,200 (41.675 MiB) |
| Three CPU room-map shadows | 6,144 | 8,640 |
| GPU raster shadow staging, GPU builds only | 8,192 | 11,520 |
| Voxel screen-space layer stack, GPU builds only | 737,280 | 1,092,480 |

The optional pool increase for capacity 384 → 569 would be 14,208,000 bytes
(13.550 MiB) of virtual/static storage. Multiplying all pools by 192 instead
would reserve another 8,739,840 bytes (8.335 MiB) at capacity 569 without a
profile needing it. These are source-size calculations: BSS reservation,
touched anonymous pages, RSS/PSS and GPU allocations are different quantities.
The audit step did not increase pool or build capacity; the subsequent
release-capacity update uses the audited width-569 column above.

The PC base framebuffer and LCD accumulator are each 1,843,200 bytes
(`1280 * 360 * 4`), and the PPU scratch VRAM is 4 MiB. All remain unchanged.
The normalized maximum pitch/height of 1280 × 192 fits those frame arrays.
The two native-row IO snapshot/staging arrays each remain 163,840 bytes;
software affine scratch already uses the bounded render height. Per-thread
scanline arrays grow with compile-time width, not with width × height.

## Path decisions

| Path | Geometry/storage contract | Decision before enabling new profiles |
| --- | --- | --- |
| Software PPU | Bounded height, visible width and independent pitch; affine scratch through row 191 | Synthetic 569 × 192 extent is checked under ASan/UBSan. World residency, effects and layout remain separate gates. |
| Nearest/linear internal scale | One reusable fixed pool; packed scaled output from pitched input | Reject invalid/alignment/overflow cases before copying; reject insufficient pixel budget and present raw pixels. Scale 1 uses the original frame. |
| xBRZ | Both 2× and 4× pools must fit before either pass | Compare dimensions by division before multiplying. Failed selection clears both extents. SDL_Renderer presents raw pixels; SDL_GPU can try the selected internal scale, then raw pixels. |
| CPU filters | In-place operations on tight scaled pixels; no extra image or per-frame allocation | All filters are exercised on fitting wide/tall buffers. Raw modes request at least 4× for a selected filter; a failed workspace falls back to raw presentation. |
| SDL_Renderer | Textures sized to the actual presentation frame; source upload respects pitch | Existing texture recreation happens on extent/scale changes. The surface fallback wraps existing base storage and crops to normalized render height. |
| SDL_GPU stock presentation/filters | Source RGBA texture, upload buffer and intermediate texture follow actual scaled dimensions | Rows are packed from the source pitch. Shared viewport fitting now consumes the published effective aspect, including 4:3, parity and unsupported preferences. GPU execution remains unverified locally. |
| GPU raster, Vulkan/GLES | Native-row staging and packed OAM/window shader interpretation | Existing guards reject non-160 height or bound signed OBJ metadata before frame preparation. Software raster can still feed SDL_GPU presentation. |
| Voxel view | Three 160-row layers, 160-line projection and packed OBJ coordinates | New gate rejects a taller or >512-wide canvas, a non-native raster height, or bound signed OBJ metadata. It runs before voxel initialization/depth allocation/uploads and remains off during a temporary native aperture inside an expanded canvas. Use normal 2D presentation. |
| Custom `.glslp` presets | Preset-dependent intermediates plus a history ring | Source-scaled intermediates currently use initial source geometry and do not follow source-size changes. New >512-wide/taller canvases bypass the preset and use stock GPU presentation/filtering. Existing preset allocations may remain resident until unload; they are optional workspace, not a new artwork cache. |
| HD dialogue | Bounded 208 × 32 source canvas and existing ImGui draw data | Retain raster text for non-160 height or frame width above 512 until shared layout exists. A failed canvas/coverage match also retains original text. No additional full-screen buffer. |
| Captures/overlays | Base PNG rows follow normalized height/pitch; row scratch follows compiled width; PPU2 stores explicit geometry | Storage fits 569 × 192; presentation PNG uses the actual window surface. Captures remain explicit actions with allocation/I/O. Existing native capture compatibility is checked separately. |
| Shared-memory experiment | Three native 240 × 160 planes plus native composite | Preserve this native-only producer contract; it temporarily sets native raster geometry and restores the live geometry afterward. Do not widen its arrays or infer full-canvas support. |

At the largest planned profile and scale 10, one RGBA source/upload image is
36,416,000 bytes. A GPU intermediate has the same pixel extent; its bytes per
pixel depend on the chosen intermediate format. GPU/driver history, swapchain,
depth, shader-preset targets and ImGui buffers are additional optional costs.
CPU pool sizes are not a bound on total process or driver memory. Textures
can be recreated when scenes or settings change; steady frames reuse them.

## Scene-composition boundary and menu-family extension (2026-10-25)

The [menu-family revision](menu-background-family-revision.md) extends the
allocation audit for title-frame completion, descriptor publication, and
new-family caching. The existing 569 × 192 capacity totals, upscale pool formulas,
and path gates remain completely valid: 32 artwork names must not become 32
resident images in memory.

### Descriptor and title-cap storage bounds

1. **Family/heading descriptor**:
   Published at scene and native screen installation boundaries. Struct
   `PortMenuBackgroundDescriptor` in `port_screen_background.h` is a 24-byte
   POD record (family, installed health, heading style, heading X/Y/transition,
   and generation). Double-buffered publication requires zero heap allocation
   and fits within 48 bytes of static storage.

2. **Title-cap storage in added margins**:
   The top-left title frame completion covers added margins strictly outside
   the native 240 × 160 content rectangle:
   - In 4:3 (256 × 192, native content at offset `(8, 16)`): top margin is 16 rows,
     left margin is 8 columns. The native heading OBJ piece is at most 64 × 32 at
     stable pose. The margin cap intersection is bounded by top margin
     `[0, 71] × [0, 15]` (72 × 16 = 1,152 pixels) and left margin
     `[0, 7] × [16, 47]` (8 × 32 = 256 pixels). Total maximum cap area in added
     margins is 1,408 pixels (5,632 bytes RGBA).
   - In wide profiles (16:9, 21:9, 32:9): vertical margins are 0; left side
     margin is `nativeContent.x` pixels wide (22, 67, or 164 pixels) by at most
     32 rows (704 to 5,248 pixels RGBA).
   - The completion cap is rendered directly into the existing primary canvas
     (`capacityWidth * 192`), or using bounded static scratch of at most 16 KiB.
     No second full-frame 569 × 192 buffer (436,992 bytes raw, or up to 43.7 MiB
     at scale 10) is allocated. Zero per-frame heap allocation.

### Menu-family cache budgeting and new-family switches

The catalog defines 32 artwork names across 5 menu families (plain, titled,
pieces, techniques, fusion) each with 3 health variants, plus prologue, credits,
title and file select:

| Storage mode | Active resident images | Unscaled RGBA bytes (569 × 160) | Status |
| --- | ---: | ---: | --- |
| Naive resident cache (all 32 names) | 32 | 11,653,120 (~11.11 MiB) | **Rejected** (exceeds 8 MiB steady budget) |
| Steady-state cache | 1 | 364,160 (~355.6 KiB) | Contractual target |
| Active cross-fade transition cache | 2 | 728,320 (~711.25 KiB) | Contractual target (peak during transition) |

1. **Resident buffer limit**:
   The cache is strictly bounded to **2 resident image slots** (current active
   image + incoming transition image). At the widest profile (32:9, 569 × 160),
   steady-state unscaled storage is 364,160 bytes (~0.35 MiB) and peak transition
   storage is 728,320 bytes (~0.69 MiB).
2. **Budget compliance**:
   Both steady-state (< 1 MiB) and peak during decode with libpng scratch
   (< 2.8 MiB) are strictly below the contractual limits: **8 MiB steady /
   16 MiB peak**. The remaining 30 inactive artwork images remain unallocated on
   disk/filesystem.
3. **Switch eviction**:
   Family transitions (e.g. titled → pieces → techniques → plain → fusion)
   load the incoming image into slot 1; upon transition completion, slot 1 is
   promoted to current and the outgoing slot is evicted/released. Same-family
   same-health navigation (inventory ↔ quest ↔ map) matches the existing key
   and performs 0 decodes and 0 buffer reallocations.

The current PPU frame is still the world/native aperture, rather than a finished
canvas compositor. A room cap must not become the PNG size or HUD safe rectangle.
The geometry snapshot preserves those separate dimensions and floor-centered
offsets. The now-shared SDL_Renderer/SDL_GPU viewport calculation fits the actual
frame into the selected stage, including fixed native frames inside a future
4:3 stage; this does not translate BG/OAM/text into a composed logical canvas.

The legacy ImGui menu background owns two fixed `256 * 160` RGBA arrays
(327,680 bytes together), and draws repeated source spans into the side margins.
It is not a PNG cache or a top/bottom margin compositor. Its size is independent
of the build width. It still depends on ImGui and is suppressed by filters,
persistence and other optional rendering modes. Section 5 of the task list
must replace it as contexts migrate; do not enlarge it for 4:3 or layer a new
background behind it without retiring the duplicate path.

Future composition should fill the existing bounded canvas, copy/translate each
native composition once, and send that result through display transforms and
presentation. Solid fields need no full-size image allocation. Keep only current
and incoming decoded artwork, enforce 8 MiB steady / 16 MiB peak including decode
scratch, and publish/release at scene boundaries. No loader/cache/compositor is
implemented by this audit, so those allocations are not included in the table.

## Verification and remaining gates

`ASAN_OPTIONS=detect_leaks=0 python3 tools/presentation_geometry_regression_test.py`
compiles the production C++ workspace definitions/helper bodies and C upscaler/
filters at capacities 240, 384 and 569. It checks independent nearest-pixel
expectations at scales 2/4/10, pitched xBRZ input, all CPU filters, trailing
canaries, exact/excess budgets, malformed pitches, arithmetic extremes, optional
path guards, all five aspect fitting rules and published versus pending aspect.
The 569 × 192 synthetic Cartesian case deliberately exceeds the xBRZ/10× pixel
budget and verifies fallback instead of a larger allocation. The extended
suite additionally checks 24-byte POD `PortMenuBackgroundDescriptor` publication,
the 1,408-pixel (5.5 KiB) title-cap bounds in added 4:3 margins avoiding duplicate
full-frame buffers, the 2-slot (711 KiB) resident image cache bounding within
8 MiB steady / 16 MiB peak, rejection of naive 32-image resident caching (>11.1 MiB),
and zero-reload retention on same-family pause transitions.

`ASAN_OPTIONS=detect_leaks=0 python3 tools/ppu_geometry_regression_test.py`
also runs the existing PPU/capture contracts at capacity 569 with a full
569 × 192 synthetic extent, retaining the N64 native-only check. Production PPU
sources were not changed. HD text, catalog/profile, engine, scroll and CPU PPU
regressions pass. The configured RG ARM64 release build passes; the local EU
ROM reaches `Port layer initialized. Entering AgbMain...` under QEMU with dummy
video/audio and `--no-audio`.

No device PSS/RSS/driver memory or timing was measured, and no actual GPU or
shader-preset execution was available. Full regional native-golden captures,
scene composition, graphics-manager residency, effect continuation, semantic
HUD/dialogue layout and packaging/capacity synchronization remain their listed
tasks. Passing this storage audit permits that later work; it does not enable
4:3/32:9 or certify their gameplay/rendering behavior.
