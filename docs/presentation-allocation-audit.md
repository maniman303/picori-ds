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

## Scene-composition boundary

The [menu-family revision](menu-background-family-revision.md) requires a
follow-up for title-frame completion and family-descriptor publication. The
existing width/height/pool calculations remain valid: 32 artwork names must not
become 32 resident images. Keep current/incoming byte limits, use bounded cap
spans or native sprite data in the existing canvas, and verify both presenters,
screenshots and optional paths before claiming cap support. That follow-up is
open; no new allocation or renderer behavior is implemented by the revision.

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
budget and verifies fallback instead of a larger allocation.

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
