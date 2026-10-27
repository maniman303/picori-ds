#!/usr/bin/env python3
"""Verify the actual shared-framebuffer producer/consumer OAM fallback."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from widescreen_engine_regression_test import function
ROOT = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-shm-oam-') as directory:
    tmp = Path(directory)
    producer = (ROOT / 'port/port_shm_framebuffer.c').read_text()
    consumer = (ROOT / 'port/vk_rt_experiment/FrameSource.cpp').read_text()
    (tmp / 'shm_oam_functions.inc').write_text(
        function(producer, 'void Port_Shm_PublishFramebuffer(') + '\n' +
        function(consumer, 'int ShmFrameSource::oamCount() const'))
    cmd = shlex.split(os.environ.get('CXX', 'c++')) + [
        '-std=c++17', '-O1', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
        '-fno-sanitize-recover=all', '-I' + directory, 'tools/tests/shm_oam_guard.cpp', '-o', str(tmp / 'test')]
    subprocess.run(cmd, cwd=ROOT, check=True)
    env = os.environ.copy(); env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(tmp / 'test')], cwd=ROOT, env=env, check=True)
