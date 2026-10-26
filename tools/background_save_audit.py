"""Native inventory/quest save choices, checksum readback and EEPROM failure."""


def save_cases():
    labels = ('accept', 'down_up_accept', 'down_a_decline', 'b_b_decline',
              'down_b_decline', 'write_error')
    return [dict(decoration=health, mode=mode, label=label, accept=mode in (0, 1, 5),
                 error=mode == 5) for health in range(3) for mode, label in enumerate(labels)]


def validate_save(rows, events, frames, inventory=False):
    ready = "save_inventory_ready" if inventory else "save_quest_ready"
    returned = "save_inventory_returned" if inventory else "save_quest_returned"
    page, prompt_screen = (1, 11) if inventory else (2, 10)
    cases = save_cases()
    captures = {r['frame']: r for r in frames}
    states = {r['frame']: r for r in rows if 'event' not in r}
    if [r['value'] for r in events if r['event'] == 'save_complete'] != [18]:
        raise ValueError('incorrect save completion')
    if [r['value'] for r in events if r['event'] == 'menu_decoration'] != [c['decoration'] for c in cases]:
        raise ValueError('save decoration reinstalled or missing')
    if [r['value'] for r in events if r['event'] == 'pause_screen'] != ([1, 11, 1] if inventory else [1, 2, 10, 2]) * 18:
        raise ValueError('incorrect native save screen sequence')
    expected = []
    for index, case in enumerate(cases):
        names = ['save_case_request', ready, 'save_prompt_request', 'save_prompt',
                 'save_ready', 'save_health_mutated', 'save_held', 'save_answer']
        if case['accept']:
            names += (['save_write_fault', 'save_error'] if case['error'] else ['save_success'])
            names += ['save_result_ready', 'save_readback']
            if case['error']:
                names += ['save_dismiss_request']
        else:
            names += ['save_decline_unchanged']
        names += [returned, 'save_exit_request', 'save_game_restored']
        expected += [(name, index) for name in names]
    names = {name for name, _ in expected}
    if [(r['event'], r.get('save_case')) for r in events if r['event'] in names] != expected:
        raise ValueError('incorrect native save lifecycle/order')
    for index, case in enumerate(cases):
        current = [r for r in events if r.get('save_case') == index]
        one = lambda name: next(r for r in current if r['event'] == name)
        for name in ('save_case_request', ready, 'save_prompt_request',
                     'save_ready', 'save_held', returned, 'save_exit_request', 'save_game_restored'):
            if one(name)['value'] != index:
                raise ValueError('incorrect save case event value')
        if case['accept']:
            for name in ('save_result_ready', 'save_dismiss_request') if case['error'] else ('save_result_ready',):
                if one(name)['value'] != index:
                    raise ValueError('incorrect save result case value')
        elif one('save_decline_unchanged')['value'] != index:
            raise ValueError('incorrect declined save case value')
        if one('save_prompt')['value'] != 0:
            raise ValueError('incorrect initial native save cursor')
        request = one('save_case_request')
        if (request.get('health'), request.get('max_health')) != ((24, 15, 8)[case['decoration']], 24):
            raise ValueError('incorrect save health preload')
        reset = ([12, 8, 4, 0] if inventory else [4, 8, 3, 1, 0]) if index else []
        reset_keys = ([16, 64, 64, 64] if inventory else [32, 32, 64, 64, 32]) if index else []
        navigation = [r for r in current if r['event'] == ('inventory_choice' if inventory else 'quest_choice')]
        if [r['value'] for r in navigation] != reset + ([12, 16] if inventory else [11, 5]):
            raise ValueError('incorrect save-entry navigation to save')
        if [r.get('save_new_keys') for r in navigation] != reset_keys + ([64, 32] if inventory else [32, 64]):
            raise ValueError('incorrect save-entry navigation input')
        choices = ([], [1, 0], [1], [1], [1], [])[case['mode']]
        keys = ([], [128, 64], [128], [2], [128], [])[case['mode']]
        if [(r['value'], r.get('save_new_keys')) for r in current if r['event'] == 'save_choice'] != list(zip(choices, keys)):
            raise ValueError('incorrect save choices/input')
        answer = one('save_answer')
        if (answer['value'], answer.get('save_cursor'), answer.get('save_new_keys')) != (
                0 if case['accept'] else 1, 0 if case['accept'] else 1,
                2 if case['mode'] in (3, 4) else 1):
            raise ValueError('incorrect save answer/input')
        phases = [ready, 'save_ready', 'save_held', returned]
        if case['accept']:
            phases += ['save_result_ready']
        for phase in phases:
            event = one(phase)
            capture = captures.get(event['frame'])
            if not capture or capture.get('fade_active') or tuple(capture.get(k) for k in ('task', 'state', 'substate', 'ui')) != (2, 2, 7, 1):
                raise ValueError('missing neutral save/quest capture')
            prompt = phase in ('save_ready', 'save_held') or (phase == 'save_result_ready' and case['error'])
            for sample in (event, capture):
                if tuple(sample.get(k) for k in ('save_case', 'schema', 'family', 'pause_screen', 'decoration', 'save_menu', 'save_pause_phase')) != (
                        index, 2, 'plain' if prompt else 'titled', prompt_screen if prompt else page,
                        case['decoration'], 3 if phase == 'save_result_ready' and case['error'] else 1, 2):
                    raise ValueError('incorrect installed save context')
                if tuple(sample.get(k) for k in ('heading_style', 'heading_x', 'heading_y', 'heading_transition')) != ((0, 0, 0, 0) if prompt else (1, 64, 16, 0)):
                    raise ValueError('incorrect save heading')
                health = (24, 15, 8)[case['decoration']] if phase in ('save_ready', ready) else 24 if case['decoration'] == 2 else 8
                if (sample.get('health'), sample.get('max_health')) != (health, 24):
                    raise ValueError('incorrect save health mutation')
                cursor = (0 if phase == ready else (16 if inventory else 5)) if not prompt else 0
                if sample.get('save_cursor') != cursor:
                    raise ValueError('incorrect save/quest cursor')
        mutated = 24 if case['decoration'] == 2 else 8
        if one('save_health_mutated')['value'] != mutated:
            raise ValueError('incorrect save mutation event')
        if case['accept']:
            if one('save_readback')['value'] != (0 if case['error'] else mutated):
                raise ValueError('save checksum readback mismatch')
            if case['error'] and one('save_result_ready').get('save_timer') != 0:
                raise ValueError('save result timer not completed')
            if case['error']:
                error = one('save_error')
                if error['value'] != 0 or one('save_write_fault')['value'] != 1:
                    raise ValueError('missing native EEPROM failure')
                # Trace boundaries lead native work by one tick: the first
                # following sample observes the newly installed 60-tick wait.
                interval = [r for r in rows if 'event' not in r and error['frame'] < r['frame'] <= error['frame'] + 61]
                if len(interval) != 61 or [r.get('save_timer') for r in interval] != list(range(60, -1, -1)):
                    raise ValueError('native save error wait skipped')
            elif one('save_success')['value'] != 1:
                raise ValueError('incorrect native save success')
            if case['error']:
                dismissed = one('save_dismiss_request')
                if states.get(dismissed['frame'] + 1, {}).get('save_new_keys') != 1:
                    raise ValueError('missing native save dismissal input')
        exit_request = one('save_exit_request')
        if states.get(exit_request['frame'] + 1, {}).get('save_new_keys') != 8:
            raise ValueError('missing native save START exit')
        result = one('save_game_restored')
        capture = captures.get(result['frame'])
        if not capture or capture.get('fade_active') or tuple(capture.get(k) for k in ('task', 'state', 'substate', 'ui')) != (2, 2, 2, 0):
            raise ValueError('missing save game restoration')
        for sample in (result, capture):
            if (sample.get('save_case'), sample.get('family'), sample.get('world_area'), sample.get('world_room')) != (index, 'unknown', request['world_area'], request['world_room']):
                raise ValueError('save exit lost original room/source')
    installed = 'unknown'
    for row in rows:
        if row.get('event') == 'pause_screen':
            installed = 'plain' if row['value'] in (10, 11) else 'titled'
        elif row.get('event') in ('save_case_request', 'save_game_restored', 'save_complete') or (
                'event' not in row and (row.get('task'), row.get('substate'), row.get('ui')) != (2, 7, 1)):
            installed = 'unknown'
        if row.get('family', 'unknown') != installed:
            raise ValueError('save source changed outside native installation/restoration')


def validate_save_pixels(directory, events, references, inventory=False):
    ready = "save_inventory_ready" if inventory else "save_quest_ready"
    returned = "save_inventory_returned" if inventory else "save_quest_returned"
    from PIL import Image
    from background_sleep_audit import FIELD_X
    fields = [bytes(channel for x in FIELD_X for channel in ref[x * 3:x * 3 + 3]) for ref in references]
    if len(fields) != 3 or len(set(fields)) != 3:
        raise ValueError('save references must distinguish exposed health fields')
    checked = 0
    for event in events:
        if event['event'] not in (ready, 'save_ready', 'save_held', 'save_result_ready', returned):
            continue
        with Image.open(directory / f"{event['frame']:06d}.png") as image:
            if image.size != (240, 160):
                raise ValueError('non-native save capture')
            image = image.convert('RGB')
            actual = bytes(channel for x in FIELD_X for channel in image.getpixel((x, 159)))
            if actual != fields[event['decoration']]:
                raise ValueError('save pixels disagree with installed ROM health field')
        checked += 1
    if checked != 81:
        raise ValueError('incomplete save pixel checkpoints')
    return dict(checkpoints=checked, field_pixels_per_checkpoint=len(FIELD_X))
