#!/usr/bin/env python3
"""Validate the shipped object-override PNG set.

Checks that every shared object frame exists, is a valid PNG, and has the
expected dimensions. This is a static asset check; runtime compositing is
exercised by the game captures.
"""

import argparse
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_DIR = ROOT / 'assets' / 'override' / 'objects'

# object name -> expected (width, height)
EXPECTED = {
    'file_select_header': (164, 24),
}


def png_size(path):
    data = path.read_bytes()
    if data[:8] != b'\x89PNG\r\n\x1a\n' or data[12:16] != b'IHDR':
        raise ValueError('not a PNG')
    return struct.unpack_from('>II', data, 16)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dir', type=Path, default=DEFAULT_DIR,
                        help=f'override objects directory (default: {DEFAULT_DIR})')
    args = parser.parse_args()

    failed = False
    for name, expected in EXPECTED.items():
        path = args.dir / f'{name}.png'
        if not path.is_file():
            print(f'MISSING {path}')
            failed = True
            continue
        try:
            size = png_size(path)
        except (OSError, ValueError) as error:
            print(f'INVALID {path}: {error}')
            failed = True
            continue
        if size != expected:
            print(f'WRONG SIZE {path}: {size[0]}x{size[1]} (expected {expected[0]}x{expected[1]})')
            failed = True
            continue
        print(f'OK {path.relative_to(ROOT)} ({size[0]}x{size[1]})')
    return 1 if failed else 0


if __name__ == '__main__':
    raise SystemExit(main())
