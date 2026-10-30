#!/usr/bin/env python3
"""Verify fast-travel artwork, fallback, native crop and both presenters.

The capture matrix is produced from the EU fast-travel audit route at frame
675 (the held travel case).  Each profile is covered by software and GPU
presenter runs, with the fixed side artwork present and absent.
"""
import argparse
import json
import subprocess
from pathlib import Path

PROFILES = {
    'native': {'width': 240, 'aspect': 'native', 'override': 32},
    '16:9': {'width': 285, 'aspect': '16:9', 'override': 169},
    '21:9': {'width': 374, 'aspect': '21:9', 'override': 219},
}

RUNS = {
    'native-art': ('native', 'software', 'art'),
    'native-gpu-art': ('native', 'gpu', 'art'),
    'native-missing': ('native', 'software', 'missing'),
    'native-gpu-missing': ('native', 'gpu', 'missing'),
    'sw169-art': ('16:9', 'software', 'art'),
    'gpu169-art': ('16:9', 'gpu', 'art'),
    'sw169-missing': ('16:9', 'software', 'missing'),
    'gpu169-missing': ('16:9', 'gpu', 'missing'),
    'sw219-art': ('21:9', 'software', 'art'),
    'gpu219-art': ('21:9', 'gpu', 'art'),
    'sw219-missing': ('21:9', 'software', 'missing'),
    'gpu219-missing': ('21:9', 'gpu', 'missing'),
}

EXPECTED_CONFIG = {
    'window_scale': 4,
    'internal_scale': 1,
    'upscale_method': 'nearest',
    'color_correction': False,
    'lcd_persistence': False,
    'console_parity': False,
    'skip_launcher': True,
    'vsync': False,
}

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('directory', type=Path)
args = parser.parse_args()
root = args.directory


def rgb(name):
    """Return the full RGB presentation image as packed bytes."""
    profile = RUNS[name][0]
    width = PROFILES[profile]['width'] * 4
    path = root / name / 'presentation.png'
    size = subprocess.check_output(
        ['identify', '-format', '%wx%h', str(path)], text=True)
    assert size == f'{width}x640', (name, size)
    data = subprocess.check_output(
        ['convert', str(path), '-depth', '8', 'rgb:-'])
    assert len(data) == width * 640 * 3, (name, len(data))
    return data


def pixel(data, width, x, y):
    index = (y * width + x) * 3
    return data[index:index + 3]


def check_config(name):
    profile, backend, _ = RUNS[name]
    expected = dict(EXPECTED_CONFIG)
    expected['aspect_mode'] = PROFILES[profile]['aspect']
    expected['render_backend'] = backend
    config = json.loads((root / name / 'config.json').read_text())
    for key, value in expected.items():
        assert config.get(key) == value, (name, key, config.get(key), value)


def check_log(name):
    profile, backend, state = RUNS[name]
    override = PROFILES[profile]['override']
    log = (root / name / 'run.log').read_text()
    if backend == 'gpu':
        assert 'PPU initialized with SDL_GPU backend' in log, name
    else:
        assert 'PPU initialized with SDL renderer backend' in log, name
    assert '[capture] frame 675' in log and '(ok=1)' in log, name

    failures = [
        line for line in log.splitlines()
        if '[background] unable to load' in line
        and line.endswith('/fast_travel_background.png')
    ]
    if profile == 'native':
        # Native presentation has no side margin, so it must not request the
        # fixed fast-travel side image even when one is staged.
        assert not failures, (name, failures)
    elif state == 'missing':
        expected = f'/override/{override}/fast_travel_background.png'
        assert len(failures) == 1 and failures[0].endswith(expected), (
            name, failures)
    else:
        assert not failures, (name, failures)


def check_trace(name):
    rows = [
        json.loads(line)
        for line in (root / name / 'audit/trace.jsonl').read_text().splitlines()
        if line.strip()
    ]
    events = {(row.get('event'), row.get('frame')) for row in rows
              if 'event' in row}
    for event, frame in (('travel_case_ready', 659),
                         ('travel_case_held', 675)):
        assert (event, frame) in events, (name, event, frame)
    for frame in (659, 675):
        state = next(row for row in rows
                     if row.get('frame') == frame and 'event' not in row)
        assert (state['case'], state['decoration'],
                state['travel_field'], state['background_context']) == (
                    -1, 0, 1, 41), (name, frame, state)


images = {name: rgb(name) for name in RUNS}
for name in RUNS:
    check_config(name)
    check_log(name)
    check_trace(name)

# Software and GPU presentation must agree for every profile and artwork state.
for profile in PROFILES:
    for state in ('art', 'missing'):
        names = [name for name, run in RUNS.items()
                 if run[0] == profile and run[2] == state]
        assert len(names) == 2, (profile, state, names)
        software, gpu = images[names[0]], images[names[1]]
        assert software == gpu, (profile, state, 'presenter mismatch')

# Native has no side margin, so artwork and fallback must be identical.
for name in ('native-art', 'native-gpu-art'):
    assert images[name] == images['native-missing'] == images[
        'native-gpu-missing'], name

# Centered native crops use native-pixel floor centering, then 4x scaling.
native_full = images['native-art']
for name, (profile, _, _) in RUNS.items():
    width = PROFILES[profile]['width'] * 4
    offset = ((PROFILES[profile]['width'] - 240) // 2) * 4
    image = images[name]
    for y in range(640):
        start = y * width * 3 + offset * 3
        assert image[start:start + 960 * 3] == native_full[
            y * 960 * 3:(y + 1) * 960 * 3], (name, y, offset)

# Side artwork is visible, fallback is black, and the native crop is untouched.
for profile in ('16:9', '21:9'):
    width = PROFILES[profile]['width'] * 4
    offset = ((PROFILES[profile]['width'] - 240) // 2) * 4
    for name, run in RUNS.items():
        if run[0] != profile or run[2] != 'missing':
            continue
        artwork_name = name.replace('-missing', '-art')
        artwork, missing = images[artwork_name], images[name]
        for y in (0, 100, 639):
            for x in (0, offset - 1,
                      offset + 960, width - 1):
                assert pixel(artwork, width, x, y) != b'\0\0\0', (
                    artwork_name, x, y)
                assert pixel(missing, width, x, y) == b'\0\0\0', (
                    name, x, y)

print('Fast-travel presenters: PASS')
for profile, info in PROFILES.items():
    width = info['width'] * 4
    offset = ((info['width'] - 240) // 2) * 4
    print(f'  {profile}: {width}x640, native crop x={offset}, '
          f'{sum(run[0] == profile for run in RUNS.values())} runs')
