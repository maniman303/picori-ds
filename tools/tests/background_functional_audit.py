#!/usr/bin/env python3
"""Failure-oriented checks for standalone hints and native dungeon-map audits."""
import copy
from pathlib import Path
import sys
import tempfile
import unittest
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from capture_background_audit import validate_trace
from background_hint_audit import hint_cases, validate_hint_pixels
from background_dungeon_audit import DUNGEONS, dungeon_cases, validate_dungeon_pixels


def base():
    return dict(task=2, state=2, substate=7, ui=3, menu=1, overlay=0, installed=-1,
                fade_active=0, schema=2, family='unknown', heading_style=0,
                heading_x=0, heading_y=0, heading_transition=0, health=24, max_health=24)


def hint_trace():
    rows = []
    for index, case in enumerate(hint_cases()):
        mask = (1, 30, 32, 64, 128, 256)[case['variant']]
        sample = dict(base(), hint_case=index, hint_field=0, hint_variant=case['variant'],
                      hint_timer=60, hint_mask=0, hint_pending=mask,
                      saved_hints=0xffff if case['repeat'] else 0, palette_zero=0x4b99,
                      palette_81=0x4b99, display_control=0x9740, pause_screen=4,
                      decoration=case['decoration'], health=(24, 15, 8)[case['decoration']])

        def event(name, value, capture=False):
            sample['frame'] = len(rows)
            rows.append(dict(sample, event=name, value=value))
            if capture:
                rows.append(dict(sample, capture=True))

        event('hint_request', index)
        event('menu_decoration', case['decoration'])
        event('pause_screen', 4)
        sample['hint_field'] = 1
        sample['saved_hints'] |= mask
        event('hint_screen', case['variant'])
        sample['hint_timer'] = 54
        event('hint_before', index, True)
        sample.update(hint_timer=30, hint_mask=mask)
        event('hint_reveal', mask)
        sample['hint_timer'] = 0
        event('hint_ready', index, True)
        sample['health'] = 24 if case['decoration'] == 2 else 8
        event('hint_health_mutated', sample['health'])
        event('hint_held', index, True)
        event('hint_cancel', case['exit_key'])
        sample['hint_field'] = 0
        event('hint_restored', index)
    rows.append(dict(event='hints_complete', value=36, family='unknown', hint_field=0))
    return rows


def dungeon_trace():
    rows = []
    index = 0
    sample = dict(base(), dungeon_case=-1, ui=1)

    def event(name, value, capture=False):
        sample['frame'] = len(rows)
        rows.append(dict(sample, event=name, value=value))
        if capture:
            rows.append(dict(sample, capture=True))

    for dungeon, area, floors in DUNGEONS:
        event('dungeon_room_request', dungeon)
        sample.update(dungeon_index=dungeon, dungeon_area=area, family='unknown')
        event('dungeon_room_ready', dungeon)
        for opening in range(6):
            sample.update(dungeon_case=index, dungeon_items=7 if opening % 2 else 0, decoration=opening // 2,
                          health=(24, 15, 8)[opening // 2], family='unknown', dungeon_floor=1)
            sample['dungeon_area'] = 0  # Pending request precedes the native room snapshot.
            event('dungeon_request', opening)
            sample['dungeon_area'] = area
            event('menu_decoration', opening // 2)
            sample['family'] = 'titled'
            event('pause_screen', 1)
            event('pause_screen', 5)
            event('dungeon_screen', dungeon)
            sample.update(heading_style=1, heading_x=64, heading_y=16, heading_transition=0, pause_screen=5)
            sample['dungeon_floor'] = 0
            event('dungeon_floor', 0)
            for floor in range(floors):
                sample.update(dungeon_floor=floor, dungeon_case=index)
                if floor:
                    event('dungeon_floor', floor)
                event('dungeon_ready', index, True)
                sample['health'] = 24 if opening // 2 == 2 else 8
                event('dungeon_health_mutated', sample['health'])
                event('dungeon_held', index, True)
                index += 1
            sample['family'] = 'unknown'
            event('dungeon_restored', opening)
    event('dungeons_complete', index)
    return rows


class FunctionalAudit(unittest.TestCase):
    def test_complete_hint_matrix(self):
        self.assertEqual(len(hint_cases()), 36)
        self.assertEqual({c['exit_key'] for c in hint_cases()}, {1, 2, 8})
        validate_trace(hint_trace(), 'hints')

    def test_hint_native_events_required(self):
        for name in ('hint_screen', 'hint_reveal', 'hint_cancel', 'hint_restored'):
            rows = hint_trace()
            rows.pop(next(i for i, r in enumerate(rows) if r.get('event') == name))
            with self.subTest(name=name), self.assertRaises(ValueError):
                validate_trace(rows, 'hints')

    def test_hint_checkpoint_and_capture_require_correct_source_and_reveal(self):
        for name in ('hint_before', 'hint_ready', 'hint_held', None):
            for field, value in (('hint_variant', 4), ('hint_pending', 0), ('hint_mask', 3), ('hint_timer', 30),
                                 ('saved_hints', 0), ('palette_zero', 0), ('display_control', 0x9f40), ('health', 1),
                                 ('family', 'plain'), ('heading_style', 1), ('hint_field', 0)):
                rows = hint_trace()
                next(r for r in rows if r.get('event') == name)[field] = value
                with self.subTest(name=name, field=field), self.assertRaises(ValueError):
                    validate_trace(rows, 'hints')

    def test_hint_repeated_fixture_and_native_reveal_boundary(self):
        for name, field, value in (('hint_request', 'saved_hints', 1), ('hint_reveal', 'hint_timer', 29),
                                    ('hint_cancel', 'value', 4)):
            rows = hint_trace()
            next(r for r in rows if r.get('event') == name)[field] = value
            with self.assertRaises(ValueError):
                validate_trace(rows, 'hints')

    def test_hint_lifetime_and_no_reinstallation(self):
        for name, field, value in (('pause_screen', 'hint_field', 1), ('hint_cancel', 'hint_field', 0)):
            rows = hint_trace()
            next(r for r in rows if r.get('event') == name)[field] = value
            with self.assertRaisesRegex(ValueError, 'outside installation'):
                validate_trace(rows, 'hints')
        rows = hint_trace()
        rows.insert(2, copy.copy(rows[1]))
        with self.assertRaisesRegex(ValueError, 'reinstallation'):
            validate_trace(rows, 'hints')

    def test_hint_pixels_reject_wrong_source_mask_and_corruption(self):
        events = [r for r in hint_trace() if r.get('event') in ('hint_before', 'hint_ready', 'hint_held')]
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            reference = dict(color=0x4b99, masks=[1, 30, 32, 64, 128, 256])
            color = tuple(((reference['color'] >> sh) & 31) << 3 for sh in (0, 5, 10))
            for row in events:
                image = Image.new('RGB', (240, 160), color)
                image.putpixel((120, 80), (255, 255, 255))
                image.save(directory / f"{row['frame']:06d}.png")
            self.assertEqual(validate_hint_pixels(directory, events, reference)['checkpoints'], 108)
            with self.assertRaisesRegex(ValueError, 'native ROM'):
                validate_hint_pixels(directory, events, dict(reference, color=0))
            with self.assertRaisesRegex(ValueError, 'native ROM'):
                validate_hint_pixels(directory, events, dict(reference, masks=[2]*6))
            path = directory / f"{events[0]['frame']:06d}.png"
            with Image.open(path) as image:
                image.putpixel((0, 159), (0, 0, 0))
                image.save(path)
            with self.assertRaisesRegex(ValueError, 'live field'):
                validate_hint_pixels(directory, events)

    def test_all_six_dungeons_and_every_floor_items_health(self):
        cases = dungeon_cases()
        self.assertEqual(len(cases), 138)
        for dungeon, _, floors in DUNGEONS:
            group = [c for c in cases if c['dungeon'] == dungeon]
            self.assertEqual(len(group), floors * 6)
            self.assertEqual({(c['items'], c['decoration']) for c in group}, {(items, health) for items in (0, 7) for health in range(3)})
        validate_trace(dungeon_trace(), 'dungeons')

    def test_native_dungeon_room_install_floor_and_restore_events_required(self):
        for name in ('dungeon_room_request', 'dungeon_room_ready', 'dungeon_screen', 'dungeon_floor', 'dungeon_restored'):
            rows = dungeon_trace()
            rows.pop(next(i for i, r in enumerate(rows) if r.get('event') == name))
            with self.subTest(name=name), self.assertRaises(ValueError):
                validate_trace(rows, 'dungeons')

    def test_dungeon_checkpoint_and_capture_state(self):
        for name in ('dungeon_ready', 'dungeon_held', None):
            for field, value in (('dungeon_index', 0), ('dungeon_area', 3), ('dungeon_items', 7), ('dungeon_floor', 7),
                                 ('decoration', 2), ('family', 'plain'), ('heading_x', 0), ('health', 1)):
                rows = dungeon_trace()
                next(r for r in rows if r.get('event') == name)[field] = value
                with self.subTest(name=name, field=field), self.assertRaises(ValueError):
                    validate_trace(rows, 'dungeons')

    def test_faded_and_missing_captures(self):
        for scenario, factory in (('hints', hint_trace), ('dungeons', dungeon_trace)):
            for field, value in (('fade_active', 1), ('capture', False), ('task', 1), ('ui', 9)):
                rows = factory()
                next(r for r in rows if 'event' not in r)[field] = value
                with self.assertRaisesRegex(ValueError, 'neutral'):
                    validate_trace(rows, scenario)

    def test_dungeon_pixels_follow_installed_health_not_live_health(self):
        rows = [r for r in dungeon_trace() if r.get('event') in ('dungeon_ready', 'dungeon_held')]
        refs = [bytes([color] * 720) for color in (32, 96, 160)]
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            for row in rows:
                color = (32, 96, 160)[row['decoration']]
                image = Image.new('RGB', (240, 160), (color, color, color))
                image.putpixel((100, 80), (255, 255, 255))
                image.save(directory / f"{row['frame']:06d}.png")
            self.assertEqual(validate_dungeon_pixels(directory, rows, refs)['checkpoints'], 276)
            held = next(r for r in rows if r['event'] == 'dungeon_held')
            self.assertEqual(held['health'], 8)
            Image.new('RGB', (240, 160), (160, 160, 160)).save(directory / f"{held['frame']:06d}.png")
            with self.assertRaisesRegex(ValueError, 'installed health'):
                validate_dungeon_pixels(directory, rows, refs)
            with self.assertRaisesRegex(ValueError, 'incomplete'):
                validate_dungeon_pixels(directory, [], refs)

    def test_missing_checkpoint_or_out_of_order_exit_fails(self):
        for scenario, factory, name in (('hints', hint_trace, 'hint_ready'),
                                         ('dungeons', dungeon_trace, 'dungeon_held')):
            rows = factory()
            rows.pop(next(i for i, r in enumerate(rows) if r.get('event') == name))
            with self.assertRaisesRegex(ValueError, 'incomplete or unordered'):
                validate_trace(rows, scenario)
        rows = hint_trace()
        before = next(i for i, r in enumerate(rows) if r.get('event') == 'hint_before')
        reveal = next(i for i, r in enumerate(rows) if r.get('event') == 'hint_reveal')
        rows[before], rows[reveal] = rows[reveal], rows[before]
        with self.assertRaisesRegex(ValueError, 'lifecycle order'):
            validate_trace(rows, 'hints')

    def test_dungeon_restoration_and_no_reinstallation(self):
        rows = dungeon_trace()
        next(r for r in rows if r.get('event') == 'dungeon_restored')['family'] = 'titled'
        with self.assertRaisesRegex(ValueError, 'installation/restoration'):
            validate_trace(rows, 'dungeons')
        rows = dungeon_trace()
        index = next(i for i, r in enumerate(rows) if r.get('event') == 'menu_decoration')
        rows.insert(index, copy.copy(rows[index]))
        with self.assertRaisesRegex(ValueError, 'reinstallation'):
            validate_trace(rows, 'dungeons')


if __name__ == '__main__':
    unittest.main()
