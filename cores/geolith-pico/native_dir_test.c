#include "native_include/dirent.h"
#include "../../emu/gba/fat_ro.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
void geo_native_dir_bind(fat_vol *);
static unsigned rd(void *ctx,uint32_t lba,uint32_t n,void *p)
{
    FILE *fp=ctx;
    if(fseek(fp,(long)lba*512,SEEK_SET))return 0;
    return (unsigned)fread(p,512,n,fp);
}
int main(int argc,char **argv)
{
    assert(argc==2);
    assert(!opendir("/") && errno==ENODEV);
    FILE *fp=fopen(argv[1],"rb");assert(fp);
    fat_vol v;assert(!fat_mount(&v,rd,fp));geo_native_dir_bind(&v);
    assert(!opendir("relative") && errno==EINVAL);
    assert(!opendir("/missing") && errno==ENOENT);
    DIR *d=opendir("/roms");assert(d);
    struct dirent *e=readdir(d);assert(e && e->d_type==DT_DIR && fat_name_eq(e->d_name,"neo"));
    assert(!readdir(d));assert(!closedir(d));
    assert(closedir(d)==-1 && errno==EBADF);
    DIR *handles[8];
    for(unsigned i=0;i<8;i++){handles[i]=opendir("/roms/neo");assert(handles[i]);}
    assert(!opendir("/") && errno==EMFILE);
    for(unsigned i=0;i<8;i++){
        unsigned count=0,found=0;
        while((e=readdir(handles[i]))){assert(e->d_type==DT_REG);count++;found+=fat_name_eq(e->d_name,"Test Cart.NEO");}
        assert(count==2 && found==1);assert(!closedir(handles[i]));
    }
    geo_native_dir_bind(0);assert(!opendir("/") && errno==ENODEV);
    fclose(fp);puts("Native FAT directory adapter passed");return 0;
}
