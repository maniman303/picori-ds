/**
 * Per-region runtime remap for compiled blob offsets (M7 multi-region fix).
 * The committed mapping tables in port/port_offset_remap.c are authoritative.
 *
 * Compiled-offset helpers translate from the build's selected asset baseline
 * (USA, EU or JP) to the active ROM. The USA helpers are for explicitly
 * USA-based literals. ROM-native and asset-extracted offsets are already
 * region-correct and must not be translated.
 *
 *   Port_RemapMapOffset — indices into gMapData          (dungeon maps,
 *                         cave borders, gyorg mappings)
 *   Port_RemapGfxOffset — indices into gGlobalGfxAndPalettes (obj palettes,
 *                         Hyrule Town tilesets). JP == USA for this blob.
 *
 * Identity on USA, for non-divergent offsets, and on non-multi-region builds.
 */
#ifndef PORT_OFFSET_REMAP_H
#define PORT_OFFSET_REMAP_H

#include "global.h"

#if defined(PC_PORT) && defined(MULTI_REGION)
u32 Port_RemapMapOffset(u32 usa_offset);
u32 Port_RemapGfxOffset(u32 usa_offset);
u32 Port_RemapCompiledMapOffset(u32 offset);
u32 Port_RemapCompiledGfxOffset(u32 offset);
#else
#define Port_RemapMapOffset(off) (off)
#define Port_RemapGfxOffset(off) (off)
#define Port_RemapCompiledMapOffset(off) (off)
#define Port_RemapCompiledGfxOffset(off) (off)
#endif

#endif /* PORT_OFFSET_REMAP_H */
