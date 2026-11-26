#ifndef GEO_SD_CART_H
#define GEO_SD_CART_H
#include <stddef.h>
#include "../../emu/gba/fat_ro.h"
/* Caller owns mounted volume and ROM arena. On error discard destination. */
int geo_sd_cart_load(fat_vol *,const char *,void *,size_t);
/* aes=1 selects aes.zip, aes=0 selects neogeo.zip. On failure loaded=0. */
int geo_sd_bios_load(fat_vol *,int,void *,size_t,size_t *);
#endif
