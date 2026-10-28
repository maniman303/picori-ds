#ifndef PORT_TITLE_CAP_H
#define PORT_TITLE_CAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Expand a native frame in place and fill only its added margins. When
 * supplied, margins points to a separate, already effect-resolved canvas;
 * otherwise fill is used. The margins buffer must not overlap canvas. */
bool Port_TitleCap_ExpandFrame(uint32_t* canvas, int width, int height, int pitch,
                               int native_x, int native_y, const uint32_t* margins,
                               int margins_pitch, uint32_t fill);

/* Draw only the added margins of a validated native title heading. Canvas
 * pixels in the native rectangle are never read or modified. The background
 * must already be composited. background_layer is the GBA second-target bit
 * (0..3 for BG0..BG3, 5 for backdrop) represented by that margin. Frame
 * points to the real frame-0 ROM data. */
bool Port_TitleCap_Draw(uint32_t* canvas, int width, int height, int pitch,
                        int native_x, int native_y, int heading_x, int heading_y,
                        const uint8_t* frame, size_t frame_size,
                        const uint8_t* vram, size_t vram_size,
                        const uint16_t* obj_palette, const uint8_t* io, unsigned background_layer,
                        const uint8_t* (*io_for_line)(int));
bool Port_TitleCap_CanDraw(int width, int height, int pitch, int native_x, int native_y,
                           int heading_x, int heading_y, const uint8_t* frame, size_t frame_size,
                           size_t vram_size, const uint8_t* io);

#ifdef __cplusplus
}
#endif

#endif
