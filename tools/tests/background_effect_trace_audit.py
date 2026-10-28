#!/usr/bin/env python3
"""Ensure the effect audit requires actual native transition observations."""
import copy
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from background_effect_trace_audit import check_traces


class TraceTests(unittest.TestCase):
    def setUp(self):
        self.coverage = {'images': {name: {'layer_mask': mask} for name, mask in
                         (('title_screen', 2), ('kinstone_pieces_background_normal', 4),
                          ('kinstone_background_normal', 14), ('credits_01', 4))}}
        self.menus = [dict(task=0, bldcnt=0x241, alpha=v) for v in (0x0909, 0x0d05)]
        self.menus += [dict(family='pieces', bldcnt=0xe46, alpha=v) for v in (0x1000, 0x020e)]
        self.fusion = [dict(family='fusion', bldcnt=0x3f90, fusion_brightness=v) for v in (0, 1, 14)]
        self.credits = [dict(task=4, bldcnt=0x2442, installed=1, alpha=v, fade_mask=0xffffffff)
                        for v in (0x0010, 0x1000)]

    def test_complete_evidence_does_not_invent_masked_credit_trace(self):
        result = check_traces(self.menus, self.fusion, self.credits, self.coverage)
        self.assertFalse(result['masked_credits_observed'])
        self.assertEqual(result['fusion_flash']['values'], [0, 1, 14])

    def test_missing_effect_phase_and_wrong_source_rejected(self):
        with self.assertRaises(ValueError):
            check_traces(self.menus[:1], self.fusion, self.credits, self.coverage)
        wrong = copy.deepcopy(self.coverage)
        wrong['images']['title_screen']['layer_mask'] = 4
        with self.assertRaisesRegex(ValueError, 'ownership'):
            check_traces(self.menus, self.fusion, self.credits, wrong)

    def test_figurine_flash_requires_live_field_and_actual_brightness(self):
        self.coverage['figurine_field'] = [{'layer_mask': 0x20}]
        frames = [dict(figurine_field=1, figurine_blend=0xbf, figurine_brightness=v) for v in (0, 8, 16)]
        result = check_traces(self.menus, self.fusion, self.credits, self.coverage, frames)
        self.assertEqual(result['figurine_prize_brightness']['values'], [0, 8, 16])
        with self.assertRaises(ValueError):
            check_traces(self.menus, self.fusion, self.credits, self.coverage, frames[:2])

    def test_gameover_masks_and_travel_screen_effects(self):
        self.coverage['images']['fast_travel_background'] = {'layer_mask': 8}
        gameover = [dict(task=3, fade_mask=m, bldcnt=0) for m in (0xffff0001, 0xffff, 0xffffffff)]
        travel = [dict(travel_field=1, bldcnt=0, alpha=0, fade_mask=0xffffffff, display_control=0x9f40),
                  dict(travel_field=1, bldcnt=0x3648, alpha=0x0f01, fade_mask=0xffffffff, display_control=0x1f40)]
        result = check_traces(self.menus, self.fusion, self.credits, self.coverage, None, gameover, travel)
        self.assertEqual(result['fast_travel']['excluded_replaced_frames'], 1)
        invalid = [dict(travel[0], bldcnt=0x3648)]
        with self.assertRaisesRegex(ValueError, 'unexpected effect'):
            check_traces(self.menus, self.fusion, self.credits, self.coverage, None, gameover, invalid)
        check_traces(self.menus, self.fusion, self.credits, self.coverage, None, gameover, travel)
        gameover[0]['fade_mask'] = 0xffff0000
        with self.assertRaises(ValueError):
            check_traces(self.menus, self.fusion, self.credits, self.coverage, None, gameover, travel)

    def test_prologue_requires_alpha_second_input_timeline(self):
        for i in range(1, 7):
            self.coverage['images'][f'prologue_{i:02d}'] = {'layer_mask': 4}
        frames = [dict(installed=1, bldcnt=0x2244, alpha=v) for v in (0x10, 0x1008)]
        result = check_traces(self.menus, self.fusion, self.credits, self.coverage, prologue=frames)
        self.assertEqual(result['prologue_alpha']['values'], [0x10, 0x1008])
        with self.assertRaises(ValueError):
            check_traces(self.menus, self.fusion, self.credits, self.coverage, prologue=frames[:1])


if __name__ == '__main__':
    unittest.main()
