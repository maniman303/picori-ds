#!/usr/bin/env python3
"""Check that PPU2 does not silently discard live signed OBJ metadata."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from widescreen_engine_regression_test import function
ROOT = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-perfcap-guard-') as directory:
    tmp = Path(directory)
    (tmp / 'perfcap_dump.inc').write_text(function((ROOT / 'port/port_repro_perfcap.c').read_text(),
                                                   'static int perfcap_dump('))
    cmd = shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-DPC_PORT', '-DUSA',
        '-DMODE1_GBA_WIDTH=569', '-I.', '-Iinclude', '-Iport', '-Iport/ppu/include', '-I' + directory,
        'tools/tests/perfcap_snapshot_guard.c', '-o', str(tmp / 'test')]
    subprocess.run(cmd, cwd=ROOT, check=True)
    subprocess.run([str(tmp / 'test')], cwd=tmp, check=True)
