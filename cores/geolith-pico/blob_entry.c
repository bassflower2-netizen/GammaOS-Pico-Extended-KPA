/* Flat overlay boundary. Actual FAT mutations go to the firmware writer,
 * retaining its shared lock rather than duplicating LK synchronization. */
#include "geo_blob_abi.h"
#include "device_session.h"
#include "../../emu/gba/fat_wr.h"
#include <string.h>
static struct geo_blob_host host;
static struct geo_blob_av av;
static int ready,opened;
extern void __libc_init_array(void);
void _init(void){}
static ssize_t log_bridge(void *ctx,int fd,const void *p,size_t n)
{return host.log(ctx,fd,p,(uint32_t)n);}
static int clock_bridge(void *ctx,int64_t *s,int32_t *us)
{return host.clock(ctx,s,us);}
static void fatal_bridge(void *ctx,int status){host.fatal(ctx,status);}
int fat_wr_put(fat_vol *v,const char *dir,const char *name,const void *p,uint32_t n)
{return ready?host.put(v,dir,name,p,n):-1;}
int fat_wr_mkpath(fat_vol *v,const char *path)
{return ready?host.mkpath(v,path):-1;}
static int32_t init(void *heap,uint32_t n,const struct geo_blob_host *h)
{
    if(ready||!h||h->size!=sizeof *h||!h->log||!h->fatal||!h->put||!h->mkpath)return -1;
    host=*h;
    if(geo_device_init(heap,n,log_bridge,h->clock?clock_bridge:NULL,fatal_bridge,h->context)){
        memset(&host,0,sizeof host);return -1;
    }
    ready=1;__libc_init_array();return 0;
}
static void video_bridge(const void *p,unsigned w,unsigned h,size_t pitch)
{av.video(p,w,h,(uint32_t)pitch);}
static size_t audio_bridge(const int16_t *p,size_t n)
{return av.audio(p,(uint32_t)n);}
static void av_log(const char *p){if(av.log)av.log(p);}
static int32_t open(void *v,const char *path,uint32_t aes,void *scratch,uint32_t n,
                    const struct geo_blob_av *callbacks)
{
    if(!ready||opened||!callbacks||!callbacks->video||!callbacks->audio)return -1;
    av=*callbacks;
    const geo_pico_callbacks cb={video_bridge,audio_bridge,av_log};
    int rc=geo_device_open(v,path,aes,scratch,n,&cb);
    if(rc){memset(&av,0,sizeof av);return rc;}
    opened=1;return 0;
}
static void close(void)
{
    if(!opened)return;
    geo_device_close();opened=0;memset(&av,0,sizeof av);
}
static int32_t frame(uint16_t a,uint16_t b){return geo_device_frame(a,b);}
static int32_t reset(void){return geo_device_reset();}
static uint32_t state_size(void){return (uint32_t)geo_device_state_size();}
static int32_t save(int32_t slot,void *p,uint32_t n){return geo_device_save(slot,p,n);}
static int32_t load(int32_t slot,void *p,uint32_t n){return geo_device_load(slot,p,n);}
static int32_t timing(uint32_t *fps,uint32_t *rate){return geo_pico_timing(fps,rate);}
static const struct geo_blob_exports api={
    GEO_BLOB_MAGIC,GEO_BLOB_VERSION,sizeof api,init,open,close,frame,reset,state_size,save,load,timing
};
const struct geo_blob_exports *geo_blob_entry(void){return &api;}
