#!/usr/bin/env python3
"""Check boundary publication using the actual bounded PNG cache."""
import ctypes.util
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import sys
from background_png_regression_test import png

root = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-background-runtime-') as tmp:
    fixture = Path(tmp)
    folder = fixture / 'override' / '169'
    folder.mkdir(parents=True)
    (folder / 'fast_travel_background.png').write_bytes(png(285, 160, sample=bytes([55, 89, 167, 129])))
    (folder / 'menu_background_normal.png').write_bytes(png(285, 160, sample=bytes([11, 22, 33, 255])))
    wide = fixture / 'override' / '219'
    wide.mkdir()
    (wide / 'menu_background_normal.png').write_bytes(png(374, 160, sample=bytes([77, 22, 33, 255])))
    binary = fixture / 'test'
    try:
        flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'libpng'], text=True, stderr=subprocess.DEVNULL))
    except (FileNotFoundError, subprocess.CalledProcessError):
        library = ctypes.util.find_library('png16')
        flags = ['-l:' + library] if sys.platform.startswith('linux') and library else ['-lpng']
    subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=c11', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-Iport',
        'tools/tests/background_runtime.c', 'port/port_background_runtime.c',
        'port/port_background_cache.c', 'port/port_background_png.c',
        'port/port_screen_background.c', 'port/port_viewport.c', 'port/port_background_effect.c',
        'port/port_background_policy.c', *flags, '-o', str(binary)],
        cwd=root, check=True)
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(binary), str(fixture)], cwd=root, env=env, check=True)
