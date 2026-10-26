"""Native figurine prize ownership, brightness controls and description input."""
from background_figurine_audit import FIELD_POINTS


def figurine_action_cases():
    cases = []
    for health in range(3):
        for post in range(2):
            figures = (47, 1, 68, 136 if post else 130)
            for capsule, index in enumerate(figures):
                for owned in range(2):
                    cases.append(dict(decoration=health, staffroll=post, index=index, capsule=capsule,
                                      initial_owned=owned, duplicate=(1 << (index % 8)) if owned else 0,
                                      collection=False))
            cases.append(dict(decoration=health, staffroll=post, index=figures[3], capsule=None,
                              initial_owned=1, duplicate=0, collection=True))
    return cases


def figurine_action_start(events):
    starts = [r['value'] for r in events if r['event'] == 'figurine_actions_start']
    if not starts:
        return 0  # Retained full-run schema-v2 traces predate range observation.
    if len(starts) != 1 or not 0 <= starts[0] < 54:
        raise ValueError('incorrect native figurine action range')
    return starts[0]


def validate_figurine_actions(rows, events, frames, *, start=None, end=54, require_complete=True):
    native_start = figurine_action_start(events)
    if start is None:
        start = native_start
    if not 0 <= start < end <= 54 or start != native_start:
        raise ValueError('incorrect figurine action validation range')
    all_cases = figurine_action_cases()
    cases = all_cases[start:end]
    by_frame = {row['frame']: row for row in frames}
    state_frames = {row['frame']: row for row in rows if 'event' not in row}
    neutral = ('figurine_action_ready', 'figurine_action_held', 'figurine_action_bottom', 'figurine_action_top')
    for phase in ('figurine_action_request', *neutral, 'figurine_action_restored'):
        if [r['value'] for r in events if r['event'] == phase] != list(range(start, end)):
            raise ValueError('incomplete or unordered figurine action cases')
    if require_complete and (end != 54 or [r['value'] for r in events if r['event'] == 'figurine_actions_complete'] != [54]):
        raise ValueError('incorrect figurine action completion count')
    if [r['value'] for r in events if r['event'] == 'menu_decoration'] != [c['decoration'] for c in cases]:
        raise ValueError('unexpected figurine action decoration reinstallation')
    if [r['value'] for r in events if r['event'] == 'pause_screen'] != [3] * len(cases):
        raise ValueError('incorrect figurine action layout installation')
    sequence = []
    for index, case in enumerate(cases, start):
        names = ['figurine_action_request', 'figurine_screen']
        if not case['collection']:
            names += ['figurine_capsule', 'figurine_capsule_open', 'figurine_award', 'figurine_reveal_done']
        names += ['figurine_action_ready', 'figurine_action_health_mutated', 'figurine_action_held',
                  'figurine_action_bottom', 'figurine_action_top', 'figurine_cancel', 'figurine_action_restored']
        sequence += [(name, index) for name in names]
    required = {name for name, _ in sequence}
    if [(r['event'], r.get('figurine_action_case')) for r in events if r['event'] in required] != sequence:
        raise ValueError('incorrect native figurine capsule/award/scroll/restore order')
    for index, case in enumerate(cases, start):
        current = [r for r in events if r.get('figurine_action_case') == index]
        one = lambda name: next(r for r in current if r['event'] == name)
        request = one('figurine_action_request')
        if (request.get('health'), request.get('max_health'), request.get('staffroll')) != ((24, 15, 8)[case['decoration']], 24, case['staffroll']):
            raise ValueError('incorrect figurine prize health/credits fixture')
        if one('figurine_screen')['value'] != (1 if case['collection'] else case['index']):
            raise ValueError('incorrect native prize/viewer installation')
        limit = one('figurine_action_ready')['detail_limit']
        if not 0 <= limit <= 120 or limit % 8 or case['index'] in (130, 136) and not limit:
            raise ValueError('missing or invalid scrollable figurine description')
        for phase in neutral:
            row = one(phase)
            capture = by_frame.get(row['frame'])
            if not capture or capture.get('fade_active') or (capture.get('task'), capture.get('state'), capture.get('substate'), capture.get('ui')) != (2, 2, 7, 7):
                raise ValueError('missing neutral figurine action capture')
            scroll = limit if phase == 'figurine_action_bottom' else 0
            for sample in (row, capture):
                if tuple(sample.get(k) for k in ('figurine_action_case', 'figurine_field', 'figurine_index', 'figurine_owned',
                          'staffroll', 'decoration', 'pause_screen', 'family', 'schema')) != (
                          index, 1, case['index'], 1, case['staffroll'], case['decoration'], 3, 'unknown', 2):
                    raise ValueError('wrong installed figurine action context/ownership')
                if tuple(sample.get(k) for k in ('figurine_menu', 'figurine_variant', 'figurine_duplicate', 'figurine_counter')) != (
                          1 if case['collection'] else 3, 255 if case['collection'] else case['index'], case['duplicate'], 0 if case['collection'] else 128):
                    raise ValueError('wrong native prize/duplicate completion state')
                if tuple(sample.get(k) for k in ('detail_scroll', 'detail_limit', 'figurine_bg1_y')) != (scroll, limit, scroll - 112):
                    raise ValueError('incorrect figurine description scroll/BG1 geometry')
                if (sample.get('figurine_blend'), sample.get('figurine_brightness')) != (0, 0):
                    raise ValueError('figurine reveal left brightness effect installed')
                if sample.get('palette_zero') is None or sample['palette_zero'] != sample.get('palette_251'):
                    raise ValueError('figurine prize lost live palette-zero source')
                if tuple(sample.get(k) for k in ('heading_style', 'heading_x', 'heading_y', 'heading_transition')) != (0, 0, 0, 0):
                    raise ValueError('figurine prize claimed a menu heading')
                health = (24, 15, 8)[case['decoration']] if phase == 'figurine_action_ready' else 24 if case['decoration'] == 2 else 8
                if (sample.get('health'), sample.get('max_health')) != (health, 24):
                    raise ValueError('incorrect figurine action health mutation')
        if not case['collection']:
            for name in ('figurine_capsule', 'figurine_capsule_open'):
                if one(name)['value'] != case['capsule']:
                    raise ValueError('incorrect native capsule type')
            for name, counter in (('figurine_award', 64), ('figurine_reveal_done', 128)):
                row = one(name)
                if (row['value'], row.get('figurine_duplicate'), row.get('figurine_counter'), row.get('figurine_owned')) != (case['duplicate'], case['duplicate'], counter, 1):
                    raise ValueError('incorrect native award/duplicate timing')
            if one('figurine_capsule_open').get('figurine_counter') != 0:
                raise ValueError('capsule animation did not start the native reveal timer')
        scrolls = [r for r in current if r['event'] == 'figurine_scroll']
        expected = [(16, n) for n in range(8, limit + 1, 8)] + [(16, limit)]
        expected += [(32, n) for n in range(limit - 8, -1, -8)] + [(32, 0)]
        if [(r['value'], r.get('detail_scroll')) for r in scrolls] != expected:
            raise ValueError('incorrect native scroll edges or boundary clamps')
        if any((r.get('figurine_input'), r.get('detail_limit'), r.get('figurine_bg1_y')) != (r['value'], limit, r['detail_scroll'] - 112) for r in scrolls):
            raise ValueError('native scroll event disagrees with input/BG1 geometry')
        choices = [r['value'] for r in current if r['event'] == 'figurine_choice']
        expected_choices = []
        if case['collection']:
            expected_choices = list(range(6, case['index'] + 1, 5))
            expected_choices += list(range(expected_choices[-1] + 1, case['index'] + 1))
        if choices != expected_choices:
            raise ValueError('incorrect native collection navigation')
        cancel = one('figurine_cancel')
        key = 8 if index % 2 else 2
        if case['collection']:
            # Collection first requests menu type 3. The following native exit
            # runs after the edge is released, so the cancel event has input 0.
            boundary = state_frames.get(cancel['frame'], {})
            if (boundary.get('figurine_input'), boundary.get('figurine_menu')) != (key, 3):
                raise ValueError('missing native collection exit input boundary')
        if (cancel['value'], cancel.get('figurine_input')) != (case['index'], 0 if case['collection'] else key):
            raise ValueError('incorrect native figurine B/START exit')
        restored = one('figurine_action_restored')
        capture = by_frame.get(restored['frame'])
        if not capture or capture.get('fade_active'):
            raise ValueError('missing completed figurine gameplay capture')
        for sample in (restored, capture):
            if tuple(sample.get(k) for k in ('figurine_action_case', 'figurine_field', 'figurine_owned', 'world_area', 'world_room')) != (
                          index, 0, 1, request['world_area'], request['world_room']):
                raise ValueError('figurine award/result capture did not survive native gameplay restoration')
        if (capture.get('task'), capture.get('state'), capture.get('substate')) != (2, 2, 2):
            raise ValueError('figurine did not restore native gameplay')
    installed = False
    awarded = False
    brightness_counters = {}
    for row in rows:
        name = row.get('event')
        if name == 'figurine_screen':
            installed, awarded = True, False
        elif name in ('figurine_action_request', 'figurine_action_restored', 'figurine_actions_complete'):
            installed = False
        elif name is None and (row.get('task'), row.get('substate'), row.get('ui')) != (2, 7, 7):
            installed = False
        if row.get('figurine_field', 0) != int(installed):
            raise ValueError('figurine action source changed outside installation/restoration')
        if not installed:
            continue
        case = all_cases[row['figurine_action_case']]
        if case['collection']:
            continue
        if name == 'figurine_award':
            awarded = True
        if row.get('figurine_owned') != (1 if awarded else case['initial_owned']):
            raise ValueError('prize ownership changed outside native award boundary')
        counter = row.get('figurine_counter', 0)
        if name is None and 0 < counter < 128:
            brightness = counter // 4
            if brightness > 16:
                brightness = 32 - brightness
            if (row.get('figurine_brightness'), row.get('figurine_blend')) != (brightness, 0xbf):
                raise ValueError('incorrect native prize brightness controls')
            brightness_counters.setdefault(row['figurine_action_case'], set()).add(counter)
    if brightness_counters != {i: set(range(1, 128)) for i, c in enumerate(cases, start) if not c['collection']}:
        raise ValueError('incomplete native prize brightness timeline')


def validate_figurine_action_pixels(directory, events, reference, *, start=None, end=54):
    from PIL import Image
    checked = 0
    if start is None:
        start = figurine_action_start(events)
    descriptions = {}
    description_mask = (16, 120, 224, 144)
    for row in events:
        if row['event'] not in ('figurine_action_ready', 'figurine_action_held', 'figurine_action_bottom', 'figurine_action_top'):
            continue
        if row['palette_zero'] != reference:
            raise ValueError('figurine prize source disagrees with native ROM palette')
        rgb = tuple(((reference >> shift) & 31) << 3 for shift in (0, 5, 10))
        with Image.open(directory / f"{row['frame']:06d}.png") as image:
            if any(image.convert('RGB').getpixel(point) != rgb for point in FIELD_POINTS):
                raise ValueError('figurine action pixels disagree with live field')
            description = image.convert('RGB').crop(description_mask).tobytes()
        index = row['figurine_action_case']
        if row['event'] == 'figurine_action_ready':
            descriptions[index] = description
        elif row['event'] == 'figurine_action_bottom' and row['detail_limit']:
            if description == descriptions[index]:
                raise ValueError('scrollable figurine description pixels did not move')
        elif description != descriptions[index]:
            raise ValueError('figurine description pixels did not return to their native top')
        checked += 1
    if checked != 4 * (end - start):
        raise ValueError('incomplete figurine action pixel checkpoints')
    return dict(checkpoints=checked, pixels_per_checkpoint=2, points=FIELD_POINTS,
                description_mask=description_mask, description_comparisons=checked - len(descriptions))
