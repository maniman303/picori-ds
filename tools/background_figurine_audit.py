"""Native collection-viewer evidence; no artwork or runtime context selection."""


def figurine_cases():
    return [dict(opening=health * 2 + post, decoration=health, staffroll=post,
                 index=index, owned=int(index in (1, 130, 136)))
            for health in range(3) for post in range(2)
            for index in ((1, 2, 7, 130, 136) if post else (1, 2, 7, 130))]


def validate_figurines(rows, events, frames):
    cases = figurine_cases()
    by_frame = {row['frame']: row for row in frames}
    for phase in ('figurine_ready', 'figurine_held'):
        checkpoints = [row for row in events if row['event'] == phase]
        if [row['value'] for row in checkpoints] != list(range(len(cases))):
            raise ValueError('incomplete or unordered figurine checkpoints')
        for row, case in zip(checkpoints, cases):
            capture = by_frame.get(row['frame'])
            if not capture or capture.get('fade_active') or not capture.get('capture'):
                raise ValueError('missing neutral figurine capture')
            for sample in (row, capture):
                expected = (row['value'], 1, case['index'], case['owned'], case['staffroll'],
                            case['decoration'], 3, 'unknown', 2)
                actual = tuple(sample.get(key) for key in ('figurine_case', 'figurine_field', 'figurine_index',
                               'figurine_owned', 'staffroll', 'decoration', 'pause_screen', 'family', 'schema'))
                if actual != expected:
                    raise ValueError('wrong installed figurine context/ownership')
                if tuple(sample.get(key) for key in ('heading_style', 'heading_x', 'heading_y', 'heading_transition')) != (0, 0, 0, 0):
                    raise ValueError('unexpected figurine heading')
                if sample.get('palette_zero') != sample.get('palette_251') or sample.get('palette_zero') is None:
                    raise ValueError('figurine field lost live palette-zero source')
                # Only the first view precedes the live health mutation.
                health = (24, 15, 8)[case['decoration']] if phase == 'figurine_ready' and case['index'] == 1 else (24 if case['decoration'] == 2 else 8)
                if (sample.get('health'), sample.get('max_health')) != (health, 24):
                    raise ValueError('figurine health mutation not exercised')
    sequence = []
    for opening in range(6):
        sequence += [('figurine_request', opening), ('figurine_screen', 1)]
        for index, case in enumerate(cases):
            if case['opening'] == opening:
                sequence += [('figurine_ready', index), ('figurine_health_mutated', 24 if opening // 2 == 2 else 8),
                             ('figurine_held', index)]
        sequence += [('figurine_cancel', 136 if opening % 2 else 130), ('figurine_restored', opening)]
    sequence += [('figurines_complete', len(cases))]
    if [(row['event'], row['value']) for row in events if row['event'] in {event for event, _ in sequence}] != sequence:
        raise ValueError('incorrect figurine installation/checkpoint/cancel order')
    if [row['value'] for row in events if row['event'] == 'menu_decoration'] != [0, 0, 1, 1, 2, 2]:
        raise ValueError('unexpected figurine decoration reinstallation')
    if [row['value'] for row in events if row['event'] == 'pause_screen'] != [3] * 6:
        raise ValueError('incorrect figurine layout installation')
    for opening in range(6):
        start = next(i for i, row in enumerate(events) if row['event'] == 'figurine_request' and row['value'] == opening)
        end = next(i for i, row in enumerate(events) if row['event'] == 'figurine_restored' and row['value'] == opening)
        choices = [row['value'] for row in events[start:end] if row['event'] == 'figurine_choice']
        expected = [2, 7] + list(range(12, 128, 5)) + [128, 129, 130]
        if opening % 2:
            expected += [135, 136]
        if choices != expected:
            raise ValueError('incorrect native figurine cursor sequence')
    installed = False
    for row in rows:
        event = row.get('event')
        if event == 'figurine_screen':
            installed = True
        elif event in ('figurine_request', 'figurine_restored', 'figurines_complete'):
            installed = False
        elif event is None and (row.get('task'), row.get('substate'), row.get('ui')) != (2, 7, 7):
            installed = False
        if row.get('figurine_field', 0) != int(installed):
            raise ValueError('figurine field changed outside installation/restoration')


FIELD_POINTS = ((100, 40), (220, 100))


def figurine_reference(rom, language):
    """Verify transparent native BG2/BG3 at the mask, independently of captures.
    BG1's description window starts at row 120. List text/OBJ avoid these points.
    """
    from export_screen_backgrounds import Rom, Scene, BG_BUFFERS, u16
    values = []
    for health in range(3):
        scene = Scene(Rom(rom, language))
        for group in (16, 23, 25, 27, 0x56 + health):
            scene.gfx(group)
        scene.gfx(89)
        scene.pal(11)
        scene.pal(12)
        scene.pal(181)
        scene.pal(206)
        scene.upload(2, 0x1d03)
        start = BG_BUFFERS[3]
        scene.ewram[start:start + 0x1000] = bytes(0x1000)
        scene.upload(3, 0x1e0b)
        if any(scene.sample(control, x, y) is not None
               for control in (0x1d03, 0x1e0b) for x, y in FIELD_POINTS):
            raise ValueError('figurine field mask is covered by native graphics')
        values.append(u16(scene.palette, 251 * 2))
    if len(set(values)) != 1:
        raise ValueError('figurine field depends on overwritten health preload')
    return values[0]


def validate_figurine_pixels(directory, events, reference=None):
    """Exposed list-field samples exclude frame, figure, text and description.
    Compare the completed raster with the unfaded live palette value, not art.
    """
    from PIL import Image
    checked = 0
    points = FIELD_POINTS
    for row in events:
        if row['event'] not in ('figurine_ready', 'figurine_held'):
            continue
        color = row['palette_zero']
        if reference is not None and color != reference:
            raise ValueError('figurine palette zero disagrees with native ROM source')
        rgb = tuple(((color >> shift) & 31) << 3 for shift in (0, 5, 10))
        with Image.open(directory / f"{row['frame']:06d}.png") as image:
            image = image.convert('RGB')
            if any(image.getpixel(point) != rgb for point in points):
                raise ValueError('figurine pixels disagree with live palette-zero field')
        checked += 1
    if checked != len(figurine_cases()) * 2:
        raise ValueError('incomplete figurine pixel checkpoints')
    return dict(checkpoints=checked, pixels_per_checkpoint=len(points), points=points)
