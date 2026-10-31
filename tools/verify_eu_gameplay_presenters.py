#!/usr/bin/env python3
"""Run the EU live-gameplay presenter matrix and check integration invariants.

This covers the remaining live-gameplay part of the broad EU acceptance item:
world/HUD/dialogue at native, 16:9, 21:9 and 4:3 (256x192), software and
SDL_GPU presenters, live profile switches while dialogue is open, and a
conspicuous artwork probe that must never appear in gameplay. It does not
replace the functional-screen, fade, PNG-failure and regional matrices already
retained under `.tmp/`.

The ROM-derived captures and logs stay in --output; the script restores the
temporary magenta artwork before returning, including on failure. PNG I/O uses
ImageMagick (`identify`/`convert`) so the verifier does not require PIL.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent.parent
MAGENTA = (255, 0, 255)


class RgbImage:
    """Minimal RGB PNG reader/writer via ImageMagick (no PIL dependency)."""

    def __init__(self, width, height, data):
        if len(data) != width * height * 3:
            raise SystemExit(f'raw RGB payload {len(data)}, expected {width * height * 3}')
        self.width = width
        self.height = height
        self._data = data

    @classmethod
    def open(cls, path):
        path = Path(path)
        size = subprocess.run(['identify', '-format', '%w %h', str(path)],
                              check=True, text=True, capture_output=True).stdout.split()
        if len(size) != 2:
            raise SystemExit(f'{path}: cannot read PNG dimensions')
        width, height = int(size[0]), int(size[1])
        data = subprocess.run(['convert', str(path), '-depth', '8', 'rgb:-'],
                              check=True, capture_output=True).stdout
        return cls(width, height, data)

    @property
    def size(self):
        return (self.width, self.height)

    def tobytes(self):
        return self._data

    def load(self):
        image = self

        class Pixels:
            def __getitem__(self, xy):
                x, y = xy
                index = (y * image.width + x) * 3
                return tuple(image._data[index:index + 3])

        return Pixels()


def write_solid_png(path, width, height, color):
    subprocess.run(['convert', '-size', f'{width}x{height}', f'xc:rgb({color[0]},{color[1]},{color[2]})',
                    str(path)], check=True)

PROFILES = {
    'native': {'aspect': 'native', 'mode': 0, 'window': (960, 640), 'width': 240, 'height': 160},
    '169': {'aspect': '16:9', 'mode': 1, 'window': (1140, 640), 'width': 285, 'height': 160},
    '219': {'aspect': '21:9', 'mode': 2, 'window': (1496, 640), 'width': 374, 'height': 160},
    '43': {'aspect': '4:3', 'mode': 4, 'window': (1024, 768), 'width': 256, 'height': 192},
}
SCENES = {
    'field_hud': {},
    'field_clean': {'TMC_ROOMCAP_CLEAN': '1'},
    'dialogue_bottom': {'TMC_ROOMCAP_MSG': '0x501'},
    'dialogue_top': {'TMC_ROOMCAP_MSG': '0x501,1'},
}
SWITCHES = {
    'switch_native_to_169_bottom': ('native', 1, 'dialogue_bottom'),
    'switch_169_to_219_top': ('169', 2, 'dialogue_top'),
    'switch_native_to_43_bottom': ('native', 4, 'dialogue_bottom'),
    'switch_43_to_169_top': ('43', 1, 'dialogue_top'),
}
BASE_CONFIG = {
    'hd_dialogue': False,
    'window_scale': 4,
    'internal_scale': 1,
    'upscale_method': 'nearest',
    'color_correction': False,
    'lcd_persistence': False,
    'vsync': False,
    'menu_hint_seen': True,
    'decouple_render': False,
    'show_fps': False,
    'ra_enabled': False,
    'discord_rpc': False,
}


def stage_art(assets):
    saved = {}
    for name, profile in PROFILES.items():
        path = assets / str({'native': 32, '169': 169, '219': 219, '43': 43}[name]) / 'fast_travel_background.png'
        if not path.is_file():
            continue
        backup = path.with_suffix('.png.matrix-backup')
        shutil.copy2(path, backup)
        saved[path] = backup
        write_solid_png(path, profile['width'], profile['height'], MAGENTA)
    return saved


def restore_art(saved):
    for path, backup in saved.items():
        if backup.is_file():
            shutil.move(backup, path)


def native_sample(image, profile):
    """Sample the center of each 4× block; block edges may differ slightly
    between SDL_Renderer and SDL_GPU even when the composed frame is identical."""
    if image.width != profile['width'] * 4 or image.height != profile['height'] * 4:
        raise SystemExit(f'presentation {image.size}, expected '
                         f'{profile["width"] * 4}x{profile["height"] * 4}')
    px = image.load()
    out = bytearray()
    for y in range(profile['height']):
        for x in range(profile['width']):
            out.extend(px[x * 4 + 1, y * 4 + 1])
    return bytes(out)


def run_case(binary, rom, output, name, profile_name, scene_name, backend, switch_mode=None,
             presentation=True):
    profile = PROFILES[profile_name]
    scene = SCENES[scene_name]
    run_dir = output / name
    run_dir.mkdir(parents=True, exist_ok=True)
    config = dict(BASE_CONFIG)
    config['aspect_mode'] = profile['aspect']
    config['render_backend'] = backend
    (run_dir / 'config.json').write_text(json.dumps(config))
    env = {key: value for key, value in os.environ.items()
           if not key.startswith(('TMC_', 'SDL_'))}
    env.update({
        'TMC_AUTOPLAY': '1',
        'TMC_ROOMCAP': '1',
        'TMC_ROOMCAP_WARP': '0x03,0x08,0x1d8,0x138,0',
        'TMC_ROOMCAP_SETTLE': '300',
        'TMC_ROOMCAP_WINDOW': ','.join(str(v) for v in profile['window']),
        'TMC_ROOMCAP_OUT': 'presentation.png' if presentation else 'base.png',
        'TMC_BASEROM': str(rom),
        'TMC_COLOR_CORRECTION': '0',
        'TMC_LCD_PERSISTENCE': '0',
        'SDL_AUDIODRIVER': 'dummy',
        'SDL_VIDEODRIVER': 'offscreen' if backend == 'gpu' else 'dummy',
        'OMP_NUM_THREADS': '1',
    })
    env.update(scene)
    if presentation:
        env['TMC_ROOMCAP_PRESENTATION'] = '1'
    if switch_mode is not None:
        env['TMC_ROOMCAP_SWITCH_ASPECT'] = str(switch_mode)
    log_path = run_dir / 'run.log'
    with log_path.open('wb') as log:
        result = subprocess.run([str(binary), '--no-audio'], cwd=run_dir, env=env,
                                stdout=log, stderr=subprocess.STDOUT, timeout=90)
    if result.returncode:
        raise SystemExit(f'{name}: game exited {result.returncode}; see {log_path}')
    text = log_path.read_text(errors='replace')
    if backend == 'gpu' and 'PPU initialized with SDL_GPU backend' not in text:
        raise SystemExit(f'{name}: SDL_GPU was unavailable; refusing software fallback')
    if switch_mode is not None and f'switched aspect -> {switch_mode}' not in text:
        raise SystemExit(f'{name}: profile switch was not applied')
    image_name = 'presentation.png' if presentation else 'base.png'
    image = RgbImage.open(run_dir / image_name)
    expected = profile['window'] if presentation else (profile['width'], profile['height'])
    if image.size != expected:
        raise SystemExit(f'{name}: {image_name} {image.size}, expected {expected}')
    if presentation:
        raw = image.tobytes()
        if any(raw[index:index + 3] == bytes(MAGENTA) for index in range(0, len(raw), 3)):
            raise SystemExit(f'{name}: conspicuous artwork leaked into live gameplay')
    return image, text


def band_difference(a, b, profile, y0, y1):
    width = a.width
    sx = a.width / profile['width']
    sy = a.height / profile['height']
    native_x = (profile['width'] - 240) // 2
    x0, x1 = max(0, int((native_x + 8) * sx)), min(width, int((native_x + 232) * sx))
    top, bottom = max(0, int(y0 * sy)), min(a.height, int(y1 * sy))
    changed = 0
    pa, pb = a.load(), b.load()
    for y in range(top, bottom):
        for x in range(x0, x1):
            ca, cb = pa[x, y], pb[x, y]
            if abs(ca[0] - cb[0]) + abs(ca[1] - cb[1]) + abs(ca[2] - cb[2]) > 30:
                changed += 1
    return changed


def check_hud_and_dialogue(images, profile_name, backend):
    profile = PROFILES[profile_name]
    hud = images[(profile_name, 'field_hud', backend)]
    bottom = images[(profile_name, 'dialogue_bottom', backend)]
    top = images[(profile_name, 'dialogue_top', backend)]
    # Hearts are red in the native top-left; buttons are bright low-chroma
    # pixels in the top-right. Scale the native regions into the presentation.
    sx, sy = hud.width / profile['width'], hud.height / profile['height']
    red = grey = 0
    px = hud.load()
    for y in range(0, int(18 * sy)):
        for x in range(0, int(20 * sx)):
            r, g, b = px[x, y]
            if r > 140 and g < 100 and b < 100:
                red += 1
    for y in range(0, int(48 * sy)):
        for x in range(int((profile['width'] - 48) * sx), hud.width):
            r, g, b = px[x, y]
            if abs(r - g) < 25 and abs(g - b) < 25 and r > 90:
                grey += 1
    if red < 20 or grey < 100:
        raise SystemExit(f'{profile_name}: HUD corners missing (red={red}, grey={grey})')
    # Standard top messages stay at the native top; standard bottom messages
    # follow the canvas bottom inset (Port_Viewport_StandardMessageTranslation).
    # The same rule applies to 160 and taller canvases.
    top_band = (8, 56)
    bottom_band = (96, 144) if profile['height'] == 160 else (profile['height'] - 64, profile['height'] - 16)
    bottom_changed = band_difference(bottom, hud, profile, *bottom_band)
    top_changed = band_difference(top, hud, profile, *top_band)
    if bottom_changed < 200 or top_changed < 200:
        raise SystemExit(f'{profile_name}: dialogue band missing '
                         f'(bottom={bottom_changed}, top={top_changed})')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build/pc/tmc_pc')
    parser.add_argument('--rom', type=Path, default=ROOT / 'build/pc/baserom.gba')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--assets', type=Path, default=ROOT / 'build/pc/assets/override')
    parser.add_argument('--backends', default='software,gpu',
                        help='comma-separated presenters to exercise (software,gpu)')
    args = parser.parse_args()
    backends = [item.strip() for item in args.backends.split(',') if item.strip()]
    if not backends or any(item not in ('software', 'gpu') for item in backends):
        raise SystemExit('--backends must be a subset of software,gpu')
    binary = args.binary.resolve(strict=True)
    rom = args.rom.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)

    saved = stage_art(args.assets)
    images = {}
    logs = {}
    try:
        for profile_name in PROFILES:
            for scene_name in SCENES:
                for backend in backends:
                    name = f'{profile_name}_{scene_name}_{backend}'
                    image, text = run_case(binary, rom, output, name, profile_name,
                                           scene_name, backend)
                    images[(profile_name, scene_name, backend)] = image
                    logs[(profile_name, scene_name, backend)] = text
        for name, (initial, switch_mode, scene_name) in SWITCHES.items():
            for backend in backends:
                run_name = f'{name}_{backend}'
                image, text = run_case(binary, rom, output, run_name, initial,
                                       scene_name, backend, switch_mode)
                images[(run_name, 'switch', backend)] = image
                logs[(run_name, 'switch', backend)] = text
        # The native framebuffer itself must be byte-identical between the
        # software and GPU presentation paths; block-edge sampling differences
        # in the presented window are checked separately below.
        for profile_name in PROFILES:
            for backend in backends:
                run_name = f'{profile_name}_base_{backend}'
                image, text = run_case(binary, rom, output, run_name, profile_name,
                                       'field_hud', backend, presentation=False)
                images[(profile_name, 'base', backend)] = image
    finally:
        restore_art(saved)

    if 'software' in backends and 'gpu' in backends:
        for profile_name in PROFILES:
            sw = images[(profile_name, 'base', 'software')]
            gpu = images[(profile_name, 'base', 'gpu')]
            if sw.tobytes() != gpu.tobytes():
                raise SystemExit(f'{profile_name}: software/GPU base framebuffer mismatch')
            for scene_name in SCENES:
                sw_p = images[(profile_name, scene_name, 'software')]
                gpu_p = images[(profile_name, scene_name, 'gpu')]
                if native_sample(sw_p, PROFILES[profile_name]) != native_sample(gpu_p, PROFILES[profile_name]):
                    raise SystemExit(f'{profile_name}/{scene_name}: native sample mismatch')

    for profile_name in PROFILES:
        check_hud_and_dialogue(images, profile_name, backends[0])

    print('EU live-gameplay presenters: PASS')
    detail = ('software/GPU base framebuffers byte-identical, native presentation '
              'samples identical' if len(backends) > 1 else f'{backends[0]} presenter only')
    print(f'  {len(images)} captures, {detail}, magenta artwork absent, '
          f'HUD/dialogue bands present')


if __name__ == '__main__':
    main()
