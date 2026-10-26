"""Standalone map-hint installation, native reveal and procedural source checks."""


def hint_cases():
    return [dict(decoration=health, repeat=repeat, variant=variant, exit_key=(1, 2, 8)[variant % 3])
            for health in range(3) for repeat in range(2) for variant in range(6)]


def hint_reference(path, language):
    from export_screen_backgrounds import Rom, Scene, u16
    rom = Rom(path, language)
    masks = [u16(rom.symbol('gUnk_08128F4C'), index * 2) for index in range(6)]
    colors = []
    for health in range(3):
        scene = Scene(rom)
        for group in (16, 23, 25, 27, 0x56 + health, 94, 129):
            scene.gfx(group)
        for group in (11, 12, 181, 185):
            scene.pal(group)
        scene.upload(2, 0x1d0b)
        # Map BG2 scrolls upward by four pixels; row 159 lies in its clear border.
        for x in (0, 239):
            if scene.sample(0x1d0b, x, 155) is not None:
                raise ValueError('hint sample covered by native BG2')
        colors.append(u16(scene.palette, 81 * 2))
    if len(set(colors)) != 1:
        raise ValueError('hint source depends on health preload')
    return dict(masks=masks, color=colors[0])


def validate_hints(rows, events, frames):
    cases = hint_cases()
    by_frame = {row['frame']: row for row in frames}
    for phase in ('hint_before', 'hint_ready', 'hint_held'):
        checkpoints = [row for row in events if row['event'] == phase]
        if [row['value'] for row in checkpoints] != list(range(36)):
            raise ValueError('incomplete or unordered hint checkpoints')
        for row, case in zip(checkpoints, cases):
            capture = by_frame.get(row['frame'])
            if not capture or capture.get('fade_active') or (capture.get('task'), capture.get('substate'), capture.get('ui')) != (2, 7, 3):
                raise ValueError('missing neutral hint capture')
            for sample in (row, capture):
                if tuple(sample.get(k) for k in ('hint_case', 'hint_field', 'hint_variant', 'decoration',
                          'pause_screen', 'family', 'schema')) != (row['value'], 1, case['variant'], case['decoration'], 4, 'unknown', 2):
                    raise ValueError('wrong installed hint context')
                if tuple(sample.get(k) for k in ('heading_style', 'heading_x', 'heading_y', 'heading_transition')) != (0, 0, 0, 0):
                    raise ValueError('hint claimed heading')
                if sample.get('display_control', 0) & 0x800:
                    raise ValueError('hint retained health BG3')
                if sample.get('palette_zero') is None or sample['palette_zero'] != sample.get('palette_81'):
                    raise ValueError('hint lost live palette-zero source')
                if not sample.get('hint_pending') or sample.get('saved_hints', 0) & sample['hint_pending'] != sample['hint_pending']:
                    raise ValueError('hint did not install saved reveal bits')
                before = phase == 'hint_before'
                if (before and not 30 < sample.get('hint_timer', 0) <= 54) or (not before and sample.get('hint_timer') != 0):
                    raise ValueError('incorrect native hint timer')
                if sample.get('hint_mask') != (0 if before else sample['hint_pending']):
                    raise ValueError('hint reveal outside native timer boundary')
                health = (24 if case['decoration'] == 2 else 8) if phase == 'hint_held' else (24, 15, 8)[case['decoration']]
                if (sample.get('health'), sample.get('max_health')) != (health, 24):
                    raise ValueError('incorrect hint health fixture')
    names = ('hint_request', 'hint_screen', 'hint_before', 'hint_reveal', 'hint_ready',
             'hint_health_mutated', 'hint_held', 'hint_cancel', 'hint_restored')
    if [(r['event'], r.get('hint_case')) for r in events if r['event'] in names] != [
            (name, index) for index in range(36) for name in names]:
        raise ValueError('incorrect native hint lifecycle order')
    for name in ('hint_screen', 'hint_cancel'):
        if [r['value'] for r in events if r['event'] == name] != [c['variant'] if name == 'hint_screen' else c['exit_key'] for c in cases]:
            raise ValueError('incorrect hint variant or native exit')
    if [r['value'] for r in events if r['event'] == 'hints_complete'] != [36]:
        raise ValueError('incorrect hint completion count')
    if [r['value'] for r in events if r['event'] == 'menu_decoration'] != [c['decoration'] for c in cases]:
        raise ValueError('unexpected hint decoration reinstallation')
    if [r['value'] for r in events if r['event'] == 'pause_screen'] != [4] * 36:
        raise ValueError('incorrect native hint layout')
    for row in events:
        case = cases[min(row.get('hint_case', 0), 35)]
        if row['event'] == 'hint_request' and row.get('saved_hints') != (0xffff if case['repeat'] else 0):
            raise ValueError('incorrect first/repeated hint fixture')
        if row['event'] == 'hint_reveal' and (row.get('hint_timer'), row.get('hint_mask')) != (30, row['value']):
            raise ValueError('incorrect native reveal event')
    installed = False
    for row in rows:
        name = row.get('event')
        if name == 'hint_screen':
            installed = True
        elif name in ('hint_request', 'hint_restored', 'hints_complete'):
            installed = False
        elif name is None and (row.get('task'), row.get('substate'), row.get('ui')) != (2, 7, 3):
            installed = False
        if row.get('hint_field', 0) != int(installed):
            raise ValueError('hint field changed outside installation/restoration')


def validate_hint_pixels(directory, events, reference=None):
    from PIL import Image
    checked = 0
    for row in events:
        if row['event'] not in ('hint_before', 'hint_ready', 'hint_held'):
            continue
        color = row['palette_zero']
        if reference is not None and (color != reference['color'] or row['hint_pending'] != reference['masks'][row['hint_variant']]):
            raise ValueError('hint source/mask disagrees with native ROM')
        rgb = tuple(((color >> shift) & 31) << 3 for shift in (0, 5, 10))
        with Image.open(directory / f"{row['frame']:06d}.png") as image:
            if any(image.convert('RGB').getpixel(point) != rgb for point in ((0, 159), (239, 159))):
                raise ValueError('hint pixels disagree with live field')
        checked += 1
    if checked != 108:
        raise ValueError('incomplete hint pixel checkpoints')
    return dict(checkpoints=checked, pixels_per_checkpoint=2, points=[[0, 159], [239, 159]])
