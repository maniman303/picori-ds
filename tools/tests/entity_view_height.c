/* Exercise the actual entity-specific screen-boundary logic at both heights. */
#include "global.h"
#include "entity.h"
#include "room.h"
#include "script.h"
#include "enemy/gyorg.h"
#undef static_assert
#include <assert.h>
#include <stdio.h>
#include <string.h>

RoomControls gRoomControls;
static int width = 240, height = 160, deleted;
static u32 randomValue;
static Entity spawned;
typedef struct { Entity base; u8 filler[0x14]; u16 unk_78; u8 unk_7a, unk_7b; } TestCucco;
u32 Random(void) { return randomValue; }
Entity* CreateEnemy(u32 id, u32 type) { (void)id; (void)type; memset(&spawned, 0, sizeof(spawned)); return &spawned; }
int Port_Widescreen_EffectiveViewWidth(void) { return width; }
int Port_Widescreen_EffectiveViewHeight(void) { return height; }
void LinearMoveUpdate(Entity* e) { (void)e; }
void GetNextFrame(Entity* e) { (void)e; }
void DeleteThisEntity(void) { deleted++; }
extern bool32 CuccoAggr_IsOutsideScroll(void*);
extern void sub_080391B4(void*);
extern u32 sub_080644C8(void*);
extern void sub_0806BF44(Entity*, ScriptExecutionContext*);
extern void GyorgChild_Action1(GyorgChildEntity*);
extern void GyorgChild_Action3(GyorgChildEntity*);

static void check(int h) {
    Entity e = {0}; ScriptExecutionContext context = {0};
    GyorgChildEntity gyorg = {0};
    height = h; gRoomControls.scroll_x = 100; gRoomControls.scroll_y = 200;
    e.x.HALF.HI = 150;
    e.y.HALF.HI = 200 + h + 24;
    assert(sub_080644C8(&e));
    e.y.HALF.HI++; assert(!sub_080644C8(&e));
    width = 285; e.y.HALF.HI = 200; e.x.HALF.HI = 100 + width + 8;
    assert(sub_080644C8(&e)); e.x.HALF.HI++; assert(!sub_080644C8(&e));
    width = 240; e.x.HALF.HI = 150;
    e.y.HALF.HI = 200 + h + 23;
    sub_0806BF44(&e, &context); assert(context.condition == 1);
    e.y.HALF.HI++; sub_0806BF44(&e, &context); assert(context.condition == 0);
    e.y.HALF.HI = 200 + h + 36;
    assert(!CuccoAggr_IsOutsideScroll(&e));
    e.y.HALF.HI++; assert(CuccoAggr_IsOutsideScroll(&e));
    gyorg.base.animationState = 4; gyorg.base.action = 1; gyorg.base.timer = 2;
    gyorg.base.y.HALF.HI = 200 + h + 23;
    GyorgChild_Action1(&gyorg); assert(gyorg.base.action == 1);
    gyorg.base.y.HALF.HI++; GyorgChild_Action1(&gyorg); assert(gyorg.base.action == 2);
    TestCucco cucco = {0}; cucco.unk_7a = 1;
    randomValue = 7; sub_080391B4(&cucco);
    assert(spawned.y.HALF.HI == 200 + h + 8);
    cucco.unk_7b = 0; randomValue = 0; sub_080391B4(&cucco);
    assert(spawned.y.HALF.HI == 192);
    cucco.unk_7b = 0; randomValue = 16; sub_080391B4(&cucco);
    assert(spawned.y.HALF.HI == 200 + 16 + 2 * (h - 32) / 4);
    cucco.unk_7b = 0; randomValue = 20; sub_080391B4(&cucco);
    assert(spawned.y.HALF.HI == 200 + 16 + (h - 32) / 4);
    deleted = 0; gyorg.base.y.HALF.HI = 200 + h + 39;
    GyorgChild_Action3(&gyorg); assert(deleted == 0);
    gyorg.base.y.HALF.HI++; GyorgChild_Action3(&gyorg); assert(deleted == 1);
}
int main(void) { check(160); check(192); puts("Individual NPC and enemy height bounds: PASS"); return 0; }
