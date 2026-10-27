#!/usr/bin/env python3
"""Describe unfaded native artwork ownership without reading final-frame colors.

This is an audit input for effect policy, not a runtime descriptor.  BG palette
bank is derived from the winning nontransparent source pixel, and backdrop is
kept distinct from BG index zero.  Dynamic OBJ, text and UI are deliberately
excluded, as in the exporter recipes.  A single PNG cannot infer which source
pixel owned a painted margin: consumers must resolve or reject mixed targets.
"""
import argparse
import json
from pathlib import Path
import struct

from export_screen_backgrounds import HEIGHT, WIDTH, Rom, Scene, recipes, u16


def source_coverage(scene, layers, width=WIDTH, height=HEIGHT):
    """Count each visible source's GBA layer and actual palette-bank index."""
    if width <= 0 or height <= 0:
        raise ValueError('coverage dimensions must be positive')
    counts = {}
    # Each entry encodes (GBA layer << 4) | BG palette bank. The backdrop
    # has layer 5, just like BLDCNT; OBJ overlays are a separate foreground.
    # A margin consumer can extend edge ownership without keying on PNG color.
    edges = dict(left=[0] * height, right=[0] * height,
                 top=[0] * width, bottom=[0] * width)
    for y in range(height):
        ordered = [(bg, scene.row_controls[bg][y] if bg in scene.row_controls else control, sx, sy)
                   for bg, control, sx, sy in layers]
        ordered.sort(key=lambda layer: (layer[1] & 3, layer[0]))
        for x in range(width):
            owner, index = 'backdrop', 0
            for bg, control, sx, sy in ordered:
                pixel = scene.sample(control, x + sx, y + sy)
                if pixel is not None:
                    if not 1 <= pixel <= 255:
                        raise ValueError('opaque BG palette index is out of range')
                    owner, index = f'bg{bg}', pixel
                    break
            bank = index // 16
            key = (owner, bank)
            counts[key] = counts.get(key, 0) + 1
            if x == 0 or x == width - 1 or y == 0 or y == height - 1:
                layer = 5 if owner == 'backdrop' else int(owner[2:])
                encoded = (layer << 4) | bank
                if x == 0: edges['left'][y] = encoded
                if x == width - 1: edges['right'][y] = encoded
                if y == 0: edges['top'][x] = encoded
                if y == height - 1: edges['bottom'][x] = encoded
    sources = [dict(layer=owner, palette_bank=bank, pixels=count)
               for (owner, bank), count in sorted(counts.items())]
    return dict(width=width, height=height, pixels=width * height, sources=sources, edges=edges,
                palette_bank_mask=sum(1 << bank for bank in {s['palette_bank'] for s in sources}),
                layer_mask=sum(1 << (5 if layer == 'backdrop' else int(layer[2:]))
                               for layer in {s['layer'] for s in sources}))


def logo_coverage(rom):
    # HandleNintendoCapcomLogos after DispReset: reset leaves BG0 enabled and
    # the logo enables BG2. BG0's cleared map uses screenbase 31 / charbase 3;
    # BG2 uses screenbase 29 / charbase 0 / priority 2. Group 16 supplies
    # shared character data; group 1 supplies logo tiles/map.
    scene = Scene(rom)
    scene.gfx(16)
    scene.gfx(1)
    scene.pal(1 if rom.language == 0 else 2)
    scene.vram[0xf800:0x10000] = bytes(0x800)
    coverage = source_coverage(scene, [(0, 0x1f0c, 0, 0), (2, 0x1d02, 0, 0)])
    coverage['palette_zero_bgr555'] = u16(scene.palette, 0)
    white = {}
    for y in range(HEIGHT):
        for x in range(WIDTH):
            index = scene.sample(0x1d02, x, y)
            if index is not None and u16(scene.palette, index * 2) == 0x7fff:
                white[index] = white.get(index, 0) + 1
    coverage['bg2_white_pixels_by_palette_index'] = white
    return coverage


def local_hint_coverage(rom):
    """Reconstruct sub_080A6B04's filled BG3 for each installed health."""
    result = []
    for group in (86, 87, 88):
        scene = Scene(rom)
        # LoadGfxGroups, sub_080A4D34, then EU screen-6's palette/gfx setup.
        # The item groups here are the native no-upgrade fixture; all occur
        # before the health group which installs the BG3 character data.
        for gfx in (16, 23, 25, 27, group):
            scene.gfx(gfx)
        for palette in (11, 12, 181, 185):
            scene.pal(palette)
        scene.gfx(129)
        scene.vram[0xf000:0x10000] = struct.pack('<H', 0x5001) * 2048
        # Both unfaded palette entries are 0x475f; their fade banks differ.
        scene.palette[0:2] = scene.palette[190:192] = struct.pack('<H', 0x475f)
        indices = {scene.sample(0x1e0b, x, y) for y in range(HEIGHT) for x in range(WIDTH)}
        coverage = source_coverage(scene, [(3, 0x1e0b, 0, 0)])
        coverage.update(graphics_group=group, palette_indices=sorted(indices, key=lambda x: -1 if x is None else x))
        result.append(coverage)
    return result


def figurine_field_coverage(rom):
    """Reconstruct the cleared BG3 below the native figurine viewer."""
    result = []
    for group in (86, 87, 88):
        scene = Scene(rom)
        for gfx in (16, 23, 25, 27, group, 89):
            scene.gfx(gfx)
        for palette in (11, 12, 181, 206):
            scene.pal(palette)
        scene.vram[0xf000:0x10000] = bytes(0x1000)
        # FigurineMenu_080A4608 copies entry 251 into the backdrop.
        scene.palette[0:2] = scene.palette[502:504]
        coverage = source_coverage(scene, [(3, 0x1e0b, 0, 0)])
        coverage.update(graphics_group=group, palette_zero_bgr555=u16(scene.palette, 0))
        result.append(coverage)
    return result


def audit(rom):
    result = {}
    for name, scene, layers, backdrop in recipes(rom):
        if name == 'map_hint':
            continue  # a live palette-zero field, not the reference swatch
        entry = source_coverage(scene, layers)
        entry['groups'], entry['palette_groups'] = scene.groups, scene.palettes
        entry['backdrop_override_bgr555'] = backdrop
        entry['omitted_obj_overlay_pixels'] = len(scene.overlays)
        result[name] = entry
    if len(result) != 33:
        raise ValueError(f'expected 33 independent image recipes, got {len(result)}')
    return dict(region=rom.region, rom_sha256=rom.digest, language=rom.language,
                images=result, logo=logo_coverage(rom), local_hint=local_hint_coverage(rom),
                figurine_field=figurine_field_coverage(rom))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--language', type=int, choices=range(1, 6), default=1)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('output already exists')
    rom = Rom(args.rom, args.language)
    if rom.region == 'USA' and args.language != 1:
        parser.error('the USA ROM supports English only')
    result = audit(rom)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(f"{result['region']}: {len(result['images'])} image sources and logo; {args.output}")


if __name__ == '__main__':
    main()
