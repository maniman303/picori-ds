#!/usr/bin/env python3
"""Verify published adapter contexts in completed native audit traces."""
import argparse
import json
from collections import Counter
from pathlib import Path


def read(path, allow_partial=False):
    data = path.read_text()
    if allow_partial and data and not data.endswith('\n'):
        data = data.rsplit('\n', 1)[0]
    return [json.loads(line) for line in data.splitlines()]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--travel', type=Path, required=True)
    parser.add_argument('--travel-actions', type=Path, required=True)
    parser.add_argument('--menus', type=Path, required=True)
    parser.add_argument('--prologue', type=Path, required=True)
    parser.add_argument('--credits', type=Path, required=True)
    parser.add_argument('--files', type=Path, required=True)
    parser.add_argument('--gameover', type=Path, required=True)
    args = parser.parse_args()
    travel = read(args.travel)
    assert any(r.get('event') == 'travel_complete' for r in travel), 'travel run is incomplete'
    counts = Counter()
    for r in travel:
        event = r.get('event')
        if event in ('travel_case_ready', 'travel_case_held'):
            assert r['background_context'] == 41, (event, r['background_context'])
            counts[event] += 1
        elif event == 'travel_case_restored':
            assert r['background_context'] == 1, (event, r['background_context'])
            counts[event] += 1
    assert all(counts[k] == 27 for k in ('travel_case_ready', 'travel_case_held', 'travel_case_restored')), counts
    # This route may be deliberately stopped after both result paths; retain
    # completed records while discarding only an interrupted trailing write.
    actions = read(args.travel_actions, allow_partial=True)
    action_counts = Counter()
    for r in actions:
        event = r.get('event')
        if event in ('travel_action_ready', 'travel_action_held', 'travel_action_prompt_ready',
                     'fast_travel_fade', 'fast_travel_depart'):
            assert r['background_context'] == 41, (event, r['background_context'])
            action_counts[event] += 1
        elif event in ('travel_action_restored', 'travel_action_arrived'):
            assert r['background_context'] == 1, (event, r['background_context'])
            action_counts[event] += 1
    assert all(action_counts[k] >= 1 for k in ('travel_action_ready', 'travel_action_held', 'travel_action_prompt_ready',
                                                'fast_travel_fade', 'fast_travel_depart', 'travel_action_restored',
                                                'travel_action_arrived')), action_counts
    menus = read(args.menus)
    assert any(r.get('event') == 'menus_complete' for r in menus), 'menu run is incomplete'
    contexts = {'plain': (5, 27, 28), 'titled': (32, 33, 34),
                'pieces': (35, 36, 37), 'techniques': (38, 39, 40), 'fusion': (6, 29, 30)}
    menu_counts = Counter()
    for r in menus:
        if r.get('event') in ('menu_case_ready', 'menu_case_held'):
            family, health = r['family'], r['decoration']
            assert family in contexts and r['background_context'] == contexts[family][health], (family, health, r['background_context'])
            menu_counts[family, health] += 1
    assert all(menu_counts[family, health] for family in contexts for health in range(3)), menu_counts
    for path, prefix, expected in ((args.prologue, 'prologue', range(7, 13)),
                                    (args.credits, 'credits', range(13, 22))):
        rows = read(path)
        frames = [r for r in rows if 'event' not in r]
        install_frames = {r['frame'] + 1 for r in rows if r.get('event') == prefix + '_install'}
        for r in frames:
            installed = r.get('installed', -1)
            # The audit observes the installed game state before this wait's
            # VBlank upload; its PNG still contains the preceding raster.
            if installed >= 0 and r['frame'] not in install_frames:
                expected_context = (7 if prefix == 'prologue' else 13) + installed
                assert r['background_context'] == expected_context, (prefix, r['frame'], installed, r['background_context'])
        for context in expected:
            assert any(r.get('background_context') == context for r in frames), (prefix, context)
        if prefix == 'credits':
            assert any(r.get('event') == 'credits_clear' for r in rows), 'missing credits clear'
            assert any(r.get('event') == 'credits_save' for r in rows), 'missing ending save'
            assert any(r.get('background_context') == 23 for r in frames), 'missing cleared credits context'
            assert any(r.get('background_context') == 24 for r in frames), 'missing ending save context'
    files = [r for r in read(args.files) if 'event' not in r and r.get('task') == 1]
    # Task entry precedes file-screen initialization and its VBlank upload.
    assert len(files) > 2 and all(r['background_context'] == 4 for r in files[2:]), 'file select context'
    gameover = read(args.gameover)
    active, install, checked = False, 0, 0
    for r in gameover:
        if r.get('event') == 'gameover_install':
            active, install = True, r['frame']
        elif 'event' not in r:
            if r.get('task') != 3:
                active = False
            elif active and r['frame'] > install + 1:
                assert r['background_context'] == 22, ('game over', r['frame'], r['background_context'])
                checked += 1
    assert checked, 'missing installed Game Over frames'
    print('PASS: installed menu, travel, prologue, credits, file and Game Over contexts')


if __name__ == '__main__':
    main()
