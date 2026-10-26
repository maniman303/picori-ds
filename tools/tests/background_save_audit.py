#!/usr/bin/env python3
"""Failure-oriented checks for the native pause save audit."""
import copy
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from PIL import Image
from capture_background_audit import validate_trace
from background_save_audit import save_cases, validate_save_pixels


def trace():
    rows = []
    clock = 0
    sample = dict(schema=2, task=2, state=2, substate=2, ui=0, family='unknown',
                  fade_active=0, max_health=24, world_area=3, world_room=8,
                  save_menu=1, save_pause_phase=2, save_new_keys=0,
                  heading_style=0, heading_x=0, heading_y=0, heading_transition=0,
                  save_timer=0)

    def event(name, value, capture=False):
        nonlocal clock
        clock += 1
        sample['frame'] = clock
        rows.append(dict(sample, event=name, value=value))
        if capture:
            rows.append(dict(sample, capture=True))

    def frame(**changes):
        nonlocal clock
        clock += 1
        sample.update(frame=clock, **changes)
        rows.append(dict(sample, capture=False))

    for index, case in enumerate(save_cases()):
        sample.update(save_case=index, decoration=case['decoration'], health=(24, 15, 8)[case['decoration']],
                      substate=2, ui=0, family='unknown', save_cursor=0, save_new_keys=0, save_menu=1)
        event('save_case_request', index)
        sample.update(substate=7, ui=1)
        event('menu_decoration', case['decoration'])
        sample.update(family='titled', pause_screen=1, heading_style=1, heading_x=64, heading_y=16)
        event('pause_screen', 1)
        sample['pause_screen'] = 2
        event('pause_screen', 2)
        if index:
            for cursor, key in ((4, 32), (8, 32), (3, 64), (1, 64), (0, 32)):
                sample.update(save_cursor=cursor, save_new_keys=key)
                event('quest_choice', cursor)
        sample['save_new_keys'] = 0
        event('save_quest_ready', index, True)
        for cursor, key in ((11, 32), (5, 64)):
            sample.update(save_cursor=cursor, save_new_keys=key)
            event('quest_choice', cursor)
        sample['save_new_keys'] = 0
        event('save_prompt_request', index)
        sample.update(family='plain', pause_screen=10, heading_style=0, heading_x=0, heading_y=0, save_cursor=0)
        event('pause_screen', 10)
        event('save_prompt', 0)
        event('save_ready', index, True)
        sample['health'] = 24 if case['decoration'] == 2 else 8
        event('save_health_mutated', sample['health'])
        event('save_held', index, True)
        choices = ([], [(1, 128), (0, 64)], [(1, 128)], [(1, 2)], [(1, 128)], [])[case['mode']]
        for cursor, key in choices:
            sample.update(save_cursor=cursor, save_new_keys=key)
            event('save_choice', cursor)
        sample['save_new_keys'] = 2 if case['mode'] in (3, 4) else 1
        event('save_answer', 0 if case['accept'] else 1)
        sample['save_new_keys'] = 0
        if case['accept']:
            sample['save_menu'] = 2
            if case['error']:
                event('save_write_fault', 1)
                sample['save_timer'] = 255
                event('save_error', 0)
                sample['save_menu'] = 3
                for timer in range(60, -1, -1):
                    frame(save_timer=timer)
            else:
                event('save_success', 1)
                sample['save_menu'] = 3
            if case['error']:
                event('save_result_ready', index, True)
                event('save_readback', 0)
                event('save_dismiss_request', index)
                frame(save_new_keys=1)
        else:
            event('save_decline_unchanged', index)
        sample.update(save_new_keys=0, save_menu=1, pause_screen=2, family='titled',
                      save_cursor=5, heading_style=1, heading_x=64, heading_y=16)
        event('pause_screen', 2)
        if case['accept'] and not case['error']:
            event('save_result_ready', index, True)
            event('save_readback', sample['health'])
        event('save_quest_returned', index, True)
        event('save_exit_request', index)
        frame(save_new_keys=8)
        sample.update(save_new_keys=0, substate=2, ui=0, family='unknown')
        event('save_game_restored', index, True)
    event('save_complete', 18)
    return rows


class SaveAuditTests(unittest.TestCase):
    def setUp(self):
        self.rows = trace()

    def reject_event(self, name, **changes):
        rows = copy.deepcopy(self.rows)
        next(r for r in rows if r.get('event') == name).update(changes)
        with self.assertRaises((ValueError, StopIteration)):
            validate_trace(rows, 'save')

    def test_complete_matrix(self):
        events, frames = validate_trace(self.rows, 'save')
        self.assertEqual(sum(r['event'] == 'save_success' for r in events), 6)
        self.assertEqual(sum(r['event'] == 'save_error' for r in events), 3)
        self.assertEqual(sum(r['event'] == 'save_decline_unchanged' for r in events), 9)
        self.assertEqual(len(frames), 99)

    def test_missing_native_events_and_terminal(self):
        for name in ('save_complete', 'save_prompt', 'save_success', 'save_write_fault',
                     'save_error', 'save_readback', 'save_decline_unchanged', 'save_game_restored'):
            rows = copy.deepcopy(self.rows)
            rows.pop(next(i for i, r in enumerate(rows) if r.get('event') == name))
            with self.assertRaises((ValueError, StopIteration)):
                validate_trace(rows, 'save')

    def test_case_values_must_match_receipts(self):
        self.reject_event('save_held', value=17)
        self.reject_event('save_prompt', value=1)
        self.reject_event('save_result_ready', value=1)

    def test_navigation_and_answers(self):
        self.reject_event('quest_choice', value=4)
        self.reject_event('quest_choice', save_new_keys=16)
        self.reject_event('save_choice', save_new_keys=16)
        self.reject_event('save_answer', value=1)
        self.reject_event('save_answer', save_new_keys=2)

    def test_native_result_order(self):
        rows = copy.deepcopy(self.rows)
        indices = [i for i, r in enumerate(rows) if r.get('event') in ('save_write_fault', 'save_error')][:2]
        rows[indices[0]], rows[indices[1]] = rows[indices[1]], rows[indices[0]]
        with self.assertRaises(ValueError):
            validate_trace(rows, 'save')

    def test_refusal_cannot_save(self):
        rows = copy.deepcopy(self.rows)
        index = next(i for i, r in enumerate(rows) if r.get('event') == 'save_decline_unchanged')
        rows.insert(index, dict(rows[index], event='save_success', value=1))
        with self.assertRaises(ValueError):
            validate_trace(rows, 'save')

    def test_source_and_live_health(self):
        for key, value in (('health', 24), ('decoration', 2), ('family', 'titled'),
                           ('heading_style', 1), ('save_cursor', 1), ('save_case', 1)):
            self.reject_event('save_held', **{key: value})
        self.reject_event('save_case_request', family='plain')
        self.reject_event('save_result_ready', family='unknown')

    def test_native_checksum_readback_and_timer(self):
        self.reject_event('save_readback', value=24)
        rows = copy.deepcopy(self.rows)
        next(r for r in rows if r.get('event') == 'save_result_ready' and r['save_case'] == 5)['save_timer'] = 1
        with self.assertRaises(ValueError):
            validate_trace(rows, 'save')
        rows = copy.deepcopy(self.rows)
        next(r for r in rows if 'event' not in r and not r['capture'] and r.get('save_timer') == 59)['save_timer'] = 0
        with self.assertRaisesRegex(ValueError, 'wait skipped'):
            validate_trace(rows, 'save')

    def test_missing_capture_or_non_neutral_state(self):
        rows = copy.deepcopy(self.rows)
        capture = next(r for r in rows if 'event' not in r and r['capture'])
        capture['fade_active'] = 1
        with self.assertRaises(ValueError):
            validate_trace(rows, 'save')

    def test_restore_original_room(self):
        self.reject_event('save_game_restored', world_room=9)
        self.reject_event('save_game_restored', family='plain')

    def test_dismissal_and_exit_inputs(self):
        for key in (1, 8):
            rows = copy.deepcopy(self.rows)
            next(r for r in rows if 'event' not in r and r.get('save_new_keys') == key)['save_new_keys'] = 0
            with self.assertRaises(ValueError):
                validate_trace(rows, 'save')

    def test_rom_pixels_and_exposed_identity(self):
        events = [r for r in self.rows if 'event' in r]
        refs = [bytes([8 + health * 8]) * 720 for health in range(3)]
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            phases = ('save_quest_ready', 'save_ready', 'save_held', 'save_result_ready', 'save_quest_returned')
            for event in events:
                if event['event'] in phases:
                    Image.new('RGB', (240, 160), (8 + event['decoration'] * 8,) * 3).save(directory / f"{event['frame']:06d}.png")
            self.assertEqual(validate_save_pixels(directory, events, refs)['checkpoints'], 81)
            with self.assertRaises(ValueError):
                validate_save_pixels(directory, events, [refs[0]] * 3)
            event = next(r for r in events if r['event'] == 'save_result_ready')
            Image.new('RGB', (240, 160)).save(directory / f"{event['frame']:06d}.png")
            with self.assertRaisesRegex(ValueError, 'ROM health field'):
                validate_save_pixels(directory, events, refs)


if __name__ == '__main__':
    unittest.main()
