#!/usr/bin/env python3
"""Verify retained EU title-cap artwork/fallback captures."""
import argparse
from pathlib import Path
from PIL import Image, ImageChops

parser = argparse.ArgumentParser()
parser.add_argument('directory', type=Path)
args = parser.parse_args()
checks = [('captures', 'captures-missing', (608, 660, 688)),
          ('captures-fusion', 'captures-fusion-missing', (608, 616, 624, 632, 660, 666, 676, 685, 688))]
for art_dir, fallback_dir, frames in checks:
    for frame in frames:
        name = f'{frame:06}.png'
        with Image.open(args.directory / art_dir / name) as a, Image.open(args.directory / fallback_dir / name) as b:
            a, b = a.convert('RGB'), b.convert('RGB')
            assert a.size == b.size == (285, 160), (art_dir, frame, a.size, b.size)
            assert ImageChops.difference(a.crop((22, 0, 262, 160)), b.crop((22, 0, 262, 160))).getbbox() is None, (art_dir, frame, 'native crop')
            assert a.getpixel((14, 10)) == b.getpixel((14, 10)), (art_dir, frame, 'cap')
            assert a.getpixel((0, 100)) != b.getpixel((0, 100)), (art_dir, frame, 'artwork')
print('EU menu/fusion cap, native crop and fade checkpoints: PASS')
