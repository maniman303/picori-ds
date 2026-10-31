#!/usr/bin/env python3
"""Minish Village group-grid policy tests (no ROM or SDL)."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    with tempfile.TemporaryDirectory(prefix='tmc-village-groups-') as directory:
        binary = Path(directory) / 'village-groups'
        command = shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
            '-DPC_PORT', '-DMULTI_REGION', '-DUSA', '-DENGLISH',
            '-include', 'region.h',
            '-I' + str(ROOT), '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'port'),
            '-I' + str(ROOT / 'port/ppu/include'), '-I' + str(ROOT / 'build/USA'),
            '-ffunction-sections', '-fdata-sections',
            str(ROOT / 'tools/tests/village_groups.c'),
            str(ROOT / 'port/port_village_groups.c'),
            '-Wl,--gc-sections', '-o', str(binary),
        ]
        subprocess.run(command, cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == '__main__':
    main()
