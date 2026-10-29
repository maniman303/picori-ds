/* Inspect the real visible OBJ-window mask from a static PPU2 capture. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu/mode1.h"
#include "ppu_snapshot.h"
#include "virtuappu.h"

extern __thread uint8_t virtuappu_mode1_obj_window[MODE1_GBA_WIDTH];
static uint8_t io[0x400], vram[0x18000];
static uint16_t bgpal[256], objpal[256], oam[512];
static uint32_t pixels[MODE1_GBA_WIDTH];
static uint8_t priority[MODE1_GBA_WIDTH];

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    FILE* file = fopen(argv[1], "rb");
    if (!file) return 2;
    VirtuaPPUMode1GbaMemory mem = {io, vram, bgpal, objpal, oam};
    PPUMemory geometry = {0};
    int ok = virtuappu_snapshot_read(file, &mem, &geometry);
    fclose(file);
    if (!ok || !(io[1] & 0x80)) return 2;
    virtuappu_mode1_bind_gba_memory(&mem);
    virtuappu_mode1_set_frame_geometry(&geometry);
    virtuappu_mode1_render_frame(&geometry);
    unsigned total = 0, lines = 0;
    for (int y = 0; y < 160; ++y) {
        virtuappu_mode1_render_obj_line(y, (io[0] & 0x40) != 0, pixels, priority);
        unsigned count = 0;
        for (int x = 0; x < 240; ++x) count += virtuappu_mode1_obj_window[x] != 0;
        total += count; lines += count != 0;
    }
    printf("visible OBJ-window mask: %u pixels across %u lines\n", total, lines);
    return total ? 0 : 1;
}
