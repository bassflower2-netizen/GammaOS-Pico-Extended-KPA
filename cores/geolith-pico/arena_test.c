#include "arena.h"
#include <assert.h>
#include <stdint.h>
#include <errno.h>
int main(void)
{
    _Alignas(16) unsigned char buf[64];
    struct geo_arena a = {0};
    assert(geo_arena_sbrk(&a, 0) == (void *)-1);
    assert(geo_arena_init(&a, buf+1, 63) == -1);
    assert(geo_arena_init(&a, buf, 64) == 0);
    assert(geo_arena_sbrk(&a, 16) == buf);
    assert(geo_arena_sbrk(&a, 48) == buf+16);
    assert(geo_arena_sbrk(&a, 1) == (void *)-1 && errno == ENOMEM);
    assert(geo_arena_sbrk(&a, PTRDIFF_MIN) == (void *)-1);
    assert(geo_arena_sbrk(&a, 0) == buf+64);
    assert(geo_arena_sbrk(&a, -64) == buf+64);
    assert(geo_arena_sbrk(&a, -1) == (void *)-1);
    assert(geo_arena_sbrk(&a, 0) == buf);
    return 0;
}
