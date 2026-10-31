#!/usr/bin/env python3
"""Exercise production widescreen engine helpers without a ROM or SDL build."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def main():
    stubs = (ROOT / 'port/port_linked_stubs.c').read_text()
    message = (ROOT / 'src/message.c').read_text()
    functions = [function(stubs, name) for name in (
        'static uint64_t Port_WidescreenRoomKey(void)',
        'static s32 Port_WidescreenScanContentPx(void)',
        'int Port_Widescreen_TargetViewWidth(void)',
        'int Port_Widescreen_FallbackNative(void)',
        'int Port_Widescreen_IsActive(void)',
        'int Port_Widescreen_LayoutActive(void)',
        'int Port_Widescreen_LayoutWidth(void)',
        'int Port_Widescreen_LayoutHeight(void)',
        'int Port_Widescreen_EffectiveViewWidth(void)',
        'int Port_Widescreen_HudRightAnchor(void)',
        'int Port_Widescreen_CameraRestX(int target_x)',
        'static void Port_WidescreenShadow_Populate(',
        'static void Port_WidescreenShadow_PopulateOverlay(',
        'static int Port_WidescreenPpuBgForControl(',
        'static int Port_WidescreenMapShadowsLive(void)',
        'void Port_Widescreen_SetTallValidated(int validated)',
        'static bool Port_WidescreenMapSnapshotMatches(int height)',
        'static void Port_WidescreenUploadMapBuffers(void)',
    )]
    functions.extend(function(stubs, name) for name in (
        'static int Port_WidescreenRepeatingOverlayLive(void)',
        'static int Port_WidescreenTallAperture(void)',
        'static int Port_WidescreenTallRows(void)',
        'static int Port_WidescreenCandidateWidth(void)',
        'static int Port_WidescreenManagersSafe(int apertureHeight)',
        'static int Port_WidescreenEntitiesSafe(int apertureHeight)',
        'int Port_Widescreen_ShadowsLive(void)',
        'void Port_Widescreen_PublishScene(void)',
        'void Port_Widescreen_MapUploaded(void)',
        'void Port_Widescreen_BeforePresent(void)',
        'void Port_Widescreen_RoomLoaded(void)',
    ))
    functions.append(function(message, 'bool Message_GetWindowRect('))
    functions.append(function(message, 'bool Message_GetFinalWindowRect('))
    functions.append(function((ROOT / 'src/ui.c').read_text(), 'static s16 HudButtonTargetX('))
    functions.append(function(stubs, 'void Port_Widescreen_UpdateShadows(void)'))
    functions.append(function((ROOT / 'port/port_ppu.cpp').read_text(), 'int Port_PPU_VisibleFrameWidth(void)'))
    with tempfile.TemporaryDirectory(prefix='tmc-ws-engine-') as directory:
        generated = Path(directory) / 'widescreen_helpers.h'
        generated.write_text('\n'.join(functions))
        binary = Path(directory) / 'test'
        command = shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c11', '-Wall', '-Wextra', '-Werror', '-DPORT_VIEWPORT_TEST_TALL', '-DPC_PORT', '-DMULTI_REGION', '-DUSA',
            '-I' + directory, '-I' + str(ROOT / 'port'), '-I' + str(ROOT / 'include'), '-I' + str(ROOT),
            str(ROOT / 'tools/tests/widescreen_engine.c'), str(ROOT / 'port/port_viewport.c'),
            str(ROOT / 'port/port_tall_scene.c'), '-o', str(binary)]
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
