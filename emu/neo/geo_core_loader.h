#ifndef LK_GEO_CORE_LOADER_H
#define LK_GEO_CORE_LOADER_H
#include "../gba/fat_ro.h"
#include "../../cores/geolith-pico/geo_blob_abi.h"
/* Metadata only: no overlay writes or execution. */
int geo_core_check(fat_vol *v);
/* Caller exclusively owns the shared overlay until close, including any
 * external heap. Loader returns an uninitialized export table. */
const struct geo_blob_exports *geo_core_load(fat_vol *v,int *error);
#endif
