#!/usr/bin/env python3
"""Run the production frame compositor with bounded synthetic raster inputs."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from widescreen_engine_regression_test import function

root = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-background-frame-') as tmp:
    tmp = Path(tmp)
    source = (root / 'port/port_ppu.cpp').read_text()
    (tmp / 'background_frame_function.inc').write_text(function(source, 'static bool Port_PPU_ComposeBackground(void)'))
    flags = ['-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
             '-DMODE1_GBA_WIDTH=569', '-Iport', '-Iport/ppu/include', '-I' + str(tmp)]
    cc, cxx = shlex.split(os.environ.get('CC', 'cc')), shlex.split(os.environ.get('CXX', 'c++'))
    objects = []
    for name in ('port_background_policy', 'port_background_effect', 'port_background_compose', 'port_title_cap'):
        obj = tmp / (name + '.o')
        subprocess.run(cc + ['-std=c11'] + flags + ['-c', f'port/{name}.c', '-o', str(obj)], cwd=root, check=True)
        objects.append(str(obj))
    binary = tmp / 'test'
    subprocess.run(cxx + ['-std=c++17'] + flags + ['tools/tests/background_frame_composition.cpp'] + objects +
                   ['-o', str(binary)], cwd=root, check=True)
    env = os.environ.copy(); env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(binary)], cwd=root, env=env, check=True)
    print('Production frame composition, 4:3 perimeter and fail-closed behavior: PASS')
