#!/usr/bin/env python3
"""Exercise production presentation workspaces and optional geometry gates."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from widescreen_engine_regression_test import function

ROOT = Path(__file__).resolve().parent.parent


def main():
    source = (ROOT / 'port/port_ppu.cpp').read_text()
    # Keep the actual production capacities/storage, not test-sized substitutes.
    definitions = source[source.index('static constexpr int kMaxInternalScale'):
                         source.index('/* GPU supersample present:')]
    definitions += source[source.index('static constexpr size_t kMaxXbrz2xPixels'):
                          source.index('/* Width VirtuaPPU')]
    bodies = [function(source, signature) for signature in (
        'static void Port_PPU_FitAspectRect(',
        'void Port_PPU_ComputeViewportRects(',
        'static bool Port_PPU_EnsureXbrzBuffers(',
        'static void Port_PPU_ReplicateNearest(',
        'static uint32_t* Port_PPU_BuildScaledFrame(',
    )]
    bodies.append(function((ROOT / 'port/port_voxel.cpp').read_text(), 'bool ScreenGeometrySupported(void)'))
    bodies.append(function((ROOT / 'port/port_gpu_renderer.cpp').read_text(),
                           'static bool Port_GPU_PresetGeometrySupported(void)'))
    flags = ['-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
             '-fno-sanitize-recover=all', '-I' + str(ROOT / 'port')]
    cc = shlex.split(os.environ.get('CC', 'cc'))
    cxx = shlex.split(os.environ.get('CXX', 'c++'))
    with tempfile.TemporaryDirectory(prefix='tmc-presentation-geometry-') as directory:
        tmp = Path(directory)
        (tmp / 'presentation_functions.inc').write_text(definitions + '\n'.join(bodies))
        objects = []
        for name in ('port_upscale', 'port_filter'):
            output = tmp / (name + '.o')
            subprocess.run(cc + ['-std=c11'] + flags + ['-c', str(ROOT / 'port' / (name + '.c')),
                                                       '-o', str(output)], check=True)
            objects.append(str(output))
        for width in (240, 384, 569):
            binary = tmp / f'presentation-{width}'
            subprocess.run(cxx + ['-std=c++17'] + flags + [f'-DMODE1_GBA_WIDTH={width}', '-I' + directory,
                           str(ROOT / 'tools/tests/presentation_geometry.cpp')] + objects +
                           ['-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
