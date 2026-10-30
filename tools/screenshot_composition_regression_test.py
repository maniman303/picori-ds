#!/usr/bin/env python3
"""Check production screenshot encoding of composed fixed-screen pixels."""
import ctypes.util
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import struct
import zlib
from widescreen_engine_regression_test import function

ROOT = Path(__file__).resolve().parent.parent
ppu = (ROOT / 'port/port_ppu.cpp').read_text()
writer = (ROOT / 'port/port_bugreport.cpp').read_text()
functions = [function(ppu, signature) for signature in (
    'static const uint32_t* Port_PPU_SelectPresentFrame(',
    'extern "C" const uint32_t* Port_PPU_GetComposedFrame(',
)]
functions.append(function(writer, 'bool WriteScreenshotPNG('))
with tempfile.TemporaryDirectory(prefix='tmc-screenshot-composition-') as directory:
    tmp = Path(directory)
    (tmp / 'screenshot_functions.inc').write_text('\n'.join(functions))
    flags = ['-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
             '-fno-sanitize-recover=all', '-I' + str(ROOT / 'port')]
    obj = tmp / 'cap.o'
    subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + ['-std=c11'] + flags +
                   ['-c', str(ROOT / 'port/port_title_cap.c'), '-o', str(obj)], check=True)
    lib = ctypes.util.find_library('png16')
    if not lib:
        raise RuntimeError('libpng16 is required')
    binary = tmp / 'test'
    subprocess.run(shlex.split(os.environ.get('CXX', 'c++')) + ['-std=c++17'] + flags +
                   ['-I' + directory, str(ROOT / 'tools/tests/screenshot_composition.cpp'), str(obj),
                    '-Wl,-l:' + lib, '-o', str(binary)], check=True)
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(binary), directory], env=env, check=True)
    profiles = [(240, 160, 0, 0), (256, 192, 8, 16), (285, 160, 22, 0),
                (374, 160, 67, 0), (569, 160, 164, 0), (288, 162, 24, 1)]
    for width, height, nx, ny in profiles:
        data = (tmp / f'{width}.png').read_bytes()
        assert data[:8] == b'\x89PNG\r\n\x1a\n'
        offset, compressed = 8, bytearray()
        while offset < len(data):
            length = struct.unpack_from('>I', data, offset)[0]
            kind = data[offset + 4:offset + 8]
            chunk = data[offset + 8:offset + 8 + length]
            if kind == b'IHDR':
                assert struct.unpack('>IIBBBBB', chunk) == (width, height, 8, 2, 0, 0, 0)
            elif kind == b'IDAT':
                compressed.extend(chunk)
            offset += length + 12
        raw = zlib.decompress(compressed)
        stride = width * 3
        previous = bytearray(stride)
        for y in range(height):
            row_start = y * (stride + 1)
            filter_type = raw[row_start]
            row = bytearray(raw[row_start + 1:row_start + 1 + stride])
            for i in range(stride):
                left = row[i - 3] if i >= 3 else 0
                up = previous[i]
                upper_left = previous[i - 3] if i >= 3 else 0
                if filter_type == 1: predictor = left
                elif filter_type == 2: predictor = up
                elif filter_type == 3: predictor = (left + up) // 2
                elif filter_type == 4:
                    p = left + up - upper_left
                    distances = (abs(p-left), abs(p-up), abs(p-upper_left))
                    predictor = (left, up, upper_left)[distances.index(min(distances))]
                else:
                    assert filter_type == 0
                    predictor = 0
                row[i] = (row[i] + predictor) & 255
            for x in range(width):
                if nx <= x < nx + 240 and ny <= y < ny + 160:
                    expected = ((x - nx) & 255, (y - ny) & 255, 73)
                elif width != 288 and width > 240 and nx - 8 <= x < nx and ny <= y < ny + 32:
                    expected = (248, 0, 0)
                else:
                    expected = (3, 5, 7)
                actual = tuple(row[x*3:x*3+3])
                assert actual == expected, (width, x, y, actual, expected)
            previous = row
    print('Composed title-cap screenshot encoding, all profile geometries: PASS')
