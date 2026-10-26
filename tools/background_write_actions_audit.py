"""Native file commits and post-credits Save source/lifecycle evidence."""
from pathlib import Path


def _base(rows, frames):
    if any(r.get('family', 'unknown') != 'unknown' or r.get('heading_style', 0) for r in rows):
        raise ValueError('write route selected a pause family/heading')
    return {r['frame']: r for r in frames}, {r['frame']: r for r in rows if 'event' not in r}


def validate_ending_actions(rows, events, frames):
    captures, states = _base(rows, frames)
    if [r['value'] for r in events if r['event'] == 'ending_complete'] != [4]:
        raise ValueError('incomplete ending matrix')
    expected = []
    for case in range(4):
        names = ['ending_request', 'credits_save', 'ending_ready', 'ending_answer']
        if case >= 2:
            names += ['writes_write_fault', 'ending_error', 'ending_error_ready', 'ending_error_unchanged',
                      'ending_dismiss_input', 'ending_retry_ready', 'ending_answer']
        if case in (0, 2): names += ['ending_success']
        names += ['ending_departure', 'ending_readback', 'ending_title_returned']
        expected += [(name, case) for name in names]
    expected += [('ending_complete', 3)]
    names = {name for name, _ in expected}
    if [(r['event'], r.get('ending_case')) for r in events if r['event'] in names] != expected:
        raise ValueError('incorrect ending lifecycle/order')
    for case in range(4):
        current = [r for r in events if r.get('ending_case') == case]
        one = lambda name: next(r for r in current if r['event'] == name)
        if (one('ending_request')['health'], one('ending_request')['writes_clear'], one('ending_request')['writes_staffroll']) != ((24, 15, 8, 15)[case], 1, 1):
            raise ValueError('incorrect cleared-game ending fixture')
        answers = [(r['value'], r['writes_keys']) for r in current if r['event'] == 'ending_answer']
        if answers != ([(1, 1)] if case == 1 else [(0, 1), (int(case == 3), 1)] if case >= 2 else [(0, 1)]):
            raise ValueError('incorrect ending answers/input')
        choices = [(r['value'], r['writes_keys']) for r in current if r['event'] == 'ending_choice']
        if choices != ([(1, 128)] if case in (1, 3) else []):
            raise ValueError('incorrect ending cursor navigation')
        inputs = [r['value'] for r in current if r['event'] == 'ending_input']
        if inputs != ([128, 1] if case == 1 else [1]):
            raise ValueError('incorrect ending input route')
        if [r['value'] for r in current if r['event'] == 'ending_retry_input'] != ([128, 1] if case == 3 else [1] if case == 2 else []):
            raise ValueError('incorrect ending retry/refusal input')
        for event in current:
            if event['event'] in ('ending_input', 'ending_retry_input', 'ending_dismiss_input'):
                if states.get(event['frame'] + 1, {}).get('writes_keys') != event['value']:
                    raise ValueError('missing real ending key edge')
        phases = ['ending_ready']
        if case >= 2:
            phases += ['ending_error_ready', 'ending_retry_ready']
            error = one('ending_error')
            interval = [r for r in rows if 'event' not in r and error['frame'] < r['frame'] <= error['frame'] + 61]
            if [r['writes_timer'] for r in interval] != list(range(60, -1, -1)):
                raise ValueError('ending error wait skipped')
            if one('ending_error_unchanged')['value'] != 1 or one('writes_write_fault')['value'] != 1:
                raise ValueError('ending failure did not preserve prior valid save')
            if one('ending_dismiss_input')['value'] != (2 if case == 2 else 8):
                raise ValueError('incorrect ending error dismissal')
        for name in phases:
            event = one(name)
            capture = captures.get(event['frame'])
            if event['value'] != case or not capture: raise ValueError('missing ending neutral capture')
            for sample in (event, capture):
                if (sample['writes_task'], sample['writes_state'], sample['writes_overlay'], sample['writes_timer'],
                    sample['writes_fade']) != (4, 2, 3 if name == 'ending_error_ready' else 1, 0, 0):
                    raise ValueError('incorrect neutral ending state')
                if sample['installed'] != -1 or sample['palette_zero'] != 0 or sample['display_control'] & 0x800:
                    raise ValueError('ending retained an illustration or nonblack field')
        if one('ending_readback')['value'] != (24 if case == 0 else 8 if case == 2 else 0):
            raise ValueError('ending checksum/record readback mismatch')
        if case in (0, 2) and one('ending_success')['value'] != 1:
            raise ValueError('incorrect native ending save result')
        result = one('ending_title_returned')
        capture = captures.get(result['frame'])
        if result['value'] != case or not capture or any((s['writes_task'], s['writes_fade']) != (0, 0) for s in (result, capture)):
            raise ValueError('ending did not return to native title')


def validate_file_writes(rows, events, frames, extended=False):
    count = 9 if extended else 7
    checked_delete = any(r.get("writes_delete_checked") == 1 for r in rows)
    captures, states = _base(rows, frames)
    if [r['value'] for r in events if r['event'] == 'file_writes_complete'] != [count]:
        raise ValueError('incomplete file commit matrix')
    for case in range(count):
        current = [r for r in events if r.get('file_write_case') == case]
        one = lambda name: next(r for r in current if r['event'] == name)
        create, copy, error = case in (0, 4), case in (1, 5), 4 <= case <= 7
        sequence = ['file_write_request', 'file_write_root_ready', 'file_write_entry_ready']
        if not create: sequence += ['file_write_operation_ready']
        if error: sequence += ['file_host_fault'] if case == 7 else ['writes_write_fault']
        if case != 2: sequence += ['file_new_result' if create else 'file_copy_result' if copy else 'file_erase_result']
        if error and (create or copy or checked_delete): sequence += (['file_host_disk_unchanged', 'file_host_recovered'] if case == 7 else []) + ['file_write_error_ready', 'file_write_dismiss_input']
        sequence += ['file_write_readback', 'file_write_returned']
        lifecycle_names = {'file_write_request', 'file_write_root_ready', 'file_write_entry_ready', 'file_write_operation_ready',
                           'writes_write_fault', 'file_new_result', 'file_copy_result', 'file_erase_result',
                           'file_write_error_ready', 'file_write_dismiss_input', 'file_write_readback', 'file_write_returned', 'file_host_fault', 'file_host_recovered', 'file_host_disk_unchanged'}
        if [r['event'] for r in current if r['event'] in lifecycle_names] != sequence:
            raise ValueError('incorrect file commit lifecycle/order')
        for event in current:
            if event['event'] in ('file_write_input', 'file_write_dismiss_input'):
                if states.get(event['frame'] + 1, {}).get('writes_keys') != event['value']:
                    raise ValueError('missing native file operation key edge')
        # Entry selection uses real row navigation, then native view operations.
        input_keys = [r['value'] for r in current if r['event'] == 'file_write_input']
        # Row-reset prefix depends on the preceding native return selection.
        row = one('file_write_request')['writes_row']
        target = 1 if create else 0
        prefix = [64 if row > target else 128] * abs(row - target)
        expected = prefix + ([1, 1, 8, 8] if create else [1, 16, 1, 64, 64, 1] if copy else
                             [1, 16, 16, 1] + ([16, 1] if case != 2 else [1]))
        if input_keys != expected: raise ValueError('incorrect native file navigation/confirmation')
        if create:
            chars = [r['value'] for r in current if r['event'] == 'file_name_char']
            if len(chars) != 1 or not chars[0]: raise ValueError('missing native name entry')
        if error and one('file_host_fault' if case == 7 else 'writes_write_fault')['value'] != 1: raise ValueError('missing file device write fault')
        if case != 2:
            result = one('file_new_result' if create else 'file_copy_result' if copy else 'file_erase_result')
            if result['value'] != (0xffffffff if error and (create or copy or checked_delete) else 1):
                raise ValueError('incorrect native file save result')
        if case == 7 and (one('file_host_disk_unchanged')['value'], one('file_host_recovered')['value']) != (1, 1):
            raise ValueError('host failure changed durable data or did not recover')
        phases = ['file_write_root_ready', 'file_write_entry_ready', 'file_write_returned']
        if not create: phases += ['file_write_operation_ready']
        if error and (create or copy or checked_delete):
            phases += ['file_write_error_ready']
            result = one('file_new_result' if create else 'file_copy_result' if copy else 'file_erase_result')
            interval = [r for r in rows if 'event' not in r and result['frame'] < r['frame'] <= result['frame'] + 31]
            if [r['writes_timer'] for r in interval] != list(range(30, -1, -1)):
                raise ValueError('native file error wait skipped')
        for name in phases:
            event = one(name)
            capture = captures.get(event['frame'])
            if event['value'] != case or not capture: raise ValueError('missing native file neutral capture')
            expected_ui = (1 if create else 4) if name == 'file_write_entry_ready' else (5 if copy else 6) if name == 'file_write_operation_ready' else (1 if create else 5 if copy else 6) if name == 'file_write_error_ready' else 0
            for sample in (event, capture):
                if (sample['writes_task'], sample['writes_state'], sample['writes_ui'], sample['writes_fade']) != (1, 1, expected_ui, 0):
                    raise ValueError('incorrect file screen restoration/context')
        status = (1 if create or copy else 0) if not error and case != 2 else (0 if create or copy else 1)
        if one('file_write_readback')['value'] != status: raise ValueError('file checksum/unchanged-record readback mismatch')
        for sample in (one('file_write_returned'), captures[one('file_write_returned')['frame']]):
            if tuple(sample[k] for k in ('writes_slot0', 'writes_slot1', 'writes_slot2')) != (0 if case == 3 or case == 8 or (case == 6 and not checked_delete) else 1, 1 if case in (0, 1) else 0, 0):
                raise ValueError('incorrect native file-list result (including failed erase cache)')
    if [r['value'] for r in events if r['event'] == 'file_write_request'] != list(range(count)):
        raise ValueError('incorrect file case order')


def validate_ending_pixels(directory, events):
    from PIL import Image
    checked = 0
    for event in events:
        if event['event'] not in ('ending_ready', 'ending_error_ready', 'ending_retry_ready'): continue
        with Image.open(Path(directory) / f"{event['frame']:06d}.png") as image:
            if image.size != (240, 160): raise ValueError('non-native ending capture')
            rgb = image.convert('RGB')
            if not any(rgb.crop((24, 16, 216, 144)).tobytes()): raise ValueError('missing native ending foreground')
            for box in ((0, 0, 240, 8), (0, 152, 240, 160), (0, 8, 8, 152), (232, 8, 240, 152)):
                if any(rgb.crop(box).tobytes()): raise ValueError('ending black field mismatch')
        checked += 1
    if checked != 8: raise ValueError('incomplete ending pixel checkpoints')
    return dict(checkpoints=checked, black_pixels_per_checkpoint=6144)


def file_reference(rom, language):
    from export_screen_backgrounds import Rom, recipes
    source = Rom(rom, language)
    for name, scene, layers, backdrop in recipes(source):
        if name != 'file_select': continue
        # File-screen palettes 11..14 cycle through eight ROM offsets and
        # reverse direction. Require one complete legal palette frame per
        # checkpoint; never select colors independently at individual points.
        references = []
        for offset in range(0, 0x100, 0x20):
            for bank in range(11, 15):
                address = source.blob + (0x11a60 if source.region == 'EU' else 0x11aa0) + (bank - 11) * 0x100 + offset
                scene.palette[bank * 32:(bank + 1) * 32] = source.read(address, 32)
            references.append(scene.render(layers, backdrop))
        return references
    raise ValueError('missing file-select ROM reference')


def validate_file_pixels(directory, events, reference, extended=False, checked_delete=False):
    from PIL import Image
    # The top edge contains the native heading OBJ; the exposed bottom
    # corners retain BG3 across file, keyboard and dialog screens.
    points = [(x, 159) for x in (*range(8), *range(232, 240))]
    if isinstance(reference, bytes): reference = [reference]
    expected = [bytes(channel for x, y in points for channel in ref[(y * 240 + x) * 3:(y * 240 + x) * 3 + 3]) for ref in reference]
    checked = 0
    for event in events:
        if event['event'] not in ('file_write_root_ready', 'file_write_entry_ready', 'file_write_operation_ready', 'file_write_error_ready', 'file_write_returned'): continue
        with Image.open(Path(directory) / f"{event['frame']:06d}.png") as image:
            if image.size != (240, 160): raise ValueError('non-native file capture')
            rgb = image.convert('RGB')
            actual = bytes(channel for x, y in points for channel in rgb.getpixel((x, y)))
            if actual not in expected: raise ValueError('file field differs from ROM reference')
        checked += 1
    if checked != (38 if extended else 29 if checked_delete else 28): raise ValueError('incomplete file field checkpoints')
    return dict(checkpoints=checked, field_pixels_per_checkpoint=len(points))
