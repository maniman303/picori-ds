#!/usr/bin/env python3
"""Verify compiled offsets across every supported build/ROM baseline."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'port/port_offset_remap.c').read_text()
for kind, table in (('gfx', 'sGfxRemapEU'), ('map', 'sMapRemap')):
    paths = [root / f'build/{region}/assets/{kind}_offsets.h' for region in ('USA', 'EU')]
    if not all(path.is_file() for path in paths):
        continue  # The generated regional assets are optional for this ROM-free test.
    def offsets(path):
        return {name: int(value, 0) for name, value in re.findall(
            r'#define\s+(offset_\w+)\s+(0x[\da-fA-F]+|\d+)', path.read_text())}
    usa, eu = (offsets(path) for path in paths)
    section = source.split('static const u32 ' + table, 1)[1].split('};', 1)[0]
    rows = [tuple(int(v, 16) for v in re.findall(r'0x[\da-fA-F]+', line)) for line in section.splitlines()]
    mapping = {row[0]: row[1] for row in rows if len(row) >= 2}
    for name in usa.keys() & eu.keys():
        assert mapping.get(usa[name], usa[name]) == eu[name], (kind, name)
    print(f'{kind}: {len(usa.keys() & eu.keys())} USA/EU generated offsets match the mapping')
with tempfile.TemporaryDirectory(prefix='tmc-offset-baseline-') as directory:
    for baseline in ('USA', 'EU', 'JP'):
        binary = Path(directory) / baseline
        cmd = shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
            '-DPC_PORT', '-DMULTI_REGION', '-D' + baseline, '-DENGLISH',
            '-I.', '-Iinclude', '-Iport', '-Ibuild/USA', '-Iport/ppu/include',
            'tools/tests/offset_baseline.c', '-o', str(binary)]
        subprocess.run(cmd, cwd=root, check=True)
        env = os.environ.copy(); env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
        subprocess.run([str(binary)], cwd=root, env=env, check=True)
