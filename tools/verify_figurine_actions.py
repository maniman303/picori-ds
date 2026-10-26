#!/usr/bin/env python3
"""Verify a native figurine-action run, or explicitly recover completed cases.

--completed-prefix never certifies an interrupted process as successful. It
validates only complete case receipts and their captures, preserves the raw
trace unchanged, and reports the exact canonical range needing a separate run.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image
from background_figurine_actions_audit import (figurine_action_start, figurine_action_cases,
                                               validate_figurine_actions, validate_figurine_action_pixels)
from background_figurine_audit import figurine_reference


def completed_prefix(rows):
    events = [r for r in rows if 'event' in r]
    start = figurine_action_start(events)
    results = [r for r in events if r['event'] == 'figurine_action_restored']
    if [r['value'] for r in results] != list(range(start, start + len(results))):
        raise ValueError('completed prefix has missing or duplicate result IDs')
    frames = {r['frame']: (i, r) for i, r in enumerate(rows) if 'event' not in r and r.get('capture')}
    while results:
        result = results[-1]
        sample = frames.get(result['frame'])
        if sample and sample[1].get('figurine_action_case') == result['value']:
            break
        results.pop()  # A killed writer may have recorded the event but not its frame receipt.
    if not results:
        raise ValueError('no completed figurine case/capture receipts')
    end = results[-1]['value'] + 1
    terminal = next((i for i, r in enumerate(rows) if r.get('event') == 'figurine_actions_complete' and r['value'] == 54), None)
    if end == 54 and terminal is not None:
        return rows[:terminal + 1], start, end, True
    return rows[:frames[results[-1]['frame']][0] + 1], start, end, False


def verify_capture(directory, rom, language, recover=False):
    trace = directory / 'trace.jsonl'
    raw = trace.read_bytes()
    lines = raw.splitlines()
    rows = []
    ignored_tail = False
    for index, line in enumerate(lines):
        try:
            rows.append(json.loads(line))
        except (ValueError, UnicodeError):
            if not recover or index != len(lines) - 1:
                raise ValueError('corrupt figurine trace before recoverable EOF')
            ignored_tail = True
    if recover:
        rows, start, end, terminal = completed_prefix(rows)
    else:
        start, end, terminal = figurine_action_start([r for r in rows if 'event' in r]), 54, True
    events = [r for r in rows if 'event' in r]
    frames = [r for r in rows if 'event' not in r and r.get('capture')]
    validate_figurine_actions(rows, events, frames, start=start, end=end, require_complete=terminal)
    hashes = {}
    for row in frames:
        filename = f"{row['frame']:06d}.png"
        with Image.open(directory / filename) as image:
            image.load()
            if image.size != (240, 160):
                raise ValueError('figurine capture has non-native dimensions')
            hashes[filename] = hashlib.sha256(image.convert('RGBA').tobytes()).hexdigest()
    pixels = validate_figurine_action_pixels(directory, events, figurine_reference(rom, language), start=start, end=end)
    checkpoints = {str(case): {} for case in range(start, end)}
    for row in events:
        if row['event'] in ('figurine_action_ready', 'figurine_action_held', 'figurine_action_bottom',
                            'figurine_action_top', 'figurine_action_restored'):
            checkpoints[str(row['value'])][row['event']] = str((directory / f"{row['frame']:06d}.png").resolve())
    cases = figurine_action_cases()[start:end]
    return dict(preferred_language=language, directory=str(directory.resolve()),
                scope='completed-prefix-of-interrupted-run' if recover else 'native-complete-run',
                native_terminal_observed=terminal, ignored_incomplete_eof=ignored_tail,
                trace_sha256=hashlib.sha256(raw).hexdigest(), rom_sha256=hashlib.sha256(rom.read_bytes()).hexdigest(),
                start_case=start, end_case=end, validated_cases=list(range(start, end)),
                prize_cases=sum(not c['collection'] for c in cases), collection_cases=sum(c['collection'] for c in cases),
                captures=hashes, native_field_pixels=pixels, checkpoints=checkpoints)


def combine_coverage(reports):
    if not reports or len({r['preferred_language'] for r in reports}) != 1 or len({r['rom_sha256'] for r in reports}) != 1:
        raise ValueError('coverage reports must use one language and ROM')
    seen = set()
    for report in reports:
        ids = report['validated_cases']
        if ids != list(range(report['start_case'], report['end_case'])) or not 0 <= report['start_case'] < report['end_case'] <= 54:
            raise ValueError('invalid verified case range')
        if seen.intersection(ids):
            raise ValueError('duplicate figurine coverage cases')
        seen.update(ids)
    if seen != set(range(54)):
        raise ValueError('incomplete figurine coverage cases')
    return dict(preferred_language=reports[0]['preferred_language'], rom_sha256=reports[0]['rom_sha256'],
                validated_cases=sorted(seen), prize_cases=sum(r['prize_cases'] for r in reports),
                collection_cases=sum(r['collection_cases'] for r in reports),
                field_checkpoints=sum(r['native_field_pixels']['checkpoints'] for r in reports),
                description_comparisons=sum(r['native_field_pixels']['description_comparisons'] for r in reports),
                segments=[dict(directory=r['directory'], scope=r['scope'], start=r['start_case'], end=r['end_case'],
                               trace_sha256=r['trace_sha256'], native_terminal_observed=r['native_terminal_observed']) for r in reports])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True, help='figurine_actions directory containing trace.jsonl')
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--language', type=int, choices=range(1, 6), default=1)
    parser.add_argument('--completed-prefix', action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = verify_capture(args.capture, args.rom, args.language, args.completed_prefix)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f"Validated canonical cases {report['start_case']}..{report['end_case'] - 1}; scope={report['scope']}")


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError) as error:
        raise SystemExit(f'Figurine verification failed: {error}') from error
