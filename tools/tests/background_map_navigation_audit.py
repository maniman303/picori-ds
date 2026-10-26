#!/usr/bin/env python3
"""Failure-oriented native navigation/source lifetime audit checks."""
import copy
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from PIL import Image
from capture_background_audit import validate_trace
from background_map_navigation_audit import hint_variants, validate_navigation_pixels


def trace():
    rows = []; clock = 0
    sample = dict(schema=2, family='unknown', heading_style=0, nav_case=0, nav_target=0, nav_hints=len(hint_variants()),
                  nav_cursor=0, nav_markers=1, nav_keys=0, nav_task=2, nav_substate=7, nav_ui=1, nav_next=2,
                  world_area=3, world_room=8, health=24, nav_fade=0, nav_id=0, nav_icon=0, nav_timer=0, palette_zero=0, pause_screen=4, decoration=0)
    def event(name, value, capture=False):
        nonlocal clock
        clock += 1; sample['frame'] = clock
        rows.append(dict(sample, event=name, value=value))
        if capture: rows.append(dict(sample, capture=True))
    def key(name, value):
        nonlocal clock
        event(name, value); clock += 1; sample.update(frame=clock, nav_keys=value)
        rows.append(dict(sample, capture=False))
    for index in range(44):
        recipe = index % 22
        target = recipe if recipe < 17 else 9 if recipe == 17 else 11 if recipe == 18 else 14
        sample.update(nav_case=index, nav_cursor=target, nav_target=target, nav_ui=1, nav_substate=7,
                      family='titled', heading_style=1, pause_screen=4, decoration=0 if index < 22 else 2,
                      nav_markers=1 if index < 22 else 4)
        if index % 22 == 0: event('nav_open', index // 22)
        event('nav_world_ready', index, True)
        key('nav_enter_input', 1)
        event('world_detail_enter', target)
        sample['pause_screen'] = 6
        event('map_detail', 0x5f + recipe if recipe < 17 else 0x70 + recipe - 17)
        event('nav_detail_ready', index, True)
        key('nav_back_input', 2)
        sample['pause_screen'] = 4
        event('nav_world_returned', index, True)
        if index % 22 == 21:
            sample.update(nav_substate=2, family='unknown', heading_style=0)
            event('nav_game_restored', index // 22)
    for index, hint in enumerate(hint_variants(), 44):
        sample.update(nav_case=index, family='unknown', heading_style=0, nav_ui=10, nav_cursor=hint['area'],
                      nav_substate=7, health=(24, 15, 8)[index % 3], nav_icon=hint['icon'], nav_id=hint['id'], palette_zero=0x475f)
        event('nav_hint_request', hint['id'])
        event('fusion_local_hint_install', hint['area'])
        event('nav_hint_ready', hint['id'], True)
        sample['health'] = 8 if sample['health'] == 24 else 24
        event('nav_hint_held', hint['id'], True)
        key('nav_hint_exit_input', (1, 2, 8)[index % 3])
        sample.update(nav_ui=0, nav_substate=2)
        event('nav_hint_restored', hint['id'], True)
    event('map_navigation_complete', 44 + len(hint_variants()))
    return rows


class MapNavigationTests(unittest.TestCase):
    def test_complete_and_independent_hints(self):
        events, frames = validate_trace(trace(), 'map_navigation')
        self.assertEqual(len(hint_variants()), 49)
        self.assertEqual(len(frames), 132 + 3 * 49)
        self.assertEqual(len({(h['area'], h['icon']) for h in hint_variants()}), 49)

    def reject(self, name, **changes):
        rows = trace()
        next(r for r in rows if r.get('event') == name).update(changes)
        with self.assertRaises((ValueError, StopIteration)): validate_trace(rows, 'map_navigation')

    def test_missing_native_checkpoints_and_handoff(self):
        for name in ('map_navigation_complete', 'nav_detail_ready', 'world_detail_enter',
                     'nav_world_returned', 'fusion_local_hint_install', 'nav_hint_restored'):
            rows = trace(); rows.remove(next(r for r in rows if r.get('event') == name))
            with self.assertRaises((ValueError, StopIteration)): validate_trace(rows, 'map_navigation')

    def test_cursor_input_and_recipes(self):
        self.reject('world_detail_enter', nav_keys=2)
        self.reject('world_detail_enter', value=16)
        self.reject('map_detail', value=0x70)
        self.reject('nav_world_returned', nav_cursor=16)

    def test_source_and_neutral_capture(self):
        self.reject('nav_detail_ready', family='plain')
        self.reject('nav_detail_ready', decoration=2)
        self.reject('nav_detail_ready', nav_fade=1)
        self.reject('nav_hint_held', family='titled')
        self.reject('nav_hint_ready', palette_zero=0)
        self.reject('nav_hint_ready', nav_timer=30)

    def test_hint_variants_cannot_alias(self):
        self.reject('nav_hint_ready', nav_icon=8)
        self.reject('nav_hint_ready', nav_cursor=16)
        self.reject('nav_hint_ready', value=100)
        self.reject('nav_hint_restored', nav_substate=7)
        self.reject('nav_game_restored', world_room=9)
        self.reject('nav_hint_held', health=0)

    def test_marker_fixture_and_bounds(self):
        rows = trace()
        for r in rows: r['nav_markers'] = 1
        with self.assertRaisesRegex(ValueError, 'added no native markers'): validate_trace(rows, 'map_navigation')
        self.reject('nav_detail_ready', nav_markers=129)

    def test_key_edges_and_missing_capture(self):
        rows = trace()
        next(r for r in rows if 'event' not in r and not r.get('capture'))['nav_keys'] = 0
        with self.assertRaisesRegex(ValueError, 'key edge'): validate_trace(rows, 'map_navigation')
        rows = trace(); rows.remove(next(r for r in rows if r.get('capture')))
        with self.assertRaises(ValueError): validate_trace(rows, 'map_navigation')

    def test_source_pixels(self):
        events = [r for r in trace() if 'event' in r]
        refs = [bytes([16 + d * 8]) * 720 for d in range(3)]
        phases = ('nav_world_ready', 'nav_detail_ready', 'nav_world_returned', 'nav_hint_ready', 'nav_hint_held')
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            for r in events:
                if r['event'] in phases:
                    color = (248, 208, 136) if r['event'].startswith('nav_hint') else (16 + r['decoration'] * 8,) * 3
                    Image.new('RGB', (240, 160), color).save(directory / f"{r['frame']:06d}.png")
            self.assertEqual(validate_navigation_pixels(directory, events, refs)['checkpoints'], 230)
            r = next(r for r in events if r['event'] == 'nav_hint_held')
            Image.new('RGB', (240, 160)).save(directory / f"{r['frame']:06d}.png")
            with self.assertRaisesRegex(ValueError, 'field pixels'): validate_navigation_pixels(directory, events, refs)


if __name__ == '__main__': unittest.main()
