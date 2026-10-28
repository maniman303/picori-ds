#ifndef PORT_BACKGROUND_PNG_H
#define PORT_BACKGROUND_PNG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "port_screen_background.h"
#include "port_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Decode the exact catalog image in root/override/<profile>/<name>.png into
 * tightly packed RGBA8. The caller supplies at least width*height*4 bytes.
 * Failure leaves the destination untouched. This performs file I/O and must
 * be called at an installation/reload boundary, not during presentation. */
bool Port_Background_LoadPng(const char* root, PortBackgroundContext context,
                             PortAspectMode aspect, bool parity,
                             uint8_t* rgba, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif
