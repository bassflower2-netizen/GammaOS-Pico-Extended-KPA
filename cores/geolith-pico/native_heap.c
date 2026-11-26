#include "native_heap.h"
#include "arena.h"
#include <errno.h>
static struct geo_arena heap;
int geo_heap_bind(void *base, size_t capacity)
{
    if (heap.base) { errno = EBUSY; return -1; }
    return geo_arena_init(&heap, base, capacity);
}
void *geo_heap_break(ptrdiff_t delta)
{
    return geo_arena_sbrk(&heap, delta);
}
#ifdef GEO_NEWLIB
void *_sbrk(ptrdiff_t delta)
{
    return geo_heap_break(delta);
}
#endif
