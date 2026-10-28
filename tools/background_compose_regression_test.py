#!/usr/bin/env python3
"""Exercise explicit margin coverage and resolved effect inputs."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-background-compose-') as temp:
    binary = Path(temp) / 'test'
    subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=c11', '-Wall', '-Wextra', '-Werror', '-Iport',
        'tools/tests/background_compose.c', 'port/port_background_compose.c',
        'port/port_background_effect.c', '-o', str(binary)], cwd=root, check=True)
    subprocess.run([str(binary)], cwd=root, check=True)
