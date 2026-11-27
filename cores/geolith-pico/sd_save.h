#ifndef GEO_SD_SAVE_H
#define GEO_SD_SAVE_H
#include "../../emu/gba/fat_ro.h"
/* Caller supplies a stable cartridge identity, not a ROM filename.
 * slot -1 = persistent RAM, 0..9 = separate state slots.
 * Return 0 on success, -1 invalid argument/no volume, -2 unavailable
 * or wrong size, -3 storage I/O failure. Discard read buffer on failure. */
int geo_sd_save_put(fat_vol *v,uint64_t key,unsigned aes,int slot,const void *buf,uint32_t len);
int geo_sd_save_get(fat_vol *v,uint64_t key,unsigned aes,int slot,void *buf,uint32_t len);
#endif
