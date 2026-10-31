#!/usr/bin/env python3
"""Per-cell charset group selection in the production software PPU."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    with tempfile.TemporaryDirectory(prefix='tmc-ppu-group-cells-') as directory:
        binary = Path(directory) / 'ppu-group-cells'
        command = shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unknown-pragmas',
            '-DMODE1_GBA_WIDTH=569',
            '-I' + str(ROOT / 'port/ppu/include'),
            str(ROOT / 'tools/tests/ppu_group_cells.c'),
            str(ROOT / 'port/ppu/src/mode1.c'),
            '-lm', '-o', str(binary),
        ]
        subprocess.run(command, cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == '__main__':
    main()
