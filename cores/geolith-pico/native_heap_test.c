#include "native_heap.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
int main(void)
{
    _Alignas(16) unsigned char buf[64], other[64];
    assert(geo_heap_break(0) == (void *)-1 && errno == EINVAL);
    assert(geo_heap_bind(buf+1, 63) == -1);
    assert(geo_heap_bind(buf, sizeof buf) == 0);
    assert(geo_heap_break(16) == buf);
    assert(geo_heap_bind(other, sizeof other) == -1 && errno == EBUSY);
    assert(geo_heap_break(48) == buf+16);
    assert(geo_heap_break(1) == (void *)-1 && errno == ENOMEM);
    assert(geo_heap_break(PTRDIFF_MIN) == (void *)-1);
    assert(geo_heap_break(-64) == buf+64);
    assert(geo_heap_break(0) == buf);
    assert(geo_heap_bind(other, sizeof other) == -1 && errno == EBUSY);
    return 0;
}
