#!/usr/bin/env python3
"""Exercise the production cache and PNG loader with real files."""
import ctypes.util
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

from background_png_regression_test import png

ROOT = Path(__file__).resolve().parent.parent


def main():
    with tempfile.TemporaryDirectory(prefix='tmc-background-cache-') as tmp:
        root = Path(tmp)
        folder = root / 'override' / '169'
        folder.mkdir(parents=True)
        for name, red in [('menu_background_normal.png', 19),
                          ('menu_titled_background_normal.png', 33),
                          ('fast_travel_background.png', 55)]:
            (folder / name).write_bytes(png(285, 160, sample=bytes([red, 89, 167, 129])))
        (root / 'fixture.png').write_bytes(png(285, 160, sample=bytes([44, 89, 167, 129])))
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
            'port/port_viewport.c', *flags, '-o', str(binary)]
        subprocess.run(cmd, cwd=ROOT, check=True)
        env = os.environ.copy()
        env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
        result = subprocess.run([str(binary), str(root)], cwd=ROOT, env=env, check=True,
                                text=True, capture_output=True)
        assert result.stderr.count('[background] unable to load') == 3, result.stderr
        print(result.stdout, end='')


if __name__ == '__main__':
    main()
