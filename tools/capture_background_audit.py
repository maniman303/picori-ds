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

from background_travel_actions_audit import (travel_action_cases, validate_travel_actions,
                                            travel_action_reference, validate_travel_action_pixels)
from background_fusion_actions_audit import (fusion_action_cases, validate_fusion_actions,
                                             fusion_reference_rows, validate_fusion_action_pixels)
from background_gameover_audit import validate_gameover, validate_gameover_pixels
from background_save_audit import save_cases, validate_save, validate_save_pixels
from background_sleep_audit import sleep_cases, validate_sleep, validate_sleep_pixels
from background_detail_map_audit import detail_map_cases, validate_detail_maps, validate_detail_map_pixels
from background_dungeon_audit import dungeon_cases, validate_dungeons, validate_dungeon_pixels
from background_hint_audit import hint_cases, validate_hints, validate_hint_pixels, hint_reference
from background_figurine_actions_audit import (figurine_action_cases, figurine_action_start, validate_figurine_actions,
                                              validate_figurine_action_pixels)
from background_figurine_audit import figurine_cases, validate_figurines, validate_figurine_pixels, figurine_reference
from background_travel_audit import travel_cases, validate_travel, travel_reference_row, validate_travel_pixels


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
    for decoration, health in enumerate(('normal', 'damaged', 'critical')):
        for label, screen in screens:
            cases.append(dict(label=f'{health}_{label}', decoration=decoration, screen=screen, map_group=None))
        for label, group in maps:
            cases.append(dict(label=f'{health}_{label}', decoration=decoration, screen=6, map_group=group))
    for decoration, health in enumerate(('normal', 'damaged', 'critical')):
        cases.append(dict(label=f'{health}_fusion', decoration=decoration, screen=-1, map_group=None,
                          health=(24, 15, 8)[decoration], max_health=24))
    # Keep the original 109 case IDs stable for retained capture comparisons.
    for decoration, health in enumerate(('normal', 'damaged', 'critical')):
        for layout in range(7):
            cases.append(dict(label=f'{health}_fusion_layout_{layout}', decoration=decoration,
                              screen=-1, map_group=None, fusion_layout=layout,
                              health=(24, 15, 8)[decoration], max_health=24))
    for case in cases:
        screen = case['screen']
        case['family'] = ('fusion' if screen == -1 else 'pieces' if screen == 7 else
                          'techniques' if screen == 8 else 'plain' if screen in (9, 10, 11) else 'titled')
    return cases


def validate_menus(events, frames):
    cases = menu_cases()
    by_frame = {row['frame']: row for row in frames}
    if [row['value'] for row in events if row['event'] == 'menus_complete'] != [len(cases)]:
        raise ValueError('incorrect menu completion count')
    if [row['value'] for row in events if row['event'] == 'pause_screen'] != [
            case['screen'] for case in cases if case['screen'] >= 0]:
        raise ValueError('incorrect native screen installation sequence')
    fusion = [row for row in events if row['event'] == 'fusion_screen']
    if len(fusion) != 24:
        raise ValueError('incomplete native fusion installations')
    if [row['value'] for row in fusion[3:]] != list(range(7)) * 3:
        raise ValueError('incorrect native fusion layout installation sequence')
    openings = [row for row in events if row['event'] == 'menu_decoration']
    # Ordinary screen changes must not reinstall the decoration, even when
    # current health would select another variant on a fresh opening.
    if [row['value'] for row in openings] != [0, 0, 1, 1, 2, 2, 2, 0, 1, 2, 0, 1, 2] + [i for i in range(3) for _ in range(7)]:
        raise ValueError('incorrect decoration selection or unexpected reinstallation')
    initial = [(24, 24), (16, 24), (15, 24), (9, 24), (8, 24), (1, 24), (8, 8),
               (24, 24), (15, 24), (8, 24), (24, 24), (15, 24), (8, 24)]
    initial += [(health, 24) for health in (24, 15, 8) for _ in range(7)]
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
                if evidence.get('schema') != 2 or evidence.get('family') != case['family']:
                    raise ValueError(f'{case["label"]}: wrong semantic family/schema')
                validate_heading(evidence, neutral=True)
                if 'fusion_layout' in case and evidence.get('fusion_layout') != case['fusion_layout']:
                    raise ValueError(f'{case["label"]}: wrong installed fusion layout')
                if case['map_group'] is not None and evidence['map_group'] != case['map_group']:
                    raise ValueError(f'{case["label"]}: wrong native detail-map recipe')
            if phase == 'menu_case_ready' and 'health' in case and (
                    row['health'], row['max_health']) != (case['health'], case['max_health']):
                raise ValueError(f'{case["label"]}: incorrect health fixture')
            if phase == 'menu_case_held' and row['health'] != (24 if case['decoration'] == 2 else 8):
                raise ValueError(f'{case["label"]}: health mutation was not exercised')


def validate_heading(row, neutral=False):
    family = row.get('family')
    style = 0 if family == 'plain' else 2 if family in ('pieces', 'techniques') else 1
    transition = row.get('heading_transition', -1)
    if not 0 <= transition <= 16 or (neutral and transition != 0):
        raise ValueError('incorrect heading transition pose')
    x, y = ((0, 0) if style == 0 else (96, 24) if style == 2 else
            (64, 16 - transition * (1 if family == 'fusion' else 2)))
    if (row.get('heading_style'), row.get('heading_x'), row.get('heading_y')) != (style, x, y):
        raise ValueError('incorrect heading style/pose')


def validate_family_timeline(rows):
    """Requesting a screen must retain outgoing identity until installation.
    Native installation events, not pending screen IDs, change observations.
    """
    installed = 'unknown'
    opening_cases = {index for index, case in enumerate(menu_cases())
                     if 'health' in case or case['label'] in
                     ('normal_inventory', 'damaged_inventory', 'critical_inventory')}
    pairs, switches, animated = set(), set(), set()
    previous = None
    fusion_layouts = set()
    for row in rows:
        event = row.get('event')
        if event == 'menus_complete' or (event == 'menu_case_request' and row['value'] in opening_cases):
            # The previous opening has restored gameplay before this request.
            installed = 'unknown'
        elif event == 'pause_screen':
            screen = row['value']
            installed = ('pieces' if screen == 7 else 'techniques' if screen == 8 else
                         'plain' if screen in (9, 10, 11) else 'titled')
        elif event == 'fusion_screen':
            installed = 'fusion'
        elif 'event' not in row and row.get('family') == 'unknown':
            installed = 'unknown'
        if row.get('schema') != 2 or row.get('family') != installed:
            raise ValueError('semantic family changed outside native installation boundary')
        if 'event' not in row and installed != 'unknown':
            validate_heading(row)
            if installed == 'titled':
                animated.add(row['heading_transition'])
        if event == 'menu_case_ready':
            current = (row['family'], row['decoration'])
            pairs.add(current)
            if row['value'] >= 109:
                fusion_layouts.add((row['decoration'], row['fusion_layout']))
            if previous and previous[1] == current[1] and previous[0] != current[0]:
                switches.add((previous[0], current[0], current[1]))
            previous = current
    if pairs != {(family, health) for family in ('plain', 'titled', 'pieces', 'techniques', 'fusion')
                 for health in range(3)}:
        raise ValueError('incomplete 15 family/health combinations')
    for health in range(3):
        for source, target in (('titled', 'plain'), ('plain', 'titled'), ('titled', 'pieces'),
                               ('pieces', 'techniques'), ('techniques', 'titled')):
            if (source, target, health) not in switches:
                raise ValueError('missing same-health family transition')
    if not {0, 16}.issubset(animated) or not any(0 < value < 16 for value in animated):
        raise ValueError('missing animated heading poses')
    if fusion_layouts != {(health, layout) for health in range(3) for layout in range(7)}:
        raise ValueError('incomplete 21 fusion layout/health combinations')
    return dict(family_health_combinations=len(pairs),
                fusion_layout_health_combinations=len(fusion_layouts),
                same_health_switches=[list(value) for value in sorted(switches)],
                titled_transition_poses=sorted(animated))


def validate_panel_pixels(directory, events, references):
    """Only the independently decoded static BG2 lower panel is a valid mask.
    Upper heading/lettering and OBJ controls are deliberately excluded.
    """
    checked = 0
    for row in events:
        if row['event'] not in ('menu_case_ready', 'menu_case_held') or row.get('family') not in ('pieces', 'techniques'):
            continue
        with Image.open(directory / f"{row['frame']:06d}.png") as image:
            actual = image.convert('RGB').crop((0, 64, 240, 160)).tobytes()
        if actual != references[(row['family'], row['decoration'])]:
            raise ValueError('pieces/techniques pixels disagree with native static panel')
        checked += 1
    if checked != 12:
        raise ValueError('incomplete pieces/techniques pixel checkpoints')
    return dict(checkpoints=checked, pixels_per_checkpoint=240 * 96,
                mask=[0, 64, 240, 160])


def panel_references(rom, language):
    from export_screen_backgrounds import Rom, menu_recipes
    result = {}
    for name, scene, layers, backdrop in menu_recipes(Rom(rom, language)):
        for stem, family in (('kinstone_pieces_background_', 'pieces'), ('sword_techniques_background_', 'techniques')):
            if name.startswith(stem):
                health = ('normal', 'damaged', 'critical').index(name[len(stem):])
                image = Image.frombytes('RGB', (240, 160), scene.render(layers, backdrop))
                result[family, health] = image.crop((0, 64, 240, 160)).tobytes()
    return result


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


def compare_menu_baseline(directory, events, baseline):
    """Compare retained native checkpoints without upgrading them to goldens.
    Old v1 traces intentionally remain readable only as pixel baselines.
    """
    old_rows = [json.loads(line) for line in (baseline / 'trace.jsonl').read_text().splitlines()]
    compared = 0
    for phase in ('menu_case_ready', 'menu_case_held'):
        old = [row for row in old_rows if row.get('event') == phase]
        new = [row for row in events if row.get('event') == phase]
        if len(new) < len(old):
            raise ValueError('missing new checkpoints for native baseline comparison')
        if len(old) not in (106, 109, 130) or [row['value'] for row in old] != list(range(len(old))):
            raise ValueError('baseline must contain all 106 pause, 109 family or 130 fusion native menu cases')
        for before, after in zip(old, new):
            if before['value'] != after['value']:
                raise ValueError('baseline checkpoint order differs')
            with Image.open(baseline / f"{before['frame']:06d}.png") as image:
                before_pixels = image.convert('RGBA').tobytes()
                before_size = image.size
            with Image.open(directory / f"{after['frame']:06d}.png") as image:
                if image.size != before_size or image.convert('RGBA').tobytes() != before_pixels:
                    raise ValueError(f'case {after["value"]}: native pixels differ from retained baseline')
            compared += 1
    return dict(directory=str(baseline), checkpoints=compared)


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
    if scenario not in ('menus', 'dungeons', 'detail_maps', 'sleep', 'save', 'inventory_save', 'fusion_actions') and any(row.get('family', 'unknown') != 'unknown' for row in rows):
        raise ValueError('non-menu route claimed a pause/fusion artwork family')
    events = [row for row in rows if 'event' in row]
    install_event = 'credits_install' if scenario == 'credits' else 'prologue_install'
    installs = {row['value'] for row in events if row['event'] == install_event}
    if scenario == 'files':
        states = {row['value'] for row in events if row['event'] == 'file_state'}
        # Language selection is EU-only. All runs must visit the six other
        # screens through native input before claiming file-operation coverage.
        if not {0, 1, 3, 4, 5, 6}.issubset(states):
            raise ValueError(f'incomplete file-screen states: {sorted(states)}')
    elif scenario not in ('local_hint', 'menus', 'travel', 'figurines', 'hints', 'dungeons', 'travel_actions', 'figurine_actions', 'detail_maps', 'sleep', 'save', 'inventory_save', 'gameover', 'fusion_actions'):
        expected = set(range({'credits': 9, 'prologue': 6, 'intro': 5}[scenario]))
        if installs != expected:
            raise ValueError(f'{scenario}: expected installations {sorted(expected)}, got {sorted(installs)}')
    terminal = {'credits': 'credits_save', 'prologue': 'prologue_clear',
                'intro': 'prologue_world', 'files': 'files_complete',
                'local_hint': 'local_hint_ready', 'menus': 'menus_complete', 'travel': 'travel_complete', 'figurines': 'figurines_complete', 'hints': 'hints_complete', 'dungeons': 'dungeons_complete',
                'travel_actions': 'travel_actions_complete', 'figurine_actions': 'figurine_actions_complete', 'detail_maps': 'detail_maps_complete', 'sleep': 'sleep_complete', 'save': 'save_complete', 'inventory_save': 'save_complete', 'gameover': 'gameover_complete', 'fusion_actions': 'fusion_actions_complete'}[scenario]
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
        validate_family_timeline(rows)
    if scenario == 'travel':
        validate_travel(rows, events, frames)
    if scenario == 'figurines':
        validate_figurines(rows, events, frames)
    if scenario == 'hints':
        validate_hints(rows, events, frames)
    if scenario == 'fusion_actions':
        validate_fusion_actions(rows, events, frames)
    if scenario == 'gameover':
        validate_gameover(rows, events, frames)
    if scenario in ('save', 'inventory_save'):
        validate_save(rows, events, frames, inventory=scenario == "inventory_save")
    if scenario == 'sleep':
        validate_sleep(rows, events, frames)
    if scenario == 'detail_maps':
        validate_detail_maps(rows, events, frames)
    if scenario == 'dungeons':
        validate_dungeons(rows, events, frames)
    if scenario == 'travel_actions':
        validate_travel_actions(rows, events, frames)
    if scenario == 'figurine_actions':
        validate_figurine_actions(rows, events, frames)
    return events, frames


def write_native_matrix(directory, events, phase, cases, prefix):
    phases = (phase,) if isinstance(phase, str) else phase
    checkpoints = [row for row in events if row['event'] in phases]
    for offset in range(0, len(checkpoints), 36):
        batch = checkpoints[offset:offset + 36]
        sheet = Image.new('RGB', (4 * 240, ((len(batch) + 3) // 4) * 184), '#202020')
        draw = ImageDraw.Draw(sheet)
        for index, row in enumerate(batch):
            x, y = index % 4 * 240, index // 4 * 184
            with Image.open(directory / f"{row['frame']:06d}.png") as image:
                sheet.paste(image, (x, y))
            case = cases[offset + index]
            if prefix.startswith(('sleep', 'save')):
                label = f"health={case['decoration']} {case['label']}"
            elif prefix.startswith('fusion-actions'):
                label = f"health={case['decoration']} {case['label']}"
            elif prefix.startswith('detail-maps'):
                label = f"health={case['decoration']} area={case['area']} group={case['map_group']}"
            elif prefix == 'hints':
                label = f"health={case['decoration']} repeat={case['repeat']} hint={case['variant']}"
            elif prefix.startswith('figurine-actions'):
                label = f"health={case['decoration']} post={case['staffroll']} fig={case['index']} dup={bool(case['duplicate'])}"
            elif prefix.startswith('travel-actions'):
                label = f"health={case['decoration']} dest={case['destination']} accept={case['accept']}"
            else:
                label = f"d={case['dungeon']} health={case['decoration']} items={case['items']} floor={case['floor']}"
            draw.text((x + 2, y + 160), label, fill='white')
        sheet.save(directory / f'{prefix}-neutral-{offset // 36 + 1:02d}.png')


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
               frame['installed'], frame['fade_active'], frame.get('case', -1),
               frame.get('family'), frame.get('heading_style'), frame.get('heading_y'),
               frame.get('travel_case', -1), frame.get('travel_destination'),
               frame.get('figurine_case', -1), frame.get('figurine_index'), frame.get('hint_case', -1), frame.get('dungeon_case', -1), frame.get('travel_action_case', -1), frame.get('figurine_action_case', -1), frame.get('detail_map_case', -1), frame.get('sleep_case', -1))
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
                      f"{frame.get('family', 'unknown')} h={frame.get('decoration', -1)} "
                      f"pose={frame.get('heading_x', 0)},{frame.get('heading_y', 0)} "
                      f"travel={frame.get('travel_case', -1)}/{frame.get('travel_destination', -1)}", fill='white')
        sheet.save(directory / f'contact-{offset // 48 + 1:02d}.png')
    result = dict(events=events, captures=hashes, review_frames=[row['frame'] for row in selected])
    if scenario == 'travel':
        result['travel_cases'] = travel_cases()
        result['travel_checkpoints'] = sum(row['event'] in ('travel_case_ready', 'travel_case_held') for row in events)
        checkpoints = [row for row in events if row['event'] == 'travel_case_ready']
        sheet = Image.new('RGB', (4 * 240, ((len(checkpoints) + 3) // 4) * 184), '#202020')
        draw = ImageDraw.Draw(sheet)
        for index, row in enumerate(checkpoints):
            x, y = index % 4 * 240, index // 4 * 184
            with Image.open(directory / f"{row['frame']:06d}.png") as image:
                sheet.paste(image, (x, y))
            draw.text((x + 2, y + 160), travel_cases()[index]['label'], fill='white')
        sheet.save(directory / 'travel-neutral.png')
    if scenario == 'fusion_actions':
        result['fusion_cases'] = fusion_action_cases()
        for phase in ('ready', 'held'):
            write_native_matrix(directory, events, f'fusion_action_{phase}', fusion_action_cases(), f'fusion-actions-{phase}')
    if scenario in ('save', 'inventory_save'):
        result['save_cases'] = save_cases()
        for phase in ('ready', 'held', 'inventory_returned' if scenario == 'inventory_save' else 'quest_returned'):
            write_native_matrix(directory, events, f'save_{phase}', save_cases(), f'save-{phase}')
    if scenario == 'sleep':
        result['sleep_cases'] = sleep_cases()
        for phase in ('ready', 'held', 'quest_returned'):
            write_native_matrix(directory, events, f'sleep_{phase}', sleep_cases(), f'sleep-{phase}')
    if scenario == 'detail_maps':
        result['detail_map_cases'] = detail_map_cases()
        for phase in ('ready', 'top', 'bottom', 'top_returned'):
            write_native_matrix(directory, events, f'detail_{phase}', detail_map_cases(), f'detail-maps-{phase}')
    if scenario == 'figurine_actions':
        cases = figurine_action_cases()[figurine_action_start(events):]
        result['figurine_action_cases'] = cases
        for phase in ('ready', 'bottom', 'top'):
            write_native_matrix(directory, events, f'figurine_action_{phase}', cases,
                                f'figurine-actions-{phase}')
    if scenario == 'travel_actions':
        for row in events:
            if row['event'] in ('travel_action_arrived', 'travel_action_restored'):
                with Image.open(directory / f"{row['frame']:06d}.png") as image:
                    if image.convert('RGB').getbbox() is None:
                        raise ValueError('completed travel gameplay capture is blank')
        result['travel_action_cases'] = travel_action_cases()
        write_native_matrix(directory, events, 'travel_action_prompt_ready', travel_action_cases(), 'travel-actions-prompts')
        write_native_matrix(directory, events, ('travel_action_arrived', 'travel_action_restored'),
                            travel_action_cases(), 'travel-actions-results')
    if scenario == 'dungeons':
        result['dungeon_cases'] = dungeon_cases()
        write_native_matrix(directory, events, 'dungeon_ready', dungeon_cases(), 'dungeons')
    if scenario == 'hints':
        result['hint_cases'] = hint_cases()
        write_native_matrix(directory, events, 'hint_ready', hint_cases(), 'hints')
        result['native_field_pixels'] = validate_hint_pixels(directory, events)
    if scenario == 'figurines':
        result['figurine_cases'] = figurine_cases()
        result['native_field_pixels'] = validate_figurine_pixels(directory, events)
        checkpoints = [row for row in events if row['event'] == 'figurine_ready']
        sheet = Image.new('RGB', (4 * 240, ((len(checkpoints) + 3) // 4) * 184), '#202020')
        draw = ImageDraw.Draw(sheet)
        for index, row in enumerate(checkpoints):
            x, y = index % 4 * 240, index // 4 * 184
            with Image.open(directory / f"{row['frame']:06d}.png") as image:
                sheet.paste(image, (x, y))
            case = figurine_cases()[index]
            draw.text((x + 2, y + 160), f"health={case['decoration']} post={case['staffroll']} "
                      f"figure={case['index']} owned={case['owned']}", fill='white')
        sheet.save(directory / 'figurines-neutral.png')
    if scenario == 'menus':
        checkpoints = [row for row in events if row['event'] == 'menu_case_ready']
        for row in events:
            if row['event'] in ('menu_case_ready', 'menu_case_held'):
                with Image.open(directory / f"{row['frame']:06d}.png") as image:
                    if image.convert('RGB').getbbox() is None:
                        raise ValueError(f'case {row["value"]}: neutral menu checkpoint is blank')
        result['semantic_coverage'] = validate_family_timeline(rows)
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
    parser.add_argument('--scenario', choices=['credits', 'prologue', 'intro', 'files', 'local_hint', 'menus', 'travel', 'figurines', 'hints', 'dungeons', 'travel_actions', 'figurine_actions', 'detail_maps', 'sleep', 'save', 'inventory_save', 'gameover', 'fusion_actions'], action='append')
    parser.add_argument('--language', type=int, choices=range(6), default=1,
                        help='preferred game language: 0 Japanese, 1 English, 2 French, 3 German, 4 Spanish, 5 Italian')
    parser.add_argument('--menu-baseline', type=Path, help='retained 106/109/130-case menus directory for pixel comparison')
    parser.add_argument('--figurine-start-case', type=int, choices=range(54), default=0,
                        help='start an independent figurine_actions suffix at this canonical case ID')
    parser.add_argument('--timeout', type=float,
                        help='seconds per scenario (default: 2400 for figurine_actions/detail_maps, otherwise 1200)')
    args = parser.parse_args()
    if args.timeout is not None and args.timeout <= 0:
        parser.error('--timeout must be positive')
    if args.figurine_start_case and args.scenario != ['figurine_actions']:
        parser.error('--figurine-start-case requires only --scenario figurine_actions')
    binary, rom = args.binary.resolve(strict=True), args.rom.resolve(strict=True)
    with rom.open('rb') as file:
        file.seek(0xac)
        code = file.read(4).decode('ascii')
    if code not in ('BZME', 'BZMP', 'BZMJ'):
        parser.error('unsupported ROM game code')
    if args.scenario and any(s in args.scenario for s in ('travel_actions', 'sleep', 'save', 'inventory_save', 'gameover', 'fusion_actions')) and code != 'BZMP':
        parser.error('travel_actions, sleep, save, inventory_save, gameover and fusion_actions currently cover EU native interaction behavior')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    summary = dict(game_code=code, preferred_language=args.language,
                   rom_sha1=hashlib.sha1(rom.read_bytes()).hexdigest(),
                   executable_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(), scenarios={})
    for scenario in dict.fromkeys(args.scenario or ['credits', 'prologue', 'intro', 'files', 'local_hint', 'menus', 'travel', 'figurines', 'hints', 'dungeons', 'figurine_actions', 'detail_maps'] + (['travel_actions', 'sleep', 'save', 'inventory_save', 'gameover', 'fusion_actions'] if code == 'BZMP' else [])):
        directory = output / scenario
        directory.mkdir()
        with tempfile.TemporaryDirectory(prefix='tmc-background-audit-') as temporary:
            config = dict(aspect_mode='native', window_scale=1, internal_scale=1, upscale_method='nearest',
                          render_backend='software', color_correction=False, lcd_persistence=False,
                          hd_dialogue=False, menu_art_background=False, vsync=False, frame_time_ns=0,
                          decouple_render=False, ra_enabled=False, discord_rpc=False, autosave_enabled=False,
                          preferred_language=args.language)
            (Path(temporary) / 'config.json').write_text(json.dumps(config))
            env = {key: value for key, value in os.environ.items() if not key.startswith(('TMC_', 'SDL_'))}
            env.update(TMC_AUTOPLAY='1', TMC_BASEROM=str(rom), TMC_NO_UPDATE_CHECK='1',
                       TMC_BACKGROUND_AUDIT=scenario, TMC_BACKGROUND_AUDIT_OUT=str(directory),
                       TMC_FIGURINE_ACTION_START=str(args.figurine_start_case),
                       SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SDL_RENDER_DRIVER='software',
                       OMP_NUM_THREADS='1', XDG_CONFIG_HOME=temporary, XDG_DATA_HOME=temporary)
            with (directory / 'run.log').open('wb') as log:
                subprocess.run([str(binary), '--no-audio'], cwd=temporary, env=env, stdout=log,
                               stderr=subprocess.STDOUT,
                               timeout=args.timeout if args.timeout is not None else (2400 if scenario in ('figurine_actions', 'detail_maps') else 1200),
                               check=True)
        summary['scenarios'][scenario] = summarize(directory, scenario)
        if scenario == 'menus':
            if args.menu_baseline:
                summary['scenarios'][scenario]['native_baseline'] = compare_menu_baseline(
                    directory, summary['scenarios'][scenario]['events'], args.menu_baseline.resolve(strict=True))
            if code in ('BZME', 'BZMP'):
                summary['scenarios'][scenario]['panel_pixels'] = validate_panel_pixels(
                    directory, summary['scenarios'][scenario]['events'], panel_references(rom, args.language))
                summary['scenarios'][scenario]['inventory_pixels'] = validate_inventory_pixels(
                    directory, summary['scenarios'][scenario]['events'], menu_reference_rows(rom, args.language))
            else:
                summary['scenarios'][scenario]['inventory_pixels'] = dict(
                    unavailable='The ROM exporter has no JP reference recipes yet.')
        if scenario == 'travel_actions':
            summary['scenarios'][scenario]['native_field_pixels'] = validate_travel_action_pixels(
                directory, summary['scenarios'][scenario]['events'], travel_action_reference(rom, args.language))
        if scenario == 'fusion_actions':
            summary['scenarios'][scenario]['native_field_pixels'] = validate_fusion_action_pixels(
                directory, summary['scenarios'][scenario]['events'], fusion_reference_rows(rom, args.language))
        if scenario == 'gameover':
            summary['scenarios'][scenario]['native_field_pixels'] = validate_gameover_pixels(
                directory, summary['scenarios'][scenario]['events'])
        if scenario in ('save', 'inventory_save'):
            summary['scenarios'][scenario]['native_field_pixels'] = validate_save_pixels(
                directory, summary['scenarios'][scenario]['events'], menu_reference_rows(rom, args.language),
                inventory=scenario == 'inventory_save')
        if scenario == 'sleep' and code in ('BZME', 'BZMP'):
            summary['scenarios'][scenario]['native_field_pixels'] = validate_sleep_pixels(
                directory, summary['scenarios'][scenario]['events'], menu_reference_rows(rom, args.language))
        if scenario == 'detail_maps' and code in ('BZME', 'BZMP'):
            summary['scenarios'][scenario]['native_map_pixels'] = validate_detail_map_pixels(
                directory, summary['scenarios'][scenario]['events'], rom, args.language)
        if scenario == 'dungeons' and code in ('BZME', 'BZMP'):
            summary['scenarios'][scenario]['native_field_pixels'] = validate_dungeon_pixels(
                directory, summary['scenarios'][scenario]['events'], menu_reference_rows(rom, args.language))
        if scenario == 'hints' and code in ('BZME', 'BZMP'):
            summary['scenarios'][scenario]['native_field_pixels'] = validate_hint_pixels(
                directory, summary['scenarios'][scenario]['events'], hint_reference(rom, args.language))
        if scenario == 'figurine_actions' and code in ('BZME', 'BZMP'):
            summary['scenarios'][scenario]['native_field_pixels'] = validate_figurine_action_pixels(
                directory, summary['scenarios'][scenario]['events'], figurine_reference(rom, args.language))
        if scenario == 'figurines' and code in ('BZME', 'BZMP'):
            summary['scenarios'][scenario]['native_field_pixels'] = validate_figurine_pixels(
                directory, summary['scenarios'][scenario]['events'], figurine_reference(rom, args.language))
        if scenario == 'travel':
            summary['scenarios'][scenario]['native_field_pixels'] = (
                validate_travel_pixels(directory, summary['scenarios'][scenario]['events'],
                                       travel_reference_row(rom, args.language))
                if code in ('BZME', 'BZMP') else dict(unavailable='No JP ROM decoder recipe.'))
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
