#include "port_oam.h"

#include <limits.h>
#include <string.h>

static VirtuaPPUObjPosition sBuild[MODE1_GBA_OAM_COUNT];
static VirtuaPPUObjPosition sLatched[MODE1_GBA_OAM_COUNT];

void Port_Oam_BeginFrame(void) {
    memset(sBuild, 0, sizeof(sBuild));
}

void Port_Oam_Record(unsigned slot, uint16_t attr0, uint16_t attr1, int x, int y, bool hidden) {
    if (slot >= MODE1_GBA_OAM_COUNT)
        return;
    VirtuaPPUObjPosition position = { 0 };
    position.attr0 = attr0;
    position.attr1 = attr1;
    position.flags = VIRTUAPPU_OBJ_POSITION_VALID;
    if (hidden || x < INT16_MIN || x > INT16_MAX || y < INT16_MIN || y > INT16_MAX) {
        position.flags |= VIRTUAPPU_OBJ_POSITION_HIDDEN;
    } else {
        position.x = (int16_t)x;
        position.y = (int16_t)y;
    }
    sBuild[slot] = position;
}

void Port_Oam_Latch(unsigned count) {
    if (count > MODE1_GBA_OAM_COUNT)
        count = MODE1_GBA_OAM_COUNT;
    memcpy(sLatched, sBuild, count * sizeof(sBuild[0]));
    memset(sLatched + count, 0, (MODE1_GBA_OAM_COUNT - count) * sizeof(sLatched[0]));
}

void Port_Oam_Invalidate(void) {
    memset(sBuild, 0, sizeof(sBuild));
    memset(sLatched, 0, sizeof(sLatched));
}

const VirtuaPPUObjPosition* Port_Oam_Positions(void) {
    return sLatched;
}
