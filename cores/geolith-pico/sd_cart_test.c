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
    size_t loaded=99;
    assert(!geo_sd_bios_load(&v,1,dst,SIZE,&loaded)&&loaded>22);
    assert(dst[0]=='P'&&dst[1]=='K');
    assert(!geo_sd_bios_zip_directory_valid(dst,loaded));
    geo_zip_member m={0};
    assert(!geo_sd_bios_member_find(dst,loaded,"neo-po.bin",&m));
    assert(m.data>=dst&&m.data<dst+loaded&&m.compressed_size>0);
    assert(m.method==8||m.method==0);
    assert((size_t)(m.data-dst)+m.compressed_size<=loaded);
    assert(geo_sd_bios_member_find(dst,loaded,"missing.bin",&m)<0);
    assert(m.data==NULL&&m.compressed_size==0);
    assert(geo_sd_bios_member_find(dst,loaded,"neo-po.bin",NULL)<0);
    assert(geo_sd_bios_member_find(dst,loaded,NULL,&m)<0);
    assert(geo_sd_bios_member_find(dst,loaded,"neo-po.bin",&m)==0);
    assert(geo_sd_bios_zip_directory_valid(dst,loaded-1)<0);
    unsigned char saved=dst[loaded-22];
    dst[loaded-22]=0;
    assert(geo_sd_bios_zip_directory_valid(dst,loaded)<0);
    dst[loaded-22]=saved;
    saved=dst[loaded-6];dst[loaded-6]=0xff;
    assert(geo_sd_bios_zip_directory_valid(dst,loaded)<0);
    dst[loaded-6]=saved;
    saved=dst[loaded-16];dst[loaded-16]=1;
    assert(geo_sd_bios_zip_directory_valid(dst,loaded)<0);
    dst[loaded-16]=saved;
    assert(geo_sd_bios_zip_directory_valid(NULL,loaded)<0);
    /* Corrupt a later central entry: the first entry alone is insufficient. */
    size_t cd=0, second=0;
    for(size_t k=0;k+4<loaded;k++){
        if(dst[k]=='P'&&dst[k+1]=='K'&&dst[k+2]==1&&dst[k+3]==2){
            if(cd) {second=k;break;}
            cd=k;
        }
    }
    assert(cd&&second);
    saved=dst[second];dst[second]=0;
    assert(geo_sd_bios_zip_directory_valid(dst,loaded)<0);
    dst[second]=saved;
    /* Make the compressed payload overlap the central directory. */
    unsigned char original_size[4];
    for(unsigned i=0;i<4;i++){
        original_size[i]=dst[cd+20+i];
        dst[cd+20+i]=0xff;
    }
    assert(geo_sd_bios_zip_directory_valid(dst,loaded)<0);
    for(unsigned i=0;i<4;i++)dst[cd+20+i]=original_size[i];
    saved=dst[cd+8];dst[cd+8]|=1;
    assert(geo_sd_bios_zip_directory_valid(dst,loaded)<0);
    dst[cd+8]=saved;
    /* Header count must match the entries walked to directory end. */
    saved=dst[loaded-12];dst[loaded-12]=1;
    assert(geo_sd_bios_zip_directory_valid(dst,loaded)<0);
    dst[loaded-12]=saved;

    assert(!geo_sd_bios_load(&v,0,dst,SIZE,&loaded)&&loaded>22);
    assert(geo_sd_bios_load(&v,1,dst,21,&loaded)<0&&loaded==0);
    assert(geo_sd_bios_load(&v,2,dst,SIZE,&loaded)<0&&loaded==0);
    fail=1;assert(geo_sd_bios_load(&v,0,dst,SIZE,&loaded)<0&&loaded==0);
    assert(geo_sd_cart_load(&v,"/roms/neo/Test Cart.NEO",dst,SIZE)<0);
    fclose(f);puts("Neo Geo FAT cartridge fixture passed");return 0;
}
