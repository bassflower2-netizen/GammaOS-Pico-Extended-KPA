#ifndef GEO_DEVICE_SESSION_H
#define GEO_DEVICE_SESSION_H
#include "geo_adapter.h"
#include "native_file.h"
#include "native_platform.h"
/* Initialize a freshly loaded private core once, before any newlib allocation.
 * Heap and provider context must remain valid until core unload. */
int geo_device_init(void *heap,size_t capacity,geo_native_log_sink,
                    geo_clock_fn,geo_fatal_fn,void *ctx);
/* Open one direct child of /roms/neo on a caller-owned mounted volume.
 * Scratch (>=4096 bytes) is used only during validation and content hashing.
 * namespace_aes selects save storage only; actual upstream mode selection is
 * not yet wired. Caller must ensure it matches the running engine mode.
 * Volume and callbacks must remain valid through close. */
int geo_device_open(fat_vol *,const char *path,unsigned namespace_aes,
                    void *scratch,size_t capacity,const geo_pico_callbacks *);
int geo_device_frame(uint16_t,uint16_t);
int geo_device_reset(void);
size_t geo_device_state_size(void);
int geo_device_save(int slot,void *scratch,size_t capacity);
int geo_device_load(int slot,void *scratch,size_t capacity);
void geo_device_close(void);
#endif
