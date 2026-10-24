#pragma once

/* Standalone PPU captures, not save states. Never dump PPUMemory's C layout.
 * PPU1: magic, five LE u32 region sizes, region data (native 240x160 only).
 * PPU2: magic, five LE u32 region sizes, LE u32 width/height/pitch, region data.
 * IO/VRAM are byte arrays; palettes/OAM contain little-endian halfwords.
 * No scanline/shadow state is serialized; these are static-render fixtures. */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "virtuappu.h"
#include "cpu/mode1.h"

static inline bool virtuappu_snapshot_read_u32(FILE* file, uint32_t* value) {
    uint8_t bytes[4];
    if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes))
        return false;
    *value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
             ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
    return true;
}

static inline bool virtuappu_snapshot_write_u32(FILE* file, uint32_t value) {
    const uint8_t bytes[4] = { (uint8_t)value, (uint8_t)(value >> 8),
                               (uint8_t)(value >> 16), (uint8_t)(value >> 24) };
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
}

static inline bool virtuappu_snapshot_geometry_valid(uint32_t width, uint32_t height, uint32_t pitch) {
    return width > 0 && width <= MODE1_GBA_WIDTH && height > 0 && height <= MODE1_MAX_RENDER_HEIGHT &&
           pitch >= width && pitch <= VIRTUAPPU_MAX_FRAME_WIDTH &&
           (uint64_t)pitch * height <= VIRTUAPPU_FRAME_BUFFER_SIZE;
}

static inline bool virtuappu_snapshot_read_halfwords(FILE* file, uint16_t* out, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        uint8_t bytes[2];
        if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes))
            return false;
        out[i] = (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
    }
    return true;
}

static inline bool virtuappu_snapshot_write_halfwords(FILE* file, const uint16_t* in, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        const uint8_t bytes[2] = { (uint8_t)in[i], (uint8_t)(in[i] >> 8) };
        if (fwrite(bytes, 1, sizeof(bytes), file) != sizeof(bytes))
            return false;
    }
    return true;
}

static inline bool virtuappu_snapshot_read(FILE* file, const VirtuaPPUMode1GbaMemory* memory, PPUMemory* geometry) {
    static const uint32_t sizes[5] = { 0x400, 0x18000, 0x200, 0x200, 0x400 };
    char magic[4];
    if (fread(magic, 1, sizeof(magic), file) != sizeof(magic))
        return false;
    const bool legacy = memcmp(magic, "PPU1", 4) == 0;
    if (!legacy && memcmp(magic, "PPU2", 4) != 0)
        return false;
    for (size_t i = 0; i < 5; ++i) {
        uint32_t size;
        if (!virtuappu_snapshot_read_u32(file, &size) || size != sizes[i])
            return false;
    }
    uint32_t width = 240, height = 160, pitch = 240;
    if (!legacy && (!virtuappu_snapshot_read_u32(file, &width) ||
                    !virtuappu_snapshot_read_u32(file, &height) ||
                    !virtuappu_snapshot_read_u32(file, &pitch)))
        return false;
    if (!virtuappu_snapshot_geometry_valid(width, height, pitch))
        return false;
    if (fread(memory->io_mem, 1, sizes[0], file) != sizes[0] ||
        fread(memory->vram, 1, sizes[1], file) != sizes[1] ||
        !virtuappu_snapshot_read_halfwords(file, memory->bg_palette, 256) ||
        !virtuappu_snapshot_read_halfwords(file, memory->obj_palette, 256) ||
        !virtuappu_snapshot_read_halfwords(file, memory->oam_mem, 512) ||
        fgetc(file) != EOF || ferror(file))
        return false;
    memset(geometry, 0, sizeof(*geometry));
    geometry->frame_width = (uint16_t)width;
    geometry->frame_height = (uint16_t)height;
    geometry->frame_pitch = (uint16_t)pitch;
    const unsigned gba_mode = memory->io_mem[0] & 7;
    geometry->mode = (gba_mode == 1 || gba_mode == 2) ? 2 : 1;
    return true;
}

static inline bool virtuappu_snapshot_write(FILE* file, const VirtuaPPUMode1GbaMemory* memory,
                                            const PPUMemory* geometry) {
    static const uint32_t sizes[5] = { 0x400, 0x18000, 0x200, 0x200, 0x400 };
    const uint32_t width = geometry->frame_width ? geometry->frame_width : MODE1_GBA_WIDTH;
    const uint32_t height = geometry->frame_height ? geometry->frame_height : 160;
    const uint32_t pitch = geometry->frame_pitch ? geometry->frame_pitch : width;
    if (!virtuappu_snapshot_geometry_valid(width, height, pitch) || fwrite("PPU2", 1, 4, file) != 4)
        return false;
    for (size_t i = 0; i < 5; ++i) {
        if (!virtuappu_snapshot_write_u32(file, sizes[i]))
            return false;
    }
    return virtuappu_snapshot_write_u32(file, width) && virtuappu_snapshot_write_u32(file, height) &&
           virtuappu_snapshot_write_u32(file, pitch) &&
           fwrite(memory->io_mem, 1, sizes[0], file) == sizes[0] &&
           fwrite(memory->vram, 1, sizes[1], file) == sizes[1] &&
           virtuappu_snapshot_write_halfwords(file, memory->bg_palette, 256) &&
           virtuappu_snapshot_write_halfwords(file, memory->obj_palette, 256) &&
           virtuappu_snapshot_write_halfwords(file, memory->oam_mem, 512);
}
