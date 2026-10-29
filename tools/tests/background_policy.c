#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "port_background_policy.h"

int main(void) {
    PortBackgroundEffectPolicy p;
    PortPaletteFade banks[16] = {{0,0}};
    PortBackgroundComposeRow r, prior;
    const uint8_t bg3[3] = {30, 60, 90};
    for (int c = PORT_BACKGROUND_BOOT_LOGOS; c < PORT_BACKGROUND_COUNT; ++c)
        assert(Port_Background_EffectPolicy((PortBackgroundContext)c, &p));
    assert(!Port_Background_EffectPolicy(PORT_BACKGROUND_LIVE_WORLD, &p));
    assert(Port_Background_EffectPolicy(PORT_BACKGROUND_KINSTONE_PIECES_NORMAL, &p));
    assert(p.layers == 4 && p.banks == 0x2000);
    memset(&r, 0x55, sizeof(r)); prior = r;
    assert(!Port_Background_ResolveEffect(&p, 4, 0x2000, banks, 0xffff, 0x0e46, 0x0808, 0, 1, -1, NULL, &r));
    assert(memcmp(&r, &prior, sizeof(r)) == 0);
    assert(Port_Background_ResolveEffect(&p, 4, 0x2000, banks, 0xffff, 0x0e46, 0x0808, 0, 1, 3, bg3, &r));
    assert(r.layer_effect == PORT_LAYER_EFFECT_NEEDS_ALPHA_INPUT && r.second_valid && r.second_rgb[1] == 60);
    assert(Port_Background_ResolveEffect(&p, 4, 0x2000, banks, 0xffff, 0x0e46, 0x0808, 0, 1, 0, NULL, &r));
    assert(r.layer_effect == PORT_LAYER_EFFECT_NONE); /* actual layer isn't selected */
    assert(Port_Background_ResolveEffect(&p, 4, 0x2000, banks, 0xffff, 0x0e46, 0x0808, 0, 0, -1, NULL, &r));
    assert(r.layer_effect == PORT_LAYER_EFFECT_NONE); /* window disables SFX */
    assert(!Port_Background_ResolveEffect(&p, 4, 0x2000, banks, 0, 0, 0, 0, 1, -1, NULL, &r));
    assert(Port_Background_EffectPolicy(PORT_BACKGROUND_KINSTONE_NORMAL, &p));
    assert(!Port_Background_ResolveEffect(&p, 0x0e, 0x7800, banks, 0xffff, 0x00c2, 0, 8, 1, -1, NULL, &r));
    banks[11].intensity = 64; banks[11].color = 248;
    assert(!Port_Background_ResolveEffect(&p, 0x0e, 0x7800, banks, 0xffff, 0, 0, 0, 1, -1, NULL, &r));
    assert(Port_Background_EffectPolicy(PORT_BACKGROUND_LOCAL_MAP_HINT, &p));
    assert(Port_Background_ResolveEffect(&p, 8, 0x20, NULL, 0, 0x0088, 0, 8, 1, -1, NULL, &r));
    assert(!r.apply_palette_fade && r.layer_effect == PORT_LAYER_EFFECT_BRIGHTEN);
    assert(Port_Background_EffectPolicy(PORT_BACKGROUND_FIGURINES, &p));
    assert(Port_Background_ResolveEffect(&p, 0x20, 1, NULL, 0, 0x2060, 0x0808, 0, 1, -1, NULL, &r));
    assert(!r.apply_palette_fade && r.layer_effect == PORT_LAYER_EFFECT_NONE);
    assert(Port_Background_EffectPolicy(PORT_BACKGROUND_GAME_OVER, &p));
    /* Generated black is the live backdrop: its hardware palette already
     * includes quantized fades and brightness, but still receives layer FX. */
    assert(Port_Background_ResolveEffect(&p, 0x20, 1, NULL, 0, 0x00a0, 0, 8, 1, -1, NULL, &r));
    assert(!r.apply_palette_fade && r.layer_effect == PORT_LAYER_EFFECT_BRIGHTEN);
    puts("Background context effect and second-input policy: PASS");
}
