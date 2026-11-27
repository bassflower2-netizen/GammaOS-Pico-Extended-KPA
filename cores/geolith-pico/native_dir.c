/* Private read-only directory adapter; bind only for a single core session. */
#include "native_include/dirent.h"
#include "../../emu/gba/fat_ro.h"
#include <errno.h>
#include <string.h>
#define GEO_DIR_COUNT 8
struct geo_native_dir { fat_dir cursor; struct dirent entry; int used; };
static struct geo_native_dir dirs[GEO_DIR_COUNT];
static fat_vol *volume;
void geo_native_dir_bind(fat_vol *v)
{
    memset(dirs,0,sizeof dirs);
    volume=v;
}
DIR *opendir(const char *path)
{
    if(!volume || !volume->mounted){errno=ENODEV;return 0;}
    if(!path || path[0]!='/'){errno=EINVAL;return 0;}
    for(unsigned i=0;i<GEO_DIR_COUNT;i++)if(!dirs[i].used){
        if(fat_opendir(volume,path,&dirs[i].cursor)){errno=ENOENT;return 0;}
        dirs[i].used=1;return &dirs[i];
    }
    errno=EMFILE;return 0;
}
static DIR *valid(DIR *d)
{
    for(unsigned i=0;i<GEO_DIR_COUNT;i++)if(d==&dirs[i] && dirs[i].used)return d;
    errno=EBADF;return 0;
}
struct dirent *readdir(DIR *d)
{
    if(!valid(d))return 0;
    fat_dirent e;
    int r=fat_readdir(&d->cursor,&e);
    if(r<=0){if(r<0)errno=EIO;return 0;}
    memcpy(d->entry.d_name,e.name,sizeof e.name);
    d->entry.d_name[sizeof d->entry.d_name-1]=0;
    d->entry.d_type=e.is_dir?DT_DIR:DT_REG;
    return &d->entry;
}
int closedir(DIR *d)
{
    if(!valid(d))return -1;
    d->used=0;return 0;
}
