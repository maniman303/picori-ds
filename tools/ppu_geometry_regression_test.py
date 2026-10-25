#!/usr/bin/env python3
"""Check real software PPU geometry, raster bounds and capture compatibility."""
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def check_capture_tools(compiler, temporary):
    sources = ['port/ppu/src/virtuappu.c', 'port/ppu/src/mode1.c']
    oracle, bench = temporary / 'parity', temporary / 'bench'
    common = compiler + ['-std=gnu11', '-O1', '-Wno-unknown-pragmas',
                         '-DMODE1_GBA_WIDTH=569', '-Iport/ppu/include']
    subprocess.run(common + ['tools/ppu_parity.c'] + sources + ['-lm', '-o', str(oracle)], cwd=ROOT, check=True)
    subprocess.run(common + ['tools/ppu_bench.c', 'port/ppu/src/mode1.c', '-lm', '-o', str(bench)], cwd=ROOT, check=True)
    sizes = (0x400, 0x18000, 0x200, 0x200, 0x400)
    data = bytes(sizes[0] + sizes[1]) + struct.pack('<H', 0x3e0) + bytes(sum(sizes[2:]) - 2)
    for version, width, height, pitch in ((1, 240, 160, 240), (2, 240, 160, 384),
                                         (2, 256, 192, 384), (2, 569, 160, 569), (2, 569, 192, 569)):
        header = f'PPU{version}'.encode() + struct.pack('<5I', *sizes)
        if version == 2:
            header += struct.pack('<3I', width, height, pitch)
        capture = temporary / f'{version}-{height}.ppu'
        capture.write_bytes(header + data)
        # Independent expected pixel/hash: solid green backdrop. The pitch's
        # padding and compiled width capacity must never enter the checksum.
        digest = 1469598103934665603
        for _ in range(width * height):
            digest = ((digest ^ 0xff00f800) * 1099511628211) & ((1 << 64) - 1)
        expected = f'0x{digest:016x}'
        result = subprocess.run([str(oracle), str(capture)], check=True, capture_output=True, text=True)
        assert result.stdout.strip() == expected, result
        result = subprocess.run([str(bench), str(capture), '1'], check=True, capture_output=True, text=True)
        assert f'checksum     : {expected}' in result.stdout, result
    print('Capture tools honor legacy/native/tall geometry: PASS')


def main():
    compiler = shlex.split(os.environ.get('CC', 'cc'))
    with tempfile.TemporaryDirectory(prefix='tmc-ppu-geometry-') as directory:
        for width, n64 in ((240, False), (384, False), (569, False), (240, True)):
            binary = Path(directory) / f'geometry-{width}-{n64}'
            command = compiler + ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                                  '-Wno-unknown-pragmas', '-fsanitize=address,undefined',
                                  '-fno-sanitize-recover=all', f'-DMODE1_GBA_WIDTH={width}',
                                  '-Iport/ppu/include', '-Iport']
            if n64:
                command += ['-DTMC_N64']
            command += ['tools/tests/ppu_geometry.c', 'port/ppu/src/mode1.c',
                        'port/ppu/src/virtuappu.c', 'port/port_hdma.c', 'port/port_viewport.c',
                        '-lm', '-o', str(binary)]
            subprocess.run(command, cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)
        check_capture_tools(compiler, Path(directory))


if __name__ == '__main__':
    main()
