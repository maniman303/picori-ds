#!/usr/bin/env python3
"""Extract a game object from a PPU snapshot into an override PNG.

The port's room-capture harness writes the PPU state (I/O, VRAM, palettes,
OAM) with `TMC_ROOMCAP_DUMP=<base>`. This tool renders the OAM pieces that
belong to one logical object and writes a tight, transparent RGBA PNG suitable
for `assets/override/objects/<name>.png`.

The first supported object is the file-select "CHOOSE A FILE" heading. It is
the only OAM object at y=0/16 using OBJ palette 7. The PNG is written at the
original object's frame bounds, so using it unmodified reproduces the same
pixels. Artists can then enlarge the canvas (keeping the centre aligned) to
extend the artwork.

The language gfx sets can land at different OBJ VRAM bases. If the extracted
heading has the wrong tiles/colours, retry with `--char-base 0x14000` (or the
other base shown by your dump). The default English EU extraction uses
0x10000.

Example (EU, English):
  TMC_AUTOPLAY=1 TMC_ROOMCAP=1 TMC_ROOMCAP_FILESELECT=1 \
    TMC_ROOMCAP_DUMP=/tmp/fsel_ppu TMC_ROOMCAP_OUT=/tmp/fsel \
    TMC_BASEROM=... SDL_VIDEODRIVER=dummy ... ./tmc_pc --no-audio
  python3 tools/export_object_override.py --dump /tmp/fsel_ppu_a.bin \
    --profile file_select_header \
    --output assets/override/objects/file_select_header_english.png
"""

import argparse
import struct
import sys
import zlib
from pathlib import Path

# Standard GBA OAM dimensions: [shape][size] -> (width, height)
OAM_SIZES = {
    0: ((8, 8), (16, 16), (32, 32), (64, 64)),
    1: ((16, 8), (32, 8), (32, 16), (64, 32)),
    2: ((8, 16), (8, 32), (16, 32), (32, 64)),
}

PPU_HEADER_SIZE = 4 + 5 * 4
PPU_IO_SIZE = 0x400
PPU_VRAM_SIZE = 0x18000
PPU_BG_PALETTE_SIZE = 0x200
PPU_OBJ_PALETTE_SIZE = 0x200
PPU_OAM_SIZE = 0x400


def read_ppu(path):
    data = Path(path).read_bytes()
    minimum = (PPU_HEADER_SIZE + PPU_IO_SIZE + PPU_VRAM_SIZE + PPU_BG_PALETTE_SIZE +
               PPU_OBJ_PALETTE_SIZE + PPU_OAM_SIZE)
    if len(data) < minimum or data[:4] != b'TMCP':
        raise ValueError(f'{path}: not a TMC PPU snapshot')
    sizes = struct.unpack_from('<5I', data, 4)
    expected = (PPU_IO_SIZE, PPU_VRAM_SIZE, PPU_BG_PALETTE_SIZE, PPU_OBJ_PALETTE_SIZE, PPU_OAM_SIZE)
    if sizes != expected:
        raise ValueError(f'{path}: unexpected PPU section sizes {sizes}')
    offset = PPU_HEADER_SIZE
    io = data[offset:offset + PPU_IO_SIZE]
    offset += PPU_IO_SIZE
    vram = data[offset:offset + PPU_VRAM_SIZE]
    offset += PPU_VRAM_SIZE
    offset += PPU_BG_PALETTE_SIZE
    obj_palette = data[offset:offset + PPU_OBJ_PALETTE_SIZE]
    offset += PPU_OBJ_PALETTE_SIZE
    oam = data[offset:offset + PPU_OAM_SIZE]
    return io, vram, obj_palette, oam


def oam_entries(oam):
    for index in range(128):
        attr0, attr1, attr2, attr3 = struct.unpack_from('<4H', oam, index * 8)
        shape = (attr0 >> 14) & 3
        size = (attr1 >> 14) & 3
        if shape > 2:
            continue
        width, height = OAM_SIZES[shape][size]
        y = attr0 & 0xFF
        x = attr1 & 0x1FF
        if y > 160:
            y -= 256
        if x > 240:
            x -= 512
        if (attr0 & 0x300) == 0x200:
            continue
        yield {
            'slot': index,
            'x': x,
            'y': y,
            'width': width,
            'height': height,
            'tile': attr2 & 0x3FF,
            'palette': (attr2 >> 12) & 0xF,
            'attr0': attr0,
            'attr1': attr1,
            'attr2': attr2,
        }


PROFILES = {
    # Name, OAM Y maximum, OBJ palette, OBJ char base, 1D tile mapping.
    # text_window is the localized text area that the game itself draws; the
    # shared header asset is punched transparent there so the original text
    # remains visible for every language.
    'file_select_header': {'y_max': 16, 'palette': 7, 'char_base': 0x10000,
                           'one_dimensional': True, 'text_window': (32, 0, 136, 16)},
}


def select_pieces(oam, profile):
    pieces = []
    for piece in oam_entries(oam):
        if piece['palette'] != profile['palette']:
            continue
        if piece['y'] < 0 or piece['y'] > profile['y_max']:
            continue
        pieces.append(piece)
    return pieces


def render(pieces, vram, palette, char_base, one_dimensional):
    if not pieces:
        raise ValueError('no OAM pieces matched the object profile')
    x0 = min(piece['x'] for piece in pieces)
    y0 = min(piece['y'] for piece in pieces)
    x1 = max(piece['x'] + piece['width'] for piece in pieces)
    y1 = max(piece['y'] + piece['height'] for piece in pieces)
    width, height = x1 - x0, y1 - y0
    rgba = bytearray(width * height * 4)

    for piece in pieces:
        for py in range(piece['height']):
            for px in range(piece['width']):
                if one_dimensional:
                    tile = piece['tile'] + (px // 8) + (py // 8) * (piece['width'] // 8)
                else:
                    tile = piece['tile'] + (px // 8) + (py // 8) * 32
                tile_address = char_base + tile * 32 + ((px % 8) + (py % 8) * 8) // 2
                if tile_address + 1 > len(vram):
                    continue
                palette_index = (vram[tile_address] >> (((px % 8) & 1) * 4)) & 0xF
                if palette_index == 0:
                    continue
                color = struct.unpack_from('<H', palette,
                                           (piece['palette'] * 16 + palette_index) * 2)[0]
                red = (color & 31) << 3
                green = ((color >> 5) & 31) << 3
                blue = ((color >> 10) & 31) << 3
                dst = ((piece['y'] - y0 + py) * width + (piece['x'] - x0 + px)) * 4
                rgba[dst:dst + 4] = bytes((red, green, blue, 255))
    return width, height, x0, y0, rgba


def read_reference_rgb(path):
    """Read an 8-bit non-interlaced RGB/RGBA PNG into tightly packed RGB."""
    data = Path(path).read_bytes()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError(f'{path}: not a PNG')
    pos = 8
    width = height = depth = color_type = interlace = None
    idat = bytearray()
    while pos + 8 <= len(data):
        length = struct.unpack_from('>I', data, pos)[0]
        tag = data[pos + 4:pos + 8]
        chunk = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if tag == b'IHDR':
            width, height, depth, color_type, _, _, interlace = struct.unpack('>IIBBBBB', chunk)
        elif tag == b'IDAT':
            idat.extend(chunk)
        elif tag == b'IEND':
            break
    if depth != 8 or color_type not in (2, 6) or interlace != 0:
        raise ValueError(f'{path}: expected an 8-bit non-interlaced RGB/RGBA PNG')
    channels = 3 if color_type == 2 else 4
    stride = width * channels
    raw = zlib.decompress(bytes(idat))
    rgb = bytearray(width * height * 3)
    previous = bytearray(stride)
    offset = 0
    for y in range(height):
        filter_type = raw[offset]
        offset += 1
        row = bytearray(raw[offset:offset + stride])
        offset += stride
        if filter_type == 1:
            for i in range(channels, stride):
                row[i] = (row[i] + row[i - channels]) & 0xFF
        elif filter_type == 2:
            for i in range(stride):
                row[i] = (row[i] + previous[i]) & 0xFF
        elif filter_type == 3:
            for i in range(stride):
                left = row[i - channels] if i >= channels else 0
                row[i] = (row[i] + ((left + previous[i]) >> 1)) & 0xFF
        elif filter_type == 4:
            for i in range(stride):
                left = row[i - channels] if i >= channels else 0
                up = previous[i]
                upper_left = previous[i - channels] if i >= channels else 0
                p = left + up - upper_left
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - upper_left)
                predictor = left if pa <= pb and pa <= pc else (up if pb <= pc else upper_left)
                row[i] = (row[i] + predictor) & 0xFF
        elif filter_type != 0:
            raise ValueError(f'{path}: unsupported PNG filter {filter_type}')
        for x in range(width):
            src = x * channels
            dst = (y * width + x) * 3
            rgb[dst:dst + 3] = row[src:src + 3]
        previous = row
    return width, height, rgb


def match_error(width, height, x0, y0, rgba, ref_width, ref_height, ref_rgb):
    """Sum RGB error over opaque asset pixels against a reference screenshot."""
    error = 0
    for y in range(height):
        ref_y = y0 + y
        if ref_y < 0 or ref_y >= ref_height:
            continue
        for x in range(width):
            src = (y * width + x) * 4
            if rgba[src + 3] != 255:
                continue
            ref_x = x0 + x
            if ref_x < 0 or ref_x >= ref_width:
                continue
            dst = (ref_y * ref_width + ref_x) * 3
            error += abs(rgba[src] - ref_rgb[dst])
            error += abs(rgba[src + 1] - ref_rgb[dst + 1])
            error += abs(rgba[src + 2] - ref_rgb[dst + 2])
    return error


def fill_from_reference(width, height, x0, y0, rgba, ref_width, ref_height, ref_rgb):
    """Use the reference frame's exact RGB for opaque object pixels.

    The OAM mask decides which pixels belong to the object; the screenshot
    supplies the final colours. This handles language gfx sets whose pieces
    land at different OBJ VRAM bases (the mask/colour can come from different
    runs of the same object)."""
    result = bytearray(rgba)
    for y in range(height):
        ref_y = y0 + y
        if ref_y < 0 or ref_y >= ref_height:
            continue
        for x in range(width):
            src = (y * width + x) * 4
            if result[src + 3] != 255:
                continue
            ref_x = x0 + x
            if ref_x < 0 or ref_x >= ref_width:
                continue
            dst = (ref_y * ref_width + ref_x) * 3
            result[src:src + 3] = ref_rgb[dst:dst + 3]
    return result


def punch_text_window(width, height, rgba, window):
    """Make the localized text area transparent so the game text shows through."""
    x0, y0, x1, y1 = window
    for y in range(max(0, y0), min(height, y1)):
        for x in range(max(0, x0), min(width, x1)):
            rgba[(y * width + x) * 4 + 3] = 0


def png_chunk(tag, data):
    return (struct.pack('>I', len(data)) + tag + data +
            struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF))


def write_png(path, width, height, rgba):
    raw = bytearray()
    stride = width * 4
    for y in range(height):
        raw.append(0)
        raw.extend(rgba[y * stride:(y + 1) * stride])
    png = (b'\x89PNG\r\n\x1a\n' +
           png_chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)) +
           png_chunk(b'IDAT', zlib.compress(bytes(raw), 9)) +
           png_chunk(b'IEND', b''))
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_bytes(png)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--dump', type=Path, required=True,
                        help='PPU snapshot (*.bin) from TMC_ROOMCAP_DUMP')
    parser.add_argument('--profile', choices=sorted(PROFILES), default='file_select_header')
    parser.add_argument('--char-base', type=lambda value: int(value, 0),
                        help='OBJ character base; defaults to the profile (try 0x14000 if colours/tiles are wrong)')
    parser.add_argument('--reference', type=Path,
                        help='captured screenshot from the same run; auto-selects the OBJ char base')
    parser.add_argument('--tile-mapping', choices=('1d', '2d'),
                        help='OBJ tile mapping; defaults to the profile')
    parser.add_argument('--keep-text', action='store_true',
                        help='do not punch the profile text window (full-frame replacement)')
    parser.add_argument('--output', type=Path,
                        help='PNG path, e.g. assets/override/objects/file_select_header_english.png')
    parser.add_argument('--list-pieces', action='store_true',
                        help='print the selected OAM pieces and exit')
    args = parser.parse_args()

    _, vram, obj_palette, oam = read_ppu(args.dump)
    profile = PROFILES[args.profile]
    pieces = select_pieces(oam, profile)
    if args.list_pieces:
        for piece in pieces:
            print('slot {slot:3d} x={x:4d} y={y:3d} {width:2d}x{height:2d} '
                  'tile={tile:3d} palette={palette}'.format(**piece))
        return 0
    if not args.output:
        parser.error('--output is required unless --list-pieces is used')
    one_dimensional = (args.tile_mapping != '2d') if args.tile_mapping else profile['one_dimensional']
    char_base = args.char_base
    rendered = None
    reference = None
    try:
        if args.reference:
            reference = read_reference_rgb(args.reference)
        if char_base is None and reference:
            ref_width, ref_height, ref_rgb = reference
            best = None
            for candidate in (0x10000, 0x14000):
                try:
                    candidate_render = render(pieces, vram, obj_palette, candidate, one_dimensional)
                except ValueError:
                    continue
                error = match_error(candidate_render[0], candidate_render[1],
                                    candidate_render[2], candidate_render[3], candidate_render[4],
                                    ref_width, ref_height, ref_rgb)
                print(f'  char base 0x{candidate:05x}: reference error {error}')
                if best is None or error < best[0]:
                    best = (error, candidate, candidate_render)
            if best is None:
                raise ValueError('could not render the object with either OBJ char base')
            char_base = best[1]
            rendered = best[2]
            print(f'  selected char base 0x{char_base:05x}')
        if rendered is None:
            if char_base is None:
                char_base = profile['char_base']
            rendered = render(pieces, vram, obj_palette, char_base, one_dimensional)
        width, height, x0, y0, rgba = rendered
        if reference:
            ref_width, ref_height, ref_rgb = reference
            rgba = fill_from_reference(width, height, x0, y0, rgba,
                                       ref_width, ref_height, ref_rgb)
        if not args.keep_text and profile.get('text_window'):
            punch_text_window(width, height, rgba, profile['text_window'])
    except (OSError, ValueError) as error:
        print(f'error: {error}', file=sys.stderr)
        return 1
    write_png(args.output, width, height, rgba)
    print(f'{args.profile}: {len(pieces)} pieces, bounds {x0},{y0} {width}x{height} '
          f'-> {args.output}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
