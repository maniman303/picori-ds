#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "port_oam.h"
#include "port_gba_mem.h"

static void install(void) {
    for (int i = 0; i < 128; ++i) gOamMem[i * 4] = 0x200;
    gOamMem[0] = 170; gOamMem[1] = 10;
    Port_Oam_Invalidate();
    Port_Oam_Record(0, 170, 10, 10, 170, false);
    Port_Oam_Latch(1);
    assert(Port_Oam_VisibleSlotsTagged(gOamMem));
}
int main(void) {
    install();
    gba_write16(0x07000000, 170); /* identical attr0 write must discard stale position */
    assert(!Port_Oam_VisibleSlotsTagged(gOamMem));
    assert(!(Port_Oam_Positions()[0].flags & VIRTUAPPU_OBJ_POSITION_VALID));
    install();
    gba_write16(0x07000004, 5); /* tile-only edit preserves position */
    assert(Port_Oam_VisibleSlotsTagged(gOamMem));
    gba_write32(0x07000000, 170 | (10u << 16));
    assert(!Port_Oam_VisibleSlotsTagged(gOamMem));
    install();
    gba_write32(0x07000004, 0); /* tile and affine matrix do not move the sprite */
    assert(Port_Oam_VisibleSlotsTagged(gOamMem));
    assert(!Port_Oam_VisibleSlotsTagged(NULL));
    puts("Raw OAM writes invalidate stale signed positions: PASS");
    return 0;
}
