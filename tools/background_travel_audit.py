"""ROM-free checks for native fast-travel background captures."""


def travel_cases():
    return [dict(label=f'{health}_travel_{view if view < 8 else "only_7"}',
                 decoration=decoration, destination=min(view, 7), unlocks=255 if view < 8 else 128,
                 health=(24, 15, 8)[decoration])
            for decoration, health in enumerate(('normal', 'damaged', 'critical'))
            for view in range(9)]


def validate_travel(rows, events, frames):
    cases = travel_cases()
    by_frame = {row['frame']: row for row in frames}
    if [row['value'] for row in events if row['event'] == 'travel_complete'] != [len(cases)]:
        raise ValueError('incorrect travel completion count')
    for phase in ('travel_case_request', 'travel_case_ready', 'travel_case_held', 'travel_case_restored'):
        if [row['value'] for row in events if row['event'] == phase] != list(range(len(cases))):
            raise ValueError(f'{phase}: incomplete or unordered travel cases')
    installs = [row for row in events if row['event'] == 'fast_travel_screen']
    decorations = [row for row in events if row['event'] == 'menu_decoration']
    screens = [row for row in events if row['event'] == 'pause_screen']
    if len(installs) != len(cases) or [row['value'] for row in decorations] != [c['decoration'] for c in cases]:
        raise ValueError('incorrect travel decoration installation or unexpected reinstallation')
    if [row['value'] for row in screens] != [13] * len(cases):
        raise ValueError('incorrect native travel layout installation')
    cancels = [row for row in events if row['event'] == 'fast_travel_cancel']
    if [row.get('travel_case') for row in cancels] != list(range(len(cases))):
        raise ValueError('missing native travel cancellation')
    sequence = ('travel_case_request', 'fast_travel_screen', 'travel_case_ready',
                'travel_health_mutated', 'travel_case_held', 'fast_travel_cancel', 'travel_case_restored')
    milestones = [row for row in events if row['event'] in sequence]
    if [(row['event'], row.get('travel_case')) for row in milestones] != [
            (event, index) for index in range(len(cases)) for event in sequence]:
        raise ValueError('incorrect travel installation/checkpoint/cancel order')
    for index, case in enumerate(cases):
        install = installs[index]
        if install.get('travel_case') != index or install['value'] != (0 if case['unlocks'] == 255 else 7):
            raise ValueError('incorrect initial unlocked destination')
        if (install['health'], install['max_health']) != (case['health'], 24):
            raise ValueError('incorrect travel opening health fixture')
        choices = [row['value'] for row in events if row['event'] == 'fast_travel_choice'
                   and row.get('travel_case') == index]
        if choices != (list(range(1, case['destination'] + 1)) if case['unlocks'] == 255 else []):
            raise ValueError('incorrect native travel cursor sequence')
        if cancels[index]['value'] != case['destination']:
            raise ValueError('cancelled wrong travel destination')
        for phase in ('travel_case_ready', 'travel_case_held'):
            row = next(row for row in events if row['event'] == phase and row['value'] == index)
            capture = by_frame.get(row['frame'])
            if not capture or capture.get('travel_case') != index or capture['fade_active']:
                raise ValueError('missing neutral travel checkpoint')
            for sample in (row, capture):
                if (sample.get('family'), sample.get('schema'), sample.get('decoration'),
                    sample.get('pause_screen'), sample.get('travel_destination'), sample.get('travel_unlocks')) != (
                        'unknown', 2, case['decoration'], 13, case['destination'], case['unlocks']):
                    raise ValueError('wrong installed travel context/decoration/destination')
                if tuple(sample.get(key) for key in ('heading_style', 'heading_x', 'heading_y', 'heading_transition')) != (0, 0, 0, 0):
                    raise ValueError('incorrect travel heading style/pose')
                if not sample.get('display_control', 0) & 0x800:
                    raise ValueError('travel lost its native BG3 decoration')
                expected_health = case['health'] if phase.endswith('ready') else 24 if case['decoration'] == 2 else 8
                if (sample.get('health'), sample.get('max_health')) != (expected_health, 24):
                    raise ValueError('travel health mutation fixture not exercised')
    installed = False
    for row in rows:
        event = row.get('event')
        if event == 'fast_travel_screen':
            installed = True
        elif event in ('travel_case_request', 'travel_case_restored', 'travel_complete'):
            installed = False
        elif event is None and (row.get('task'), row.get('substate'), row.get('ui')) != (2, 7, 9):
            installed = False
        if row.get('travel_field', 0) != int(installed):
            raise ValueError('travel field changed outside native installation/restoration')


def travel_reference_row(rom, language):
    """Independently decode the native BG3 after screen 13 replaces its map.
    All three preload paths must yield the same fixed field, not health art.
    """
    from PIL import Image
    from export_screen_backgrounds import Rom, Scene
    rom = Rom(rom, language)
    frames = []
    for health in range(3):
        scene = Scene(rom)
        for group in (0x56 + health, 93, 129):
            scene.gfx(group)
        for palette in (181, 185):
            scene.pal(palette)
        scene.upload(3, 0x1e0b)
        frames.append(scene.render([(3, 0x1e0b, 0, 0)], None))
    if len(set(frames)) != 1:
        raise ValueError('fast-travel BG3 did not replace all three health decorations')
    return Image.frombytes('RGB', (240, 160), frames[0]).crop((0, 159, 240, 160)).tobytes()


def validate_travel_pixels(directory, events, reference):
    from PIL import Image
    checked = 0
    for row in events:
        if row['event'] not in ('travel_case_ready', 'travel_case_held'):
            continue
        with Image.open(directory / f"{row['frame']:06d}.png") as image:
            actual = image.convert('RGB').crop((0, 159, 240, 160)).tobytes()
        if actual != reference:
            raise ValueError('travel pixels disagree with native fixed BG3 field')
        checked += 1
    if checked != 54:
        raise ValueError('incomplete travel pixel checkpoints')
    return dict(checkpoints=checked, pixels_per_checkpoint=240, mask=[0, 159, 240, 160])
