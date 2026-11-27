/* Newlib lacks POSIX allocation entry point in the selected toolchain.
 * memalign uses the same allocator as free; no hidden pointer metadata. */
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <stdint.h>
int posix_memalign(void **out, size_t alignment, size_t size)
{
    if (!out || alignment < sizeof(void *) ||
        alignment % sizeof(void *) || (alignment & (alignment - 1)))
        return EINVAL;
    /* Reject arithmetic overflow before entering newlib memalign. */
    if (size > SIZE_MAX - alignment - sizeof(void *) * 8u)
        return ENOMEM;
    void *p = memalign(alignment, size ? size : 1);
    if (!p) return ENOMEM;
    *out = p;
    return 0;
}
