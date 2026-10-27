/* Exercise the production PPU2 writer guard without a running game. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "virtuappu.h"
#include "cpu/mode1.h"
#include "ppu_snapshot.h"

uint8_t gIoMem[0x400], gVram[0x18000];
uint16_t gBgPltt[256], gObjPltt[256], gOamMem[512], gPaletteBuffer[512];
PPUMemory virtuappu_registers;
static int metadata;
bool virtuappu_mode1_has_obj_positions(void) { return metadata; }
static uint64_t perfcap_frame_hash(void) { return 0; }
#include "perfcap_dump.inc"
int main(void) {
    const char* path = "snapshot-guard.bin";
    FILE* f = fopen(path, "wb"); assert(f); assert(fwrite("keep", 1, 4, f) == 4); assert(fclose(f) == 0);
    metadata = 1; assert(!perfcap_dump(path));
    char contents[4]; f = fopen(path, "rb"); assert(f); assert(fread(contents, 1, 4, f) == 4);
    assert(memcmp(contents, "keep", 4) == 0); fclose(f);
    metadata = 0; assert(perfcap_dump(path));
    f = fopen(path, "rb"); assert(f); assert(fread(contents, 1, 4, f) == 4);
    assert(memcmp(contents, "PPU2", 4) == 0); fclose(f); remove(path);
    puts("PPU2 capture rejects unrecorded signed OBJ metadata: PASS");
    return 0;
}
