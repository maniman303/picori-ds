#!/usr/bin/env python3
"""Failure-oriented checks for native Game Over evidence."""
import copy
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from PIL import Image
from capture_background_audit import validate_trace
from background_gameover_audit import validate_gameover_pixels


def trace(extended=False):
    count = 6 if extended else 5
    quit_case = count - 1
    errors = (3, 4) if extended else (3,)
    saved = (0, 1, 4, 5) if extended else (0, 1, 3)
    rows = []
    clock = 0
    sample = dict(schema=2, family='unknown', heading_style=0, task=2, state=2,
                  substate=2, ui=0, health=0, max_health=24, fade_active=0,
                  gameover_case=0, gameover_menu=1, gameover_cursor=0,
                  gameover_timer=0, gameover_keys=0, gameover_y=37, death_flag=0,
                  display_control=0x1740, palette_zero=0,
                  player_action=1, player_control=0, world_area=3, world_room=8)

    def event(name, value=0, capture=False):
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

    for case in range(count):
        sample.update(gameover_case=case, task=2, state=2, health=0, death_flag=0,
                      gameover_cursor=0, gameover_menu=1, gameover_timer=0, gameover_keys=0)
        event('gameover_death_request', case)
        frame(death_flag=1)
        sample.update(task=3, state=1, health=24)
        event('menu_decoration', 2)
        event('gameover_install')
        sample.update(state=2)
        event('gameover_save_prompt')
        event('gameover_save_ready', case, True)
        if case in (1, 2) or (not extended and case == quit_case):
            sample.update(gameover_cursor=1, gameover_keys=128)
            event('gameover_save_choice', 1)
            if case == 1:
                sample.update(gameover_cursor=0, gameover_keys=64)
                event('gameover_save_choice', 0)
        sample['gameover_keys'] = 1
        event('gameover_save_answer', int(case == 2 or (not extended and case == quit_case)))
        sample['gameover_keys'] = 0
        if case in errors:
            event('gameover_write_fault', 1)
            event('gameover_save_error')
            sample['gameover_menu'] = 3
            for timer in range(60, -1, -1):
                frame(gameover_timer=timer)
            event('gameover_error_ready', case, True)
            event('gameover_error_unchanged', 1)
            event('gameover_dismiss_request', case)
            frame(gameover_keys=2 if extended and case == 3 else 8 if extended else 1)
            sample.update(gameover_menu=1, gameover_keys=0)
            event('gameover_save_prompt')
            event('gameover_retry_ready', case, True)
            if extended and case == 3:
                sample.update(gameover_cursor=1, gameover_keys=128)
                event('gameover_save_choice', 1)
            sample['gameover_keys'] = 1
            event('gameover_save_answer', int(extended and case == 3))
            sample['gameover_keys'] = 0
        if case in saved:
            event('gameover_save_success', 1)
        sample.update(state=3, gameover_cursor=0)
        event('gameover_continue_prompt')
        event('gameover_readback', 24 if case in saved else 0)
        event('gameover_continue_ready', case, True)
        if case in (2, quit_case):
            sample.update(gameover_cursor=1, gameover_keys=128)
            event('gameover_continue_choice', 1)
            if case == 2:
                sample.update(gameover_cursor=0, gameover_keys=64)
                event('gameover_continue_choice', 0)
        sample['gameover_keys'] = 1
        event('gameover_continue_answer', int(case == quit_case))
        event('gameover_quit' if case == quit_case else 'gameover_continue', int(case == quit_case))
        sample.update(task=0 if case == quit_case else 2, state=2, gameover_keys=0, death_flag=0)
        event('gameover_title_returned' if case == quit_case else 'gameover_game_returned', case, True)
    event('gameover_complete', count)
    for row in rows:
        for key in ("task", "state", "substate", "fade_active"):
            row["gameover_" + key] = row[key]
    return rows


class GameOverAuditTests(unittest.TestCase):
    def setUp(self):
        self.rows = trace()

    def reject(self, name, **changes):
        for key in ("task", "state", "substate", "fade_active"):
            if key in changes:
                changes["gameover_" + key] = changes[key]
        rows = copy.deepcopy(self.rows)
        next(r for r in rows if r.get('event') == name).update(changes)
        with self.assertRaises((ValueError, StopIteration)):
            validate_trace(rows, 'gameover')

    def test_complete(self):
        events, frames = validate_trace(self.rows, 'gameover')
        self.assertEqual(len(frames), 17)
        self.assertEqual(sum(r['event'] == 'gameover_save_success' for r in events), 3)

    def test_missing_lifecycle_events(self):
        for name in ('gameover_install', 'gameover_save_answer', 'gameover_save_success',
                     'gameover_save_error', 'gameover_retry_ready', 'gameover_readback',
                     'gameover_quit', 'gameover_title_returned', 'gameover_complete'):
            rows = [r for r in self.rows if r.get('event') != name]
            with self.assertRaises((ValueError, StopIteration)):
                validate_trace(rows, 'gameover')

    def test_native_death_required(self):
        self.reject('gameover_death_request', health=24)
        rows = copy.deepcopy(self.rows)
        for row in rows:
            row['death_flag'] = 0
        with self.assertRaisesRegex(ValueError, 'native death'):
            validate_trace(rows, 'gameover')

    def test_artwork_and_heading_excluded(self):
        self.reject('gameover_save_ready', family='plain')
        self.reject('gameover_continue_ready', heading_style=1)
        self.reject('gameover_save_ready', display_control=0x1f40)
        self.reject('gameover_save_ready', palette_zero=1)
        self.reject('menu_decoration', value=0)

    def test_real_save_input(self):
        self.reject('gameover_save_choice', gameover_keys=16)
        self.reject('gameover_save_answer', value=1)
        self.reject('gameover_save_answer', gameover_keys=2)

    def test_real_continue_input(self):
        self.reject('gameover_continue_choice', value=0)
        self.reject('gameover_continue_answer', gameover_keys=8)
        self.reject('gameover_continue_answer', value=1)

    def test_readback_and_write_fault(self):
        self.reject('gameover_readback', value=0)
        self.reject('gameover_save_success', value=0)
        self.reject('gameover_error_unchanged', value=0)
        self.reject('gameover_write_fault', value=0)

    def test_error_wait_and_dismissal(self):
        rows = copy.deepcopy(self.rows)
        next(r for r in rows if 'event' not in r and r['gameover_timer'] == 59)['gameover_timer'] = 0
        with self.assertRaisesRegex(ValueError, 'wait skipped'):
            validate_trace(rows, 'gameover')
        rows = copy.deepcopy(self.rows)
        dismissal = next(r['frame'] for r in rows if r.get('event') == 'gameover_dismiss_request')
        next(r for r in rows if 'event' not in r and r['frame'] == dismissal + 1)['gameover_keys'] = 0
        with self.assertRaisesRegex(ValueError, 'dismissal input'):
            validate_trace(rows, 'gameover')

    def test_neutral_capture_state(self):
        self.reject('gameover_error_ready', gameover_timer=1)
        self.reject('gameover_save_ready', gameover_cursor=1)
        self.reject('gameover_continue_ready', fade_active=1)
        self.reject('gameover_save_ready', gameover_y=80)
        rows = copy.deepcopy(self.rows)
        rows.remove(next(r for r in rows if r.get('capture')))
        with self.assertRaises(ValueError):
            validate_trace(rows, 'gameover')

    def test_restore_control_room_and_title(self):
        self.reject('gameover_game_returned', world_room=9)
        self.reject('gameover_game_returned', player_control=3)
        self.reject('gameover_title_returned', task=2)

    def test_generated_black_pixels(self):
        events = [r for r in self.rows if 'event' in r]
        neutral = [r for r in events if r['event'] in ('gameover_save_ready', 'gameover_continue_ready',
                                                       'gameover_error_ready', 'gameover_retry_ready')]
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            for event in neutral:
                image = Image.new('RGB', (240, 160))
                image.putpixel((120, 80), (255, 255, 255))  # native foreground is excluded
                image.save(directory / f"{event['frame']:06d}.png")
            self.assertEqual(validate_gameover_pixels(directory, events)['checkpoints'], 12)
            path = directory / f"{neutral[0]['frame']:06d}.png"
            image = Image.open(path).convert('RGB')
            image.putpixel((239, 159), (1, 0, 0))
            image.save(path)
            with self.assertRaisesRegex(ValueError, 'not black'):
                validate_gameover_pixels(directory, events)
            with self.assertRaisesRegex(ValueError, 'incomplete'):
                validate_gameover_pixels(directory, [])


class GameOverReturnTests(unittest.TestCase):
    def test_extended_complete(self):
        events, frames = validate_trace(trace(True), 'gameover_returns')
        self.assertEqual(sum(r['event'] == 'gameover_save_success' for r in events), 4)
        self.assertEqual(sum(r['event'] == 'gameover_save_error' for r in events), 2)
        self.assertEqual(len(frames), 22)

    def test_retry_refusal_and_quit_required(self):
        rows = trace(True)
        next(r for r in rows if r.get('event') == 'gameover_save_answer' and r['gameover_case'] == 3 and r['value'] == 1)['value'] = 0
        with self.assertRaises(ValueError): validate_trace(rows, 'gameover_returns')
        rows = trace(True)
        rows.remove(next(r for r in rows if r.get('event') == 'gameover_save_success' and r['gameover_case'] == 5))
        with self.assertRaises(ValueError): validate_trace(rows, 'gameover_returns')

    def test_b_and_start_error_dismissal(self):
        for case in (3, 4):
            rows = trace(True)
            tick = next(r['frame'] for r in rows if r.get('event') == 'gameover_dismiss_request' and r['gameover_case'] == case) + 1
            next(r for r in rows if 'event' not in r and r['frame'] == tick)['gameover_keys'] = 1
            with self.assertRaises(ValueError): validate_trace(rows, 'gameover_returns')


if __name__ == '__main__':
    unittest.main()
