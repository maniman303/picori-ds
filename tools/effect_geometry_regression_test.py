#!/usr/bin/env python3
"""Compare the canvas iris against the actual GBA engine circle generator."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    compiler = shlex.split(os.environ.get('CC', 'cc'))
    with tempfile.TemporaryDirectory(prefix='tmc-effect-geometry-') as directory:
        binary = Path(directory) / 'effect-geometry'
        native = Path(directory) / 'common.o'
        assets = Path(directory) / 'assets'
        assets.mkdir()
        (assets / 'map_offsets.h').write_text('/* supplied by port_offset_USA.h */\n')
        # The full decompiled file has unrelated pointer-width/redefinition
        # warnings; only its two native circle functions are linked by GC.
        common = compiler + ['-std=gnu11', '-O1', '-ffunction-sections', '-fdata-sections',
                             '-funsigned-char', '-DPC_PORT', '-DNON_MATCHING', '-DUSE_HDMA',
                             '-DUSA', '-DENGLISH', '-DMULTI_REGION', '-include', 'region.h',
                             '-Iinclude', '-Ilibs', '-Iport', '-I.', '-I' + directory, '-Iport/ppu/include']
        subprocess.run(common + ['-w', '-c', 'src/common.c', '-o', str(native)], cwd=ROOT, check=True)
        command = compiler + ['-std=gnu11', '-O1', '-Wall', '-Wextra', '-Werror', '-Iport',
                              'tools/tests/effect_geometry.c', 'port/port_effect_geometry.c', str(native),
                              '-Wl,--gc-sections', '-lm', '-o', str(binary)]
        subprocess.run(command, cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == '__main__':
    main()
