#ifndef PCE_FAST_PICO_ADAPTER_H
#define PCE_FAST_PICO_ADAPTER_H
#include <stddef.h>
#include <stdint.h>
/* Experimental portable HuCard interface; not yet an LK blob ABI.
 * Video: RGB565, pitch in bytes. Audio: stereo signed 16-bit at 44100 Hz.
 * Input: libretro joypad bit indices. Callbacks synchronous on emulation thread. */
struct pce_pico_callbacks {
    void (*video)(const uint16_t *,unsigned,unsigned,size_t);
    size_t (*audio)(const int16_t *,size_t);
};
int pce_pico_open(const void *,size_t,const struct pce_pico_callbacks *);
void pce_pico_close(void);
int pce_pico_frame(uint16_t);
void pce_pico_reset(void);
size_t pce_pico_state_size(void);
int pce_pico_save(void *,size_t);
int pce_pico_load(const void *,size_t);
void *pce_pico_ram(void);
size_t pce_pico_ram_size(void);
void *pce_pico_bram(void);
size_t pce_pico_bram_size(void);
#ifdef PCE_NATIVE_CD
#include "pce_cd_io.h"
/* Native CD entry exported by ABI 2. */
int pce_pico_open_cd(const char *,const struct pce_cd_io *,const struct pce_pico_callbacks *);
#endif
#endif
