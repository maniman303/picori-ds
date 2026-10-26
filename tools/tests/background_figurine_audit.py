#!/usr/bin/env python3
"""Failure-oriented coverage checks for the native figurine route."""
import copy
from pathlib import Path
import sys
import tempfile
import unittest
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from capture_background_audit import validate_trace
from background_figurine_audit import figurine_cases, validate_figurine_pixels


def trace():
    rows = []
    for opening in range(6):
        sample = dict(task=2, substate=7, ui=7, state=1, menu=1, overlay=0, installed=-1,
                      fade_active=0, fade_progress=0, schema=2, family='unknown',
                      heading_style=0, heading_x=0, heading_y=0, heading_transition=0,
                      figurine_field=0, figurine_case=sum(4 + i % 2 for i in range(opening)),
                      figurine_index=1, figurine_owned=1, staffroll=opening % 2,
                      health=(24, 15, 8)[opening // 2], max_health=24,
                      decoration=opening // 2, pause_screen=3, palette_zero=0x1234, palette_251=0x1234)

        def event(name, value, capture=False):
            sample['frame'] = len(rows)
            rows.append(dict(sample, event=name, value=value))
            if capture:
                rows.append(dict(sample, capture=True))

        event('figurine_request', opening)
        event('menu_decoration', opening // 2)
        event('pause_screen', 3)
        sample['figurine_field'] = 1
        event('figurine_screen', 1)
        for index, case in enumerate(figurine_cases()):
            if case['opening'] != opening:
                continue
            sample['figurine_case'] = index
            current = sample['figurine_index']
            while current < case['index']:
                current += 5 if case['index'] - current >= 5 else 1
                sample['figurine_index'] = current
                event('figurine_choice', current)
            sample['figurine_owned'] = case['owned']
            event('figurine_ready', index, True)
            sample['health'] = 24 if opening // 2 == 2 else 8
            event('figurine_health_mutated', sample['health'])
            event('figurine_held', index, True)
        event('figurine_cancel', sample['figurine_index'])
        sample['figurine_field'] = 0
        event('figurine_restored', opening)
    rows.append(dict(event='figurines_complete', value=27, figurine_field=0, family='unknown'))
    return rows


class Figurines(unittest.TestCase):
    def test_coverage(self):
        self.assertEqual(len(figurine_cases()), 27)
        for opening in range(6):
            cases = [c for c in figurine_cases() if c['opening'] == opening]
            self.assertEqual([c['index'] for c in cases], [1, 2, 7, 130] + ([136] if opening % 2 else []))
            self.assertEqual([c['owned'] for c in cases], [1, 0, 0, 1] + ([1] if opening % 2 else []))
        validate_trace(trace(), 'figurines')

    def test_missing_native_events(self):
        for name in ('figurine_request', 'figurine_screen', 'figurine_choice', 'figurine_cancel', 'figurine_restored'):
            rows = trace()
            rows.pop(next(i for i, r in enumerate(rows) if r.get('event') == name))
            with self.subTest(name=name), self.assertRaises(ValueError):
                validate_trace(rows, 'figurines')

    def test_incorrect_checkpoint_and_capture(self):
        for name in ('figurine_ready', 'figurine_held', None):
            for field, value in (('figurine_index', 136), ('figurine_owned', 0), ('staffroll', 1),
                                 ('decoration', 2), ('figurine_field', 0), ('pause_screen', 1),
                                 ('health', 1), ('heading_style', 1), ('palette_zero', 1)):
                rows = trace()
                next(r for r in rows if r.get('event') == name)[field] = value
                with self.subTest(name=name, field=field), self.assertRaises(ValueError):
                    validate_trace(rows, 'figurines')

    def test_neutral_capture_required(self):
        for field, value in (('capture', False), ('fade_active', 1)):
            rows = trace()
            next(r for r in rows if 'event' not in r)[field] = value
            with self.assertRaisesRegex(ValueError, 'neutral figurine'):
                validate_trace(rows, 'figurines')

    def test_no_early_source_or_cancel_clear(self):
        for name, value in (('pause_screen', 1), ('figurine_cancel', 0)):
            rows = trace()
            next(r for r in rows if r.get('event') == name)['figurine_field'] = value
            with self.assertRaisesRegex(ValueError, 'outside installation'):
                validate_trace(rows, 'figurines')

    def test_no_menu_alias_or_reinstallation(self):
        rows = trace()
        next(r for r in rows if r.get('event') == 'figurine_screen')['family'] = 'titled'
        with self.assertRaisesRegex(ValueError, 'non-menu route'):
            validate_trace(rows, 'figurines')
        rows = trace()
        rows.insert(2, copy.copy(rows[1]))
        with self.assertRaisesRegex(ValueError, 'reinstallation'):
            validate_trace(rows, 'figurines')

    def test_pixel_mask_excludes_foreground_and_detects_bad_field(self):
        events = [r for r in trace() if r.get('event') in ('figurine_ready', 'figurine_held')]
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            color = events[0]['palette_zero']
            rgb = tuple(((color >> shift) & 31) << 3 for shift in (0, 5, 10))
            for row in events:
                image = Image.new('RGB', (240, 160), rgb)
                image.putpixel((120, 80), (255, 255, 255))
                image.save(directory / f"{row['frame']:06d}.png")
            self.assertEqual(validate_figurine_pixels(directory, events)['checkpoints'], 54)
            path = directory / f"{events[0]['frame']:06d}.png"
            with Image.open(path) as image:
                image.putpixel((100, 40), (0, 0, 0))
                image.save(path)
            with self.assertRaisesRegex(ValueError, 'live palette-zero'):
                validate_figurine_pixels(directory, events)
            with self.assertRaisesRegex(ValueError, 'incomplete'):
                validate_figurine_pixels(directory, [])


if __name__ == '__main__':
    unittest.main()
