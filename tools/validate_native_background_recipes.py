#!/usr/bin/env python3
"""Check EU export recipes against retained native captures (requires ImageMagick).

Default paths are the local audit artifacts; never reads or writes artist PNGs.
"""
import argparse
import json
from pathlib import Path
import subprocess

from export_screen_backgrounds import Rom, recipes

ROOT = Path(__file__).resolve().parent.parent

def capture(directory, frame):
    path = directory / f'{frame:06d}.png'
    dimensions = subprocess.check_output(['identify', '-format', '%w %h', str(path)], text=True)
    if dimensions != '240 160':
        raise ValueError(f'{path}: expected a 240x160 capture, got {dimensions}')
    data = subprocess.check_output(['convert', str(path), '-depth', '8', 'rgb:-'])
    if len(data) != 240 * 160 * 3:
        raise ValueError(f'{path}: expected a 240x160 RGB capture')
    return data

def compare(name, expected, actual, y0, y1):
    begin, end = y0 * 240 * 3, y1 * 240 * 3
    if expected[begin:end] != actual[begin:end]:
        raise ValueError(f'{name}: native capture differs on rows {y0}..{y1 - 1}')
    return (y1 - y0) * 240

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--audit-root', type=Path, default=ROOT / '.tmp')
    args = parser.parse_args()
    rom = Rom(args.rom, 1)
    if rom.region != 'EU':
        parser.error('this capture fixture requires the EU ROM')
    # State1MenuType1 takes the installed image and BG2 offset from the
    # selected staff-roll command, not from the illustration resource table.
    expected_offsets = [0, 0xff88, 0xff88, 0, 0, 0xff88, 0xff88, 0, 0]
    for symbol in ('gUnk_081272F0', 'gUnk_08127644', 'gUnk_08127998'):
        commands = rom.symbol(symbol)
        installs = [(int.from_bytes(commands[i+6:i+8], 'little'),
                     int.from_bytes(commands[i+8:i+10], 'little'))
                    for i in range(0, len(commands) - 11, 12) if commands[i] == 1]
        if installs != list(enumerate(expected_offsets)):
            raise ValueError(f'{symbol}: staff-roll image offsets changed: {installs}')
    refs = {name: scene.render(layers, backdrop) for name, scene, layers, backdrop in recipes(rom)}
    counts = {}
    base = args.audit_root
    prologue = base / 'background-state-audit-eu-prologue' / 'prologue'
    # Rows below 96 can contain foreground story text and the page-6 wipe.
    for index, frame in enumerate((960, 1140, 1680, 2160, 3060, 3240), 1):
        name = f'prologue_{index:02}'
        counts[name] = compare(name, refs[name], capture(prologue, frame), 0, 96)
    counts['title_screen'] = compare('title_screen', refs['title_screen'], capture(prologue, 300), 120, 160)
    credits = base / 'background-state-audit-eu-en-v2' / 'credits'
    for index, frame in enumerate((960, 2426, 3004, 4494, 7202, 8160, 9576, 10214, 12480)):
        name = f'credits_{index + 1:02}' if index < 8 else 'ending_screen'
        counts[name] = compare(name, refs[name], capture(credits, frame), 0, 160)
    family = base / 'background-family-audit' / 'eu-en'
    events = json.loads((family / 'summary.json').read_text())['scenarios']['menus']['events']
    stems = {'plain': 'menu_background_', 'titled': 'menu_titled_background_',
             'pieces': 'kinstone_pieces_background_', 'techniques': 'sword_techniques_background_',
             'fusion': 'kinstone_background_'}
    for row in events:
        if row['event'] != 'menu_case_ready' or row['family'] not in stems:
            continue
        name = stems[row['family']] + ('normal', 'damaged', 'critical')[row['decoration']]
        if name in counts: continue
        y0 = 64 if row['family'] in ('pieces', 'techniques') else 159
        counts[name] = compare(name, refs[name], capture(family / 'menus', row['frame']), y0, 160)
    travel = base / 'background-travel-audit' / 'eu-en-fixed'
    events = json.loads((travel / 'summary.json').read_text())['scenarios']['travel']['events']
    for row in events:
        if row['event'] == 'travel_case_ready':
            counts['fast_travel_background'] = compare('fast_travel_background', refs['fast_travel_background'],
                                                        capture(travel / 'travel', row['frame']), 159, 160)
            break
    # File-select's palette animates: accept one coherent ROM-derived frame.
    from background_write_actions_audit import file_reference
    files = base / 'background-navigation-errors-audit' / 'eu-en-errors'
    events = json.loads((files / 'summary.json').read_text())['scenarios']['file_errors']['events']
    row = next(row for row in events if row['event'] == 'file_write_root_ready')
    actual = capture(files / 'file_errors', row['frame'])
    points = [159 * 240 + x for x in (*range(8), *range(232, 240))]
    pick = lambda image: b''.join(image[p*3:p*3+3] for p in points)
    if pick(actual) not in [pick(ref) for ref in file_reference(args.rom, 1)]:
        raise ValueError('file_select: native field does not match an animated ROM palette frame')
    counts['file_select'] = len(points)
    expected = set(refs) - {'map_hint'}
    if set(counts) != expected or len(counts) != 33:
        raise ValueError(f'recipe coverage mismatch: {expected - set(counts)}')
    print(f'PASS: {len(counts)} EU artwork recipes, {sum(counts.values())} native pixels')
    for name in sorted(counts): print(f'{name}: {counts[name]} pixels')

if __name__ == '__main__': main()
