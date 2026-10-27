#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#define TMC_SHM_HEADER 24u
#define TMC_SHM_OAM_BYTES 1024u
static uint8_t storage[TMC_SHM_HEADER + 8 * 8 * 4 + TMC_SHM_OAM_BYTES];
static uint8_t* sShmBase = storage;
static size_t sShmBytes = sizeof(storage);
static uint32_t sFrameSeq;
uint16_t gOamMem[512];
static bool metadata;
static bool shmEnabled() { return true; }
static int shmInit(int, int) { return 0; }
bool virtuappu_mode1_has_obj_positions() { return metadata; }
class ShmFrameSource { public: void* mBase = nullptr; uint32_t mVersion = 4; int oamCount() const; };
#include "shm_oam_functions.inc"
int main() {
    uint32_t pixels[64] = {};
    auto* h = reinterpret_cast<uint32_t*>(storage);
    h[2] = 8; h[3] = 8;
    ShmFrameSource reader; reader.mBase = storage;
    metadata = false; Port_Shm_PublishFramebuffer(pixels, 8, 8);
    assert(reader.oamCount() == 128 && h[4] == 1);
    metadata = true; Port_Shm_PublishFramebuffer(pixels, 8, 8);
    assert(reader.oamCount() == 0 && h[4] == 2);
    metadata = false; Port_Shm_PublishFramebuffer(pixels, 8, 8);
    assert(reader.oamCount() == 128 && h[4] == 3);
    h[5] = 129; assert(reader.oamCount() == 0);
    reader.mVersion = 1; assert(reader.oamCount() == 0);
    std::puts("Shared-framebuffer OAM fallback: PASS");
}
