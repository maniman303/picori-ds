#!/usr/bin/env python3
"""ROM-free checks for the native background exporter's binary formats."""

from pathlib import Path
import struct
import sys
import unittest
import tempfile
from types import SimpleNamespace
from unittest.mock import patch
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from export_screen_backgrounds import (IMAGE_CONTEXTS, MENU_CONTEXTS, MENU_HEALTH_VARIANTS, SOLID_CONTEXTS, TITLED_MENU_CONTEXTS,
                                       Rom, Scene, main, menu_recipes, fast_travel_recipe, png_bytes, solid_bottom_center_reference)


class ExportFormats(unittest.TestCase):
    def test_fast_travel_does_not_alias_plain_health_art(self):
        self.assertNotIn('fast_travel_map', MENU_CONTEXTS)
        self.assertEqual(IMAGE_CONTEXTS['fast_travel_background'], ('fast_travel_map',))
        self.assertEqual([name for name in IMAGE_CONTEXTS if name.startswith('fast_travel')],
                         ['fast_travel_background'])
        for name, contexts in IMAGE_CONTEXTS.items():
            if name != 'fast_travel_background':
                self.assertFalse(any(context.startswith('fast_travel_map') for context in contexts))


    def test_fast_travel_exports_only_fixed_bg3_without_native_map_or_sprites(self):
        scene = SimpleNamespace(gfx=lambda _: None, pal=lambda _: None, upload=lambda *_: None)
        with patch('export_screen_backgrounds.Scene', return_value=scene), \
             patch.object(scene, 'gfx') as gfx, patch.object(scene, 'pal') as pal, \
             patch.object(scene, 'upload') as upload:
            value, layers, backdrop = fast_travel_recipe(None)
        self.assertIs(value, scene)
        self.assertEqual([call.args for call in gfx.call_args_list], [(86,), (93,), (129,)])
        self.assertEqual([call.args for call in pal.call_args_list], [(181,), (185,)])
        self.assertEqual([call.args for call in upload.call_args_list], [(3, 0x1e0b)])
        self.assertEqual(layers, [(3, 0x1e0b, 0, 0)])
        self.assertIsNone(backdrop)

    def test_add_missing_preserves_artist_pngs_and_refreshes_generated_index(self):
        pixels = bytes([248, 0, 0, 0, 248, 0]) * (240*160//2)
        scene = SimpleNamespace(render=lambda *_: pixels)
        exports = [('title_screen', scene, [], None), ('file_select', scene, [], None)]
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / '32'
            output.mkdir()
            artist = output / 'title_screen.png'
            artist.write_bytes(b'artist bytes must survive')
            (output / 'README.md').write_text('old generated index')
            args = ['export', '--rom', 'unused.gba', '--output', str(output), '--add-missing']
            with patch('export_screen_backgrounds.Rom', return_value=SimpleNamespace(region='EU')), \
                 patch('export_screen_backgrounds.recipes', return_value=exports), \
                 patch.object(sys, 'argv', args), patch('builtins.print'):
                main()
                new_png = (output / 'file_select.png').read_bytes()
                main()
            self.assertEqual(artist.read_bytes(), b'artist bytes must survive')
            self.assertTrue(new_png.startswith(b'\x89PNG'))
            self.assertEqual((output / 'file_select.png').read_bytes(), new_png)
            self.assertIn('menu_titled_background', (output / 'README.md').read_text())
            self.assertTrue((output.parent / 'background_contact_sheet.png').exists())

    def test_blank_title_removes_letters_preserving_border_and_transparency(self):
        scene = Scene(None)
        # First OBJ tile (64), row zero: native border index 10.
        scene.vram[0x10800] = 0xaa
        # Same tile, row five: two foreground letter/shadow pixels (index 6).
        scene.vram[0x10814] = 0x66
        struct.pack_into('<H', scene.palette, (256 + 80 + 10) * 2, 31 << 5)
        struct.pack_into('<H', scene.palette, (256 + 80 + 12) * 2, 31)
        scene.blank_menu_heading()
        pixels = scene.render([])
        self.assertEqual(pixels[:3], bytes([0, 248, 0]))
        self.assertEqual(pixels[(5*240)*3:(5*240)*3+6], bytes([248, 0, 0]*2))
        self.assertEqual(pixels[(5*240+2)*3:(5*240+2)*3+3], bytes([0, 0, 0]))

    def test_local_hint_has_no_image_lookup(self):
        self.assertIn('local_map_hint', SOLID_CONTEXTS)
        self.assertNotIn('local_map_hint', MENU_CONTEXTS)
        for contexts in IMAGE_CONTEXTS.values():
            self.assertFalse(any(context.startswith('local_map_hint') for context in contexts))

    def test_menu_health_recipes(self):
        class RecordingScene:
            def __init__(self, rom):
                self.groups, self.palettes, self.uploads = [], [], []
                self.heading = False

            def blank_menu_heading(self):
                self.heading = True

            def gfx(self, group):
                self.groups.append(group)

            def pal(self, group):
                self.palettes.append(group)

            def upload(self, bg, control):
                self.uploads.append((bg, control))

        with patch('export_screen_backgrounds.Scene', RecordingScene):
            exports = list(menu_recipes(None))
        self.assertEqual(MENU_HEALTH_VARIANTS, ('normal', 'damaged', 'critical'))
        expected = {
            'menu_background_normal': [86], 'menu_background_damaged': [87],
            'menu_background_critical': [88], 'kinstone_background_normal': [86,117,118],
            'kinstone_background_damaged': [87,117,118], 'kinstone_background_critical': [88,117,118],
        }
        for stem, group in (('menu_titled_background',90), ('kinstone_pieces_background',125), ('sword_techniques_background',126)):
            for i, health in enumerate(MENU_HEALTH_VARIANTS):
                expected[f'{stem}_{health}'] = [86+i, group]
        self.assertEqual({name for name, *_ in exports}, set(expected))
        for name, scene, layers, backdrop in exports:
            self.assertEqual(scene.groups, expected[name])
            fusion = name.startswith('kinstone_background_')
            titled = name.startswith('menu_titled_')
            pieces = name.startswith('kinstone_pieces_')
            techniques = name.startswith('sword_techniques_')
            palette = 203 if fusion else 182 if titled else 204 if pieces else 205 if techniques else None
            self.assertEqual(scene.palettes, [181,palette] if palette else [181])
            self.assertEqual(scene.heading, titled)
            self.assertEqual(scene.uploads, [(3,0x1e0b), (2,0x1d03)] if pieces or techniques else
                             [(1,0x1c01),(2,0x1d02),(3,0x1e0b)] if fusion else [(3,0x1e0b)])
            self.assertEqual(layers, [(1,0x1c01,0,0), (2,0x1d02,0,0), (3,0x1e0b,0,0)] if fusion else
                             [(2,0x1d03,0,0), (3,0x1e0b,0,0)] if pieces or techniques else [(3,0x1e0b,0,0)])
            self.assertIsNone(backdrop)
        self.assertEqual(len(IMAGE_CONTEXTS), 33)
        self.assertNotIn('menu_background', IMAGE_CONTEXTS)
        self.assertNotIn('kinstone_background', IMAGE_CONTEXTS)
        for health in MENU_HEALTH_VARIANTS:
            self.assertEqual(IMAGE_CONTEXTS[f'menu_background_{health}'],
                             tuple(f'{context}_{health}' for context in MENU_CONTEXTS))
            self.assertEqual(IMAGE_CONTEXTS[f'menu_titled_background_{health}'],
                             tuple(f'{context}_{health}' for context in TITLED_MENU_CONTEXTS))
            self.assertEqual(IMAGE_CONTEXTS[f'kinstone_pieces_background_{health}'], (f'pause_kinstone_pieces_{health}',))
            self.assertEqual(IMAGE_CONTEXTS[f'sword_techniques_background_{health}'], (f'pause_sword_techniques_{health}',))
            self.assertEqual(IMAGE_CONTEXTS[f'kinstone_background_{health}'],
                             tuple(f'kinstone_fusion_{i:02d}_{health}' for i in range(1,8)))

    def test_bottom_center_reference(self):
        # Distinguish the requested border pixel from the center and corners.
        pixels = bytearray([1, 2, 3] * (240*160))
        offset = (159*240+120)*3
        pixels[offset:offset+3] = bytes([200, 224, 144])
        self.assertEqual(solid_bottom_center_reference(pixels), bytes([200, 224, 144])*(240*160))
        with self.assertRaises(ValueError):
            solid_bottom_center_reference(b'\0'*3)

    def test_tile_addressing(self):
        scene = Scene(None)
        scene.vram[0xe000:0xe002] = struct.pack('<H', 1 | (2 << 12))
        scene.vram[32] = 0x21
        self.assertEqual(scene.sample(0x1c00, 0, 0), 33)
        self.assertEqual(scene.sample(0x1c00, 1, 0), 34)
        scene.vram[0xe000:0xe002] = struct.pack('<H', 1 | 0x400 | (2 << 12))
        self.assertEqual(scene.sample(0x1c00, 6, 0), 34)
        self.assertEqual(scene.sample(0x1c00, 7, 0), 33)
        scene.vram[0xe000:0xe002] = struct.pack('<H', 1 | 0x800 | (2 << 12))
        self.assertEqual(scene.sample(0x1c00, 0, 7), 33)
        scene.vram[0xe000:0xe002] = struct.pack('<H', 2)
        scene.vram[128] = 7
        self.assertEqual(scene.sample(0x1c80, 0, 0), 7)
        scene.vram[0xe800:0xe802] = struct.pack('<H', 1)
        self.assertEqual(scene.sample(0x5c00, 256, 0), 1)
        self.assertIsNone(scene.sample(0x1c00, 2, 0))

    def test_lz77_backreference(self):
        rom = object.__new__(Rom)
        rom.data = bytes.fromhex('10060000104142430002')
        self.assertEqual(rom.lz77(0), b'ABCABC')
        # Copy distance 1 must support overlapping output (AAAAAA).
        rom.data = bytes.fromhex('1006000040412000')
        self.assertEqual(rom.lz77(0), b'AAAAAA')

    def test_invalid_compression(self):
        rom = object.__new__(Rom)
        for blob in ['10030000800000', '100300000041', '11000000']:
            rom.data = bytes.fromhex(blob)
            with self.assertRaises(ValueError):
                rom.lz77(0)

    def test_png_pixels_and_checksums(self):
        pixels = bytes([255, 0, 0, 0, 255, 0, 0, 0, 255, 1, 2, 3])
        png = png_bytes(pixels, 2, 2)
        self.assertEqual(png[:8], b'\x89PNG\r\n\x1a\n')
        pos, image_data, kinds = 8, bytearray(), []
        while pos < len(png):
            size = struct.unpack_from('>I', png, pos)[0]
            kind, data = png[pos+4:pos+8], png[pos+8:pos+8+size]
            crc = struct.unpack_from('>I', png, pos+8+size)[0]
            self.assertEqual(crc, zlib.crc32(kind+data) & 0xffffffff)
            kinds.append(kind)
            if kind == b'IHDR':
                self.assertEqual(struct.unpack('>2I5B', data), (2, 2, 8, 2, 0, 0, 0))
            if kind == b'IDAT':
                image_data.extend(data)
            pos += size+12
        self.assertEqual(kinds, [b'IHDR', b'IDAT', b'IEND'])
        self.assertEqual(zlib.decompress(image_data), b'\0'+pixels[:6]+b'\0'+pixels[6:])


if __name__ == '__main__':
    unittest.main()
