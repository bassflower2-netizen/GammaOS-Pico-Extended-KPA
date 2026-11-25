#ifndef PCE_SD_IO_H
#define PCE_SD_IO_H
#include "../gba/fat_ro.h"
#include "../../cores/pce-fast-pico/pce_cd_io.h"
/* One exclusive CD session at a time, owned by the emulation thread.
 * The caller supplies the real directory of the selected, unarchived CUE. */
int pce_sd_io_begin(fat_vol *,const char *directory);
void pce_sd_io_end(void);
extern const struct pce_cd_io pce_sd_io;
#endif
