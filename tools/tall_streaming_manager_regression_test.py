#!/usr/bin/env python3
"""Test real tileset managers with the PC immediate resource-copy routine."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from widescreen_engine_regression_test import function

ROOT = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-tall-manager-') as directory:
    tmp = Path(directory)
    (tmp / 'immediate_resource.inc').write_text(function((ROOT / 'port/stubs_autogen.c').read_text(),
                                                        'void LoadResourceAsync('))
    main_source = (ROOT / 'src/main.c').read_text()
    (tmp / 'region_intersection.inc').write_text(
        function(main_source, 'u32 CheckRegionOnScreen(') + '\n' +
        function(main_source, 'u32 CheckRegionsOnScreenMask('))
    flags = ['-std=gnu11', '-O1', '-g', '-ffunction-sections', '-fdata-sections', '-DPC_PORT',
             '-DMULTI_REGION', '-DUSA', '-DENGLISH', '-include', 'region.h',
             '-I.', '-Iinclude', '-Iport', '-Ibuild/USA', '-Iport/ppu/include', '-I' + directory,
             '-Wno-multichar', '-Wno-int-to-pointer-cast']
    compiler = shlex.split(os.environ.get('CC', 'cc'))
    exe = tmp / 'managers'
    subprocess.run(compiler + flags + ['tools/tests/tall_streaming_managers.c',
                    'src/manager/hyruleTownTileSetManager.c', 'src/manager/minishVillageTileSetManager.c', 'port/port_hdma.c',
                    '-Wl,--gc-sections', '-o', str(exe)], cwd=ROOT, check=True)
    subprocess.run([str(exe)], cwd=ROOT, check=True)
