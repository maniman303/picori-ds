#!/usr/bin/env python3
"""Check production SDL_Renderer/GPU viewport geometry without SDL or a ROM."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from widescreen_engine_regression_test import function

ROOT = Path(__file__).resolve().parent.parent


def main():
    header = (ROOT / 'port/port_runtime_config.h').read_text()
    enum_start = header.rfind('typedef enum', 0, header.index('PORT_ASPECT_NATIVE_3_2 ='))
    enum_end = header.index('} PortAspectMode;', enum_start) + len('} PortAspectMode;')
    ppu = (ROOT / 'port/port_ppu.cpp').read_text()
    gpu = (ROOT / 'port/port_gpu_renderer.cpp').read_text()
    text = (ROOT / 'src/text.c').read_text()
    source = '''#include "port_hd_text_geometry.h"
#include "port_narrow.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstdio>
using u32 = uint32_t;
struct WStruct { int unk1; };
static u32 glyph[32];
static u32* sub_0805F25C(u32) { return glyph; }
''' + function(text, 'static u32 sub_0805F7A0(u32 param_1) {') + '\n' + function(text, 'u32 Port_Text_GlyphAdvance(') + '\n' + header[enum_start:enum_end] + '''
static PortAspectMode mode;
static PortAspectMode Port_Config_AspectMode() { return mode; }
''' + function(ppu, 'static void Port_PPU_FitAspectRect(') + '\n' + function(
        ppu, 'static void Port_PPU_ComputeViewportRects(') + '''
struct Rects { int sx, sy, sw, sh, fx, fy, fw, fh; };
static Rects gpuRects(int swap_w, int swap_h, int fb_w, int fb_h) {
    int stageX, stageY, stageW, stageH, frameX, frameY, frameW, frameH;
''' + function(gpu, '    {\n        const int FW = fb_w;') + '''
    return {stageX, stageY, stageW, stageH, frameX, frameY, frameW, frameH};
}
int main() {
    const int sizes[][2] = {{240,180}, {320,240}, {640,480}, {800,600},
                            {1920,1080}, {1280,1024}, {480,640}, {853,480}};
    const int widths[] = {212, 240, 284, 384, 480};
    for (int m = 0; m < PORT_ASPECT_COUNT; ++m) {
        mode = (PortAspectMode)m;
        for (auto& size : sizes) for (int width : widths) {
            Rects cpu;
            Port_PPU_ComputeViewportRects(size[0], size[1], width, 160,
                &cpu.sx, &cpu.sy, &cpu.sw, &cpu.sh, &cpu.fx, &cpu.fy, &cpu.fw, &cpu.fh);
            Rects gpu = gpuRects(size[0], size[1], width, 160);
            assert(cpu.sx == gpu.sx && cpu.sy == gpu.sy && cpu.sw == gpu.sw && cpu.sh == gpu.sh);
            assert(cpu.fx == gpu.fx && cpu.fy == gpu.fy && cpu.fw == gpu.fw && cpu.fh == gpu.fh);
            assert(cpu.sx >= 0 && cpu.sy >= 0);
            assert(cpu.sx + cpu.sw <= size[0] && cpu.sy + cpu.sh <= size[1]);
            assert(cpu.fx >= cpu.sx && cpu.fy >= cpu.sy);
            assert(cpu.fx + cpu.fw <= cpu.sx + cpu.sw && cpu.fy + cpu.fh <= cpu.sy + cpu.sh);
            if (mode == PORT_ASPECT_STANDARD_4_3 && width == 212) {
                if (size[0] == 640) assert(cpu.fw == 636 && cpu.fh == 480 && cpu.fx == 2 && cpu.fy == 0);
                assert(std::abs(cpu.fw * 160 - cpu.fh * 212) <= 212);
                if (cpu.fw >= 424) {
                    for (int y : {8, 120}) for (int dpi : {1, 2}) {
                        PortHDText::CanvasViewport text;
                        assert(PortHDText::MapCanvasViewport(8, y, 192, 32, width, 160,
                            (float)cpu.fx / dpi, (float)cpu.fy / dpi,
                            (float)cpu.fw / dpi, (float)cpu.fh / dpi, dpi, dpi, text));
                        assert(text.x * dpi >= cpu.fx && text.y * dpi >= cpu.fy);
                        assert((text.x + text.width) * dpi <= cpu.fx + cpu.fw + 0.001f);
                        assert((text.y + text.height) * dpi <= cpu.fy + cpu.fh + 0.001f);
                    }
                }
                for (int upscale : {2, 4}) {
                    Rects scaled = gpuRects(size[0], size[1], width * upscale, 160 * upscale);
                    assert(scaled.fx == cpu.fx && scaled.fy == cpu.fy && scaled.fw == cpu.fw && scaled.fh == cpu.fh);
                }
            }
        }
    }
    assert(!Port_Narrow_TextOverflow(0, 184, 8, 192));
    assert(Port_Narrow_TextOverflow(0, 185, 8, 192));
    assert(!Port_Narrow_TextOverflow(1, 208 + 184, 8, 192));
    assert(Port_Narrow_TextOverflow(1, 208 + 185, 8, 192));
    assert(Port_Narrow_TextOverflow(0, 153, 8, 160));
    for (int col = 0; col < 13; ++col) {
        int cursor = Port_Narrow_KeyboardCursorX(col) - PORT_NARROW_CROP;
        assert(cursor - 8 >= 0 && cursor + 8 <= PORT_NARROW_WIDTH);
    }
    WStruct style = {0};
    glyph[0] = 0xffff1111; // Four foreground columns, no glyph pixels trimmed.
    glyph[16] = 0xff111111; // Six foreground columns in the second half.
    assert(Port_Text_GlyphAdvance(0x400, &style) == 4);
    assert(Port_Text_GlyphAdvance(0x500, &style) == 10);
    style.unk1 = 1;
    assert(Port_Text_GlyphAdvance(0x400, &style) == 8);
    assert(Port_Text_GlyphAdvance(0x500, &style) == 16);
    puts("aspect viewport regression checks passed");
}
'''
    with tempfile.TemporaryDirectory(prefix='tmc-aspect-test-') as directory:
        cpp = Path(directory) / 'test.cpp'
        cpp.write_text(source)
        binary = Path(directory) / 'test'
        command = shlex.split(os.environ.get('CXX', 'c++'))
        subprocess.run(command + ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'port'),
                                 str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
