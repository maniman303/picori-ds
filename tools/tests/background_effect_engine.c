/* Exercise the resolver against the game's actual masked FadeMain update. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fade.h"
#include "screen.h"
#include "structures.h"
#include "menu.h"
#include "port_background_effect.h"
#undef static_assert
#include <assert.h>

FadeControl gFadeControl;
struct_020354C0 gUnk_020354C0[0x20];
Screen gScreen;
OAMControls gOAMControls;
u32 gUsedPalettes;
u16 gPaletteBuffer[512];
u8 gEwram[0x40000];
static u8 rom[0x2000];
u8* gRomData = rom;
int gActiveRegion = TMC_REGION_EU;
static u16 live_palette[512];
u8 _gMenuSharedStorage[0x40] __attribute__((aligned(8)));

void StaffrollTask_State1MenuType4(void);
void MemClear(void* dest, u32 size) { memset(dest, 0, size); }
void Port_LogFadeCall(const char* name, u32 a, u32 b, u32 c, u32 d, u32 e) {
    (void)name; (void)a; (void)b; (void)c; (void)d; (void)e;
}
void sub_0801E1B8(u32 inside, u32 outside) { (void)inside; (void)outside; abort(); }

void* port_resolve_addr(uintptr_t address) {
    assert(address >= 0x05000000u && address < 0x05000400u);
    return (u8*)live_palette + (address - 0x05000000u);
}

static void install_identity_brightness_tables(void) {
    /* EU fade LUT offsets for brightness 0, using identity five-bit channels. */
    static const unsigned offsets[] = {0x0fcc, 0x100c, 0x104c};
    for (unsigned channel = 0; channel < 3; ++channel)
        for (unsigned value = 0; value < 32; ++value) {
            u16 entry = (u16)(value << (channel * 5));
            memcpy(rom + offsets[channel] + value * 2, &entry, sizeof(entry));
        }
}

void sub_0801E104(void) { abort(); }
void sub_0801E1EC(u32 x, u32 y, u32 radius) { (void)x; (void)y; (void)radius; abort(); }

static PortPaletteFadeResult resolve(uint16_t mask, PortPaletteFade* result) {
    PortPaletteFade banks[16];
    for (int i = 0; i < 16; ++i) {
        banks[i].intensity = gUnk_020354C0[i].unk2;
        banks[i].color = gUnk_020354C0[i].unk1;
    }
    return Port_Background_ResolvePaletteFade(mask, banks, result);
}

int main(void) {
    PortPaletteFade result;
    install_identity_brightness_tables();
    /* Run the real staff-roll menu command, including its mask assignment,
     * SetFade, progress setup and script advance, before the VBlank stages. */
    static u16 script_words[6];
    gMenu.field_0xc = (u8*)script_words;
    gMenu.transitionTimer = 0;
    StaffrollTask_State1MenuType4();
    assert(gFadeControl.mask == 0xffff7fff && gFadeControl.speed == 4);
    FadeMain();
    assert(gUnk_020354C0[7].unk2 == 4 && gUnk_020354C0[15].unk2 == 0);
    assert(resolve(0x7f80, &result) == PORT_PALETTE_FADE_UNIFORM && result.intensity == 4);
    memset(&gFadeControl, 0, sizeof(gFadeControl));
    memset(gUnk_020354C0, 0, sizeof(gUnk_020354C0));
    /* Staffroll mode 4 excludes bank 15; all BG2 illustration banks 7..14
     * still receive the same update. FADE_IN_OUT moves intensity 0 -> 256. */
    gFadeControl.active = 1;
    gFadeControl.type = FADE_IN_OUT | FADE_INSTANT;
    gFadeControl.progress = 256;
    gFadeControl.speed = 64;
    gFadeControl.color = 248;
    gFadeControl.mask = 0xffff7fff;
    FadeMain();
    assert(gUnk_020354C0[7].unk2 == 64 && gUnk_020354C0[14].unk2 == 64);
    assert(gUnk_020354C0[15].unk2 == 0);
    assert(resolve(0x7f80, &result) == PORT_PALETTE_FADE_UNIFORM);
    assert(result.intensity == 64 && result.color == 248);
    assert(resolve(0xff80, &result) == PORT_PALETTE_FADE_MIXED);
    FadeMain();
    assert(resolve(0x7f80, &result) == PORT_PALETTE_FADE_UNIFORM && result.intensity == 128);
    /* A different mask excludes a bank genuinely used by the source. */
    gFadeControl.mask = 0xffff3fff;
    FadeMain();
    assert(gUnk_020354C0[14].unk2 == 0);
    assert(resolve(0x7f80, &result) == PORT_PALETTE_FADE_MIXED);
    /* Local hint's BG3 index 95 is in bank 5; backdrop index 0 is bank 0.
     * Their equal unfaded colors must not erase this target distinction. */
    gFadeControl.mask = 1u << 5;
    FadeMain();
    assert(gUnk_020354C0[0].unk2 == 0 && gUnk_020354C0[5].unk2 == 256);
    assert(resolve(1u << 5, &result) == PORT_PALETTE_FADE_UNIFORM && result.intensity == 256);
    assert(resolve(1u << 0, &result) == PORT_PALETTE_FADE_UNIFORM && result.intensity == 0);
    gPaletteBuffer[0] = gPaletteBuffer[95] = 0x475f;
    FadeVBlank(); /* Real palette upload into the live PAL RAM shim. */
    assert(live_palette[0] == 0x475f && live_palette[95] == 0x7fff);
    puts("Native masked FadeMain and source-bank resolution: PASS");
    return 0;
}
