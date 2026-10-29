#include <cassert>
#include <cstdint>
#include <cstring>
#include "port_background_runtime.h"
#include "port_oam.h"
#include "port_title_cap.h"
#include "cpu/mode1.h"

static PortViewportGeometry geometry;
static PortBackgroundRuntimeSnapshot snapshot;
static PortBackgroundArtworkEdges edges;
static PortOamMenuHeading heading;
static uint32_t virtuappu_frame_buffer[569 * 192];
static uint8_t image[569 * 192 * 4];
static uint8_t gIoMem[0x400], gVram[0x18000], rom[0x10000];
static uint8_t* gRomData = rom;
static uint16_t gBgPltt[256], gObjPltt[256];
static uint32_t gFrameObjLists[1];
static int sComposedWidth, sComposedHeight;
static int activeHdma, fixedBrightnessHdma;
const PortViewportGeometry* Port_Viewport_Get() { return &geometry; }
const PortOamMenuHeading* Port_Oam_MenuHeading() { return &heading; }
const PortBackgroundRuntimeSnapshot* Port_BackgroundRuntime_Get() { return &snapshot; }
int virtuappu_mode1_frame_width() { return 240; }
int virtuappu_mode1_frame_height() { return 160; }
int virtuappu_mode1_frame_pitch() { return 569; }
static int port_hdma_has_active_channels() { return activeHdma; }
static int port_hdma_can_continue_brightness(const void*, size_t) { return fixedBrightnessHdma; }
static void* sub_080AD8F0(uint32_t, uint32_t) { return nullptr; }
static uint32_t Port_FrameObjListsSizeForRegion() { return sizeof(gFrameObjLists); }
const uint8_t* virtuappu_mode1_frame_io_for_line(int) { return gIoMem; }
bool virtuappu_mode1_sample_below_bg(int, int, int, unsigned, unsigned* layer, uint32_t* color) {
    *layer = 5; *color = 0xff000000; return true;
}
bool Port_Effect_ResolveFadeIris(int, int, int, int, PortEffectSpan*, int) { return false; }
bool Port_BackgroundRuntime_ResolveEffect(const PortBackgroundRuntimeSnapshot* s, uint8_t layers,
                                                uint16_t banks, uint16_t bldcnt, uint16_t alpha, uint16_t bldy,
                                                int window, int second, const uint8_t rgb[3],
                                                PortBackgroundComposeRow* output) {
    return Port_Background_ResolveEffect(&s->effect_policy, layers, banks, s->palette_fades,
                                          s->valid_palette_fades, bldcnt, alpha, bldy, window, second, rgb, output);
}
#include "background_frame_function.inc"

static void reset_frame() {
    for (int y = 0; y < 192; ++y) for (int x = 0; x < 569; ++x)
        virtuappu_frame_buffer[y * 569 + x] = 0xff123456;
    sComposedWidth = sComposedHeight = 0;
}
int main() {
    geometry.scene.capabilities = PORT_SCENE_FIXED_SCREEN;
    geometry.canvas = {0,0,256,192}; geometry.nativeContent = {8,16,240,160};
    snapshot.canvas = geometry.canvas; snapshot.native = geometry.nativeContent;
    snapshot.source = PORT_BACKGROUND_SOURCE_IMAGE; snapshot.effect_policy_valid = true;
    snapshot.effect_policy = {4, (uint16_t)((1u << 7) | (1u << 8)), false};
    snapshot.artwork_edges = &edges; snapshot.valid_palette_fades = 0xffff;
    snapshot.width = 256; snapshot.height = 192; snapshot.rgba = image;
    for (int i = 0; i < 16; ++i) snapshot.brightness[i] = 1;
    snapshot.palette_fades[8] = {128,0};
    std::memset(&edges, 0x27, sizeof(edges));
    for (int x = 120; x < 240; ++x) edges.top[x] = edges.bottom[x] = 0x28;
    for (int y = 0; y < 160; ++y) edges.right[y] = 0x28;
    for (int i = 0; i < 256 * 192; ++i) {
        image[i*4] = 200; image[i*4+1] = 100; image[i*4+2] = 50; image[i*4+3] = 255;
    }
    gIoMem[1] = 4; /* BG2 enabled */
    reset_frame(); assert(Port_PPU_ComposeBackground());
    assert(sComposedWidth == 256 && sComposedHeight == 192);
    assert(virtuappu_frame_buffer[0] == 0xff3264c8 && virtuappu_frame_buffer[255] == 0xff193264);
    assert(virtuappu_frame_buffer[191*569] == 0xff3264c8 && virtuappu_frame_buffer[191*569+255] == 0xff193264);
    assert(virtuappu_frame_buffer[50*569] == 0xff3264c8 && virtuappu_frame_buffer[50*569+255] == 0xff193264);
    for (int y = 16; y < 176; ++y) for (int x = 8; x < 248; ++x)
        assert(virtuappu_frame_buffer[y*569+x] == 0xff123456);
    /* A supported 16:9 frame uses the same per-side effects. */
    geometry.canvas = {0,0,285,160}; geometry.nativeContent = {22,0,240,160};
    snapshot.canvas = geometry.canvas; snapshot.native = geometry.nativeContent;
    snapshot.width = 285; snapshot.height = 160;
    for (int i = 0; i < 285 * 160; ++i) {
        image[i*4] = 200; image[i*4+1] = 100; image[i*4+2] = 50; image[i*4+3] = 255;
    }
    reset_frame(); assert(Port_PPU_ComposeBackground());
    assert(sComposedWidth == 285 && sComposedHeight == 160);
    assert(virtuappu_frame_buffer[50*569] == 0xff3264c8 && virtuappu_frame_buffer[50*569+284] == 0xff193264);
    assert(virtuappu_frame_buffer[50*569+22] == 0xff123456);
    geometry.canvas = {0,0,256,192}; geometry.nativeContent = {8,16,240,160};
    snapshot.canvas = geometry.canvas; snapshot.native = geometry.nativeContent;
    snapshot.width = 256; snapshot.height = 192;
    for (int i = 0; i < 256 * 192; ++i) {
        image[i*4] = 200; image[i*4+1] = 100; image[i*4+2] = 50; image[i*4+3] = 255;
    }
    /* The resolved alpha input is the actual lower native layer, not a
     * hard-coded fade or the unfaded artwork color. */
    reset_frame(); gIoMem[0x50] = 0x44; gIoMem[0x51] = 0x20;
    gIoMem[0x52] = 0x00; gIoMem[0x53] = 0x10;
    assert(Port_PPU_ComposeBackground());
    assert(virtuappu_frame_buffer[0] == 0xff000000 && virtuappu_frame_buffer[191*569+255] == 0xff000000);
    gIoMem[0x50] = gIoMem[0x51] = gIoMem[0x52] = gIoMem[0x53] = 0;
    /* A disabled source layer cannot be presented using stale coverage. */
    reset_frame(); gIoMem[1] = 0;
    assert(Port_PPU_ComposeBackground());
    assert(virtuappu_frame_buffer[0] == 0xff000000 && virtuappu_frame_buffer[16*569+8] == 0xff123456);
    gIoMem[1] = 4;
    reset_frame(); activeHdma = 1;
    assert(Port_PPU_ComposeBackground());
    assert(virtuappu_frame_buffer[0] == 0xff000000 && virtuappu_frame_buffer[16*569+8] == 0xff123456);
    reset_frame(); fixedBrightnessHdma = 1;
    assert(Port_PPU_ComposeBackground());
    assert(virtuappu_frame_buffer[0] == 0xff3264c8);
    activeHdma = fixedBrightnessHdma = 0;
    /* Unknown effects must keep the native crop and a full black canvas. */
    reset_frame(); snapshot.effect_policy_valid = false;
    assert(Port_PPU_ComposeBackground());
    assert(virtuappu_frame_buffer[0] == 0xff000000 && virtuappu_frame_buffer[16*569+8] == 0xff123456);
    assert(sComposedWidth == 256 && sComposedHeight == 192);
    reset_frame(); gIoMem[0] = 0x80;
    assert(Port_PPU_ComposeBackground());
    assert(virtuappu_frame_buffer[0] == 0xffffffff && virtuappu_frame_buffer[16*569+8] == 0xff123456);
}
