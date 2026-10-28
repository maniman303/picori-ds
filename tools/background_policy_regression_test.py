#!/usr/bin/env python3
"""Exercise context effect ownership and explicit alpha inputs."""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
parser = argparse.ArgumentParser()
parser.add_argument('--coverage', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='tmc-background-policy-') as directory:
    binary = Path(directory) / 'test'
    subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=c11', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-Iport',
        'tools/tests/background_policy.c', 'port/port_background_policy.c',
        'port/port_background_effect.c', '-o', str(binary)], cwd=root, check=True)
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    subprocess.run([str(binary)], cwd=root, env=env, check=True)
    if args.coverage:
        coverage_path = args.coverage
        catalog = Path(directory) / 'catalog'
        subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c11', '-Wall', '-Wextra', '-Werror', '-Iport',
            'tools/tests/background_policy_catalog.c', 'port/port_background_policy.c',
            'port/port_background_effect.c', 'port/port_screen_background.c', '-o', str(catalog)],
            cwd=root, check=True)
        entries = subprocess.check_output([str(catalog)], text=True).splitlines()
        coverage = json.loads(coverage_path.read_text())['images']
        assert len(entries) == len(coverage) == 33
        for entry in entries:
            name, layers, banks = entry.split()
            item = coverage[Path(name).stem]
            assert (int(layers), int(banks)) == (item['layer_mask'], item['palette_bank_mask']), name
        print('ROM-derived context coverage: PASS')
