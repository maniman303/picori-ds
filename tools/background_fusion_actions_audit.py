"""Native fusion input, ownership, effects and source-lifetime checks."""


def fusion_action_cases():
    labels = ('cancel_b', 'cancel_start', 'cancel_l', 'mismatch_cancel',
              'match_direct', 'match_world_event')
    return [dict(decoration=health, mode=mode, label=label, success=mode >= 4,
                 target=9 if mode == 4 else 10, piece=110 if mode == 5 else 109)
            for health in range(3) for mode, label in enumerate(labels)]


def validate_fusion_actions(rows, events, frames):
    cases = fusion_action_cases()
    captures = {r['frame']: r for r in frames}
    states = {r['frame']: r for r in rows if 'event' not in r}
    if [r['value'] for r in events if r['event'] == 'fusion_actions_complete'] != [18]:
        raise ValueError('incomplete fusion action matrix')
    expected = []
    for index, case in enumerate(cases):
        names = ['fusion_action_request', 'fusion_screen', 'fusion_action_ready',
                 'fusion_action_health_mutated', 'fusion_action_held', 'fusion_action_input']
        if case['mode'] == 3:
            names += ['fusion_attempt', 'fusion_mismatch', 'fusion_action_mismatch_returned',
                      'fusion_action_cancel_request', 'fusion_cancel', 'fusion_cancel_exit']
        elif case['success']:
            names += ['fusion_attempt', 'fusion_match', 'fusion_award', 'fusion_success_exit']
            if case['mode'] == 5:
                names += ['fusion_world_event_install', 'fusion_local_hint_install']
        else:
            names += ['fusion_cancel', 'fusion_cancel_exit']
        names += ['fusion_action_game_restored']
        expected += [(name, index) for name in names]
    names = {name for name, _ in expected}
    if [(r['event'], r.get('fusion_action_case')) for r in events if r['event'] in names] != expected:
        raise ValueError('incorrect native fusion lifecycle/order')
    for index, case in enumerate(cases):
        current = [r for r in events if r.get('fusion_action_case') == index]
        one = lambda name: next(r for r in current if r['event'] == name)
        mode = case['mode']
        health = (24, 15, 8)[case['decoration']]
        mutated = 24 if case['decoration'] == 2 else 8
        request = one('fusion_action_request')
        if (request['health'], request['max_health'], request['fusion_id'], request['fusion_count'],
            request['fusion_owned'], request['fusion_piece109'], request['fusion_piece110'], request['family']) != (
                health, 24, case['target'], 0, 0, 2, 2, 'unknown'):
            raise ValueError('incorrect fusion fixture')
        # The local hint reinstalls a decoration then overwrites it with a field.
        decorations = [r['value'] for r in current if r['event'] == 'menu_decoration']
        if decorations != [case['decoration']] + ([0 if mutated == 24 else 2] if mode == 5 else []):
            raise ValueError('fusion decoration reload/lifetime mismatch')
        phases = ['fusion_action_ready', 'fusion_action_held']
        if mode == 3:
            phases += ['fusion_action_mismatch_returned']
        for name in phases:
            event = one(name)
            capture = captures.get(event['frame'])
            if event['value'] != index or not capture:
                raise ValueError('missing neutral fusion capture')
            for sample in (event, capture):
                if (sample['fusion_task'], sample['fusion_substate'], sample['fusion_ui'], sample['fusion_next'],
                    sample['fusion_menu'], sample['fusion_column'], sample['fusion_cursor'], sample['fusion_fade'],
                    sample['family'], sample['decoration']) != (2, 7, 4, 2, 1, 2, 0, 0, 'fusion', case['decoration']):
                    raise ValueError('incorrect installed fusion context')
                if (sample['health'], sample['max_health']) != (health if name == 'fusion_action_ready' else mutated, 24):
                    raise ValueError('fusion installed-health mutation mismatch')
                if (sample['fusion_count'], sample['fusion_owned'], sample['fusion_piece109'], sample['fusion_piece110']) != (0, 0, 2, 2):
                    raise ValueError('fusion ownership changed before match')
                if (sample['heading_style'], sample['heading_x'], sample['heading_y'], sample['heading_transition']) != (1, 64, 16, 0):
                    raise ValueError('incorrect fusion heading')
        if one('fusion_action_health_mutated')['value'] != mutated:
            raise ValueError('incorrect fusion health mutation event')
        key = (2, 8, 512, 1, 1, 1)[mode]
        input_event = one('fusion_action_input')
        if input_event['value'] != key or states.get(input_event['frame'] + 1, {}).get('fusion_keys') != key:
            raise ValueError('missing native fusion input')
        choices = [(r['value'], r['fusion_keys']) for r in current if r['event'] == 'fusion_choice']
        if choices != ([(1, 128)] if mode == 5 else []):
            raise ValueError('incorrect fusion selection navigation')
        if mode < 4:
            cancel = one('fusion_cancel')
            if (cancel['value'], cancel['fusion_keys']) != ((2 if mode == 3 else key),) * 2:
                raise ValueError('incorrect fusion cancellation input')
            if one('fusion_cancel_exit')['value'] != 5:
                raise ValueError('incorrect native fusion refusal result')
        if mode >= 3:
            attempt = one('fusion_attempt')
            if (attempt['value'], attempt['fusion_selected'], attempt['fusion_keys']) != (case['piece'], case['piece'], 1):
                raise ValueError('incorrect attempted fusion piece/input')
        if mode == 3 and one('fusion_mismatch')['value'] != 109:
            raise ValueError('missing native mismatch')
        if case['success']:
            if one('fusion_match')['value'] != case['piece']:
                raise ValueError('incorrect native shape match')
            award = one('fusion_award')
            if (award['value'], award['fusion_count'], award['fusion_owned'], award['fusion_piece109'], award['fusion_piece110']) != (
                    case['target'], 1, 1, 1 if mode == 4 else 2, 1 if mode == 5 else 2):
                raise ValueError('incorrect native fusion award/consumption')
            if one('fusion_success_exit')['value'] != (8 if mode == 5 else 0):
                raise ValueError('incorrect fusion scene handoff')
            # The local matching flash increments brightness once per native
            # update from 0 to 14, then clears it as the award phase begins.
            match = one('fusion_match')
            interval = [r for r in rows if 'event' not in r and match['frame'] < r['frame'] <= match['frame'] + 16]
            if [r['fusion_brightness'] for r in interval] != list(range(15)) + [0]:
                raise ValueError('native fusion match brightness timeline skipped')
            if mode == 5:
                if one('fusion_world_event_install')['value'] != 20:
                    raise ValueError('incorrect native fusion world event')
                for name in ('fusion_world_event_install', 'fusion_local_hint_install'):
                    if one(name)['family'] != 'unknown':
                        raise ValueError('fusion artwork survived scene replacement')
        result = one('fusion_action_game_restored')
        capture = captures.get(result['frame'])
        if result['value'] != index or not capture:
            raise ValueError('missing fusion restoration capture')
        for sample in (result, capture):
            if (sample['fusion_task'], sample['fusion_substate'], sample['fusion_fade'], sample['family'],
                sample['world_area'], sample['world_room'], sample['fusion_count'], sample['fusion_owned'], sample['fusion_result'],
                sample['fusion_piece109'], sample['fusion_piece110']) != (
                    2, 2, 0, 'unknown', request['world_area'], request['world_room'], int(case['success']), int(case['success']),
                    6 if case['success'] else 5, 1 if mode == 4 else 2, 1 if mode == 5 else 2):
                raise ValueError('fusion ownership/room/source restoration mismatch')
    installed = 'unknown'
    for row in rows:
        if row.get('event') == 'fusion_screen':
            installed = 'fusion'
        elif row.get('event') in ('fusion_action_request', 'fusion_world_event_install', 'fusion_local_hint_install',
                                  'fusion_action_game_restored', 'fusion_actions_complete') or (
                'event' not in row and (row.get('fusion_task'), row.get('fusion_substate'), row.get('fusion_ui')) != (2, 7, 4)):
            installed = 'unknown'
        if row.get('family', 'unknown') != installed:
            raise ValueError('fusion source changed outside native scene boundary')


def fusion_reference_rows(rom, language):
    from export_screen_backgrounds import Rom, menu_recipes
    from PIL import Image
    result = []
    for name, scene, layers, backdrop in menu_recipes(Rom(rom, language)):
        if name.startswith('kinstone_background_'):
            image = Image.frombytes('RGB', (240, 160), scene.render(layers, backdrop))
            result.append(image.crop((0, 159, 240, 160)).tobytes())
    return result


def validate_fusion_action_pixels(directory, events, references):
    from PIL import Image
    # The fixed stone frame masks both lower corners. Its complete bottom
    # row is static and retains independently decoded decoration differences.
    xs = range(240)
    fields = [bytes(channel for x in xs for channel in ref[x * 3:x * 3 + 3]) for ref in references]
    if len(fields) != 3 or len(set(fields)) != 3:
        raise ValueError('fusion ROM references must distinguish health fields')
    checked = 0
    for event in events:
        if event['event'] not in ('fusion_action_ready', 'fusion_action_held', 'fusion_action_mismatch_returned'):
            continue
        with Image.open(directory / f"{event['frame']:06d}.png") as image:
            if image.size != (240, 160):
                raise ValueError('non-native fusion capture')
            rgb = image.convert('RGB')
            if bytes(channel for x in xs for channel in rgb.getpixel((x, 159))) != fields[event['decoration']]:
                raise ValueError('fusion pixels disagree with installed ROM health field')
        checked += 1
    if checked != 39:
        raise ValueError('incomplete fusion field checkpoints')
    return dict(checkpoints=checked, field_pixels_per_checkpoint=len(xs))
