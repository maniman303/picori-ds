"""Native EU fast-travel enquiry, fade, scene handoff and arrival evidence."""

# Independent expectations from the engine's native destination definitions.
# area, room, spawn x, spawn y, layer (not final player animation coordinates).
DESTINATIONS = ((6, 2, 0x98, 0xf8, 1), (10, 0, 0xf8, 0xf8, 1),
                (8, 0, 0x1e8, 0x1a8, 1), (2, 0, 0x278, 0x58, 1),
                (11, 0, 0xa8, 0x1b8, 1), (4, 0, 0x228, 0x398, 1),
                (3, 1, 0x2c8, 0x128, 1), (0, 0, 0x128, 0x2a8, 1))


def travel_action_cases():
    return [dict(decoration=health, destination=min(view, 7), unlocks=128 if view == 8 else 255,
                 accept=accept, health=(24, 15, 8)[health])
            for health in range(3) for view in range(9) for accept in range(2)]


def validate_travel_actions(rows, events, frames):
    cases = travel_action_cases()
    by_frame = {r['frame']: r for r in frames}
    for phase in ('travel_action_request', 'travel_action_ready', 'travel_action_held', 'travel_action_prompt_ready'):
        if [r['value'] for r in events if r['event'] == phase] != list(range(54)):
            raise ValueError('incomplete or unordered travel action cases')
    if [r['value'] for r in events if r['event'] == 'travel_actions_complete'] != [54]:
        raise ValueError('incorrect travel action completion count')
    if [r['value'] for r in events if r['event'] == 'menu_decoration'] != [c['decoration'] for c in cases]:
        raise ValueError('unexpected travel action decoration reinstallation')
    if [r['value'] for r in events if r['event'] == 'pause_screen'] != [13] * 54:
        raise ValueError('incorrect travel action native layout')
    sequence = []
    for index, case in enumerate(cases):
        names = ['travel_action_request', 'fast_travel_screen', 'travel_action_ready',
                 'travel_action_health_mutated', 'travel_action_held', 'fast_travel_prompt',
                 'travel_action_prompt_ready', 'fast_travel_answer']
        names += (['fast_travel_fade', 'fast_travel_depart', 'fast_travel_handoff', 'travel_action_arrived']
                  if case['accept'] else ['fast_travel_cancel', 'travel_action_restored'])
        sequence += [(name, index) for name in names]
    required = {name for name, _ in sequence}
    if [(r['event'], r.get('travel_action_case')) for r in events if r['event'] in required] != sequence:
        raise ValueError('incorrect native travel enquiry/answer/handoff/arrival order')
    for index, case in enumerate(cases):
        current = [r for r in events if r.get('travel_action_case') == index]
        one = lambda name: next(r for r in current if r['event'] == name)
        request = one('travel_action_request')
        install = one('fast_travel_screen')
        if (request.get('travel_story_progress'), request.get('travel_ocarina'), request.get('travel_story_flags')) != (5, 1, 31):
            raise ValueError('incorrect ocarina-era story fixture')
        if request.get('travel_first_warp') != (0 if index < 2 else 1):
            raise ValueError('incorrect first/repeated native travel fixture')
        if install['value'] != (7 if case['unlocks'] == 128 else 0) or install['health'] != case['health']:
            raise ValueError('incorrect travel action initial destination/health')
        choices = [r['value'] for r in current if r['event'] == 'fast_travel_choice']
        if choices != (list(range(1, case['destination'] + 1)) if case['unlocks'] == 255 else []):
            raise ValueError('incorrect native travel action navigation')
        for phase in ('travel_action_ready', 'travel_action_held', 'travel_action_prompt_ready'):
            row = one(phase)
            capture = by_frame.get(row['frame'])
            if not capture or capture.get('fade_active') or (capture.get('task'), capture.get('state'), capture.get('substate'), capture.get('ui')) != (2, 2, 7, 9):
                raise ValueError('missing neutral travel action capture')
            for sample in (row, capture):
                if tuple(sample.get(k) for k in ('travel_action_case', 'travel_field', 'travel_destination', 'travel_unlocks',
                          'decoration', 'pause_screen', 'family', 'schema')) != (
                          index, 1, case['destination'], case['unlocks'], case['decoration'], 13, 'unknown', 2):
                    raise ValueError('wrong installed travel action context')
                if (sample.get('travel_story_progress'), sample.get('travel_ocarina'), sample.get('travel_story_flags')) != (5, 1, 31):
                    raise ValueError('travel lost ocarina-era story fixture')
                if tuple(sample.get(k) for k in ('heading_style', 'heading_x', 'heading_y', 'heading_transition')) != (0, 0, 0, 0):
                    raise ValueError('unexpected travel action heading')
                if not sample.get('display_control', 0) & 0x800:
                    raise ValueError('travel action lost fixed BG3 field')
                health = case['health'] if phase == 'travel_action_ready' else 24 if case['decoration'] == 2 else 8
                if (sample.get('health'), sample.get('max_health')) != (health, 24):
                    raise ValueError('incorrect travel action health mutation')
                if phase == 'travel_action_prompt_ready':
                    if sample.get('travel_phase') != 2 or sample.get('travel_choice') != (0 if case['accept'] else 1) or not sample.get('travel_message', 0) & 0x7f:
                        raise ValueError('missing native enquiry/selected choice')
        answer = one('fast_travel_answer')
        if answer['value'] != (0 if case['accept'] else 1) or answer.get('travel_message', 0) & 0x7f:
            raise ValueError('incorrect native travel answer')
        if case['accept']:
            fade = one('fast_travel_fade')
            if (fade.get('travel_fade_active'), fade.get('travel_fade_type'), fade.get('travel_phase')) != (1, 5, 4):
                raise ValueError('missing native outgoing travel fade')
            depart = one('fast_travel_depart')
            if depart.get('travel_fade_active') or depart.get('travel_field') != 1:
                raise ValueError('travel departed before fade completion or lost outgoing field')
            handoff = one('fast_travel_handoff')
            if tuple(handoff.get(k) for k in ('target_area', 'target_room', 'target_x', 'target_y', 'target_layer')) != DESTINATIONS[case['destination']]:
                raise ValueError('incorrect native travel destination handoff')
            if (handoff.get('travel_game_state'), handoff.get('travel_subtask'), handoff.get('travel_field')) != (1, 0, 0):
                raise ValueError('travel handoff did not replace the installed field')
            result = one('travel_action_arrived')
            expected_room = DESTINATIONS[case['destination']][:2]
        else:
            result = one('travel_action_restored')
            expected_room = (request['world_area'], request['world_room'])
        if (result.get('world_area'), result.get('world_room')) != expected_room:
            raise ValueError('incorrect travel arrival/restoration room')
        if tuple(result.get(k) for k in ('travel_game_state', 'travel_subtask', 'travel_field', 'player_action', 'player_control')) != (2, 2, 0, 1, 0):
            raise ValueError('travel did not restore normal gameplay control')
        if result.get('travel_first_warp') != (0 if index == 0 else 1):
            raise ValueError('native first-travel tutorial flag was not retained')
        capture = by_frame.get(result['frame'])
        if not capture or capture.get('fade_active') or capture.get('travel_field') != 0:
            raise ValueError('missing completed travel gameplay capture')
        if tuple(capture.get(k) for k in ('travel_action_case', 'world_area', 'world_room', 'player_action', 'player_control')) != (index, *expected_room, 1, 0):
            raise ValueError('completed travel capture disagrees with native gameplay event')
    installed = False
    for row in rows:
        event = row.get('event')
        if event == 'fast_travel_screen':
            installed = True
        elif event in ('travel_action_request', 'fast_travel_handoff', 'travel_action_arrived', 'travel_action_restored', 'travel_actions_complete'):
            installed = False
        elif event is None and (row.get('task'), row.get('substate'), row.get('ui')) != (2, 7, 9):
            installed = False
        if row.get('travel_field', 0) != int(installed):
            raise ValueError('travel action field changed outside installation/replacement')


def travel_action_reference(path, language):
    from export_screen_backgrounds import Rom, Scene
    values = []
    for health in range(3):
        scene = Scene(Rom(path, language))
        for group in (0x56 + health, 93, 129):
            scene.gfx(group)
        for group in (181, 185):
            scene.pal(group)
        for bg, control in ((1, 0x1c03), (2, 0x1d0b), (3, 0x1e0b)):
            scene.upload(bg, control)
        points = ((0, 0), (239, 0))
        if any(scene.sample(control, x, y - 4) is not None
               for control in (0x1c03, 0x1d0b) for x, y in points):
            raise ValueError('travel enquiry mask covered by native map/frame')
        pixels = scene.render([(3, 0x1e0b, 0, 0)], None)
        values.append([tuple(pixels[(y * 240 + x) * 3:(y * 240 + x) * 3 + 3]) for x, y in points])
    if any(value != values[0] for value in values):
        raise ValueError('travel action field depends on health preload')
    return values[0]


def validate_travel_action_pixels(directory, events, reference):
    from PIL import Image
    checked = 0
    for row in events:
        if row['event'] not in ('travel_action_ready', 'travel_action_held', 'travel_action_prompt_ready'):
            continue
        with Image.open(directory / f"{row['frame']:06d}.png") as image:
            if [image.convert('RGB').getpixel(point) for point in ((0, 0), (239, 0))] != reference:
                raise ValueError('travel action pixels disagree with native fixed BG3')
        checked += 1
    if checked != 162:
        raise ValueError('incomplete travel action pixel checkpoints')
    return dict(checkpoints=checked, pixels_per_checkpoint=2, points=[[0, 0], [239, 0]])
