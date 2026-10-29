#include <stdio.h>
#include "region.h"
int gActiveRegion;
#include "../../port/port_offset_remap.c"
#undef static_assert
#include <assert.h>

int main(void) {
#if defined(EU)
    const int mapSource = 1, gfxSource = 1;
#elif defined(JP)
    const int mapSource = 2, gfxSource = 0;
#else
    const int mapSource = 0, gfxSource = 0;
#endif
    for (int target = 0; target < 3; ++target) {
        gActiveRegion = target;
        for (unsigned i = 0; i < sizeof(sMapRemap)/sizeof(sMapRemap[0]); ++i)
            assert(Port_RemapCompiledMapOffset(sMapRemap[i][mapSource]) == sMapRemap[i][target]);
        for (unsigned i = 0; i < sizeof(sGfxRemapEU)/sizeof(sGfxRemapEU[0]); ++i)
            assert(Port_RemapCompiledGfxOffset(sGfxRemapEU[i][gfxSource]) == sGfxRemapEU[i][target == 1]);
        assert(Port_RemapGfxOffset(0x1ad40) == (target == 1 ? 0x1ad00u : 0x1ad40u));
        assert(Port_RemapCompiledGfxOffset(0x1234) == 0x1234);
    }
    puts("Compiled and explicit USA offset translation: PASS");
}
