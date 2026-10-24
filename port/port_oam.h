#ifndef PORT_OAM_H
#define PORT_OAM_H

#include "ppu/include/cpu/mode1.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Position side tables follow the same build/DMA lifetime as voxel tags.
 * They do not change packed OAM, entity/save layouts or quicksave formats. */
void Port_Oam_BeginFrame(void);
void Port_Oam_Record(unsigned slot, uint16_t attr0, uint16_t attr1, int x, int y, bool hidden);
void Port_Oam_Latch(unsigned count);
void Port_Oam_Invalidate(void);
const VirtuaPPUObjPosition* Port_Oam_Positions(void);

#ifdef __cplusplus
}
#endif
#endif
