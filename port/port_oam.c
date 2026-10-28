#include "port_oam.h"

#include <limits.h>
#include <string.h>

static VirtuaPPUObjPosition sBuild[MODE1_GBA_OAM_COUNT];
static VirtuaPPUObjPosition sLatched[MODE1_GBA_OAM_COUNT];
static PortOamMenuHeading sHeadingBuild, sHeadingLatched;

void Port_Oam_BeginFrame(void) {
    memset(sBuild, 0, sizeof(sBuild));
    memset(&sHeadingBuild, 0, sizeof(sHeadingBuild));
}

void Port_Oam_RecordMenuHeading(int x, int y, unsigned sprite) {
    memset(&sHeadingBuild, 0, sizeof(sHeadingBuild));
    if (x < INT16_MIN || x > INT16_MAX || y < INT16_MIN || y > INT16_MAX || sprite > UINT16_MAX)
        return;
    sHeadingBuild.x = (int16_t)x;
    sHeadingBuild.y = (int16_t)y;
    sHeadingBuild.sprite = (uint16_t)sprite;
    sHeadingBuild.valid = true;
}

const PortOamMenuHeading* Port_Oam_MenuHeading(void) { return &sHeadingLatched; }

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
    sHeadingLatched = sHeadingBuild;
}

void Port_Oam_Invalidate(void) {
    memset(sBuild, 0, sizeof(sBuild));
    memset(sLatched, 0, sizeof(sLatched));
    memset(&sHeadingBuild, 0, sizeof(sHeadingBuild));
    memset(&sHeadingLatched, 0, sizeof(sHeadingLatched));
}

const VirtuaPPUObjPosition* Port_Oam_Positions(void) {
    return sLatched;
}

void Port_Oam_InvalidateLatchedRange(unsigned offset, unsigned length) {
    if (offset >= MODE1_GBA_OAM_COUNT * 8u || !length) return;
    /* A raw write bypasses the heading producer and may change its piece or
     * palette attributes. Drop the host-only pose until the next OAM latch. */
    sHeadingLatched.valid = false;
    unsigned end = length > MODE1_GBA_OAM_COUNT * 8u - offset
                       ? MODE1_GBA_OAM_COUNT * 8u : offset + length;
    for (unsigned slot = offset / 8; slot < MODE1_GBA_OAM_COUNT && slot * 8 < end; ++slot) {
        unsigned base = slot * 8;
        if (offset < base + 4 && end > base)
            sLatched[slot].flags = 0;
    }
}

bool Port_Oam_VisibleSlotsTagged(const uint16_t* oam) {
    if (!oam) return false;
    for (unsigned i = 0; i < MODE1_GBA_OAM_COUNT; ++i) {
        uint16_t a0 = oam[i * 4], a1 = oam[i * 4 + 1];
        if (!(a0 & 0x100) && (a0 & 0x200)) continue; /* hardware disabled */
        if ((a0 >> 14) == 3) continue; /* prohibited shape */
        const VirtuaPPUObjPosition* p = &sLatched[i];
        if (!(p->flags & VIRTUAPPU_OBJ_POSITION_VALID) || p->attr0 != a0 || p->attr1 != a1)
            return false;
    }
    return true;
}
