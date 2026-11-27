#include "native_file.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
struct disk {FILE *fp;int fail;};
static unsigned rd(void *ctx,uint32_t lba,uint32_t n,void *p)
{
    struct disk *d=ctx;
    if(d->fail || fseek(d->fp,(long)lba*512,SEEK_SET))return 0;
    return (unsigned)fread(p,512,n,d->fp);
}
int main(int argc,char **argv)
{
    assert(argc==2);
    const char *path="/roms/neo/Test Cart.NEO";
    assert(geo_native_open(path,O_RDONLY)==-1 && errno==ENODEV);
    struct disk d={fopen(argv[1],"rb"),0};assert(d.fp);
    fat_vol v;assert(!fat_mount(&v,rd,&d));geo_native_file_bind(&v);
    assert(geo_native_open("relative",O_RDONLY)==-1 && errno==EINVAL);
    assert(geo_native_open("/missing",O_RDONLY)==-1 && errno==ENOENT);
    assert(geo_native_open(path,O_RDWR)==-1 && errno==EROFS);
    assert(geo_native_open(path,O_RDONLY|O_CREAT)==-1 && errno==EROFS);
    int fd[8];for(int i=0;i<8;i++){fd[i]=geo_native_open(path,O_RDONLY);assert(fd[i]>=3);}
    assert(geo_native_open(path,O_RDONLY)==-1 && errno==EMFILE);
    unsigned char buf[32];struct stat st;
    assert(!geo_native_fstat(fd[0],&st) && S_ISREG(st.st_mode) && st.st_size>4096);
    assert(geo_native_read(fd[0],buf,4)==4 && !memcmp(buf,"NEO\1",4));
    assert(geo_native_seek(fd[0],4096,SEEK_SET)==4096);
    assert(geo_native_read(fd[0],buf,sizeof buf)==sizeof buf);
    for(unsigned i=0;i<sizeof buf;i++)assert(buf[i]==0x5a);
    assert(geo_native_read(fd[1],buf,4)==4 && !memcmp(buf,"NEO\1",4));
    assert(geo_native_seek(fd[0],-2,SEEK_END)==st.st_size-2);
    assert(geo_native_read(fd[0],buf,sizeof buf)==2);
    assert(geo_native_read(fd[0],buf,sizeof buf)==0);
    assert(geo_native_seek(fd[0],1,SEEK_END)==st.st_size+1);
    assert(geo_native_read(fd[0],buf,sizeof buf)==0);
    assert(geo_native_seek(fd[0],-1,SEEK_SET)==-1 && errno==EINVAL);
    assert(geo_native_seek(fd[0],0,SEEK_CUR)==st.st_size+1);
    assert(geo_native_seek(fd[0],0,SEEK_SET)==0);
    d.fail=1;assert(geo_native_read(fd[0],buf,4)==-1 && errno==EIO);
    d.fail=0;assert(geo_native_read(fd[0],buf,4)==4 && !memcmp(buf,"NEO\1",4));
    for(int i=0;i<8;i++)assert(!geo_native_close(fd[i]));
    assert(geo_native_close(fd[0])==-1 && errno==EBADF);
    int bios=geo_native_open("/bios/neogeo.zip",O_RDONLY);assert(bios>=3);
    assert(geo_native_read(bios,buf,4)==4 && !memcmp(buf,"PK\3\4",4));
    geo_native_file_bind(NULL);assert(geo_native_read(bios,buf,4)==-1 && errno==EBADF);
    fclose(d.fp);puts("Native FAT file adapter passed");return 0;
}
