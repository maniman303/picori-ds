#!/usr/bin/env python3
"""Exercise individual native entity screen bounds at 160 and 192 rows."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-entity-height-') as directory:
    tmp = Path(directory)
    flags = ['-std=gnu11', '-O1', '-g', '-ffunction-sections', '-fdata-sections', '-DPC_PORT',
             '-DMULTI_REGION', '-DUSA', '-DENGLISH', '-DMODE1_GBA_WIDTH=569', '-include', 'region.h',
             '-I.', '-Iinclude', '-Iport', '-Ibuild/USA', '-Iport/ppu/include']
    cc = shlex.split(os.environ.get('CC', 'cc'))
    objects = []
    for src in ('src/npc/guardWithSpear.c', 'src/npc/dampe.c', 'src/enemy/cuccoAggr.c', 'src/enemy/gyorgChild.c'):
        obj = tmp / (Path(src).stem + '.o')
        subprocess.run(cc + flags + ['-w', '-c', src, '-o', str(obj)], cwd=ROOT, check=True)
        objects.append(str(obj))
    binary = tmp / 'test'
    subprocess.run(cc + flags + ['tools/tests/entity_view_height.c'] + objects +
                   ['-Wl,--gc-sections', '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)
