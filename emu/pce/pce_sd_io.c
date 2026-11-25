/* Read-only bridge for the PCE ABI 2 CD entry. No OS filesystem dependency. */
#include "pce_sd_io.h"
#include <string.h>
#define HANDLE_COUNT 104
#define FILE_MAX (1024u*1024u*1024u)
static fat_vol *volume;
static char directory[384];
static struct { fat_file f; int used; } slots[HANDLE_COUNT];

static int component_ok(const char *p,size_t n)
{
    if(!n || (n==1&&p[0]=='.') || (n==2&&p[0]=='.'&&p[1]=='.'))return 0;
    for(size_t i=0;i<n;i++)
        if((unsigned char)p[i]<32||p[i]=='/'||p[i]=='\\'||p[i]==':')return 0;
    return 1;
}
static int path_ok(const char *p)
{
    if(!p||p[0]!='/'||strlen(p)>=sizeof directory)return 0;
    if(!p[1])return 1;
    for(const char *a=p+1;*a;){
        const char *e=strchr(a,'/');
        size_t n=e?(size_t)(e-a):strlen(a);
        if(!component_ok(a,n))return 0;
        if(!e)return 1;
        if(!e[1])return 0;
        a=e+1;
    }
    return 0;
}
int pce_sd_io_begin(fat_vol *v,const char *dir)
{
    if(volume||!v||!v->mounted||!path_ok(dir))return -1;
    fat_dir d;
    if(fat_opendir(v,dir,&d))return -1;
    memset(slots,0,sizeof slots);
    strcpy(directory,dir);
    volume=v;return 0;
}
void pce_sd_io_end(void)
{
    memset(slots,0,sizeof slots);directory[0]=0;volume=0;
}
static int open_read(const char *path,uint32_t *size)
{
    static const char prefix[]="/pico/disc/";
    char real[640]; unsigned i;
    if(!volume||!path||!size)return -1;
    if(!strcmp(path,"/pico/bios/syscard3.pce"))strcpy(real,"/bios/syscard3.pce");
    else {
        if(strncmp(path,prefix,sizeof prefix-1))return -1;
        const char *name=path+sizeof prefix-1;
        size_t n=strlen(name),d=strlen(directory);
        if(n>240||!component_ok(name,n))return -1;
        const char *e=strrchr(name,'.');
        if(!e||!(fat_name_eq(e,".cue")||fat_name_eq(e,".bin")||
           fat_name_eq(e,".iso")||fat_name_eq(e,".wav")||
           fat_name_eq(e,".ogg")||fat_name_eq(e,".sbi")))return -1;
        if(d+n+2>sizeof real)return -1;
        memcpy(real,directory,d);
        if(real[d-1]!='/')real[d++]='/';
        memcpy(real+d,name,n+1);
    }
    for(i=0;i<HANDLE_COUNT&&slots[i].used;i++){}
    if(i==HANDLE_COUNT)return -1;
    if(fat_open(volume,real,&slots[i].f))return -1;
    uint32_t n=slots[i].f.size;
    if(!n||n>FILE_MAX)return -1;
    slots[i].used=1;*size=n;return (int)i;
}
static int read_at(int id,uint32_t at,void *dst,uint32_t n)
{
    if(!volume||id<0||id>=HANDLE_COUNT||!slots[id].used||(!dst&&n))return -1;
    fat_file *f=&slots[id].f;
    if(at>f->size||n>f->size-at)return -1;
    if(!n)return 0;
    return fat_read(f,at,dst,n)==n?(int)n:-1;
}
static void close_read(int id)
{
    if(id>=0&&id<HANDLE_COUNT)slots[id].used=0;
}
const struct pce_cd_io pce_sd_io={open_read,read_at,close_read};
