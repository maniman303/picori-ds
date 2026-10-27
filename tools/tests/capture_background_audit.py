#!/usr/bin/env python3
"""ROM-free failure/coverage checks for the native state-audit review tool."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from capture_background_audit import (menu_cases, menu_case_ids, summarize, validate_trace, validate_language, validate_inventory_pixels,
                                      validate_panel_pixels, compare_menu_baseline, travel_cases, validate_travel_pixels)


def frame(number, ui=0):
    return dict(frame=number, capture=True, task=1, state=1, ui=ui, menu=1,
                overlay=0, installed=-1, fade_active=0, fade_progress=0,
                fade_mask=0xffffffff)


def menu_trace():
    rows = []
    for health, maximum, decoration in ((24, 24, 0), (16, 24, 0), (15, 24, 1), (9, 24, 1),
                                       (8, 24, 2), (1, 24, 2), (8, 8, 2),
                                       (24, 24, 0), (15, 24, 1), (8, 24, 2),
                                       (24, 24, 0), (15, 24, 1), (8, 24, 2)):
        rows.append(dict(event='menu_decoration', value=decoration, health=health, max_health=maximum,
                         schema=2, family='unknown'))
    for decoration, health in enumerate((24, 15, 8)):
        rows.extend(dict(event='menu_decoration', value=decoration, health=health, max_health=24,
                         schema=2, family='unknown') for _ in range(7))
    for index, case in enumerate(menu_cases()):
        rows.append(dict(event='fusion_screen' if case['screen'] == -1 else 'pause_screen',
                         value=case.get('fusion_layout', 0) if case['screen'] == -1 else case['screen'],
                         family=case['family'], schema=2))
        for phase in ('menu_case_ready', 'menu_case_held'):
            number = len(rows)
            sample = dict(frame(number, 1), case=index, decoration=case['decoration'],
                          pause_screen=case['screen'], map_group=case['map_group'],
                          fusion_layout=case.get('fusion_layout', 0) if case['screen'] == -1 else -1,
                          health=case.get('health', 24), max_health=case.get('max_health', 24))
            if phase.endswith('held'):
                sample['health'] = 24 if case['decoration'] == 2 else 8
                sample['max_health'] = 24
            style = 0 if case['family'] == 'plain' else 2 if case['family'] in ('pieces', 'techniques') else 1
            sample.update(schema=2, family=case['family'], heading_style=style, heading_transition=0,
                          heading_x=(0, 64, 96)[style], heading_y=(0, 16, 24)[style])
            event = dict(sample, event=phase, value=index)
            del event['capture']
            rows += [event, sample]
    rows.append(dict(event='menus_complete', value=len(menu_cases()), family='unknown', schema=2))
    index = next(i for i, row in enumerate(rows) if row.get('event') == 'menu_case_ready')
    for transition in (0, 8, 16):
        rows.insert(index, dict(frame(-transition - 1), family='titled', schema=2, heading_style=1,
                                heading_x=64, heading_y=16 - 2 * transition, heading_transition=transition))
    return rows


def travel_trace():
    rows = []
    for index, case in enumerate(travel_cases()):
        sample = dict(frame(0, 9), task=2, substate=7, travel_case=index, schema=2,
                      decoration=case['decoration'], pause_screen=13,
                      travel_destination=0 if case['unlocks'] == 255 else 7,
                      travel_unlocks=case['unlocks'], health=case['health'], max_health=24,
                      travel_field=0, display_control=0x1f40, heading_style=0, heading_x=0, heading_y=0,
                      heading_transition=0, family='unknown')

        def event(name, value, capture=False):
            sample['frame'] = len(rows)
            row = dict(sample, event=name, value=value)
            row.pop('capture')
            rows.append(row)
            if capture:
                rows.append(dict(sample))

        event('travel_case_request', index)
        event('menu_decoration', case['decoration'])
        event('pause_screen', 13)
        sample['travel_field'] = 1
        event('fast_travel_screen', sample['travel_destination'])
        if case['unlocks'] == 255:
            for choice in range(1, case['destination'] + 1):
                sample['travel_destination'] = choice
                event('fast_travel_choice', choice)
        event('travel_case_ready', index, True)
        sample['health'] = 24 if case['decoration'] == 2 else 8
        event('travel_health_mutated', sample['health'])
        event('travel_case_held', index, True)
        event('fast_travel_cancel', case['destination'])
        sample['travel_field'] = 0
        event('travel_case_restored', index)
    rows.append(dict(event='travel_complete', value=27, travel_case=27, family='unknown'))
    return rows


class AuditCoverage(unittest.TestCase):
    def test_travel_covers_all_destinations_and_sparse_unlock_at_each_health(self):
        self.assertEqual(len(travel_cases()), 27)
        for health in range(3):
            group = travel_cases()[health * 9:(health + 1) * 9]
            self.assertEqual([case['destination'] for case in group], list(range(8)) + [7])
            self.assertEqual([case['unlocks'] for case in group], [255] * 8 + [128])
        validate_trace(travel_trace(), 'travel')

    def test_travel_checks_both_checkpoint_and_capture_state(self):
        for event in ('travel_case_ready', 'travel_case_held', None):
            for field, value in (('travel_destination', 6), ('travel_unlocks', 1),
                                 ('decoration', 0)):
                rows = travel_trace()
                sample = next(row for row in rows if row.get('event') == event and row.get('travel_case') == 26)
                sample[field] = value
                with self.assertRaisesRegex(ValueError, 'wrong installed travel'):
                    validate_trace(rows, 'travel')

    def test_travel_requires_native_installation_input_cancel_and_restore(self):
        for event, message in (('fast_travel_screen', 'installation'),
                               ('fast_travel_choice', 'cursor sequence'),
                               ('fast_travel_cancel', 'cancellation'),
                               ('travel_case_restored', 'incomplete or unordered')):
            rows = travel_trace()
            index = next(i for i, row in enumerate(rows) if row.get('event') == event)
            rows.pop(index)
            with self.assertRaisesRegex(ValueError, message):
                validate_trace(rows, 'travel')
        rows = travel_trace()
        next(row for row in rows if row.get('event') == 'fast_travel_screen')['value'] = 7
        with self.assertRaisesRegex(ValueError, 'initial unlocked destination'):
            validate_trace(rows, 'travel')

    def test_travel_reused_initializer_does_not_publish_family_early(self):
        rows = travel_trace()
        next(row for row in rows if row.get('event') == 'pause_screen')['travel_field'] = 1
        with self.assertRaisesRegex(ValueError, 'outside native installation'):
            validate_trace(rows, 'travel')
        rows = travel_trace()
        next(row for row in rows if row.get('event') == 'fast_travel_cancel')['travel_field'] = 0
        with self.assertRaisesRegex(ValueError, 'outside native installation'):
            validate_trace(rows, 'travel')

    def test_travel_requires_neutral_retained_decoration_and_absent_heading(self):
        for field, value, message in (('fade_active', 1, 'neutral travel'),
                                      ('health', 24, 'health mutation'),
                                      ('heading_style', 1, 'heading style/pose'),
                                      ('display_control', 0x1740, 'native BG3')):
            rows = travel_trace()
            row = next(row for row in rows if row.get('event') == 'travel_case_held') if field == 'health' else next(row for row in rows if row.get('event') is None)
            row[field] = value
            with self.assertRaisesRegex(ValueError, message):
                validate_trace(rows, 'travel')
        rows = travel_trace()
        index = next(i for i, row in enumerate(rows) if row.get('event') == 'menu_decoration')
        rows.insert(index, dict(rows[index]))
        with self.assertRaisesRegex(ValueError, 'unexpected reinstallation'):
            validate_trace(rows, 'travel')

    def test_travel_cannot_alias_plain_menu_art(self):
        rows = travel_trace()
        next(row for row in rows if row.get('event') == 'fast_travel_screen')['family'] = 'plain'
        with self.assertRaisesRegex(ValueError, 'non-menu route'):
            validate_trace(rows, 'travel')

    def test_travel_pixel_mask_and_corruption(self):
        events = [dict(event=phase, frame=1) for phase in ('travel_case_ready', 'travel_case_held')
                  for _ in range(27)]
        reference = bytes([96] * 720)
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            image = Image.new('RGB', (240, 160), (96, 96, 96))
            image.putpixel((120, 100), (0, 0, 0))  # Live map/marker content is outside this mask.
            image.save(directory / '000001.png')
            self.assertEqual(validate_travel_pixels(directory, events, reference)['checkpoints'], 54)
            image.putpixel((0, 159), (0, 0, 0))
            image.save(directory / '000001.png')
            with self.assertRaisesRegex(ValueError, 'native fixed BG3'):
                validate_travel_pixels(directory, events, reference)

    def test_retained_v1_baseline_compares_native_pixels(self):
        events = [dict(event=phase, value=index, frame=1) for phase in ('menu_case_ready', 'menu_case_held')
                  for index in range(106)]
        with tempfile.TemporaryDirectory() as temporary:
            baseline, current = Path(temporary) / 'old', Path(temporary) / 'new'
            baseline.mkdir()
            current.mkdir()
            trace = '\n'.join(json.dumps(row) for row in events)
            (baseline / 'trace.jsonl').write_text(trace)
            for directory in (baseline, current):
                Image.new('RGB', (240, 160), (96, 96, 96)).save(directory / '000001.png')
            self.assertEqual(compare_menu_baseline(current, events, baseline)['checkpoints'], 212)
            Image.new('RGB', (240, 160), (97, 96, 96)).save(current / '000001.png')
            with self.assertRaisesRegex(ValueError, 'native pixels differ'):
                compare_menu_baseline(current, events, baseline)
            self.assertEqual((baseline / 'trace.jsonl').read_text(), trace)


    def test_retained_family_baseline_includes_three_original_fusion_cases(self):
        events = [dict(event=phase, value=index, frame=1) for phase in ('menu_case_ready', 'menu_case_held')
                  for index in range(109)]
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            (directory / 'trace.jsonl').write_text('\n'.join(json.dumps(row) for row in events))
            Image.new('RGB', (240, 160)).save(directory / '000001.png')
            self.assertEqual(compare_menu_baseline(directory, events, directory)['checkpoints'], 218)
            with self.assertRaisesRegex(ValueError, 'missing new checkpoints'):
                compare_menu_baseline(directory, events[:106], directory)

    def test_retained_130_case_fusion_baseline(self):
        events = [dict(event=phase, value=index, frame=1)
                  for phase in ('menu_case_ready', 'menu_case_held') for index in range(130)]
        with tempfile.TemporaryDirectory() as temporary:
            baseline, current = Path(temporary) / 'old', Path(temporary) / 'new'
            baseline.mkdir()
            current.mkdir()
            (baseline / 'trace.jsonl').write_text('\n'.join(json.dumps(row) for row in events))
            for directory in (baseline, current):
                Image.new('RGB', (240, 160)).save(directory / '000001.png')
            self.assertEqual(compare_menu_baseline(current, events, baseline)['checkpoints'], 260)
            Image.new('RGB', (240, 160), 'white').save(current / '000001.png')
            with self.assertRaisesRegex(ValueError, 'native pixels differ'):
                compare_menu_baseline(current, events, baseline)

    def test_all_seven_fusion_layouts_at_each_health(self):
        cases = menu_cases()
        self.assertEqual(len(cases), 130)
        self.assertEqual([(case['decoration'], case['fusion_layout']) for case in cases[109:]],
                         [(health, layout) for health in range(3) for layout in range(7)])
        validate_trace(menu_trace(), 'menus')

    def test_fusion_layout_requires_native_install_and_checkpoint_agreement(self):
        rows = menu_trace()
        installs = [row for row in rows if row.get('event') == 'fusion_screen']
        installs[-1]['value'] = 5
        with self.assertRaisesRegex(ValueError, 'layout installation sequence'):
            validate_trace(rows, 'menus')
        for event in ('menu_case_ready', 'menu_case_held', None):
            rows = menu_trace()
            sample = next(row for row in rows if row.get('event') == event and row.get('case') == 129)
            sample['fusion_layout'] = 5
            with self.assertRaisesRegex(ValueError, 'wrong installed fusion layout'):
                validate_trace(rows, 'menus')

    def test_family_and_heading_failures(self):
        for field, value, message in (('family', 'plain', 'semantic family'),
                                      ('heading_y', 14, 'heading style/pose'),
                                      ('heading_style', 0, 'heading style/pose')):
            rows = menu_trace()
            held = next(row for row in rows if row.get('event') == 'menu_case_held')
            held[field] = value
            with self.assertRaisesRegex(ValueError, message):
                validate_trace(rows, 'menus')

    def test_pending_family_cannot_publish_before_installation(self):
        rows = menu_trace()
        index = next(i for i, row in enumerate(rows) if row.get('event') == 'pause_screen')
        rows.insert(index, dict(event='menu_case_request', value=0, family='titled', schema=2))
        with self.assertRaisesRegex(ValueError, 'installation boundary'):
            validate_trace(rows, 'menus')

    def test_new_opening_clears_previous_family(self):
        rows = menu_trace()
        installs = [i for i, row in enumerate(rows) if row.get('event') == 'pause_screen']
        rows.insert(installs[1], dict(event='menu_case_request', value=1, family='unknown', schema=2))
        validate_trace(rows, 'menus')

    def test_same_opening_request_retains_outgoing_family(self):
        rows = menu_trace()
        index = next(i for i, row in enumerate(rows) if row.get('event') == 'pause_screen' and row['value'] == 11)
        rows.insert(index, dict(event='menu_case_request', value=8, family='plain', schema=2))
        with self.assertRaisesRegex(ValueError, 'installation boundary'):
            validate_trace(rows, 'menus')
        rows[index]['family'] = 'titled'
        validate_trace(rows, 'menus')

    def test_animation_requires_intermediate_pose_and_checks_formula(self):
        rows = menu_trace()
        rows = [row for row in rows if row.get('heading_transition') != 8]
        with self.assertRaisesRegex(ValueError, 'animated heading poses'):
            validate_trace(rows, 'menus')
        rows = menu_trace()
        next(row for row in rows if row.get('heading_transition') == 8)['heading_y'] = 16
        with self.assertRaisesRegex(ValueError, 'heading style/pose'):
            validate_trace(rows, 'menus')

    def test_panel_mask_excludes_heading_and_rejects_wrong_panel(self):
        references = {(family, health): bytes([96] * (240 * 96 * 3))
                      for family in ('pieces', 'techniques') for health in range(3)}
        events = [dict(event=phase, frame=1, family=family, decoration=health)
                  for family, health in references for phase in ('menu_case_ready', 'menu_case_held')]
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            image = Image.new('RGB', (240, 160), (96, 96, 96))
            image.putpixel((0, 0), (255, 255, 255))
            image.save(directory / '000001.png')
            self.assertEqual(validate_panel_pixels(directory, events, references)['checkpoints'], 12)
            image.putpixel((0, 64), (0, 0, 0))
            image.save(directory / '000001.png')
            with self.assertRaisesRegex(ValueError, 'static panel'):
                validate_panel_pixels(directory, events, references)

    def test_inventory_pixels_follow_installed_state_after_health_changes(self):
        references = [bytes([value] * 720) for value in (32, 96, 160)]
        # Damaged decoration stays installed even when live health is critical.
        event = dict(event='menu_case_held', value=2, frame=1, pause_screen=1, decoration=1, health=8)
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            Image.new('RGB', (240, 160), (96, 96, 96)).save(directory / '000001.png')
            result = validate_inventory_pixels(directory, [event], references)
            self.assertEqual(result['checkpoints'], 1)
            Image.new('RGB', (240, 160), (160, 160, 160)).save(directory / '000001.png')
            with self.assertRaisesRegex(ValueError, 'disagree with installed'):
                validate_inventory_pixels(directory, [event], references)

    def test_menu_matrix_requires_every_neutral_case_and_held_decoration(self):
        rows = menu_trace()
        validate_trace(rows, 'menus')
        held = next(row for row in rows if row.get('event') == 'menu_case_held')
        held['decoration'] = 2
        with self.assertRaisesRegex(ValueError, 'wrong installed decoration'):
            validate_trace(rows, 'menus')
        rows = [row for row in menu_trace() if not (row.get('event') == 'menu_case_ready' and row['value'] == 105)]
        with self.assertRaisesRegex(ValueError, 'incomplete or unordered'):
            validate_trace(rows, 'menus')

    def test_menu_matrix_rejects_reinstall_and_wrong_detail_recipe(self):
        rows = menu_trace()
        rows.insert(1, dict(rows[0]))
        with self.assertRaisesRegex(ValueError, 'unexpected reinstallation'):
            validate_trace(rows, 'menus')
        rows = menu_trace()
        capture = next(row for row in rows if 'event' not in row and row.get('map_group') == 0x74)
        capture['map_group'] = 0x73
        with self.assertRaisesRegex(ValueError, 'wrong native detail-map recipe'):
            validate_trace(rows, 'menus')

    def test_menu_matrix_rejects_missing_or_faded_checkpoint(self):
        rows = menu_trace()
        capture = next(row for row in rows if 'event' not in row and row.get('case', -1) >= 0)
        capture['capture'] = False
        with self.assertRaisesRegex(ValueError, 'missing neutral checkpoint'):
            validate_trace(rows, 'menus')
        capture['capture'] = True
        capture['fade_active'] = 1
        with self.assertRaisesRegex(ValueError, 'missing neutral checkpoint'):
            validate_trace(rows, 'menus')

    def test_text_commands_do_not_prove_retained_artwork(self):
        rows = [dict(event='credits_install', value=i, installed=i) for i in range(9)]
        rows += [dict(event='credits_clear', value=0, installed=-1),
                 dict(event='credits_save', value=0, installed=-1), frame(1)]
        with self.assertRaisesRegex(ValueError, 'retaining'):
            validate_trace(rows, 'credits')
        rows.append(dict(event='credits_command', value=2, installed=-1))
        with self.assertRaisesRegex(ValueError, 'retaining'):
            validate_trace(rows, 'credits')
        rows.append(dict(event='credits_command', value=2, installed=8))
        validate_trace(rows, 'credits')
        rows = [row for row in rows if row.get('event') != 'credits_clear']
        with self.assertRaisesRegex(ValueError, 'explicit credits clear'):
            validate_trace(rows, 'credits')

    def test_incomplete_opening_cannot_validate_six_page_retelling(self):
        rows = [dict(event='prologue_install', value=i, installed=i) for i in range(5)]
        rows += [dict(event='prologue_clear', value=0, installed=-1), frame(1)]
        with self.assertRaisesRegex(ValueError, 'installations'):
            validate_trace(rows, 'prologue')

    def test_file_operations_have_distinct_review_frames(self):
        rows = [dict(event='file_state', value=i, installed=-1) for i in (0, 1, 3, 4, 5, 6)]
        rows += [dict(event='files_complete', value=0, installed=-1), frame(1, 3), frame(2, 5)]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            (path / 'trace.jsonl').write_text('\n'.join(json.dumps(row) for row in rows))
            for number in (1, 2):
                Image.new('RGB', (240, 160)).save(path / f'{number:06d}.png')
            result = summarize(path, 'files')
            self.assertEqual(result['review_frames'], [1, 2])
            Image.new('RGB', (256, 192)).save(path / '000002.png')
            with self.assertRaisesRegex(ValueError, 'expected native'):
                summarize(path, 'files')

    def test_reused_map_initializer_cannot_classify_hint_as_menu(self):
        rows = [dict(event='local_hint_ready', value=0, installed=-1),
                dict(frame(1, 10), schema=2, family='titled')]
        with self.assertRaisesRegex(ValueError, 'non-menu route'):
            validate_trace(rows, 'local_hint')

    def test_local_hint_border_is_not_its_foreground_label(self):
        rows = [dict(event='local_hint_ready', value=0, installed=-1), frame(1, 10)]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            (path / 'trace.jsonl').write_text('\n'.join(json.dumps(row) for row in rows))
            image = Image.new('RGB', (240, 160), (248, 208, 136))
            image.putpixel((120, 159), (248, 248, 248))
            image.save(path / '000001.png')
            summarize(path, 'local_hint')
            image.putpixel((0, 159), (0, 0, 0))
            image.save(path / '000001.png')
            with self.assertRaisesRegex(ValueError, 'solid field'):
                summarize(path, 'local_hint')


class NativeLanguageTests(unittest.TestCase):
    def test_complete_spanish_menu_trace_keeps_all_other_contracts(self):
        rows = []
        opening = fusion = 0
        for original in menu_trace():
            row = dict(original, language_slot=5)
            if row.get('event') == 'menu_decoration':
                opening += 1
                if opening in (16, 23, 30):
                    continue
            if row.get('event') == 'fusion_screen':
                fusion += 1
                if fusion in (6, 13, 20):
                    continue
            if row.get('case') in (111, 118, 125):
                continue
            rows.append(row)
        discovery = dict(schema=2, family='unknown', language_slot=5,
                         nav_task=2, nav_substate=2, nav_ui=0, nav_fade=0)
        rows[:0] = [dict(discovery, event='fusion_layout_mask', value=0x7b)] + [
            dict(discovery, event='menu_case_unavailable', value=i) for i in (111, 118, 125)]
        validate_trace(rows, 'menus')
        skipped = next(r for r in rows if r.get('event') == 'menu_case_unavailable')
        skipped['nav_substate'] = 7
        with self.assertRaisesRegex(ValueError, 'did not restore native gameplay'):
            validate_trace(rows, 'menus')
        skipped['nav_substate'] = 2
        # A gap in a reachable layout still fails, even with valid discovery.
        rows = [r for r in rows if r.get('case') != 112]
        with self.assertRaises(ValueError):
            validate_trace(rows, 'menus')

    def test_spanish_unavailable_width_has_explicit_canonical_gaps(self):
        rows = [dict(event='fusion_layout_mask', value=0x7b, language_slot=5)]
        rows += [dict(event='menu_case_unavailable', value=i, language_slot=5) for i in (111, 118, 125)]
        ids = menu_case_ids(rows)
        self.assertEqual(len(ids), 127)
        self.assertEqual(set(range(130)) - set(ids), {111, 118, 125})

    def test_layout_gaps_cannot_be_inferred_or_applied_to_another_language(self):
        for rows in ([dict(language_slot=5)],
                     [dict(event='fusion_layout_mask', value=0x7b, language_slot=3)],
                     [dict(event='menu_case_unavailable', value=111)],
                     [dict(event='fusion_layout_mask', value=0x7f, language_slot=5)]):
            with self.assertRaises(ValueError):
                menu_case_ids(rows)

    def test_eu_resource_slots(self):
        for language in range(1, 6):
            rows = [dict(task=0, language_slot=0), dict(task=1, language_slot=language + 1),
                    dict(writes_task=2, language_slot=language + 1)]
            self.assertEqual(validate_language(rows, 'BZMP', language),
                             dict(language_slot=language + 1, observations=2))

    def test_eu_japanese_preference_falls_back_to_english(self):
        self.assertEqual(validate_language([dict(task=1, language_slot=2)], 'BZMP', 0)['language_slot'], 2)

    def test_usa_and_japan_slots(self):
        for code, language, slot in (('BZME', 1, 1), ('BZMJ', 1, 0)):
            self.assertEqual(validate_language([dict(task=1, language_slot=slot)], code, language)['language_slot'], slot)

    def test_requested_language_is_not_native_evidence(self):
        for row in (dict(task=1), dict(task=1, language_slot=2), dict(task=2, language_slot=4)):
            with self.assertRaisesRegex(ValueError, 'native language slot'):
                validate_language([row], 'BZMP', 2)

    def test_language_must_survive_return_and_events(self):
        for row in (dict(writes_task=2, language_slot=2), dict(writes_task=4, language_slot=2),
                    dict(task=4, language_slot=2)):
            with self.assertRaisesRegex(ValueError, 'native language slot'):
                validate_language([dict(task=1, language_slot=3), row], 'BZMP', 2)

    def test_startup_alone_cannot_certify_language(self):
        with self.assertRaisesRegex(ValueError, 'no initialized'):
            validate_language([dict(task=0, language_slot=3)], 'BZMP', 2)


if __name__ == '__main__':
    unittest.main()
