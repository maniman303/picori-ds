#include "port_object_override.h"
#include "region.h"

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT_OBJECT_OVERRIDE_MAX_REQUESTS 8
#define PORT_OBJECT_OVERRIDE_MAX_IMAGES 32
#define PORT_OBJECT_OVERRIDE_MAX_DIMENSION 4096

typedef struct {
    char filename[96];
    uint8_t* rgba;
    int width;
    int height;
    bool loaded;
} PortObjectOverrideImage;

typedef struct {
    char name[64];
    int image;
    int palette;
    int x;
    int y;
    int width;
    int height;
} PortObjectOverrideRequest;

/* Layout matches the engine's struct_020354C0 (include/structures.h) without
 * pulling game headers into this C-only port module. */
typedef struct {
    uint8_t unk0;
    uint8_t unk1;
    uint16_t unk2;
} PortObjectFadeBank;
extern PortObjectFadeBank gUnk_020354C0[0x20];

static char sAssetRoot[4096];
static PortObjectOverrideImage sImages[PORT_OBJECT_OVERRIDE_MAX_IMAGES];
static int sImageCount;
static PortObjectOverrideRequest sRequests[PORT_OBJECT_OVERRIDE_MAX_REQUESTS];
static int sRequestCount;

static void png_error_quiet(png_structp png, png_const_charp message) {
    (void)message;
    png_longjmp(png, 1);
}

static void png_warning_quiet(png_structp png, png_const_charp message) {
    (void)png;
    (void)message;
}

static void FreeImages(void) {
    for (int i = 0; i < sImageCount; ++i)
        free(sImages[i].rgba);
    memset(sImages, 0, sizeof(sImages));
    sImageCount = 0;
}

void Port_ObjectOverride_Init(const char* asset_root) {
    size_t length = 0;
    FreeImages();
    sRequestCount = 0;
    sAssetRoot[0] = '\0';
    if (!asset_root || !*asset_root)
        return;
    length = strlen(asset_root);
    if (length >= sizeof(sAssetRoot))
        length = sizeof(sAssetRoot) - 1;
    memcpy(sAssetRoot, asset_root, length);
    while (length > 0 && (sAssetRoot[length - 1] == '/' || sAssetRoot[length - 1] == '\\'))
        --length;
    sAssetRoot[length] = '\0';
}

void Port_ObjectOverride_RequestReload(void) {
    FreeImages();
}

void Port_ObjectOverride_BeginFrame(void) {
    sRequestCount = 0;
}

static bool LoadPng(const char* path, int* out_width, int* out_height, uint8_t** out_rgba) {
    FILE* file = NULL;
    png_structp png = NULL;
    png_infop info = NULL;
    png_bytep* rows = NULL;
    uint8_t* pixels = NULL;
    int width = 0, height = 0;
    bool ok = false;

    if (!path || !out_width || !out_height || !out_rgba)
        return false;
    file = fopen(path, "rb");
    if (!file)
        return false;
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, png_error_quiet, png_warning_quiet);
    if (!png)
        goto done;
    info = png_create_info_struct(png);
    if (!info)
        goto done;
    if (setjmp(png_jmpbuf(png)))
        goto done;
    png_set_user_limits(png, PORT_OBJECT_OVERRIDE_MAX_DIMENSION, PORT_OBJECT_OVERRIDE_MAX_DIMENSION);
#ifdef PNG_SET_USER_LIMITS_SUPPORTED
    png_set_chunk_cache_max(png, 16);
    png_set_chunk_malloc_max(png, 256 * 1024);
#endif
    png_init_io(png, file);
    png_read_info(png, info);
    width = (int)png_get_image_width(png, info);
    height = (int)png_get_image_height(png, info);
    {
        const int type = png_get_color_type(png, info);
        const int depth = png_get_bit_depth(png, info);
        const int transparency = png_get_valid(png, info, PNG_INFO_tRNS) != 0;
        if (width <= 0 || height <= 0 || width > PORT_OBJECT_OVERRIDE_MAX_DIMENSION ||
            height > PORT_OBJECT_OVERRIDE_MAX_DIMENSION)
            goto done;
        if (depth == 16)
            png_set_strip_16(png);
        if (type == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png);
        if (type == PNG_COLOR_TYPE_GRAY && depth < 8)
            png_set_expand_gray_1_2_4_to_8(png);
        if (transparency)
            png_set_tRNS_to_alpha(png);
        if (type == PNG_COLOR_TYPE_GRAY || type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png);
        if (!(type & PNG_COLOR_MASK_ALPHA) && !transparency)
            png_set_add_alpha(png, 255, PNG_FILLER_AFTER);
    }
    png_set_interlace_handling(png);
    png_read_update_info(png, info);
    if (png_get_rowbytes(png, info) != (png_size_t)width * 4u)
        goto done;
    pixels = (uint8_t*)malloc((size_t)width * (size_t)height * 4u);
    if (!pixels)
        goto done;
    rows = (png_bytep*)malloc((size_t)height * sizeof(*rows));
    if (!rows)
        goto done;
    for (int y = 0; y < height; ++y)
        rows[y] = pixels + (size_t)y * (size_t)width * 4u;
    png_read_image(png, rows);
    png_read_end(png, info);
    *out_width = width;
    *out_height = height;
    *out_rgba = pixels;
    pixels = NULL;
    ok = true;

done:
    free(rows);
    free(pixels);
    if (png)
        png_destroy_read_struct(&png, info ? &info : NULL, NULL);
    if (file)
        fclose(file);
    return ok;
}

static const char* LanguageSuffix(int language) {
    /* gSaveHeader->language is the direct Language enum on USA/JP but the
     * EU resource slot (LANGUAGE_EN occupies slot 2, then FR/DE/ES/IT). */
    if (REGION_IS_EU) {
        switch (language) {
            case 0: return "japanese";
            case 2: return "english";
            case 3: return "french";
            case 4: return "german";
            case 5: return "spanish";
            case 6: return "italian";
            default: return NULL;
        }
    }
    switch (language) {
        case 0: return "japanese";
        case 1: return "english";
        case 2: return "french";
        case 3: return "german";
        case 4: return "spanish";
        case 5: return "italian";
        default: return NULL;
    }
}

static int FindImage(const char* filename) {
    for (int i = 0; i < sImageCount; ++i) {
        if (strcmp(sImages[i].filename, filename) == 0)
            return i;
    }
    return -1;
}

static int LoadImage(const char* filename) {
    char path[4096];
    int index = FindImage(filename);
    if (index >= 0)
        return sImages[index].loaded ? index : -1;
    if (sImageCount >= PORT_OBJECT_OVERRIDE_MAX_IMAGES)
        return -1;
    index = sImageCount++;
    memset(&sImages[index], 0, sizeof(sImages[index]));
    snprintf(sImages[index].filename, sizeof(sImages[index].filename), "%s", filename);
    if (!sAssetRoot[0])
        return -1;
    snprintf(path, sizeof(path), "%s/override/objects/%s", sAssetRoot, filename);
    if (!LoadPng(path, &sImages[index].width, &sImages[index].height, &sImages[index].rgba)) {
        /* Repository development builds resolve runtime assets next to the
         * executable, while artists edit the source tree at assets/override.
         * Fall back to that repository-relative path when the runtime tree
         * does not have the file. */
        char source_path[4096];
        snprintf(source_path, sizeof(source_path), "assets/override/objects/%s", filename);
        if (strcmp(source_path, path) == 0 ||
            !LoadPng(source_path, &sImages[index].width, &sImages[index].height, &sImages[index].rgba)) {
            fprintf(stderr, "[object-override] missing or invalid %s\n", path);
            return -1;
        }
        snprintf(path, sizeof(path), "%s", source_path);
    }
    sImages[index].loaded = true;
    fprintf(stderr, "[object-override] loaded %s (%dx%d)\n", path, sImages[index].width, sImages[index].height);
    return index;
}

static int ResolveImage(const char* name, int language) {
    char filename[96];
    const char* suffix = LanguageSuffix(language);
    if (suffix) {
        snprintf(filename, sizeof(filename), "%s_%s.png", name, suffix);
        {
            const int index = LoadImage(filename);
            if (index >= 0)
                return index;
        }
    }
    snprintf(filename, sizeof(filename), "%s.png", name);
    return LoadImage(filename);
}

bool Port_ObjectOverride_Submit(const char* name, int language,
                                int x, int y, int width, int height, int obj_palette) {
    if (!name || !*name || width <= 0 || height <= 0 || sRequestCount >= PORT_OBJECT_OVERRIDE_MAX_REQUESTS)
        return false;
    const int image = ResolveImage(name, language);
    if (image < 0)
        return false;
    /* One logical object should submit at most one request per frame. */
    for (int i = 0; i < sRequestCount; ++i) {
        if (strcmp(sRequests[i].name, name) == 0) {
            sRequests[i].image = image;
            sRequests[i].palette = obj_palette;
            sRequests[i].x = x;
            sRequests[i].y = y;
            sRequests[i].width = width;
            sRequests[i].height = height;
            return true;
        }
    }
    PortObjectOverrideRequest* request = &sRequests[sRequestCount++];
    memset(request, 0, sizeof(*request));
    snprintf(request->name, sizeof(request->name), "%s", name);
    request->image = image;
    request->palette = obj_palette;
    request->x = x;
    request->y = y;
    request->width = width;
    request->height = height;
    return true;
}

bool Port_ObjectOverride_SubmitOverlay(const char* name, int language,
                                       int x, int y, int width, int height, int obj_palette) {
    /* The override module only records the draw; suppression is the caller's
     * choice. The separate entry point keeps the caller contract explicit. */
    return Port_ObjectOverride_Submit(name, language, x, y, width, height, obj_palette);
}

static void ApplyPaletteFade(uint8_t* r, uint8_t* g, uint8_t* b, int obj_palette) {
    const PortObjectFadeBank* fade;
    unsigned intensity, target;
    if (obj_palette < 0 || obj_palette > 15)
        return;
    /* FadeVBlank stores per-bank parameters in 0..31 with OBJ palettes at
     * 16..31. Reading it keeps the PNG on the same fade curve as the sprite
     * it replaces. Unknown/stale banks have intensity 0 and are no-ops. */
    fade = &gUnk_020354C0[16 + obj_palette];
    intensity = fade->unk2;
    if (intensity == 0 || intensity > 256)
        return;
    if (fade->unk1 != 0 && fade->unk1 != 248)
        return;
    target = fade->unk1 == 248 ? 255u : 0u;
    *r = (uint8_t)(((unsigned)*r * (256u - intensity) + target * intensity) >> 8);
    *g = (uint8_t)(((unsigned)*g * (256u - intensity) + target * intensity) >> 8);
    *b = (uint8_t)(((unsigned)*b * (256u - intensity) + target * intensity) >> 8);
}

static inline uint8_t BlendChannel(uint8_t src, uint8_t dst, unsigned alpha) {
    return (uint8_t)(((unsigned)src * alpha + (unsigned)dst * (255u - alpha) + 127u) / 255u);
}

void Port_ObjectOverride_Draw(uint32_t* canvas, int width, int height, int pitch,
                              int native_x, int native_y) {
    if (!canvas || width <= 0 || height <= 0 || pitch < width || sRequestCount == 0)
        return;
    for (int i = 0; i < sRequestCount; ++i) {
        const PortObjectOverrideRequest* request = &sRequests[i];
        const PortObjectOverrideImage* image;
        int dst_x, dst_y, src_x, src_y;
        if (request->image < 0 || request->image >= sImageCount)
            continue;
        image = &sImages[request->image];
        if (!image->loaded || !image->rgba)
            continue;
        dst_x = native_x + request->x + (request->width - image->width) / 2;
        dst_y = native_y + request->y + (request->height - image->height) / 2;
        src_x = 0;
        src_y = 0;
        if (dst_x < 0) { src_x = -dst_x; dst_x = 0; }
        if (dst_y < 0) { src_y = -dst_y; dst_y = 0; }
        if (dst_x >= width || dst_y >= height)
            continue;
        const int copy_w = image->width - src_x < width - dst_x ? image->width - src_x : width - dst_x;
        const int copy_h = image->height - src_y < height - dst_y ? image->height - src_y : height - dst_y;
        for (int y = 0; y < copy_h; ++y) {
            uint32_t* out = canvas + (size_t)(dst_y + y) * (size_t)pitch + (size_t)dst_x;
            const uint8_t* in = image->rgba + ((size_t)(src_y + y) * (size_t)image->width + (size_t)src_x) * 4u;
            for (int x = 0; x < copy_w; ++x, out += 1, in += 4) {
                const unsigned alpha = in[3];
                uint8_t r, g, b;
                if (alpha == 0)
                    continue;
                r = in[0];
                g = in[1];
                b = in[2];
                ApplyPaletteFade(&r, &g, &b, request->palette);
                if (alpha >= 255u) {
                    *out = 0xFF000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
                } else {
                    const uint32_t back = *out;
                    const uint8_t br = (uint8_t)(back & 0xFFu);
                    const uint8_t bg = (uint8_t)((back >> 8) & 0xFFu);
                    const uint8_t bb = (uint8_t)((back >> 16) & 0xFFu);
                    *out = 0xFF000000u | ((uint32_t)BlendChannel(b, bb, alpha) << 16) |
                           ((uint32_t)BlendChannel(g, bg, alpha) << 8) | BlendChannel(r, br, alpha);
                }
            }
        }
    }
}
