/* Expected pixels and hardware-line bounds for the production software PPU. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "cpu/mode1.h"
#include "virtuappu.h"
#include "ppu_snapshot.h"
#include "port_hdma.h"
#include "port_viewport.h"

#ifdef TMC_N64
int main(void) {
    PPUMemory p = {0};
    p.frame_width = 569;
    p.frame_pitch = 569;
    p.frame_height = 192;
    virtuappu_mode1_set_frame_geometry(&p);
    assert(virtuappu_mode1_frame_width() == 240 && virtuappu_mode1_frame_pitch() == 240);
    assert(MODE1_MAX_RENDER_HEIGHT == 160);
    assert(virtuappu_mode1_frame_height() == 160);
    assert(VIRTUAPPU_FRAME_BUFFER_SIZE == 240 * 160);
    assert(Port_Viewport_AspectSupported(PORT_ASPECT_NATIVE_3_2, 240, 160));
    for (int aspect = 1; aspect < PORT_ASPECT_COUNT; ++aspect) {
        assert(!Port_Viewport_AspectSupported((PortAspectMode)aspect, 569, 192));
        assert(Port_Viewport_Publish((PortAspectMode)aspect, false, 569, 192, 569));
        assert(Port_Viewport_Get()->effectiveAspect == PORT_ASPECT_NATIVE_3_2);
        assert(Port_Viewport_Get()->canvas.width == 240 && Port_Viewport_Get()->canvas.height == 160);
    }
    puts("N64 geometry remains native: PASS");
    return 0;
}
#else
static uint8_t io[0x400], vram[0x18000];
static uint16_t bgpal[256], objpal[256], oam[512];
static const uint32_t canary = 0x12345678;
static const int pitch = MODE1_GBA_WIDTH;
static int width;

static void io16(unsigned offset, unsigned value) {
    io[offset] = (uint8_t)value;
    io[offset + 1] = (uint8_t)(value >> 8);
}

static void reset(void) {
    memset(io, 0, sizeof(io));
    memset(vram, 0, sizeof(vram));
    memset(bgpal, 0, sizeof(bgpal));
    memset(objpal, 0, sizeof(objpal));
    memset(oam, 0, sizeof(oam));
    for (unsigned i = 0; i < 128; ++i) oam[i * 4] = 0x200;
    for (size_t i = 0; i < VIRTUAPPU_FRAME_BUFFER_SIZE; ++i) virtuappu_frame_buffer[i] = canary;
    virtuappu_mode1_pre_line_callback = NULL;
    virtuappu_mode1_continue_constant_hdma = false;
    virtuappu_mode1_bg2x_hdma_strobe = false;
    virtuappu_mode1_bg2y_hdma_strobe = false;
    bgpal[1] = 0x001f;
    bgpal[2] = 0x03e0;
    bgpal[3] = 0x7c00;
}

static PPUMemory geometry(int height, int mode) {
    PPUMemory p = {0};
    p.frame_width = (uint16_t)width;
    p.frame_height = (uint16_t)height;
    p.frame_pitch = (uint16_t)pitch;
    p.mode = (uint8_t)mode;
    return p;
}

static void untouched(int height) {
    for (int y = 0; y < height; ++y)
        for (int x = width; x < pitch; ++x)
            assert(virtuappu_frame_buffer[y * pitch + x] == canary);
    assert(virtuappu_frame_buffer[height * pitch] == canary);
}

static void stripes(bool affine) {
    io16(0, affine ? 0x401 : 0x200); // BG2 affine or BG1 text
    io16(affine ? 0xc : 0xa, affine ? 0x5000 : 0x1000); // screenbase 16
    if (affine) {
        io16(0x20, 0x100); // PA: one source pixel per X
        io16(0x26, 0x100); // PD: one source pixel per Y
        for (int tile = 1; tile <= 3; ++tile) memset(vram + tile * 64, tile, 64);
    } else {
        for (int tile = 1; tile <= 3; ++tile) memset(vram + tile * 32, tile * 0x11, 32);
    }
    for (int row = 0; row < 32; ++row) {
        unsigned tile = row >= 23 ? 3 : row >= 20 ? 2 : 1;
        for (int col = 0; col < 32; ++col) {
            const int offset = 0x8000 + (row * 32 + col) * (affine ? 1 : 2);
            vram[offset] = (uint8_t)tile;
        }
    }
}

static void check_stripes(bool affine) {
    reset();
    stripes(affine);
    PPUMemory p = geometry(192, affine ? 2 : 1);
    virtuappu_mode1_render_frame(&p);
    assert(virtuappu_mode1_frame_height() == 192);
    assert(virtuappu_frame_buffer[159 * pitch] == 0xff0000f8);
    assert(virtuappu_frame_buffer[160 * pitch] == 0xff00f800);
    assert(virtuappu_frame_buffer[191 * pitch] == 0xfff80000);
    untouched(192);
}

/* A black palette fade changes live palette entries, not scanline IO. Verify
 * both the old native area and newly exposed rows at each arrival step. */
static void check_tall_palette_fade(void) {
    reset();
    stripes(false);
    PPUMemory p = geometry(192, 1);
    for (unsigned red = 0; red <= 31; ++red) {
        bgpal[1] = bgpal[2] = bgpal[3] = (uint16_t)red;
        virtuappu_mode1_render_frame(&p);
        assert(virtuappu_mode1_frame_height() == 192);
        const uint32_t expected = 0xff000000u | (red << 3);
        for (int y = 0; y < 192; ++y)
            for (int x = 0; x < 240; ++x)
                assert(virtuappu_frame_buffer[y * pitch + x] == expected);
    }
    untouched(192);
}

/* A static window is just as unclassified as an HDMA-driven one. */
static void check_static_window_fallback(void) {
    reset();
    stripes(false);
    PPUMemory p = geometry(192, 1);
    for (int window = 0x20; window <= 0x80; window <<= 1) {
        io[1] |= (uint8_t)window;
        virtuappu_mode1_render_frame(&p);
        assert(virtuappu_mode1_frame_height() == 160);
        untouched(160);
        io[1] &= (uint8_t)~window;
    }
    virtuappu_mode1_render_frame(&p);
    assert(virtuappu_mode1_frame_height() == 192);
    untouched(192);
}

static int callbacks;
static uint16_t hardware_table[160];
static void scanline(int line) {
    assert(line == callbacks && line < 160);
    io16(0x12, hardware_table[line]);
    ++callbacks;
}

static void counted_hdma(int line) {
    assert(line == callbacks && line < 160);
    ++callbacks;
    port_hdma_step_line(line);
}

static void check_constant_brightness(void) {
    static uint32_t expected[MODE1_GBA_WIDTH * MODE1_MAX_RENDER_HEIGHT];
    static uint8_t rows[MODE1_MAX_RENDER_HEIGHT * MODE1_IO_MEM_SIZE];
    uint16_t controls[MODE1_MAX_RENDER_HEIGHT];
    int32_t x[MODE1_MAX_RENDER_HEIGHT], y[MODE1_MAX_RENDER_HEIGHT];
    uint16_t brightness = 8;
    reset();
    stripes(false);
    io16(0x50, 0x82); /* brighten BG1 */
    io16(0x54, brightness);
    PPUMemory p = geometry(192, 1);
    virtuappu_mode1_render_frame(&p);
    memcpy(expected, virtuappu_frame_buffer, sizeof(expected));

    io16(0x54, 0);
    port_hdma_register(0, &brightness, io + 0x54, 0x160, 1);
    assert(port_hdma_can_continue_brightness(io, sizeof(io)));
    assert(!port_hdma_can_continue_brightness(NULL, sizeof(io)));
    assert(!port_hdma_can_continue_brightness(io, 0x55));
    assert(port_hdma_classify(io, sizeof(io)) == (PORT_HDMA_BLEND | PORT_HDMA_FIXED_SOURCE));
    for (int window = 0x20; window <= 0x80; window <<= 1) {
        io[1] |= (uint8_t)window;
        assert(!port_hdma_can_continue_brightness(io, sizeof(io)));
        io[1] &= (uint8_t)~window;
    }
    virtuappu_mode1_pre_line_callback = counted_hdma;
    virtuappu_mode1_continue_constant_hdma = true;
    callbacks = 0;
    virtuappu_mode1_render_frame(&p);
    assert(callbacks == 160 && virtuappu_mode1_frame_height() == 192);
    for (int row = 0; row < 192; ++row)
        for (int col = 0; col < width; ++col)
            assert(virtuappu_frame_buffer[row * pitch + col] == expected[row * pitch + col]);
    untouched(192);

    port_hdma_vblank_reset();
    memset(rows, 0xa5, sizeof(rows));
    callbacks = 0;
    virtuappu_mode1_prepare_frame(&p, rows, controls, x, y, NULL);
    assert(callbacks == 160 && virtuappu_mode1_frame_height() == 192);
    assert(rows[159 * MODE1_IO_MEM_SIZE + 0x54] == 8);
    assert(rows[160 * MODE1_IO_MEM_SIZE + 0x54] == 8);
    assert(rows[191 * MODE1_IO_MEM_SIZE + 0x54] == 8);
    port_hdma_unregister(0);
    assert(!port_hdma_can_continue_brightness(io, sizeof(io)));

    /* Unknown sources, drifting destinations, aliasing and multiple effects
     * cannot claim a stable continuation. */
    port_hdma_register(0, hardware_table, io + 0x54, 0x60, 1);
    assert(!port_hdma_can_continue_brightness(io, sizeof(io)));
    assert(port_hdma_classify(io, sizeof(io)) == (PORT_HDMA_BLEND | PORT_HDMA_INCREMENTING_SOURCE));
    port_hdma_register(0, &brightness, io + 0x54, 0x100, 1);
    assert(!port_hdma_can_continue_brightness(io, sizeof(io)));
    assert(port_hdma_classify(io, sizeof(io)) & PORT_HDMA_OTHER); /* increment reaches other registers */
    port_hdma_register(0, &brightness, io + 0x54, 0x1e0, 1); /* prohibited source mode */
    assert(!port_hdma_can_continue_brightness(io, sizeof(io)));
    assert(port_hdma_classify(io, sizeof(io)) & PORT_HDMA_OTHER);
    port_hdma_register(0, &brightness, io + 0x54, 0x0e0, 1); /* decrement source */
    assert(!port_hdma_can_continue_brightness(io, sizeof(io)));
    assert(port_hdma_classify(io, sizeof(io)) == (PORT_HDMA_BLEND | PORT_HDMA_OTHER));
    port_hdma_register(0, &brightness, io + 0x54, 0x120, 1); /* decrement destination */
    assert(!port_hdma_can_continue_brightness(io, sizeof(io)));
    assert(port_hdma_classify(io, sizeof(io)) == (PORT_HDMA_FIXED_SOURCE | PORT_HDMA_OTHER));
    port_hdma_register(0, &brightness, io + 0x54, 0x160, 2);
    assert(!port_hdma_can_continue_brightness(io, sizeof(io)));
    port_hdma_register(0, &brightness, io + 0x54, 0x560, 1);
    assert(!port_hdma_can_continue_brightness(io, sizeof(io)));
    port_hdma_register(0, io + 0x54, io + 0x54, 0x160, 1);
    assert(!port_hdma_can_continue_brightness(io, sizeof(io)));
    port_hdma_register(0, &brightness, io + 0x54, 0x160, 1);
    port_hdma_register(1, &brightness, io + 0x40, 0x160, 1);
    assert(!port_hdma_can_continue_brightness(io, sizeof(io)));
    assert(port_hdma_classify(io, sizeof(io)) == (PORT_HDMA_BLEND | PORT_HDMA_WINDOW | PORT_HDMA_FIXED_SOURCE));
    port_hdma_unregister(0);
    port_hdma_unregister(1);
    port_hdma_register(0, hardware_table, io + 0x20, 0x60, 8);
    assert(port_hdma_classify(io, sizeof(io)) == (PORT_HDMA_AFFINE | PORT_HDMA_INCREMENTING_SOURCE));
    port_hdma_register(0, hardware_table, io + 0x1c, 0x60, 1);
    assert(port_hdma_classify(io, sizeof(io)) == (PORT_HDMA_BG_SCROLL | PORT_HDMA_INCREMENTING_SOURCE));
    port_hdma_register(0, hardware_table, io + 0x0e, 0x60, 1);
    assert(port_hdma_classify(io, sizeof(io)) == (PORT_HDMA_BG_CONTROL | PORT_HDMA_INCREMENTING_SOURCE));
    port_hdma_unregister(0);
    virtuappu_mode1_pre_line_callback = NULL;
    virtuappu_reset();
    assert(!virtuappu_mode1_continue_constant_hdma);
}

static void check_white_bg_vs_backdrop(void) {
    reset();
    bgpal[0] = 0x7fff;
    bgpal[8] = 0x7fff;
    memset(vram + 32, 0x88, 32); /* one opaque white BG2 tile */
    vram[0x8000] = 1;           /* subsequent BG2 tiles are transparent */
    io16(0, 0x400);
    io16(0x0c, 0x1000);        /* BG2 screenbase 16 */
    io16(0x54, 8);
    PPUMemory p = geometry(160, 1);
    io16(0x50, 0xe0);          /* darken backdrop only */
    virtuappu_mode1_render_frame(&p);
    assert(virtuappu_frame_buffer[0] == 0xfff8f8f8);
    assert(virtuappu_frame_buffer[8] == 0xff808080);
    io16(0x50, 0xc4);          /* darken BG2 only */
    virtuappu_mode1_render_frame(&p);
    assert(virtuappu_frame_buffer[0] == 0xff808080);
    assert(virtuappu_frame_buffer[8] == 0xfff8f8f8);
}

static void check_raster(void) {
    reset();
    stripes(false);
    PPUMemory p = geometry(192, 1);
    for (int i = 0; i < 160; ++i) hardware_table[i] = (uint16_t)(i + 1);
    callbacks = 0;
    virtuappu_mode1_pre_line_callback = scanline;
    virtuappu_mode1_render_frame(&p);
    assert(callbacks == 160);
    assert(virtuappu_mode1_frame_height() == 160);
    assert(virtuappu_mode1_frame_io_for_line(-1)[0x12] == 1);
    assert(virtuappu_mode1_frame_io_for_line(7)[0x12] == 8);
    assert(virtuappu_mode1_frame_io_for_line(200)[0x12] == 160);
    untouched(160);
    // Cropping the visible frame must not shorten the hardware sequence.
    p.frame_height = 80;
    callbacks = 0;
    virtuappu_mode1_render_frame(&p);
    assert(callbacks == 160 && virtuappu_mode1_frame_height() == 80);

    static uint8_t rows[MODE1_MAX_RENDER_HEIGHT * MODE1_IO_MEM_SIZE];
    uint16_t controls[MODE1_MAX_RENDER_HEIGHT];
    int32_t x[MODE1_MAX_RENDER_HEIGHT], y[MODE1_MAX_RENDER_HEIGHT];
    memset(rows, 0xa5, sizeof(rows));
    memset(controls, 0xa5, sizeof(controls));
    p.frame_height = 192;
    callbacks = 0;
    virtuappu_mode1_prepare_frame(&p, rows, controls, x, y, NULL);
    assert(callbacks == 160 && virtuappu_mode1_frame_height() == 160);
    assert(rows[160 * MODE1_IO_MEM_SIZE] == 0xa5 && controls[160] == 0xa5a5);

    // Uniform IO needs one snapshot; affine references continue mathematically.
    reset();
    stripes(true);
    p.mode = 2;
    memset(rows, 0xa5, sizeof(rows));
    virtuappu_mode1_prepare_frame(&p, rows, controls, x, y, NULL);
    assert(virtuappu_mode1_frame_height() == 192);
    assert(x[191] == 0 && y[160] == 160 * 256 && y[191] == 191 * 256);
    assert(rows[MODE1_IO_MEM_SIZE] == 0xa5);

    // The HDMA layer itself rejects accidental extra/negative callback lines.
    uint16_t dest = 0;
    for (int i = 0; i < 160; ++i) hardware_table[i] = (uint16_t)(i + 1);
    port_hdma_register(0, hardware_table, &dest, 0x40, 1);
    for (int line = 0; line < 160; ++line) port_hdma_step_line(line);
    assert(dest == hardware_table[158]);
    port_hdma_step_line(-1);
    for (int line = 160; line < 200; ++line) port_hdma_step_line(line);
    assert(dest == hardware_table[158]);
    port_hdma_vblank_reset();
    port_hdma_step_line(0);
    assert(dest == hardware_table[0]);
    port_hdma_unregister(0);
}

static void check_snapshot(void) {
    static uint8_t bytes[0x19000];
    reset();
    stripes(true);
    bgpal[0] = 0x1234;
    objpal[255] = 0x4567;
    oam[511] = 0x789a;
    VirtuaPPUMode1GbaMemory mem = { io, vram, bgpal, objpal, oam };
    PPUMemory p = geometry(192, 2), loaded = {0};
    FILE* f = tmpfile();
    assert(f && virtuappu_snapshot_write(f, &mem, &p));
    rewind(f);
    size_t size = fread(bytes, 1, sizeof(bytes), f);
    assert(size == 36 + 0x18c00 && memcmp(bytes, "PPU2", 4) == 0);
    rewind(f);
    bgpal[0] = objpal[255] = oam[511] = 0;
    assert(virtuappu_snapshot_read(f, &mem, &loaded));
    assert(loaded.frame_width == width && loaded.frame_height == 192 && loaded.frame_pitch == pitch);
    assert(loaded.mode == 2 && bgpal[0] == 0x1234 && objpal[255] == 0x4567 && oam[511] == 0x789a);
    fclose(f);

    // Legacy files have no geometry: explicitly default to native, independent
    // of the reader's compiled width capacity.
    f = tmpfile();
    assert(f);
    assert(fwrite("PPU1", 1, 4, f) == 4);
    assert(fwrite(bytes + 4, 1, 20, f) == 20);
    assert(fwrite(bytes + 36, 1, size - 36, f) == size - 36);
    rewind(f);
    assert(virtuappu_snapshot_read(f, &mem, &loaded));
    assert(loaded.frame_width == 240 && loaded.frame_height == 160 && loaded.frame_pitch == 240);
    fclose(f);

    // Reject malformed magic, sizes, geometry, truncation and trailing data.
    const unsigned offsets[] = { 0, 4, 24, 28, 32 };
    for (unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
        unsigned at = offsets[i];
        uint8_t saved[4];
        memcpy(saved, bytes + at, 4);
        memset(bytes + at, 0xff, 4);
        f = tmpfile();
        assert(f && fwrite(bytes, 1, size, f) == size);
        rewind(f);
        assert(!virtuappu_snapshot_read(f, &mem, &loaded));
        fclose(f);
        memcpy(bytes + at, saved, 4);
    }
    const size_t lengths[] = { 0, 3, 23, 35, 36, size - 1, size + 1 };
    for (unsigned i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i) {
        f = tmpfile();
        assert(f && fwrite(bytes, 1, lengths[i], f) == lengths[i]);
        rewind(f);
        assert(!virtuappu_snapshot_read(f, &mem, &loaded));
        fclose(f);
    }
}

static void check_below_bg_edges(void) {
    reset();
    io16(0, 0x0e00); /* BG1, BG2, BG3 */
    io16(0x0a, (17 << 8) | 3); /* BG1 priority 3 */
    io16(0x0c, 1);              /* source BG2 priority 1 */
    io16(0x0e, (16 << 8) | 2); /* BG3 priority 2 */
    memset(vram + 32, 0x11, 32);
    memset(vram + 64, 0x22, 32);
    for (int row = 0; row < 32; ++row) for (int col = 0; col < 32; ++col) {
        vram[0x8000 + 2 * (row * 32 + col)] = col == 0 ? 1 : 0;
        vram[0x8800 + 2 * (row * 32 + col)] = 2;
    }
    PPUMemory p = geometry(160, 1);
    virtuappu_mode1_render_frame(&p);
    unsigned layer = 99; uint32_t color = 0;
    assert(virtuappu_mode1_sample_below_bg(0, 0, 2, &layer, &color));
    assert(layer == 3 && color == 0xff0000f8);
    assert(virtuappu_mode1_sample_below_bg(0, 239, 2, &layer, &color));
    assert(layer == 1 && color == 0xff00f800);
    io16(0x0a, (17 << 8)); /* BG1 is now above BG2 */
    assert(virtuappu_mode1_sample_below_bg(0, 239, 2, &layer, &color));
    assert(layer == 5 && color == 0xff000000);
    io16(0, 0x0e01);
    assert(!virtuappu_mode1_sample_below_bg(0, 0, 2, &layer, &color));
}

int main(void) {
    VirtuaPPUMode1GbaMemory mem = { io, vram, bgpal, objpal, oam };
    virtuappu_mode1_bind_gba_memory(&mem);
    // Exercise the full future width/height allocation envelope without
    // enabling any runtime profile. Smaller builds retain padded-pitch cases.
    width = MODE1_GBA_WIDTH >= 569 ? 569 : MODE1_GBA_WIDTH >= 256 ? 256 : 240;
    check_below_bg_edges();
    reset();
    PPUMemory p = geometry(0, 1);
    virtuappu_mode1_set_frame_geometry(&p);
    assert(virtuappu_mode1_frame_height() == 160);
    p.frame_height = 65535;
    virtuappu_mode1_set_frame_geometry(&p);
    assert(virtuappu_mode1_frame_height() == 192);
    check_stripes(false);
    check_stripes(true);
    check_tall_palette_fade();
    check_raster();
    check_static_window_fallback();
    check_constant_brightness();
    check_white_bg_vs_backdrop();
    check_snapshot();
    reset();
    p = geometry(192, 1);
    io16(0, 0x80); // Forced blank covers the complete render extent.
    virtuappu_mode1_render_frame(&p);
    assert(virtuappu_frame_buffer[191 * pitch + width - 1] == 0xffffffff);
    untouched(192);
    reset();
    io16(0, 0x1000); // Native hidden-sprite Y interpretation must not change.
    oam[0] = 160;
    objpal[1] = 0x001f;
    memset(vram + 0x10000, 0x11, 32);
    virtuappu_mode1_render_frame(&p);
    assert(virtuappu_frame_buffer[160 * pitch] == 0xff000000);
    // Supersampling accepts the effective tall extent and retains signed GBA
    // OAM decoding. This sprite starts below the HUD and crosses row 160.
    static uint32_t scaled[MODE1_GBA_WIDTH * 2 * 192 * 2];
    for (size_t i = 0; i < sizeof(scaled) / sizeof(scaled[0]); ++i) scaled[i] = canary;
    oam[0] = 152 | 0x100; // affine, 16x16, matrix zero
    oam[1] = 0x4000;
    oam[3] = oam[15] = 0x100;
    memset(vram + 0x10000, 0x11, 128);
    io16(0, 0x1040);
    virtuappu_mode1_render_affine_obj_overlay(scaled, width * 2, 192 * 2, 2);
    assert(scaled[160 * 2 * width * 2] == 0xff0000f8);
    puts("PPU height, raster bounds, pixels and snapshot formats: PASS");
    return 0;
}
#endif
