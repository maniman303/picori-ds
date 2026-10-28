#!/usr/bin/env python3
"""Exercise title-cap ROM source validation, geometry and OBJ effects."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-title-cap-') as directory:
    binary = Path(directory) / 'title-cap'
    cmd = shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
        '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-Iport',
        'tools/tests/title_cap.c', 'port/port_title_cap.c', '-o', str(binary)]
    subprocess.run(cmd, cwd=root, check=True)
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(binary)], cwd=root, env=env, check=True)
