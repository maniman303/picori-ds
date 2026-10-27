#!/usr/bin/env python3
"""Exercise masked native-bank resolution and RGB888 fade arithmetic."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    compiler = shlex.split(os.environ.get('CC', 'cc'))
    with tempfile.TemporaryDirectory(prefix='tmc-background-effect-') as directory:
        binary = Path(directory) / 'background-effect'
        subprocess.run(compiler + ['-std=c11', '-Wall', '-Wextra', '-Werror', '-Iport',
                                   'tools/tests/background_effect.c', 'port/port_background_effect.c',
                                   '-o', str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)
        # The port consumes this C API from C++17 as well as C.
        obj = Path(directory) / 'background-effect.o'
        subprocess.run(compiler + ['-std=c11', '-Iport', '-c', 'port/port_background_effect.c',
                                   '-o', str(obj)], cwd=ROOT, check=True)
        cxx = shlex.split(os.environ.get('CXX', 'c++'))
        subprocess.run(cxx + ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-Iport',
                              'tools/tests/background_effect.c', str(obj), '-o', str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)
        engine = Path(directory) / 'fade.o'
        staffroll = Path(directory) / 'staffroll.o'
        native_common = Path(directory) / 'common.o'
        native_main = Path(directory) / 'main.o'
        common = compiler + ['-std=gnu11', '-O1', '-ffunction-sections', '-fdata-sections',
                             '-funsigned-char', '-DPC_PORT', '-DNON_MATCHING', '-DUSE_HDMA',
                             '-DUSA', '-DENGLISH', '-DMULTI_REGION', '-include', 'region.h',
                             '-Iinclude', '-Ilibs', '-Iport', '-I.', '-Ibuild/USA', '-Iport/ppu/include']
        subprocess.run(common + ['-w', '-c', 'src/fade.c', '-o', str(engine)], cwd=ROOT, check=True)
        subprocess.run(common + ['-w', '-c', 'src/staffroll.c', '-o', str(staffroll)], cwd=ROOT, check=True)
        subprocess.run(common + ['-w', '-c', 'src/common.c', '-o', str(native_common)], cwd=ROOT, check=True)
        subprocess.run(common + ['-w', '-c', 'src/main.c', '-o', str(native_main)], cwd=ROOT, check=True)
        subprocess.run(common + ['tools/tests/background_effect_engine.c', 'port/port_background_effect.c', 'port/port_effect_geometry.c', 'port/port_hdma.c',
                                 str(engine), str(staffroll), str(native_common), str(native_main), '-Wl,--gc-sections', '-lm', '-o', str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == '__main__':
    main()
