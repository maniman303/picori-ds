#!/usr/bin/env python3
"""Exercise composed margins through the production display post-process."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from widescreen_engine_regression_test import function

ROOT = Path(__file__).resolve().parent.parent
source = (ROOT / 'port/port_ppu.cpp').read_text()
functions = [function(source, signature) for signature in (
    'static const uint32_t* Port_PPU_SelectPresentFrame(',
    'extern "C" void Port_PPU_SetPersistence(',
    'extern "C" void Port_PPU_ColorCorrectBuffer(',
    'static void Port_PPU_ApplyDisplayPostProcess(',
)]
with tempfile.TemporaryDirectory(prefix='tmc-composed-postprocess-') as directory:
    tmp = Path(directory)
    (tmp / 'postprocess_functions.inc').write_text('\n'.join(functions))
    flags = ['-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
             '-fno-sanitize-recover=all', '-I' + str(ROOT / 'port')]
    objects = []
    for name in ('port_background_compose', 'port_background_effect', 'port_title_cap'):
        obj = tmp / (name + '.o')
        subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + ['-std=c11'] + flags +
                       ['-c', str(ROOT / 'port' / (name + '.c')), '-o', str(obj)], check=True)
        objects.append(str(obj))
    binary = tmp / 'test'
    subprocess.run(shlex.split(os.environ.get('CXX', 'c++')) + ['-std=c++17'] + flags +
                   ['-I' + directory, str(ROOT / 'tools/tests/presentation_postprocess.cpp')] +
                   objects + ['-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
