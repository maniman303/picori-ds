#!/usr/bin/env python3
"""Check observed native local-effect registers against unfaded source owners.

Inputs are completed, retained background-audit traces and a ROM-derived
coverage report. This does not infer a transition from framebuffer colors and
does not claim that the traces cover every possible intermediate effect.
"""
import argparse
import hashlib
import json
from pathlib import Path


def frame_rows(path):
    with path.open() as stream:
        return [row for line in stream if 'event' not in (row := json.loads(line))]


def check_traces(menus, fusion, credits, coverage, figurines=None, gameover=None, travel=None, prologue=None):
    images = coverage['images']
    for name, layer_mask in (('title_screen', 0x02), ('kinstone_pieces_background_normal', 0x04),
                             ('kinstone_background_normal', 0x0e), ('credits_01', 0x04)):
        if images[name]['layer_mask'] != layer_mask:
            raise ValueError(f'{name}: unexpected unfaded layer ownership')

    def selected(rows, condition, field, expected_values):
        matches = [row for row in rows if condition(row)]
        values = {row[field] for row in matches}
        if not matches or not expected_values.issubset(values):
            raise ValueError(f'missing native {field} timeline: {sorted(values)}')
        return dict(frames=len(matches), values=sorted(values))

    # EU title's rays (BG0 first, BG1 second) animate over BG1 artwork.
    title = selected(menus, lambda r: r.get('task') == 0 and r.get('bldcnt') == 0x241,
                     'alpha', {0x0909, 0x0d05})
    # On tab switches BG2 can itself be first target; its second input is
    # required. Never classify all alpha registers as either local or global.
    pieces = selected(menus, lambda r: r.get('family') == 'pieces' and r.get('bldcnt') == 0xe46,
                      'alpha', {0x1000, 0x020e})
    # The fusion success flash brightens OBJ, not the source BG1/BG2/BG3.
    flash = selected(fusion, lambda r: r.get('family') == 'fusion' and r.get('bldcnt') == 0x3f90,
                     'fusion_brightness', {0, 1, 14})
    # Credits text is BG1 first target; the installed illustration is BG2.
    staff = selected(credits, lambda r: r.get('task') == 4 and r.get('bldcnt') == 0x2442 and r.get('installed', -1) >= 0,
                     'alpha', {0x0010, 0x1000})
    masks = sorted({r.get('fade_mask') for r in credits if r.get('task') == 4})
    result = dict(title_alpha=title, pieces_transition=pieces, fusion_flash=flash, credits_alpha=staff,
                  credits_fade_masks=masks, masked_credits_observed=0xffff7fff in masks)
    if figurines is not None:
        if any(row['layer_mask'] != 0x20 for row in coverage['figurine_field']):
            raise ValueError('figurine field did not resolve to the backdrop')
        result['figurine_prize_brightness'] = selected(
            figurines, lambda r: r.get('figurine_field') == 1 and r.get('figurine_blend') == 0xbf,
            'figurine_brightness', {0, 8, 16})
    if gameover is not None:
        frames = [r for r in gameover if r.get('task') == 3]
        gameover_masks = {r['fade_mask'] for r in frames}
        if not {0xffff0001, 0xffff, 0xffffffff}.issubset(gameover_masks):
            raise ValueError('missing native Game Over palette-mask states')
        if any((mask & 1) == 0 for mask in gameover_masks):
            raise ValueError('Game Over backdrop bank zero was excluded')
        result['gameover'] = dict(frames=len(frames), masks=sorted(gameover_masks),
                                  bldcnt=sorted({r['bldcnt'] for r in frames}))
    if travel is not None:
        if images['fast_travel_background']['layer_mask'] != 0x08:
            raise ValueError('fast-travel source is not BG3')
        frames = [r for r in travel if r.get('travel_field') == 1]
        controls = {r['bldcnt'] for r in frames}
        if not {0, 0x3648, 0x2f40}.issubset(controls):
            raise ValueError('missing native fast-travel blend states')
        # 0x3648 has BG3 as first target and backdrop among second targets.
        if not any(r['bldcnt'] == 0x3648 and r['alpha'] == 0x0f01 for r in frames):
            raise ValueError('missing native BG3/backdrop alpha coefficients')
        result['fast_travel'] = dict(frames=len(frames), bldcnt=sorted(controls),
                                     fade_masks=sorted({r['fade_mask'] for r in frames}))
    if prologue is not None:
        if any(images[f'prologue_{i:02d}']['layer_mask'] != 0x04 for i in range(1, 7)):
            raise ValueError('prologue image source is not BG2')
        result['prologue_alpha'] = selected(
            prologue, lambda r: r.get('installed', -1) >= 0 and r.get('bldcnt') == 0x2244,
            'alpha', {0x0010, 0x1008})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('menus', 'fusion', 'credits', 'coverage', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--figurines', type=Path, help='completed figurine-prize trace, if available')
    parser.add_argument('--gameover', type=Path, help='completed Game Over trace, if available')
    parser.add_argument('--travel', type=Path, help='completed fast-travel interaction trace, if available')
    parser.add_argument('--prologue', type=Path, help='completed prologue trace, if available')
    args = parser.parse_args()
    if args.output.exists():
        parser.error('output already exists')
    coverage = json.loads(args.coverage.read_text())
    if coverage['region'] != 'EU':
        parser.error('these native observations currently require the EU fixture')
    result = check_traces(frame_rows(args.menus), frame_rows(args.fusion), frame_rows(args.credits), coverage,
                          frame_rows(args.figurines) if args.figurines else None,
                          frame_rows(args.gameover) if args.gameover else None,
                          frame_rows(args.travel) if args.travel else None,
                          frame_rows(args.prologue) if args.prologue else None)
    result['inputs'] = {name: dict(path=str(getattr(args, name)),
                                   sha256=hashlib.sha256(getattr(args, name).read_bytes()).hexdigest())
                        for name in ('menus', 'fusion', 'credits', 'coverage')}
    for name in ('figurines', 'gameover', 'travel', 'prologue'):
        path = getattr(args, name)
        if path:
            result['inputs'][name] = dict(path=str(path), sha256=hashlib.sha256(path.read_bytes()).hexdigest())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: title, pause, fusion and retained credits native effect timelines')


if __name__ == '__main__':
    main()
