"""Native dungeon-map coverage. Room loading establishes dungeon identity."""

# First field (numFloors), not highestFloor, from gDungeonFloorMetadatas.
DUNGEONS = ((1, 0x48, 3), (2, 0x50, 4), (3, 0x58, 3),
            (4, 0x60, 3), (5, 0x70, 5), (6, 0x88, 5))


def dungeon_cases():
    return [dict(dungeon=dungeon, area=area, opening=opening, decoration=opening // 2,
                 items=7 if opening % 2 else 0, floor=floor)
            for dungeon, area, floors in DUNGEONS for opening in range(6) for floor in range(floors)]


def validate_dungeons(rows, events, frames):
    cases = dungeon_cases()
    by_frame = {row['frame']: row for row in frames}
    for phase in ('dungeon_ready', 'dungeon_held'):
        checkpoints = [r for r in events if r['event'] == phase]
        if [r['value'] for r in checkpoints] != list(range(len(cases))):
            raise ValueError('incomplete or unordered dungeon checkpoints')
        for row, case in zip(checkpoints, cases):
            capture = by_frame.get(row['frame'])
            if not capture or capture.get('fade_active') or (capture.get('task'), capture.get('substate'), capture.get('ui')) != (2, 7, 1):
                raise ValueError('missing neutral dungeon capture')
            for sample in (row, capture):
                if tuple(sample.get(k) for k in ('dungeon_case', 'dungeon_index', 'dungeon_area', 'dungeon_items',
                          'dungeon_floor', 'decoration', 'pause_screen', 'family', 'schema')) != (
                          row['value'], case['dungeon'], case['area'], case['items'], case['floor'], case['decoration'], 5, 'titled', 2):
                    raise ValueError('wrong installed dungeon context/floor/items')
                if tuple(sample.get(k) for k in ('heading_style', 'heading_x', 'heading_y', 'heading_transition')) != (1, 64, 16, 0):
                    raise ValueError('incorrect dungeon heading')
                health = (24, 15, 8)[case['decoration']] if phase == 'dungeon_ready' and case['floor'] == 0 else (24 if case['decoration'] == 2 else 8)
                if (sample.get('health'), sample.get('max_health')) != (health, 24):
                    raise ValueError('incorrect dungeon health fixture')
    if [r['value'] for r in events if r['event'] == 'dungeons_complete'] != [len(cases)]:
        raise ValueError('incorrect dungeon completion count')
    for name in ('dungeon_room_request', 'dungeon_room_ready'):
        if [r['value'] for r in events if r['event'] == name] != list(range(1, 7)):
            raise ValueError('missing native dungeon room entry')
    sequence = []
    index = 0
    for dungeon, _, floors in DUNGEONS:
        sequence += [('dungeon_room_request', dungeon), ('dungeon_room_ready', dungeon)]
        for opening in range(6):
            sequence += [('dungeon_request', opening), ('dungeon_screen', dungeon)]
            for _ in range(floors):
                sequence += [('dungeon_ready', index), ('dungeon_health_mutated', 24 if opening // 2 == 2 else 8), ('dungeon_held', index)]
                index += 1
            sequence += [('dungeon_restored', opening)]
    names = {name for name, _ in sequence}
    if [(r['event'], r['value']) for r in events if r['event'] in names] != sequence:
        raise ValueError('incorrect dungeon room/install/checkpoint/restore order')
    if [r['value'] for r in events if r['event'] == 'pause_screen'] != [1, 5] * 36:
        raise ValueError('incorrect native dungeon layout installation')
    if [r['value'] for r in events if r['event'] == 'menu_decoration'] != [health for _ in DUNGEONS for health in (0, 0, 1, 1, 2, 2)]:
        raise ValueError('unexpected dungeon decoration reinstallation')
    installs = [i for i, r in enumerate(events) if r['event'] == 'dungeon_screen']
    restores = [i for i, r in enumerate(events) if r['event'] == 'dungeon_restored']
    for start, end in zip(installs, restores):
        install = events[start]
        floor = install['dungeon_floor']
        floors = DUNGEONS[install['dungeon_index'] - 1][2]
        if not 0 <= floor < floors:
            raise ValueError('invalid native initial dungeon floor')
        if [r['value'] for r in events[start:end] if r['event'] == 'dungeon_floor'] != list(range(floor - 1, -1, -1)) + list(range(1, floors)):
            raise ValueError('incorrect native dungeon floor navigation')
    installed = 'unknown'
    for row in rows:
        event = row.get('event')
        if event == 'pause_screen':
            installed = 'titled'
        elif event in ('dungeon_room_request', 'dungeon_room_ready', 'dungeon_request',
                       'dungeon_restored', 'dungeons_complete'):
            installed = 'unknown'
        elif event is None and (row.get('task'), row.get('substate'), row.get('ui')) != (2, 7, 1):
            installed = 'unknown'
        if row.get('family', 'unknown') != installed:
            raise ValueError('dungeon family changed outside native installation/restoration')
    for row in events:
        if row['event'] in ('dungeon_room_ready', 'dungeon_restored'):
            dungeon = row.get('dungeon_index')
            if dungeon not in range(1, 7) or row.get('dungeon_area') != DUNGEONS[dungeon - 1][1]:
                raise ValueError('dungeon identity was not established by native room load')


def validate_dungeon_pixels(directory, events, references):
    """Exposed bottom row is native installed health BG3; live map is excluded."""
    from PIL import Image
    if len(references) != 3 or len(set(references)) != 3:
        raise ValueError('dungeon references must distinguish health decorations')
    checked = 0
    for row in events:
        if row['event'] not in ('dungeon_ready', 'dungeon_held'):
            continue
        with Image.open(directory / f"{row['frame']:06d}.png") as image:
            if image.convert('RGB').crop((0, 159, 240, 160)).tobytes() != references[row['decoration']]:
                raise ValueError('dungeon pixels disagree with installed health decoration')
        checked += 1
    if checked != len(dungeon_cases()) * 2:
        raise ValueError('incomplete dungeon pixel checkpoints')
    return dict(checkpoints=checked, pixels_per_checkpoint=240, mask=[0, 159, 240, 160])
