#include "sd_cart.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "geo_zlib.h"
#define SIZE (4096u+0x100000u+0x10000u+0x10000u+1u+0x100000u)
static unsigned char dst[SIZE+1];
static int fail;
static unsigned rd(void *ctx,uint32_t lba,uint32_t n,void *p)
{
    FILE *f=ctx;
    if(fail||fseek(f,(long)lba*512,SEEK_SET))return 0;
    return (unsigned)fread(p,512,n,f);
}

static int fake_inflate(void *ctx,const unsigned char *src,uint32_t src_len,
                        void *dst,uint32_t capacity,uint32_t *consumed,
                        uint32_t *produced)
{
    (void)ctx;(void)src;(void)dst;
    *consumed=src_len-1;*produced=capacity;
    return 0;
}
static unsigned char bios_payload[0x20000+1];

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
    /* Real FAT -> ZIP directory -> native zlib -> CRC-checked payload. */
    size_t chained=99;
    memset(bios_payload,0xa5,sizeof bios_payload);
    assert(!geo_sd_bios_member_from_fat(&v,1,"neo-po.bin",dst,SIZE,
            bios_payload,sizeof bios_payload,geo_zip_zlib_raw_inflate,NULL,&chained));
    assert(chained==0x20000&&bios_payload[chained]==0xa5);
    assert(geo_sd_bios_member_from_fat(&v,1,"missing.bin",dst,SIZE,
            bios_payload,sizeof bios_payload,geo_zip_zlib_raw_inflate,NULL,&chained)<0
            &&chained==0);
    assert(geo_sd_bios_member_from_fat(&v,1,"neo-po.bin",dst,SIZE,
            bios_payload,0x1ffff,geo_zip_zlib_raw_inflate,NULL,&chained)<0
            &&chained==0);
    assert(geo_sd_bios_member_from_fat(&v,1,"neo-po.bin",dst,SIZE,
            (unsigned char *)dst+4,sizeof bios_payload,geo_zip_zlib_raw_inflate,NULL,&chained)<0
            &&chained==0);
    assert(geo_sd_bios_member_from_fat(&v,1,"neo-po.bin",dst,SIZE,
            bios_payload,sizeof bios_payload,NULL,NULL,&chained)<0&&chained==0);
    assert(!geo_sd_bios_member_from_fat(&v,0,"sp-u2.sp1",dst,SIZE,
            bios_payload,sizeof bios_payload,geo_zip_zlib_raw_inflate,NULL,&chained));
    assert(chained==0x20000);
    /* Restore AES archive so the original ZIP mutation tests still apply. */
    assert(!geo_sd_bios_load(&v,1,dst,SIZE,&loaded));
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

    size_t bios_written=99;
    memset(bios_payload,0xa5,sizeof bios_payload);
    assert(!geo_sd_bios_member_decode(&m,bios_payload,sizeof bios_payload,
                                     geo_zip_zlib_raw_inflate,NULL,&bios_written));
    assert(bios_written==0x20000&&bios_payload[bios_written]==0xa5);
    geo_zip_member invalid=m;
    invalid.crc32^=1u;
    assert(geo_sd_bios_member_decode(&invalid,bios_payload,sizeof bios_payload,
                                    geo_zip_zlib_raw_inflate,NULL,&bios_written)<0&&bios_written==0);
    assert(geo_sd_bios_member_decode(&m,bios_payload,0x1ffffu,
                                    geo_zip_zlib_raw_inflate,NULL,&bios_written)<0&&bios_written==0);
    assert(geo_sd_bios_member_decode(&m,bios_payload,sizeof bios_payload,
                                    fake_inflate,NULL,&bios_written)<0&&bios_written==0);
    invalid=m;invalid.compressed_size--;
    assert(geo_sd_bios_member_decode(&invalid,bios_payload,sizeof bios_payload,
                                    geo_zip_zlib_raw_inflate,NULL,&bios_written)<0&&bios_written==0);
    assert(geo_sd_bios_member_decode(&m,bios_payload,sizeof bios_payload,
                                    NULL,NULL,&bios_written)<0&&bios_written==0);
    uint32_t consumed=111,produced=222;
    assert(geo_zip_zlib_raw_inflate(NULL,NULL,5,bios_payload,16,
                                    &consumed,&produced)<0);
    assert(consumed==0&&produced==0);
    assert(geo_zip_zlib_raw_inflate(NULL,m.data,m.compressed_size,
                                    bios_payload,0,&consumed,&produced)<0);
    assert(consumed==0&&produced==0);

    /* Deflate bytes are not safe to pass to the engine as uncompressed BIOS. */
    if(m.method==8){
        size_t n=99;
        assert(geo_sd_bios_member_copy_stored(&m,dst,SIZE,&n)<0&&n==0);
    }
    /* Independent original STORED-member fixture with known CRC32("hello"). */
    static const unsigned char hello[]={'h','e','l','l','o'};
    unsigned char decoded[6]={0};
    size_t nbytes=99;
    geo_zip_member stored={hello,5,5,0x3610a686u,0};
    assert(!geo_sd_bios_member_copy_stored(&stored,decoded,5,&nbytes));
    assert(nbytes==5&&decoded[0]=='h'&&decoded[4]=='o'&&decoded[5]==0);
    stored.crc32^=1u;
    assert(geo_sd_bios_member_copy_stored(&stored,decoded,5,&nbytes)<0&&nbytes==0);
    stored.crc32^=1u;
    assert(geo_sd_bios_member_copy_stored(&stored,decoded,4,&nbytes)<0&&nbytes==0);
    stored.method=8;
    assert(geo_sd_bios_member_copy_stored(&stored,decoded,5,&nbytes)<0&&nbytes==0);
    stored.method=0;stored.uncompressed_size=6;
    assert(geo_sd_bios_member_copy_stored(&stored,decoded,6,&nbytes)<0&&nbytes==0);
    assert(geo_sd_bios_member_copy_stored(NULL,decoded,6,&nbytes)<0&&nbytes==0);
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
