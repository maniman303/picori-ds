#!/usr/bin/env python3
"""Reject incomplete or non-native fusion interaction evidence."""
import copy
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from PIL import Image
from capture_background_audit import validate_trace
from background_fusion_actions_audit import fusion_action_cases, validate_fusion_action_pixels


def trace():
    rows = []
    clock = 0
    sample = dict(schema=2, fusion_task=2, fusion_substate=2, fusion_ui=0, fusion_next=0,
                  fusion_menu=1, fusion_overlay=0, fusion_column=2, fusion_cursor=0, fusion_selected=0,
                  fusion_timer=0, fusion_keys=0, fusion_fade=0, fusion_brightness=0, fusion_result=0,
                  family='unknown', decoration=-1, health=24, max_health=24, world_area=3, world_room=8,
                  heading_style=0, heading_x=0, heading_y=0, heading_transition=0, fade_active=0)

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

    for index, case in enumerate(fusion_action_cases()):
        mode = case['mode']
        sample.update(fusion_action_case=index, fusion_substate=2, fusion_ui=0, fusion_next=0,
                      fusion_menu=1, fusion_cursor=0, fusion_keys=0, fusion_selected=0, fusion_result=0,
                      fusion_id=case['target'], fusion_count=0, fusion_owned=0,
                      fusion_piece109=2, fusion_piece110=2, health=(24, 15, 8)[case['decoration']],
                      family='unknown', heading_style=0, heading_x=0, heading_y=0)
        event('fusion_action_request', index)
        sample.update(fusion_substate=7, fusion_ui=4, fusion_next=2, decoration=case['decoration'])
        event('menu_decoration', case['decoration'])
        sample.update(family='fusion', heading_style=1, heading_x=64, heading_y=16)
        event('fusion_screen', 0)
        event('fusion_action_ready', index, True)
        sample['health'] = 24 if case['decoration'] == 2 else 8
        event('fusion_action_health_mutated', sample['health'])
        event('fusion_action_held', index, True)
        if mode == 5:
            sample.update(fusion_cursor=1, fusion_keys=128)
            event('fusion_choice', 1)
        key = (2, 8, 512, 1, 1, 1)[mode]
        event('fusion_action_input', key)
        frame(fusion_keys=key)
        if mode >= 3:
            sample['fusion_selected'] = case['piece']
            event('fusion_attempt', case['piece'])
        if mode == 3:
            event('fusion_mismatch', 109)
            sample.update(fusion_keys=0)
            event('fusion_action_mismatch_returned', index, True)
            event('fusion_action_cancel_request', 2)
            sample['fusion_keys'] = 2
        if mode < 4:
            event('fusion_cancel', 2 if mode == 3 else key)
            sample['fusion_result'] = 5
            event('fusion_cancel_exit', 5)
        else:
            event('fusion_match', case['piece'])
            for brightness in (*range(15), 0):
                frame(fusion_brightness=brightness)
            sample.update(fusion_count=1, fusion_owned=1, fusion_piece109=1 if mode == 4 else 2,
                          fusion_piece110=1 if mode == 5 else 2)
            event('fusion_award', case['target'])
            sample['fusion_result'] = 6
            event('fusion_success_exit', 8 if mode == 5 else 0)
            if mode == 5:
                sample.update(family='unknown', heading_style=0, heading_x=0, heading_y=0, fusion_ui=8)
                event('fusion_world_event_install', 20)
                sample.update(fusion_ui=10, decoration=0 if sample['health'] == 24 else 2)
                event('menu_decoration', sample['decoration'])
                event('fusion_local_hint_install', 0)
        sample.update(fusion_substate=2, fusion_ui=0, fusion_next=0, family='unknown', fusion_keys=0,
                      heading_style=0, heading_x=0, heading_y=0)
        event('fusion_action_game_restored', index, True)
    event('fusion_actions_complete', 18)
    return rows


class FusionActionTests(unittest.TestCase):
    def setUp(self):
        self.rows = trace()

    def reject(self, name, **changes):
        rows = copy.deepcopy(self.rows)
        next(r for r in rows if r.get('event') == name).update(changes)
        with self.assertRaises((ValueError, StopIteration)):
            validate_trace(rows, 'fusion_actions')

    def test_full_matrix(self):
        events, frames = validate_trace(self.rows, 'fusion_actions')
        self.assertEqual(len(frames), 57)
        self.assertEqual(sum(r['event'] == 'fusion_award' for r in events), 6)

    def test_missing_native_lifecycle(self):
        for name in ('fusion_actions_complete', 'fusion_screen', 'fusion_mismatch', 'fusion_match',
                     'fusion_award', 'fusion_success_exit', 'fusion_cancel_exit',
                     'fusion_world_event_install', 'fusion_local_hint_install', 'fusion_action_game_restored'):
            rows = copy.deepcopy(self.rows)
            rows.remove(next(r for r in rows if r.get('event') == name))
            with self.assertRaises((ValueError, StopIteration)):
                validate_trace(rows, 'fusion_actions')

    def test_installed_health_and_heading(self):
        self.reject('fusion_action_ready', health=8)
        self.reject('fusion_action_held', health=24)
        self.reject('fusion_action_held', family='plain')
        self.reject('fusion_action_held', heading_style=0)
        self.reject('fusion_action_held', decoration=2)

    def test_no_reload_on_health_change(self):
        rows = copy.deepcopy(self.rows)
        i = next(i for i, r in enumerate(rows) if r.get('event') == 'fusion_action_held')
        rows.insert(i, dict(rows[i], event='menu_decoration', value=2))
        with self.assertRaisesRegex(ValueError, 'reload/lifetime'):
            validate_trace(rows, 'fusion_actions')

    def test_real_input_and_selection(self):
        self.reject('fusion_action_input', value=1)
        self.reject('fusion_cancel', fusion_keys=1)
        self.reject('fusion_choice', fusion_keys=64)
        self.reject('fusion_attempt', fusion_selected=110)

    def test_ownership_consumed_only_after_match(self):
        self.reject('fusion_action_mismatch_returned', fusion_piece109=1)
        self.reject('fusion_award', fusion_count=0)
        self.reject('fusion_award', fusion_piece109=2)
        self.reject('fusion_award', fusion_owned=0)

    def test_brightness_timeline(self):
        rows = copy.deepcopy(self.rows)
        next(r for r in rows if 'event' not in r and r['fusion_brightness'] == 14)['fusion_brightness'] = 0
        with self.assertRaisesRegex(ValueError, 'brightness timeline'):
            validate_trace(rows, 'fusion_actions')

    def test_scene_handoff_and_source_lifetime(self):
        self.reject('fusion_success_exit', value=8)
        self.reject('fusion_world_event_install', family='fusion')
        self.reject('fusion_world_event_install', value=77)
        self.reject('fusion_local_hint_install', family='fusion')
        self.reject('fusion_action_request', family='fusion')

    def test_restore_room_and_result(self):
        self.reject('fusion_action_game_restored', world_room=9)
        self.reject('fusion_action_game_restored', fusion_result=6)
        self.reject('fusion_action_game_restored', fusion_piece109=1)
        self.reject('fusion_action_game_restored', family='fusion')

    def test_capture_and_neutral_state(self):
        self.reject('fusion_action_ready', fusion_fade=1)
        self.reject('fusion_action_mismatch_returned', fusion_menu=4)
        rows = copy.deepcopy(self.rows)
        rows.remove(next(r for r in rows if r.get('capture')))
        with self.assertRaises(ValueError):
            validate_trace(rows, 'fusion_actions')

    def test_rom_field_pixels(self):
        events = [r for r in self.rows if 'event' in r]
        references = [bytes([8 + i * 8]) * 720 for i in range(3)]
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            for r in events:
                if r['event'] in ('fusion_action_ready', 'fusion_action_held', 'fusion_action_mismatch_returned'):
                    Image.new('RGB', (240, 160), (8 + r['decoration'] * 8,) * 3).save(directory / f"{r['frame']:06d}.png")
            self.assertEqual(validate_fusion_action_pixels(directory, events, references)['checkpoints'], 39)
            with self.assertRaises(ValueError):
                validate_fusion_action_pixels(directory, events, [references[0]] * 3)
            r = next(r for r in events if r['event'] == 'fusion_action_mismatch_returned')
            Image.new('RGB', (240, 160)).save(directory / f"{r['frame']:06d}.png")
            with self.assertRaisesRegex(ValueError, 'installed ROM health field'):
                validate_fusion_action_pixels(directory, events, references)


if __name__ == '__main__':
    unittest.main()
