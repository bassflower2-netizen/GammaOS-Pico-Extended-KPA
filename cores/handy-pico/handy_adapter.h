#ifndef HANDY_PICO_ADAPTER_H
#define HANDY_PICO_ADAPTER_H
#include <stddef.h>
#include <stdint.h>
/* Temporary portable interface; not the final LK blob ABI. Buttons use libretro
 * joypad bit indices. Video data is RGB565, owned by the core until next callback.
 * Audio is signed 16-bit interleaved stereo at 48 kHz. */
struct handy_pico_callbacks {
    void (*video)(const uint16_t *, unsigned, unsigned, size_t);
    size_t (*audio)(const int16_t *, size_t);
};
int handy_pico_open(const void *rom, size_t size, const struct handy_pico_callbacks *cb);
void handy_pico_close(void);
int handy_pico_frame(uint16_t buttons);
void handy_pico_reset(void);
size_t handy_pico_state_size(void);
int handy_pico_save(void *dst, size_t size);
int handy_pico_load(const void *src, size_t size);
void *handy_pico_ram(void);
size_t handy_pico_ram_size(void);
#endif
