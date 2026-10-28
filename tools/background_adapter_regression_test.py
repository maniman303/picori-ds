#!/usr/bin/env python3
"""Build the ROM-free installed-background publication regression."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='picori-background-adapter-') as directory:
    binary = Path(directory) / 'test'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-Iport', 'tools/tests/background_adapter.c', 'port/port_screen_background.c',
                    '-o', str(binary)], cwd=root, check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: background adapter publication')
