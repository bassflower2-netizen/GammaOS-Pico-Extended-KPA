/* Private read-only descriptor table. No host filesystem fallback. */
#include "native_file.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define GEO_FILE_COUNT 8
#define GEO_FD_BASE 3
struct geo_file { fat_file file; uint32_t pos; int used; };
static struct geo_file files[GEO_FILE_COUNT];
static fat_vol *volume;
void geo_native_file_bind(fat_vol *v)
{
    memset(files,0,sizeof files);
    volume=v;
}
static struct geo_file *lookup(int fd)
{
    if(fd<GEO_FD_BASE || fd>=GEO_FD_BASE+GEO_FILE_COUNT ||
       !files[fd-GEO_FD_BASE].used){errno=EBADF;return NULL;}
    if(!volume || !volume->mounted){errno=ENODEV;return NULL;}
    return &files[fd-GEO_FD_BASE];
}
int geo_native_open(const char *path,int flags)
{
    if(!volume || !volume->mounted){errno=ENODEV;return -1;}
    if(!path || path[0]!='/'){errno=EINVAL;return -1;}
    if((flags&O_ACCMODE)!=O_RDONLY || (flags&(O_CREAT|O_TRUNC|O_APPEND|O_EXCL))){
        errno=EROFS;return -1;
    }
    for(int i=0;i<GEO_FILE_COUNT;i++)if(!files[i].used){
        if(fat_open(volume,path,&files[i].file)){errno=ENOENT;return -1;}
        /* Keep offsets representable in target newlib's signed 32-bit off_t. */
        if(files[i].file.size>INT32_MAX){errno=EOVERFLOW;return -1;}
        files[i].pos=0;files[i].used=1;return i+GEO_FD_BASE;
    }
    errno=EMFILE;return -1;
}
int geo_native_close(int fd)
{
    struct geo_file *f=lookup(fd);
    if(!f)return -1;
    f->used=0;return 0;
}
ssize_t geo_native_read(int fd,void *buf,size_t count)
{
    struct geo_file *f=lookup(fd);
    if(!f)return -1;
    if(!count || f->pos>=f->file.size)return 0;
    if(!buf){errno=EFAULT;return -1;}
    uint32_t left=f->file.size-f->pos;
    uint32_t want=count>left?left:(uint32_t)count;
    uint32_t got=fat_read(&f->file,f->pos,buf,want);
    f->pos+=got;
    if(!got){errno=EIO;return -1;}
    return (ssize_t)got;
}
off_t geo_native_seek(int fd,off_t offset,int whence)
{
    struct geo_file *f=lookup(fd);
    if(!f)return -1;
    int64_t base;
    switch(whence){
    case SEEK_SET:base=0;break;
    case SEEK_CUR:base=f->pos;break;
    case SEEK_END:base=f->file.size;break;
    default:errno=EINVAL;return -1;
    }
    if(offset < -base || offset > INT32_MAX-base){errno=EINVAL;return -1;}
    f->pos=(uint32_t)(base+offset);return (off_t)f->pos;
}
int geo_native_fstat(int fd,struct stat *st)
{
    struct geo_file *f=lookup(fd);
    if(!f)return -1;
    if(!st){errno=EFAULT;return -1;}
    memset(st,0,sizeof *st);
    st->st_mode=S_IFREG|0444;st->st_size=(off_t)f->file.size;st->st_nlink=1;
    return 0;
}
#ifdef GEO_NEWLIB_SYSCALLS
int _open(const char *path,int flags,int mode)
{(void)mode;return geo_native_open(path,flags);}
int _close(int fd){return geo_native_close(fd);}
ssize_t _read(int fd,void *buf,size_t count){return geo_native_read(fd,buf,count);}
off_t _lseek(int fd,off_t offset,int whence){return geo_native_seek(fd,offset,whence);}
int _fstat(int fd,struct stat *st){return geo_native_fstat(fd,st);}
#endif
