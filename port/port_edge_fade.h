#ifndef PORT_EDGE_FADE_H
#define PORT_EDGE_FADE_H

#include <stdbool.h>
#include <stdint.h>
#include "port_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Number of pixels in the live-gameplay margin fade. The published world
 * rectangle is untouched; the canvas outside it is black, with a short band
 * around the world that repeats the nearest edge color and darkens to black
 * after this many pixels. */
#define PORT_EDGE_FADE_PIXELS 5

/* Apply the margin fade in place to an ABGR8888 frame. `world` is the live
 * world aperture inside the canvas (origin 0,0). `ui_mask`, when non-NULL, is
 * one byte per pixel (ui_mask_pitch bytes per row) and marks canvas-anchored
 * HUD/dialogue pixels that must be left untouched. The function is pure,
 * allocation-free and safe to call every frame. Returns false for invalid
 * input or when the world already fills the frame (no margins to paint). */
bool Port_EdgeFade_Apply(uint32_t* frame, int pitch, int width, int height,
                         PortViewportRect world, const uint8_t* ui_mask,
                         int ui_mask_pitch);

#ifdef __cplusplus
}
#endif
#endif
