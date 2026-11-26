#include "native_heap.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
/* Link real newlib allocation routines against the private break hook.
 * This probe is not executable on the host and is never packaged. */
static _Alignas(16) unsigned char arena[65536];
int geo_newlib_probe(void)
{
    if (geo_heap_bind(arena, sizeof arena)) return 1;
    unsigned char *p = calloc(32, 4);
    if (!p) return 2;
    for (size_t i = 0; i < 128; ++i) if (p[i]) return 3;
    memset(p, 0x5a, 128);
    unsigned char *q = realloc(p, 256);
    if (!q) { free(p); return 4; }
    for (size_t i = 0; i < 128; ++i) if (q[i] != 0x5a) return 5;
    /* Exhaustion must fail without corrupting the live allocation. */
    volatile size_t oversized = sizeof arena * 2u;
    void *too_large = malloc(oversized);
    if (too_large) { free(too_large); return 6; }
    for (size_t i = 0; i < 128; ++i) if (q[i] != 0x5a) return 7;
    free(q);
    p = calloc(16, 4);
    if (!p) return 8;
    for (size_t i = 0; i < 64; ++i) if (p[i]) return 9;
    free(p);
    return 0;
}
