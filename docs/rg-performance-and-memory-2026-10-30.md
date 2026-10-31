# RG performance and memory probe (2026-10-30)

This records both the host proxy and an actual Anbernic RG DS Plus run. The
device was reached over SSH and ran the same EU room profile through the ARM64
release-like build. QEMU and WSL2 numbers are still labelled as host proxies;
they are not substitutes for the physical run.

## Acceptance target

- Deliver the native GBA cadence on the RG: a stable 60 fps (59.7275 Hz) with no
  audio/video pacing drops during normal gameplay.
- Leave reasonable headroom for a future **second screen** used only for map,
  inventory and similar non-gameplay views. Gameplay itself stays on one screen;
  a dual-screen gameplay map is not a target.
- Do not chase maximum frame rate. Targeted optimization is deferred until the
  dual-screen work starts; the current goal is correctness plus native cadence.

## Method

- Host: WSL2 Linux, x86_64, 16 logical CPUs, GCC, no physical handheld.
- Snapshot: EU room `area=0x03 room=0x08`, scroll `240,232`, captured after a
  300-frame settle with the repro harness. The dump is the ROM-derived PPU2
  snapshot in `.tmp/rg-performance-2026-10-30/room.ppu`, SHA256
  `66d079a18102c3ceba6aadeca12d605185d5ba4d19a866bba6171582bda734d8`.
- Benchmark: production `port/ppu/src/mode1.c` through `tools/ppu_bench.c`,
  1,000 frames per repetition, three repetitions per worker count, median
  reported. `tools/rg_performance_probe.py` rebuilds the benchmark, sets
  `OMP_NUM_THREADS`, and requires one identical framebuffer checksum.
- Two builds were measured: native capacity 240 with AVX2, and the release-like
  capacity 569 without AVX2. The snapshot is a native 240 × 160 frame in both;
  capacity changes the compiled scratch/working width, not the visible frame.
  The snapshot does not carry wide shadow tilemaps, so true 16:9/21:9 reveal
  cost is not included. ARM NEON code generation remains unmeasured.

## Thread results

Native capacity 240, AVX2:

| OpenMP threads | Median ms/frame | Speedup | Framebuffer checksum |
| ---: | ---: | ---: | --- |
| 1 | 0.6203 | 1.00× | `0x43991bb091f426d3` |
| 2 | 0.3204 | 1.94× | `0x43991bb091f426d3` |
| 4 | 0.1674 | 3.71× | `0x43991bb091f426d3` |

Capacity 569, no AVX2 (closer to the RG release flags):

| OpenMP threads | Median ms/frame | Speedup | Framebuffer checksum |
| ---: | ---: | ---: | --- |
| 1 | 0.5779 | 1.00× | `0x43991bb091f426d3` |
| 2 | 0.3077 | 1.88× | `0x43991bb091f426d3` |
| 4 | 0.1573 | 3.67× | `0x43991bb091f426d3` |

The checksum is identical across single-, two- and four-worker runs, so the
thread-count checks exercise the same frame. At 60 Hz the host PPU render uses
about 3.7% of a 16.67 ms frame on one worker and about 1.0% on four; these
percentages are host-only and are not device frame-time claims.

The production cap remains 12 workers on this 16-CPU host. The benchmark does not
exercise the render-thread cap logic, audio, engine update, presentation or
driver cost; `docs/perf-render-threads-2026-06-08.md` records why oversubscribing
is avoided.

## Physical RG results

Device and build:

- Anbernic RG DS Plus, kernel `7.2.11 aarch64`, 4 × Cortex-A55
  (`0xd05`, 408 MHz–1.992 GHz, ondemand), 975 MiB RAM.
- ARM64 cross build: `pc_avx2=n`, width capacity 569, `gpu_renderer=y`,
  `ra=n`, `repro_harness=y`, static packages; glibc 2.44 on the device.
- EU ROM, room `area=0x03 room=0x08`, native 240 × 160 frame, 60 Hz config,
  `decouple_render=true`, `internal_scale=1`, `upscale_method=nearest`.
- The room-cap harness bootstrapped through the EU quicksave and warped to the
  same room as the host snapshot. `TMC_PROFILE_DECOUPLED=1` kept the normal
  tick/render decoupling so `game` is engine work, not the legacy pacer sleep.

Decoupled steady-state medians over 12 report windows after the warp:

| Workers | Game tick | Present | PPU render | Game+present | Headroom in 16.67 ms |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0.466 ms | 8.625 ms | 6.362 ms | 9.091 ms | 7.58 ms |
| 2 | 0.526 ms | 6.348 ms | 3.878 ms | 6.926 ms | 9.74 ms |
| 4 | 0.600 ms | 5.411 ms | 2.564 ms | 6.041 ms | 10.63 ms |

The host build's `[render] OpenMP scanline threads` value is `OMP_NUM_THREADS`;
the device launcher defaults to 4, and 4 workers gave the lowest present cost.
A second full-screen present at a similar cost would still fit inside the
16.67 ms budget in this room, but this is a headroom estimate, not a dual-screen
implementation result.

The legacy capture path reports a `game` value that includes the pacer sleep, so
its `game+present` always sits near the 16.67 ms frame period. The decoupled
numbers above are the meaningful work measurements. The first report after the
warp contains the warp/asset hitch and is excluded.

A longer 4-worker run produced 40 steady 120-frame windows after the warp,
spaced exactly 2.0 s apart (the only 5 s gap is the warp transition). Median
work was 6.009 ms with a 6.395 ms worst window, leaving 10.66 ms of the
16.67 ms budget in this room. PSS stayed flat at 99,522–99,523 KiB across 81
samples. The run ended through the room-cap settle timeout rather than the
requested capture, but the profiler windows cover the steady gameplay period.
Post-run temperatures were 43.75 °C and 41.875 °C on the two reported thermal
zones. This is one EU field room; heavier scenes and long thermal saturation
remain to be sampled.

The device has no usable SDL_GPU backend (`SDL_CreateGPUDevice failed`), so the
forced GPU-raster preference falls back to the SDL_Renderer `opengles2`
presentation path and the software PPU. In this historical run a `4:3` config
was effective-native and the captured gameplay frame is exactly 240 × 160.
Current builds enable 4:3; the 256 × 192 device measurement is still pending.

## Memory and cache limits

The fixed CPU pools and cache are bounded by source constants; they were
rechecked by the existing ROM-free regressions:

- Background cache: two resident `569 × 192 × 4` slots plus a 4 KiB root
  = 878,080 bytes (~0.84 MiB). It is within the contractual 8 MiB steady /
  16 MiB peak budget, including the bounded libpng scratch documented in
  `docs/presentation-allocation-audit.md`.
- PPU base framebuffer and LCD accumulator: 1,843,200 bytes each
  (`1280 × 360 × 4`).
- PPU scratch VRAM: 4 MiB.
- Capacity-569 optional pools: 36,416,000-byte internal-scale/GPU-supersample
  workspace, 1,456,640-byte xBRZ 2× and 5,826,560-byte xBRZ 4× workspace
  (41.675 MiB total virtual/static storage). These are demand-paged optional
  workspaces, not steady gameplay RSS; scale 1 does not fill them, and a failed
  budget selection falls back to raw presentation.
- Host room-capture high-water RSS: 85,796 KiB. This includes the 16 MiB ROM
  image, extracted assets, SDL/ImGui and the host runtime.
- RG steady gameplay PSS: 99,326 KiB; RSS and VmHWM: 122,124 KiB. The device
  has 975 MiB total RAM, so the current native profile leaves ample resident
  memory. These are process values from `/proc/<pid>`; GPU/driver allocations
  and swapchain history are outside them.

The host process high-water mark is useful only as a sanity check that no
unbounded cache growth occurred during the warp/settle run. Device GPU/driver
allocations and shader-preset intermediates remain unmeasured.

## Optional paths and regional gaps

- `4:3` is now enabled; `32:9` remains runtime-disabled behind its independent
  scene, layout, sprite and profile gates. RG 4:3 frame-time/thermal numbers
  are still pending (the capture below predates enablement).
- GPU raster/presentation requires a working SDL_GPU backend. The RG has none
  available, so `gpu_raster=true` falls back to SDL_Renderer `opengles2` plus
  the software PPU. Vulkan/GLES parity was exercised separately through
  `tools/ppu_gpu_parity` on local software drivers.
- The RG `4:3` preference in this historical capture was effective-native
  (240 × 160); the current build enables 4:3. Physical-RG 4:3 performance is
  the remaining WP8 measurement.
- Internal scale above 1, xBRZ, voxel view, custom `.glslp` presets and HD
  dialogue keep their documented safe fallbacks. Their worst-case pools are
  bounded in `docs/presentation-allocation-audit.md`; they were not exercised on
  the RG.
- EU is the tested region. USA and JP behavior remains source-reviewed only;
  no USA/JP ROM performance or memory run was performed.

## Evidence and reproduction

Evidence stays local because the snapshot contains ROM-derived graphics:

```sh
python3 tools/rg_performance_probe.py \
  --snapshot .tmp/rg-performance-2026-10-30/room.ppu \
  --threads 1,2,4 --iterations 1000 --repeats 3 \
  --json .tmp/rg-performance-2026-10-30/results-240-avx2.json

python3 tools/rg_performance_probe.py \
  --snapshot .tmp/rg-performance-2026-10-30/room.ppu \
  --threads 1,2,4 --iterations 1000 --repeats 3 \
  --width 569 --cflags '-O3 -fopenmp' \
  --json .tmp/rg-performance-2026-10-30/results-569-noavx2.json
```

The room capture was produced with:

```sh
TMC_AUTOPLAY=1 TMC_ROOMCAP=1 \
TMC_ROOMCAP_WARP=0x03,0x08,0x1d8,0x138,0 TMC_ROOMCAP_SETTLE=300 \
TMC_ROOMCAP_OUT=.tmp/rg-performance-2026-10-30/room.png \
TMC_ROOMCAP_DUMP=.tmp/rg-performance-2026-10-30/room.tmcp \
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/pc/tmc_pc --no-audio
```

`.tmp/rg-performance-2026-10-30/room.tmcp` is the room-cap dump; `room.ppu` is
the same five memory regions with the PPU2 geometry header (`240 × 160`, pitch
240) required by `ppu_bench`. The conversion is not a save state.

The physical run used an ARM64 repro build staged as
`/storage/roms/ports/picori/tmc_pc.perf` plus a copied `launch-perf.sh`:

```sh
systemd-run --unit=picori-perf --collect \
  --property=WorkingDirectory=/storage/roms/ports/picori \
  --setenv=TMC_PROFILE=1 --setenv=TMC_PROFILE_DECOUPLED=1 \
  --setenv=TMC_ROOMCAP=1 \
  --setenv=TMC_ROOMCAP_WARP=0x03,0x08,0x1d8,0x138,0 \
  --setenv=OMP_NUM_THREADS=4 \
  /storage/roms/ports/picori/launch-perf.sh --no-audio
```

The launcher stops EmulationStation, starts Sway/Wayland, runs the game, and
restores the frontend on normal exit. The long room-cap timeout ended the game
abruptly and bypassed that cleanup once; EmulationStation was restarted
manually afterward. `TMC_PROFILE_DECOUPLED=1` keeps the normal tick/render
decoupling despite the room-capture harness, so `game` and `present` are
reported separately. Evidence is retained under
`.tmp/rg-physical-2026-10-30/`: `device-t1.log`, `device-t4.log`,
`device-gameplay-decoupled-t1.log`, `-t2.log`, `-t4.log`, the long-run journal,
and the 240 × 160 `room-native.png` capture.

## Remaining coverage

The native-profile field room meets the immediate 60 fps target with ~10.6 ms of
measured headroom at four workers. Still unmeasured: heavier rooms/effect
scenes, longer thermal saturation, optional upscale/filter/voxel paths, and
USA/JP behavior. A future dual-screen implementation must measure its own
render/present cost; the headroom estimate above is not a dual-screen result.
