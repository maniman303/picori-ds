#!/usr/bin/env python3
"""Check real OBJ-window title-cap screenshots from both presenters."""
import argparse
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('directory', type=Path)
a = p.parse_args()
root = a.directory

def rgb(name, width):
    path = root / name / 'presentation.png'
    size = subprocess.check_output(['identify', '-format', '%wx%h', str(path)], text=True)
    assert size == f'{width * 4}x640', (name, size)
    full = subprocess.check_output(['convert', str(path), '-depth', '8', 'rgb:-'])
    raw = subprocess.check_output(['convert', str(path), '-sample', f'{width}x160!', '-depth', '8', 'rgb:-'])
    assert len(full) == width * 4 * 640 * 3 and len(raw) == width * 160 * 3
    return full, raw

def pixel(data, width, x, y):
    i = (y * width + x) * 3
    return data[i:i+3]

def trace(name):
    with (root / name / 'audit/trace.jsonl').open() as file:
        rows = [json.loads(line) for line in file if line.endswith('\n')]
    state = next(r for r in rows if r.get('frame') == 1690 and 'event' not in r)
    assert state['case'] == 15 and state['heading_y'] == 16 and state['display_control'] & 0x8000, (name, state)
    log = (root / name / 'run.log').read_text()
    assert '[capture] frame 1690' in log and '(ok=1)' in log
    if name.startswith('gpu'):
        assert 'PPU initialized with SDL_GPU backend' in log
    else:
        assert 'PPU initialized with SDL renderer backend' in log

native_full, native = rgb('native', 240)
trace('native')
for width, names in ((285, ('software-art-final', 'software-missing-final', 'gpu-art-final', 'gpu-missing-final')),
                     (374, ('sw21-art', 'sw21-missing', 'gpu21-art', 'gpu21-missing'))):
    nx = (width - 240) // 2
    images = [rgb(name, width) for name in names]
    full = [image[0] for image in images]
    data = [image[1] for image in images]
    for name in names: trace(name)
    assert full[0] == full[2] and full[1] == full[3], (width, 'presenter mismatch')
    for image in full:
        for y in range(640):
            assert image[(y*width*4+nx*4)*3:(y*width*4+(nx+240)*4)*3] == native_full[y*960*3:(y+1)*960*3], (width, y)
    assert pixel(data[0], width, 0, 100) != pixel(data[1], width, 0, 100)
    assert pixel(data[1], width, 0, 100) == b'\0\0\0'
    # On this real native frame, the right corner occupies native x=24..31.
    # The cut left edge is the mirrored corner. Compare only opaque cap
    # pixels, since transparent pixels expose different artwork/fallback.
    checked = 0
    for y in range(32):
        for x in range(8):
            cap = pixel(data[1], width, nx-8+x, y)
            if cap == b'\0\0\0':
                continue
            assert cap == pixel(data[0], width, nx-8+x, y), (width, x, y, 'art/fallback cap')
            assert cap == pixel(native, 240, 24+7-x, y), (width, x, y, 'native mirror')
            checked += 1
    assert checked >= 100, (width, checked)
    print(f'{width}x160: identical native crops and presenters; {checked} cap pixels match native corner')
