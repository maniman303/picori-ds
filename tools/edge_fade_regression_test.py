#!/usr/bin/env python3
"""ROM-free live-gameplay margin fade regression."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    flags = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
             '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
             '-I' + str(ROOT / 'port')]
    with tempfile.TemporaryDirectory(prefix='tmc-edge-fade-') as directory:
        binary = Path(directory) / 'test'
        subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + flags + [
            str(ROOT / 'tools/tests/edge_fade.c'),
            str(ROOT / 'port/port_edge_fade.c'), '-o', str(binary)], check=True)
        env = os.environ.copy()
        env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
        subprocess.run([str(binary)], check=True, env=env)


if __name__ == '__main__':
    main()
