#!/usr/bin/env python3
"""Exercise production piece emission, metadata latching and software OBJ pixels."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from widescreen_engine_regression_test import function

ROOT = Path(__file__).resolve().parent.parent


def main():
    draw = (ROOT / 'port/port_draw.c').read_text()
    stubs = (ROOT / 'port/port_linked_stubs.c').read_text()
    affine = (ROOT / 'src/affine.c').read_text()
    # Skip the forward declaration; retain the actual multi-line definition.
    pieces = function(draw, 'static void RenderSpritePieces(const u8* data, /*')
    bodies = [pieces, function(draw, 'u32 CheckOnScreen(Entity* entity)'),
              function(stubs, 'u32 CheckRectOnScreen(s32 x, s32 y, u32 halfW, u32 halfH)'),
              function(affine, 'void FlushSprites(void)')]
    with tempfile.TemporaryDirectory(prefix='tmc-oam-position-') as directory:
        tmp = Path(directory)
        (tmp / 'oam_functions.inc').write_text('\n'.join(bodies))
        for capacity in (384, 640):
            binary = tmp / f'oam-{capacity}'
            command = shlex.split(os.environ.get('CC', 'cc')) + [
                '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-unknown-pragmas',
                '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                '-DPC_PORT', '-DMULTI_REGION', '-DUSA', f'-DMODE1_GBA_WIDTH={capacity}',
                '-I.', '-Iinclude', '-Iport', '-Iport/ppu/include', '-I' + directory,
                'tools/tests/oam_position.c', 'port/port_oam.c', 'port/ppu/src/mode1.c',
                'port/ppu/src/virtuappu.c', '-lm', '-o', str(binary)]
            subprocess.run(command, cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
