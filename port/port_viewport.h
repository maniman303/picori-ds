#ifndef PORT_VIEWPORT_H
#define PORT_VIEWPORT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Version 1 is the public PNG size/directory contract. These are logical
 * square-pixel dimensions, independent of window size and internal scale.
 * Except for DS at 256x192, height is 160 and width is ceil(160 * aspect).
 * Fractional widths round up to a pixel, without even-width/tile alignment.
 * Hardware timing and original source coordinates remain 240x160. */
#define PORT_VIEWPORT_PROFILE_VERSION 1
#define PORT_VIEWPORT_NATIVE_WIDTH 240
#define PORT_VIEWPORT_NATIVE_HEIGHT 160

typedef struct {
    const char* aspect;
    const char* directory;
    int width;
    int height;
} PortViewportProfile;

typedef struct {
    int x, y, width, height;
} PortViewportRect;

/* Activation-time lookup by canonical override directory, e.g. "43".
 * Unknown/NULL keys return NULL; do not silently substitute another ratio.
 * Profiles describe the asset contract, not current renderer capabilities. */
const PortViewportProfile* Port_Viewport_FindProfile(const char* directory);

/* Reject insufficient capacity rather than truncating the selected profile.
 * A renderer must also validate raster/sprite support before exposing a mode. */
bool Port_Viewport_ProfileFits(const PortViewportProfile* profile, int capacityWidth, int capacityHeight);

/* One translation for the complete original fixed-screen composition. This
 * rectangle does not describe the world aperture or live-gameplay UI bounds. */
PortViewportRect Port_Viewport_NativeContentRect(const PortViewportProfile* profile);

#ifdef __cplusplus
}
#endif

#endif
