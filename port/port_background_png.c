#include "port_background_png.h"

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void png_error_quiet(png_structp png, png_const_charp message) {
    (void)message;
    png_longjmp(png, 1);
}

static void png_warning_quiet(png_structp png, png_const_charp message) {
    (void)png;
    (void)message;
}

bool Port_Background_LoadPng(const char* root, PortBackgroundContext context,
                             PortAspectMode aspect, bool parity,
                             uint8_t* rgba, size_t capacity) {
    const PortScreenBackground* background = Port_Background_Get(context, parity);
    const PortViewportProfile* profile = Port_Viewport_ProfileForAspect(aspect);
    char path[4096];
    FILE* file;
    png_structp png;
    png_infop info;
    png_bytep rows[192];
    uint8_t* pixels;
    size_t bytes;
    int path_length;
    int ok = 0;

    if (!root || !rgba || !profile || background->source != PORT_BACKGROUND_SOURCE_IMAGE ||
        !background->filename || profile->width <= 0 || profile->width > 569 ||
        profile->height <= 0 || profile->height > 192)
        return false;
    bytes = (size_t)profile->width * (size_t)profile->height * 4;
    if (capacity < bytes) return false;
    path_length = snprintf(path, sizeof(path), "%s/override/%s/%s", root,
                           profile->directory, background->filename);
    if (path_length < 0 || (size_t)path_length >= sizeof(path)) return false;
    file = fopen(path, "rb");
    if (!file) return false;
    pixels = (uint8_t*)malloc(bytes);
    if (!pixels) { fclose(file); return false; }
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, png_error_quiet, png_warning_quiet);
    if (!png) { free(pixels); fclose(file); return false; }
    info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png, NULL, NULL); free(pixels); fclose(file); return false; }
    if (!setjmp(png_jmpbuf(png))) {
        png_set_user_limits(png, (png_uint_32)profile->width, (png_uint_32)profile->height);
#ifdef PNG_SET_USER_LIMITS_SUPPORTED
        /* Bound retained ancillary metadata as well as individual chunks. */
        png_set_chunk_cache_max(png, 16);
        png_set_chunk_malloc_max(png, 256 * 1024);
#endif
        png_init_io(png, file);
        png_read_info(png, info);
        png_uint_32 width = png_get_image_width(png, info);
        png_uint_32 height = png_get_image_height(png, info);
        int type = png_get_color_type(png, info);
        int depth = png_get_bit_depth(png, info);
        int transparency = png_get_valid(png, info, PNG_INFO_tRNS) != 0;
        if (width != (png_uint_32)profile->width || height != (png_uint_32)profile->height)
            png_error(png, "wrong dimensions");
        if (depth == 16) png_set_strip_16(png);
        if (type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
        if (type == PNG_COLOR_TYPE_GRAY && depth < 8) png_set_expand_gray_1_2_4_to_8(png);
        if (transparency) png_set_tRNS_to_alpha(png);
        if (type == PNG_COLOR_TYPE_GRAY || type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
        if (!(type & PNG_COLOR_MASK_ALPHA) && !transparency) png_set_add_alpha(png, 255, PNG_FILLER_AFTER);
        png_set_interlace_handling(png);
        png_read_update_info(png, info);
        if (png_get_rowbytes(png, info) != (png_size_t)width * 4) png_error(png, "unexpected row size");
        for (png_uint_32 y = 0; y < height; ++y) rows[y] = pixels + (size_t)y * width * 4;
        png_read_image(png, rows);
        png_read_end(png, info);
        ok = 1;
    }
    png_destroy_read_struct(&png, &info, NULL);
    fclose(file);
    if (ok) memcpy(rgba, pixels, bytes);
    free(pixels);
    return ok != 0;
}
