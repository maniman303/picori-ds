#!/usr/bin/env python3
"""ROM-free failure/coverage checks for the native state-audit review tool."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from capture_background_audit import summarize, validate_trace


def frame(number, ui=0):
    return dict(frame=number, capture=True, task=1, state=1, ui=ui, menu=1,
                overlay=0, installed=-1, fade_active=0, fade_progress=0,
                fade_mask=0xffffffff)


class AuditCoverage(unittest.TestCase):
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
