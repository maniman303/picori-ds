"""Native Game Over save/retry and Continue/Quit lifecycle checks."""
from pathlib import Path


def validate_gameover(rows, events, frames):
    captures = {r['frame']: r for r in frames}
    states = {r['frame']: r for r in rows if 'event' not in r}
    if [r['value'] for r in events if r['event'] == 'gameover_complete'] != [5]:
        raise ValueError('incomplete gameover matrix')
    if any(r.get('family', 'unknown') != 'unknown' or r.get('heading_style', 0) for r in rows):
        raise ValueError('gameover selected menu artwork/heading')
    if [r['value'] for r in events if r['event'] == 'menu_decoration'] != [2] * 5:
        raise ValueError('unexpected gameover decoration installation')
    lifecycle = []
    for case in range(5):
        names = ['gameover_death_request', 'gameover_install', 'gameover_save_prompt', 'gameover_save_ready',
                 'gameover_save_answer']
        if case == 3:
            names += ['gameover_write_fault', 'gameover_save_error', 'gameover_error_ready',
                      'gameover_error_unchanged', 'gameover_dismiss_request', 'gameover_save_prompt',
                      'gameover_retry_ready', 'gameover_save_answer']
        if case in (0, 1, 3):
            names += ['gameover_save_success']
        names += ['gameover_continue_prompt', 'gameover_readback', 'gameover_continue_ready',
                  'gameover_continue_answer', 'gameover_quit' if case == 4 else 'gameover_continue',
                  'gameover_title_returned' if case == 4 else 'gameover_game_returned']
        lifecycle += [(name, case) for name in names]
    lifecycle += [('gameover_complete', 4)]
    names = {name for name, _ in lifecycle}
    if [(r['event'], r.get('gameover_case')) for r in events if r['event'] in names] != lifecycle:
        raise ValueError('incorrect native gameover lifecycle')
    for case in range(5):
        current = [r for r in events if r.get('gameover_case') == case]
        one = lambda name: next(r for r in current if r['event'] == name)
        request = one('gameover_death_request')
        if (request['gameover_task'], request['gameover_state'], request['gameover_substate'], request['health']) != (2, 2, 2, 0):
            raise ValueError('gameover did not start with gameplay death')
        installed = one('gameover_install')
        death = [r for r in rows if request['frame'] < r['frame'] < installed['frame']]
        if not any(r.get('death_flag') == 1 for r in death):
            raise ValueError('native death/room exit missing')
        for name in ('gameover_install', 'gameover_save_prompt', 'gameover_continue_prompt'):
            if one(name)['value'] != 0:
                raise ValueError('incorrect native gameover installation')
        choices = [(r['value'], r.get('gameover_keys')) for r in current if r['event'] == 'gameover_save_choice']
        if choices != ([(1, 128), (0, 64)] if case == 1 else [(1, 128)] if case in (2, 4) else []):
            raise ValueError('incorrect gameover save navigation')
        for answer in (r for r in current if r['event'] == 'gameover_save_answer'):
            if (answer['value'], answer['gameover_cursor'], answer['gameover_keys']) != (1 if case in (2, 4) else 0,) * 2 + (1,):
                raise ValueError('incorrect gameover save answer')
        choices = [(r['value'], r.get('gameover_keys')) for r in current if r['event'] == 'gameover_continue_choice']
        if choices != ([(1, 128), (0, 64)] if case == 2 else [(1, 128)] if case == 4 else []):
            raise ValueError('incorrect gameover continue navigation')
        answer = one('gameover_continue_answer')
        if (answer['value'], answer['gameover_cursor'], answer['gameover_keys']) != (int(case == 4), int(case == 4), 1):
            raise ValueError('incorrect gameover continue answer')
        if one('gameover_readback')['value'] != (24 if case in (0, 1, 3) else 0):
            raise ValueError('gameover checksum readback mismatch')
        if case in (0, 1, 3) and one('gameover_save_success')['value'] != 1:
            raise ValueError('incorrect gameover save success')
        phases = ['gameover_save_ready', 'gameover_continue_ready']
        if case == 3:
            phases += ['gameover_error_ready', 'gameover_retry_ready']
            error = one('gameover_save_error')
            if error['value'] != 0 or one('gameover_write_fault')['value'] != 1 or one('gameover_error_unchanged')['value'] != 1:
                raise ValueError('incorrect gameover write failure/readback')
            interval = [r for r in rows if 'event' not in r and error['frame'] < r['frame'] <= error['frame'] + 61]
            if [r.get('gameover_timer') for r in interval] != list(range(60, -1, -1)):
                raise ValueError('native gameover error wait skipped')
            dismissal = one('gameover_dismiss_request')
            if states.get(dismissal['frame'] + 1, {}).get('gameover_keys') != 1:
                raise ValueError('missing gameover error dismissal input')
        for name in phases:
            event = one(name)
            capture = captures.get(event['frame'])
            if event['value'] != case or not capture:
                raise ValueError('missing neutral gameover capture')
            for sample in (event, capture):
                if sample['display_control'] & 0x800 or sample['palette_zero'] != 0:
                    raise ValueError('gameover black field register mismatch')
                if (sample['gameover_task'], sample['gameover_state'], sample['gameover_menu'], sample['gameover_timer'],
                    sample['gameover_cursor'], sample['gameover_fade_active'], sample['health'], sample['gameover_y']) != (
                        3, 3 if name == 'gameover_continue_ready' else 2,
                        3 if name == 'gameover_error_ready' else 1, 0, 0, 0, 24, 37):
                    raise ValueError('incorrect neutral gameover state')
        result = one('gameover_title_returned' if case == 4 else 'gameover_game_returned')
        capture = captures.get(result['frame'])
        if result['value'] != case or not capture:
            raise ValueError('missing gameover restoration capture')
        for sample in (result, capture):
            if sample['gameover_task'] != (0 if case == 4 else 2) or sample['gameover_fade_active']:
                raise ValueError('incorrect gameover restoration task')
            if case != 4 and (sample['gameover_state'], sample['gameover_substate'], sample['health'],
                              sample['player_action'], sample['player_control'],
                              sample['world_area'], sample['world_room']) != (
                                  2, 2, 24, 1, 0, request['world_area'], request['world_room']):
                raise ValueError('gameover did not restore player/room')


def validate_gameover_pixels(directory, events):
    from PIL import Image
    checked = 0
    for event in events:
        if event['event'] not in ('gameover_save_ready', 'gameover_continue_ready',
                                  'gameover_error_ready', 'gameover_retry_ready'):
            continue
        with Image.open(Path(directory) / f"{event['frame']:06d}.png") as image:
            if image.size != (240, 160):
                raise ValueError('non-native gameover capture')
            # Native letters/dialog occupy the center. Test every outer column
            # plus the top/bottom eight rows, independently of palette metadata.
            rgb = image.convert('RGB')
            if not any(rgb.crop((32, 32, 216, 144)).tobytes()):
                raise ValueError('gameover native foreground is missing')
            for box in ((0, 0, 240, 8), (0, 152, 240, 160), (0, 8, 8, 152), (232, 8, 240, 152)):
                if any(rgb.crop(box).tobytes()):
                    raise ValueError('gameover generated-black field is not black')
        checked += 1
    if checked != 12:
        raise ValueError('incomplete gameover pixel checkpoints')
    return dict(checkpoints=checked, black_pixels_per_checkpoint=6144)
