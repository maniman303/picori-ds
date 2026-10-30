#!/usr/bin/env python3
"""Exercise the production cache and PNG loader with real files."""
import ctypes.util
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

from background_png_regression_test import FAMILIES, HEALTH, png

ROOT = Path(__file__).resolve().parent.parent


def main():
    with tempfile.TemporaryDirectory(prefix='tmc-background-cache-') as tmp:
        root = Path(tmp)
        folder = root / 'override' / '169'
        folder.mkdir(parents=True)
        mod_folder = root / 'mods' / 'art-impostor' / 'override' / '169'
        mod_folder.mkdir(parents=True)
        for family, name in enumerate(FAMILIES):
            for health, suffix in enumerate(HEALTH):
                red = 19 + family * 14 + health * 3
                (folder / f'{name}_{suffix}.png').write_bytes(png(285, 160, sample=bytes([red, 89, 167, 129])))
                (mod_folder / f'{name}_{suffix}.png').write_bytes(png(285, 160, sample=bytes([200, 1, 2, 255])))
        (folder / 'fast_travel_background.png').write_bytes(png(285, 160, sample=bytes([55, 89, 167, 129])))
        folder43 = root / 'override' / '43'
        folder43.mkdir(parents=True)
        for family, name in enumerate(FAMILIES):
            for health, suffix in enumerate(HEALTH):
                red = 60 + family * 11 + health * 2
                (folder43 / f'{name}_{suffix}.png').write_bytes(png(256, 192, sample=bytes([red, 89, 167, 129])))
        (folder43 / 'fast_travel_background.png').write_bytes(png(256, 192, sample=bytes([66, 89, 167, 129])))
        wide = root / 'override' / '219'
        wide.mkdir()
        (wide / 'menu_background_normal.png').write_bytes(png(374, 160, sample=bytes([76, 89, 167, 129])))
        (root / 'fixture.png').write_bytes(png(285, 160, sample=bytes([44, 89, 167, 129])))
        (root / 'edited.png').write_bytes(png(285, 160, sample=bytes([88, 17, 23, 128])))
        (root / 'corrupt.png').write_bytes(b'broken png')
        try:
            flags = shlex.split(subprocess.check_output(
                ['pkg-config', '--cflags', '--libs', 'libpng'], text=True, stderr=subprocess.DEVNULL))
        except (FileNotFoundError, subprocess.CalledProcessError):
            library = ctypes.util.find_library('png16')
            flags = ['-l:' + library] if sys.platform.startswith('linux') and library else ['-lpng']
        binary = root / 'test'
        cmd = shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-Iport',
            'tools/tests/background_cache.c', 'port/port_background_cache.c',
            'port/port_background_png.c', 'port/port_screen_background.c',
            'port/port_viewport.c', 'port/port_title_cap.c', *flags, '-o', str(binary)]
        subprocess.run(cmd, cwd=ROOT, check=True)
        env = os.environ.copy()
        env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
        env['TMC_MODS'] = 'art-impostor'
        result = subprocess.run([str(binary), str(root)], cwd=ROOT, env=env,
                                text=True, capture_output=True)
        if result.returncode != 0:
            print(result.stdout, end='')
            print(result.stderr, end='')
            result.check_returncode()
        assert result.stderr.count('override/43/') >= 2, result.stderr
        assert result.stderr.count('[background] unable to load') >= 8, result.stderr
        print(result.stdout, end='')


if __name__ == '__main__':
    main()
