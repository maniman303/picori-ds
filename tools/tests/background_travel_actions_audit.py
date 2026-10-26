#!/usr/bin/env python3
"""Failure-oriented checks for native travel choices and scene replacement."""
from pathlib import Path
import sys
import tempfile
import unittest
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from capture_background_audit import validate_trace
from background_travel_actions_audit import (DESTINATIONS, travel_action_cases, validate_travel_action_pixels)


def trace():
    rows = []
    origin = (3, 8)
    for index, case in enumerate(travel_action_cases()):
        sample = dict(task=2, state=2, substate=7, ui=9, menu=1, overlay=0, installed=-1,
                      fade_active=0, schema=2, family='unknown', heading_style=0,
                      heading_x=0, heading_y=0, heading_transition=0,
                      travel_action_case=index, travel_field=0, travel_destination=0 if case['unlocks'] == 255 else 7,
                      travel_unlocks=case['unlocks'], decoration=case['decoration'], pause_screen=13,
                      health=case['health'], max_health=24, display_control=0x9f40,
                      travel_story_progress=5, travel_ocarina=1, travel_story_flags=31, travel_first_warp=0 if index < 2 else 1, travel_phase=1, travel_choice=-1, travel_message=0, travel_game_state=2, travel_subtask=7,
                      travel_fade_active=0, travel_fade_type=4, travel_fade_progress=0, travel_fade_mask=0xffffffff,
                      world_area=origin[0], world_room=origin[1], player_layer=1, player_action=1, player_control=0)

        def event(name, value, capture=False):
            sample['frame'] = len(rows)
            rows.append(dict(sample, event=name, value=value))
            if capture:
                rows.append(dict(sample, capture=True))

        event('travel_action_request', index)
        event('menu_decoration', case['decoration'])
        event('pause_screen', 13)
        sample['travel_field'] = 1
        event('fast_travel_screen', sample['travel_destination'])
        if case['unlocks'] == 255:
            for choice in range(1, case['destination'] + 1):
                sample['travel_destination'] = choice
                event('fast_travel_choice', choice)
        event('travel_action_ready', index, True)
        sample['health'] = 24 if case['decoration'] == 2 else 8
        event('travel_action_health_mutated', sample['health'])
        event('travel_action_held', index, True)
        sample.update(travel_phase=2, travel_choice=0 if case['accept'] else 1, travel_message=1)
        event('fast_travel_prompt', case['destination'])
        event('travel_action_prompt_ready', index, True)
        sample['travel_message'] = 0
        event('fast_travel_answer', sample['travel_choice'])
        if case['accept']:
            sample.update(travel_fade_active=1, travel_fade_type=5, travel_phase=4)
            event('fast_travel_fade', case['destination'])
            sample['travel_fade_active'] = 0
            event('fast_travel_depart', case['destination'])
            target = DESTINATIONS[case['destination']]
            sample.update(target_area=target[0], target_room=target[1], target_x=target[2], target_y=target[3], target_layer=target[4],
                          travel_field=0, travel_game_state=1, travel_subtask=0)
            event('fast_travel_handoff', case['destination'])
            sample.update(world_area=target[0], world_room=target[1], travel_first_warp=1)
            result = 'travel_action_arrived'
            origin = target[:2]
        else:
            event('fast_travel_cancel', case['destination'])
            sample['travel_field'] = 0
            result = 'travel_action_restored'
        sample.update(travel_game_state=2, travel_subtask=2)
        event(result, index, True)
    rows.append(dict(event='travel_actions_complete', value=54, travel_field=0, family='unknown'))
    return rows


class TravelActions(unittest.TestCase):
    def test_all_destinations_health_answers_and_sparse_unlocks(self):
        self.assertEqual(len(travel_action_cases()), 54)
        for health in range(3):
            group = [c for c in travel_action_cases() if c['decoration'] == health]
            self.assertEqual([(c['destination'], c['accept']) for c in group], [(min(view, 7), answer) for view in range(9) for answer in range(2)])
            self.assertEqual([c['unlocks'] for c in group], [255] * 16 + [128] * 2)
        validate_trace(trace(), 'travel_actions')

    def test_missing_native_lifecycle_events_fail(self):
        for name in ('fast_travel_screen', 'fast_travel_prompt', 'fast_travel_answer', 'fast_travel_cancel',
                     'fast_travel_fade', 'fast_travel_depart', 'fast_travel_handoff', 'travel_action_arrived', 'travel_action_restored'):
            rows = trace()
            rows.pop(next(i for i, r in enumerate(rows) if r.get('event') == name))
            with self.subTest(name=name), self.assertRaises(ValueError):
                validate_trace(rows, 'travel_actions')

    def test_missing_or_faded_neutral_checkpoints(self):
        for name in ('travel_action_ready', 'travel_action_held', 'travel_action_prompt_ready'):
            rows = trace()
            rows.pop(next(i for i, r in enumerate(rows) if r.get('event') == name))
            with self.assertRaisesRegex(ValueError, 'incomplete or unordered'):
                validate_trace(rows, 'travel_actions')
        for field, value in (('capture', False), ('fade_active', 1), ('task', 1), ('ui', 1)):
            rows = trace()
            next(r for r in rows if 'event' not in r)[field] = value
            with self.assertRaisesRegex(ValueError, 'neutral travel'):
                validate_trace(rows, 'travel_actions')

    def test_checkpoint_and_capture_context(self):
        for name in ('travel_action_ready', 'travel_action_held', 'travel_action_prompt_ready', None):
            for field, value in (('travel_destination', 6), ('travel_unlocks', 1), ('decoration', 2),
                                 ('family', 'plain'), ('health', 1), ('heading_style', 1), ('travel_field', 0),
                                 ('display_control', 0x9740)):
                rows = trace()
                next(r for r in rows if r.get('event') == name)[field] = value
                with self.subTest(name=name, field=field), self.assertRaises(ValueError):
                    validate_trace(rows, 'travel_actions')

    def test_native_enquiry_and_selected_answer_required(self):
        for field, value in (('travel_phase', 1), ('travel_choice', 0), ('travel_message', 0)):
            rows = trace()
            next(r for r in rows if r.get('event') == 'travel_action_prompt_ready')[field] = value
            with self.assertRaisesRegex(ValueError, 'enquiry/selected choice'):
                validate_trace(rows, 'travel_actions')
        rows = trace()
        next(r for r in rows if r.get('event') == 'fast_travel_answer')['value'] = 0
        with self.assertRaisesRegex(ValueError, 'native travel answer'):
            validate_trace(rows, 'travel_actions')

    def test_fade_handoff_target_and_normal_control(self):
        for name, field, value, message in (
                ('fast_travel_fade', 'travel_fade_type', 4, 'outgoing travel fade'),
                ('fast_travel_depart', 'travel_fade_active', 1, 'before fade completion'),
                ('fast_travel_handoff', 'target_room', 63, 'destination handoff'),
                ('fast_travel_handoff', 'target_x', 0, 'destination handoff'),
                ('fast_travel_handoff', 'travel_field', 1, 'replace the installed field'),
                ('travel_action_arrived', 'world_area', 3, 'arrival/restoration room'),
                ('travel_action_restored', 'world_room', 0, 'arrival/restoration room'),
                ('travel_action_arrived', 'player_action', 0, 'normal gameplay control'),
                ('travel_action_arrived', 'player_control', 1, 'normal gameplay control')):
            rows = trace()
            next(r for r in rows if r.get('event') == name)[field] = value
            with self.subTest(name=name, field=field), self.assertRaisesRegex(ValueError, message):
                validate_trace(rows, 'travel_actions')

    def test_field_cannot_publish_early_or_clear_at_prompt_cancel_depart(self):
        for name, value in (('pause_screen', 1), ('fast_travel_prompt', 0), ('fast_travel_cancel', 0)):
            rows = trace()
            next(r for r in rows if r.get('event') == name)['travel_field'] = value
            with self.assertRaisesRegex(ValueError, 'outside installation/replacement'):
                validate_trace(rows, 'travel_actions')

    def test_completed_capture_keeps_case_room_and_control(self):
        for name in ('travel_action_arrived', 'travel_action_restored'):
            for field, value in (('travel_action_case', 54), ('world_area', 63), ('player_control', 3)):
                rows = trace()
                result = next(r for r in rows if r.get('event') == name)
                next(r for r in rows if r.get('frame') == result['frame'] and 'event' not in r)[field] = value
                with self.assertRaisesRegex(ValueError, 'disagrees with native gameplay'):
                    validate_trace(rows, 'travel_actions')

    def test_first_travel_tutorial_flag_cannot_be_preset_or_lost(self):
        for name, value in (('travel_action_request', 1), ('travel_action_arrived', 0)):
            rows = trace()
            next(r for r in rows if r.get('event') == name)['travel_first_warp'] = value
            with self.assertRaisesRegex(ValueError, 'first/repeated|tutorial flag'):
                validate_trace(rows, 'travel_actions')

    def test_story_fixture_must_allow_native_town_and_ocarina(self):
        for name in ('travel_action_request', 'travel_action_ready'):
            for field, value in (('travel_story_progress', 1), ('travel_ocarina', 0), ('travel_story_flags', 0)):
                rows = trace()
                next(r for r in rows if r.get('event') == name)[field] = value
                with self.assertRaisesRegex(ValueError, 'story fixture'):
                    validate_trace(rows, 'travel_actions')

    def test_no_reinstallation_or_incorrect_navigation(self):
        rows = trace()
        i = next(i for i, r in enumerate(rows) if r.get('event') == 'menu_decoration')
        rows.insert(i, dict(rows[i]))
        with self.assertRaisesRegex(ValueError, 'reinstallation'):
            validate_trace(rows, 'travel_actions')
        rows = trace()
        next(r for r in rows if r.get('event') == 'fast_travel_choice')['value'] = 7
        with self.assertRaisesRegex(ValueError, 'navigation'):
            validate_trace(rows, 'travel_actions')

    def test_pixels_exclude_dialogue_and_gameplay_but_detect_stale_field(self):
        events = [r for r in trace() if r.get('event') in ('travel_action_ready', 'travel_action_held', 'travel_action_prompt_ready')]
        reference = [(168, 216, 248)] * 2
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            for row in events:
                image = Image.new('RGB', (240, 160), reference[0])
                image.putpixel((120, 159), (0, 0, 0))
                image.save(directory / f"{row['frame']:06d}.png")
            self.assertEqual(validate_travel_action_pixels(directory, events, reference)['checkpoints'], 162)
            path = directory / f"{events[0]['frame']:06d}.png"
            with Image.open(path) as image:
                image.putpixel((239, 0), (0, 0, 0))
                image.save(path)
            with self.assertRaisesRegex(ValueError, 'fixed BG3'):
                validate_travel_action_pixels(directory, events, reference)
            with self.assertRaisesRegex(ValueError, 'incomplete'):
                validate_travel_action_pixels(directory, [], reference)


if __name__ == '__main__':
    unittest.main()
