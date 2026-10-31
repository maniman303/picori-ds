/*
 * Part of the The Minish Cap PC port — GPL-3.0-or-later.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Software GBA PPU, vendored as first-party port source. Derived from
 * VirtuaPPU by Mathéo Vignaud (https://github.com/MatheoVignaud/VirtuaPPU,
 * commit 5cf5e99) and incorporating this project's 15 accuracy/portability
 * patches (formerly port/patches/viruappu-*.patch; preserved in git history).
 * Maintained here directly — not kept in sync with upstream.
 */

#include "cpu/mode1.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "virtuappu.h"

virtuappu_mode1_pre_line_fn virtuappu_mode1_pre_line_callback = NULL;
bool virtuappu_mode1_continue_constant_hdma = false;

typedef struct Mode1TilemapEntry {
    uint16_t raw;
} Mode1TilemapEntry;

/* Widescreen Option A — port-side shadow tilemap pointers (declared in
 * include/cpu/mode1.h, populated by the PC port). NULL => no widescreen
 * reveal for that BG; render_text_bg_line then clips at MODE1_GBA_BG_CLIP_X.
 * Other layers and the backdrop still composite normally. */
uint16_t* virtuappu_mode1_ws_shadow[MODE1_GBA_BG_COUNT] = { NULL, NULL, NULL, NULL };
int virtuappu_mode1_ws_shadow_base_tile[MODE1_GBA_BG_COUNT] = { 0, 0, 0, 0 };
int virtuappu_mode1_ws_shadow_start_x[MODE1_GBA_BG_COUNT] = {
    MODE1_GBA_BG_CLIP_X, MODE1_GBA_BG_CLIP_X, MODE1_GBA_BG_CLIP_X, MODE1_GBA_BG_CLIP_X
};
int virtuappu_mode1_ws_hud_right_anchor = 0;
int virtuappu_mode1_ws_hud_shift_y = 0;
/* Widescreen message-box centering (see mode1.h). All zero = inactive. */
int virtuappu_mode1_ws_msg_shift = 0;
int virtuappu_mode1_ws_msg_shift_y = 0;
int virtuappu_mode1_ws_msg_x0 = 0;
int virtuappu_mode1_ws_msg_x1 = 0;
int virtuappu_mode1_ws_msg_y0 = 0;
int virtuappu_mode1_ws_msg_y1 = 0;

/* View-dependent charset groups (PC port). Some rooms stream several tileset
 * "groups" through the same VRAM span and choose one from the camera region.
 * A taller aperture can show two regions at once, which cannot be resident in
 * GBA VRAM. The port keeps the complete group charsets in host memory and
 * publishes a per-tilemap-cell group id; the tile fetch below then reads the
 * matching charset and palette without swapping VRAM or changing composition.
 * group 0xFF = native VRAM + native palette (inert default). */
static const uint8_t* mode1_group_cells;
static int mode1_group_cols;
static int mode1_group_rows;
static const uint32_t* mode1_group_luts;
static int mode1_group_count;
static const uint8_t* mode1_group_chars;
static int mode1_group_char_stride;
static unsigned mode1_group_bg_mask;
static const uint8_t* mode1_group_shadow_cells;
static int mode1_group_shadow_cols;
static int mode1_group_shadow_rows;

void virtuappu_mode1_bind_bg_group_cells(const uint8_t* cells, int cols, int rows,
                                          const uint32_t* palette_luts, int palette_count,
                                          const uint8_t* chars, int char_stride,
                                          unsigned bg_mask) {
    if (cells == NULL || palette_luts == NULL || chars == NULL ||
        cols < 1 || cols > 64 || rows < 1 || rows > 64 ||
        palette_count < 1 || palette_count > 8 ||
        char_stride < 0xC000 || (bg_mask & ~0xFu) != 0u) {
        virtuappu_mode1_clear_bg_group_cells();
        return;
    }
    mode1_group_cells = cells;
    mode1_group_cols = cols;
    mode1_group_rows = rows;
    mode1_group_luts = palette_luts;
    mode1_group_count = palette_count;
    mode1_group_chars = chars;
    mode1_group_char_stride = char_stride;
    mode1_group_bg_mask = bg_mask;
}

void virtuappu_mode1_bind_bg_group_shadow(const uint8_t* cells, int cols, int rows) {
    if (cells == NULL || cols < 1 || cols > 64 || rows < 1 || rows > 64) {
        virtuappu_mode1_clear_bg_group_shadow();
        return;
    }
    mode1_group_shadow_cells = cells;
    mode1_group_shadow_cols = cols;
    mode1_group_shadow_rows = rows;
}

void virtuappu_mode1_clear_bg_group_shadow(void) {
    mode1_group_shadow_cells = NULL;
    mode1_group_shadow_cols = 0;
    mode1_group_shadow_rows = 0;
}

void virtuappu_mode1_clear_bg_group_cells(void) {
    mode1_group_cells = NULL;
    mode1_group_luts = NULL;
    mode1_group_chars = NULL;
    mode1_group_bg_mask = 0;
    virtuappu_mode1_clear_bg_group_shadow();
}

typedef struct Mode1OAMAttr {
    uint16_t attr0;
    uint16_t attr1;
    uint16_t attr2;
} Mode1OAMAttr;

typedef enum Mode1BlendEffect {
    MODE1_BLEND_NONE = 0,
    MODE1_BLEND_ALPHA = 1,
    MODE1_BLEND_BRIGHTEN = 2,
    MODE1_BLEND_DARKEN = 3
} Mode1BlendEffect;

/* ---------------------------------------------------------------------------
 * PPU module state & threading contract (single reference).
 *
 * Bound GBA memory:
 *   mode1_default_*        backing buffers used until the engine binds real
 *                          memory, or when a NULL pointer is bound (see
 *                          virtuappu_mode1_bind_gba_memory — it validates each
 *                          pointer and falls back to these defaults).
 *   mode1_memory           the live binding (engine arrays or the defaults).
 *   mode1_frame_width/_pitch  render geometry (set per frame by
 *                          virtuappu_mode1_set_frame_geometry).
 *
 * Per-frame render threading (virtuappu_mode1_render_frame is OpenMP-parallel
 * over scanlines):
 *   TLS (VPPU_TLS, per render thread): io_thread_override, obj_semitrans,
 *       obj_window. Each thread owns a whole scanline; these must NOT be shared.
 *       IO regs are read via virtuappu_mode1_io_read16/32, which honour the
 *       per-thread override snapshot — never read mode1_memory.io_mem directly
 *       in render code or you get the wrong scanline's registers.
 *   SHARED, read-only during render (set once per frame by the port before the
 *       render): obj_clip_* (swamp sink), ws_shadow* (widescreen Option A),
 *       and the precomputed affine reference arrays.
 *
 * OOB safety: bound pointers are trusted for size; every VRAM/OAM/palette
 * access is range-clamped against the MODE1_*_SIZE constants at use sites, so
 * degraded/short room data can't read out of bounds.
 * ------------------------------------------------------------------------- */
static uint8_t mode1_default_io_mem[MODE1_IO_MEM_SIZE];
static uint8_t mode1_default_vram[MODE1_VRAM_SIZE];
static uint16_t mode1_default_bg_palette[MODE1_PALETTE_COLORS];
static uint16_t mode1_default_obj_palette[MODE1_PALETTE_COLORS];
static uint16_t mode1_default_oam_mem[MODE1_OAM_HALFWORDS];

static VirtuaPPUMode1GbaMemory mode1_memory = { mode1_default_io_mem, mode1_default_vram, mode1_default_bg_palette,
                                                mode1_default_obj_palette, mode1_default_oam_mem };

static int mode1_frame_width = MODE1_GBA_WIDTH;
static uint8_t mode1_frame_io_snapshots[MODE1_GBA_HEIGHT][MODE1_IO_MEM_SIZE];
static bool mode1_frame_has_io_snapshots;

const uint8_t* virtuappu_mode1_frame_io_for_line(int line) {
    if (!mode1_frame_has_io_snapshots) return mode1_memory.io_mem;
    if (line < 0) line = 0;
    if (line >= MODE1_GBA_HEIGHT) line = MODE1_GBA_HEIGHT - 1;
    return mode1_frame_io_snapshots[line];
}
static int mode1_frame_pitch = MODE1_GBA_WIDTH;
static int mode1_frame_height = MODE1_GBA_HEIGHT;
/* Host-visible canvas/HUD mask for the live-world margin fade. One byte per
 * rendered pixel, indexed with the current frame width. */
static uint8_t mode1_ui_mask[MODE1_MAX_RENDER_HEIGHT * MODE1_GBA_WIDTH];
static int mode1_ui_mask_width;
/* Optional live-world aperture; disabled keeps every legacy path byte-exact. */
static int mode1_world_x, mode1_world_y, mode1_world_w, mode1_world_h;
static bool mode1_world_enabled;
static const VirtuaPPUObjPosition* mode1_obj_positions;
static int mode1_oam_x(Mode1OAMAttr attr);
static int mode1_oam_y(Mode1OAMAttr attr);

void virtuappu_mode1_bind_obj_positions(const VirtuaPPUObjPosition* positions) {
    mode1_obj_positions = positions;
}

bool virtuappu_mode1_has_obj_positions(void) {
    return mode1_obj_positions != NULL;
}

static bool mode1_obj_position(int index, Mode1OAMAttr attr, int viewport_width, int* x, int* y,
                               bool* canvas) {
    *canvas = false;
    if (mode1_obj_positions != NULL) {
        const VirtuaPPUObjPosition* position = &mode1_obj_positions[index];
        if ((position->flags & VIRTUAPPU_OBJ_POSITION_VALID) &&
            position->attr0 == attr.attr0 && position->attr1 == attr.attr1) {
            if (position->flags & VIRTUAPPU_OBJ_POSITION_HIDDEN)
                return false;
            *canvas = (position->flags & VIRTUAPPU_OBJ_POSITION_CANVAS) != 0;
            *x = position->x;
            *y = position->y;
            return true;
        }
        /* Beyond 512, raw OAM cannot distinguish a native negative X from
         * a wrapped positive coordinate. Untagged entries stay native. */
        if (viewport_width > 512)
            viewport_width = MODE1_GBA_BG_CLIP_X;
    }
    *x = mode1_oam_x(attr);
    if (*x >= viewport_width)
        *x -= 512;
    *y = mode1_oam_y(attr);
    if (*y >= MODE1_GBA_HEIGHT)
        *y -= 256;
    return true;
}

void virtuappu_mode1_set_frame_geometry(const PPUMemory* ppu) {
    int width = MODE1_GBA_WIDTH;
    int pitch = MODE1_GBA_WIDTH;
    int height = ppu != NULL && ppu->frame_height != 0u ? ppu->frame_height : MODE1_GBA_HEIGHT;
    if (height > MODE1_MAX_RENDER_HEIGHT)
        height = MODE1_MAX_RENDER_HEIGHT;
    /* No HDMA continuation is implied by extra framebuffer storage. Keep
     * unknown raster effects in a native-height aperture until classified. */
    if (height > MODE1_GBA_HEIGHT &&
        ((virtuappu_mode1_pre_line_callback != NULL && !virtuappu_mode1_continue_constant_hdma) ||
         /* Static WIN0/WIN1/OBJWIN geometry is native too. The absence of an
          * HDMA callback does not establish a full-canvas window policy. */
         (mode1_memory.io_mem[MODE1_IO_DISPCNT + 1] & 0xe0u) != 0u))
        height = MODE1_GBA_HEIGHT;

    if (ppu != NULL && ppu->frame_width != 0u) {
        width = (int)ppu->frame_width;
    }
    if (width < 1) {
        width = 1;
    } else if (width > MODE1_GBA_WIDTH) {
        width = MODE1_GBA_WIDTH;
    }

    if (ppu != NULL && ppu->frame_pitch != 0u) {
        pitch = (int)ppu->frame_pitch;
    }
    if (pitch < width) {
        pitch = width;
    } else if (pitch > VIRTUAPPU_MAX_FRAME_WIDTH) {
        pitch = VIRTUAPPU_MAX_FRAME_WIDTH;
    }

    mode1_frame_width = width;
    mode1_frame_pitch = pitch;
    mode1_frame_height = height;
    if (mode1_world_enabled &&
        (mode1_world_x + mode1_world_w > width || mode1_world_y + mode1_world_h > height))
        mode1_world_enabled = false;
}

int virtuappu_mode1_frame_width(void) {
    return mode1_frame_width;
}

int virtuappu_mode1_frame_pitch(void) {
    return mode1_frame_pitch;
}

int virtuappu_mode1_frame_height(void) {
    return mode1_frame_height;
}

void virtuappu_mode1_set_world_rect(int x, int y, int width, int height) {
    /* Store the published rectangle; set_frame_geometry decides whether it
     * fits the final raster (callers may publish before the next geometry). */
    if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
        x > VIRTUAPPU_MAX_FRAME_WIDTH || y > VIRTUAPPU_MAX_FRAME_HEIGHT ||
        width > VIRTUAPPU_MAX_FRAME_WIDTH - x || height > VIRTUAPPU_MAX_FRAME_HEIGHT - y) {
        mode1_world_enabled = false;
        return;
    }
    mode1_world_x = x;
    mode1_world_y = y;
    mode1_world_w = width;
    mode1_world_h = height;
    mode1_world_enabled = true;
}

void virtuappu_mode1_clear_world_rect(void) {
    mode1_world_enabled = false;
}

const uint8_t* virtuappu_mode1_frame_ui_mask(int* width) {
    if (width != NULL) *width = mode1_ui_mask_width;
    return mode1_ui_mask;
}

static const uint8_t mode1_obj_widths[3][4] = { { 8, 16, 32, 64 }, { 16, 32, 32, 64 }, { 8, 8, 16, 32 } };

static const uint8_t mode1_obj_heights[3][4] = { { 8, 16, 32, 64 }, { 8, 8, 16, 32 }, { 16, 32, 32, 64 } };

static uint16_t mode1_tile_index(Mode1TilemapEntry entry) {
    return entry.raw & 0x03FFu;
}

static bool mode1_tile_hflip(Mode1TilemapEntry entry) {
    return ((entry.raw >> 10u) & 1u) != 0u;
}

static bool mode1_tile_vflip(Mode1TilemapEntry entry) {
    return ((entry.raw >> 11u) & 1u) != 0u;
}

static uint8_t mode1_tile_palette(Mode1TilemapEntry entry) {
    return (uint8_t)((entry.raw >> 12u) & 0x0Fu);
}

static bool mode1_oam_affine(Mode1OAMAttr attr) {
    return ((attr.attr0 >> 8u) & 1u) != 0u;
}

static bool mode1_oam_double_size(Mode1OAMAttr attr) {
    return mode1_oam_affine(attr) && (((attr.attr0 >> 9u) & 1u) != 0u);
}

static bool mode1_oam_hidden(Mode1OAMAttr attr) {
    return !mode1_oam_affine(attr) && (((attr.attr0 >> 9u) & 1u) != 0u);
}

static bool mode1_oam_bpp8(Mode1OAMAttr attr) {
    return ((attr.attr0 >> 13u) & 1u) != 0u;
}

static int mode1_oam_y(Mode1OAMAttr attr) {
    return attr.attr0 & 0xFF;
}

static uint8_t mode1_oam_shape(Mode1OAMAttr attr) {
    return (uint8_t)((attr.attr0 >> 14u) & 3u);
}

static int mode1_oam_x(Mode1OAMAttr attr) {
    return attr.attr1 & 0x1FF;
}

static bool mode1_oam_hflip(Mode1OAMAttr attr) {
    return !mode1_oam_affine(attr) && (((attr.attr1 >> 12u) & 1u) != 0u);
}

static bool mode1_oam_vflip(Mode1OAMAttr attr) {
    return !mode1_oam_affine(attr) && (((attr.attr1 >> 13u) & 1u) != 0u);
}

static uint8_t mode1_oam_affine_index(Mode1OAMAttr attr) {
    return (uint8_t)((attr.attr1 >> 9u) & 0x1Fu);
}

static uint8_t mode1_oam_size(Mode1OAMAttr attr) {
    return (uint8_t)((attr.attr1 >> 14u) & 3u);
}

static uint16_t mode1_oam_tile_index(Mode1OAMAttr attr) {
    return attr.attr2 & 0x03FFu;
}

static uint8_t mode1_oam_priority(Mode1OAMAttr attr) {
    return (uint8_t)((attr.attr2 >> 10u) & 3u);
}

static uint8_t mode1_oam_palette(Mode1OAMAttr attr) {
    return (uint8_t)((attr.attr2 >> 12u) & 0x0Fu);
}

/* OBJ mode field (attr0 bits 10-11): 0 = normal, 1 = semi-transparent
 * (forced alpha-blend 1st target), 2 = OBJ window, 3 = prohibited. */
static uint8_t mode1_oam_mode(Mode1OAMAttr attr) {
    return (uint8_t)((attr.attr0 >> 10u) & 3u);
}

/* OBJ mosaic enable (attr0 bit 12). Block size comes from MOSAIC (0x4C)
 * bits 8-11 (h-1) / 12-15 (v-1). */
static bool mode1_oam_mosaic(Mode1OAMAttr attr) {
    return ((attr.attr0 >> 12u) & 1u) != 0u;
}

static bool mode1_is_first_target(uint16_t bldcnt, int layer_id) {
    return ((bldcnt >> layer_id) & 1u) != 0u;
}

static bool mode1_is_second_target(uint16_t bldcnt, int layer_id) {
    return ((bldcnt >> (layer_id + 8)) & 1u) != 0u;
}

static uint32_t mode1_alpha_blend(uint32_t top_abgr, uint32_t bottom_abgr, int eva, int evb) {
    /* VIRTUAPPU_BLEND_5BIT: GBA alpha blend is defined on 5-bit channels
     * (GBATEK: I = MIN(31, I1*eva + I2*evb), eva/evb already clamped 0..16 by
     * the caller). The framebuffer is 8-bit (palette value <<3), so recover the
     * 5-bit intensity (>>3), blend with a truncating >>4 exactly as the
     * hardware does, clamp to 31, then re-expand (<<3). This reproduces the
     * GBA's quantised blend banding instead of a smoother 8-bit-domain blend. */
    int top_r = (int)((top_abgr >> 0u) & 0xFFu) >> 3;
    int top_g = (int)((top_abgr >> 8u) & 0xFFu) >> 3;
    int top_b = (int)((top_abgr >> 16u) & 0xFFu) >> 3;
    int bottom_r = (int)((bottom_abgr >> 0u) & 0xFFu) >> 3;
    int bottom_g = (int)((bottom_abgr >> 8u) & 0xFFu) >> 3;
    int bottom_b = (int)((bottom_abgr >> 16u) & 0xFFu) >> 3;
    int out_r = (top_r * eva + bottom_r * evb) >> 4;
    int out_g = (top_g * eva + bottom_g * evb) >> 4;
    int out_b = (top_b * eva + bottom_b * evb) >> 4;

    if (out_r > 31) {
        out_r = 31;
    }
    if (out_g > 31) {
        out_g = 31;
    }
    if (out_b > 31) {
        out_b = 31;
    }

    return 0xFF000000u | ((uint32_t)(out_b << 3) << 16u) | ((uint32_t)(out_g << 3) << 8u) | (uint32_t)(out_r << 3);
}

static uint32_t mode1_brighten(uint32_t abgr, int evy) {
    /* GBA brightness-increase on 5-bit channels (GBATEK):
     * I = I + (31-I)*evy/16, truncating; evy already clamped 0..16. */
    int r = (int)((abgr >> 0u) & 0xFFu) >> 3;
    int g = (int)((abgr >> 8u) & 0xFFu) >> 3;
    int b = (int)((abgr >> 16u) & 0xFFu) >> 3;

    r = r + (((31 - r) * evy) >> 4);
    g = g + (((31 - g) * evy) >> 4);
    b = b + (((31 - b) * evy) >> 4);

    if (r > 31) {
        r = 31;
    }
    if (g > 31) {
        g = 31;
    }
    if (b > 31) {
        b = 31;
    }

    return 0xFF000000u | ((uint32_t)(b << 3) << 16u) | ((uint32_t)(g << 3) << 8u) | (uint32_t)(r << 3);
}

static uint32_t mode1_darken(uint32_t abgr, int evy) {
    /* GBA brightness-decrease on 5-bit channels (GBATEK):
     * I = I - I*evy/16, truncating; evy already clamped 0..16. */
    int r = (int)((abgr >> 0u) & 0xFFu) >> 3;
    int g = (int)((abgr >> 8u) & 0xFFu) >> 3;
    int b = (int)((abgr >> 16u) & 0xFFu) >> 3;

    r -= ((r * evy) >> 4);
    g -= ((g * evy) >> 4);
    b -= ((b * evy) >> 4);

    if (r < 0) {
        r = 0;
    }
    if (g < 0) {
        g = 0;
    }
    if (b < 0) {
        b = 0;
    }

    return 0xFF000000u | ((uint32_t)(b << 3) << 16u) | ((uint32_t)(g << 3) << 8u) | (uint32_t)(r << 3);
}

void virtuappu_mode1_bind_gba_memory(const VirtuaPPUMode1GbaMemory* memory) {
    mode1_frame_has_io_snapshots = false;
    /* Validate at bind: any missing pointer falls back to a (zeroed) default
     * buffer so render never dereferences NULL. A fallback means the engine's
     * memory wasn't wired up — surface it once instead of silently rendering a
     * blank/garbage frame. (Per-access range clamps remain the OOB guard for
     * the bound buffers themselves; see the module-state contract above.) */
    const bool fell_back = memory == NULL || memory->io_mem == NULL || memory->vram == NULL ||
                           memory->bg_palette == NULL || memory->obj_palette == NULL || memory->oam_mem == NULL;

    mode1_memory.io_mem = (memory != NULL && memory->io_mem != NULL) ? memory->io_mem : mode1_default_io_mem;
    mode1_memory.vram = (memory != NULL && memory->vram != NULL) ? memory->vram : mode1_default_vram;
    mode1_memory.bg_palette =
        (memory != NULL && memory->bg_palette != NULL) ? memory->bg_palette : mode1_default_bg_palette;
    mode1_memory.obj_palette =
        (memory != NULL && memory->obj_palette != NULL) ? memory->obj_palette : mode1_default_obj_palette;
    mode1_memory.oam_mem = (memory != NULL && memory->oam_mem != NULL) ? memory->oam_mem : mode1_default_oam_mem;

    if (fell_back) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            fprintf(stderr, "[ppu] WARNING: bind_gba_memory received a NULL pointer; using zeroed "
                            "default buffer(s). The PPU will render blank until real GBA memory is "
                            "bound.\n");
        }
    }
}

void virtuappu_mode1_get_bound_gba_memory(VirtuaPPUMode1GbaMemory* memory) {
    if (memory == NULL) {
        return;
    }

    *memory = mode1_memory;
}

/* PORT_PARALLEL_RENDER: when the per-line IO snapshot pass populates
 * a thread-local override (set by the parallel render loop in
 * virtuappu_mode1_render_frame), read from the snapshot instead of the
 * global io_mem. Lets per-scanline rendering be parallelized over OpenMP
 * threads while still seeing the correct HDMA-mutated register state
 * (BG SCROLL, BLDY, etc.) for that scanline. NULL on the main / setup
 * thread keeps the existing single-threaded behavior intact. */
#ifdef TMC_N64
/* N64: single-threaded (no OpenMP) and bare-metal (no TLS). __thread compiles to
 * RDHWR $29, which the VR4300 (MIPS III) doesn't implement -> Reserved Instruction.
 * Single-threaded rendering makes plain file-scope storage correct here. */
#define VPPU_TLS
#else
#define VPPU_TLS __thread
#endif
VPPU_TLS const uint8_t* virtuappu_mode1_io_thread_override = NULL;

/* Per-scanline scratch (one slot per screen column) marking which OBJ pixels
 * came from a semi-transparent OBJ (attr0 mode 1). Written by render_obj_line
 * for the winning OBJ pixel, read by composite_line to force alpha blending —
 * GBA-accurate (a mode-1 OBJ is an unconditional blend 1st target). __thread so
 * each OpenMP render thread, which processes a whole scanline at a time, has
 * its own copy; render_obj_line clears it at the top of every line. */
VPPU_TLS uint8_t virtuappu_mode1_obj_semitrans[MODE1_GBA_WIDTH];
/* Per-line canvas tag for the OBJ pixel that won the priority test. Used by
 * the host margin fade to keep HUD sprites out of the live-world border. */
VPPU_TLS uint8_t virtuappu_mode1_obj_canvas[MODE1_GBA_WIDTH];

/* Per-scanline OBJ-window mask (attr0 mode 2). A mode-2 OBJ is INVISIBLE on
 * GBA — it doesn't draw colour; its opaque pixels just carve the OBJ window,
 * inside which WINOUT's high byte (WINOBJ) selects which layers show. Written
 * by render_obj_line, read by composite_line. __thread + cleared per line, as
 * with obj_semitrans. */
VPPU_TLS uint8_t virtuappu_mode1_obj_window[MODE1_GBA_WIDTH];

/* Per-pixel vertical OBJ clip (PC port "swamp sink"): the port marks which OAM
 * entries to clip (one byte per OAM index) and a waterline scanline; a marked
 * entry's pixels at line >= waterline are dropped, leaving everything else
 * untouched. SHARED (not VPPU_TLS): set once per frame by the port before the
 * render and read-only during the OpenMP-parallel scanline render. */
uint8_t virtuappu_mode1_obj_clip_mark[MODE1_GBA_OAM_COUNT];
int virtuappu_mode1_obj_clip_y;
int virtuappu_mode1_obj_clip_enable;

uint16_t virtuappu_mode1_io_read16(uint16_t offset) {
    const uint8_t* src = virtuappu_mode1_io_thread_override ? virtuappu_mode1_io_thread_override : mode1_memory.io_mem;
#ifdef TMC_N64
    return *(const uint16_t*)(src + offset); /* native: engine stores IO regs in host order (BE on N64) */
#else
    return (uint16_t)src[offset] | ((uint16_t)src[offset + 1u] << 8u);
#endif
}

uint32_t virtuappu_mode1_io_read32(uint16_t offset) {
    return (uint32_t)virtuappu_mode1_io_read16(offset) |
           ((uint32_t)virtuappu_mode1_io_read16((uint16_t)(offset + 2u)) << 16u);
}

/* Per-frame ABGR8888 palette lookup tables. The GBA palette is only 256 BG +
 * 256 OBJ entries and is frame-constant (the fade/brightness engine rewrites
 * palette RAM once per frame BEFORE render; the HDMA pre-line callback mutates
 * only IO registers, never palette RAM). So converting all 512 entries once per
 * frame and indexing them per pixel replaces the per-pixel rgb555->ABGR math
 * (5-6 shifts/masks/ORs) with a single load — a big win on the in-order A53,
 * with ZERO added branches. Built by PublishPaletteLuts before the parallel
 * scanline render; SHARED read-only during it (same contract as ws_shadow).
 * Byte-exact: LUT[i] == rgb555_to_abgr(palette[i]) by construction. */
static uint32_t mode1_bg_abgr_lut[MODE1_PALETTE_COLORS];
static uint32_t mode1_obj_abgr_lut[MODE1_PALETTE_COLORS];

uint32_t virtuappu_mode1_rgb555_to_abgr8888(uint16_t color) {
    uint8_t r = (uint8_t)((color & 0x1Fu) << 3u);
    uint8_t g = (uint8_t)(((color >> 5u) & 0x1Fu) << 3u);
    uint8_t b = (uint8_t)(((color >> 10u) & 0x1Fu) << 3u);

    return 0xFF000000u | ((uint32_t)b << 16u) | ((uint32_t)g << 8u) | (uint32_t)r;
}

/* Convert the current BG + OBJ palette RAM into the ABGR8888 LUTs. Call once
 * per frame after the palette is final (post pre-line callback) and before the
 * parallel scanline render. */
static void virtuappu_mode1_publish_palette_luts(void) {
    int i;
    for (i = 0; i < MODE1_PALETTE_COLORS; ++i) {
        mode1_bg_abgr_lut[i] = virtuappu_mode1_rgb555_to_abgr8888(mode1_memory.bg_palette[i]);
        mode1_obj_abgr_lut[i] = virtuappu_mode1_rgb555_to_abgr8888(mode1_memory.obj_palette[i]);
    }
}

static void mode1_render_text_bg_line_internal(int bg_index, int line, int render_width, int dst_x,
                                                  uint32_t* line_buffer, uint8_t* priority_buffer) {
    uint16_t bgcnt = virtuappu_mode1_io_read16((uint16_t)(MODE1_IO_BG0CNT + bg_index * 2));
    uint8_t priority = (uint8_t)(bgcnt & 3u);
    uint32_t char_base = (uint32_t)((bgcnt >> 2u) & 3u) * 0x4000u;
    bool mosaic_on = ((bgcnt >> 6u) & 1u) != 0u;
    bool bpp8 = ((bgcnt >> 7u) & 1u) != 0u;
    uint32_t screen_base = (uint32_t)((bgcnt >> 8u) & 0x1Fu) * 0x800u;
    uint16_t size_flag = (uint16_t)((bgcnt >> 14u) & 3u);
    int map_width_tiles = (size_flag & 1u) ? 64 : 32;
    int map_height_tiles = (size_flag & 2u) ? 64 : 32;
    int scroll_x = virtuappu_mode1_io_read16((uint16_t)(MODE1_IO_BG0HOFS + bg_index * 4)) & 0x1FF;
    int scroll_y = virtuappu_mode1_io_read16((uint16_t)(MODE1_IO_BG0VOFS + bg_index * 4)) & 0x1FF;
    uint16_t mosaic_reg = virtuappu_mode1_io_read16(MODE1_IO_MOSAIC);
    /* BG mosaic honored when BGCNT bit 6 is set (GBA-accurate; gate removed). */
    int mosaic_h = mosaic_on ? (int)((mosaic_reg & 0x0Fu) + 1u) : 1;
    int mosaic_v = mosaic_on ? (int)(((mosaic_reg >> 4u) & 0x0Fu) + 1u) : 1;
    /* N64 perf: map_{width,height}_tiles*8 is always 256 or 512 (pow2) and the
     * operands are non-negative, so `% (w)` == `& (w-1)` — avoids the R4300's
     * ~37-cyc idiv. The mosaic `/` is guarded to the (rare) mosaic-on case.
     * Identity-preserving on every host. */
    int eff_line = (mosaic_v == 1) ? line : (line / mosaic_v) * mosaic_v;
    int src_y = (eff_line + scroll_y) & (map_height_tiles * 8 - 1);
    int tile_row = src_y / 8;
    int pixel_y = src_y % 8;
    int x;
    const int frame_width = render_width;
    /* View-dependent charset group override (Minish Village). Only the tall
     * live-world path binds it; every other render keeps native VRAM. */
    const bool group_bg = mode1_group_cells != NULL && mode1_group_chars != NULL &&
                          (mode1_group_bg_mask & (1u << bg_index)) != 0;
    /* Widescreen Option A: 32-tile BGs have valid VRAM tile data only
     * within the native 240 px. Cull the line to the current visible frame
     * width, but keep the fixed framebuffer pitch separate for presentation. */
    int render_max_x = (map_width_tiles >= 64)
                           ? frame_width
                           : ((virtuappu_mode1_ws_shadow[bg_index] != NULL) ? frame_width : MODE1_GBA_BG_CLIP_X);
    if (render_max_x > frame_width)
        render_max_x = frame_width;
    const bool ws_shadow_active = (map_width_tiles < 64) && (virtuappu_mode1_ws_shadow[bg_index] != NULL);
    const int ws_shadow_base = virtuappu_mode1_ws_shadow_base_tile[bg_index];
    uint16_t* const ws_shadow = virtuappu_mode1_ws_shadow[bg_index];
    const bool ws_hud_right_anchor =
        (bg_index == 0) && (virtuappu_mode1_ws_hud_right_anchor != 0) && (frame_width > MODE1_GBA_BG_CLIP_X);
    /* The normal key and rupee HUD owns only these BG0 cells. Other BG0
     * content on the same scanlines must remain at its native position. */
    const int ws_hud_bit = line < 144 ? MODE1_WS_HUD_KEYS : MODE1_WS_HUD_RUPEES;
    const bool ws_hud_cells = ws_hud_right_anchor && scroll_x == 0 && scroll_y == 0 &&
                              line >= 128 && line < 160 &&
                              (virtuappu_mode1_ws_hud_right_anchor & ws_hud_bit);
    const int ws_hud_x0 = line < 144 ? 200 : 192;
    const int ws_hud_x1 = 232;
    const int ws_hud_shift = frame_width - MODE1_GBA_BG_CLIP_X;
    /* Message-box centering: on BG0 lines inside the published box band,
     * draw the box's native columns shifted right by ws_msg_shift and skip
     * them at their native position (see mode1.h). */
    const int ws_msg_shift = virtuappu_mode1_ws_msg_shift;
    const bool ws_msg_line = (bg_index == 0) && (ws_msg_shift != 0) && (frame_width > MODE1_GBA_BG_CLIP_X) &&
                             (line >= virtuappu_mode1_ws_msg_y0) && (line < virtuappu_mode1_ws_msg_y1);
    const int ws_msg_x0 = virtuappu_mode1_ws_msg_x0;
    const int ws_msg_x1 = virtuappu_mode1_ws_msg_x1;
    const int ws_msg_shift_y = virtuappu_mode1_ws_msg_shift_y;
    const int ws_msg_y0 = virtuappu_mode1_ws_msg_y0;
    const int ws_msg_y1 = virtuappu_mode1_ws_msg_y1;
    const bool ws_msg_vertical =
        (bg_index == 0) && (ws_msg_shift_y > 0) && (ws_msg_x1 > ws_msg_x0) &&
        (ws_msg_y1 > ws_msg_y0) && (ws_msg_x0 >= 0) && (ws_msg_x1 <= MODE1_GBA_BG_CLIP_X) &&
        (ws_msg_y0 >= 0) && (ws_msg_y1 <= MODE1_GBA_HEIGHT) &&
        (ws_msg_y0 + ws_msg_shift_y >= 0) &&
        (ws_msg_y1 + ws_msg_shift_y <= mode1_frame_height);
    const int ws_hud_shift_y = virtuappu_mode1_ws_hud_shift_y;
    const bool ws_hud_vertical = (bg_index == 0) && (frame_width > MODE1_GBA_BG_CLIP_X) &&
                                 ws_hud_right_anchor && scroll_x == 0 && scroll_y == 0 &&
                                 (ws_hud_shift_y > 0) &&
                                 (MODE1_GBA_HEIGHT + ws_hud_shift_y <= mode1_frame_height);
    const bool ws_msg_src_line =
        ws_msg_vertical && (line >= ws_msg_y0) && (line < ws_msg_y1);
    const bool ws_msg_dst_line =
        ws_msg_vertical && (line >= ws_msg_y0 + ws_msg_shift_y) &&
        (line < ws_msg_y1 + ws_msg_shift_y);
    const bool ws_hud_src_line =
        ws_hud_vertical && (line >= 128) && (line < MODE1_GBA_HEIGHT);
    const bool ws_hud_dst_line =
        ws_hud_vertical && (line >= 128 + ws_hud_shift_y) &&
        (line < MODE1_GBA_HEIGHT + ws_hud_shift_y);
    const bool ws_vertical_line =
        ws_msg_src_line || ws_msg_dst_line || ws_hud_src_line || ws_hud_dst_line;

    if (ws_hud_cells || ws_msg_line || ws_vertical_line) {
        render_max_x = frame_width;
    }
    /* N64 perf: the tilemap screen-entry read + its address math are constant
     * across a tile's 8 px; cache them and recompute only when the tile cell
     * (or the shadow/native source) changes. Byte-identical: the cached value
     * equals the per-pixel recompute. Line-constants hoisted out of the x-loop. */
    const int screen_block_y = tile_row / 32;
    const int local_row = tile_row % 32;
    const int blocks_per_row = map_width_tiles / 32;
    int bg_cache_key = -1;
    Mode1TilemapEntry bg_tile_entry;
    bg_tile_entry.raw = 0u;
    /* Per-tile derived values, recomputed only on a cache refill (tile change).
     * Constant across a tile's 8 px, so hoisting them out of the per-pixel body
     * removes ~5 ALU ops/pixel with zero added branches (A53-friendly). */
    bool bg_hflip = false;
    int bg_tpy = 0;
    uint32_t bg_row_base = 0u;
    size_t bg_pal_bank = 0u;
    const uint8_t* bg_char_base = mode1_memory.vram;
    uint32_t bg_char_limit = MODE1_VRAM_SIZE;
    const uint32_t* bg_color_lut = mode1_bg_abgr_lut;

    /* The per-pixel body is identical for the fast (native / no-remap) and the
     * widescreen-remap paths; keep it in one macro so the two loops can never
     * drift. `_sx` is the tilemap SAMPLE column; the loop var `x` is always the
     * destination column (and drives the x>=240 shadow/sentinel guards). */
#define MODE1_BG_PIXEL(_sx)                                                                                            \
    do {                                                                                                               \
        int eff_x = (mosaic_h == 1) ? (_sx) : ((_sx) / mosaic_h) * mosaic_h;                                           \
        int src_x = (eff_x + scroll_x) & (map_width_tiles * 8 - 1);                                                    \
        int tile_col = src_x / 8;                                                                                      \
        int pixel_x = src_x % 8;                                                                                       \
        int cache_use_shadow = (ws_shadow_active && x >= virtuappu_mode1_ws_shadow_start_x[bg_index]) ? 1 : 0;         \
        int cache_key = (tile_col << 1) | cache_use_shadow;                                                            \
        if (cache_key != bg_cache_key) {                                                                               \
            bg_cache_key = cache_key;                                                                                  \
            int group_cell = 0xFF;                                                                                     \
            if (cache_use_shadow) {                                                                                    \
                int shadow_idx = (tile_col - ws_shadow_base + 32) % 32;                                                \
                bg_tile_entry.raw = (shadow_idx < MODE1_WS_SHADOW_COLS)                                                \
                                        ? ws_shadow[(size_t)local_row * MODE1_WS_SHADOW_COLS + shadow_idx]             \
                                        : (uint16_t)0u;                                                                \
                if (mode1_group_shadow_cells != NULL && shadow_idx < mode1_group_shadow_cols) {                        \
                    group_cell = mode1_group_shadow_cells[(local_row % mode1_group_shadow_rows) *                    \
                                                              mode1_group_shadow_cols + shadow_idx];                   \
                }                                                                                                      \
            } else {                                                                                                   \
                int screen_block_x = tile_col / 32;                                                                    \
                int screen_block_index = screen_block_x + screen_block_y * blocks_per_row;                             \
                int local_col = tile_col % 32;                                                                         \
                uint32_t map_addr =                                                                                    \
                    screen_base + (uint32_t)screen_block_index * 0x800u + (uint32_t)(local_row * 32 + local_col) * 2u; \
                bg_tile_entry.raw =                                                                                    \
                    (uint16_t)mode1_memory.vram[map_addr] | ((uint16_t)mode1_memory.vram[map_addr + 1u] << 8u);        \
            }                                                                                                          \
            bg_hflip = mode1_tile_hflip(bg_tile_entry);                                                                \
            bg_tpy = mode1_tile_vflip(bg_tile_entry) ? (7 - pixel_y) : pixel_y;                                        \
            if (bpp8) {                                                                                                \
                bg_row_base = char_base + (uint32_t)mode1_tile_index(bg_tile_entry) * 64u + (uint32_t)bg_tpy * 8u;     \
            } else {                                                                                                   \
                bg_row_base = char_base + (uint32_t)mode1_tile_index(bg_tile_entry) * 32u + (uint32_t)bg_tpy * 4u;     \
            }                                                                                                          \
            bg_pal_bank = (size_t)mode1_tile_palette(bg_tile_entry) * 16u;                                             \
            bg_char_base = mode1_memory.vram;                                                                          \
            bg_char_limit = MODE1_VRAM_SIZE;                                                                           \
            bg_color_lut = mode1_bg_abgr_lut;                                                                          \
            if (group_bg && !cache_use_shadow) {                                                                       \
                const bool group_addr = bg_row_base < 0x4000u ||                                                       \
                                        (bg_row_base >= 0x8000u && bg_row_base < 0xC000u);                             \
                group_cell = group_addr                                                                                 \
                                 ? mode1_group_cells[(local_row % mode1_group_rows) * mode1_group_cols +               \
                                                     (tile_col % mode1_group_cols)]                                    \
                                 : 0xFF;                                                                               \
            }                                                                                                          \
            if (group_cell < mode1_group_count) {                                                                      \
                bg_char_base = mode1_group_chars + (size_t)group_cell * mode1_group_char_stride;                       \
                bg_char_limit = 0xC000u;                                                                               \
                bg_color_lut = mode1_group_luts + (size_t)group_cell * MODE1_PALETTE_COLORS;                           \
            }                                                                                                          \
        }                                                                                                              \
        int tile_pixel_x = bg_hflip ? (7 - pixel_x) : pixel_x;                                                         \
        uint8_t color_index;                                                                                           \
        if (bpp8) {                                                                                                    \
            uint32_t addr = bg_row_base + (uint32_t)tile_pixel_x;                                                      \
            color_index = (addr < bg_char_limit) ? bg_char_base[addr] : 0u;                                            \
        } else {                                                                                                       \
            uint32_t addr = bg_row_base + (uint32_t)(tile_pixel_x / 2);                                                \
            uint8_t packed = (addr < bg_char_limit) ? bg_char_base[addr] : 0u;                                         \
            color_index = (tile_pixel_x & 1) ? (packed >> 4u) : (packed & 0x0Fu);                                      \
        }                                                                                                              \
        if (color_index != 0u) {                                                                                       \
            size_t pal_idx = bpp8 ? (size_t)color_index : (bg_pal_bank + color_index);                                 \
            if (!(x >= virtuappu_mode1_ws_shadow_start_x[bg_index] && bg_color_lut == mode1_bg_abgr_lut &&             \
                  (mode1_memory.bg_palette[pal_idx] & 0x7FFFu) == 0x7C1Fu)) {                                           \
                line_buffer[dst_x + x] = bg_color_lut[pal_idx];                                                        \
                if (priority_buffer != NULL) {                                                                         \
                    priority_buffer[dst_x + x] = priority;                                                             \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

    /* Tall-canvas variant: the tilemap row is a per-pixel input because the
     * message window and lower-right HUD can be sampled from a different
     * native scanline. Only the 192-line canvases take this path. */
#define MODE1_BG_PIXEL_SY(_sx, _sy)                                                                                    \
    do {                                                                                                               \
        int eff_x_sy = (mosaic_h == 1) ? (_sx) : ((_sx) / mosaic_h) * mosaic_h;                                       \
        int src_x_sy = (eff_x_sy + scroll_x) & (map_width_tiles * 8 - 1);                                             \
        int tile_col_sy = src_x_sy / 8;                                                                                \
        int pixel_x_sy = src_x_sy % 8;                                                                                 \
        int eff_line_sy = (mosaic_v == 1) ? (_sy) : ((_sy) / mosaic_v) * mosaic_v;                                    \
        int src_y_sy = (eff_line_sy + scroll_y) & (map_height_tiles * 8 - 1);                                         \
        int tile_row_sy = src_y_sy / 8;                                                                                \
        int pixel_y_sy = src_y_sy % 8;                                                                                 \
        int screen_block_y_sy = tile_row_sy / 32;                                                                      \
        int local_row_sy = tile_row_sy % 32;                                                                           \
        int cache_use_shadow_sy = (ws_shadow_active && x >= virtuappu_mode1_ws_shadow_start_x[bg_index]) ? 1 : 0;     \
        int cache_key_sy = (tile_row_sy << 8) | (tile_col_sy << 1) | cache_use_shadow_sy;                             \
        if (cache_key_sy != bg_cache_key) {                                                                            \
            bg_cache_key = cache_key_sy;                                                                               \
            if (cache_use_shadow_sy) {                                                                                 \
                int shadow_idx_sy = (tile_col_sy - ws_shadow_base + 32) % 32;                                          \
                bg_tile_entry.raw = (shadow_idx_sy < MODE1_WS_SHADOW_COLS)                                            \
                                        ? ws_shadow[(size_t)local_row_sy * MODE1_WS_SHADOW_COLS + shadow_idx_sy]       \
                                        : (uint16_t)0u;                                                                \
            } else {                                                                                                   \
                int screen_block_x_sy = tile_col_sy / 32;                                                              \
                int screen_block_index_sy = screen_block_x_sy + screen_block_y_sy * blocks_per_row;                    \
                int local_col_sy = tile_col_sy % 32;                                                                   \
                uint32_t map_addr_sy = screen_base + (uint32_t)screen_block_index_sy * 0x800u +                        \
                                       (uint32_t)(local_row_sy * 32 + local_col_sy) * 2u;                              \
                bg_tile_entry.raw = (uint16_t)mode1_memory.vram[map_addr_sy] |                                         \
                                    ((uint16_t)mode1_memory.vram[map_addr_sy + 1u] << 8u);                             \
            }                                                                                                          \
            bg_hflip = mode1_tile_hflip(bg_tile_entry);                                                                \
            bg_tpy = mode1_tile_vflip(bg_tile_entry) ? (7 - pixel_y_sy) : pixel_y_sy;                                  \
            if (bpp8) {                                                                                                \
                bg_row_base = char_base + (uint32_t)mode1_tile_index(bg_tile_entry) * 64u +                            \
                              (uint32_t)bg_tpy * 8u;                                                                   \
            } else {                                                                                                   \
                bg_row_base = char_base + (uint32_t)mode1_tile_index(bg_tile_entry) * 32u +                            \
                              (uint32_t)bg_tpy * 4u;                                                                   \
            }                                                                                                          \
            bg_pal_bank = (size_t)mode1_tile_palette(bg_tile_entry) * 16u;                                             \
            bg_char_base = mode1_memory.vram;                                                                          \
            bg_char_limit = MODE1_VRAM_SIZE;                                                                           \
            bg_color_lut = mode1_bg_abgr_lut;                                                                          \
            if (group_bg && !cache_use_shadow_sy) {                                                                    \
                const bool group_addr_sy = bg_row_base < 0x4000u ||                                                    \
                                           (bg_row_base >= 0x8000u && bg_row_base < 0xC000u);                          \
                int group_cell_sy = group_addr_sy                                                                      \
                                        ? mode1_group_cells[(local_row_sy % mode1_group_rows) * mode1_group_cols +     \
                                                            (tile_col_sy % mode1_group_cols)]                          \
                                        : 0xFF;                                                                    \
                if (group_cell_sy < mode1_group_count) {                                                               \
                    bg_char_base = mode1_group_chars + (size_t)group_cell_sy * mode1_group_char_stride;                \
                    bg_char_limit = 0xC000u;                                                                           \
                    bg_color_lut = mode1_group_luts + (size_t)group_cell_sy * MODE1_PALETTE_COLORS;                    \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
        int tile_pixel_x_sy = bg_hflip ? (7 - pixel_x_sy) : pixel_x_sy;                                                \
        uint8_t color_index_sy;                                                                                        \
        if (bpp8) {                                                                                                    \
            uint32_t addr_sy = bg_row_base + (uint32_t)tile_pixel_x_sy;                                                \
            color_index_sy = (addr_sy < bg_char_limit) ? bg_char_base[addr_sy] : 0u;                                  \
        } else {                                                                                                       \
            uint32_t addr_sy = bg_row_base + (uint32_t)(tile_pixel_x_sy / 2);                                          \
            uint8_t packed_sy = (addr_sy < bg_char_limit) ? bg_char_base[addr_sy] : 0u;                               \
            color_index_sy = (tile_pixel_x_sy & 1) ? (packed_sy >> 4u) : (packed_sy & 0x0Fu);                          \
        }                                                                                                              \
        if (color_index_sy != 0u) {                                                                                    \
            size_t pal_idx_sy = bpp8 ? (size_t)color_index_sy : (bg_pal_bank + color_index_sy);                        \
            if (!(x >= virtuappu_mode1_ws_shadow_start_x[bg_index] && bg_color_lut == mode1_bg_abgr_lut &&             \
                  (mode1_memory.bg_palette[pal_idx_sy] & 0x7FFFu) == 0x7C1Fu)) {                                        \
                line_buffer[dst_x + x] = bg_color_lut[pal_idx_sy];                                                     \
                if (priority_buffer != NULL) {                                                                         \
                    priority_buffer[dst_x + x] = priority;                                                             \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

    if (ws_vertical_line) {
        /* 192-line canvases: the message and lower-right HUD keep their native
         * tilemap content but move to the canvas corners. Sample the source
         * scanline only inside the published x band; unrelated BG0 pixels on
         * the same line stay native. Banners publish shift_y == 0. */
        for (x = 0; x < render_max_x; ++x) {
            int sample_x = x;
            int sample_line = line;
            if (ws_msg_dst_line) {
                if (x >= ws_msg_x0 + ws_msg_shift && x < ws_msg_x1 + ws_msg_shift) {
                    sample_x = x - ws_msg_shift;
                    sample_line = line - ws_msg_shift_y;
                } else if ((x >= ws_msg_x0 && x < ws_msg_x1) ||
                           x >= MODE1_GBA_BG_CLIP_X) {
                    continue;
                }
            } else if (ws_msg_src_line) {
                if (x >= ws_msg_x0 && x < ws_msg_x1) {
                    continue;
                }
                if (x >= MODE1_GBA_BG_CLIP_X) {
                    continue;
                }
            } else if (ws_hud_dst_line) {
                const int source_line = line - ws_hud_shift_y;
                const int source_bit =
                    source_line < 144 ? MODE1_WS_HUD_KEYS : MODE1_WS_HUD_RUPEES;
                const int source_x0 = source_line < 144 ? 200 : 192;
                const bool installed =
                    (virtuappu_mode1_ws_hud_right_anchor & source_bit) != 0;
                if (installed &&
                    x >= source_x0 + ws_hud_shift && x < ws_hud_x1 + ws_hud_shift) {
                    sample_x = x - ws_hud_shift;
                    sample_line = source_line;
                } else if ((x >= source_x0 && x < ws_hud_x1) ||
                           x >= MODE1_GBA_BG_CLIP_X) {
                    /* Even an uninstalled counter must not sample the wrapped
                     * scanline from outside the native BG0 map. */
                    continue;
                }
            } else if (ws_hud_src_line) {
                const int source_bit =
                    line < 144 ? MODE1_WS_HUD_KEYS : MODE1_WS_HUD_RUPEES;
                const int source_x0 = line < 144 ? 200 : 192;
                const bool installed =
                    (virtuappu_mode1_ws_hud_right_anchor & source_bit) != 0;
                if ((installed && x >= source_x0 && x < ws_hud_x1) ||
                    x >= MODE1_GBA_BG_CLIP_X) {
                    continue;
                }
            }
            MODE1_BG_PIXEL_SY(sample_x, sample_line);
        }
    } else if (!ws_msg_line && !ws_hud_cells) {
        /* Fast path: no widescreen column remap, so sample_x == x. Hoists the
         * per-pixel remap dispatch (its two flags are per-line invariants) out
         * of the hot loop entirely — A53 win, zero added per-pixel branch. */
        for (x = 0; x < render_max_x; ++x) {
            MODE1_BG_PIXEL(x);
        }
    } else {
        for (x = 0; x < render_max_x; ++x) {
            int sample_x = x;
            if (ws_msg_line && x >= ws_msg_x0 + ws_msg_shift && x < ws_msg_x1 + ws_msg_shift) {
                /* Inside the shifted box: sample the box's native columns. */
                sample_x = x - ws_msg_shift;
            } else if (ws_msg_line && x >= ws_msg_x0 && x < ws_msg_x1) {
                /* The box's native columns are suppressed (moved right). */
                continue;
            } else if (ws_hud_cells && !ws_msg_line) {
                /* Anchor is suspended on box lines: a top-anchored box overlaps
                 * the rupee rows, and remapping cols 176..239 there would draw a
                 * second copy of the box fragment at the far right. */
                if (x >= ws_hud_x0 + ws_hud_shift && x < ws_hud_x1 + ws_hud_shift) {
                    sample_x = x - ws_hud_shift;
                } else if (x >= ws_hud_x0 && x < ws_hud_x1) {
                    continue;
                } else if (x >= MODE1_GBA_BG_CLIP_X) {
                    continue;
                }
            } else if (ws_msg_line && x >= MODE1_GBA_BG_CLIP_X) {
                /* Box lines extend past 240 only for the shifted box copy;
                 * everything else on the line keeps native clipping. */
                continue;
            }
            MODE1_BG_PIXEL(sample_x);
        }
    }
#undef MODE1_BG_PIXEL
#undef MODE1_BG_PIXEL_SY
}

void virtuappu_mode1_render_text_bg_line(int bg_index, int line, uint32_t* line_buffer, uint8_t* priority_buffer) {
    mode1_render_text_bg_line_internal(bg_index, line, mode1_frame_width, 0, line_buffer, priority_buffer);
}

bool virtuappu_mode1_sample_below_bg(int line, int x, int source_bg, unsigned visible_bgs,
                                     unsigned* layer, uint32_t* color) {
    if (!layer || !color || line < 0 || line >= MODE1_GBA_HEIGHT || x < 0 || x >= MODE1_GBA_BG_CLIP_X ||
        source_bg < 0 || source_bg >= MODE1_GBA_BG_COUNT || mode1_frame_width < MODE1_GBA_BG_CLIP_X)
        return false;
    const uint8_t* io = virtuappu_mode1_frame_io_for_line(line);
    if (!io || (io[0] & 7u)) return false;
    const uint8_t* previous = virtuappu_mode1_io_thread_override;
    virtuappu_mode1_io_thread_override = io;
    uint16_t display = virtuappu_mode1_io_read16(MODE1_IO_DISPCNT);
    unsigned source_priority = virtuappu_mode1_io_read16(MODE1_IO_BG0CNT + source_bg * 2) & 3u;
    unsigned best_priority = 4, best_layer = 5;
    uint32_t best_color = mode1_bg_abgr_lut[0];
    uint32_t pixels[MODE1_GBA_WIDTH];
    for (unsigned bg = 0; bg < MODE1_GBA_BG_COUNT; ++bg) {
        if (bg == (unsigned)source_bg || !(display & (1u << (8 + bg))) || !(visible_bgs & (1u << bg))) continue;
        unsigned priority = virtuappu_mode1_io_read16(MODE1_IO_BG0CNT + bg * 2) & 3u;
        if (priority < source_priority || (priority == source_priority && bg < (unsigned)source_bg) ||
            priority > best_priority || (priority == best_priority && bg > best_layer)) continue;
        memset(pixels, 0, sizeof(pixels));
        virtuappu_mode1_render_text_bg_line((int)bg, line, pixels, NULL);
        if (!(pixels[x] & 0xff000000u)) continue;
        best_priority = priority; best_layer = bg; best_color = pixels[x];
    }
    virtuappu_mode1_io_thread_override = previous;
    *layer = best_layer; *color = best_color;
    return true;
}

void virtuappu_mode1_render_obj_line(int line, bool obj_1d, uint32_t* line_buffer, uint8_t* priority_buffer) {
    const uint32_t obj_tile_base = 0x10000u;
    /* OBJ mosaic block dimensions for this line (MOSAIC is in the per-line IO
     * snapshot). GBATEK: block = (Mh+1) x (Mv+1); a mosaic sprite samples the
     * texel of the block's top-left SCREEN position. */
    const uint16_t mosaic_reg = virtuappu_mode1_io_read16(0x4Cu);
    const int mosaic_h = (int)((mosaic_reg >> 8u) & 0x0Fu) + 1;
    const int mosaic_v = (int)((mosaic_reg >> 12u) & 0x0Fu) + 1;
    int i;

    memset(virtuappu_mode1_obj_semitrans, 0, (size_t)mode1_frame_width);
    memset(virtuappu_mode1_obj_canvas, 0, (size_t)mode1_frame_width);
    memset(virtuappu_mode1_obj_window, 0, (size_t)mode1_frame_width);

    for (i = MODE1_GBA_OAM_COUNT - 1; i >= 0; --i) {
        Mode1OAMAttr attr;
        uint8_t shape;
        uint8_t size;
        int obj_width;
        int obj_height;
        bool is_affine;
        int bounds_width;
        int bounds_height;
        int obj_y;
        int obj_x;
        bool bpp8;
        uint8_t priority;
        uint16_t base_tile;
        int tiles_w;
        int16_t pa = 0x100;
        int16_t pb = 0;
        int16_t pc = 0;
        int16_t pd = 0x100;
        int half_width;
        int half_height;
        int sprite_half_width;
        int sprite_half_height;
        int input_rel_y;
        int sx;
        int sx_start;
        int sx_end;
        int viewport_width;
        int clip_left, clip_right, clip_top, clip_bottom;
        bool canvas_position;
        bool mosaic_x_on;
        int eff_line;

        attr.attr0 = mode1_memory.oam_mem[i * 4];
        attr.attr1 = mode1_memory.oam_mem[i * 4 + 1];
        attr.attr2 = mode1_memory.oam_mem[i * 4 + 2];

        if (mode1_oam_hidden(attr)) {
            continue;
        }

        shape = mode1_oam_shape(attr);
        size = mode1_oam_size(attr);
        if (shape >= 3) {
            /* OAM shape 3 is prohibited on GBA hardware and absent from the
             * mode1_obj_widths/heights[3][4] tables; skip rather than index out
             * of bounds (UBSan: degraded room data can feed a shape-3 entry). */
            continue;
        }
        obj_width = mode1_obj_widths[shape][size];
        obj_height = mode1_obj_heights[shape][size];
        is_affine = mode1_oam_affine(attr);
        bounds_width = obj_width;
        bounds_height = obj_height;

        if (is_affine && mode1_oam_double_size(attr)) {
            bounds_width *= 2;
            bounds_height *= 2;
        }

        if (!mode1_obj_position(i, attr, mode1_frame_width, &obj_x, &obj_y, &canvas_position))
            continue;

        /* World sprites follow the live aperture; canvas-tagged HUD widgets
         * keep their absolute canvas position. Margins never show world
         * sprites, and the aperture never shows a canvas widget clipped away. */
        const bool world_sprite = mode1_world_enabled && !canvas_position;
        clip_left = 0;
        clip_right = mode1_frame_width;
        clip_top = 0;
        clip_bottom = mode1_frame_height;
        if (world_sprite) {
            clip_left = mode1_world_x;
            clip_right = mode1_world_x + mode1_world_w;
            if (clip_right > mode1_frame_width) clip_right = mode1_frame_width;
            clip_top = mode1_world_y;
            clip_bottom = mode1_world_y + mode1_world_h;
            if (clip_bottom > mode1_frame_height) clip_bottom = mode1_frame_height;
            obj_x += mode1_world_x;
            obj_y += mode1_world_y;
        }
        if (line < clip_top || line >= clip_bottom) {
            continue;
        }
        if (line < obj_y || line >= obj_y + bounds_height) {
            continue;
        }

        viewport_width = mode1_frame_width;
        if (viewport_width > MODE1_GBA_VIEWPORT_X) {
            viewport_width = MODE1_GBA_VIEWPORT_X;
        }
        if (clip_right > viewport_width) {
            clip_right = viewport_width;
        }
        sx_start = 0;
        sx_end = bounds_width;
        if (obj_x + bounds_width <= clip_left || obj_x >= clip_right) {
            continue;
        }
        if (obj_x < clip_left) {
            sx_start = clip_left - obj_x;
        }
        if (obj_x + sx_end > clip_right) {
            sx_end = clip_right - obj_x;
        }

        bpp8 = mode1_oam_bpp8(attr);
        priority = mode1_oam_priority(attr);
        base_tile = mode1_oam_tile_index(attr);
        tiles_w = obj_width / 8;

        if (is_affine) {
            int affine_group = mode1_oam_affine_index(attr);
            pa = (int16_t)mode1_memory.oam_mem[affine_group * 16 + 3];
            pb = (int16_t)mode1_memory.oam_mem[affine_group * 16 + 7];
            pc = (int16_t)mode1_memory.oam_mem[affine_group * 16 + 11];
            pd = (int16_t)mode1_memory.oam_mem[affine_group * 16 + 15];
        }

        half_width = bounds_width / 2;
        half_height = bounds_height / 2;
        sprite_half_width = obj_width / 2;
        sprite_half_height = obj_height / 2;

        /* OBJ mosaic (attr0 bit 12): the pixel shown at screen (x, line)
         * samples the sprite as if at the mosaic block's top-left screen
         * position (x - x%Mh, line - line%Mv). Vertical part is constant for
         * the whole scanline; a snapped line above the sprite's top samples
         * nothing (transparent). */
        {
            bool mosaic_on = mode1_oam_mosaic(attr) && (mosaic_h > 1 || mosaic_v > 1);
            eff_line = line;
            if (mosaic_on) {
                eff_line = line - (line % mosaic_v);
                if (eff_line < obj_y) {
                    continue;
                }
            }
            input_rel_y = eff_line - obj_y - half_height;
            mosaic_x_on = mosaic_on && mosaic_h > 1;
        }

        /* Horizontal frustum culling is done once per sprite. Partially visible
         * sprites enter the loop at the first on-screen column, so the hot
         * pixel path has no screen_x bounds branch. */
        for (sx = sx_start; sx < sx_end; ++sx) {
            int screen_x = obj_x + sx;
            int eff_sx = sx;
            int tex_x;
            int tex_y;
            int tile_row;
            int pixel_y;
            int tile_col;
            int pixel_x;
            uint16_t tile_index;
            uint8_t color_index;

            if (mosaic_x_on) {
                /* Sample at the mosaic block's left SCREEN column; a snapped
                 * column left of the sprite's edge samples nothing. */
                int snapped_x = screen_x - (screen_x % mosaic_h);
                eff_sx = snapped_x - obj_x;
                if (eff_sx < 0) {
                    continue;
                }
            }

            if (is_affine) {
                int input_rel_x = eff_sx - half_width;
                tex_x = ((pa * input_rel_x + pb * input_rel_y) >> 8) + sprite_half_width;
                tex_y = ((pc * input_rel_x + pd * input_rel_y) >> 8) + sprite_half_height;
                if (tex_x < 0 || tex_x >= obj_width || tex_y < 0 || tex_y >= obj_height) {
                    continue;
                }
            } else {
                int draw_x = mode1_oam_hflip(attr) ? (obj_width - 1 - eff_sx) : eff_sx;
                int draw_y = eff_line - obj_y;
                if (mode1_oam_vflip(attr)) {
                    draw_y = obj_height - 1 - draw_y;
                }
                tex_x = draw_x;
                tex_y = draw_y;
            }

            tile_row = tex_y / 8;
            pixel_y = tex_y % 8;
            tile_col = tex_x / 8;
            pixel_x = tex_x % 8;

            if (obj_1d) {
                tile_index = (uint16_t)(base_tile + tile_row * tiles_w + tile_col);
                if (bpp8) {
                    tile_index = (uint16_t)(base_tile + (tile_row * tiles_w + tile_col) * 2);
                }
            } else {
                tile_index = (uint16_t)(base_tile + tile_row * 32 + tile_col);
                if (bpp8) {
                    tile_index = (uint16_t)(base_tile + tile_row * 32 + tile_col * 2);
                }
            }

            if (bpp8) {
                uint32_t addr = obj_tile_base + (uint32_t)tile_index * 32u + (uint32_t)pixel_y * 8u + (uint32_t)pixel_x;
                color_index = (addr < MODE1_VRAM_SIZE) ? mode1_memory.vram[addr] : 0u;
            } else {
                uint32_t addr =
                    obj_tile_base + (uint32_t)tile_index * 32u + (uint32_t)pixel_y * 4u + (uint32_t)(pixel_x / 2);
                uint8_t packed = (addr < MODE1_VRAM_SIZE) ? mode1_memory.vram[addr] : 0u;
                color_index = (pixel_x & 1) ? (packed >> 4u) : (packed & 0x0Fu);
            }

            if (color_index == 0u) {
                continue;
            }

            if (mode1_oam_mode(attr) == 2u) {
                /* OBJ-window sprite: invisible, only marks the window mask. */
                virtuappu_mode1_obj_window[screen_x] = 1u;
                continue;
            }

            /* OBJ-vs-OBJ is resolved by OAM index ALONE (GBATEK): the lowest
             * OAM index's opaque pixel wins the OBJ layer, and only that
             * pixel's priority then competes against BGs. This loop walks
             * i = 127..0, so the later (lower-index) sprite must ALWAYS
             * overwrite — the old `priority_buffer < priority` guard let a
             * higher-index sprite with a lower priority number stay on top
             * (wrong sprite visible in the classic quirk case). */

            /* Per-pixel vertical OBJ clip (swamp sink): drop this marked entry's
             * pixels at or below the waterline. `i` is the OAM index, `line` the
             * scanline. */
            if (virtuappu_mode1_obj_clip_enable && virtuappu_mode1_obj_clip_mark[i] &&
                line >= virtuappu_mode1_obj_clip_y) {
                continue;
            }

            size_t pal_idx = bpp8 ? (size_t)color_index : ((size_t)mode1_oam_palette(attr) * 16u + color_index);
            line_buffer[screen_x] = mode1_obj_abgr_lut[pal_idx];
            priority_buffer[screen_x] = priority;
            virtuappu_mode1_obj_semitrans[screen_x] = (mode1_oam_mode(attr) == 1u) ? 1u : 0u;
            virtuappu_mode1_obj_canvas[screen_x] = canvas_position ? 1u : 0u;
        }
    }
}

void virtuappu_mode1_composite_line(int line, uint32_t bg_layers[MODE1_GBA_BG_COUNT][MODE1_GBA_WIDTH],
                                    uint8_t bg_priority[MODE1_GBA_BG_COUNT][MODE1_GBA_WIDTH],
                                    uint32_t obj_layer[MODE1_GBA_WIDTH], uint8_t obj_priority[MODE1_GBA_WIDTH],
                                    uint16_t dispcnt) {
    uint32_t backdrop_color = mode1_bg_abgr_lut[0];
    bool bg_enabled[MODE1_GBA_BG_COUNT] = { (dispcnt & MODE1_DISP_BG0_ON) != 0u, (dispcnt & MODE1_DISP_BG1_ON) != 0u,
                                            (dispcnt & MODE1_DISP_BG2_ON) != 0u, (dispcnt & MODE1_DISP_BG3_ON) != 0u };
    bool obj_enabled = (dispcnt & MODE1_DISP_OBJ_ON) != 0u;
    uint16_t bldcnt = virtuappu_mode1_io_read16(MODE1_IO_BLDCNT);
    uint16_t bldalpha = virtuappu_mode1_io_read16(MODE1_IO_BLDALPHA);
    uint16_t bldy = virtuappu_mode1_io_read16(MODE1_IO_BLDY);
    Mode1BlendEffect effect = (Mode1BlendEffect)((bldcnt >> 6u) & 3u);
    int eva = bldalpha & 0x1Fu;
    int evb = (bldalpha >> 8u) & 0x1Fu;
    int evy = bldy & 0x1Fu;
    uint8_t bg_order[MODE1_GBA_BG_COUNT] = { 0, 1, 2, 3 };
    uint8_t bg_order_priority[MODE1_GBA_BG_COUNT];
    bool win0_on = (dispcnt & MODE1_DISP_WIN0_ON) != 0u;
    bool win1_on = (dispcnt & MODE1_DISP_WIN1_ON) != 0u;
    bool objwin_on = (dispcnt & MODE1_DISP_OBJWIN_ON) != 0u;
    bool any_window = win0_on || win1_on || objwin_on;
    uint16_t winin = virtuappu_mode1_io_read16(MODE1_IO_WININ);
    uint16_t winout = virtuappu_mode1_io_read16(MODE1_IO_WINOUT);
    uint16_t win0h = virtuappu_mode1_io_read16(MODE1_IO_WIN0H);
    uint16_t win0v = virtuappu_mode1_io_read16(MODE1_IO_WIN0V);
    int win0_left = win0h >> 8u;
    int win0_right = win0h & 0xFFu;
    int win0_top = win0v >> 8u;
    int win0_bottom = win0v & 0xFFu;
    bool win0_h_wrap;
    bool win0_v_wrap;
    bool win0_v_active;
    uint16_t win1h = virtuappu_mode1_io_read16(MODE1_IO_WIN1H);
    uint16_t win1v = virtuappu_mode1_io_read16(MODE1_IO_WIN1V);
    int win1_left = win1h >> 8u;
    int win1_right = win1h & 0xFFu;
    int win1_top = win1v >> 8u;
    int win1_bottom = win1v & 0xFFu;
    bool win1_h_wrap;
    bool win1_v_wrap;
    bool win1_v_active;
    uint8_t win0_ctrl = (uint8_t)(winin & 0x3Fu);
    uint8_t win1_ctrl = (uint8_t)((winin >> 8u) & 0x3Fu);
    uint8_t outside_ctrl = (uint8_t)(winout & 0x3Fu);
    uint8_t objwin_ctrl = (uint8_t)((winout >> 8u) & 0x3Fu);
    int i;
    int x;
    const int frame_width = mode1_frame_width;
    uint32_t* const out_row = &virtuappu_frame_buffer[(size_t)line * (size_t)mode1_frame_pitch];

    (void)bg_priority;

    if (eva > 16) {
        eva = 16;
    }
    if (evb > 16) {
        evb = 16;
    }
    if (evy > 16) {
        evy = 16;
    }

    if (win0_right > frame_width) {
        win0_right = frame_width;
    }
    if (win0_bottom > MODE1_GBA_HEIGHT) {
        win0_bottom = MODE1_GBA_HEIGHT;
    }
    if (win1_right > frame_width) {
        win1_right = frame_width;
    }
    if (win1_bottom > MODE1_GBA_HEIGHT) {
        win1_bottom = MODE1_GBA_HEIGHT;
    }

    win0_h_wrap = win0_left > win0_right;
    /* GBA: WINxV with top > bottom is an inverted/wrapped vertical span,
     * active for (line >= top || line < bottom) — mirrors the horizontal
     * winX_h_wrap handling. Previously the window went fully inactive on
     * inversion, blanking the digging-cave iris spotlight (src/scroll.c
     * Scroll5Sub2/Sub5) on small-iris frames where an inverted WINxV is packed. */
    win0_v_wrap = win0_top > win0_bottom;
    win0_v_active =
        win0_on && (win0_v_wrap ? (line >= win0_top || line < win0_bottom) : (line >= win0_top && line < win0_bottom));
    win1_h_wrap = win1_left > win1_right;
    win1_v_wrap = win1_top > win1_bottom;
    win1_v_active =
        win1_on && (win1_v_wrap ? (line >= win1_top || line < win1_bottom) : (line >= win1_top && line < win1_bottom));

    for (i = 0; i < MODE1_GBA_BG_COUNT; ++i) {
        bg_order_priority[i] = (uint8_t)(virtuappu_mode1_io_read16((uint16_t)(MODE1_IO_BG0CNT + i * 2)) & 3u);
    }

    /* Stable insertion sort by priority. GBA hardware breaks priority ties by
     * BG index (lower BG number drawn on top), so equal-priority BGs MUST keep
     * their initial 0,1,2,3 order. The previous selection sort swapped
     * non-adjacent entries, so a lower-priority BG (e.g. a disabled BG3 left at
     * priority 0 by a prior dark room) could swap forward and displace BG1 past
     * BG2 — flipping the BG1/BG2 tie-break and hiding a same-priority BG1 layer
     * behind BG2. #139: ToGrimblade's BG1 flame braziers vanished behind the
     * BG2 bowl after a dark-dojo round-trip left BG3CNT at priority 0. Using a
     * stable sort (`>` strict, only shift higher-priority entries) keeps the
     * GBA index tie-break intact regardless of the other BGs' priorities. */
    for (i = 1; i < MODE1_GBA_BG_COUNT; ++i) {
        uint8_t key = bg_order[i];
        uint8_t key_pri = bg_order_priority[key];
        int j = i - 1;
        while (j >= 0 && bg_order_priority[bg_order[j]] > key_pri) {
            bg_order[j + 1] = bg_order[j];
            --j;
        }
        bg_order[j + 1] = key;
    }

    for (x = 0; x < frame_width; ++x) {
        uint8_t win_ctrl = 0x3Fu;
        bool visible_bg[MODE1_GBA_BG_COUNT];
        bool visible_obj;
        bool allow_sfx;
        uint32_t top_color = backdrop_color;
        int top_layer = 5;
        uint32_t bottom_color = backdrop_color;
        int bottom_layer = 5;
        bool found_top = false;
        bool found_bottom = false;

        if (any_window) {
            win_ctrl = outside_ctrl;
            /* Window priority is WIN0 > WIN1 > OBJ-window > outside, so OBJ
             * window sits below WIN1/WIN0 (applied after this) and above the
             * plain outside control. */
            if (objwin_on && virtuappu_mode1_obj_window[x]) {
                win_ctrl = objwin_ctrl;
            }
            if (win1_v_active) {
                bool in_h = win1_h_wrap ? (x >= win1_left || x < win1_right) : (x >= win1_left && x < win1_right);
                if (in_h) {
                    win_ctrl = win1_ctrl;
                }
            }
            if (win0_v_active) {
                bool in_h = win0_h_wrap ? (x >= win0_left || x < win0_right) : (x >= win0_left && x < win0_right);
                if (in_h) {
                    win_ctrl = win0_ctrl;
                }
            }
        }

        visible_bg[0] = (win_ctrl & 0x01u) != 0u;
        visible_bg[1] = (win_ctrl & 0x02u) != 0u;
        visible_bg[2] = (win_ctrl & 0x04u) != 0u;
        visible_bg[3] = (win_ctrl & 0x08u) != 0u;
        visible_obj = (win_ctrl & 0x10u) != 0u;
        allow_sfx = (win_ctrl & 0x20u) != 0u;

        /* Single merged pass over the priority-sorted bg_order, inserting OBJ at
         * its priority slot — byte-identical layering to the former
         * priority(0..3) x bg(0..3) double loop (up to 20 iters/pixel), but ~5
         * iters and one loop. OBJ is emitted right before the first bg whose
         * priority >= obj_priority[x] (GBA ties: OBJ over same-priority BG); if
         * no such bg, OBJ is emitted after the walk. Removing per-pixel work with
         * no added data-dependent branch is the A53-correct optimisation. */
        {
            bool obj_candidate = obj_enabled && visible_obj && (obj_layer[x] != 0u);
            unsigned obj_p = obj_priority[x];
            bool obj_emitted = false;
            int order_index;

#define MODE1_CONSIDER(_color, _layer) \
    do {                               \
        if (!found_top) {              \
            top_color = (_color);      \
            top_layer = (_layer);      \
            found_top = true;          \
        } else {                       \
            bottom_color = (_color);   \
            bottom_layer = (_layer);   \
            found_bottom = true;       \
        }                              \
    } while (0)

            for (order_index = 0; order_index < MODE1_GBA_BG_COUNT && !found_bottom; ++order_index) {
                int bg = bg_order[order_index];
                if (obj_candidate && !obj_emitted && bg_order_priority[bg] >= obj_p) {
                    MODE1_CONSIDER(obj_layer[x], 4);
                    obj_emitted = true;
                    if (found_bottom) {
                        break;
                    }
                }
                if (!bg_enabled[bg] || !visible_bg[bg] || bg_layers[bg][x] == 0u) {
                    continue;
                }
                MODE1_CONSIDER(bg_layers[bg][x], bg);
            }
            if (obj_candidate && !obj_emitted && !found_bottom) {
                MODE1_CONSIDER(obj_layer[x], 4);
            }
#undef MODE1_CONSIDER
        }

        if (allow_sfx) {
            if (top_layer == 4 && virtuappu_mode1_obj_semitrans[x]) {
                /* Semi-transparent OBJ (attr0 mode 1): GBA forces it to be an
                 * alpha-blend 1st target regardless of BLDCNT's effect mode and
                 * 1st-target bits. It blends with the layer directly below when
                 * that layer is a BLDCNT 2nd target (the backdrop counts via the
                 * BD bit) and that blend preempts any brighten/darken. When the
                 * layer below is NOT a 2nd target, GBATEK: "the brightness
                 * effect will take place" — the OBJ is still an always-1st
                 * target, so BLDY brighten/darken applies (previously skipped,
                 * leaving sprites unfaded during BLDY screen fades). */
                if (mode1_is_second_target(bldcnt, bottom_layer)) {
                    top_color = mode1_alpha_blend(top_color, bottom_color, eva, evb);
                } else if (effect == MODE1_BLEND_BRIGHTEN) {
                    top_color = mode1_brighten(top_color, evy);
                } else if (effect == MODE1_BLEND_DARKEN) {
                    top_color = mode1_darken(top_color, evy);
                }
            } else {
                switch (effect) {
                    case MODE1_BLEND_ALPHA:
                        if (mode1_is_first_target(bldcnt, top_layer) && mode1_is_second_target(bldcnt, bottom_layer)) {
                            top_color = mode1_alpha_blend(top_color, bottom_color, eva, evb);
                        }
                        break;
                    case MODE1_BLEND_BRIGHTEN:
                        if (mode1_is_first_target(bldcnt, top_layer)) {
                            top_color = mode1_brighten(top_color, evy);
                        }
                        break;
                    case MODE1_BLEND_DARKEN:
                        if (mode1_is_first_target(bldcnt, top_layer)) {
                            top_color = mode1_darken(top_color, evy);
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        /* Transparent BGs reveal OBJ/backdrop in every column. Scene bounds
         * are enforced by the port's frame width, not by pixel opacity.
         * The UI mask covers BG0/canvas OBJ everywhere, so the margin fade
         * never samples a HUD pixel as the world-edge source. */
        if (mode1_world_enabled && found_top &&
            (top_layer == 0 || (top_layer == 4 && virtuappu_mode1_obj_canvas[x])))
            mode1_ui_mask[(size_t)line * (size_t)frame_width + (size_t)x] = 1;
        out_row[x] = top_color;
    }
}

/* Sub-pixel overlay for OAM affine sprites at internal-render-scale > 1.
 *
 * Called by the PC port AFTER the standard 240x160 render has already been
 * S*S nearest-replicated into `dst` (a 240*S by 160*S buffer). For each
 * affine OAM entry we re-run the matrix at sub-pixel density and write
 * straight to `dst`, which produces visibly smoother diagonals on rotated
 * sprites (Vaati's tornado, screen-shrink cinematic, every spinning enemy).
 *
 * Caveats:
 *   * No priority/blend layering — affine pixels overwrite whatever's at
 *     the target. Acceptable for a first pass because the affine sprite
 *     was already top-of-stack at scale 1; in TMC there are very few
 *     scenes where a non-affine sprite occludes an affine one.
 *   * Source texture is still 1x pixel art — no information to "recover".
 *     What you gain is sub-pixel sampling at the screen-space rotation,
 *     which trades the 240-grid staircase for an S*240-grid one.
 */
void virtuappu_mode1_render_affine_obj_overlay(uint32_t* dst, int dst_w, int dst_h, int scale) {
    if (dst == NULL || scale <= 1) {
        return;
    }
    if ((dst_w % scale) != 0 || dst_h != mode1_frame_height * scale) {
        return;
    }
    const int viewport_width = dst_w / scale;
    if (viewport_width < 1 || viewport_width > MODE1_GBA_WIDTH) {
        return;
    }

    uint16_t dispcnt = virtuappu_mode1_io_read16(MODE1_IO_DISPCNT);
    if ((dispcnt & MODE1_DISP_FORCED_BLANK) != 0u) {
        return;
    }
    if ((dispcnt & MODE1_DISP_OBJ_ON) == 0u) {
        return;
    }
    bool obj_1d = (dispcnt & MODE1_DISP_OBJ_1D) != 0u;
    const uint32_t obj_tile_base = 0x10000u;

    /* OAM is iterated lowest-priority-first so higher-priority entries
     * overwrite — same convention as virtuappu_mode1_render_obj_line, just
     * applied at sub-pixel resolution. */
    for (int i = MODE1_GBA_OAM_COUNT - 1; i >= 0; --i) {
        Mode1OAMAttr attr;
        attr.attr0 = mode1_memory.oam_mem[i * 4];
        attr.attr1 = mode1_memory.oam_mem[i * 4 + 1];
        attr.attr2 = mode1_memory.oam_mem[i * 4 + 2];

        if (!mode1_oam_affine(attr))
            continue;
        if (mode1_oam_hidden(attr))
            continue;

        uint8_t shape = mode1_oam_shape(attr);
        uint8_t size = mode1_oam_size(attr);
        if (shape >= 3) {
            /* Prohibited OAM shape — absent from the width/height tables;
             * skip like render_obj_line does (degraded room data can feed a
             * shape-3 entry; indexing the [3][4] tables is UB). */
            continue;
        }
        int obj_width = mode1_obj_widths[shape][size];
        int obj_height = mode1_obj_heights[shape][size];
        int bounds_width = obj_width;
        int bounds_height = obj_height;
        if (mode1_oam_double_size(attr)) {
            bounds_width *= 2;
            bounds_height *= 2;
        }

        int obj_x, obj_y;
        bool canvas_position;
        if (!mode1_obj_position(i, attr, viewport_width, &obj_x, &obj_y, &canvas_position))
            continue;

        /* The supersampled overlay shares the live-world composition: world
         * affine sprites follow the aperture, canvas HUD sprites stay absolute. */
        const bool world_sprite = mode1_world_enabled && !canvas_position;
        int clip_left = 0, clip_right = viewport_width;
        int clip_top = 0, clip_bottom = mode1_frame_height;
        if (world_sprite) {
            clip_left = mode1_world_x;
            clip_right = mode1_world_x + mode1_world_w;
            if (clip_right > viewport_width) clip_right = viewport_width;
            clip_top = mode1_world_y;
            clip_bottom = mode1_world_y + mode1_world_h;
            if (clip_bottom > mode1_frame_height) clip_bottom = mode1_frame_height;
            obj_x += mode1_world_x;
            obj_y += mode1_world_y;
        }
        const int clip_top_sub = clip_top * scale;
        const int clip_bottom_sub = clip_bottom * scale;
        const int clip_left_sub = clip_left * scale;
        const int clip_right_sub = clip_right * scale;

        int sy_sub_start = 0;
        int sy_sub_end = bounds_height * scale;
        int sx_sub_start = 0;
        int sx_sub_end = bounds_width * scale;
        const int sprite_top_sub = obj_y * scale;
        const int sprite_left_sub = obj_x * scale;
        if (sprite_top_sub + sy_sub_end <= clip_top_sub || sprite_top_sub >= clip_bottom_sub) {
            continue;
        }
        if (sprite_top_sub < clip_top_sub) {
            sy_sub_start = clip_top_sub - sprite_top_sub;
        }
        if (sprite_top_sub + sy_sub_end > clip_bottom_sub) {
            sy_sub_end = clip_bottom_sub - sprite_top_sub;
        }
        if (sprite_left_sub + sx_sub_end <= clip_left_sub || sprite_left_sub >= clip_right_sub) {
            continue;
        }
        if (sprite_left_sub < clip_left_sub) {
            sx_sub_start = clip_left_sub - sprite_left_sub;
        }
        if (sprite_left_sub + sx_sub_end > clip_right_sub) {
            sx_sub_end = clip_right_sub - sprite_left_sub;
        }

        bool bpp8 = mode1_oam_bpp8(attr);
        uint16_t base_tile = mode1_oam_tile_index(attr);
        int tiles_w = obj_width / 8;

        int affine_group = mode1_oam_affine_index(attr);
        int16_t pa = (int16_t)mode1_memory.oam_mem[affine_group * 16 + 3];
        int16_t pb = (int16_t)mode1_memory.oam_mem[affine_group * 16 + 7];
        int16_t pc = (int16_t)mode1_memory.oam_mem[affine_group * 16 + 11];
        int16_t pd = (int16_t)mode1_memory.oam_mem[affine_group * 16 + 15];

        int half_width = bounds_width / 2;
        int half_height = bounds_height / 2;
        int sprite_half_w = obj_width / 2;
        int sprite_half_h = obj_height / 2;

        /* Iterate only the visible output sub-pixels. The frustum clamp above
         * removes both the vertical and horizontal bounds branches from the
         * hot affine sprite loop. */
        for (int sy_sub = sy_sub_start; sy_sub < sy_sub_end; ++sy_sub) {
            /* line_sub = output row = sprite-top output row + sub-row within sprite. */
            int line_sub = sprite_top_sub + sy_sub;

            /* input_rel_y in (1/scale) source-pixel units. */
            int input_rel_y_subS = sy_sub - half_height * scale;

            for (int sx_sub = sx_sub_start; sx_sub < sx_sub_end; ++sx_sub) {
                int screen_x_sub = sprite_left_sub + sx_sub;
                int input_rel_x_subS = sx_sub - half_width * scale;
                /* tex_x / tex_y in source-pixel units. pa is 8.8 fixed,
                 * input_rel_*_subS is 1/scale source-pixels, so the product
                 * is (8.8 / scale) source-pixel units. >>8 trims the fixed
                 * fraction; / scale converts back to integer source-pixels. */
                int tex_x = (((int)pa * input_rel_x_subS + (int)pb * input_rel_y_subS) >> 8) / scale + sprite_half_w;
                int tex_y = (((int)pc * input_rel_x_subS + (int)pd * input_rel_y_subS) >> 8) / scale + sprite_half_h;
                if (tex_x < 0 || tex_x >= obj_width)
                    continue;
                if (tex_y < 0 || tex_y >= obj_height)
                    continue;

                int tile_row = tex_y / 8;
                int pixel_y = tex_y % 8;
                int tile_col = tex_x / 8;
                int pixel_x = tex_x % 8;

                uint16_t tile_index;
                if (obj_1d) {
                    tile_index = (uint16_t)(base_tile + tile_row * tiles_w + tile_col);
                    if (bpp8)
                        tile_index = (uint16_t)(base_tile + (tile_row * tiles_w + tile_col) * 2);
                } else {
                    tile_index = (uint16_t)(base_tile + tile_row * 32 + tile_col);
                    if (bpp8)
                        tile_index = (uint16_t)(base_tile + tile_row * 32 + tile_col * 2);
                }

                uint8_t color_index;
                if (bpp8) {
                    uint32_t addr =
                        obj_tile_base + (uint32_t)tile_index * 32u + (uint32_t)pixel_y * 8u + (uint32_t)pixel_x;
                    color_index = (addr < MODE1_VRAM_SIZE) ? mode1_memory.vram[addr] : 0u;
                } else {
                    uint32_t addr =
                        obj_tile_base + (uint32_t)tile_index * 32u + (uint32_t)pixel_y * 4u + (uint32_t)(pixel_x / 2);
                    uint8_t packed = (addr < MODE1_VRAM_SIZE) ? mode1_memory.vram[addr] : 0u;
                    color_index = (pixel_x & 1) ? (packed >> 4u) : (packed & 0x0Fu);
                }
                if (color_index == 0u)
                    continue;

                uint16_t rgb555;
                if (bpp8) {
                    rgb555 = mode1_memory.obj_palette[color_index];
                } else {
                    rgb555 = mode1_memory.obj_palette[(size_t)mode1_oam_palette(attr) * 16u + color_index];
                }

                dst[(size_t)line_sub * (size_t)dst_w + (size_t)screen_x_sub] =
                    virtuappu_mode1_rgb555_to_abgr8888(rgb555);
            }
        }
    }
}
static inline int32_t mode1_sign_extend_28(uint32_t v) {
    int32_t s = (int32_t)v;
    if ((v & 0x08000000u) != 0u) {
        s |= (int32_t)0xF0000000u;
    }
    return s;
}

/* Host-set per-frame write strobes (see mode1.h). */
bool virtuappu_mode1_bg2x_hdma_strobe = false;
bool virtuappu_mode1_bg2y_hdma_strobe = false;

void virtuappu_mode1_affine_precompute(int height, int32_t init_ref_x, int32_t init_ref_y, const int32_t* line_ref_x,
                                       const int32_t* line_ref_y, const int16_t* line_pb, const int16_t* line_pd,
                                       bool reload_x_every_line, bool reload_y_every_line, int32_t* out_ref_x,
                                       int32_t* out_ref_y) {
    int32_t aint_x = init_ref_x;
    int32_t aint_y = init_ref_y;
    int32_t aprev_x = init_ref_x;
    int32_t aprev_y = init_ref_y;
    for (int line = 0; line < height; ++line) {
        const int32_t rx = line_ref_x[line];
        const int32_t ry = line_ref_y[line];
        /* A write to BG2X/BG2Y (e.g. the rolling barrel's per-scanline HBlank
         * DMA) reloads the internal reference; otherwise it keeps the value
         * advanced by pb/pd. Value-diff detection alone misses IDEMPOTENT
         * writes — constant-value HDMA pins the layer on hardware — so the
         * reload_*_every_line strobes force the reload when the host knows a
         * write event happens each line. */
        if (reload_x_every_line || rx != aprev_x) {
            aint_x = rx;
        }
        if (reload_y_every_line || ry != aprev_y) {
            aint_y = ry;
        }
        aprev_x = rx;
        aprev_y = ry;
        out_ref_x[line] = aint_x;
        out_ref_y[line] = aint_y;
        aint_x += line_pb[line];
        aint_y += line_pd[line];
    }
}

/* Render one scanline of the affine BG2 (GBA modes 1/2) from a precomputed
 * internal reference (virtuappu_mode1_affine_precompute). Reads pa/pc and
 * BG2CNT through the IO accessors — in the parallel pass the thread override
 * points at this line's snapshot — so it is independent per line. Byte-for-byte
 * the former mode2.c inner loop. */
static void mode1_render_affine_bg2_line(int frame_width, int32_t ref_x, int32_t ref_y, uint32_t* line_buffer,
                                         uint8_t* priority_buffer) {
    static const int affine_sizes[4] = { 128, 256, 512, 1024 };
    VirtuaPPUMode1GbaMemory memory;
    virtuappu_mode1_get_bound_gba_memory(&memory);

    const uint16_t bgcnt = virtuappu_mode1_io_read16(MODE1_IO_BG2CNT);
    const uint8_t bg_priority_value = (uint8_t)(bgcnt & 3u);
    const uint32_t char_base = (uint32_t)((bgcnt >> 2u) & 3u) * 0x4000u;
    const uint32_t screen_base = (uint32_t)((bgcnt >> 8u) & 0x1Fu) * 0x800u;
    const bool wrap = ((bgcnt >> 13u) & 1u) != 0u;
    const uint16_t size_flag = (uint16_t)((bgcnt >> 14u) & 3u);
    const int map_size = affine_sizes[size_flag];
    const int map_tiles = map_size / 8;
    const int16_t pa = (int16_t)virtuappu_mode1_io_read16(0x20u);
    const int16_t pc = (int16_t)virtuappu_mode1_io_read16(0x24u);

    for (int x = 0; x < frame_width; ++x) {
        int32_t src_x = (ref_x + pa * x) >> 8;
        int32_t src_y = (ref_y + pc * x) >> 8;

        if (wrap) {
            /* map_size is a power of two (128..1024) — mask instead of the
             * former double-idiv per axis per pixel (same trick as the text
             * BG path). */
            src_x &= (map_size - 1);
            src_y &= (map_size - 1);
        } else if (src_x < 0 || src_x >= map_size || src_y < 0 || src_y >= map_size) {
            continue;
        }

        const int tile_col = src_x / 8;
        const int tile_row = src_y / 8;
        const int pixel_x = src_x % 8;
        const int pixel_y = src_y % 8;
        const uint32_t map_addr = screen_base + (uint32_t)(tile_row * map_tiles + tile_col);
        const uint8_t tile_index = (map_addr < MODE1_VRAM_SIZE) ? memory.vram[map_addr] : 0u;
        const uint32_t tile_addr = char_base + (uint32_t)tile_index * 64u + (uint32_t)pixel_y * 8u + (uint32_t)pixel_x;
        const uint8_t color_index = (tile_addr < MODE1_VRAM_SIZE) ? memory.vram[tile_addr] : 0u;

        if (color_index == 0u) {
            continue;
        }

        line_buffer[x] = mode1_bg_abgr_lut[color_index];
        if (priority_buffer != NULL) {
            priority_buffer[x] = bg_priority_value; /* see render_text_bg_line */
        }
    }
}

int virtuappu_mode1_prepare_frame(const PPUMemory* ppu, uint8_t* io_per_line, uint16_t* dispcnt_per_line,
                                  int32_t* aff_ref_x, int32_t* aff_ref_y, uint16_t* out_frame_dispcnt) {
    /* Sequential portion of render_frame (see there for the threading contract):
     * run the per-line HDMA callback, snapshot IO + DISPCNT per line, and
     * precompute the affine BG2 per-line reference. No parallel render — the GPU
     * rasterizer does that. Kept byte-identical to render_frame's setup so the
     * GPU path sees exactly the CPU's per-line inputs. */
    const bool affine = (ppu->mode == 2);
    uint16_t dispcnt;
    int line;

    virtuappu_mode1_set_frame_geometry(ppu);
    const int raster_rows = mode1_frame_height > MODE1_GBA_HEIGHT ? mode1_frame_height : MODE1_GBA_HEIGHT;

    dispcnt = virtuappu_mode1_io_read16(MODE1_IO_DISPCNT);
    if (out_frame_dispcnt != NULL) {
        *out_frame_dispcnt = dispcnt;
    }

    const bool do_affine_bg2 = affine && ((dispcnt & MODE1_DISP_BG2_ON) != 0u);
    int32_t aff_line_ref_x[MODE1_MAX_RENDER_HEIGHT];
    int32_t aff_line_ref_y[MODE1_MAX_RENDER_HEIGHT];
    int16_t aff_pb[MODE1_MAX_RENDER_HEIGHT];
    int16_t aff_pd[MODE1_MAX_RENDER_HEIGHT];
    int32_t aff_init_x = 0;
    int32_t aff_init_y = 0;
    if (do_affine_bg2) {
        aff_init_x = mode1_sign_extend_28(virtuappu_mode1_io_read32(0x28u));
        aff_init_y = mode1_sign_extend_28(virtuappu_mode1_io_read32(0x2Cu));
    }

    const bool per_line_io = (virtuappu_mode1_pre_line_callback != NULL);
    for (line = 0; line < raster_rows; ++line) {
        if (per_line_io && line < MODE1_GBA_HEIGHT) {
            virtuappu_mode1_pre_line_callback(line);
        }
        /* Snapshot IO for the GPU. When there is no per-line HDMA callback,
         * io_mem is identical on every scanline, so snapshot ONLY row 0 — the
         * GPU uploads one row and the shader reads row 0 for all lines. This
         * removes 159 KB of redundant memcpy + bus transfer per frame. With a
         * callback, every line can differ, so snapshot them all. */
        if (per_line_io || line == 0) {
            memcpy(&io_per_line[(size_t)line * MODE1_IO_MEM_SIZE], mode1_memory.io_mem, MODE1_IO_MEM_SIZE);
        }
        dispcnt_per_line[line] = affine ? dispcnt
                                        : (uint16_t)((uint16_t)mode1_memory.io_mem[MODE1_IO_DISPCNT] |
                                                     ((uint16_t)mode1_memory.io_mem[MODE1_IO_DISPCNT + 1] << 8));
        if (do_affine_bg2) {
            aff_line_ref_x[line] = mode1_sign_extend_28(virtuappu_mode1_io_read32(0x28u));
            aff_line_ref_y[line] = mode1_sign_extend_28(virtuappu_mode1_io_read32(0x2Cu));
            aff_pb[line] = (int16_t)virtuappu_mode1_io_read16(0x22u);
            aff_pd[line] = (int16_t)virtuappu_mode1_io_read16(0x26u);
        }
    }
    if (do_affine_bg2 && aff_ref_x != NULL && aff_ref_y != NULL) {
        virtuappu_mode1_affine_precompute(raster_rows, aff_init_x, aff_init_y, aff_line_ref_x, aff_line_ref_y,
                                          aff_pb, aff_pd, virtuappu_mode1_bg2x_hdma_strobe,
                                          virtuappu_mode1_bg2y_hdma_strobe, aff_ref_x, aff_ref_y);
    }
    return (dispcnt & MODE1_DISP_FORCED_BLANK) != 0u;
}

/* World BGs (1..3) render into the published live aperture, or into the full
 * canvas when no aperture is published. BG0 keeps canvas coordinates so the
 * HUD/dialogue anchors remain independent of the room aperture. */
static void mode1_render_world_bg_line(int bg_index, int line, uint32_t* line_buffer) {
    if (!mode1_world_enabled || bg_index == 0) {
        mode1_render_text_bg_line_internal(bg_index, line, mode1_frame_width, 0, line_buffer, NULL);
        return;
    }
    if (line < mode1_world_y || line >= mode1_world_y + mode1_world_h)
        return; /* caller cleared the layer; margins keep the backdrop */
    mode1_render_text_bg_line_internal(bg_index, line - mode1_world_y, mode1_world_w,
                                       mode1_world_x, line_buffer, NULL);
}

void virtuappu_mode1_render_frame(const PPUMemory* ppu) {
    uint16_t dispcnt;
    int line;
    /* VPPU mode 2 = GBA affine (BG2 is rotation/scaling); mode 1 = all-tiled.
     * The two formerly lived in separate render_frame functions (mode1.c vs
     * mode2.c); they now share this one loop, differing only in how BG2 is
     * drawn and that the affine path latches DISPCNT once per frame. */
    const bool affine = (ppu->mode == 2);

    virtuappu_mode1_set_frame_geometry(ppu);
    const int frame_width = mode1_frame_width;
    const int frame_height = mode1_frame_height;
    const int raster_rows = frame_height > MODE1_GBA_HEIGHT ? frame_height : MODE1_GBA_HEIGHT;
    mode1_ui_mask_width = frame_width;
    memset(mode1_ui_mask, 0, (size_t)frame_height * (size_t)frame_width);

    mode1_frame_has_io_snapshots = false;
    dispcnt = virtuappu_mode1_io_read16(MODE1_IO_DISPCNT);
    if ((dispcnt & MODE1_DISP_FORCED_BLANK) != 0u) {
        for (line = 0; line < frame_height; ++line) {
            memset(&virtuappu_frame_buffer[(size_t)line * (size_t)mode1_frame_pitch], 0xFF,
                   (size_t)mode1_frame_width * sizeof(uint32_t));
        }
        return;
    }

    /* PORT_PARALLEL_RENDER: scanlines are parallel-renderable but the HDMA
     * pre-line callback (water-FX BG_VOFS scrolls, BLDY fades, etc.) must
     * run sequentially because it mutates the live IO register file. So:
     *   1. Sequential pass — call the callback once per line and snapshot
     *      the (post-callback) IO state for that line.
     *   2. Parallel pass — each thread points its `io_thread_override` at
     *      its line's snapshot, then renders BG / OBJ / composite normally
     *      via the existing single-line functions, which now read IO regs
     *      through the override. */
    uint16_t per_line_dispcnt[MODE1_MAX_RENDER_HEIGHT];

    /* Affine BG2 carries an internal reference point across scanlines (#132).
     * Gather per-line, post-callback inputs during the sequential pass and
     * precompute the per-line reference so the parallel pass stays independent. */
    const bool do_affine_bg2 = affine && ((dispcnt & MODE1_DISP_BG2_ON) != 0u);
    int32_t aff_ref_x[MODE1_MAX_RENDER_HEIGHT];
    int32_t aff_ref_y[MODE1_MAX_RENDER_HEIGHT];
    int32_t aff_line_ref_x[MODE1_MAX_RENDER_HEIGHT];
    int32_t aff_line_ref_y[MODE1_MAX_RENDER_HEIGHT];
    int16_t aff_pb[MODE1_MAX_RENDER_HEIGHT];
    int16_t aff_pd[MODE1_MAX_RENDER_HEIGHT];
    int32_t aff_init_x = 0;
    int32_t aff_init_y = 0;
    if (do_affine_bg2) {
        aff_init_x = mode1_sign_extend_28(virtuappu_mode1_io_read32(0x28u));
        aff_init_y = mode1_sign_extend_28(virtuappu_mode1_io_read32(0x2Cu));
    }

    /* No HDMA pre-line callback -> IO is identical on every scanline, so the
     * per-line snapshot is redundant: skip 160x1KB of memcpy on the sequential
     * critical path and point every thread's override straight at io_mem in the
     * parallel loop below. Byte-exact: each snapshot equalled io_mem anyway. */
    const bool per_line_io = (virtuappu_mode1_pre_line_callback != NULL);
    mode1_frame_has_io_snapshots = per_line_io;
    for (line = 0; line < raster_rows; ++line) {
        if (per_line_io) {
            const int snapshot_line = line < MODE1_GBA_HEIGHT ? line : MODE1_GBA_HEIGHT - 1;
            if (line < MODE1_GBA_HEIGHT) {
                virtuappu_mode1_pre_line_callback(line);
                memcpy(mode1_frame_io_snapshots[snapshot_line], mode1_memory.io_mem, MODE1_IO_MEM_SIZE);
            }
            per_line_dispcnt[line] =
#ifdef TMC_N64
                *(const uint16_t*)&mode1_frame_io_snapshots[snapshot_line][MODE1_IO_DISPCNT];
#else
                (uint16_t)mode1_frame_io_snapshots[snapshot_line][MODE1_IO_DISPCNT] |
                ((uint16_t)mode1_frame_io_snapshots[snapshot_line][MODE1_IO_DISPCNT + 1] << 8);
#endif
        } else {
            per_line_dispcnt[line] = dispcnt;
        }
        if (do_affine_bg2) {
            /* Live IO is this line's post-callback state here (== the snapshot),
             * matching what the old mode2.c read inline. */
            aff_line_ref_x[line] = mode1_sign_extend_28(virtuappu_mode1_io_read32(0x28u));
            aff_line_ref_y[line] = mode1_sign_extend_28(virtuappu_mode1_io_read32(0x2Cu));
            aff_pb[line] = (int16_t)virtuappu_mode1_io_read16(0x22u);
            aff_pd[line] = (int16_t)virtuappu_mode1_io_read16(0x26u);
        }
    }
    if (do_affine_bg2) {
        virtuappu_mode1_affine_precompute(raster_rows, aff_init_x, aff_init_y, aff_line_ref_x, aff_line_ref_y,
                                          aff_pb, aff_pd, virtuappu_mode1_bg2x_hdma_strobe,
                                          virtuappu_mode1_bg2y_hdma_strobe, aff_ref_x, aff_ref_y);
    }

    /* Palette RAM is final for the frame here (fade engine wrote it before this
     * render; the pre-line callback above only touches IO regs). Convert it to
     * the ABGR LUTs once so the parallel per-pixel loops below do a single table
     * load instead of the rgb555->ABGR math. */
    virtuappu_mode1_publish_palette_luts();

#pragma omp parallel for schedule(static)
    for (line = 0; line < frame_height; ++line) {
        /* Affine (mode 2) renders every scanline against the frame-start DISPCNT
         * (GBA latches BGMODE once per frame; matches the former mode2.c). The
         * tiled path honours per-line DISPCNT for HBlank-DMA changes. */
        uint16_t line_dispcnt = affine ? dispcnt : per_line_dispcnt[line];
        uint32_t bg_layers[MODE1_GBA_BG_COUNT][MODE1_GBA_WIDTH];
        uint32_t obj_layer[MODE1_GBA_WIDTH];
        uint8_t obj_priority[MODE1_GBA_WIDTH];
        bool obj_1d = (line_dispcnt & MODE1_DISP_OBJ_1D) != 0u;
        const uint8_t* prev_override = virtuappu_mode1_io_thread_override;

        virtuappu_mode1_io_thread_override = per_line_io ? mode1_frame_io_snapshots[line < MODE1_GBA_HEIGHT ? line : MODE1_GBA_HEIGHT - 1]
                                                          : mode1_memory.io_mem;

        memset(obj_layer, 0, (size_t)mode1_frame_width * sizeof(uint32_t));
        memset(obj_priority, 0xFF, (size_t)mode1_frame_width);

        /* Clear + render only ENABLED BGs: composite reads bg_layers[b][x] only
         * where bg_enabled[b] (same line_dispcnt), so a disabled BG's buffer is
         * never touched — clearing all four every line was wasted bandwidth.
         * BG priority buffers stay NULL (composite resolves order from BGCNT). */
        if ((line_dispcnt & MODE1_DISP_BG0_ON) != 0u) {
            memset(bg_layers[0], 0, (size_t)mode1_frame_width * sizeof(uint32_t));
            virtuappu_mode1_render_text_bg_line(0, line, bg_layers[0], NULL);
        }
        if ((line_dispcnt & MODE1_DISP_BG1_ON) != 0u) {
            memset(bg_layers[1], 0, (size_t)mode1_frame_width * sizeof(uint32_t));
            mode1_render_world_bg_line(1, line, bg_layers[1]);
        }
        if ((line_dispcnt & MODE1_DISP_BG2_ON) != 0u) {
            memset(bg_layers[2], 0, (size_t)mode1_frame_width * sizeof(uint32_t));
            if (affine) {
                mode1_render_affine_bg2_line(frame_width, aff_ref_x[line], aff_ref_y[line], bg_layers[2], NULL);
            } else {
                mode1_render_world_bg_line(2, line, bg_layers[2]);
            }
        }
        if ((line_dispcnt & MODE1_DISP_BG3_ON) != 0u) {
            /* Mode 2 (affine) has no BG3 tile render, but if DISPCNT enables it
             * the composite still reads bg_layers[3]; keep it cleared to 0 so it
             * contributes nothing — exactly as the old unconditional memset did. */
            memset(bg_layers[3], 0, (size_t)mode1_frame_width * sizeof(uint32_t));
            if (!affine) {
                mode1_render_world_bg_line(3, line, bg_layers[3]);
            }
        }
        if ((line_dispcnt & MODE1_DISP_OBJ_ON) != 0u) {
            virtuappu_mode1_render_obj_line(line, obj_1d, obj_layer, obj_priority);
        } else {
            /* render_obj_line clears the per-thread obj_window / obj_semitrans
             * masks at line start; when DISPCNT.12 (OBJ) is off they'd retain
             * whatever scanline this WORKER THREAD rendered last — composite
             * still reads them when OBJWIN (DISPCNT.15) is on, consuming a
             * stale mask. Hardware: no OBJ rendering pass -> empty OBJ window. */
            memset(virtuappu_mode1_obj_window, 0, (size_t)mode1_frame_width);
            memset(virtuappu_mode1_obj_semitrans, 0, (size_t)mode1_frame_width);
        }

        virtuappu_mode1_composite_line(line, bg_layers, NULL, obj_layer, obj_priority, line_dispcnt);

        virtuappu_mode1_io_thread_override = prev_override;
    }
}
