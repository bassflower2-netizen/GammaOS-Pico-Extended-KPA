#include "cart_io.h"
#include "neo_format.h"
int geo_cart_load(geo_cart_read_at read_at,void *ctx,uint32_t size,void *dst,size_t capacity)
{
    uint8_t *p=dst;
    if(!read_at||!p||size<PICO_NEO_HEADER||size>PICO_NEO_MAX||capacity<size) return -1;
    if(read_at(ctx,0,p,PICO_NEO_HEADER)!=(int)PICO_NEO_HEADER ||
       pico_neo_validate_header(p,PICO_NEO_HEADER,size)) return -1;
    for(uint32_t off=PICO_NEO_HEADER;off<size;){
        uint32_t n=size-off;
        if(n>32768u)n=32768u;
        if(read_at(ctx,off,p+off,n)!=(int)n)return -1;
        off+=n;
    }
    return 0;
}

uint64_t geo_cart_key(const void *data,size_t size)
{
    const uint8_t *p=data;
    uint64_t key=UINT64_C(14695981039346656037);
    for(size_t i=0;i<size;i++){
        key^=p[i];key*=UINT64_C(1099511628211);
    }
    return key;
}
