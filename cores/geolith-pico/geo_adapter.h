#ifndef GEO_PICO_ADAPTER_H
#define GEO_PICO_ADAPTER_H
#include <stddef.h>
#include <stdint.h>
/* Single-session, synchronous callbacks. Video XRGB8888, pitch in bytes;
 * audio interleaved signed 16-bit stereo; pad bits follow libretro joypad IDs.
 * The system/save directories and cartridge path are copied at open. */
typedef struct {
    void (*video)(const void *,unsigned,unsigned,size_t);
    size_t (*audio)(const int16_t *,size_t);
    void (*log)(const char *);
} geo_pico_callbacks;
int geo_pico_open(const char *path,const char *system_dir,const char *save_dir,
                  const geo_pico_callbacks *);
void geo_pico_close(void);
int geo_pico_frame(uint16_t pad1,uint16_t pad2);
int geo_pico_reset(void);
size_t geo_pico_state_size(void);
int geo_pico_save(void *,size_t);
int geo_pico_load(const void *,size_t);
#endif
