#!/usr/bin/env python3
"""Exercise production scene and map-fill bodies with a test-only tall profile."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from widescreen_engine_regression_test import function
ROOT = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-tall-stage-') as directory:
    tmp = Path(directory)
    source = (ROOT / 'port/port_linked_stubs.c').read_text()
    signatures = ('static uint64_t Port_WidescreenRoomKey(void)',
                  'static int Port_Widescreen_MapTileX(void)', 'static int Port_Widescreen_MapTileY(void)',
                  'void Port_Widescreen_EntityCreated(void)',
                  'void Port_Widescreen_ManagerCreated(unsigned id)',
                  'static int Port_WidescreenTallAperture(void)', 'static int Port_WidescreenTallRows(void)',
                  'static int Port_WidescreenCandidateWidth(void)',
                  'static int Port_WidescreenManagersSafe(int apertureHeight)',
                  'static int Port_WidescreenEntitiesSafe(int apertureHeight)',
                  'static int Port_WidescreenMapShadowsLive(void)',
                  'static bool Port_WidescreenMapSnapshotMatches(int height)',
                  'static void Port_WidescreenUploadMapBuffers(void)',
                  'void Port_Widescreen_SetTallValidated(int validated)',
                  'void Port_Widescreen_MapFilled(unsigned mode)', 'void Port_Widescreen_MapUploaded(void)',
                  'static void ram_sub_080B197C_c(', 'void UpdateScrollVram(void)',
                  'void Port_Widescreen_PublishScene(void)', 'void Port_Widescreen_BeforePresent(void)',
                  'int Port_Widescreen_EffectiveViewHeight(void)',
                  'unsigned Port_Widescreen_MapBufferSize(void)')
    bodies = [function(source, sig) for sig in signatures]
    bodies.append(function((ROOT / 'src/scroll.c').read_text(), 'void InitScreenShake(u32 time, u32 magnitude)'))
    (tmp / 'tall_aperture_functions.inc').write_text('\n'.join(bodies))
    cmd = shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
        '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-DPC_PORT', '-DMULTI_REGION',
        '-DUSA', '-DMODE1_GBA_WIDTH=569', '-DPORT_VIEWPORT_TEST_TALL', '-I.', '-Iinclude', '-Iport',
        '-Iport/ppu/include', '-I' + directory, 'tools/tests/tall_aperture_staging.c',
        'port/port_viewport.c', 'port/port_tall_scene.c', 'port/port_oam.c', '-o', str(tmp / 'test')]
    subprocess.run(cmd, cwd=ROOT, check=True)
    env = os.environ.copy(); env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(tmp / 'test')], cwd=ROOT, env=env, check=True)
