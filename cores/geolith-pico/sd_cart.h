#ifndef GEO_SD_CART_H
#define GEO_SD_CART_H
#include <stddef.h>
#include "../../emu/gba/fat_ro.h"
/* Caller owns mounted volume and ROM arena. On error discard destination. */
int geo_sd_cart_load(fat_vol *,const char *,void *,size_t);
/* aes=1 selects aes.zip, aes=0 selects neogeo.zip. On failure loaded=0. */
int geo_sd_bios_load(fat_vol *,int,void *,size_t,size_t *);
/* ZIP central-directory structure only; no decompression/CRC verification. */
int geo_sd_bios_zip_directory_valid(const void *,size_t);
/* ZIP member span is owned by the input archive; do not free or retain it
 * beyond the archive lifetime. Deflate members remain compressed (method 8).
 * Caller must verify decompression size and CRC before using BIOS bytes. */
typedef struct {
    const unsigned char *data;
    uint32_t compressed_size,uncompressed_size,crc32;
    unsigned short method;
} geo_zip_member;
int geo_sd_bios_member_find(const void *,size_t,const char *,geo_zip_member *);
#endif
