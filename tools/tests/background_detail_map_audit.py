#!/usr/bin/env python3
"""Failure-oriented native detail-map audit checks."""
import copy
from pathlib import Path
import sys
import unittest
import tempfile
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from capture_background_audit import validate_trace
from background_detail_map_audit import detail_map_cases, validate_detail_map_pixel_samples, MAP_POINTS


def trace():
    rows = []
    sample = dict(schema=2, family='unknown', task=2, state=2, substate=2, ui=1,
                  fade_active=0, capture=True, world_area=3, world_room=8,
                  max_health=24, heading_style=1, heading_x=64, heading_y=16,
                  heading_transition=0, detail_pause_phase=2, detail_input=0,
                  detail_has_map=1)

    def event(name, value, capture=False):
        sample['frame'] = len(rows)
        rows.append(dict(sample, event=name, value=value))
        if capture:
            rows.append(dict(sample))

    for health in range(3):
        sample.update(family='unknown', substate=2, detail_map_case=health * 22)
        event('detail_open_request', health)
        sample.update(substate=7, ui=1, decoration=health, pause_screen=1)
        event('menu_decoration', health)
        sample['family']='titled'
        event('pause_screen', 1)
        for index, case in enumerate(detail_map_cases()):
            if case['decoration'] != health:
                continue
            sample.update(detail_map_case=index, map_group=case['map_group'], detail_map_area=case['area'],
                          detail_map_limit=case['limit'], detail_map_scroll=case['limit']//2,
                          detail_bg1_y=-case['origin'], detail_bg2_y=case['limit']//2-case['origin'],
                          detail_house=int(case['recipe']==17), detail_fused=int(case['recipe']==18),
                          detail_snapshot_area=8 if case['recipe']>=19 else 3,
                          detail_snapshot_room=case['recipe']-19 if case['recipe']>=19 else 8,
                          health=(24,15,8)[health])
            event('detail_case_request', index)
            sample['pause_screen']=6
            event('pause_screen', 6)
            event('map_detail', case['map_group'])
            event('detail_ready', index, True)
            sample['health']=24 if health==2 else 8
            event('detail_health_mutated', sample['health'])
            event('detail_held', index, True)
            segments = ((64, list(range(case['limit']//2-2,-1,-2))+[0,0], 'detail_top'),
                        (128,list(range(2,case['limit']+1,2))+[case['limit']]*2,'detail_bottom'),
                        (64,list(range(case['limit']-2,-1,-2))+[0,0],'detail_top_returned'))
            for key, offsets, name in segments:
                for offset in offsets:
                    sample.update(detail_map_scroll=offset, detail_bg2_y=offset-case['origin'], detail_input=key)
                    event('detail_scroll',key)
                sample['detail_input']=0
                event(name,index,True)
            event('detail_back_request',index)
            # Frame following injection sees the native B input.
            sample.update(frame=sample['frame']+1,detail_input=2)
            rows.append(dict(sample,capture=False))
            sample.update(detail_input=0,pause_screen=4)
            event('pause_screen',4)
            event('detail_map_returned',index,True)
        sample['detail_map_case']=(health+1)*22
        event('detail_exit_request',health)
        sample.update(frame=sample['frame']+1,detail_input=8)
        rows.append(dict(sample,capture=False))
        sample.update(detail_input=0,substate=2,ui=0,family='unknown')
        event('detail_game_restored',health,True)
    event('detail_maps_complete',66)
    return rows


class DetailMapAuditTests(unittest.TestCase):
    def setUp(self):
        self.rows=trace()

    def reject(self, mutate):
        rows=copy.deepcopy(self.rows)
        mutate(rows)
        with self.assertRaises((ValueError,StopIteration)):
            validate_trace(rows,'detail_maps')

    def test_complete_22_recipe_three_health_matrix(self):
        events,frames=validate_trace(self.rows,'detail_maps')
        self.assertEqual(len([r for r in events if r['event']=='detail_ready']),66)
        self.assertEqual(len([r for r in frames if r['pause_screen']==6]),330)
        self.assertEqual(len({c['map_group'] for c in detail_map_cases()}),22)
        self.assertEqual(sum(c['limit']==0 for c in detail_map_cases()),12)

    def test_missing_terminal_or_case_rejected(self):
        for name in ('detail_maps_complete','detail_ready','detail_map_returned','map_detail'):
            self.reject(lambda rows: rows.pop(next(i for i,r in enumerate(rows) if r.get('event')==name)))

    def test_case_order_and_interaction_order(self):
        def swap(rows):
            points=[i for i,r in enumerate(rows) if r.get('event') in ('detail_top','detail_bottom')][:2]
            rows[points[0]],rows[points[1]]=rows[points[1]],rows[points[0]]
        self.reject(swap)

    def test_wrong_recipe_and_conditional_fixtures(self):
        for key,value in (('map_group',0x70),('detail_house',1),('detail_fused',1),('detail_map_area',2)):
            self.reject(lambda rows: next(r for r in rows if r.get('event')=='detail_ready').update({key:value}))
        self.reject(lambda rows: next(r for r in rows if r.get('event')=='detail_ready' and r['value']==19).update(detail_snapshot_room=2))

    def test_installed_health_and_family_retained(self):
        for key,value in (('decoration',2),('health',24),('family','plain'),('heading_x',0)):
            self.reject(lambda rows: next(r for r in rows if r.get('event')=='detail_held').update({key:value}))
        self.reject(lambda rows: rows.append(dict(next(r for r in rows if r.get('event')=='menu_decoration'))))

    def test_capture_receipt_must_be_neutral_and_matching(self):
        for key,value in (('fade_active',1),('detail_map_case',2),('substate',2),('detail_map_scroll',2)):
            self.reject(lambda rows: next(r for r in rows if 'event' not in r and r.get('pause_screen')==6).update({key:value}))

    def test_offsets_and_both_bg_origins(self):
        for key,value in (('detail_map_limit',90),('detail_map_scroll',3),('detail_bg1_y',0),('detail_bg2_y',4)):
            self.reject(lambda rows: next(r for r in rows if r.get('event')=='detail_bottom').update({key:value}))

    def test_every_held_tick_and_clamp_required(self):
        self.reject(lambda rows: rows.pop(next(i for i,r in enumerate(rows) if r.get('event')=='detail_scroll')))
        for key,value in (('detail_input',128),('detail_map_scroll',3),('detail_bg2_y',3)):
            self.reject(lambda rows: next(r for r in rows if r.get('event')=='detail_scroll').update({key:value}))
        # Zero-limit cases must still exercise both held directions at the bounds.
        self.reject(lambda rows: rows.pop(next(i for i,r in enumerate(rows) if r.get('event')=='detail_scroll' and r['detail_map_case']==1)))

    def test_back_and_start_restore_context_and_room(self):
        for name,key,value in (('detail_map_returned','pause_screen',2),('detail_game_restored','world_room',9),
                               ('detail_game_restored','family','titled')):
            self.reject(lambda rows: next(r for r in rows if r.get('event')==name).update({key:value}))

    def test_native_exit_keys_and_map_item_prerequisite(self):
        self.reject(lambda rows: next(r for r in rows if r.get('event')=='detail_ready').update(detail_has_map=0))
        for key in (2,8):
            self.reject(lambda rows: next(r for r in rows if 'event' not in r and r.get('detail_input')==key).update(detail_input=0))

    def test_rendered_field_and_map_source_and_return(self):
        events=[r for r in self.rows if r.get('event') in ('detail_ready','detail_held','detail_top','detail_bottom','detail_top_returned')]
        refs=[bytes([8+health*8]*720) for health in range(3)]
        with tempfile.TemporaryDirectory() as temporary:
            directory=Path(temporary)
            for event in events:
                image=Image.new('RGB',(240,160),(16,24,32))
                health=event['decoration']
                for x in range(240): image.putpixel((x,159),(8+health*8,)*3)
                if event['event']=='detail_bottom' and event['detail_map_limit']:
                    image.putpixel((32,48),(40,48,56))
                image.save(directory / f"{event['frame']:06d}.png")
            reference=lambda case,scroll: {point:(16,24,32) for point in MAP_POINTS}
            result=validate_detail_map_pixel_samples(directory,events,refs,reference)
            self.assertEqual(result['opaque_map_pixel_comparisons'],5280)
            # Wrong source and insufficient source coverage cannot pass using
            # only a self-consistent screenshot pair.
            for bad in (lambda case,scroll:{point:(8,8,8) for point in MAP_POINTS}, lambda case,scroll:{}):
                with self.assertRaises(ValueError):
                    validate_detail_map_pixel_samples(directory,events,refs,bad)
            target=next(e for e in events if e['event']=='detail_top_returned')
            path=directory / f"{target['frame']:06d}.png"
            with Image.open(path) as original: image=original.copy()
            image.putpixel((32,48),(56,56,56))
            image.save(path)
            with self.assertRaisesRegex(ValueError,'top did not return'):
                validate_detail_map_pixel_samples(directory,events,refs,reference)
            target=events[0]
            path=directory / f"{target['frame']:06d}.png"
            with Image.open(path) as original: image=original.copy()
            image.putpixel((0,159),(0,0,0))
            image.save(path)
            with self.assertRaisesRegex(ValueError,'installed decoration'):
                validate_detail_map_pixel_samples(directory,events,refs,reference)

    def test_family_cannot_publish_early_or_remain_after_exit(self):
        self.reject(lambda rows: next(r for r in rows if r.get('event')=='detail_open_request').update(family='titled'))


if __name__=='__main__':
    unittest.main()
