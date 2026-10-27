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

#ifdef __cplusplus
}
#endif
#endif
