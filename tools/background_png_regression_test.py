#!/usr/bin/env python3
"""Exercise exact catalog/profile lookup and ordinary PNG decoding."""
import os
import ctypes.util
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile
import zlib
import sys

ROOT = Path(__file__).resolve().parent.parent
PROFILES = [(0, '32', 240, 160), (4, '43', 256, 192),
            (1, '169', 285, 160), (2, '219', 374, 160), (3, '329', 569, 160)]
FAMILIES = ['menu_background', 'menu_titled_background', 'kinstone_pieces_background',
            'sword_techniques_background', 'kinstone_background']
HEALTH = ['normal', 'damaged', 'critical']


def chunk(name, data):
    return struct.pack('>I', len(data)) + name + data + struct.pack('>I', zlib.crc32(name + data))


def png(width, height, color_type=6, depth=8, sample=b'\x13\x59\xa7\x81',
        extras=b'', interlace=False, packed=False):
    passes = [(0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4),
              (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2)] if interlace else [(0, 0, 1, 1)]
    raw = bytearray()
    for x, y, dx, dy in passes:
        pw, ph = len(range(x, width, dx)), len(range(y, height, dy))
        if pw and ph:
            raw.extend((b'\0' + sample * ((pw * depth + 7) // 8 if packed else pw)) * ph)
    header = struct.pack('>IIBBBBB', width, height, depth, color_type, 0, 0, int(interlace))
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', header) + extras + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')


def main():
    with tempfile.TemporaryDirectory(prefix='tmc-background-png-') as tmp:
        root = Path(tmp)
        binary = root / 'test'
        try:
            png_flags = shlex.split(subprocess.check_output(
                ['pkg-config', '--cflags', '--libs', 'libpng'], text=True, stderr=subprocess.DEVNULL))
        except (FileNotFoundError, subprocess.CalledProcessError):
            library = ctypes.util.find_library('png16')
            png_flags = ['-l:' + library] if sys.platform.startswith('linux') and library else ['-lpng']
        cmd = shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-Iport',
            'tools/tests/background_png.c', 'port/port_background_png.c',
            'port/port_screen_background.c', 'port/port_viewport.c',
            *png_flags, '-o', str(binary)]
        subprocess.run(cmd, cwd=ROOT, check=True)
        env = os.environ.copy()
        env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')

        def run(aspect, family=0, health=0, success=1, parity=0, color=(19, 89, 167, 129)):
            subprocess.run([str(binary), str(root), *map(str, (aspect, family, health, success, parity, *color))],
                           cwd=ROOT, env=env, check=True)

        for aspect, directory, width, height in PROFILES:
            folder = root / 'override' / directory
            folder.mkdir(parents=True)
            for family, name in enumerate(FAMILIES):
                for health, suffix in enumerate(HEALTH):
                    path = folder / f'{name}_{suffix}.png'
                    path.write_bytes(png(width, height))
                    run(aspect, family, health)
            (folder / 'fast_travel_background.png').write_bytes(png(width, height))
            run(aspect, -1)
            run(aspect, 0, 0, 0, 1)

        folder = root / 'override' / '169'
        path = folder / 'menu_titled_background_normal.png'
        # The old plain image exists but must never be used as a titled alias.
        path.unlink()
        run(1, 1, 0, 0)
        path.write_bytes(png(285, 160))
        (folder / 'menu_titled_background_damaged.png').unlink()
        run(1, 1, 1, 0)
        for width, height in [(284, 160), (285, 159), (285, 192), (374, 160), (9999, 9999)]:
            path.write_bytes(png(width, height) if width < 1000 else
                             b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)))
            run(1, 1, 0, 0)
        path.write_bytes(png(285, 160)[:-12])
        run(1, 1, 0, 0)
        corrupt = bytearray(png(285, 160))
        corrupt[-5] ^= 1
        path.write_bytes(corrupt)
        run(1, 1, 0, 0)
        path.write_bytes(b'not a png')
        run(1, 1, 0, 0)
        fixtures = [
            (png(285, 160, 2, sample=b'\x13\x59\xa7'), (19, 89, 167, 255)),
            (png(285, 160, 0, sample=b'\x59'), (89, 89, 89, 255)),
            (png(285, 160, 4, sample=b'\x59\x81'), (89, 89, 89, 129)),
            (png(285, 160, 3, sample=b'\0', extras=chunk(b'PLTE', b'\x13\x59\xa7') + chunk(b'tRNS', b'\x81')), (19, 89, 167, 129)),
            (png(285, 160, 6, 16, b'\x13\x00\x59\x00\xa7\x00\x81\x00'), (19, 89, 167, 129)),
            (png(285, 160, 0, 1, b'\xff', packed=True), (255, 255, 255, 255)),
            (png(285, 160, 3, 1, b'\0', extras=chunk(b'PLTE', b'\x13\x59\xa7') + chunk(b'tRNS', b'\x81'), packed=True), (19, 89, 167, 129)),
            (png(285, 160, interlace=True), (19, 89, 167, 129)),
        ]
        for data, color in fixtures:
            path.write_bytes(data)
            run(1, 1, 0, color=color)
        for aspect, directory, width in [(2, '219', 373), (3, '329', 568)]:
            path = root / 'override' / directory / 'menu_titled_background_normal.png'
            path.write_bytes(png(width, 160))
            run(aspect, 1, 0, 0)
        # Every selected profile must reject all four other valid sizes, as
        # well as a height error. Test the selected directory itself so a
        # missing-file failure cannot masquerade as a dimension check.
        for aspect, directory, width, height in PROFILES:
            path = root / 'override' / directory / 'menu_titled_background_normal.png'
            for other_aspect, _, other_width, other_height in PROFILES:
                if aspect != other_aspect:
                    path.write_bytes(png(other_width, other_height))
                    run(aspect, 1, 0, 0)
            path.write_bytes(png(width, height - 1))
            run(aspect, 1, 0, 0)
            path.write_bytes(png(width, height))
            run(aspect, 1, 0)
        print('Background PNG profiles, exact catalog names, formats and failure cleanup: PASS')


if __name__ == '__main__':
    main()
