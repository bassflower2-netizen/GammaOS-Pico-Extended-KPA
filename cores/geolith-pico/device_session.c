#include "device_session.h"
#include "native_heap.h"
#include "native_dir_bind.h"
#include "neo_format.h"
#include "sd_save.h"
#include <string.h>
static int initialized;
static fat_vol *mounted;
static uint64_t cart_key;
static unsigned save_aes;
int geo_device_init(void *heap,size_t capacity,geo_native_log_sink log,
                    geo_clock_fn clock,geo_fatal_fn fatal,void *ctx)
{
    if(initialized||!log||!fatal||geo_heap_bind(heap,capacity))return -1;
    geo_native_console_bind(log,ctx);
    geo_native_platform_bind(clock,fatal,ctx);
    initialized=1;return 0;
}
static int valid_path(const char *path)
{
    static const char prefix[]="/roms/neo/";
    if(!path||strncmp(path,prefix,sizeof prefix-1))return 0;
    const char *name=path+sizeof prefix-1;size_t n=strlen(name);
    if(n<5||n>240||strchr(name,'/')||strchr(name,'\\')||strchr(name,':'))return 0;
    return name[n-4]=='.'&&(name[n-3]=='n'||name[n-3]=='N')&&
           (name[n-2]=='e'||name[n-2]=='E')&&(name[n-1]=='o'||name[n-1]=='O');
}
static int identity(fat_vol *v,const char *path,void *scratch,size_t capacity,uint64_t *key)
{
    fat_file f;unsigned char *p=scratch;
    if(fat_open(v,path,&f)||f.size<PICO_NEO_HEADER||f.size>PICO_NEO_MAX)return -1;
    uint32_t chunk=capacity>32768u?32768u:(uint32_t)capacity;
    uint64_t k=UINT64_C(14695981039346656037);
    for(uint32_t off=0;off<f.size;){
        uint32_t count=f.size-off;if(count>chunk)count=chunk;
        if(fat_read(&f,off,p,count)!=count)return -1;
        if(!off&&pico_neo_validate_header(p,count,f.size))return -1;
        for(uint32_t i=0;i<count;i++){k^=p[i];k*=UINT64_C(1099511628211);}
        off+=count;
    }
    *key=k;return 0;
}
int geo_device_open(fat_vol *v,const char *path,unsigned aes,
                    void *scratch,size_t capacity,const geo_pico_callbacks *cb)
{
    if(!initialized||mounted||!v||!v->mounted||!valid_path(path)||aes>1||
       !scratch||capacity<PICO_NEO_HEADER||!cb||!cb->video||!cb->audio)return -1;
    uint64_t key;
    if(identity(v,path,scratch,capacity,&key))return -2;
    geo_native_file_bind(v);geo_native_dir_bind(v);
    int rc=geo_pico_open(path,"/bios","/saves/neo",cb);
    if(rc){geo_native_dir_bind(NULL);geo_native_file_bind(NULL);return -3;}
    mounted=v;cart_key=key;save_aes=aes;return 0;
}
int geo_device_frame(uint16_t a,uint16_t b)
{return mounted&&mounted->mounted?geo_pico_frame(a,b):-1;}
int geo_device_reset(void){return mounted&&mounted->mounted?geo_pico_reset():-1;}
size_t geo_device_state_size(void){return mounted?geo_pico_state_size():0;}
static size_t state_length(int slot,void *scratch,size_t capacity)
{
    if(!mounted||!mounted->mounted||slot<0||slot>9||!scratch)return 0;
    size_t n=geo_pico_state_size();
    return n&&n<=capacity&&n<=UINT32_MAX?n:0;
}
int geo_device_save(int slot,void *scratch,size_t capacity)
{
    size_t n=state_length(slot,scratch,capacity);
    if(!n||!mounted->wr)return -1;
    if(geo_pico_save(scratch,n))return -2;
    return geo_sd_save_put(mounted,cart_key,save_aes,slot,scratch,(uint32_t)n)?-3:0;
}
int geo_device_load(int slot,void *scratch,size_t capacity)
{
    size_t n=state_length(slot,scratch,capacity);
    if(!n)return -1;
    if(geo_sd_save_get(mounted,cart_key,save_aes,slot,scratch,(uint32_t)n))return -3;
    return geo_pico_load(scratch,n)?-2:0;
}
void geo_device_close(void)
{
    if(!mounted)return;
    geo_pico_close(); /* engine cleanup while its storage is still bound */
    geo_native_dir_bind(NULL);geo_native_file_bind(NULL);
    mounted=NULL;cart_key=0;save_aes=0;
}
