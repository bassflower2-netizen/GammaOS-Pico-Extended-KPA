/* Contract test: real FAT reader/writer and native binding, controller test
 * double only. Actual Geolith runtime is tested separately by smoke.c. */
#include "device_session.h"
#include "native_include/dirent.h"
#include "cart_io.h"
#include "sd_save.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
static int opened,refuse,frames,loads;
static int fd=-1;
static unsigned char engine_state[2048];
static unsigned char scratch[32768];
static unsigned char heap[4096] __attribute__((aligned(16)));
static unsigned rd(void *ctx,uint32_t lba,uint32_t n,void *p)
{FILE *f=ctx;return fseek(f,(long)lba*512,SEEK_SET)?0:(unsigned)fread(p,512,n,f);}
static unsigned wr(void *ctx,uint32_t lba,uint32_t n,const void *p)
{FILE *f=ctx;return fseek(f,(long)lba*512,SEEK_SET)?0:(unsigned)fwrite(p,512,n,f);}
static ssize_t log_out(void *ctx,int desc,const void *p,size_t n)
{(void)ctx;(void)desc;(void)p;return (ssize_t)n;}
static void fatal(void *ctx,int status){(void)ctx;(void)status;assert(0);}
static void video(const void *p,unsigned w,unsigned h,size_t pitch)
{(void)p;(void)w;(void)h;(void)pitch;}
static size_t audio(const int16_t *p,size_t n){(void)p;return n;}
int geo_pico_open(const char *path,const char *system,const char *save,const geo_pico_callbacks *cb)
{
    assert(!opened&&cb->video&&cb->audio&&!strcmp(system,"/bios")&&!strcmp(save,"/saves/neo"));
    DIR *d=opendir(system);assert(d);assert(!closedir(d));
    int bios=geo_native_open("/bios/aes.zip",O_RDONLY);assert(bios>=3);
    assert(!geo_native_close(bios));
    fd=geo_native_open(path,O_RDONLY);assert(fd>=3);
    if(refuse){assert(!geo_native_close(fd));fd=-1;return -1;}
    opened=1;return 0;
}
void geo_pico_close(void){assert(opened);assert(!geo_native_close(fd));opened=0;}
int geo_pico_frame(uint16_t a,uint16_t b){assert(opened&&a==1&&b==2);frames++;return 0;}
int geo_pico_reset(void){assert(opened);return 0;}
size_t geo_pico_state_size(void){return opened?sizeof engine_state:0;}
int geo_pico_save(void *p,size_t n){assert(opened&&n==sizeof engine_state);memcpy(p,engine_state,n);return 0;}
int geo_pico_load(const void *p,size_t n){assert(opened&&n==sizeof engine_state);memcpy(engine_state,p,n);loads++;return 0;}
int main(int argc,char **argv)
{
    assert(argc==2);
    const char *path="/roms/neo/Test Cart.NEO";
    FILE *disk=fopen(argv[1],"r+b");assert(disk);fat_vol v;assert(!fat_mount(&v,rd,disk));
    fat_set_writer(&v,wr);
    const geo_pico_callbacks cb={video,audio,NULL};
    assert(geo_device_open(&v,path,0,scratch,sizeof scratch,&cb)==-1);
    assert(!geo_device_init(heap,sizeof heap,log_out,NULL,fatal,NULL));
    assert(geo_device_init(heap,sizeof heap,log_out,NULL,fatal,NULL)==-1);
    assert(geo_device_open(&v,"/roms/neo/bad.neo",0,scratch,sizeof scratch,&cb)==-2);
    assert(geo_device_open(&v,path,0,scratch,4095,&cb)==-1);
    refuse=1;assert(geo_device_open(&v,path,0,scratch,sizeof scratch,&cb)==-3);
    assert(geo_native_open(path,O_RDONLY)==-1 && errno==ENODEV);
    assert(!opendir("/") && errno==ENODEV);
    refuse=0;assert(!geo_device_open(&v,path,0,scratch,sizeof scratch,&cb));
    assert(geo_device_open(&v,path,0,scratch,sizeof scratch,&cb)==-1);
    assert(!geo_device_frame(1,2)&&frames==1);
    assert(!geo_device_reset());
    memset(engine_state,0x6a,sizeof engine_state);
    assert(geo_device_save(0,scratch,2047)==-1);
    assert(!geo_device_save(0,scratch,sizeof scratch));
    memset(engine_state,0,sizeof engine_state);
    assert(geo_device_load(1,scratch,sizeof scratch)==-3 && !loads);
    assert(!geo_device_load(0,scratch,sizeof scratch)&&loads==1);
    for(unsigned i=0;i<sizeof engine_state;i++)assert(engine_state[i]==0x6a);
    geo_device_close();geo_device_close();
    assert(geo_device_frame(1,2)==-1 && !geo_device_state_size());
    assert(geo_native_open(path,O_RDONLY)==-1 && errno==ENODEV);
    assert(!opendir("/") && errno==ENODEV);
    assert(!geo_device_open(&v,path,1,scratch,sizeof scratch,&cb));
    assert(geo_device_load(0,scratch,sizeof scratch)==-3 && loads==1);
    geo_device_close();assert(!fclose(disk));
    puts("Neo Geo device session binding and persistence contract passed");
    return 0;
}
