#define PCE_NATIVE_CD 1
#include "pce_blob_abi.h"
extern void __libc_init_array(void);
static const struct pce_blob_exports api={
    PCE_BLOB_MAGIC,PCE_BLOB_VERSION,pce_pico_open,pce_pico_close,
    pce_pico_frame,pce_pico_reset,pce_pico_state_size,pce_pico_save,
    pce_pico_load,pce_pico_ram,pce_pico_ram_size,pce_pico_bram,pce_pico_bram_size,pce_pico_open_cd
};
const struct pce_blob_exports *pce_blob_entry(void)
{
    static int initialized;
    if(!initialized){__libc_init_array();initialized=1;}
    return &api;
}
int _write(int fd,const void *p,size_t n){(void)fd;(void)p;return (int)n;}
__attribute__((noreturn)) void _exit(int status)
{(void)status;__asm__ volatile("udf #0");for(;;){}}
