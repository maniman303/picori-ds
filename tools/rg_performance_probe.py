#!/usr/bin/env python3
"""Measure PPU render time across OpenMP worker counts with a real PPU snapshot.

This is the host/device-independent half of the RG performance task: it compiles
the exact production software rasterizer used by the game, replays a PPU2
snapshot, and checks that every thread count produces the same framebuffer
checksum. Device-specific RG measurements must still be taken on the handheld;
this tool never substitutes host or QEMU numbers for that acceptance.

The snapshot contains ROM-derived graphics. Keep it in an untracked evidence
directory, never in the repository.

Example (native):
    python3 tools/rg_performance_probe.py \
        --snapshot .tmp/rg-performance-2026-10-30/room.ppu \
        --threads 1,2,4 --iterations 1000 \
        --json .tmp/rg-performance-2026-10-30/results.json

4:3 (256x192) snapshots are validated with:
    python3 tools/rg_performance_probe.py --snapshot room43.ppu \
        --require-width 256 --require-height 192 --width 256 --threads 1,2,4
"""
import argparse
import hashlib
import json
import os
import platform
import re
import shlex
import struct
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_CFLAGS = ['-O3', '-mavx2', '-mfma', '-fopenmp']


def snapshot_geometry(path):
    """Read PPU1/PPU2 geometry without linking the PPU. PPU2 stores LE u32
    width/height/pitch after the magic and five region sizes."""
    with path.open('rb') as stream:
        header = stream.read(4 + 5 * 4 + 3 * 4)
    if len(header) < 4:
        raise SystemExit(f'{path}: truncated snapshot')
    magic = header[:4]
    if magic == b'PPU1':
        return 240, 160
    if magic != b'PPU2' or len(header) < 4 + 5 * 4 + 3 * 4:
        raise SystemExit(f'{path}: unsupported snapshot magic {magic!r}')
    width, height, pitch = struct.unpack_from('<III', header, 4 + 5 * 4)
    return width, height


def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


def build_bench(cc, width, output, cflags):
    command = cc + cflags + [
        '-I' + str(ROOT / 'port/ppu/include'),
        f'-DMODE1_GBA_WIDTH={width}',
        str(ROOT / 'tools/ppu_bench.c'),
        str(ROOT / 'port/ppu/src/mode1.c'),
        '-lm',
        '-o',
        str(output),
    ]
    subprocess.run(command, check=True)


def run_bench(binary, snapshot, threads, iterations):
    env = dict(os.environ)
    env['OMP_NUM_THREADS'] = str(threads)
    result = subprocess.run([str(binary), str(snapshot), str(iterations)],
                            check=True, text=True, capture_output=True, env=env)
    checksum = re.search(r'checksum\s*:\s*(0x[0-9a-fA-F]+)', result.stdout)
    timings = re.findall(
        r'render_frame\s*:\s*([0-9.]+) ms/frame.*?\[(\d+) threads?\]',
        result.stdout)
    if not checksum or not timings:
        raise SystemExit(f'unexpected ppu_bench output:\n{result.stdout}\n{result.stderr}')
    milliseconds, reported_threads = timings[0]
    if int(reported_threads) != threads:
        raise SystemExit(f'asked for {threads} threads, bench reported {reported_threads}')
    return {
        'threads': threads,
        'milliseconds': float(milliseconds),
        'fps': 1000.0 / float(milliseconds) if float(milliseconds) else 0.0,
        'checksum': checksum.group(1).lower(),
        'stdout': result.stdout,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--threads', default='1,2,4',
                        help='comma-separated OpenMP worker counts (default 1,2,4)')
    parser.add_argument('--iterations', type=int, default=1000)
    parser.add_argument('--repeats', type=int, default=1,
                        help='independent runs per worker count; median is reported')
    parser.add_argument('--width', type=int, default=240,
                        help='MODE1_GBA_WIDTH capacity for the benchmark build')
    parser.add_argument('--require-width', type=int,
                        help='fail unless the snapshot frame width matches')
    parser.add_argument('--require-height', type=int,
                        help='fail unless the snapshot frame height matches (192 for 4:3)')
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--cflags', default=' '.join(DEFAULT_CFLAGS))
    parser.add_argument('--json', type=Path)
    args = parser.parse_args()

    snapshot = args.snapshot.resolve()
    if not snapshot.is_file():
        parser.error(f'missing snapshot: {snapshot}')
    frame_width, frame_height = snapshot_geometry(snapshot)
    if args.require_width is not None and frame_width != args.require_width:
        parser.error(f'snapshot width {frame_width}, expected {args.require_width}')
    if args.require_height is not None and frame_height != args.require_height:
        parser.error(f'snapshot height {frame_height}, expected {args.require_height}')
    if args.width < frame_width:
        parser.error(f'--width {args.width} is smaller than the snapshot frame {frame_width}')
    threads = [int(value) for value in args.threads.split(',') if value.strip()]
    if not threads or any(value < 1 for value in threads):
        parser.error('thread counts must be positive')
    if args.iterations < 1:
        parser.error('iterations must be positive')
    if args.repeats < 1:
        parser.error('repeats must be positive')

    cc = shlex.split(args.cc)
    cflags = shlex.split(args.cflags)
    with tempfile.TemporaryDirectory(prefix='tmc-rg-probe-') as directory:
        binary = Path(directory) / 'ppu_bench'
        build_bench(cc, args.width, binary, cflags)
        runs = []
        for count in threads:
            repeats = [run_bench(binary, snapshot, count, args.iterations)
                       for _ in range(args.repeats)]
            checksums = {run['checksum'] for run in repeats}
            if len(checksums) != 1:
                raise SystemExit(f'framebuffer checksum changed for {count} threads: {checksums}')
            timings = sorted(run['milliseconds'] for run in repeats)
            median = timings[len(timings) // 2]
            runs.append({
                'threads': count,
                'milliseconds': median,
                'min_milliseconds': timings[0],
                'max_milliseconds': timings[-1],
                'fps': 1000.0 / median if median else 0.0,
                'checksum': checksums.pop(),
                'repeats': len(timings),
            })

    checksums = {run['checksum'] for run in runs}
    if len(checksums) != 1:
        raise SystemExit(f'framebuffer checksum changed across thread counts: {checksums}')

    base = next(run for run in runs if run['threads'] == min(threads))
    for run in runs:
        run['speedup_vs_min_threads'] = (base['milliseconds'] / run['milliseconds']
                                         if run['milliseconds'] else 0.0)

    report = {
        'host': {
            'machine': platform.machine(),
            'platform': platform.platform(),
            'processor': platform.processor(),
            'cpu_count': os.cpu_count(),
        },
        'build': {'width': args.width, 'cflags': cflags, 'iterations': args.iterations,
                  'repeats': args.repeats},
        'snapshot': {'sha256': sha256(snapshot), 'bytes': snapshot.stat().st_size,
                     'frame_width': frame_width, 'frame_height': frame_height},
        'checksum': next(iter(checksums)),
        'runs': [{key: value for key, value in run.items() if key != 'stdout'} for run in runs],
        'limitations': [
            'host measurement, not a physical RG device',
            'snapshot is a static PPU frame; no engine/audio/present cost',
            'keep the ROM-derived snapshot outside version control',
        ],
    }

    print(f"snapshot: {snapshot.name} ({report['snapshot']['bytes']} bytes, "
          f"{frame_width}x{frame_height}) sha256={report['snapshot']['sha256'][:16]}...")
    print(f"host: {report['host']['machine']} cpu_count={report['host']['cpu_count']} "
          f"width={args.width} iterations={args.iterations}")
    print('threads   ms/frame    fps    speedup   checksum')
    for run in runs:
        print(f"{run['threads']:>7}   {run['milliseconds']:8.4f}  "
              f"{run['fps']:7.0f}   {run['speedup_vs_min_threads']:6.2f}x   {run['checksum']}")
    print('PASS: identical framebuffer checksum across all thread counts')

    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
        print(f"wrote {args.json}")


if __name__ == '__main__':
    main()
