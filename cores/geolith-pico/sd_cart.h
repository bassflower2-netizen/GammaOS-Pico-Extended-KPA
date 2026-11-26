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
/* Only classic STORED members (method 0); CRC32 must match before copy.
 * Caller must only use spans returned from a validated archive. */
int geo_sd_bios_member_copy_stored(const geo_zip_member *,void *,size_t,size_t *);
/* Called for method 8 with raw DEFLATE input (no zlib/gzip wrapper).
 * Implementations must report exact consumed and produced byte counts.
 * On error, output bytes are invalid even if the destination was modified.
 * Method 0 uses the existing checked STORED copy and ignores inflater. */
typedef int (*geo_zip_raw_inflater)(void *context,const unsigned char *input,
                                    uint32_t input_size,void *output,
                                    uint32_t output_capacity,
                                    uint32_t *consumed,uint32_t *produced);
int geo_sd_bios_member_decode(const geo_zip_member *,void *,size_t,
                              geo_zip_raw_inflater,void *,size_t *);
/* End-to-end read-only FAT16/FAT32 BIOS member loading. Archives and
 * output must occupy disjoint caller-owned buffers; outputs are invalid on
 * failure. This does not initialize Geolith or reserve device RAM. */
int geo_sd_bios_member_from_fat(fat_vol *,int,const char *,
                                void *,size_t,void *,size_t,
                                geo_zip_raw_inflater,void *,size_t *);
/* Interface boundary only: accept() is invoked exactly once after a
 * successful full read, raw inflate, CRC verification and exact-size check.
 * A returned failure is propagated. No firmware engine calls are made here. */
typedef int (*geo_bios_accept)(void *,const void *,size_t);
int geo_sd_bios_handoff(fat_vol *,int,const char *,
                        void *,size_t,void *,size_t,size_t,
                        geo_zip_raw_inflater,void *,
                        geo_bios_accept,void *);
/* Four slots in order: program BIOS, LO, SFIX, SM1. All contents invalid
 * on failure; caller must not start engine until success. Fixed sizes are
 * the baseline system ROM layout, not support for every BIOS variant. */
typedef struct { void *data; size_t capacity; } geo_bios_slot;
int geo_sd_bios_set_load(fat_vol *,int,const char *,void *,size_t,
                         geo_bios_slot [4],geo_zip_raw_inflater,void *);
/* Callback receives all four verified slots together, in baseline order.
 * Buffers remain caller-owned; callback must copy or respect their lifetime.
 * Callback failure is propagated; caller is responsible for engine rollback. */
typedef int (*geo_bios_set_accept)(void *,int,const geo_bios_slot [4]);
int geo_sd_bios_set_handoff(fat_vol *,int,const char *,void *,size_t,
                            geo_bios_slot [4],geo_zip_raw_inflater,void *,
                            geo_bios_set_accept,void *);
#endif
