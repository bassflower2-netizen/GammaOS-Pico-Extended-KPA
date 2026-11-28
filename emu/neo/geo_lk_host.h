#ifndef LK_GEO_HOST_H
#define LK_GEO_HOST_H
#include "geo_session.h"
/* Platform-facing entry point. The caller must already own a mapped,
 * exclusive arena and the emulator overlay, with all other cores parked.
 * Run frames/reset/save/load using geo_session_* on the same thread.
 * Call geo_lk_end before releasing either memory range. */
int geo_lk_begin(fat_vol *,const char *,uint32_t,void *,uint32_t);
void geo_lk_end(void);
#endif
