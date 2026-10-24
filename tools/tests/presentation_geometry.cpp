#include "port_viewport.h"
#include "port_upscale.h"
#include "port_filter.h"
#include "port_hd_text_geometry.h"
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#define MODE1_GBA_HEIGHT 160
static PortViewportGeometry geometry = {};
static int rasterHeight = 160;
static bool signedPositions = false;
extern "C" const PortViewportGeometry* Port_Viewport_Get(void) { return &geometry; }
static int virtuappu_mode1_frame_height(void) { return rasterHeight; }
static bool virtuappu_mode1_has_obj_positions(void) { return signedPositions; }
#include "presentation_functions.inc"

static constexpr uint32_t canary = 0x12345678;
static uint32_t color(int x, int y) { return 0xff000000u | (uint32_t)((y * 257 + x) & 0xffffff); }

static void check_nearest(int width, int height, int scale) {
    const int pitch = width + 7;
    std::vector<uint32_t> source((size_t)pitch * height, canary);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            source[y * pitch + x] = color(x, y);
    int outW = -1, outH = -1;
    const size_t needed = (size_t)width * height * scale * scale;
    const bool fits = needed <= (size_t)MODE1_GBA_WIDTH * 160 * 100;
    sScaledBufStorage[kMaxScaledPixels - 1] = canary;
    if (fits && needed < kMaxScaledPixels)
        sScaledBufStorage[needed] = canary;
    uint32_t* frame = Port_PPU_BuildScaledFrame(source.data(), width, height, pitch * 4, scale, &outW, &outH);
    assert((frame != nullptr) == fits);
    if (!fits) {
        assert(outW == 0 && outH == 0 && sScaledBufStorage[kMaxScaledPixels - 1] == canary);
        return;
    }
    assert(outW == width * scale && outH == height * scale);
    assert(sScaledBufW == width && sScaledBufH == height && sScaledBufScale == scale);
    for (int y = 0; y < outH; ++y)
        for (int x = 0; x < outW; ++x)
            assert(frame[(size_t)y * outW + x] == color(x / scale, y / scale));
    if (needed < kMaxScaledPixels)
        assert(frame[needed] == canary);
    for (int y = 0; y < height; ++y)
        for (int x = width; x < pitch; ++x)
            assert(source[y * pitch + x] == canary);
}

static void check_xbrz_filters(int width, int height) {
    const bool fits = (size_t)width * height <= (size_t)MODE1_GBA_WIDTH * 160;
    assert(Port_PPU_EnsureXbrzBuffers(width, height) == fits);
    if (!fits) {
        assert(sUpscale2xPixels == 0 && sUpscale4xPixels == 0);
        return;
    }
    const int pitch = width + 5;
    const uint32_t solid = 0xff204060;
    std::vector<uint32_t> source((size_t)pitch * height, canary);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            source[y * pitch + x] = solid;
    if (sUpscale2xPixels < kMaxXbrz2xPixels) sUpscale2xBuf[sUpscale2xPixels] = canary;
    if (sUpscale4xPixels < kMaxXbrz4xPixels) sUpscale4xBuf[sUpscale4xPixels] = canary;
    Port_Upscale_xBRZ_4x_Pitch(source.data(), width, height, pitch, sUpscale2xBuf, sUpscale4xBuf);
    for (size_t i = 0; i < sUpscale4xPixels; ++i) assert(sUpscale4xBuf[i] == solid);
    // Every CPU filter must cover the complete tight tall/wide workspace.
    // Black is an independent invariant for all filters; guards catch overruns.
    for (int filter = PORT_FILTER_NONE; filter < PORT_FILTER_COUNT; ++filter) {
        for (size_t i = 0; i < sUpscale4xPixels; ++i) sUpscale4xBuf[i] = 0xff000000;
        Port_Filter_Apply(sUpscale4xBuf, width * 4, height * 4, 4, (PortFilterType)filter);
        for (size_t i = 0; i < sUpscale4xPixels; ++i) assert(sUpscale4xBuf[i] == 0xff000000);
        if (sUpscale4xPixels < kMaxXbrz4xPixels) assert(sUpscale4xBuf[sUpscale4xPixels] == canary);
    }
    if (sUpscale2xPixels < kMaxXbrz2xPixels) assert(sUpscale2xBuf[sUpscale2xPixels] == canary);
}

static void check_rejection(void) {
    const uint32_t pixel = 0;
    const int invalid[][4] = {
        {0,160,960,2}, {-1,160,960,2}, {240,0,960,2}, {240,-1,960,2},
        {240,160,959,2}, {240,160,961,2}, {240,160,-960,2},
        {INT_MAX,160,INT_MAX,10}, {240,INT_MAX,960,10},
        {INT_MAX/4,INT_MAX/4,INT_MAX-3,2},
        {240,160,960,0}, {240,160,960,1}, {240,160,960,11},
    };
    for (const auto& item : invalid) {
        int w = 17, h = 17;
        assert(!Port_PPU_BuildScaledFrame(&pixel, item[0], item[1], item[2], item[3], &w, &h));
        assert(w == 0 && h == 0);
    }
    int w = 17, h = 17;
    assert(!Port_PPU_BuildScaledFrame(nullptr,240,160,960,2,&w,&h) && w == 0 && h == 0);
    assert(!Port_PPU_EnsureXbrzBuffers(INT_MAX, INT_MAX));
    assert(!Port_PPU_EnsureXbrzBuffers(INT_MAX / 4, INT_MAX / 4));
    assert(!Port_PPU_EnsureXbrzBuffers(-1, 160));
    assert(sUpscale2xPixels == 0 && sUpscale4xPixels == 0);
}

static void check_optional_paths(void) {
    for (int width : {240, 256, 285, 374, 512, 569}) {
        for (int height : {160, 192}) {
            geometry.canvas.width = width;
            geometry.canvas.height = height;
            rasterHeight = height;
            signedPositions = false;
            const bool legacy = width <= 512 && height == 160;
            assert(ScreenGeometrySupported() == legacy);
            assert(Port_GPU_PresetGeometrySupported() == legacy);
            assert(PortHDText::SupportedFrame(width, height) == legacy);
            // A native raster fallback inside an expanded canvas is still
            // unsupported by the voxel layer stack/projection.
            rasterHeight = 160;
            assert(ScreenGeometrySupported() == legacy);
        }
    }
    geometry.canvas = {0,0,240,160};
    signedPositions = true;
    assert(!ScreenGeometrySupported());
    assert(!PortHDText::SupportedFrame(239,160));
}

static void check_presentation_layout(void) {
    struct Case {
        PortAspectMode mode;
        int outputW, outputH, frameW, frameH;
        int sx, sy, sw, sh, fx, fy, fw, fh;
    };
    const Case cases[] = {
        {PORT_ASPECT_NATIVE_3_2, 960,640,240,160, 0,0,960,640, 0,0,960,640},
        {PORT_ASPECT_NATIVE_3_2, 1000,1000,240,160, 0,0,1000,1000, 0,167,1000,666},
        {PORT_ASPECT_DS_4_3, 1024,768,256,192, 0,0,1024,768, 0,0,1024,768},
        {PORT_ASPECT_DS_4_3, 1024,768,240,160, 0,0,1024,768, 0,43,1024,682},
        {PORT_ASPECT_WIDESCREEN_16_9, 960,540,285,160, 0,0,960,540, 0,1,960,538},
        {PORT_ASPECT_ULTRAWIDE_21_9, 840,360,374,160, 0,0,840,360, 0,0,840,359},
        {PORT_ASPECT_SUPER_ULTRAWIDE_32_9, 1024,288,569,160, 0,0,1024,288, 0,0,1024,287},
    };
    // A pending selection cannot override the published effective mode.
    geometry.selectedAspect = PORT_ASPECT_ULTRAWIDE_21_9;
    for (const auto& item : cases) {
        geometry.effectiveAspect = item.mode;
        int sx,sy,sw,sh,fx,fy,fw,fh;
        Port_PPU_ComputeViewportRects(item.outputW,item.outputH,item.frameW,item.frameH,
                                      &sx,&sy,&sw,&sh,&fx,&fy,&fw,&fh);
        assert(sx == item.sx && sy == item.sy && sw == item.sw && sh == item.sh);
        assert(fx == item.fx && fy == item.fy && fw == item.fw && fh == item.fh);
    }
}

int main(void) {
    for (const auto& profile : {PortViewportRect{0,0,240,160}, {0,0,256,192},
                               {0,0,285,160}, {0,0,374,160}, {0,0,569,160}, {0,0,569,192}}) {
        for (int scale : {2, 4, 10}) check_nearest(profile.width, profile.height, scale);
        check_xbrz_filters(profile.width, profile.height);
    }
    check_rejection();
    check_optional_paths();
    check_presentation_layout();
    std::printf("Presentation scratch, pitch, filters and optional gates (capacity %d): PASS\n", MODE1_GBA_WIDTH);
}
