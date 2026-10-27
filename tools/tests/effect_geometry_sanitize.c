#include <assert.h>
#include "port_effect_geometry.h"
int main(void) {
    PortEffectSpan rows[192];
    const int p[][4] = {{240,160,0,0},{256,192,8,16},{285,160,22,0},{374,160,67,0},{569,160,164,0}};
    for (int i=0;i<5;i++) for (int x=0;x<240;x+=239) for (int y=0;y<160;y+=159)
        for (int r=0;r<=150;r++) {
            Port_Effect_PublishFadeIris(x,y,r,0x3f3f,0);
            Port_Effect_CommitFadeIris();
            assert(Port_Effect_ResolveFadeIris(p[i][0],p[i][1],p[i][2],p[i][3],rows,192));
            for (int j=0;j<p[i][1];j++) assert(rows[j].left<=rows[j].right && rows[j].right<=p[i][0]);
        }
    return 0;
}
