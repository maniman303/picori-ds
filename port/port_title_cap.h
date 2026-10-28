#ifndef PORT_TITLE_CAP_H
#define PORT_TITLE_CAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Draw only the added margins of a validated native title heading. Canvas
 * pixels in the native rectangle are never read or modified. The background
 * must already be composited. Frame points to the real frame-0 ROM data. */
bool Port_TitleCap_Draw(uint32_t* canvas, int width, int height, int pitch,
                        int native_x, int native_y, int heading_x, int heading_y,
                        const uint8_t* frame, size_t frame_size,
                        const uint8_t* vram, size_t vram_size,
                        const uint16_t* obj_palette, const uint8_t* io,
                        const uint8_t* (*io_for_line)(int));
bool Port_TitleCap_CanDraw(int width, int height, int pitch, int native_x, int native_y,
                           int heading_x, int heading_y, const uint8_t* frame, size_t frame_size,
                           size_t vram_size, const uint8_t* io);

#ifdef __cplusplus
}
#endif

#endif
