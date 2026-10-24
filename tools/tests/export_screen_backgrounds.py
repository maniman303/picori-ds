#!/usr/bin/env python3
"""ROM-free checks for the native background exporter's binary formats."""

from pathlib import Path
import struct
import sys
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from export_screen_backgrounds import Rom, Scene, png_bytes, solid_bottom_center_reference


class ExportFormats(unittest.TestCase):
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
