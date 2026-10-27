#ifndef PORT_EFFECT_GEOMETRY_H
#define PORT_EFFECT_GEOMETRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t left, right; /* canvas coordinates, left inclusive/right exclusive */
} PortEffectSpan;

/* Build a circle with the game's integer midpoint rasterization in canvas
 * coordinates. native_x/y place the original 240x160 rectangle; center_x/y
 * are native coordinates. Output contains canvas_height spans, including
 * added rows and columns, and does not touch the output on invalid input.
 * This is geometry only: the caller chooses whether the effect is scene-wide,
 * how its radius changes, and how to combine it with native window coverage. */
bool Port_Effect_BuildIrisRows(int canvas_width, int canvas_height, int native_x, int native_y,
                               int center_x, int center_y, unsigned radius,
                               PortEffectSpan* rows, size_t row_capacity);

/* Scene-wide fade iris state is published only by the native fade producer.
 * Any replacement or cancellation of its window DMA clears it. The returned
 * spans describe a full-canvas shape for use only in added margins: native
 * pixels must use native window IO.
 * A closed radius of zero gives no margin coverage; radius 150 covers the
 * complete canvas. Invalid inputs fail closed and leave output untouched. */
void Port_Effect_PublishFadeIris(int center_x, int center_y, int native_radius,
                                unsigned inside_control, unsigned outside_control);
void Port_Effect_ClearFadeIris(void);
/* Invalidate only pending DMA ownership; commit at the native VBlank latch. */
void Port_Effect_ClearPendingFadeIris(void);
void Port_Effect_CommitFadeIris(void);
bool Port_Effect_ResolveFadeIris(int canvas_width, int canvas_height, int native_x, int native_y,
                                PortEffectSpan* rows, size_t row_capacity);

#ifdef __cplusplus
}
#endif
#endif
