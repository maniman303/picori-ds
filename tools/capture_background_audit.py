#!/usr/bin/env python3
"""Capture native fixed-screen state evidence without approving golden pixels.

Requires a repro-enabled binary, a local ROM and Pillow. Each run uses a fresh
runtime directory and refuses existing output directories. A wrapper executable
may supply QEMU for cross-built binaries. The trace records native installation
events separately from transient command IDs; contact sheets are review aids,
not a replacement for inspection of individual frames.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from PIL import Image, ImageDraw


def menu_cases():
    """Independent audit expectations: health thresholds, functional screens,
    and all 17 base / five flag-or-room-dependent detail-map recipes."""
    cases = [dict(label=f'threshold_{health}_of_{maximum}', health=health, max_health=maximum,
                  decoration=decoration, screen=1, map_group=None)
             for health, maximum, decoration in ((24, 24, 0), (16, 24, 0), (15, 24, 1),
                                                 (9, 24, 1), (8, 24, 2), (1, 24, 2), (8, 8, 2))]
    screens = [('inventory', 1), ('save_from_inventory', 11), ('inventory_return', 1),
               ('quest', 2), ('save_from_quest', 10), ('quest_return', 2),
               ('sleep', 9), ('quest_after_sleep', 2), ('overworld_map', 4),
               ('kinstone_pieces', 7), ('sword_techniques', 8)]
    maps = [(f'detail_{area:02d}', 0x5f + area) for area in range(17)]
    maps += [('detail_09_house_built', 0x70), ('detail_11_fusion_done', 0x71),
             ('detail_14_interior_1', 0x72), ('detail_14_interior_2', 0x73), ('detail_14_interior_3', 0x74)]
    for decoration, family in enumerate(('normal', 'damaged', 'critical')):
        for label, screen in screens:
            cases.append(dict(label=f'{family}_{label}', decoration=decoration, screen=screen, map_group=None))
        for label, group in maps:
            cases.append(dict(label=f'{family}_{label}', decoration=decoration, screen=6, map_group=group))
    return cases


def validate_menus(events, frames):
    cases = menu_cases()
    by_frame = {row['frame']: row for row in frames}
    openings = [row for row in events if row['event'] == 'menu_decoration']
    # Ordinary screen changes must not reinstall the decoration, even when
    # current health would select another variant on a fresh opening.
    if [row['value'] for row in openings] != [0, 0, 1, 1, 2, 2, 2, 0, 1, 2]:
        raise ValueError('incorrect decoration selection or unexpected reinstallation')
    initial = [(24, 24), (16, 24), (15, 24), (9, 24), (8, 24), (1, 24), (8, 8),
               (24, 24), (15, 24), (8, 24)]
    if [(row['health'], row['max_health']) for row in openings] != initial:
        raise ValueError('menu openings did not exercise the required health thresholds')
    for phase in ('menu_case_ready', 'menu_case_held'):
        checkpoints = [row for row in events if row['event'] == phase]
        if [row['value'] for row in checkpoints] != list(range(len(cases))):
            raise ValueError(f'{phase}: incomplete or unordered menu cases')
        for row, case in zip(checkpoints, cases):
            capture = by_frame.get(row['frame'])
            if not capture or capture.get('case') != row['value'] or capture['fade_active']:
                raise ValueError(f'{case["label"]}: missing neutral checkpoint capture')
            for evidence in (row, capture):
                if evidence['decoration'] != case['decoration'] or evidence['pause_screen'] != case['screen']:
                    raise ValueError(f'{case["label"]}: wrong installed decoration/screen')
                if case['map_group'] is not None and evidence['map_group'] != case['map_group']:
                    raise ValueError(f'{case["label"]}: wrong native detail-map recipe')
            if phase == 'menu_case_ready' and 'health' in case and (
                    row['health'], row['max_health']) != (case['health'], case['max_health']):
                raise ValueError(f'{case["label"]}: incorrect health fixture')
            if phase == 'menu_case_held' and row['health'] != (24 if case['decoration'] == 2 else 8):
                raise ValueError(f'{case["label"]}: health mutation was not exercised')


def validate_inventory_pixels(directory, events, reference_rows):
    """The exposed bottom row is BG3 on inventory and remains unobscured by UI.
    Use ROM-decoded references, never an artist's override or a prior screenshot.
    Other screens can cover this row with their frame/label and are not sampled.
    """
    if len(reference_rows) != 3 or len(set(reference_rows)) != 3:
        raise ValueError('menu references must distinguish all three health decorations')
    checked = 0
    for event in events:
        if event['event'] not in ('menu_case_ready', 'menu_case_held') or event['pause_screen'] != 1:
            continue
        with Image.open(directory / f"{event['frame']:06d}.png") as image:
            actual = image.convert('RGB').crop((0, 159, 240, 160)).tobytes()
        if actual != reference_rows[event['decoration']]:
            raise ValueError(f'case {event["value"]}: inventory pixels disagree with installed decoration')
        checked += 1
    if not checked:
        raise ValueError('no inventory checkpoints for ROM pixel comparison')
    return dict(checkpoints=checked, pixels_per_checkpoint=240,
                reference_sha256=[hashlib.sha256(row).hexdigest() for row in reference_rows])


def menu_reference_rows(rom, language):
    from export_screen_backgrounds import Rom, menu_recipes
    reference_rows = []
    for _, scene, layers, backdrop in menu_recipes(Rom(rom, language)):
        image = Image.frombytes('RGB', (240, 160), scene.render(layers, backdrop))
        reference_rows.append(image.crop((0, 159, 240, 160)).tobytes())
        if len(reference_rows) == 3:
            break
    return reference_rows


def validate_trace(rows, scenario):
    events = [row for row in rows if 'event' in row]
    install_event = 'credits_install' if scenario == 'credits' else 'prologue_install'
    installs = {row['value'] for row in events if row['event'] == install_event}
    if scenario == 'files':
        states = {row['value'] for row in events if row['event'] == 'file_state'}
        # Language selection is EU-only. All runs must visit the six other
        # screens through native input before claiming file-operation coverage.
        if not {0, 1, 3, 4, 5, 6}.issubset(states):
            raise ValueError(f'incomplete file-screen states: {sorted(states)}')
    elif scenario not in ('local_hint', 'menus'):
        expected = set(range({'credits': 9, 'prologue': 6, 'intro': 5}[scenario]))
        if installs != expected:
            raise ValueError(f'{scenario}: expected installations {sorted(expected)}, got {sorted(installs)}')
    terminal = {'credits': 'credits_save', 'prologue': 'prologue_clear',
                'intro': 'prologue_world', 'files': 'files_complete',
                'local_hint': 'local_hint_ready', 'menus': 'menus_complete'}[scenario]
    if not any(row['event'] == terminal for row in events):
        raise ValueError(f'{scenario}: missing {terminal}')
    # This is the particular bug the audit must expose: text does not clear art.
    if scenario == 'credits':
        if not any(row['event'] == 'credits_command' and row['value'] == 2 and row['installed'] >= 0
                   for row in events):
            raise ValueError('no text command retaining an installed illustration')
        if not any(row['event'] == 'credits_clear' for row in events):
            raise ValueError('no explicit credits clear')
    frames = [row for row in rows if 'event' not in row and row.get('capture')]
    if not frames:
        raise ValueError('no captured frames')
    if scenario == 'menus':
        validate_menus(events, frames)
    return events, frames


def summarize(directory, scenario):
    rows = [json.loads(line) for line in (directory / 'trace.jsonl').read_text().splitlines()]
    events, frames = validate_trace(rows, scenario)
    hashes = {}
    representatives = {}
    for frame in frames:
        filename = f"{frame['frame']:06d}.png"
        with Image.open(directory / filename) as image:
            image.load()
            if image.size != (240, 160):
                raise ValueError(f'{filename}: expected native 240x160, got {image.size}')
            hashes[filename] = hashlib.sha256(image.convert('RGBA').tobytes()).hexdigest()
        # Include transient menu states and both faded and neutral artwork.
        key = (frame['task'], frame['state'], frame['ui'], frame['menu'], frame['overlay'],
               frame['installed'], frame['fade_active'], frame.get('case', -1))
        representatives[key] = frame
    if scenario == 'local_hint':
        ready = [row for row in frames if row['ui'] == 10 and not row['fade_active']]
        if not ready:
            raise ValueError('no neutral local-hint capture')
        with Image.open(directory / f"{ready[-1]['frame']:06d}.png") as image:
            # Native BG3 is tile 1 / palette bank 5, filled with entry 95.
            # sub_080A6B04 sets it and palette zero to 0x475f. Bottom center
            # belongs to the map label, so use the two exposed border corners.
            if any(image.convert('RGB').getpixel(point) != (248, 208, 136)
                   for point in ((0, 159), (239, 159))):
                raise ValueError('local hint did not replace the health decoration with its solid field')
    selected = sorted(representatives.values(), key=lambda row: row['frame'])
    # Split sheets to keep every label/image readable at original resolution.
    for offset in range(0, len(selected), 48):
        batch = selected[offset:offset + 48]
        sheet = Image.new('RGB', (4 * 240, ((len(batch) + 3) // 4) * 194), '#202020')
        draw = ImageDraw.Draw(sheet)
        for index, frame in enumerate(batch):
            x, y = (index % 4) * 240, (index // 4) * 194
            with Image.open(directory / f"{frame['frame']:06d}.png") as image:
                sheet.paste(image, (x, y))
            draw.text((x + 2, y + 160), f"{frame['frame']} art={frame['installed']} "
                      f"state={frame['state']} ui={frame['ui']} menu={frame['menu']}/{frame['overlay']}\n"
                      f"fade={frame['fade_progress']} case={frame.get('case', -1)} "
                      f"decor={frame.get('decoration', -1)}", fill='white')
        sheet.save(directory / f'contact-{offset // 48 + 1:02d}.png')
    result = dict(events=events, captures=hashes, review_frames=[row['frame'] for row in selected])
    if scenario == 'menus':
        checkpoints = [row for row in events if row['event'] == 'menu_case_ready']
        for row in events:
            if row['event'] in ('menu_case_ready', 'menu_case_held'):
                with Image.open(directory / f"{row['frame']:06d}.png") as image:
                    if image.convert('RGB').getbbox() is None:
                        raise ValueError(f'case {row["value"]}: neutral menu checkpoint is blank')
        result['menu_cases'] = [dict(case, frame=row['frame']) for row, case in zip(checkpoints, menu_cases())]
        # A separate neutral matrix is easier to review than the transition sheets.
        for offset in range(0, len(checkpoints), 36):
            batch = checkpoints[offset:offset + 36]
            sheet = Image.new('RGB', (4 * 240, ((len(batch) + 3) // 4) * 184), '#202020')
            draw = ImageDraw.Draw(sheet)
            for index, row in enumerate(batch):
                x, y = (index % 4) * 240, (index // 4) * 184
                with Image.open(directory / f"{row['frame']:06d}.png") as image:
                    sheet.paste(image, (x, y))
                draw.text((x + 2, y + 160), menu_cases()[row['value']]['label'], fill='white')
            sheet.save(directory / f'menus-{offset // 36 + 1:02d}.png')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--scenario', choices=['credits', 'prologue', 'intro', 'files', 'local_hint', 'menus'], action='append')
    parser.add_argument('--language', type=int, choices=range(6), default=1,
                        help='preferred game language: 0 Japanese, 1 English, 2 French, 3 German, 4 Spanish, 5 Italian')
    parser.add_argument('--timeout', type=float, default=1200)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error('--timeout must be positive')
    binary, rom = args.binary.resolve(strict=True), args.rom.resolve(strict=True)
    with rom.open('rb') as file:
        file.seek(0xac)
        code = file.read(4).decode('ascii')
    if code not in ('BZME', 'BZMP', 'BZMJ'):
        parser.error('unsupported ROM game code')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    summary = dict(game_code=code, preferred_language=args.language,
                   rom_sha1=hashlib.sha1(rom.read_bytes()).hexdigest(),
                   executable_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(), scenarios={})
    for scenario in dict.fromkeys(args.scenario or ['credits', 'prologue', 'intro', 'files', 'local_hint', 'menus']):
        directory = output / scenario
        directory.mkdir()
        with tempfile.TemporaryDirectory(prefix='tmc-background-audit-') as temporary:
            config = dict(aspect_mode='native', internal_scale=1, upscale_method='nearest',
                          render_backend='software', color_correction=False, lcd_persistence=False,
                          hd_dialogue=False, menu_art_background=False, vsync=False, frame_time_ns=0,
                          decouple_render=False, ra_enabled=False, discord_rpc=False, autosave_enabled=False,
                          preferred_language=args.language)
            (Path(temporary) / 'config.json').write_text(json.dumps(config))
            env = {key: value for key, value in os.environ.items() if not key.startswith(('TMC_', 'SDL_'))}
            env.update(TMC_AUTOPLAY='1', TMC_BASEROM=str(rom), TMC_NO_UPDATE_CHECK='1',
                       TMC_BACKGROUND_AUDIT=scenario, TMC_BACKGROUND_AUDIT_OUT=str(directory),
                       SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SDL_RENDER_DRIVER='software',
                       OMP_NUM_THREADS='1', XDG_CONFIG_HOME=temporary, XDG_DATA_HOME=temporary)
            with (directory / 'run.log').open('wb') as log:
                subprocess.run([str(binary), '--no-audio'], cwd=temporary, env=env, stdout=log,
                               stderr=subprocess.STDOUT, timeout=args.timeout, check=True)
        summary['scenarios'][scenario] = summarize(directory, scenario)
        if scenario == 'menus':
            if code in ('BZME', 'BZMP'):
                summary['scenarios'][scenario]['inventory_pixels'] = validate_inventory_pixels(
                    directory, summary['scenarios'][scenario]['events'], menu_reference_rows(rom, args.language))
            else:
                summary['scenarios'][scenario]['inventory_pixels'] = dict(
                    unavailable='The ROM exporter has no JP reference recipes yet.')
        if scenario == 'files' and code == 'BZMP' and not any(
                row['event'] == 'file_state' and row['value'] == 2
                for row in summary['scenarios'][scenario]['events']):
            raise ValueError('EU file route did not reach language selection')
        (output / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
        print(f'{scenario}: {len(summary["scenarios"][scenario]["captures"])} frames; {directory}')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        raise SystemExit(f'Background audit failed: {error}. Inspect the output directory logs.') from error
