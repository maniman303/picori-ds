#!/usr/bin/env python3
"""Title-screen live layers extend over fixed-screen artwork margins."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    with tempfile.TemporaryDirectory(prefix='tmc-title-overlay-') as directory:
        binary = Path(directory) / 'test'
        command = shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Werror', '-fopenmp',
            '-DMODE1_GBA_WIDTH=576',
            '-I' + str(ROOT / 'port/ppu/include'),
            str(ROOT / 'tools/tests/title_overlay.c'),
            str(ROOT / 'port/ppu/src/mode1.c'),
            '-lm', '-o', str(binary)]
        subprocess.run(command, cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == '__main__':
    main()
