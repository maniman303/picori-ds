#!/usr/bin/env python3
"""Live-world aperture composition tests for the software PPU (no ROM/SDL)."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    with tempfile.TemporaryDirectory(prefix='tmc-tall-compose-') as directory:
        binary = Path(directory) / 'test'
        command = shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Werror', '-fopenmp',
            '-DMODE1_GBA_WIDTH=384',
            '-I' + str(ROOT / 'port/ppu/include'),
            str(ROOT / 'tools/tests/tall_composition.c'),
            str(ROOT / 'port/ppu/src/mode1.c'),
            '-lm', '-o', str(binary)]
        subprocess.run(command, cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == '__main__':
    main()
