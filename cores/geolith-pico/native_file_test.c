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
struct log_capture {char bytes[16];size_t used;int fd;int result;};
static ssize_t capture(void *ctx,int fd,const void *buf,size_t count)
{
    struct log_capture *l=ctx;l->fd=fd;
    if(l->result<0)return -1;
    if(l->result==2)return (ssize_t)count+1;
    size_t n=l->result==1?count/2:count;
    assert(n<=sizeof l->bytes-l->used);
    memcpy(l->bytes+l->used,buf,n);l->used+=n;return (ssize_t)n;
}
int main(int argc,char **argv)
{
    assert(argc==2);
    const char *path="/roms/neo/Test Cart.NEO";
    struct stat meta;
    struct log_capture log={{0},0,0,0};
    assert(geo_native_write(1,"x",1)==-1 && errno==ENODEV);
    geo_native_console_bind(capture,&log);
    assert(!geo_native_fstat(1,&meta) && S_ISCHR(meta.st_mode));
    assert(geo_native_isatty(2)==0 && errno==ENOTTY);
    assert(geo_native_write(1,"abc",3)==3 && log.fd==1);
    assert(geo_native_write(2,"def",3)==3 && log.fd==2);
    log.result=1;assert(geo_native_write(2,"ghij",4)==2);
    assert(log.used==8 && !memcmp(log.bytes,"abcdefgh",8));
    log.result=-1;assert(geo_native_write(2,"x",1)==-1 && errno==EIO);
    log.result=2;assert(geo_native_write(2,"x",1)==-1 && errno==EIO);
    assert(geo_native_write(2,NULL,1)==-1 && errno==EFAULT);
    assert(geo_native_write(2,NULL,0)==0);
    assert(geo_native_write(2,"x",(size_t)INT32_MAX+1u)==-1 && errno==EOVERFLOW);
    assert(geo_native_write(99,"x",1)==-1 && errno==EBADF);
    geo_native_console_bind(NULL,NULL);
    assert(geo_native_write(2,"x",1)==-1 && errno==ENODEV);
    assert(geo_native_mutation_denied()==-1 && errno==EROFS);
    assert(geo_native_stat(path,&meta)==-1 && errno==ENODEV);
    assert(geo_native_open(path,O_RDONLY)==-1 && errno==ENODEV);
    struct disk d={fopen(argv[1],"rb"),0};assert(d.fp);
    fat_vol v;assert(!fat_mount(&v,rd,&d));geo_native_file_bind(&v);
    assert(geo_native_open("relative",O_RDONLY)==-1 && errno==EINVAL);
    assert(geo_native_open("/missing",O_RDONLY)==-1 && errno==ENOENT);
    assert(geo_native_open(path,O_RDWR)==-1 && errno==EROFS);
    assert(geo_native_open(path,O_RDONLY|O_CREAT)==-1 && errno==EROFS);
    int fd[8];for(int i=0;i<8;i++){fd[i]=geo_native_open(path,O_RDONLY);assert(fd[i]>=3);}
    assert(geo_native_open(path,O_RDONLY)==-1 && errno==EMFILE);
    assert(geo_native_write(fd[0],"x",1)==-1 && errno==EROFS);
    assert(geo_native_truncate(fd[0],0)==-1 && errno==EROFS);
    assert(geo_native_truncate(fd[0],-1)==-1 && errno==EINVAL);
    assert(geo_native_isatty(fd[0])==0 && errno==ENOTTY);
    unsigned char buf[32];struct stat st;
    assert(!geo_native_fstat(fd[0],&st) && S_ISREG(st.st_mode) && st.st_size>4096);
    /* Path lookup does not consume descriptors, even with a full table. */
    assert(!geo_native_stat("/roms/NEO/test cart.neo",&meta));
    assert(meta.st_mode==st.st_mode && meta.st_size==st.st_size && meta.st_nlink==1);
    assert(!geo_native_stat("/",&meta) && S_ISDIR(meta.st_mode));
    assert(!geo_native_stat("/roms/neo/",&meta) && S_ISDIR(meta.st_mode));
    assert(!geo_native_stat("/bios/neogeo.zip",&meta) && S_ISREG(meta.st_mode) && meta.st_size>0);
    assert(geo_native_stat("/missing",&meta)==-1 && errno==ENOENT && S_ISREG(meta.st_mode));
    assert(geo_native_stat("relative",&meta)==-1 && errno==EINVAL);
    assert(geo_native_stat(path,NULL)==-1 && errno==EFAULT);
    char longpath[514];memset(longpath,'x',sizeof longpath);longpath[0]='/';longpath[513]=0;
    assert(geo_native_stat(longpath,&meta)==-1 && errno==ENAMETOOLONG);
    longpath[257]=0;
    assert(geo_native_stat(longpath,&meta)==-1 && errno==ENAMETOOLONG);
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
