#!/usr/bin/env python3
"""Check production catalog/exporter agreement and geometry without SDL or a ROM."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from export_screen_backgrounds import COLOR_REFERENCES, IMAGE_CONTEXTS

ROOT = Path(__file__).resolve().parent.parent


def main():
    cc = shlex.split(os.environ.get('CC', 'cc'))
    cxx = shlex.split(os.environ.get('CXX', 'c++'))
    flags = ['-Wall', '-Wextra', '-Werror', '-pedantic', '-I' + str(ROOT / 'port'),
             '-fsanitize=undefined', '-fno-sanitize-recover=all']
    with tempfile.TemporaryDirectory(prefix='tmc-background-test-') as directory:
        temporary = Path(directory)
        objects = []
        for name in ('port_screen_background', 'port_viewport'):
            output = temporary / (name + '.o')
            subprocess.run(cc + ['-std=c11'] + flags + ['-c', str(ROOT / 'port' / (name + '.c')),
                                                       '-o', str(output)], check=True)
            objects.append(str(output))
        # Compile the same consumer as C and C++ and link to C objects, verifying
        # the actual public C ABI used by both renderer and engine integration.
        for compiler, language, standard in ((cc, 'c', 'c11'), (cxx, 'c++', 'c++17')):
            binary = temporary / ('test-' + language)
            consumer = temporary / ('consumer-' + language + '.o')
            subprocess.run(compiler + ['-std=' + standard, '-x', language] + flags + [
                '-c', str(ROOT / 'tools/tests/screen_background.c'), '-o', str(consumer)], check=True)
            subprocess.run(compiler + flags + [str(consumer)] + objects + ['-o', str(binary)], check=True)
            names = subprocess.check_output([str(binary)], text=True).splitlines()
            expected = {name + '.png' for name in IMAGE_CONTEXTS}
            assert len(names) == len(expected) == 19, names
            assert set(names) == expected, (set(names), expected)
            assert not set(names) & {name + '.png' for name in COLOR_REFERENCES}
    print('PASS: background catalog, exporter agreement, profile geometry, C/C++ ABI')


if __name__ == '__main__':
    main()
