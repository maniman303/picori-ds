#!/usr/bin/env python3
"""Export native screen artwork from a verified local retail ROM (stdlib only).

This reconstructs tilemaps, not tile sheets or stretched screenshots. Output is
local ROM-derived artwork, ignored by Git. Existing files require --overwrite.
Filenames are shared by screen families; no JSON or palette files are needed.
Runtime screen UI remains separate from the exported decorative artwork.
"""

import argparse
import hashlib
from pathlib import Path
import re
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
WIDTH, HEIGHT = 240, 160
LANGUAGES = {1: 'English', 2: 'French', 3: 'German', 4: 'Spanish', 5: 'Italian'}
# Values from port/port_rom_picker.c and port/port_rom.c.
ROMS = {
    'c84645731952b7677f514ae222683504066334ab2af904e64a8a84ffc1af46c6':
        ('EU', 1, 0x100204, 0xFED88, 0x5A23D0),
    'bedc74df62755f705398273de8ed3bc59be610cf55760d0b9aa277f1f5035e73':
        ('USA', 0, 0x100AA8, 0xFF850, 0x5A2E80),
}
BG_BUFFERS = {1: 0x21F30, 2: 0x344B0, 3: 0x1A40}


def u16(data, offset):
    return struct.unpack_from('<H', data, offset)[0]


def u32(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def png_bytes(pixels, width=WIDTH, height=HEIGHT):
    def chunk(kind, data):
        return (struct.pack('>I', len(data)) + kind + data +
                struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff))
    assert len(pixels) == width * height * 3
    scanlines = b''.join(b'\0' + pixels[y*width*3:(y+1)*width*3]
                         for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>2I5B', width, height, 8, 2, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(scanlines, 9)) + chunk(b'IEND', b''))


def atomic_write(path, data):
    temporary = path.with_name(path.name + '.tmp')
    temporary.write_bytes(data)
    temporary.replace(path)


class Rom:
    def __init__(self, path, language):
        self.data = path.read_bytes()
        self.digest = hashlib.sha256(self.data).hexdigest()
        if self.digest not in ROMS:
            raise ValueError('Expected a verified retail USA or EU ROM; unsupported SHA-256: ' + self.digest)
        self.region, self.region_id, self.gfx_table, self.pal_table, self.blob = ROMS[self.digest]
        # Public Language enum vs EU save-header slots (include/main.h).
        self.language = language + 1 if self.region == 'EU' else language
        # The passthrough data.pak does not relocate all regional tables. Apply
        # the very same segments/patches as Port_InitRomStubs instead.
        self.stub_source = (ROOT / 'port/port_rom_stubs.c').read_text()
        table = self.stub_source.split('static const RomStubSegment')[0]
        self.stubs = re.findall(r'\{ (\w+), 0x([0-9A-Fa-f]+)u, (\d+)u \}', table)

    def read(self, offset, size):
        if not 0 <= offset <= offset + size <= len(self.data):
            raise ValueError(f'ROM read out of bounds: {offset:x}+{size:x}')
        return self.data[offset:offset + size]

    def symbol(self, name):
        index = next(i for i, row in enumerate(self.stubs) if row[0] == name)
        _, address, size = self.stubs[index]
        if self.region_id == 0:
            return self.read(int(address, 16), int(size))
        result = bytearray(int(size))
        pattern = rf'\{{ {index}, {self.region_id}, 0x([0-9A-Fa-f]+)u, 0x([0-9A-Fa-f]+)u, (\d+)u \}}'
        segments = re.findall(pattern, self.stub_source)
        if not segments:
            raise ValueError(f'Missing regional symbol segments: {name}')
        for at, source, length in segments:
            at, length = int(at, 16), int(length)
            assert at + length <= len(result)
            result[at:at + length] = self.read(int(source, 16), length)
        pattern = rf'\{{ {index}, {self.region_id}, (\d+), 0x([0-9A-Fa-f]+)u, 0x([0-9A-Fa-f]+)u \}}'
        for length, at, value in re.findall(pattern, self.stub_source):
            at, length = int(at, 16), int(length)
            result[at:at + length] = int(value, 16).to_bytes(4, 'little')[:length]
        return bytes(result)

    def lz77(self, offset):
        header = u32(self.data, offset)
        if header & 255 != 0x10:
            raise ValueError(f'Invalid LZ77 header at {offset:x}')
        size, pos = header >> 8, offset + 4
        if size > 0x40000:
            raise ValueError('Oversized LZ77 graphics')
        output = bytearray()
        while len(output) < size:
            flags = self.read(pos, 1)[0]
            pos += 1
            for bit in range(7, -1, -1):
                if len(output) == size:
                    break
                if flags & (1 << bit):
                    first, second = self.read(pos, 2)
                    pos += 2
                    length, distance = (first >> 4) + 3, ((first & 15) << 8 | second) + 1
                    if distance > len(output):
                        raise ValueError('Invalid LZ77 backreference')
                    for _ in range(min(length, size - len(output))):
                        output.append(output[-distance])
                else:
                    output.extend(self.read(pos, 1))
                    pos += 1
        return bytes(output)


class Scene:
    def __init__(self, rom):
        self.rom = rom
        self.vram = bytearray(0x18000)
        self.ewram = bytearray(0x40000)
        self.palette = bytearray(0x400)
        self.groups, self.palettes = [], []
        self.row_controls = {}

    def gfx(self, group):
        self.groups.append(group)
        pos = u32(self.rom.data, self.rom.gfx_table + group * 4) - 0x8000000
        for _ in range(64):
            raw, dest, size = struct.unpack('<III', self.rom.read(pos, 12))
            control = (raw >> 24) & 15
            if control == 13:
                break
            load = (control == 7 or control == self.rom.language or
                    control == 14 and self.rom.language not in (0, 1) or
                    control == 15 and self.rom.language != 0)
            if load:
                source = self.rom.blob + (raw & 0xffffff)
                data = (self.rom.lz77(source) if size & 0x80000000 else
                        self.rom.read(source, size & 0x7fffffff))
                if 0x6000000 <= dest < 0x6018000:
                    target, offset = self.vram, dest - 0x6000000
                elif 0x2000000 <= dest < 0x2040000:
                    target, offset = self.ewram, dest - 0x2000000
                else:
                    raise ValueError(f'Unexpected gfx destination {dest:x}')
                if offset + len(data) > len(target):
                    raise ValueError('Graphics write out of bounds')
                target[offset:offset + len(data)] = data
            if not raw & 0x80000000:
                return
            pos += 12
        else:
            raise ValueError('Unterminated gfx group')

    def pal(self, group):
        self.palettes.append(group)
        pos = u32(self.rom.data, self.rom.pal_table + group * 4) - 0x8000000
        for _ in range(64):
            palette, dest, flags = struct.unpack('<HBB', self.rom.read(pos, 4))
            count = (flags & 15) or 16
            if dest + count > 32:
                raise ValueError('Palette write out of bounds')
            self.palette[dest*32:(dest+count)*32] = self.rom.read(self.rom.blob + palette*32, count*32)
            if not flags & 0x80:
                return
            pos += 4
        raise ValueError('Unterminated palette group')

    def upload(self, bg, control):
        offset = ((control >> 8) & 31) * 0x800
        size = 0x1000 if bg == 3 else 0x800
        self.vram[offset:offset+size] = self.ewram[BG_BUFFERS[bg]:BG_BUFFERS[bg]+size]

    def sample(self, control, x, y):
        width = 512 if control & 0x4000 else 256
        height = 512 if control & 0x8000 else 256
        x, y = x % width, y % height
        screenblock = (x // 256) + (y // 256) * (width // 256)
        map_addr = ((control >> 8) & 31) * 0x800 + screenblock*0x800
        tile = u16(self.vram, map_addr + ((y//8 % 32)*32 + x//8 % 32)*2)
        tx, ty = (x & 7) ^ (7 if tile & 0x400 else 0), (y & 7) ^ (7 if tile & 0x800 else 0)
        eight = bool(control & 0x80)
        address = ((control >> 2) & 3)*0x4000 + (tile & 1023)*(64 if eight else 32)
        address += ty*(8 if eight else 4) + (tx if eight else tx//2)
        if address >= 0x10000:
            return None
        value = self.vram[address]
        index = value if eight else (value >> ((tx & 1)*4)) & 15
        if not index:
            return None
        return index if eight else index + (tile >> 12)*16

    def render(self, layers, backdrop=None):
        # layers: (BG number, control, scroll x, scroll y). Front-to-back GBA
        # priority, with lower BG number winning ties. Dynamic OBJ/text omitted.
        colors = [u16(self.palette, i*2) for i in range(256)]
        if backdrop is not None:
            colors[0] = backdrop
        pixels = bytearray()
        for y in range(HEIGHT):
            row_layers = [(bg, self.row_controls.get(bg, [control]*HEIGHT)[y], sx, sy)
                          for bg, control, sx, sy in layers]
            row_layers.sort(key=lambda layer: (layer[1] & 3, layer[0]))
            for x in range(WIDTH):
                index = next((value for _, control, sx, sy in row_layers
                              if (value := self.sample(control, x+sx, y+sy)) is not None), 0)
                color = colors[index]
                pixels.extend(((color & 31) << 3, ((color >> 5) & 31) << 3, ((color >> 10) & 31) << 3))
        return bytes(pixels)


# Artist-facing names are literal code constants, shared across all ratios.
# Runtime implementation will mirror this semantic mapping in its port catalog;
# it will not read a per-ratio manifest or identify screens by tile hashes.
MENU_CONTEXTS = (
    'pause_inventory', 'pause_quest_status', 'pause_map', 'pause_dungeon_map',
    'local_map_hint', 'fast_travel_map', 'pause_kinstone_pieces',
    'pause_sword_techniques', 'save_confirmation', 'sleep_confirmation',
) + tuple(f'map_detail_{area:02d}' for area in range(17)) + (
    'map_detail_09_house_built', 'map_detail_11_fusion_done',
    'map_detail_14_interior_1', 'map_detail_14_interior_2', 'map_detail_14_interior_3',
)
IMAGE_CONTEXTS = {
    'title_screen': ('title',),
    'file_select': ('file_select', 'new_file', 'file_copy', 'file_erase',
                    'file_select_name_entry', 'file_select_options', 'file_select_language'),
    'menu_background': MENU_CONTEXTS,
    'kinstone_background': tuple(f'kinstone_fusion_{variant:02d}' for variant in range(1, 8)),
    **{f'prologue_{page:02d}': (f'prologue_page_{page}',) for page in range(1, 7)},
    **{f'credits_{page:02d}': (f'credits_illustration_{page}',) for page in range(1, 9)},
    'ending_screen': ('ending_card',),
}
# Kept for artists as a solid swatch, not an image override at runtime.
COLOR_REFERENCES = {'map_hint'}
SOLID_CONTEXTS = {
    'boot_logos': 'White; original logos remain inside the native screen.',
    'game_over': 'Black; original letters and prompts remain inside the native screen.',
    'ending_save': 'Black; original save prompts remain inside the native screen.',
    'credits_text': 'Black in explicitly cleared/text-only phases.',
    'figurine_collection_and_details': 'Live palette-zero field copied from palette entry 251.',
    'map_hint': 'Generated field matching native bottom-center pixel; EU reference #C8E090.',
}


def solid_bottom_center_reference(pixels):
    """Use native pixel (120,159), without sampling map/UI content or resizing."""
    offset = ((HEIGHT-1)*WIDTH + WIDTH//2)*3
    if len(pixels) != WIDTH*HEIGHT*3:
        raise ValueError('Color reference requires a complete 240x160 RGB frame')
    return pixels[offset:offset+3] * (WIDTH*HEIGHT)


def recipes(rom):
    def scene(groups, palettes):
        value = Scene(rom)
        for group in groups:
            value.gfx(group)
        for group in palettes:
            value.pal(group)
        return value

    # All original screen layouts/frames/maps are foreground, not overrides.
    yield 'title_screen', scene([2], [4]), [(1, 0x1e03, 0, 0)], None
    value = scene([5], [9])
    # sub_0805066C installs animated palette banks separately from group 9.
    # Export its first stable frame (table index 1, offset 0).
    for bank in range(11, 15):
        source = (0x11a60 if rom.region == 'EU' else 0x11aa0) + (bank-11)*0x100
        value.palette[bank*32:(bank+1)*32] = rom.read(rom.blob+source, 32)
    yield 'file_select', value, [(3, 0x1e0b, 0, 0)], None

    # sub_080A4D34: use the clean hat-pattern decoration, without any screen's
    # inventory slots, map/frame, prompts, or health-dependent tilemap variant.
    value = scene([86], [181])
    value.upload(3, 0x1e0b)
    yield 'menu_background', value, [(3, 0x1e0b, 0, 0)], None

    # All seven choices keep the same stone-table setting. The varying pieces
    # and normal game UI stay on their original native canvas at runtime.
    value = scene([86, 117, 118], [181, 203])
    for bg, control in [(1, 0x1c01), (2, 0x1d02), (3, 0x1e0b)]:
        value.upload(bg, control)
    yield 'kinstone_background', value, [(1, 0x1c01, 0, 0), (2, 0x1d02, 0, 0),
                                         (3, 0x1e0b, 0, 0)], None

    # Subtask_MapHint_0 disables BG3 and exposes the solid border. Keep only
    # its bottom-center color as a swatch; the map itself stays native UI.
    value = scene([86, 94], [181, 185])
    value.upload(1, 0x1c03)
    value.upload(2, 0x1d0b)
    yield 'map_hint', value, [(1, 0x1c03, 0, -4), (2, 0x1d0b, 0, -4)], u16(value.palette, 81*2)

    for page in range(6):
        value = scene([58+page], [138+page])
        yield f'prologue_{page+1:02d}', value, [(2, 0x1dc1, 0, 0)], 0

    table = rom.symbol('gUnk_08127CEC')
    for index in range(len(table)//2):
        palette, group = table[index*2:index*2+2]
        if not group:
            continue
        value = scene([group], [11, 12, palette])
        name = 'ending_screen' if index == 8 else f'credits_{index+1:02d}'
        yield name, value, [(2, 0x1dc3, 0, 0)], 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--language', type=int, choices=LANGUAGES, default=1,
                        help='1=English, 2=French, 3=German, 4=Spanish, 5=Italian')
    parser.add_argument('--output', type=Path, default=ROOT / 'assets/override/32')
    parser.add_argument('--overwrite', action='store_true')
    args = parser.parse_args()
    rom = Rom(args.rom, args.language)
    if rom.region == 'USA' and args.language != 1:
        parser.error('USA recipes currently support English only')
    images, skipped = [], []
    for name, value, layers, backdrop in recipes(rom):
        pixels = value.render(layers, backdrop)
        if name in COLOR_REFERENCES:
            pixels = solid_bottom_center_reference(pixels)
        solid = len(set(zip(pixels[::3], pixels[1::3], pixels[2::3]))) == 1
        if name not in IMAGE_CONTEXTS and name not in COLOR_REFERENCES:
            raise ValueError(f'Unknown export filename: {name}')
        if solid and name not in COLOR_REFERENCES:
            skipped.append(name)
        else:
            images.append((name, pixels))
    outputs = [args.output / (name+'.png') for name, _ in images]
    contact_path = args.output.parent / 'background_contact_sheet.png'
    outputs += [args.output / 'README.md', contact_path]
    conflicts = [str(path) for path in outputs if path.exists()]
    if conflicts and not args.overwrite:
        parser.error('Existing exports preserved; use --overwrite to replace: ' + ', '.join(conflicts))
    args.output.mkdir(parents=True, exist_ok=True)
    for name, pixels in images:
        atomic_write(args.output / (name+'.png'), png_bytes(pixels))
    columns, rows = 5, (len(images)+4)//5
    sheet_width, sheet_height = columns*WIDTH, rows*HEIGHT
    sheet = bytearray(sheet_width*sheet_height*3)
    for i, (_, pixels) in enumerate(images):
        for y in range(HEIGHT):
            dest = ((i//columns*HEIGHT+y)*sheet_width + i%columns*WIDTH)*3
            sheet[dest:dest+WIDTH*3] = pixels[y*WIDTH*3:(y+1)*WIDTH*3]
    atomic_write(contact_path, png_bytes(sheet, sheet_width, sheet_height))
    readme = (f'# Shared native background references\n\n{len(images)} ordinary RGB PNGs '
              f'({len(images)-len(COLOR_REFERENCES)} artwork references and {len(COLOR_REFERENCES)} color swatch), each '
              f'240 × 160, from the {rom.region}/{LANGUAGES[args.language]} ROM. These are repainting references; '
              'artist replacements may use any colors and normal PNG transparency. '
              'No palette, sidecar, JSON, or tile-format knowledge is required.\n\n'
              'Create the corresponding image at 256 × 192 in ../43/ for 4:3. '
              'The original 240 × 160 screen, including all UI, is centered at (8, 16) '
              'and covers the middle of the artwork. Only the added margins show. '
              'Missing expected artwork uses solid black; boot logos always use generated '
              'white and have no override. Runtime lookup is still planned.\n\n'
              '`menu_background.png` is the clean common hat pattern used behind inventory, '
              'quest status, map menus and all detail-map variants, fast travel, kinstone '
              'pieces, sword techniques, and save/sleep prompts. The maps and frames are '
              'original foreground UI, not separate background files.\n\n'
              '`kinstone_background.png` serves all seven fusion variants. The original '
              'fusion interface remains unchanged in the center.\n\n'
              'The health-dependent common-menu variants also use menu_background.png. '
              'The figurine viewer uses a generated live field color. '
              '`map_hint.png` is a solid color reference sampled from the native screen’s '
              'bottom-center pixel (120, 159): EU #C8E090, RGB (200, 224, 144). '
              'The planned runtime generates that border color and does not load the swatch. '
              'Game over, cleared credits '
              'and post-credits saving also use generated fills.\n\n'
              '../background_contact_sheet.png is an overview, not an override. '
              'Distinct prologue/credits illustrations retain their own files.\n\n' +
              '\n'.join(f'{i+1}. `{name}.png`' for i, (name, _) in enumerate(images))+'\n')
    atomic_write(args.output / 'README.md', readme.encode())
    print(f'Exported {len(images)} shared backgrounds to {args.output}; {len(skipped)} solid recipes skipped.')


if __name__ == '__main__':
    main()
