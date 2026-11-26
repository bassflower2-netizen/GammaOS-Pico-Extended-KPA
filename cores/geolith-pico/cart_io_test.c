#include "cart_io.h"
#include "neo_format.h"
#include <assert.h>
#include <string.h>
#define SIZE (4096u+0x100000u+0x10000u+0x10000u+1u+0x100000u)
static unsigned char dst[SIZE+1],hdr[4096];
struct fixture { unsigned calls; int fail; };
static int rd(void *ctx,uint32_t off,void *buf,uint32_t n){
    struct fixture *f=ctx; f->calls++;
    assert(off<=SIZE && n<=SIZE-off && n<=32768);
    if(!off){assert(n==4096);memcpy(buf,hdr,n);}
    else {if(f->fail)return (int)n-1;memset(buf,0x5a,n);}
    return (int)n;
}
static void put(unsigned i,uint32_t n){for(unsigned j=0;j<4;j++)hdr[4+4*i+j]=(unsigned char)(n>>(8*j));}
int main(void){
    struct fixture f={0,0};
    memcpy(hdr,"NEO\1",4);put(0,0x100000);put(1,0x10000);put(2,0x10000);put(3,1);put(5,0x100000);
    dst[SIZE]=0x7e;
    assert(geo_cart_load(rd,&f,SIZE,dst,SIZE)==0);
    assert(dst[4096]==0x5a && dst[SIZE-1]==0x5a && dst[SIZE]==0x7e);
    f.calls=0;assert(geo_cart_load(rd,&f,SIZE,dst,SIZE-1)<0 && !f.calls);
    hdr[0]=0;assert(geo_cart_load(rd,&f,SIZE,dst,SIZE)<0 && f.calls==1);
    hdr[0]='N';f.fail=1;assert(geo_cart_load(rd,&f,SIZE,dst,SIZE)<0);
    assert(pico_neo_validate_header(hdr,4095,SIZE)<0);
    return 0;
}
