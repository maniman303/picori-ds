#!/usr/bin/env python3
"""Compare native PNGs with the centered crop of a 16:9 replay (ImageMagick)."""
import argparse
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('native', type=Path)
parser.add_argument('wide', type=Path)
parser.add_argument('--min-frame', type=int, default=0)
parser.add_argument('--skip-frame', type=int, action='append', default=[])
args = parser.parse_args()
count = 0
for native in sorted(args.native.glob('*.png')):
    try:
        frame = int(native.stem)
    except ValueError:
        continue
    wide = args.wide / native.name
    if frame < args.min_frame or frame in args.skip_frame or not wide.is_file():
        continue
    size = subprocess.check_output(['identify', '-format', '%wx%h', str(wide)], text=True)
    if size != '285x160':
        raise SystemExit(f'{wide}: expected 285x160, got {size}')
    result = subprocess.run(['compare', '-metric', 'AE', '(', str(wide), '-crop', '240x160+22+0',
                             '+repage', ')', str(native), 'null:'], stdout=subprocess.DEVNULL,
                            stderr=subprocess.PIPE, text=True)
    if result.returncode:
        raise SystemExit(f'{wide}: native crop differs: {result.stderr.strip()}')
    count += 1
if not count:
    raise SystemExit('no matching PNGs')
print(f'{count} matching native crops')
