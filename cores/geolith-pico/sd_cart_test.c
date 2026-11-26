#include "sd_cart.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define SIZE (4096u+0x100000u+0x10000u+0x10000u+1u+0x100000u)
static unsigned char dst[SIZE+1];
static int fail;
static unsigned rd(void *ctx,uint32_t lba,uint32_t n,void *p)
{
    FILE *f=ctx;
    if(fail||fseek(f,(long)lba*512,SEEK_SET))return 0;
    return (unsigned)fread(p,512,n,f);
}
int main(int argc,char **argv)
{
    assert(argc==2);
    FILE *f=fopen(argv[1],"rb");assert(f);
    fat_vol v;assert(!fat_mount(&v,rd,f));
    dst[SIZE]=0x7e;
    assert(!geo_sd_cart_load(&v,"/roms/neo/Test Cart.NEO",dst,SIZE));
    assert(dst[4096]==0x5a&&dst[SIZE-1]==0x5a&&dst[SIZE]==0x7e);
    assert(geo_sd_cart_load(&v,"/roms/neo/Test Cart.NEO",dst,SIZE-1)<0);
    assert(geo_sd_cart_load(&v,"/roms/neo/../neo/Test Cart.NEO",dst,SIZE)<0);
    assert(geo_sd_cart_load(&v,"/roms/neo/missing.neo",dst,SIZE)<0);
    assert(geo_sd_cart_load(&v,"/roms/neo/bad.neo",dst,SIZE)<0);
    fail=1;assert(geo_sd_cart_load(&v,"/roms/neo/Test Cart.NEO",dst,SIZE)<0);
    fclose(f);puts("Neo Geo FAT cartridge fixture passed");return 0;
}
