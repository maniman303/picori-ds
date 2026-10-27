#ifndef PORT_HDMA_H
#define PORT_HDMA_H

/*
 * Software simulation of GBA HBlank-triggered DMA channels.
 *
 * On hardware, a DMA configured with DMA_START_HBLANK | DMA_REPEAT transfers
 * `count` units from src to dest at every HBlank. TMC uses this for the iris
 * circle / window effects (per-scanline WIN0H).
 *
 * The host PPU renders frames as a single batch, so we drive HDMA from the
 * VirtuaPPU mode-1 pre-line callback: one transfer per scanline.
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void port_hdma_register(int channel, const void* src, void* dest, uint16_t cnt_h, uint16_t count);
int port_hdma_has_active_channels(void);
enum {
    PORT_HDMA_BG_CONTROL = 1u << 0,
    PORT_HDMA_BG_SCROLL = 1u << 1,
    PORT_HDMA_AFFINE = 1u << 2,
    PORT_HDMA_WINDOW = 1u << 3,
    PORT_HDMA_BLEND = 1u << 4,
    PORT_HDMA_OTHER = 1u << 5,
    PORT_HDMA_FIXED_SOURCE = 1u << 6,
    PORT_HDMA_INCREMENTING_SOURCE = 1u << 7
};
/* Classify the entire 160-transfer destination footprint, not only its first
 * row. Unknown/out-of-IO writes remain OTHER. Source flags describe pointer
 * mode; they do not by themselves authorize a continuation. */
uint32_t port_hdma_classify(const void* io, size_t io_size);
/* A fixed halfword source written to BLDY can be continued without consuming
 * another GBA table entry. Any other channel, drifting destination or source
 * alias to a destination or an enabled window fails closed. io points to the
 * 1 KiB register file. */
int port_hdma_can_continue_brightness(const void* io, size_t io_size);
/* 1 if any ACTIVE channel's per-line destination range overlaps [lo, hi).
 * Used by the PPU glue to strobe the affine BG2X/BG2Y reference latch: a
 * write EVENT must reload the internal reference even when the written
 * value is unchanged (constant-value HDMA pins the layer on hardware). */
int port_hdma_dest_overlaps(const void* lo, const void* hi);
void port_hdma_unregister(int channel);
void port_hdma_step_line(int line);
void port_hdma_vblank_reset(void);

#ifdef __cplusplus
}
#endif

#endif
