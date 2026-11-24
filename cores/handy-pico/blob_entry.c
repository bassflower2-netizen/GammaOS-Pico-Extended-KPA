/* Experimental bootloader entry. No Linux syscalls; never include qemu_entry.c. */
#include "handy_blob_abi.h"
#include <stddef.h>
extern void __libc_init_array(void);
static const struct handy_blob_exports api = {
    HANDY_BLOB_MAGIC, HANDY_BLOB_VERSION,
    handy_pico_open, handy_pico_close, handy_pico_frame, handy_pico_reset,
    handy_pico_state_size, handy_pico_save, handy_pico_load,
    handy_pico_ram, handy_pico_ram_size
};
const struct handy_blob_exports *handy_blob_entry(void)
{
    static int initialized;
    if (!initialized) {
        __libc_init_array();
        initialized = 1;
    }
    return &api;
}
/* Newlib console output is intentionally discarded; no operating system. */
int _write(int fd, const void *buf, size_t n)
{ (void)fd; (void)buf; return (int)n; }
/* A fatal core error traps rather than issuing a syscall. Actual LK fault
 * recovery must be designed before this is used on hardware. */
__attribute__((noreturn)) void _exit(int status)
{
    (void)status;
    __asm__ volatile("udf #0");
    for (;;) {}
}
