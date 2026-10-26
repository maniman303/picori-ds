"""World cursor/A entry and distinct native local-hint source variants."""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[1]


def hint_variants():
    def records(path, symbol):
        body = (ROOT / path).read_text().split(symbol + '[] = {', 1)[1].split('};', 1)[0]
        return [[x.strip() for x in match.split(',')] for match in re.findall(r'\{([^{}]+)\}', body)]
    kinstones = records('src/common.c', 'gKinstoneWorldEvents_eu')
    events = records('src/gameData.c', 'gWorldEvents_eu')
    locations = records('src/data/areaMetadata.c', 'gOverworldLocations')
    seen, result = set(), []
    for stone in range(10, 101):
        icon = int(kinstones[stone][7], 0)
        if not icon or icon > 8: continue
        event = events[int(kinstones[stone][4], 0)]
        x, y = int(event[8], 0) >> 4, int(event[9], 0) >> 4
        area = next((int(loc[4], 0) for loc in locations if int(loc[0], 0) <= x <= int(loc[2], 0) and int(loc[1], 0) <= y <= int(loc[3], 0)), 0)
        if (area, icon) not in seen:
            seen.add((area, icon)); result.append(dict(id=stone, area=area, icon=icon))
    return result


def validate_map_navigation(rows, events, frames):
    hints = hint_variants()
    captures = {r['frame']: r for r in frames}
    states = {r['frame']: r for r in rows if 'event' not in r}
    if [r['value'] for r in events if r['event'] == 'map_navigation_complete'] != [44 + len(hints)]:
        raise ValueError('incomplete native map navigation matrix')
    for name in ('nav_world_ready', 'nav_detail_ready', 'nav_world_returned'):
        if [r['value'] for r in events if r['event'] == name] != list(range(44)):
            raise ValueError('missing world/detail/return checkpoints')
    for index in range(44):
        current = [r for r in events if r.get('nav_case') == index]
        one = lambda name: next(r for r in current if r['event'] == name)
        recipe = index % 22
        target = recipe if recipe < 17 else 9 if recipe == 17 else 11 if recipe == 18 else 14
        group = 0x5f + recipe if recipe < 17 else 0x70 + recipe - 17
        enter = one('world_detail_enter')
        if (enter['value'], enter['nav_keys']) != (target, 1): raise ValueError('detail entry bypassed native cursor/A input')
        for name, key in (('nav_enter_input', 1), ('nav_back_input', 2)):
            r = one(name)
            if r['value'] != key or states.get(r['frame'] + 1, {}).get('nav_keys') != key:
                raise ValueError('missing real map key edge')
        if one('map_detail')['value'] != group: raise ValueError('wrong native map recipe')
        for name, screen in (('nav_world_ready', 4), ('nav_detail_ready', 6), ('nav_world_returned', 4)):
            r = one(name); capture = captures.get(r['frame'])
            if not capture: raise ValueError('missing neutral map capture')
            for sample in (r, capture):
                if (sample['nav_task'], sample['nav_substate'], sample['nav_ui'], sample['nav_fade'], sample['family'],
                    sample['pause_screen'], sample['nav_cursor'], sample['decoration']) != (2, 7, 1, 0, 'titled', screen, target, 0 if index < 22 else 2):
                    raise ValueError('wrong installed world/detail source or retained cursor')
                if sample['nav_markers'] > 128: raise ValueError('map marker storage overflow')
    for name in ('nav_open', 'nav_game_restored'):
        if [r['value'] for r in events if r['event'] == name] != [0, 1]: raise ValueError('missing native map opening/restoration')
    openings = [r for r in events if r['event'] == 'nav_open']
    restored = [r for r in events if r['event'] == 'nav_game_restored']
    for before, after in zip(openings, restored):
        if (after['nav_task'], after['nav_substate'], after['family'], after['world_area'], after['world_room']) != (2, 2, 'unknown', before['world_area'], before['world_room']):
            raise ValueError('map did not restore the original gameplay room/source')
    for index, hint in enumerate(hints, 44):
        current = [r for r in events if r.get('nav_case') == index]
        expected = ['nav_hint_request', 'fusion_local_hint_install', 'nav_hint_ready', 'nav_hint_held', 'nav_hint_exit_input', 'nav_hint_restored']
        if [r['event'] for r in current if r['event'] in expected] != expected: raise ValueError('incorrect local-hint lifecycle')
        for name in ('nav_hint_ready', 'nav_hint_held'):
            r = next(r for r in current if r['event'] == name); capture = captures.get(r['frame'])
            if r['value'] != hint['id'] or not capture: raise ValueError('missing distinct hint checkpoint')
            for sample in (r, capture):
                initial = (24, 15, 8)[index % 3]
                if sample['health'] != (initial if name == 'nav_hint_ready' else 8 if initial == 24 else 24):
                    raise ValueError('local hint did not retain source through health mutation')
                if (sample['nav_ui'], sample['nav_fade'], sample['family'], sample['heading_style'], sample['nav_cursor'], sample['nav_icon'], sample['nav_timer'], sample['palette_zero']) != (10, 0, 'unknown', 0, hint['area'], hint['icon'], 0, 0x475f):
                    raise ValueError('wrong native procedural hint source')
        exit_event = next(r for r in current if r['event'] == 'nav_hint_exit_input')
        if exit_event['value'] != (1, 2, 8)[index % 3] or states.get(exit_event['frame'] + 1, {}).get('nav_keys') != exit_event['value']:
            raise ValueError('missing native hint exit input')
        result = next(r for r in current if r['event'] == 'nav_hint_restored')
        request = next(r for r in current if r['event'] == 'nav_hint_request')
        if (result['nav_task'], result['nav_substate'], result['family'], result['world_area'], result['world_room']) != (2, 2, 'unknown', request['world_area'], request['world_room']): raise ValueError('hint did not restore gameplay source/room')
    # Hidden/visible marker fixtures share map artwork; at least one native
    # marker list must increase, rather than silently omitting both sets.
    detail = [r for r in events if r['event'] == 'nav_detail_ready']
    if not any(b['nav_markers'] > a['nav_markers'] for a, b in zip(detail[:22], detail[22:])):
        raise ValueError('visible marker fixture added no native markers')


def validate_navigation_pixels(directory, events, references):
    from PIL import Image
    xs = (*range(8), *range(232, 240))
    checked = hints = 0
    for r in events:
        if r['event'] not in ('nav_world_ready', 'nav_detail_ready', 'nav_world_returned', 'nav_hint_ready', 'nav_hint_held'): continue
        with Image.open(directory / f"{r['frame']:06d}.png") as image:
            if image.size != (240, 160): raise ValueError('non-native navigation image')
            image = image.convert('RGB')
            if r['event'].startswith('nav_hint'):
                expected = bytes((248, 208, 136)) * len(xs); hints += 1
            else:
                ref = references[r['decoration']]
                expected = bytes(c for x in xs for c in ref[x * 3:x * 3 + 3])
            if bytes(c for x in xs for c in image.getpixel((x, 159))) != expected: raise ValueError('navigation field pixels disagree with native source')
        checked += 1
    if checked != 132 + 2 * len(hint_variants()): raise ValueError('missing navigation pixel checks')
    return dict(checkpoints=checked, hint_checkpoints=hints, field_pixels_per_checkpoint=len(xs))
