#!/usr/bin/env python3
"""Failure-oriented checks for file commits and post-credits Save evidence."""
import copy
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from PIL import Image
from capture_background_audit import validate_trace
from background_write_actions_audit import validate_ending_pixels, validate_file_pixels


class Fixture:
    def __init__(self):
        self.rows = []
        self.clock = 0
        self.sample = dict(schema=2, family='unknown', heading_style=0, installed=-1,
                           palette_zero=0, display_control=0x640, health=24, max_health=24,
                           ending_case=-1, file_write_case=-1, writes_task=4, writes_state=2,
                           writes_ui=0, writes_menu=1, writes_overlay=1, writes_timer=0,
                           writes_keys=0, writes_fade=0, writes_column=0, writes_row=0,
                           writes_dest=4, writes_name=0, writes_name_len=0,
                           writes_clear=1, writes_staffroll=1, writes_slot0=1, writes_slot1=0, writes_slot2=0)

    def event(self, name, value, capture=False):
        self.clock += 1
        self.sample['frame'] = self.clock
        self.rows.append(dict(self.sample, event=name, value=value))
        if capture: self.rows.append(dict(self.sample, capture=True))

    def frame(self, **changes):
        self.clock += 1
        self.sample.update(frame=self.clock, **changes)
        self.rows.append(dict(self.sample, capture=False))

    def key(self, event, key):
        self.event(event, key)
        self.frame(writes_keys=key)


def ending_trace():
    f = Fixture()
    for case in range(4):
        f.sample.update(ending_case=case, health=(24, 15, 8, 15)[case], writes_task=4,
                        writes_state=2, writes_overlay=1, writes_timer=0, writes_keys=0)
        f.event('ending_request', case)
        f.event('credits_save', 0)
        f.event('ending_ready', case, True)
        if case == 1:
            f.key('ending_input', 128)
            f.event('ending_choice', 1)
        f.key('ending_input', 1)
        f.event('ending_answer', int(case == 1))
        if case >= 2:
            f.event('writes_write_fault', 1)
            f.event('ending_error', 0)
            f.sample['writes_overlay'] = 3
            for timer in range(60, -1, -1): f.frame(writes_timer=timer)
            f.event('ending_error_ready', case, True)
            f.event('ending_error_unchanged', 1)
            f.key('ending_dismiss_input', 2 if case == 2 else 8)
            f.sample.update(writes_overlay=1, writes_keys=0)
            f.event('ending_retry_ready', case, True)
            if case == 3:
                f.key('ending_retry_input', 128)
                f.event('ending_choice', 1)
            f.key('ending_retry_input', 1)
            f.event('ending_answer', int(case == 3))
        if case in (0, 2): f.event('ending_success', 1)
        f.event('ending_departure', 0)
        f.sample['writes_task'] = 0
        f.event('ending_readback', 24 if case == 0 else 8 if case == 2 else 0)
        f.event('ending_title_returned', case, True)
    f.event('ending_complete', 4)
    return f.rows


def file_trace(checked=False, extended=False):
    f = Fixture()
    f.sample.update(writes_task=1, writes_state=1, writes_ui=0)
    if checked: f.sample["writes_delete_checked"] = 1
    for case in range(9 if extended else 7):
        create, copyfile, error = case in (0, 4), case in (1, 5), 4 <= case <= 7
        f.sample.update(file_write_case=case, writes_ui=0, writes_keys=0)
        f.event('file_write_request', case)
        row = 1 if create else 0
        for _ in range(abs(f.sample['writes_row'] - row)):
            f.key('file_write_input', 64 if f.sample['writes_row'] > row else 128)
        f.sample['writes_row'] = row
        f.event('file_write_root_ready', case, True)
        f.key('file_write_input', 1)
        f.sample['writes_ui'] = 1 if create else 4
        f.event('file_write_entry_ready', case, True)
        if create:
            f.key('file_write_input', 1)
            f.event('file_name_char', 65)
            f.key('file_write_input', 8)
            f.key('file_write_input', 8)
        else:
            f.key('file_write_input', 16)
            if not copyfile: f.key('file_write_input', 16)
            f.key('file_write_input', 1)
            f.sample['writes_ui'] = 5 if copyfile else 6
            f.event('file_write_operation_ready', case, True)
            if copyfile:
                f.key('file_write_input', 64)
                f.key('file_write_input', 64)
            elif case != 2:
                f.key('file_write_input', 16)
            f.key('file_write_input', 1)
        if error: f.event('file_host_fault' if case == 7 else 'writes_write_fault', 1)
        result = 'file_new_result' if create else 'file_copy_result' if copyfile else 'file_erase_result'
        if case != 2: f.event(result, 0xffffffff if error and (create or copyfile or checked) else 1)
        if error and (create or copyfile or checked):
            for timer in range(30, -1, -1): f.frame(writes_timer=timer)
            if case == 7:
                f.event('file_host_disk_unchanged', 1)
                f.event('file_host_recovered', 1)
            f.event('file_write_error_ready', case, True)
            f.key('file_write_dismiss_input', 1)
        f.sample['writes_ui'] = 0
        status = (1 if create or copyfile else 0) if not error and case != 2 else (0 if create or copyfile else 1)
        f.event('file_write_readback', status)
        f.sample.update(writes_slot0=0 if case in (3, 8) or (case == 6 and not checked) else 1, writes_slot1=1 if case in (0, 1) else 0, writes_slot2=0)
        f.event('file_write_returned', case, True)
    f.event('file_writes_complete', 9 if extended else 7)
    return f.rows


class WriteActionTests(unittest.TestCase):
    def reject(self, scenario, name, **changes):
        rows = ending_trace() if scenario == 'ending_actions' else file_trace()
        next(r for r in rows if r.get('event') == name).update(changes)
        with self.assertRaises((ValueError, StopIteration)):
            validate_trace(rows, scenario)

    def test_complete_ending(self):
        events, frames = validate_trace(ending_trace(), 'ending_actions')
        self.assertEqual(len(frames), 12)
        self.assertEqual(sum(r['event'] == 'ending_success' for r in events), 2)

    def test_complete_files(self):
        _, frames = validate_trace(file_trace(), 'file_writes')
        self.assertEqual(len(frames), 28)

    def test_missing_ending_events(self):
        for name in ('ending_complete', 'credits_save', 'ending_success', 'ending_error',
                     'ending_error_unchanged', 'ending_dismiss_input', 'ending_retry_ready',
                     'ending_departure', 'ending_title_returned'):
            rows = ending_trace()
            rows.remove(next(r for r in rows if r.get('event') == name))
            with self.assertRaises((ValueError, StopIteration)): validate_trace(rows, 'ending_actions')

    def test_ending_input_and_readback(self):
        for name, changes in (('ending_answer', dict(writes_keys=2)), ('ending_choice', dict(value=0)),
                              ('ending_readback', dict(value=0)), ('ending_error_unchanged', dict(value=0)),
                              ('ending_dismiss_input', dict(value=1)), ('ending_request', dict(writes_staffroll=0))):
            self.reject('ending_actions', name, **changes)

    def test_ending_error_timer_and_key_edges(self):
        rows = ending_trace()
        next(r for r in rows if 'event' not in r and r['writes_timer'] == 59)['writes_timer'] = 0
        with self.assertRaisesRegex(ValueError, 'wait skipped'): validate_trace(rows, 'ending_actions')
        rows = ending_trace()
        next(r for r in rows if 'event' not in r and r.get('writes_keys') == 1)['writes_keys'] = 0
        with self.assertRaisesRegex(ValueError, 'key edge'): validate_trace(rows, 'ending_actions')

    def test_ending_background_and_title(self):
        for changes in (dict(installed=0), dict(palette_zero=1), dict(display_control=0xe40),
                        dict(writes_overlay=4), dict(writes_fade=1), dict(family='plain')):
            self.reject('ending_actions', 'ending_ready', **changes)
        self.reject('ending_actions', 'ending_title_returned', writes_task=4)
        self.reject('file_writes', 'file_write_returned', writes_slot1=0)

    def test_missing_file_events(self):
        for name in ('file_writes_complete', 'file_write_request', 'file_name_char', 'file_copy_result',
                     'file_erase_result', 'writes_write_fault', 'file_write_error_ready', 'file_write_returned'):
            rows = file_trace()
            rows.remove(next(r for r in rows if r.get('event') == name))
            with self.assertRaises((ValueError, StopIteration)): validate_trace(rows, 'file_writes')

    def test_file_navigation_and_results(self):
        for name, changes in (('file_name_char', dict(value=0)), ('file_write_input', dict(value=8)),
                              ('file_copy_result', dict(value=0xffffffff)), ('file_erase_result', dict(value=0)),
                              ('file_write_readback', dict(value=0)), ('file_write_returned', dict(writes_ui=5))):
            self.reject('file_writes', name, **changes)

    def test_file_error_wait_and_capture(self):
        rows = file_trace()
        next(r for r in rows if 'event' not in r and r.get('writes_timer') == 29)['writes_timer'] = 0
        with self.assertRaisesRegex(ValueError, 'wait skipped'): validate_trace(rows, 'file_writes')
        rows = file_trace()
        rows.remove(next(r for r in rows if r.get('capture')))
        with self.assertRaises(ValueError): validate_trace(rows, 'file_writes')

    def test_failed_erase_cache_is_distinct_from_storage(self):
        for name, changes in (('file_write_returned', dict(writes_slot0=1)),
                              ('file_write_readback', dict(value=0))):
            rows = file_trace()
            next(r for r in rows if r.get('event') == name and r['file_write_case'] == 6).update(changes)
            with self.assertRaises(ValueError): validate_trace(rows, 'file_writes')

    def test_file_palette_frame_must_be_coherent(self):
        events = [r for r in file_trace() if 'event' in r]
        references = [bytes([value]) * (240 * 160 * 3) for value in (16, 32)]
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            checkpoints = [r for r in events if r['event'] in ('file_write_root_ready', 'file_write_entry_ready',
                                                               'file_write_operation_ready', 'file_write_error_ready', 'file_write_returned')]
            for r in checkpoints: Image.new('RGB', (240, 160), (16,) * 3).save(directory / f"{r['frame']:06d}.png")
            self.assertEqual(validate_file_pixels(directory, events, references)['checkpoints'], 28)
            image = Image.new('RGB', (240, 160), (16,) * 3)
            image.putpixel((239, 159), (32,) * 3)
            image.save(directory / f"{checkpoints[0]['frame']:06d}.png")
            with self.assertRaisesRegex(ValueError, 'ROM reference'): validate_file_pixels(directory, events, references)

    def test_ending_black_pixels(self):
        events = [r for r in ending_trace() if 'event' in r]
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            checkpoints = [r for r in events if r['event'] in ('ending_ready', 'ending_error_ready', 'ending_retry_ready')]
            for r in checkpoints:
                image = Image.new('RGB', (240, 160))
                image.putpixel((120, 80), (255, 255, 255))
                image.save(directory / f"{r['frame']:06d}.png")
            self.assertEqual(validate_ending_pixels(directory, events)['checkpoints'], 8)
            Image.new('RGB', (240, 160), 'white').save(directory / f"{checkpoints[0]['frame']:06d}.png")
            with self.assertRaisesRegex(ValueError, 'black field'): validate_ending_pixels(directory, events)

    def test_file_rom_pixels(self):
        events = [r for r in file_trace() if 'event' in r]
        reference = bytes([16]) * (240 * 160 * 3)
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            checkpoints = [r for r in events if r['event'] in ('file_write_root_ready', 'file_write_entry_ready', 'file_write_operation_ready', 'file_write_error_ready', 'file_write_returned')]
            for r in checkpoints: Image.new('RGB', (240, 160), (16,) * 3).save(directory / f"{r['frame']:06d}.png")
            self.assertEqual(validate_file_pixels(directory, events, reference)['checkpoints'], 28)
            Image.new('RGB', (240, 160)).save(directory / f"{checkpoints[0]['frame']:06d}.png")
            with self.assertRaisesRegex(ValueError, 'ROM reference'): validate_file_pixels(directory, events, reference)


class CheckedEraseTests(unittest.TestCase):
    def test_checked_erase_and_host_failure(self):
        events, frames = validate_trace(file_trace(True, True), 'file_errors')
        self.assertEqual(len(frames), 38)
        self.assertEqual(sum(r['event'] == 'file_host_disk_unchanged' for r in events), 1)

    def test_false_success_and_empty_cache_rejected(self):
        for name, changes in (('file_erase_result', dict(value=1)), ('file_write_returned', dict(writes_slot0=0))):
            rows = file_trace(True, True)
            next(r for r in rows if r.get('event') == name and r['file_write_case'] == 6).update(changes)
            with self.assertRaises(ValueError): validate_trace(rows, 'file_errors')

    def test_durable_host_readback_required(self):
        rows = file_trace(True, True)
        next(r for r in rows if r.get('event') == 'file_host_disk_unchanged')['value'] = 0
        with self.assertRaises(ValueError): validate_trace(rows, 'file_errors')
        rows = file_trace(True, True)
        rows.remove(next(r for r in rows if r.get('event') == 'file_host_fault'))
        with self.assertRaises(ValueError): validate_trace(rows, 'file_errors')

    def test_successful_recovery_required(self):
        rows = file_trace(True, True)
        next(r for r in rows if r.get('event') == 'file_erase_result' and r['file_write_case'] == 8)['value'] = 0xffffffff
        with self.assertRaises(ValueError): validate_trace(rows, 'file_errors')


if __name__ == '__main__': unittest.main()
