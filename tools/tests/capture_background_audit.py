#!/usr/bin/env python3
"""ROM-free failure/coverage checks for the native state-audit review tool."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from capture_background_audit import menu_cases, summarize, validate_trace, validate_inventory_pixels


def frame(number, ui=0):
    return dict(frame=number, capture=True, task=1, state=1, ui=ui, menu=1,
                overlay=0, installed=-1, fade_active=0, fade_progress=0,
                fade_mask=0xffffffff)


def menu_trace():
    rows = []
    for health, maximum, decoration in ((24, 24, 0), (16, 24, 0), (15, 24, 1), (9, 24, 1),
                                       (8, 24, 2), (1, 24, 2), (8, 8, 2),
                                       (24, 24, 0), (15, 24, 1), (8, 24, 2)):
        rows.append(dict(event='menu_decoration', value=decoration, health=health, max_health=maximum))
    for index, case in enumerate(menu_cases()):
        for phase in ('menu_case_ready', 'menu_case_held'):
            number = len(rows)
            sample = dict(frame(number, 1), case=index, decoration=case['decoration'],
                          pause_screen=case['screen'], map_group=case['map_group'],
                          health=case.get('health', 24), max_health=case.get('max_health', 24))
            if phase.endswith('held'):
                sample['health'] = 24 if case['decoration'] == 2 else 8
                sample['max_health'] = 24
            event = dict(sample, event=phase, value=index)
            del event['capture']
            rows += [event, sample]
    rows.append(dict(event='menus_complete', value=106))
    return rows


class AuditCoverage(unittest.TestCase):
    def test_inventory_pixels_follow_installed_state_after_health_changes(self):
        references = [bytes([value] * 720) for value in (32, 96, 160)]
        # Damaged decoration stays installed even when live health is critical.
        event = dict(event='menu_case_held', value=2, frame=1, pause_screen=1, decoration=1, health=8)
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            Image.new('RGB', (240, 160), (96, 96, 96)).save(directory / '000001.png')
            result = validate_inventory_pixels(directory, [event], references)
            self.assertEqual(result['checkpoints'], 1)
            Image.new('RGB', (240, 160), (160, 160, 160)).save(directory / '000001.png')
            with self.assertRaisesRegex(ValueError, 'disagree with installed'):
                validate_inventory_pixels(directory, [event], references)

    def test_menu_matrix_requires_every_neutral_case_and_held_decoration(self):
        rows = menu_trace()
        validate_trace(rows, 'menus')
        held = next(row for row in rows if row.get('event') == 'menu_case_held')
        held['decoration'] = 2
        with self.assertRaisesRegex(ValueError, 'wrong installed decoration'):
            validate_trace(rows, 'menus')
        rows = [row for row in menu_trace() if not (row.get('event') == 'menu_case_ready' and row['value'] == 105)]
        with self.assertRaisesRegex(ValueError, 'incomplete or unordered'):
            validate_trace(rows, 'menus')

    def test_menu_matrix_rejects_reinstall_and_wrong_detail_recipe(self):
        rows = menu_trace()
        rows.insert(1, dict(rows[0]))
        with self.assertRaisesRegex(ValueError, 'unexpected reinstallation'):
            validate_trace(rows, 'menus')
        rows = menu_trace()
        capture = next(row for row in rows if 'event' not in row and row.get('map_group') == 0x74)
        capture['map_group'] = 0x73
        with self.assertRaisesRegex(ValueError, 'wrong native detail-map recipe'):
            validate_trace(rows, 'menus')

    def test_menu_matrix_rejects_missing_or_faded_checkpoint(self):
        rows = menu_trace()
        capture = next(row for row in rows if 'event' not in row)
        capture['capture'] = False
        with self.assertRaisesRegex(ValueError, 'missing neutral checkpoint'):
            validate_trace(rows, 'menus')
        capture['capture'] = True
        capture['fade_active'] = 1
        with self.assertRaisesRegex(ValueError, 'missing neutral checkpoint'):
            validate_trace(rows, 'menus')

    def test_text_commands_do_not_prove_retained_artwork(self):
        rows = [dict(event='credits_install', value=i, installed=i) for i in range(9)]
        rows += [dict(event='credits_clear', value=0, installed=-1),
                 dict(event='credits_save', value=0, installed=-1), frame(1)]
        with self.assertRaisesRegex(ValueError, 'retaining'):
            validate_trace(rows, 'credits')
        rows.append(dict(event='credits_command', value=2, installed=-1))
        with self.assertRaisesRegex(ValueError, 'retaining'):
            validate_trace(rows, 'credits')
        rows.append(dict(event='credits_command', value=2, installed=8))
        validate_trace(rows, 'credits')
        rows = [row for row in rows if row.get('event') != 'credits_clear']
        with self.assertRaisesRegex(ValueError, 'explicit credits clear'):
            validate_trace(rows, 'credits')

    def test_incomplete_opening_cannot_validate_six_page_retelling(self):
        rows = [dict(event='prologue_install', value=i, installed=i) for i in range(5)]
        rows += [dict(event='prologue_clear', value=0, installed=-1), frame(1)]
        with self.assertRaisesRegex(ValueError, 'installations'):
            validate_trace(rows, 'prologue')

    def test_file_operations_have_distinct_review_frames(self):
        rows = [dict(event='file_state', value=i, installed=-1) for i in (0, 1, 3, 4, 5, 6)]
        rows += [dict(event='files_complete', value=0, installed=-1), frame(1, 3), frame(2, 5)]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            (path / 'trace.jsonl').write_text('\n'.join(json.dumps(row) for row in rows))
            for number in (1, 2):
                Image.new('RGB', (240, 160)).save(path / f'{number:06d}.png')
            result = summarize(path, 'files')
            self.assertEqual(result['review_frames'], [1, 2])
            Image.new('RGB', (256, 192)).save(path / '000002.png')
            with self.assertRaisesRegex(ValueError, 'expected native'):
                summarize(path, 'files')

    def test_local_hint_border_is_not_its_foreground_label(self):
        rows = [dict(event='local_hint_ready', value=0, installed=-1), frame(1, 10)]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            (path / 'trace.jsonl').write_text('\n'.join(json.dumps(row) for row in rows))
            image = Image.new('RGB', (240, 160), (248, 208, 136))
            image.putpixel((120, 159), (248, 248, 248))
            image.save(path / '000001.png')
            summarize(path, 'local_hint')
            image.putpixel((0, 159), (0, 0, 0))
            image.save(path / '000001.png')
            with self.assertRaisesRegex(ValueError, 'solid field'):
                summarize(path, 'local_hint')


if __name__ == '__main__':
    unittest.main()
