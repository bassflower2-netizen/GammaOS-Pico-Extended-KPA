/* Experimental bounded newlib heap and C++ glue.
 * This file does not implement LK memory allocation or a final blob ABI.
 * The heap below belongs only to the standalone ARM test executable. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

alignas(16) static unsigned char heap[8u * 1024u * 1024u];
static size_t used;
extern "C" void *_sbrk(ptrdiff_t increment)
{
    if (increment >= 0) {
        size_t n = (size_t)increment;
        if (n > sizeof(heap) - used) { errno = ENOMEM; return (void *)-1; }
        void *old = heap + used;
        used += n;
        return old;
    }
    size_t n = (size_t)(-(increment + 1)) + 1u;
    if (n > used) { errno = ENOMEM; return (void *)-1; }
    void *old = heap + used;
    used -= n;
    return old;
}
void *operator new(size_t n) { void *p = malloc(n ? n : 1); if (!p) abort(); return p; }
void *operator new[](size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { free(p); }
void operator delete[](void *p) noexcept { free(p); }
void operator delete(void *p, size_t) noexcept { free(p); }
void operator delete[](void *p, size_t) noexcept { free(p); }

extern "C" {
void *__dso_handle = 0;
int __cxa_atexit(void (*fn)(void *), void *obj, void *dso)
{ (void)fn; (void)obj; (void)dso; return 0; }
int __aeabi_atexit(void *obj, void (*fn)(void *), void *dso)
{ return __cxa_atexit(fn, obj, dso); }
void __cxa_pure_virtual(void) { abort(); }
void _init(void) {}
void _fini(void) {}
}
