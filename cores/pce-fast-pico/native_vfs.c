/* Bounded, read-only CD stream bridge. No OS calls inside the native core. */
#include "pce_cd_io.h"
#include <streams/file_stream.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HANDLES 104
#define MAX_DISC_FILE (1024u*1024u*1024u)
#define MAX_WHOLE_FILE (512u*1024u)
#define CACHE_SIZE 4096u
struct RFILE { uint32_t size,pos; int id,used; };
static struct RFILE handles[MAX_HANDLES];
static struct pce_cd_io io;
static int enabled,failed;
static RFILE *cache_owner;
static uint32_t cache_at,cache_len;
static unsigned char cache[CACHE_SIZE];

static int allowed(const char *path)
{
    static const char prefix[]="/pico/disc/";
    const char *p,*ext;
    if(!path)return 0;
    if(!strcmp(path,"/pico/bios/syscard3.pce"))return 1;
    if(strncmp(path,prefix,sizeof prefix-1))return 0;
    p=path+sizeof prefix-1;
    if(!*p||strlen(p)>240||!strcmp(p,".")||!strcmp(p,"..")||
       strchr(p,'/')||strchr(p,'\\')||strchr(p,':'))return 0;
    for(const unsigned char *s=(const unsigned char *)p;*s;s++)if(*s<32)return 0;
    ext=strrchr(p,'.');
    if(!ext)return 0;
    return !strcasecmp(ext,".cue")||!strcasecmp(ext,".bin")||
           !strcasecmp(ext,".iso")||!strcasecmp(ext,".wav")||
           !strcasecmp(ext,".ogg")||!strcasecmp(ext,".sbi");
}
void pce_vfs_end(void)
{
    for(unsigned i=0;i<MAX_HANDLES;i++)if(handles[i].used){
        io.close(handles[i].id);handles[i].used=0;
    }
    memset(&io,0,sizeof io);enabled=0;cache_owner=NULL;cache_len=0;
}
int pce_vfs_begin(const struct pce_cd_io *cb)
{
    if(enabled||!cb||!cb->open||!cb->read_at||!cb->close)return -1;
    io=*cb;enabled=1;failed=0;cache_owner=NULL;cache_len=0;
    return 0;
}
int pce_vfs_failed(void){return failed;}
void filestream_vfs_init(const struct retro_vfs_interface_info *info){(void)info;}
RFILE *filestream_open(const char *p,unsigned mode,unsigned hints)
{
    (void)hints;
    if(!enabled||mode!=RETRO_VFS_FILE_ACCESS_READ||!allowed(p))return NULL;
    unsigned i;
    for(i=0;i<MAX_HANDLES&&handles[i].used;i++){}
    if(i==MAX_HANDLES){failed=1;return NULL;}
    uint32_t n=0;
    int id=io.open(p,&n);
    if(id<0)return NULL; /* optional .sbi commonly absent */
    if(!n||n>MAX_DISC_FILE){io.close(id);return NULL;}
    handles[i]=(struct RFILE){n,0,id,1};
    return &handles[i];
}
static int live(RFILE *s)
{
    uintptr_t p=(uintptr_t)s,base=(uintptr_t)handles;
    return p>=base&&p<base+sizeof handles&&(p-base)%sizeof *s==0&&s->used;
}
int64_t filestream_get_size(RFILE *s){return live(s)?(int64_t)s->size:-1;}
int64_t filestream_tell(RFILE *s){return live(s)?(int64_t)s->pos:-1;}
int64_t filestream_seek(RFILE *s,int64_t n,int w)
{
    if(!live(s))return -1;
    int64_t base=w==RETRO_VFS_SEEK_POSITION_START?0:
        w==RETRO_VFS_SEEK_POSITION_CURRENT?s->pos:
        w==RETRO_VFS_SEEK_POSITION_END?s->size:-1;
    if(base<0||n< -base||n>(int64_t)MAX_DISC_FILE-base)return -1;
    s->pos=(uint32_t)(base+n);return s->pos;
}
int64_t filestream_read(RFILE *s,void *out,int64_t n)
{
    if(!live(s)||n<0||(!out&&n))return -1;
    if(s->pos>=s->size||!n)return 0;
    uint32_t avail=s->size-s->pos;
    uint32_t take=(uint64_t)n>avail?avail:(uint32_t)n,done=0;
    unsigned char *p=out;
    while(done<take){
        if(cache_owner!=s||s->pos<cache_at||s->pos-cache_at>=cache_len){
            cache_owner=NULL;cache_at=s->pos&~(CACHE_SIZE-1u);
            cache_len=s->size-cache_at;
            if(cache_len>CACHE_SIZE)cache_len=CACHE_SIZE;
            int got=io.read_at(s->id,cache_at,cache,cache_len);
            if(got!=(int)cache_len){failed=1;cache_len=0;return -1;}
            cache_owner=s;
        }
        uint32_t off=s->pos-cache_at,k=cache_len-off;
        if(k>take-done)k=take-done;
        memcpy(p+done,cache+off,k);s->pos+=k;done+=k;
    }
    return done;
}
int filestream_close(RFILE *s)
{
    if(!live(s))return -1;
    if(cache_owner==s){cache_owner=NULL;cache_len=0;}
    io.close(s->id);s->used=0;return 0;
}
char *filestream_gets(RFILE *s,char *p,size_t n)
{
    if(!p||n<2)return NULL;
    size_t k=0;unsigned char c;
    while(k<n-1&&filestream_read(s,&c,1)==1){p[k++]=(char)c;if(c=='\n')break;}
    p[k]=0;return k?p:NULL;
}
int64_t filestream_read_file(const char *path,void **out,int64_t *len)
{
    if(len)*len=0;
    if(!out)return 0;
    *out=NULL;
    RFILE *s=filestream_open(path,RETRO_VFS_FILE_ACCESS_READ,0);
    if(!s)return 0;
    uint32_t n=s->size;
    if(n>MAX_WHOLE_FILE){filestream_close(s);return 0;}
    unsigned char *p=malloc((size_t)n+1);
    if(!p){filestream_close(s);return 0;}
    int64_t got=filestream_read(s,p,n);
    filestream_close(s);
    if(got!=n){free(p);return 0;}
    p[n]=0;*out=p;if(len)*len=n;return 1;
}
int pce_vfs_bios_present(void)
{
    RFILE *s=filestream_open("/pico/bios/syscard3.pce",RETRO_VFS_FILE_ACCESS_READ,0);
    if(!s)return 0;
    int ok=s->size==262144u||s->size==262656u;
    filestream_close(s);return ok;
}
