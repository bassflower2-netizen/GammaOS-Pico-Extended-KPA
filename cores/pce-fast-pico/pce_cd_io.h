#ifndef PCE_CD_IO_H
#define PCE_CD_IO_H
#include <stddef.h>
#include <stdint.h>
/* Virtual read-only paths. The frontend maps /pico/disc/<basename> to the
 * selected CUE's directory and /pico/bios/syscard3.pce to its BIOS folder.
 * Handle IDs are nonnegative, -1 on open failure. Reads are positional.
 * The frontend owns all handles until close; no callback after session end. */
struct pce_cd_io {
    int (*open)(const char *,uint32_t *);
    int (*read_at)(int,uint32_t,void *,uint32_t);
    void (*close)(int);
};
int pce_vfs_begin(const struct pce_cd_io *);
void pce_vfs_end(void);
int pce_vfs_failed(void);
int pce_vfs_bios_present(void);
#endif
