#!/usr/bin/env python3
"""Failure-oriented native sleep confirmation and wake checks."""
import copy
from pathlib import Path
import sys
import tempfile
import unittest
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from capture_background_audit import validate_trace
from background_sleep_audit import sleep_cases, validate_sleep_pixels


def trace():
    rows=[]
    clock=0
    sample=dict(schema=2, task=2, state=2, substate=2, ui=0, family='unknown',
                fade_active=0, max_health=24, world_area=3, world_room=8,
                sleep_status=0, sleep_ticks=1000, sleep_menu=1, sleep_pause_phase=2,
                sleep_new_keys=0, sleep_raw_keys=1023, sleep_raw_display=8000,
                heading_style=0, heading_x=0, heading_y=0, heading_transition=0)

    def event(name,value,capture=False):
        nonlocal clock
        clock+=1
        sample['frame']=clock
        rows.append(dict(sample,event=name,value=value))
        if capture:rows.append(dict(sample,capture=True))

    def frame(**changes):
        nonlocal clock
        clock+=1
        sample.update(frame=clock,**changes)
        rows.append(dict(sample,capture=False))

    for index,case in enumerate(sleep_cases()):
        sample.update(sleep_case=index,decoration=case['decoration'],health=(24,15,8)[case['decoration']],
                      family='unknown',substate=2,ui=0,sleep_cursor=0,sleep_new_keys=0)
        event('sleep_case_request',index)
        sample.update(substate=7,ui=1)
        event('menu_decoration',case['decoration'])
        sample.update(family='titled',pause_screen=1,heading_style=1,heading_x=64,heading_y=16)
        event('pause_screen',1)
        sample['pause_screen']=2
        event('pause_screen',2)
        if index:
            for cursor,key in ((8,32),(3,64),(1,64),(0,32)):
                sample.update(sleep_cursor=cursor,sleep_new_keys=key)
                event('quest_choice',cursor)
            sample['sleep_new_keys']=0
        event('sleep_quest_ready',index,True)
        for cursor,key in ((11,32),(5,64),(4,32)):
            sample.update(sleep_cursor=cursor,sleep_new_keys=key)
            event('quest_choice',cursor)
        sample['sleep_new_keys']=0
        event('sleep_prompt_request',index)
        sample.update(family='plain',pause_screen=9,heading_style=0,heading_x=0,heading_y=0,sleep_cursor=0)
        event('pause_screen',9)
        event('sleep_prompt',0)
        event('sleep_ready',index,True)
        sample['health']=24 if case['decoration']==2 else 8
        event('sleep_health_mutated',sample['health'])
        event('sleep_held',index,True)
        choices=([],[(1,16),(0,32)],[(1,16)],[(1,2)],[(1,16)])[case['mode']]
        for cursor,key in choices:
            sample.update(sleep_cursor=cursor,sleep_new_keys=key)
            event('sleep_choice',cursor)
        sample['sleep_new_keys']=2 if case['mode']>=3 else 1
        event('sleep_answer',0 if case['accept'] else 1)
        sample['sleep_new_keys']=0
        if case['accept']:
            sample.update(sleep_status=1,fade_active=1)
            event('sleep_armed',1)
            sample['sleep_raw_display']=128
            event('sleep_loop_enter',1)
            for _ in range(24):frame()
            event('sleep_waiting',index,True)
            for _ in range(4):frame()
            event('sleep_wake_request',index)
            frame(sleep_raw_keys=1022)
            sample['sleep_status']=0
            event('sleep_loop_woke',0)
        sample.update(sleep_status=0,sleep_ticks=sample['sleep_ticks']+100,sleep_raw_keys=1023,
                      sleep_raw_display=8000,fade_active=0,pause_screen=2,family='titled',
                      sleep_cursor=4,heading_style=1,heading_x=64,heading_y=16)
        event('pause_screen',2)
        event('sleep_quest_returned',index,True)
        event('sleep_exit_request',index)
        frame(sleep_new_keys=8)
        sample.update(sleep_new_keys=0,substate=2,ui=0,family='unknown')
        event('sleep_game_restored',index,True)
    event('sleep_complete',15)
    return rows


class SleepAuditTests(unittest.TestCase):
    def setUp(self):self.rows=trace()

    def reject(self,mutate):
        rows=copy.deepcopy(self.rows)
        mutate(rows)
        with self.assertRaises((ValueError,StopIteration)):
            validate_trace(rows,'sleep')

    def test_five_choices_three_healths_and_six_native_sleeps(self):
        events,frames=validate_trace(self.rows,'sleep')
        self.assertEqual(len([r for r in events if r['event']=='sleep_loop_enter']),6)
        self.assertEqual(len([r for r in events if r['event']=='sleep_answer' and r['value']==1]),9)
        self.assertEqual(len([r for r in frames if r.get('sleep_raw_display')==128]),6)

    def test_missing_terminal_case_or_native_lifecycle(self):
        for name in ('sleep_complete','sleep_ready','sleep_prompt','sleep_answer','sleep_loop_woke','sleep_game_restored'):
            self.reject(lambda rows:rows.pop(next(i for i,r in enumerate(rows) if r.get('event')==name)))

    def test_lifecycle_order(self):
        def swap(rows):
            points=[i for i,r in enumerate(rows) if r.get('event') in ('sleep_armed','sleep_loop_enter')][:2]
            rows[points[0]],rows[points[1]]=rows[points[1]],rows[points[0]]
        self.reject(swap)

    def test_quest_navigation_and_native_keys(self):
        for key,value in (('value',10),('sleep_new_keys',16)):
            self.reject(lambda rows:next(r for r in rows if r.get('event')=='quest_choice').update({key:value}))

    def test_cursor_choices_and_b_as_selection_before_confirmation(self):
        self.reject(lambda rows:next(r for r in rows if r.get('event')=='sleep_choice' and r['sleep_case']==3).update(sleep_new_keys=16))
        self.reject(lambda rows:next(r for r in rows if r.get('event')=='sleep_answer' and r['sleep_case']==3).update(sleep_new_keys=1))
        self.reject(lambda rows:next(r for r in rows if r.get('event')=='sleep_choice').update(value=0))

    def test_decline_cannot_enter_sleep(self):
        def insert(rows):
            index=next(i for i,r in enumerate(rows) if r.get('event')=='sleep_answer' and r['sleep_case']==2)
            rows.insert(index+1,dict(rows[index],event='sleep_armed',value=1))
        self.reject(insert)

    def test_health_mutation_and_installed_family_heading(self):
        for key,value in (('health',24),('decoration',2),('family','titled'),('heading_style',1),('sleep_cursor',1)):
            self.reject(lambda rows:next(r for r in rows if r.get('event')=='sleep_held').update({key:value}))
        self.reject(lambda rows:rows.append(dict(next(r for r in rows if r.get('event')=='menu_decoration'))))

    def test_neutral_capture_must_match_case_and_native_context(self):
        for key,value in (('sleep_case',2),('fade_active',1),('sleep_status',1),('sleep_menu',2)):
            self.reject(lambda rows:next(r for r in rows if 'event' not in r and r.get('capture')).update({key:value}))

    def test_game_ticks_must_freeze_in_native_loop(self):
        self.reject(lambda rows:next(r for r in rows if r.get('event')=='sleep_waiting').update(sleep_ticks=2000))
        def shorten(rows):
            woke=next(r for r in rows if r.get('event')=='sleep_loop_woke')
            enter=next(r for r in rows if r.get('event')=='sleep_loop_enter')
            woke['frame']=enter['frame']+2
        self.reject(shorten)
        self.reject(lambda rows:rows.pop(next(i for i,r in enumerate(rows) if 'event' not in r and not r.get('capture') and r.get('sleep_raw_display')==128)))
        self.reject(lambda rows:next(r for r in rows if r.get('event')=='sleep_quest_returned').update(sleep_ticks=1000))

    def test_raw_display_and_release_then_wake_key(self):
        for name,key,value in (('sleep_loop_enter','sleep_raw_keys',1022),('sleep_waiting','sleep_raw_display',8000),
                               ('sleep_loop_woke','sleep_status',1),('sleep_loop_woke','sleep_raw_keys',1023)):
            self.reject(lambda rows:next(r for r in rows if r.get('event')==name).update({key:value}))

    def test_sleeping_frame_receipt_required(self):
        self.reject(lambda rows:next(r for r in rows if 'event' not in r and r.get('capture') and r.get('sleep_status')==1).update(sleep_raw_display=8000))

    def test_exit_input_and_original_game_room(self):
        self.reject(lambda rows:next(r for r in rows if 'event' not in r and r.get('sleep_new_keys')==8).update(sleep_new_keys=0))
        self.reject(lambda rows:next(r for r in rows if r.get('event')=='sleep_game_restored').update(world_room=9))
        self.reject(lambda rows:next(r for r in rows if r.get('event')=='sleep_game_restored').update(family='plain'))

    def test_family_cannot_publish_early_or_sleep_source_change(self):
        self.reject(lambda rows:next(r for r in rows if r.get('event')=='sleep_case_request').update(family='plain'))
        self.reject(lambda rows:next(r for r in rows if r.get('event')=='sleep_waiting').update(family='unknown'))

    def test_pixels_require_rom_health_and_white_sleep_frame(self):
        events=[r for r in self.rows if 'event' in r]
        refs=[bytes([8+health*8])*720 for health in range(3)]
        with tempfile.TemporaryDirectory() as temporary:
            directory=Path(temporary)
            for event in events:
                if event['event'] in ('sleep_quest_ready','sleep_ready','sleep_held','sleep_quest_returned','sleep_waiting'):
                    color=(255,)*3 if event['event']=='sleep_waiting' else (8+event['decoration']*8,)*3
                    Image.new('RGB',(240,160),color).save(directory/f"{event['frame']:06d}.png")
            result=validate_sleep_pixels(directory,events,refs)
            self.assertEqual(result['checkpoints'],60)
            self.assertEqual(result['sleeping_checkpoints'],6)
            with self.assertRaises(ValueError):validate_sleep_pixels(directory,events,[refs[0]]*3)
            # References that differ only beneath foreground UI cannot prove
            # health identity at the actual exposed sample points.
            hidden=[]
            for health in range(3):
                reference=bytearray(720);reference[300]=health;hidden.append(bytes(reference))
            with self.assertRaises(ValueError):validate_sleep_pixels(directory,events,hidden)
            e=next(e for e in events if e['event']=='sleep_waiting')
            path=directory/f"{e['frame']:06d}.png"
            image=Image.new('RGB',(240,160),(255,)*3);image.putpixel((100,100),(0,0,0));image.save(path)
            with self.assertRaisesRegex(ValueError,'forced-blank white'):validate_sleep_pixels(directory,events,refs)
            e=next(e for e in events if e['event']=='sleep_ready')
            Image.new('RGB',(240,160),(0,0,0)).save(directory/f"{e['frame']:06d}.png")
            with self.assertRaisesRegex(ValueError,'ROM health field'):validate_sleep_pixels(directory,events,refs)


if __name__=='__main__':unittest.main()
