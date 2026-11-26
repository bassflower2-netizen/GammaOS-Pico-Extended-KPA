#ifndef GEO_ZLIB_H
#define GEO_ZLIB_H
#include "sd_cart.h"
/* Uses the linked zlib allocator, which must be backed by the caller's
 * bounded native heap before invoking the adapter on bare metal.
 * No BIOS bytes are trusted until geo_sd_bios_member_decode verifies CRC. */
int geo_zip_zlib_raw_inflate(void *,const unsigned char *,uint32_t,void *,
                             uint32_t,uint32_t *,uint32_t *);
#endif
