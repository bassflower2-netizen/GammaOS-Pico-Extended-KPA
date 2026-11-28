#ifndef LK_GEO_VIDEO_H
#define LK_GEO_VIDEO_H
#include <stdint.h>
#define GEO_VIDEO_MAX_W 320u
#define GEO_VIDEO_MAX_H 264u
/* Convert an XRGB8888 callback to tightly packed RGB565 for GammaOS.
 * dst capacity is in pixels; pitch is in bytes. NULL source is a duplicate
 * frame (return 1, leave destination untouched). Malformed input: -1. */
int geo_video_rgb565(uint16_t *dst,uint32_t capacity,const void *src,
                     uint32_t w,uint32_t h,uint32_t pitch);
#endif
