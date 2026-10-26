#!/usr/bin/env python3
"""Failure-oriented checks for native figurine reveal and description input."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from capture_background_audit import validate_trace
from background_figurine_actions_audit import figurine_action_cases, validate_figurine_actions, validate_figurine_action_pixels
from verify_figurine_actions import completed_prefix, combine_coverage, verify_capture


def trace():
    rows = []
    for index, case in enumerate(figurine_action_cases()):
        limit = 32 if case['index'] in (130, 136) else 0
        sample = dict(task=2, state=2, substate=7, ui=7, menu=1, fade_active=0,
                      figurine_action_case=index, figurine_field=0, figurine_index=case['index'],
                      figurine_owned=case['initial_owned'], staffroll=case['staffroll'], decoration=case['decoration'],
                      health=(24, 15, 8)[case['decoration']], max_health=24, pause_screen=3, family='unknown', schema=2,
                      heading_style=0, heading_x=0, heading_y=0, heading_transition=0,
                      figurine_variant=255 if case['collection'] else case['index'], figurine_menu=2,
                      figurine_counter=0, figurine_duplicate=0, figurine_brightness=0, figurine_blend=0,
                      detail_scroll=0, detail_limit=limit, figurine_bg1_y=-112, figurine_input=0,
                      palette_zero=0x318e, palette_251=0x318e, world_area=3, world_room=8, player_action=1, player_control=0)

        def event(name, value, capture=False):
            sample['frame'] = len(rows)
            rows.append(dict(sample, event=name, value=value))
            if capture:
                rows.append(dict(sample, capture=True))

        event('figurine_action_request', index)
        event('menu_decoration', case['decoration'])
        event('pause_screen', 3)
        sample['figurine_field'] = 1
        event('figurine_screen', 1 if case['collection'] else case['index'])
        if case['collection']:
            choices = list(range(6, case['index'] + 1, 5))
            choices += list(range(choices[-1] + 1, case['index'] + 1))
            for choice in choices:
                event('figurine_choice', choice)
            sample['figurine_menu'] = 1
        else:
            event('figurine_capsule', case['capsule'])
            event('figurine_capsule_open', case['capsule'])
            for counter in range(1, 129):
                sample['figurine_counter'] = counter
                if counter == 64:
                    sample.update(figurine_owned=1, figurine_duplicate=case['duplicate'])
                    event('figurine_award', case['duplicate'])
                brightness = min(counter // 4, 32 - counter // 4)
                sample.update(figurine_brightness=brightness, figurine_blend=0xbf)
                if counter == 128:
                    sample.update(figurine_brightness=0, figurine_blend=0, figurine_menu=3)
                    event('figurine_reveal_done', case['duplicate'])
                sample['frame'] = len(rows)
                rows.append(dict(sample, capture=False))
        event('figurine_action_ready', index, True)
        sample['health'] = 24 if case['decoration'] == 2 else 8
        event('figurine_action_health_mutated', sample['health'])
        event('figurine_action_held', index, True)
        for scroll in list(range(8, limit + 1, 8)) + [limit]:
            sample.update(detail_scroll=scroll, figurine_bg1_y=scroll - 112, figurine_input=16)
            event('figurine_scroll', 16)
        event('figurine_action_bottom', index, True)
        for scroll in list(range(limit - 8, -1, -8)) + [0]:
            sample.update(detail_scroll=scroll, figurine_bg1_y=scroll - 112, figurine_input=32)
            event('figurine_scroll', 32)
        event('figurine_action_top', index, True)
        sample['figurine_input'] = 8 if index % 2 else 2
        if case['collection']:
            sample.update(figurine_menu=3, frame=len(rows))
            rows.append(dict(sample, capture=False))
            sample['figurine_input'] = 0
            rows.append(dict(sample, event='figurine_cancel', value=case['index']))
        else:
            event('figurine_cancel', case['index'])
        sample.update(figurine_field=0, substate=2, ui=0)
        event('figurine_action_restored', index, True)
    rows.append(dict(event='figurine_actions_complete', value=54, figurine_field=0, family='unknown'))
    return rows


class FigurineActions(unittest.TestCase):
    def test_capsule_health_credits_ownership_and_collection_matrix(self):
        cases = figurine_action_cases()
        self.assertEqual(len(cases), 54)
        self.assertEqual(sum(not c['collection'] for c in cases), 48)
        self.assertEqual({(c['decoration'], c['staffroll'], c['capsule'], c['initial_owned']) for c in cases if not c['collection']},
                         {(h, post, capsule, owned) for h in range(3) for post in range(2) for capsule in range(4) for owned in range(2)})
        self.assertEqual({c['index'] for c in cases if c['collection']}, {130, 136})
        self.assertEqual({c['duplicate'] for c in cases}, {0, 1, 2, 4, 16, 128})
        validate_trace(trace(), 'figurine_actions')

    def test_native_capsule_award_scroll_and_restore_events_required(self):
        for name in ('figurine_screen', 'figurine_capsule', 'figurine_capsule_open', 'figurine_award',
                     'figurine_reveal_done', 'figurine_scroll', 'figurine_cancel', 'figurine_action_restored'):
            rows = trace()
            rows.pop(next(i for i, r in enumerate(rows) if r.get('event') == name))
            with self.subTest(name=name), self.assertRaises(ValueError):
                validate_trace(rows, 'figurine_actions')

    def test_checkpoint_and_capture_context(self):
        for name in ('figurine_action_ready', 'figurine_action_held', 'figurine_action_bottom', 'figurine_action_top', None):
            for field, value in (('figurine_index', 2), ('figurine_owned', 0), ('figurine_variant', 255),
                                 ('figurine_menu', 1), ('staffroll', 1), ('decoration', 2), ('family', 'plain'),
                                 ('health', 1), ('palette_zero', 0), ('heading_style', 1), ('figurine_field', 0)):
                rows = trace()
                sample = next(r for r in rows if r.get('event') == name and (name is not None or r.get('capture')))
                sample[field] = value
                with self.subTest(name=name, field=field), self.assertRaises(ValueError):
                    validate_trace(rows, 'figurine_actions')

    def test_award_is_at_64_completion_at_128_and_duplicate_is_raw_mask(self):
        for name, field, value in (('figurine_award', 'figurine_counter', 63),
                                    ('figurine_reveal_done', 'figurine_counter', 127),
                                    ('figurine_capsule_open', 'figurine_counter', 1),
                                    ('figurine_capsule', 'value', 3)):
            rows = trace()
            next(r for r in rows if r.get('event') == name)[field] = value
            with self.assertRaises(ValueError):
                validate_trace(rows, 'figurine_actions')
        rows = trace()
        # Figure 47's previous ownership returns bit 7 (128), not boolean 1.
        next(r for r in rows if r.get('event') == 'figurine_award' and r['figurine_action_case'] == 1)['value'] = 1
        with self.assertRaisesRegex(ValueError, 'award/duplicate timing'):
            validate_trace(rows, 'figurine_actions')

    def test_ownership_cannot_publish_early_or_revert_after_award(self):
        for counter, owned in ((32, 1), (96, 0)):
            rows = trace()
            next(r for r in rows if 'event' not in r and r.get('figurine_action_case') == 0 and r.get('figurine_counter') == counter)['figurine_owned'] = owned
            with self.assertRaisesRegex(ValueError, 'ownership changed outside'):
                validate_trace(rows, 'figurine_actions')

    def test_entire_native_brightness_curve_required_and_neutral_effect_cleared(self):
        rows = trace()
        next(r for r in rows if 'event' not in r and r.get('figurine_counter') == 65)['figurine_brightness'] = 15
        with self.assertRaisesRegex(ValueError, 'brightness controls'):
            validate_trace(rows, 'figurine_actions')
        rows = trace()
        rows = [r for r in rows if not ('event' not in r and r.get('figurine_action_case') == 0 and r.get('figurine_counter') == 65)]
        with self.assertRaisesRegex(ValueError, 'incomplete native prize brightness'):
            validate_trace(rows, 'figurine_actions')
        rows = trace()
        next(r for r in rows if r.get('event') == 'figurine_action_ready')['figurine_blend'] = 0xbf
        with self.assertRaisesRegex(ValueError, 'left brightness'):
            validate_trace(rows, 'figurine_actions')

    def test_scroll_geometry_and_both_boundary_clamps(self):
        for field, value in (('detail_scroll', 8), ('detail_limit', 8), ('figurine_bg1_y', -80)):
            rows = trace()
            next(r for r in rows if r.get('event') == 'figurine_action_top')[field] = value
            with self.assertRaises(ValueError):
                validate_trace(rows, 'figurine_actions')
        for direction in (16, 32):
            rows = trace()
            rows.pop(next(i for i, r in enumerate(rows) if r.get('event') == 'figurine_scroll' and r['value'] == direction))
            with self.assertRaisesRegex(ValueError, 'edges or boundary clamps'):
                validate_trace(rows, 'figurine_actions')
        rows = trace()
        next(r for r in rows if r.get('event') == 'figurine_scroll')['figurine_input'] = 0
        with self.assertRaisesRegex(ValueError, 'input/BG1 geometry'):
            validate_trace(rows, 'figurine_actions')

    def test_native_collection_navigation_and_exit_inputs(self):
        for name, field, value in (('figurine_choice', 'value', 1), ('figurine_cancel', 'figurine_input', 1)):
            rows = trace()
            next(r for r in rows if r.get('event') == name)[field] = value
            with self.assertRaisesRegex(ValueError, 'navigation|B/START exit'):
                validate_trace(rows, 'figurine_actions')

    def test_missing_or_faded_checkpoint_and_result_case_mismatch(self):
        for field, value in (('capture', False), ('fade_active', 1), ('ui', 9)):
            rows = trace()
            next(r for r in rows if r.get('capture'))[field] = value
            with self.assertRaisesRegex(ValueError, 'neutral figurine action'):
                validate_trace(rows, 'figurine_actions')
        for field, value in (('figurine_action_case', 1), ('world_room', 0), ('figurine_owned', 0)):
            rows = trace()
            result = next(r for r in rows if r.get('event') == 'figurine_action_restored')
            next(r for r in rows if 'event' not in r and r.get('frame') == result['frame'])[field] = value
            with self.assertRaisesRegex(ValueError, 'restoration'):
                validate_trace(rows, 'figurine_actions')

    def test_no_early_source_or_unexpected_reinstallation(self):
        for name, value in (('pause_screen', 1), ('figurine_cancel', 0)):
            rows = trace()
            next(r for r in rows if r.get('event') == name)['figurine_field'] = value
            with self.assertRaisesRegex(ValueError, 'outside installation/restoration'):
                validate_trace(rows, 'figurine_actions')
        rows = trace()
        rows.insert(2, copy.copy(rows[1]))
        with self.assertRaisesRegex(ValueError, 'reinstallation'):
            validate_trace(rows, 'figurine_actions')

    def test_explicit_suffix_keeps_canonical_case_ids_and_requires_terminal(self):
        rows = [dict(event='figurine_actions_start', value=50, figurine_field=0, family='unknown')]
        rows += [r for r in trace() if r.get('figurine_action_case', -1) >= 50 or r.get('event') == 'figurine_actions_complete']
        validate_trace(rows, 'figurine_actions')
        rows[0]['value'] = 49
        with self.assertRaises(ValueError):
            validate_trace(rows, 'figurine_actions')
        rows[0]['value'] = 54
        with self.assertRaises(ValueError):
            validate_trace(rows, 'figurine_actions')

    def test_partial_prefix_is_not_a_complete_run(self):
        rows = trace()
        end = next(i for i, r in enumerate(rows) if r.get('capture') and r.get('figurine_action_case') == 8
                   and r.get('substate') == 2)
        rows = rows[:end + 1]
        prefix, start, stop, terminal = completed_prefix(rows)
        self.assertEqual((start, stop, terminal), (0, 9, False))
        events = [r for r in prefix if 'event' in r]
        frames = [r for r in prefix if r.get('capture')]
        validate_figurine_actions(prefix, events, frames, start=start, end=stop, require_complete=False)
        with self.assertRaises(ValueError):
            validate_trace(prefix, 'figurine_actions')

    def test_prefix_requires_matching_capture_receipt_and_ordered_results(self):
        rows = trace()
        end = next(i for i, r in enumerate(rows) if r.get('capture') and r.get('figurine_action_case') == 8
                   and r.get('substate') == 2)
        rows = rows[:end + 1]
        rows[-1]['capture'] = False
        _, _, stop, terminal = completed_prefix(rows)
        self.assertEqual((stop, terminal), (8, False))
        rows.append(dict(next(r for r in rows if r.get('event') == 'figurine_action_restored')))
        with self.assertRaisesRegex(ValueError, 'missing or duplicate'):
            completed_prefix(rows)

    def test_combined_reports_require_complete_disjoint_language_rom_coverage(self):
        def report(start, stop):
            cases = figurine_action_cases()[start:stop]
            return dict(preferred_language=1, rom_sha256='fixture', start_case=start, end_case=stop,
                        validated_cases=list(range(start, stop)), directory='fixture', scope='test',
                        trace_sha256='fixture', native_terminal_observed=stop == 54,
                        prize_cases=sum(not c['collection'] for c in cases), collection_cases=sum(c['collection'] for c in cases),
                        native_field_pixels=dict(checkpoints=4 * len(cases), description_comparisons=3 * len(cases)))
        a, b = report(0, 50), report(50, 54)
        result = combine_coverage([a, b])
        self.assertEqual((result['prize_cases'], result['collection_cases'], result['field_checkpoints']), (48, 6, 216))
        for reports in ([a], [report(0, 51), b], [a, dict(b, preferred_language=2)], [a, dict(b, rom_sha256='other')],
                        [a, dict(b, validated_cases=[50, 52, 53])]):
            with self.assertRaises(ValueError):
                combine_coverage(reports)

    def test_explicit_recovery_preserves_raw_eof_and_rejects_middle_corruption(self):
        rows = [dict(event='figurine_actions_start', value=53, figurine_field=0, family='unknown')]
        rows += [r for r in trace() if r.get('figurine_action_case', -1) == 53 or r.get('event') == 'figurine_actions_complete']
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            rom = directory / 'fixture.gba'
            rom.write_bytes(b'unit test palette source')
            color = tuple(((0x318e >> shift) & 31) << 3 for shift in (0, 5, 10))
            for row in rows:
                if not row.get('capture'):
                    continue
                image = Image.new('RGB', (240, 160), color)
                image.putpixel((100, 140), (0, 0, 0) if row['detail_scroll'] else (255, 255, 255))
                image.save(directory / f"{row['frame']:06d}.png")
            raw = ('\n'.join(json.dumps(r) for r in rows) + '\n{"frame":').encode()
            path = directory / 'trace.jsonl'
            path.write_bytes(raw)
            with patch('verify_figurine_actions.figurine_reference', return_value=0x318e):
                with self.assertRaisesRegex(ValueError, 'corrupt'):
                    verify_capture(directory, rom, 1)
                result = verify_capture(directory, rom, 1, True)
                self.assertEqual(result['validated_cases'], [53])
                self.assertTrue(result['ignored_incomplete_eof'])
                self.assertEqual(result['scope'], 'completed-prefix-of-interrupted-run')
                self.assertEqual(path.read_bytes(), raw)
                path.write_bytes(b'not JSON\n' + raw)
                with self.assertRaisesRegex(ValueError, 'corrupt'):
                    verify_capture(directory, rom, 1, True)

    def test_pixels_use_rom_source_and_exclude_scrolling_description(self):
        events = [r for r in trace() if r.get('event') in ('figurine_action_ready', 'figurine_action_held', 'figurine_action_bottom', 'figurine_action_top')]
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            ref = 0x318e
            color = tuple(((ref >> shift) & 31) << 3 for shift in (0, 5, 10))
            for row in events:
                im = Image.new('RGB', (240, 160), color)
                im.putpixel((100, 140), (0, 0, 0) if row['event'] == 'figurine_action_bottom' and row['detail_limit'] else (255, 255, 255))
                im.save(directory / f"{row['frame']:06d}.png")
            self.assertEqual(validate_figurine_action_pixels(directory, events, ref)['checkpoints'], 216)
            with self.assertRaisesRegex(ValueError, 'native ROM palette'):
                validate_figurine_action_pixels(directory, events, 0)
            top = next(r for r in events if r['event'] == 'figurine_action_top' and r['detail_limit'])
            top_path = directory / f"{top['frame']:06d}.png"
            with Image.open(top_path) as image:
                original = image.copy()
                image.putpixel((100, 140), (0, 0, 0))
                image.save(top_path)
            with self.assertRaisesRegex(ValueError, 'did not return'):
                validate_figurine_action_pixels(directory, events, ref)
            original.save(top_path)
            path = directory / f"{events[0]['frame']:06d}.png"
            with Image.open(path) as im:
                im.putpixel((100, 40), (0, 0, 0))
                im.save(path)
            with self.assertRaisesRegex(ValueError, 'live field'):
                validate_figurine_action_pixels(directory, events, ref)
            with self.assertRaisesRegex(ValueError, 'incomplete'):
                validate_figurine_action_pixels(directory, [], ref)


if __name__ == '__main__':
    unittest.main()
