#!/usr/bin/env python3
"""Exercise production camera/streaming hooks with controlled world apertures."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from widescreen_engine_regression_test import function

ROOT = Path(__file__).resolve().parent.parent


def main():
    sources = {name: (ROOT / name).read_text() for name in
               ('src/scroll.c', 'src/playerUtils.c', 'src/script.c', 'src/main.c', 'port/port_linked_stubs.c')}
    selected = {
        'port/port_linked_stubs.c': ('int Port_Widescreen_CameraRestX(int target_x)',
                                   'int Port_Widescreen_CameraRestY(int target_y)',
                                   'unsigned Port_Widescreen_MapBufferSize(void)',
                                   'static void ram_sub_080B197C_c(',
                                   'static void Port_WidescreenShadow_Populate('),
        'src/scroll.c': ('void Scroll1(RoomControls* controls)', 'void sub_08080974(u32 arg0, u32 arg1)',
                         'void sub_080809D4(void)'),
        'src/script.c': ('void WaitForCameraTouchRoomBorder(Entity* entity, ScriptExecutionContext* context)',),
        'src/main.c': ('u32 CheckRegionOnScreen(u32 x0, u32 y0, u32 x1, u32 y1)',),
    }
    bodies = [function(sources[name], signature) for name, signatures in selected.items() for signature in signatures]
    # Initialization contains alternative #ifdef branches sharing a closing
    # brace. Keep that whole source range for the C preprocessor to resolve.
    player = sources['src/playerUtils.c']
    bodies.append(player[player.index('void InitializeCamera() {'):player.index('void sub_0807C810(void) {')])
    with tempfile.TemporaryDirectory(prefix='tmc-world-view-') as directory:
        tmp = Path(directory)
        (tmp / 'world_view_functions.inc').write_text('\n'.join(bodies))
        for width in (240, 384):
            binary = tmp / f'world-{width}'
            command = shlex.split(os.environ.get('CC', 'cc')) + [
                '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-Wno-unused-parameter', '-Wno-unused-but-set-variable',
                '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                '-DPC_PORT', '-DMULTI_REGION', '-DUSA', f'-DMODE1_GBA_WIDTH={width}',
                '-Wno-unknown-pragmas', '-I.', '-Iinclude', '-Iport', '-Iport/ppu/include', '-I' + directory,
                'tools/tests/world_view.c', 'port/port_viewport.c', 'port/ppu/src/mode1.c',
                'port/ppu/src/virtuappu.c', '-lm', '-o', str(binary)]
            subprocess.run(command, cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
