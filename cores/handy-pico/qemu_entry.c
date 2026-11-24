/* QEMU user-mode test harness ONLY. Never link into an LK blob.
 * Linux syscalls here are restricted to console diagnostics and process exit.
 * The emulator VFS is independently disabled in native_vfs.c. */
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

extern void __libc_init_array(void);
extern int main(void);

static long syscall3(long number, long a, long b, long c)
{
    register long r0 asm("r0") = a;
    register long r1 asm("r1") = b;
    register long r2 asm("r2") = c;
    register long r7 asm("r7") = number;
    asm volatile("svc 0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory", "cc");
    return r0;
}
int _write(int fd, const void *buf, size_t n)
{
    if (fd != 1 && fd != 2) return -1;
    return (int)syscall3(4, fd, (long)buf, (long)n);
}
__attribute__((noreturn)) void _exit(int status)
{
    syscall3(1, status, 0, 0);
    for (;;) {}
}
__attribute__((noreturn)) void probe_start(void)
{
    __libc_init_array();
    int status = main();
    fflush(NULL);
    _exit(status);
}
