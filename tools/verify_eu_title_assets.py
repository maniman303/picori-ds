#!/usr/bin/env python3
"""Compare the EU title logo's captured OBJ graphics and palette with the ROM."""
import argparse
import hashlib
from pathlib import Path
import struct

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom', type=Path, required=True)
p.add_argument('--snapshot', type=Path, required=True)
a = p.parse_args()
rom, snap = a.rom.read_bytes(), a.snapshot.read_bytes()
if hashlib.sha256(rom).hexdigest() != 'c84645731952b7677f514ae222683504066334ab2af904e64a8a84ffc1af46c6':
    raise SystemExit('expected the audited EU retail ROM')
if snap[:4] != b'PPU2' or len(snap) != 36 + 0x400 + 0x18000 + 0x200 + 0x200 + 0x400:
    raise SystemExit('invalid PPU2 snapshot')
sizes = struct.unpack_from('<5I', snap, 4)
width, height, pitch = struct.unpack_from('<3I', snap, 24)
if sizes != (0x400, 0x18000, 0x200, 0x200, 0x400) or width != 240 or height != 160 or pitch < width:
    raise SystemExit('unexpected PPU2 region sizes or title geometry')
io = snap[36:36+0x400]
if struct.unpack_from('<H', io)[0] != 0x1741:
    raise SystemExit('not the expected title-screen display state')
vram_at = 36 + 0x400
objpal_at = vram_at + 0x18000 + 0x200
oam_at = objpal_at + 0x200
objpal = snap[objpal_at:oam_at]
oam = struct.unpack_from('<512H', snap, oam_at)
# Object BD, form 0: EU fixed-GFX entry 511 and OBJ palette ID 359.
# Both raw tables are read from the ROM, independent of generated PC offsets.
gfx_blob, gfx_table, obj_palette_table = 0x5a23d0, 0x132180, 0x1329b4
gfx = struct.unpack_from('<I', rom, gfx_table + 511 * 4)[0]
palette = struct.unpack_from('<I', rom, obj_palette_table + (359 - 22) * 4)[0]
if (palette >> 24) != 1 or (gfx & 3):
    raise SystemExit('unexpected title resource encoding')
size = ((gfx >> 24) & 0x7f) * 512
source = gfx_blob + (gfx & 0xfffffc)
expected_gfx = rom[source:source+size]
expected_palette = rom[gfx_blob+(palette & 0xffffff):gfx_blob+(palette & 0xffffff)+32]
# The logo's first visible 64x64 OBJ piece is at (32,8). Its tile and palette
# attributes select the actual dynamic allocations, rather than assuming slots.
matches = []
for i in range(128):
    attr0, attr1, attr2 = oam[i*4:i*4+3]
    if attr0 & 0xff == 8 and attr1 & 0x1ff == 32 and (attr0 & 0xc000) == 0 and (attr1 & 0xc000) == 0xc000:
        matches.append((attr2 & 0x3ff, attr2 >> 12))
if len(matches) != 1:
    raise SystemExit(f'expected one title logo OBJ, got {matches}')
tile, bank = matches[0]
actual_gfx = snap[vram_at+0x10000+tile*32:vram_at+0x10000+tile*32+size]
actual_palette = objpal[bank*32:bank*32+32]
if actual_gfx != expected_gfx:
    raise SystemExit('title logo OBJ tile data does not match its EU ROM resource')
if actual_palette != expected_palette:
    raise SystemExit(f'title logo OBJ palette bank {bank} does not match its EU ROM resource')
for palette_id in (360, 361):
    entry = struct.unpack_from('<I', rom, obj_palette_table + (palette_id - 22) * 4)[0]
    source = gfx_blob + (entry & 0xffffff)
    expected = rom[source:source+32]
    if not any(objpal[slot*32:slot*32+32] == expected for slot in range(16)):
        raise SystemExit(f'title OBJ palette {palette_id} is missing or incorrect')
print(f'EU title OBJ tiles ({size} bytes) and all three title palettes match the ROM: PASS')
