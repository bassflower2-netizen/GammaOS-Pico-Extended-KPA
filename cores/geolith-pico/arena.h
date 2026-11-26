#ifndef GEO_ARENA_H
#define GEO_ARENA_H
#include <stddef.h>
struct geo_arena { unsigned char *base; size_t capacity, used; };
int geo_arena_init(struct geo_arena *a, void *base, size_t capacity);
void *geo_arena_sbrk(struct geo_arena *a, ptrdiff_t delta);
#endif
