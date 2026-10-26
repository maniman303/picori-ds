"""Native sleep choices, frozen game ticks, released keys and wake-up."""


def sleep_cases():
    modes = ('accept', 'right_left_accept', 'right_a_decline', 'b_b_decline', 'right_b_decline')
    return [dict(decoration=health, mode=mode, label=label, accept=mode < 2)
            for health in range(3) for mode, label in enumerate(modes)]


def validate_sleep(rows, events, frames):
    cases = sleep_cases()
    by_frame = {r['frame']: r for r in frames}
    state_frames = {r['frame']: r for r in rows if 'event' not in r}
    phases = ('sleep_quest_ready', 'sleep_ready', 'sleep_held', 'sleep_quest_returned')
    for name in ('sleep_case_request', 'sleep_prompt_request', *phases, 'sleep_exit_request', 'sleep_game_restored'):
        if [r['value'] for r in events if r['event'] == name] != list(range(15)):
            raise ValueError('incomplete or unordered sleep cases')
    if [r['value'] for r in events if r['event'] == 'sleep_complete'] != [15]:
        raise ValueError('incorrect sleep completion')
    if [r['value'] for r in events if r['event'] == 'menu_decoration'] != [c['decoration'] for c in cases]:
        raise ValueError('unexpected sleep decoration reinstallation')
    if [r['value'] for r in events if r['event'] == 'pause_screen'] != [1, 2, 9, 2] * 15:
        raise ValueError('incorrect native inventory/quest/sleep transition sequence')
    sequence = []
    for index, case in enumerate(cases):
        names = ['sleep_case_request', 'sleep_quest_ready', 'sleep_prompt_request', 'sleep_prompt',
                 'sleep_ready', 'sleep_health_mutated', 'sleep_held', 'sleep_answer']
        if case['accept']:
            names += ['sleep_armed', 'sleep_loop_enter', 'sleep_waiting', 'sleep_wake_request', 'sleep_loop_woke']
        names += ['sleep_quest_returned', 'sleep_exit_request', 'sleep_game_restored']
        sequence += [(name, index) for name in names]
    names = {n for n, _ in sequence}
    if [(r['event'], r.get('sleep_case')) for r in events if r['event'] in names] != sequence:
        raise ValueError('incorrect native sleep interaction order')
    for index, case in enumerate(cases):
        current = [r for r in events if r.get('sleep_case') == index]
        one = lambda name: next(r for r in current if r['event'] == name)
        request = one('sleep_case_request')
        if (request.get('health'), request.get('max_health')) != ((24, 15, 8)[case['decoration']], 24):
            raise ValueError('incorrect sleep health preload')
        reset = [8, 3, 1, 0] if index else []
        if [r['value'] for r in current if r['event'] == 'quest_choice'] != reset + [11, 5, 4]:
            raise ValueError('incorrect native quest navigation to sleep')
        choices = ([], [1, 0], [1], [1], [1])[case['mode']]
        if [r['value'] for r in current if r['event'] == 'sleep_choice'] != choices:
            raise ValueError('incorrect native sleep choice sequence')
        expected_keys = ([], [16, 32], [16], [2], [16])[case['mode']]
        if [r.get('sleep_new_keys') for r in current if r['event'] == 'sleep_choice'] != expected_keys:
            raise ValueError('incorrect native sleep cursor input')
        reset_keys = [32, 64, 64, 32] if index else []
        if [r.get('sleep_new_keys') for r in current if r['event'] == 'quest_choice'] != reset_keys + [32, 64, 32]:
            raise ValueError('incorrect native quest cursor input')
        answer = one('sleep_answer')
        key = 2 if case['mode'] >= 3 else 1
        if (answer['value'], answer.get('sleep_cursor'), answer.get('sleep_new_keys')) != (0 if case['accept'] else 1, 0 if case['accept'] else 1, key):
            raise ValueError('incorrect native sleep answer/input')
        for phase in phases:
            event = one(phase)
            capture = by_frame.get(event['frame'])
            if not capture or capture.get('fade_active') or (capture.get('task'), capture.get('state'), capture.get('substate'), capture.get('ui')) != (2, 2, 7, 1):
                raise ValueError('missing neutral sleep/quest capture')
            prompt = phase in ('sleep_ready', 'sleep_held')
            for sample in (event, capture):
                if tuple(sample.get(k) for k in ('sleep_case', 'schema', 'family', 'pause_screen', 'decoration', 'sleep_status', 'sleep_pause_phase', 'sleep_menu')) != (
                        index, 2, 'plain' if prompt else 'titled', 9 if prompt else 2, case['decoration'], 0, 2, 1):
                    raise ValueError('incorrect installed sleep/quest context')
                heading = (0, 0, 0, 0) if prompt else (1, 64, 16, 0)
                if tuple(sample.get(k) for k in ('heading_style', 'heading_x', 'heading_y', 'heading_transition')) != heading:
                    raise ValueError('incorrect sleep/quest heading')
                health = (24, 15, 8)[case['decoration']] if phase in ('sleep_ready', 'sleep_quest_ready') else 24 if case['decoration'] == 2 else 8
                if (sample.get('health'), sample.get('max_health')) != (health, 24):
                    raise ValueError('incorrect sleep live-health mutation')
                cursor = 0 if prompt or phase == 'sleep_quest_ready' else 4
                if sample.get('sleep_cursor') != cursor:
                    raise ValueError('incorrect native sleep/quest cursor')
        if one('sleep_health_mutated')['value'] != (24 if case['decoration'] == 2 else 8):
            raise ValueError('incorrect sleep health mutation event')
        if case['accept']:
            entered, woke, waiting = one('sleep_loop_enter'), one('sleep_loop_woke'), one('sleep_waiting')
            if one('sleep_armed')['value'] != 1 or entered['value'] != 1 or woke['value'] != 0:
                raise ValueError('incorrect native sleep/wake status events')
            begin, end = rows.index(entered), rows.index(woke)
            asleep = rows[begin:end + 1]
            if ({r['frame'] for r in asleep} != set(range(entered['frame'], woke['frame'] + 1)) or
                    woke['frame'] - entered['frame'] < 28 or len({r.get('sleep_ticks') for r in asleep}) != 1):
                raise ValueError('game ticks advanced or sleep interval was skipped')
            wake_request = one('sleep_wake_request')
            for sample in asleep:
                if sample['frame'] < wake_request['frame'] and sample.get('sleep_raw_keys') != 1023:
                    raise ValueError('sleep input was not released before the native wake request')
                if (sample.get('sleep_status'), sample.get('sleep_raw_display'), sample.get('family'), sample.get('decoration')) != (
                        0 if sample is woke else 1, 128, 'plain', case['decoration']):
                    raise ValueError('native sleep forced blank/source/status changed')
            if one('sleep_quest_returned').get('sleep_ticks', 0) <= woke['sleep_ticks']:
                raise ValueError('game ticks did not resume after native wake')
            if entered.get('sleep_raw_keys') != 1023 or waiting.get('sleep_raw_keys') != 1023 or woke.get('sleep_raw_keys') != 1022:
                raise ValueError('native sleep did not release keys and wake with raw A')
            capture = by_frame.get(waiting['frame'])
            if not capture or (capture.get('sleep_status'), capture.get('sleep_raw_display'), capture.get('sleep_raw_keys'), capture.get('sleep_ticks')) != (1, 128, 1023, entered['sleep_ticks']):
                raise ValueError('missing native sleeping framebuffer capture')
        exit_request = one('sleep_exit_request')
        if state_frames.get(exit_request['frame'] + 1, {}).get('sleep_new_keys') != 8:
            raise ValueError('missing native sleep-route START exit input')
        result = one('sleep_game_restored')
        capture = by_frame.get(result['frame'])
        if not capture or capture.get('fade_active') or (capture.get('task'), capture.get('state'), capture.get('substate'), capture.get('ui')) != (2, 2, 2, 0):
            raise ValueError('missing native sleep-route game restoration')
        for sample in (result, capture):
            if (sample.get('sleep_case'), sample.get('family'), sample.get('sleep_status'), sample.get('world_area'), sample.get('world_room')) != (
                    index, 'unknown', 0, request['world_area'], request['world_room']):
                raise ValueError('sleep-route exit lost case/original room/source')
    installed = 'unknown'
    for row in rows:
        if row.get('event') == 'pause_screen':
            installed = 'plain' if row['value'] == 9 else 'titled'
        elif row.get('event') in ('sleep_case_request', 'sleep_game_restored', 'sleep_complete') or (
                'event' not in row and (row.get('task'), row.get('substate'), row.get('ui')) != (2, 7, 1)):
            installed = 'unknown'
        if row.get('family', 'unknown') != installed:
            raise ValueError('sleep family changed outside native installation/restoration')


FIELD_X = tuple(range(8)) + tuple(range(232, 240))


def validate_sleep_pixels(directory, events, references):
    from PIL import Image
    checked, sleeping = 0, 0
    if len(references) != 3:
        raise ValueError('sleep ROM references must distinguish health decorations')
    fields = [b''.join(reference[x * 3:x * 3 + 3] for x in FIELD_X) for reference in references]
    if len(set(fields)) != 3:
        raise ValueError('sleep ROM references must distinguish health decorations')
    for event in events:
        if event['event'] not in ('sleep_quest_ready', 'sleep_ready', 'sleep_held', 'sleep_quest_returned', 'sleep_waiting'):
            continue
        with Image.open(directory / f"{event['frame']:06d}.png") as image:
            if image.size != (240, 160):
                raise ValueError('sleep capture has non-native dimensions')
            image = image.convert('RGB')
            if event['event'] == 'sleep_waiting':
                if image.tobytes() != bytes([255]) * (240 * 160 * 3):
                    raise ValueError('sleep framebuffer did not render native forced-blank white')
                sleeping += 1
            else:
                field = bytes(channel for x in FIELD_X for channel in image.getpixel((x, 159)))
                reference = fields[event['decoration']]
                if field != reference:
                    raise ValueError('sleep/quest pixels disagree with installed ROM health field')
                checked += 1
    if checked != 60 or sleeping != 6:
        raise ValueError('incomplete sleep pixel checkpoints')
    return dict(checkpoints=checked, field_pixels_per_checkpoint=len(FIELD_X), field_points=[(x, 159) for x in FIELD_X],
                sleeping_checkpoints=sleeping, forced_blank_pixels_per_checkpoint=240 * 160)
