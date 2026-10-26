"""Native detail-map scrolling and installed decoration retention."""

# Native table gUnk_08128E94, independently checked against the selected ROM.
LIMITS = (88, 0, 48, 0, 56, 48, 56, 0, 24, 48, 0, 96, 48, 56, 56, 80, 56)
Y_ORIGINS = (4, 0, 4, 0, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4)


def detail_map_cases():
    recipes = [(area, 0x5f + area) for area in range(17)]
    recipes += [(9, 0x70), (11, 0x71), (14, 0x72), (14, 0x73), (14, 0x74)]
    return [dict(decoration=health, recipe=recipe, area=area, map_group=group,
                 limit=LIMITS[area], origin=Y_ORIGINS[area])
            for health in range(3) for recipe, (area, group) in enumerate(recipes)]


def validate_detail_maps(rows, events, frames):
    cases = detail_map_cases()
    by_frame = {r['frame']: r for r in frames}
    state_frames = {r['frame']: r for r in rows if 'event' not in r}
    phases = ('detail_ready', 'detail_held', 'detail_top', 'detail_bottom', 'detail_top_returned')
    for name in ('detail_case_request', *phases, 'detail_back_request', 'detail_map_returned'):
        if [r['value'] for r in events if r['event'] == name] != list(range(66)):
            raise ValueError('incomplete or unordered detail-map cases')
    if [r['value'] for r in events if r['event'] == 'detail_maps_complete'] != [66]:
        raise ValueError('incorrect detail-map completion')
    for name in ('detail_open_request', 'detail_exit_request', 'detail_game_restored', 'menu_decoration'):
        if [r['value'] for r in events if r['event'] == name] != [0, 1, 2]:
            raise ValueError('incorrect detail-map opening/decoration/restoration count')
    expected_screens = [screen for _ in range(3) for screen in [1] + [s for _ in range(22) for s in (6, 4)]]
    if [r['value'] for r in events if r['event'] == 'pause_screen'] != expected_screens:
        raise ValueError('incorrect native detail-map screen transitions')
    if [r['value'] for r in events if r['event'] == 'map_detail'] != [c['map_group'] for c in cases]:
        raise ValueError('incorrect native detail-map graphics recipe')
    sequence = []
    for health in range(3):
        sequence.append(('detail_open_request', health))
        for index in range(health * 22, (health + 1) * 22):
            sequence += [(name, index) for name in ('detail_case_request', *phases, 'detail_back_request', 'detail_map_returned')]
        sequence += [('detail_exit_request', health), ('detail_game_restored', health)]
    names = {n for n, _ in sequence}
    if [(r['event'], r['value']) for r in events if r['event'] in names] != sequence:
        raise ValueError('incorrect detail-map interaction order')
    for index, case in enumerate(cases):
        current = [r for r in events if r.get('detail_map_case') == index]
        one = lambda name: next(r for r in current if r['event'] == name)
        for phase in phases:
            event = one(phase)
            capture = by_frame.get(event['frame'])
            if not capture or capture.get('fade_active') or (capture.get('task'), capture.get('state'), capture.get('substate'), capture.get('ui')) != (2, 2, 7, 1):
                raise ValueError('missing neutral detail-map capture')
            offset = case['limit'] // 2 if phase in ('detail_ready', 'detail_held') else case['limit'] if phase == 'detail_bottom' else 0
            for sample in (event, capture):
                if tuple(sample.get(k) for k in ('detail_map_case', 'schema', 'family', 'decoration', 'pause_screen', 'map_group',
                          'detail_map_area', 'detail_map_limit', 'detail_map_scroll', 'detail_pause_phase', 'detail_has_map')) != (
                          index, 2, 'titled', case['decoration'], 6, case['map_group'], case['area'], case['limit'], offset, 2, 1):
                    raise ValueError('wrong installed detail-map context/scroll')
                if (sample.get('detail_bg1_y'), sample.get('detail_bg2_y')) != (-case['origin'], offset - case['origin']):
                    raise ValueError('incorrect native detail-map BG geometry')
                if tuple(sample.get(k) for k in ('heading_style', 'heading_x', 'heading_y', 'heading_transition')) != (1, 64, 16, 0):
                    raise ValueError('incorrect detail-map heading')
                health = (24, 15, 8)[case['decoration']] if phase == 'detail_ready' else 24 if case['decoration'] == 2 else 8
                if (sample.get('health'), sample.get('max_health')) != (health, 24):
                    raise ValueError('incorrect detail-map health mutation')
                if (bool(sample.get('detail_house')), bool(sample.get('detail_fused'))) != (case['recipe'] == 17, case['recipe'] == 18):
                    raise ValueError('incorrect detail-map flag fixture')
                if case['recipe'] >= 19 and (sample.get('detail_snapshot_area'), sample.get('detail_snapshot_room')) != (8, case['recipe'] - 19):
                    raise ValueError('incorrect detail-map interior fixture')
        mutation = one('detail_health_mutated')
        if mutation['value'] != (24 if case['decoration'] == 2 else 8):
            raise ValueError('incorrect detail-map health mutation event')
        back = one('detail_back_request')
        if state_frames.get(back['frame'] + 1, {}).get('detail_input') != 2:
            raise ValueError('missing native detail-map B input')
        # Each stage holds two extra ticks at the bound. Offset changes are the
        # native held-key cadence (2 px/tick), including immovable maps.
        expected = [(64, n) for n in range(case['limit'] // 2 - 2, -1, -2)] + [(64, 0)] * 2
        expected += [(128, n) for n in range(2, case['limit'] + 1, 2)] + [(128, case['limit'])] * 2
        expected += [(64, n) for n in range(case['limit'] - 2, -1, -2)] + [(64, 0)] * 2
        scrolls = [r for r in current if r['event'] == 'detail_scroll']
        if [(r['value'], r.get('detail_map_scroll')) for r in scrolls] != expected:
            raise ValueError('incorrect native detail-map held input/cadence/clamps')
        checkpoints = ['detail_held', 'detail_top', 'detail_bottom', 'detail_top_returned']
        for left, right, direction in zip(checkpoints, checkpoints[1:], (64, 128, 64)):
            begin = current.index(one(left))
            finish = current.index(one(right))
            segment = [r for r in current[begin + 1:finish] if r['event'] == 'detail_scroll']
            if not segment or any(r['value'] != direction for r in segment):
                raise ValueError('detail-map held input crossed a checkpoint boundary')
        for sample in scrolls:
            if (sample.get('detail_input'), sample.get('detail_bg1_y'), sample.get('detail_bg2_y'), sample.get('detail_map_limit')) != (
                    sample['value'], -case['origin'], sample['detail_map_scroll'] - case['origin'], case['limit']):
                raise ValueError('detail-map scroll event disagrees with native geometry/input')
        for phase, screen in (('detail_map_returned', 4),):
            event = one(phase)
            capture = by_frame.get(event['frame'])
            if not capture or capture.get('fade_active'):
                raise ValueError('missing native detail-map back capture')
            for sample in (event, capture):
                if (sample.get('detail_map_case'), sample.get('pause_screen'), sample.get('family'), sample.get('decoration')) != (index, screen, 'titled', case['decoration']):
                    raise ValueError('detail-map B did not restore native world map')
    for health in range(3):
        request = next(r for r in events if r['event'] == 'detail_open_request' and r['value'] == health)
        result = next(r for r in events if r['event'] == 'detail_game_restored' and r['value'] == health)
        exit_request = next(r for r in events if r['event'] == 'detail_exit_request' and r['value'] == health)
        if state_frames.get(exit_request['frame'] + 1, {}).get('detail_input') != 8:
            raise ValueError('missing native detail-map START input')
        capture = by_frame.get(result['frame'])
        if not capture or capture.get('fade_active') or (capture.get('task'), capture.get('state'), capture.get('substate'), capture.get('ui')) != (2, 2, 2, 0):
            raise ValueError('missing native detail-map game restoration')
        for sample in (result, capture):
            if sample.get('family') != 'unknown' or (sample.get('world_area'), sample.get('world_room')) != (request['world_area'], request['world_room']):
                raise ValueError('detail-map exit changed the original game room/source')
    installed = 'unknown'
    for row in rows:
        if row.get('event') == 'pause_screen':
            installed = 'titled'
        elif row.get('event') in ('detail_open_request', 'detail_game_restored', 'detail_maps_complete') or (
                'event' not in row and (row.get('task'), row.get('substate'), row.get('ui')) != (2, 7, 1)):
            installed = 'unknown'
        if row.get('family', 'unknown') != installed:
            raise ValueError('detail-map family changed outside native installation/restoration')


FIELD_X = tuple(range(8)) + tuple(range(232, 240))
MAP_POINTS = tuple((x, y) for y in (40, 64, 88, 112) for x in (48, 80, 160, 192))


def validate_detail_map_pixel_samples(directory, events, references, map_reference):
    """Compare native field/map samples; retain an explicit text/arrow-free crop."""
    from PIL import Image
    checked, samples, crops = 0, 0, {}
    for event in events:
        if event['event'] not in ('detail_ready', 'detail_held', 'detail_top', 'detail_bottom', 'detail_top_returned'):
            continue
        index = event['value']
        with Image.open(directory / f"{event['frame']:06d}.png") as image:
            if image.size != (240, 160):
                raise ValueError('detail-map capture has non-native dimensions')
            image = image.convert('RGB')
            field = bytes(channel for x in FIELD_X for channel in image.getpixel((x, 159)))
            expected_field = b''.join(references[event['decoration']][x * 3:x * 3 + 3] for x in FIELD_X)
            if field != expected_field:
                raise ValueError('detail-map pixels disagree with installed decoration')
            expected = map_reference(detail_map_cases()[index], event['detail_map_scroll'])
            if len(expected) < 4:
                raise ValueError('insufficient opaque detail-map reference points')
            if any(image.getpixel(point) != rgb for point, rgb in expected.items()):
                raise ValueError('detail-map pixels disagree with ROM-derived scrolled map')
            samples += len(expected)
            crops.setdefault(index, {})[event['event']] = image.crop((24, 40, 216, 120)).tobytes()
        checked += 1
    if checked != 330 or set(crops) != set(range(66)):
        raise ValueError('incomplete detail-map pixel checkpoints')
    cases = detail_map_cases()
    for index, values in crops.items():
        if values['detail_ready'] != values['detail_held']:
            raise ValueError('detail-map pixels changed after health mutation')
        if not cases[index]['limit'] and len(set(values.values())) != 1:
            raise ValueError('immovable detail-map pixels changed at a clamp')
        if values['detail_top'] != values['detail_top_returned']:
            raise ValueError('detail-map rendered top did not return')
        if cases[index]['limit'] and values['detail_top'] == values['detail_bottom']:
            raise ValueError('scrollable detail-map pixels did not move')
    return dict(checkpoints=checked, field_pixels_per_checkpoint=len(FIELD_X), field_points=[(x, 159) for x in FIELD_X],
                map_points=MAP_POINTS, opaque_map_pixel_comparisons=samples,
                map_crop=[24, 40, 216, 120], map_return_comparisons=len(crops), held_map_comparisons=len(crops),
                moving_map_comparisons=sum(bool(c["limit"]) for c in cases),
                immovable_map_cases=sum(not c["limit"] for c in cases))


def validate_detail_map_pixels(directory, events, rom_path, language):
    """Independently decode native geometry, decoration and scrolled BG2 tiles."""
    from functools import lru_cache
    from PIL import Image
    from export_screen_backgrounds import Rom, Scene, menu_recipes, u16
    rom = Rom(rom_path, language)
    table = rom.symbol('gUnk_08128E94')
    if (tuple(table[i * 8 + 2] for i in range(17)) != LIMITS or
            tuple(table[i * 8 + 5] for i in range(17)) != Y_ORIGINS or
            tuple(table[i * 8 + 6] for i in range(17)) != Y_ORIGINS):
        raise ValueError('detail-map ROM metadata disagrees with audited geometry')
    references = []
    for _, scene, layers, backdrop in menu_recipes(rom):
        references.append(Image.frombytes('RGB', (240, 160), scene.render(layers, backdrop)).crop((0, 159, 240, 160)).tobytes())
        if len(references) == 3:
            break

    @lru_cache(maxsize=66)
    def map_scene(health, group, area):
        scene = Scene(rom)
        for gfx in (0x56 + health, 129, group):
            scene.gfx(gfx)
        for palette in (181, 185, 186 + area):
            scene.pal(palette)
        scene.upload(2, 0x1d03)
        return scene

    def map_reference(case, scroll):
        scene = map_scene(case['decoration'], case['map_group'], case['area'])
        result = {}
        for x, y in MAP_POINTS:
            index = scene.sample(0x1d03, x, y + scroll - case['origin'])
            if index is not None:
                color = u16(scene.palette, index * 2)
                result[x, y] = tuple(((color >> shift) & 31) << 3 for shift in (0, 5, 10))
        return result

    return validate_detail_map_pixel_samples(directory, events, references, map_reference)
