#!/usr/bin/env python3
"""Profile-general viewport composition tests (no ROM or SDL)."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    with tempfile.TemporaryDirectory(prefix='tmc-viewport-compose-') as directory:
        for index, define in enumerate(('-DPORT_VIEWPORT_TEST_TALL',
                                        '-DPORT_VIEWPORT_RELEASE_TALL=1')):
            binary = Path(directory) / f'test{index}'
            command = shlex.split(os.environ.get('CC', 'cc')) + [
                '-std=c11', '-Wall', '-Wextra', '-Werror', define,
                '-I' + str(ROOT / 'port'),
                str(ROOT / 'tools/tests/viewport_composition.c'),
                str(ROOT / 'port/port_viewport.c'),
                '-o', str(binary)]
            subprocess.run(command, cwd=ROOT, check=True)
            subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == '__main__':
    main()
