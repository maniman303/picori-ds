#!/usr/bin/env python3
"""Verify the retained EU fast-travel artwork/fallback and gameplay captures."""
import argparse
import json
from pathlib import Path
from PIL import Image, ImageChops

parser = argparse.ArgumentParser()
parser.add_argument('directory', type=Path)
args = parser.parse_args()
art = args.directory / 'travel-art-final2'
missing = args.directory / 'travel-missing'
for folder in (art, missing):
    rows = [json.loads(line) for line in (folder / 'trace.jsonl').read_text().splitlines()]
    events = {(r['event'], r['frame']) for r in rows if 'event' in r}
    assert {('travel_case_ready', 659), ('travel_case_held', 675),
            ('fast_travel_cancel', 680), ('travel_case_restored', 697)} <= events
    for frame in (659, 675):
        row = next(r for r in rows if r['frame'] == frame and 'event' not in r)
        assert row['background_context'] == 41 and row['travel_field'] == 1
    row = next(r for r in rows if r['frame'] == 699 and 'event' not in r)
    assert row['background_context'] == 1 and row['travel_field'] == 0
for frame in (632, 659, 660, 675, 682):
    with Image.open(art / f'{frame:06}.png') as ai, Image.open(missing / f'{frame:06}.png') as bi:
        a, b = ai.convert('RGB'), bi.convert('RGB')
        assert a.size == b.size == (285, 160)
        assert ImageChops.difference(a.crop((22, 0, 262, 160)), b.crop((22, 0, 262, 160))).getbbox() is None
        assert a.getpixel((0, 100)) != b.getpixel((0, 100))
for frame in (699, 704):
    with Image.open(art / f'{frame:06}.png') as ai, Image.open(missing / f'{frame:06}.png') as bi:
        assert ImageChops.difference(ai.convert('RGB'), bi.convert('RGB')).getbbox() is None
print('Fast travel artwork, fallback, native crop and gameplay restoration: PASS')
