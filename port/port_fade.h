#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PC-only helpers for the palette fade pipeline. FadeVBlank() transforms the
 * engine's raw gPaletteBuffer into palette RAM with per-bank factor/bias; the
 * port can reproduce the same transform for a palette that is not currently
 * resident (view-dependent charset groups). */
void Port_Fade_RecordVBlank(void);
void Port_Fade_TransformPaletteBank(const uint16_t* src, uint16_t* dst, unsigned bank);

#ifdef __cplusplus
}
#endif
