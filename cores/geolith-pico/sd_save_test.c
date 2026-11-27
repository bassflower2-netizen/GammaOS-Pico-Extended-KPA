#include "sd_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct disk { FILE *fp; unsigned writes; int fail_write; };
static unsigned rd(void *ctx,uint32_t lba,uint32_t n,void *buf)
{
    struct disk *d=ctx;
    if(fseek(d->fp,(long)lba*512,SEEK_SET))return 0;
    return (unsigned)fread(buf,512,n,d->fp);
}
static unsigned wr(void *ctx,uint32_t lba,uint32_t n,const void *buf)
{
    struct disk *d=ctx;
    if(d->fail_write || fseek(d->fp,(long)lba*512,SEEK_SET))return 0;
    d->writes+=n;
    return (unsigned)fwrite(buf,512,n,d->fp);
}
static unsigned char a[222757],b[222757];
int main(int argc,char **argv)
{
    assert(argc==2);
    struct disk d={fopen(argv[1],"r+b"),0,0};assert(d.fp);
    fat_vol v;assert(!fat_mount(&v,rd,&d));
    const uint64_t key=UINT64_C(0x123456789abcdef0),other=key^UINT64_C(0x100000000);
    for(unsigned i=0;i<sizeof a;i++)a[i]=(unsigned char)(i*7u+(i>>9));
    assert(geo_sd_save_put(&v,key,0,-1,a,2048)==-1 && !d.writes);
    fat_set_writer(&v,wr);
    assert(geo_sd_save_get(&v,key,0,-1,b,2048)==-2);
    assert(geo_sd_save_put(&v,key,2,0,a,2048)==-1);
    assert(geo_sd_save_put(&v,key,0,10,a,2048)==-1);
    assert(geo_sd_save_put(&v,key,0,-1,a,0)==-1 && !d.writes);
    assert(!geo_sd_save_put(&v,key,0,-1,a,2048));
    assert(!geo_sd_save_put(&v,key,1,-1,a+1,2048));
    assert(!geo_sd_save_put(&v,other,0,-1,a+2,2048));
    assert(!geo_sd_save_put(&v,key,0,0,a,sizeof a));
    assert(!geo_sd_save_put(&v,key,0,9,a+3,2048));
    assert(!geo_sd_save_put(&v,key,0,-1,a+4,2048));
    assert(!fflush(d.fp));
    /* Re-mount from the actual on-disk FAT, not cached file state. */
    assert(!fat_mount(&v,rd,&d));
    assert(!geo_sd_save_get(&v,key,0,-1,b,2048) && !memcmp(b,a+4,2048));
    assert(!geo_sd_save_get(&v,key,1,-1,b,2048) && !memcmp(b,a+1,2048));
    assert(!geo_sd_save_get(&v,other,0,-1,b,2048) && !memcmp(b,a+2,2048));
    assert(!geo_sd_save_get(&v,key,0,0,b,sizeof b) && !memcmp(b,a,sizeof b));
    assert(!geo_sd_save_get(&v,key,0,9,b,2048) && !memcmp(b,a+3,2048));
    assert(geo_sd_save_get(&v,key,0,1,b,2048)==-2);
    assert(geo_sd_save_get(&v,key,0,0,b,2048)==-2);
    fat_file f;assert(!fat_open(&v,"/roms/neo/Test Cart.NEO",&f));
    assert(fat_read(&f,0,b,4)==4 && !memcmp(b,"NEO\1",4));
    assert(!fat_open(&v,"/saves/neo/MVS/12345678/9ABCDEF0.SAV",&f) && f.size==2048);
    fat_set_writer(&v,wr);d.fail_write=1;
    assert(geo_sd_save_put(&v,key,0,8,a,2048)==-3);
    fclose(d.fp);puts("Neo Geo isolated FAT save/state round-trip passed");return 0;
}
