#ifndef GEO_SD_CART_H
#define GEO_SD_CART_H
#include <stddef.h>
#include "../../emu/gba/fat_ro.h"
/* Caller owns mounted volume and ROM arena. On error discard destination. */
int geo_sd_cart_load(fat_vol *,const char *,void *,size_t);
#endif
