# PPU capture format

`TMC_PERFCAP_DUMP` writes a static PPU fixture for `tools/ppu_parity.c` and
`tools/ppu_bench.c`. It is separate from saves and quicksave states. The shared
reader/writer is `port/ppu/include/ppu_snapshot.h`; no C struct layout is stored.

New captures use **PPU2**. Every integer is little-endian, regardless of host
byte order. Header offsets are in bytes:

| Offset | Length | Meaning |
| --- | --- | --- |
| 0 | 4 | ASCII `PPU2` |
| 4 | 20 | Five unsigned 32-bit region sizes: `0x400`, `0x18000`, `0x200`, `0x200`, `0x400` |
| 24 | 4 | Unsigned 32-bit effective visible width, in pixels |
| 28 | 4 | Unsigned 32-bit effective visible height, in pixels |
| 32 | 4 | Unsigned 32-bit row pitch, in pixels |
| 36 | `0x18c00` | Region data in the order below |

Payload order is IO bytes, VRAM bytes, BG palette halfwords, OBJ palette
halfwords, then OAM halfwords. Palette/OAM halfwords are also little-endian.
Total PPU2 file length is 101,412 bytes. Display mode comes from the captured
GBA DISPCNT register, using the same routing as the live renderer.

Readers validate the exact region sizes, nonzero dimensions, width against
their compiled renderer capacity, height against the bounded software-render
height, pitch against width/buffer capacity, and total frame extent. Truncated
headers/payloads, unknown versions and trailing bytes are rejected. Unsupported
geometry fails explicitly rather than being silently clipped to another ratio.

Legacy **PPU1** files have the same five sizes and payload, with no geometry
fields: data starts at offset 24, for a total length of 101,400 bytes. Both
tools continue to read them as native **240 × 160, pitch 240**, independent of
the reader's compiled width capacity. Old wide captures have no recoverable
geometry metadata and should be recaptured as PPU2.

This remains a **static** fixture: it does not serialize per-line HDMA inputs,
shadow maps, port sprite metadata, or contextual artwork. Static rendering
cannot reproduce effects or extended-world content that depend on those inputs.
The live Tier B hash and named scene captures cover separate runtime behavior.
Version 2 adds geometry; it does not claim complete frame replay. In particular,
live [signed OBJ metadata](sprite-coordinate-audit.md) remains outside this
format; use live rendering/PNG captures to check those positions. The live
perfcap writer now rejects capture while signed OBJ metadata is bound, before
touching the output path, rather than producing an incomplete PPU2 snapshot.

The software PPU keeps the hardware line constant at 160. `PPUMemory.frame_height`
is a separate requested render extent: zero defaults to 160, PC rendering is
bounded at 192, and N64 remains bounded at 160. Query the normalized geometry
after setup. An active scanline callback currently caps effective height at
160 until its extension policy is classified. Callbacks consume the original
160-row sequence even when visible height is cropped; forced blank retains
the existing early-return behavior. Stable register state can render extra
rows and continue affine references mathematically. Window and signed OAM
coordinates still retain their hardware interpretation.

Native golden pixels are unchanged. This interface does not enable the 4:3
gameplay profile; camera, tile residency, layout and raster-effect work must
be completed before the setting becomes available.
