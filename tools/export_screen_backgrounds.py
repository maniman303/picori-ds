#!/usr/bin/env python3
"""Export native screen artwork from a verified local retail ROM (stdlib only).

This reconstructs tilemaps, not tile sheets or stretched screenshots. Output is
local ROM-derived artwork, ignored by Git. Replacing existing PNGs requires
--overwrite; --add-missing preserves them while adding new references.
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
        self.overlays = []  # Indexed, unfaded foreground reference pixels only.

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
        colors = [u16(self.palette, i*2) for i in range(512)]
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
        for x, y, index in self.overlays:
            if 0 <= x < WIDTH and 0 <= y < HEIGHT and index:
                color = colors[index]
                at = (y * WIDTH + x) * 3
                pixels[at:at+3] = bytes(((color & 31) << 3, ((color >> 5) & 31) << 3, ((color >> 10) & 31) << 3))
        return bytes(pixels)

    def blank_menu_heading(self):
        # sub_080A5128, frame 0: two 64x32 OBJ pieces at x=0/64,y=0.
        # Group 90 installs tiles 64..127. Palette 5 text uses indices 6..9;
        # replace only those interior glyph pixels with native field index 12.
        # Border indices 10/11 and transparent index zero remain intact.
        for y in range(32):
            for x in range(128):
                tile = 64 + (x // 64) * 32 + (y // 8) * 8 + (x % 64) // 8
                address = 0x10000 + tile * 32 + (y % 8) * 4 + (x % 8) // 2
                index = (self.vram[address] >> ((x & 1) * 4)) & 15
                if index in (6, 7, 8, 9) and x < 108 and 5 <= y <= 19:
                    index = 12
                if index:
                    self.overlays.append((x, y, 256 + 5 * 16 + index))


# Artist-facing names are literal code constants, shared across all ratios.
# Runtime implementation will mirror this semantic mapping in its port catalog;
# it will not read a per-ratio manifest or identify screens by tile hashes.
TITLED_MENU_CONTEXTS = (
    'pause_inventory', 'pause_quest_status', 'pause_map', 'pause_dungeon_map',
) + tuple(f'map_detail_{area:02d}' for area in range(17)) + (
    'map_detail_09_house_built', 'map_detail_11_fusion_done',
    'map_detail_14_interior_1', 'map_detail_14_interior_2', 'map_detail_14_interior_3',
)
# Group 93 replaces the health tilemap: fast travel has one separate image.
MENU_CONTEXTS = ('save_confirmation', 'sleep_confirmation')
MENU_HEALTH_VARIANTS = ('normal', 'damaged', 'critical')
IMAGE_CONTEXTS = {
    'title_screen': ('title',),
    'fast_travel_background': ('fast_travel_map',),
    'file_select': ('file_select', 'new_file', 'file_copy', 'file_erase',
                    'file_select_name_entry', 'file_select_options', 'file_select_language'),
    **{f'menu_background_{health}': tuple(f'{context}_{health}' for context in MENU_CONTEXTS)
       for health in MENU_HEALTH_VARIANTS},
    **{f'menu_titled_background_{health}': tuple(f'{context}_{health}' for context in TITLED_MENU_CONTEXTS)
       for health in MENU_HEALTH_VARIANTS},
    **{f'kinstone_pieces_background_{health}': (f'pause_kinstone_pieces_{health}',)
       for health in MENU_HEALTH_VARIANTS},
    **{f'sword_techniques_background_{health}': (f'pause_sword_techniques_{health}',)
       for health in MENU_HEALTH_VARIANTS},
    **{f'kinstone_background_{health}': tuple(f'kinstone_fusion_{variant:02d}_{health}' for variant in range(1, 8))
       for health in MENU_HEALTH_VARIANTS},
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
    'local_map_hint': 'Live BG3 palette entry 95; fusion reveal replaces the menu decoration with a solid field.',
}


def solid_bottom_center_reference(pixels):
    """Use native pixel (120,159), without sampling map/UI content or resizing."""
    offset = ((HEIGHT-1)*WIDTH + WIDTH//2)*3
    if len(pixels) != WIDTH*HEIGHT*3:
        raise ValueError('Color reference requires a complete 240x160 RGB frame')
    return pixels[offset:offset+3] * (WIDTH*HEIGHT)


def menu_recipes(rom):
    # sub_080A4D34 selects 0x56/0x57/0x58 at menu installation:
    # critical if health <= 8, otherwise damaged if missing health >= 9.
    # Keep all three decorations separate in each family. Static frame/plate
    # references guide alignment; live slots, map data and prompts stay native.
    for index, health in enumerate(MENU_HEALTH_VARIANTS):
        value = Scene(rom)
        value.gfx(0x56 + index)
        value.pal(181)
        value.upload(3, 0x1e0b)
        yield f'menu_background_{health}', value, [(3, 0x1e0b, 0, 0)], None

    for stem, group, palette in (('menu_titled_background', 90, 182),
                                 ('kinstone_pieces_background', 125, 204),
                                 ('sword_techniques_background', 126, 205)):
        for index, health in enumerate(MENU_HEALTH_VARIANTS):
            value = Scene(rom)
            for gfx in (0x56 + index, group):
                value.gfx(gfx)
            for pal in (181, palette):
                value.pal(pal)
            layers = [(3, 0x1e0b, 0, 0)]
            value.upload(3, 0x1e0b)
            if stem == 'menu_titled_background':
                value.blank_menu_heading()
            else:
                # Static BG2 frame/icon only. Headings, items, counters and
                # selection sprites are OBJ/BG0 and are intentionally omitted.
                value.upload(2, 0x1d03)
                layers.insert(0, (2, 0x1d03, 0, 0))
            yield f'{stem}_{health}', value, layers, None

    # All seven fusion choices share one stone-table reference per installed
    # health decoration. Retain the same representative fusion layout.
    for index, health in enumerate(MENU_HEALTH_VARIANTS):
        value = Scene(rom)
        for group in (0x56 + index, 117, 118):
            value.gfx(group)
        for palette in (181, 203):
            value.pal(palette)
        layers = [(1, 0x1c01, 0, 0), (2, 0x1d02, 0, 0), (3, 0x1e0b, 0, 0)]
        for bg, control, _, _ in layers:
            value.upload(bg, control)
        yield f'kinstone_background_{health}', value, layers, None


def fast_travel_recipe(rom):
    # Subtask_FastTravel_0: the normal preload supplies shared character data;
    # group 93 replaces its BG3 tilemap. Groups 87/88 give identical final BG3.
    # Only the fixed blue decoration is artwork. BG1/BG2 map/frame and live
    # OBJ windcrests/cursor remain native foreground, so do not upload them.
    value = Scene(rom)
    for group in (86, 93, 129):
        value.gfx(group)
    for palette in (181, 185):
        value.pal(palette)
    value.upload(3, 0x1e0b)
    return value, [(3, 0x1e0b, 0, 0)], None


def staffroll_offsets(rom):
    # Match StaffrollTask_State0's language-dependent command stream. Commands
    # are 12 bytes; type 1 carries the illustration index and BG2 X offset.
    symbol = ('gUnk_08127998' if rom.language >= 2 else
              'gUnk_08127644' if rom.language == 1 else 'gUnk_081272F0')
    commands = rom.symbol(symbol)
    offsets = {}
    for pos in range(0, len(commands) - 11, 12):
        if commands[pos] != 1:
            continue
        index, offset = struct.unpack_from('<HH', commands, pos + 6)
        if index in offsets and offsets[index] != offset:
            raise ValueError(f'{symbol}: conflicting offsets for illustration {index}')
        offsets[index] = offset
    return offsets


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

    yield from menu_recipes(rom)
    yield 'fast_travel_background', *fast_travel_recipe(rom)

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
    offsets = staffroll_offsets(rom)
    for index in range(len(table)//2):
        palette, group = table[index*2:index*2+2]
        if not group:
            continue
        value = scene([group], [11, 12, palette])
        name = 'ending_screen' if index == 8 else f'credits_{index+1:02d}'
        # BG2 is 512 pixels wide and wraps its unsigned hardware offset.
        if index not in offsets:
            raise ValueError(f'staff roll has no installation for illustration {index}')
        offset = offsets[index]
        yield name, value, [(2, 0x1dc3, offset, 0)], 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--language', type=int, choices=LANGUAGES, default=1,
                        help='1=English, 2=French, 3=German, 4=Spanish, 5=Italian')
    parser.add_argument('--output', type=Path, default=ROOT / 'assets/override/32')
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--overwrite', action='store_true')
    mode.add_argument('--add-missing', action='store_true',
                      help='preserve every existing PNG; add missing references and refresh generated README/contact sheet')
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
    if conflicts and not args.overwrite and not args.add_missing:
        parser.error('Existing exports preserved; use --add-missing to add new files or --overwrite to replace: ' + ', '.join(conflicts))
    args.output.mkdir(parents=True, exist_ok=True)
    for name, pixels in images:
        target = args.output / (name+'.png')
        if not (args.add_missing and target.exists()):
            atomic_write(target, png_bytes(pixels))
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
              '`menu_background_normal.png`, `menu_background_damaged.png`, and '
              '`menu_background_critical.png` are the plain-menu decorations for save/sleep '
              'prompts and verified settings without a top-left heading. '
              '`menu_titled_background_{normal,damaged,critical}.png` serves Items, quest '
              'and pause maps, with a blank native title plate as alignment reference. '
              '`kinstone_pieces_background_{normal,damaged,critical}.png` and '
              '`sword_techniques_background_{normal,damaged,critical}.png` each have their '
              'own static frame/icon setting. Live contents, cursor and localized '
              'heading lettering remain original foreground. The runtime title-frame '
              'completion in added margins is still pending.\n\n'
              'With --add-missing every existing PNG is preserved. This generated '
              'contact sheet shows ROM recipe references; preserved artist files can differ.\n\n'
              '`kinstone_background_normal.png`, `kinstone_background_damaged.png`, and '
              '`kinstone_background_critical.png` serve all seven fusion variants, '
              'keeping the installed health decoration. The original fusion interface '
              'remains unchanged in the center.\n\n'
              'Critical means one heart or less remaining. Otherwise, damaged means '
              'more than one heart missing; normal means at most one heart missing. '
              'Selection follows the background installed when opening the menu. '
              'Each state has its own file; missing artwork uses black, never a '
              'different health state or the other menu family. Old unsuffixed '
              'menu_background.png and kinstone_background.png are not lookup names.\n\n'
              '`fast_travel_background.png` is the fixed blue fast-travel decoration, '
              'shared by all destinations, unlock masks and health values. It has no '
              'normal/damaged/critical variants. The original map, frame, markers and '
              'cursor stay native foreground and are excluded from this reference.\n\n'
              'The figurine viewer and the local fusion map reveal use generated live field colors. '
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
