/* Private 8 MiB arena for newlib, reset when a new blob is loaded. */
#include <stddef.h>
#include <errno.h>
static unsigned char heap[8u*1024u*1024u] __attribute__((aligned(16)));
static size_t used;
void *_sbrk(ptrdiff_t n)
{
    if(n>=0){
        if((size_t)n>sizeof heap-used){errno=ENOMEM;return (void *)-1;}
        void *p=heap+used;used+=(size_t)n;return p;
    }
    size_t k=(size_t)(-(n+1))+1u;
    if(k>used){errno=ENOMEM;return (void *)-1;}
    void *p=heap+used;used-=k;return p;
}
void _init(void){}
void _fini(void){}
