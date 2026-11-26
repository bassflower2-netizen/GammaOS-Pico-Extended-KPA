#ifndef GEO_NATIVE_HEAP_H
#define GEO_NATIVE_HEAP_H
#include <stddef.h>
/* Bind once per freshly loaded core, before any newlib allocation. */
int geo_heap_bind(void *base, size_t capacity);
void *geo_heap_break(ptrdiff_t delta);
#endif
