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
    free(q);
    return 0;
}
