#!/usr/bin/env python3
"""Failure-oriented checks for unfaded source and palette-bank ownership."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from background_effect_coverage import source_coverage


class Scene:
    row_controls = {}

    def sample(self, control, x, y):
        if control == 1:  # front BG1 has a transparent hole
            return 33 if x == 0 else None
        if control == 2:  # lower BG2 is opaque only in the middle
            return 129 if x == 1 else None
        return None


class CoverageTests(unittest.TestCase):
    def test_transparency_does_not_confuse_backdrop_with_bg(self):
        result = source_coverage(Scene(), [(2, 2, 0, 0), (1, 1, 0, 0)], 3, 1)
        self.assertEqual(result['sources'], [dict(layer='backdrop', palette_bank=0, pixels=1),
                                             dict(layer='bg1', palette_bank=2, pixels=1),
                                             dict(layer='bg2', palette_bank=8, pixels=1)])
        self.assertEqual(result['layer_mask'], 0x26)
        self.assertEqual(result['palette_bank_mask'], 0x105)
        self.assertEqual(result['edges'], dict(left=[0x12], right=[0x50],
                                               top=[0x12, 0x28, 0x50],
                                               bottom=[0x12, 0x28, 0x50]))

    def test_row_control_can_change_which_layer_owns_a_pixel(self):
        class Changing(Scene):
            row_controls = {1: [1, 3]}

            def sample(self, control, x, y):
                return 16 if control in (1, 3) else 32

        result = source_coverage(Changing(), [(1, 1, 0, 0), (2, 2, 0, 0)], 1, 2)
        self.assertEqual(result['sources'], [dict(layer='bg1', palette_bank=1, pixels=1),
                                             dict(layer='bg2', palette_bank=2, pixels=1)])

    def test_invalid_dimensions_fail(self):
        with self.assertRaises(ValueError):
            source_coverage(Scene(), [], 0, 1)


if __name__ == '__main__':
    unittest.main()
