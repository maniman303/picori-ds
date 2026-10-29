#include <stdio.h>
#include "port_background_policy.h"
int main(void) {
    for (int c = 0; c < PORT_BACKGROUND_COUNT; ++c) {
        const PortScreenBackground* b = Port_Background_Get((PortBackgroundContext)c, false);
        PortBackgroundEffectPolicy p;
        if (b->source != PORT_BACKGROUND_SOURCE_IMAGE) continue;
        if (!Port_Background_EffectPolicy((PortBackgroundContext)c, &p)) return 1;
        const PortBackgroundArtworkEdges* edges = Port_Background_ArtworkEdges((PortBackgroundContext)c);
        if (!edges) return 1;
        printf("%s %u %u", b->filename, p.layers, p.banks);
        const uint8_t* sides[] = {edges->left, edges->right, edges->top, edges->bottom};
        for (int side = 0; side < 4; ++side) for (int y = 0; y < (side < 2 ? 160 : 240); ++y) {
            unsigned owner = sides[side][y], layer = owner >> 4, bank = owner & 15;
            if (layer > 5 || layer == 4 || !(p.layers & (1u << layer)) || !(p.banks & (1u << bank))) return 1;
            printf(" %u", owner);
        }
        putchar('\n');
    }
    return 0;
}
