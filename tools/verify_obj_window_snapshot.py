#!/usr/bin/env python3
"""Render the OBJ-window mask from a PPU2 snapshot using the real PPU."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('snapshot', type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-obj-window-') as tmp:
    binary = Path(tmp) / 'verify'
    subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=c11', '-O2', '-Wno-unknown-pragmas', '-DMODE1_GBA_WIDTH=569', '-Iport/ppu/include',
        'tools/verify_obj_window_snapshot.c', 'port/ppu/src/mode1.c', 'port/ppu/src/virtuappu.c',
        '-lm', '-o', str(binary)], cwd=root, check=True)
    subprocess.run([str(binary), str(a.snapshot.resolve())], cwd=root, check=True)
