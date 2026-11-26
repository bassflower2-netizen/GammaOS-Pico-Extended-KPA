#include "arena.h"
#include <stdint.h>
#include <errno.h>
int geo_arena_init(struct geo_arena *a, void *base, size_t capacity)
{
    if (!a || !base || !capacity || ((uintptr_t)base & 15u) ||
        capacity > PTRDIFF_MAX || capacity > UINTPTR_MAX - (uintptr_t)base) {
        errno = EINVAL; return -1;
    }
    a->base = base; a->capacity = capacity; a->used = 0;
    return 0;
}
void *geo_arena_sbrk(struct geo_arena *a, ptrdiff_t delta)
{
    if (!a || !a->base || a->used > a->capacity) {
        errno = EINVAL; return (void *)-1;
    }
    size_t n = delta < 0 ? (size_t)(-(delta + 1)) + 1u : (size_t)delta;
    if (delta < 0 ? n > a->used : n > a->capacity - a->used) {
        errno = ENOMEM; return (void *)-1;
    }
    void *old = a->base + a->used;
    if (delta < 0) a->used -= n; else a->used += n;
    return old;
}
