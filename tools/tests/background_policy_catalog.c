#include <stdio.h>
#include "port_background_policy.h"
int main(void) {
    for (int c = 0; c < PORT_BACKGROUND_COUNT; ++c) {
        const PortScreenBackground* b = Port_Background_Get((PortBackgroundContext)c, false);
        PortBackgroundEffectPolicy p;
        if (b->source != PORT_BACKGROUND_SOURCE_IMAGE) continue;
        if (!Port_Background_EffectPolicy((PortBackgroundContext)c, &p)) return 1;
        printf("%s %u %u\n", b->filename, p.layers, p.banks);
    }
    return 0;
}
