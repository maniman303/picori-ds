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
/* HUD/dialogue widgets are positioned against the canvas, not the live world
 * aperture. Tag every OAM piece emitted inside this scope so a tall canvas
 * composition leaves it at its absolute position. */
void Port_Oam_SetCanvasContext(bool canvas);
bool Port_Oam_CanvasContext(void);
void Port_Oam_Latch(unsigned count);
void Port_Oam_Invalidate(void);
/* Raw hardware writes cannot retain stale signed coordinates, even when the
 * packed attributes happen to match the previous slot. */
void Port_Oam_InvalidateLatchedRange(unsigned offset, unsigned length);
/* A tall scene may use only visible slots with matching signed metadata. */
bool Port_Oam_VisibleSlotsTagged(const uint16_t* oam);
const VirtuaPPUObjPosition* Port_Oam_Positions(void);

/* The direct menu heading can be clipped completely before OAM emission.
 * Retain its source and signed pose with the OAM build/VBlank lifetime. */
typedef struct {
    int16_t x, y;
    uint16_t sprite;
    bool valid;
} PortOamMenuHeading;
void Port_Oam_RecordMenuHeading(int x, int y, unsigned sprite);
const PortOamMenuHeading* Port_Oam_MenuHeading(void);

#ifdef __cplusplus
}
#endif
#endif
