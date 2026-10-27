#!/usr/bin/env python3
"""Check actual hardware OAM writes against signed-position lifetime."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-oam-raw-') as directory:
    binary = Path(directory) / 'test'
    cmd = shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=gnu11', '-O1', '-g', '-ffunction-sections', '-fdata-sections',
        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
        '-DPC_PORT', '-DUSA', '-I.', '-Iinclude', '-Iport', '-Iport/ppu/include',
        'tools/tests/oam_raw_write.c', 'port/port_gba_mem.c', 'port/port_oam.c',
        '-Wl,--gc-sections', '-o', str(binary)]
    subprocess.run(cmd, cwd=ROOT, check=True)
    env = os.environ.copy(); env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(binary)], cwd=ROOT, env=env, check=True)
