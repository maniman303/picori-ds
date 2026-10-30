#!/usr/bin/env python3
"""Exercise profile-general tall-scene policy (no ROM or SDL build)."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    with tempfile.TemporaryDirectory(prefix='tmc-tall-scene-') as directory:
        binary = Path(directory) / 'test'
        command = shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c11', '-Wall', '-Wextra', '-Werror',
            '-I' + str(ROOT / 'port'),
            str(ROOT / 'tools/tests/tall_scene.c'),
            str(ROOT / 'port/port_viewport.c'),
            str(ROOT / 'port/port_tall_scene.c'),
            '-o', str(binary)]
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
