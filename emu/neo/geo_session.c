#include "geo_session.h"
#include <string.h>
static const struct geo_blob_exports *api;
static struct geo_session_memory mem;
static struct geo_session_output out;
static uint32_t width,height,fps,rate;
static int busy,active,bad;
static int range(uintptr_t p,uint32_t n)
{return p&&n&&n<=UINTPTR_MAX-p;}
static int apart(uintptr_t a,uint32_t an,uintptr_t b,uint32_t bn)
{return range(a,an)&&range(b,bn)&&(a+an<=b||b+bn<=a);}
static int memory_ok(const struct geo_session_memory *m)
{
    if(!m||m->heap_size<4096u||m->scratch_size<4096u||
       m->video_pixels<GEO_VIDEO_MAX_W*GEO_VIDEO_MAX_H||
       m->video_pixels>UINT32_MAX/2u)return 0;
    uintptr_t a=(uintptr_t)m->heap,b=(uintptr_t)m->scratch,c=(uintptr_t)m->video;
    uint32_t n=m->video_pixels*2u;
    return !(a&15u)&&!(b&3u)&&!(c&1u)&&
        apart(a,m->heap_size,b,m->scratch_size)&&apart(a,m->heap_size,c,n)&&
        apart(b,m->scratch_size,c,n)&&apart(a,m->heap_size,GEO_BLOB_VMA,GEO_BLOB_SLOT_SIZE)&&
        apart(b,m->scratch_size,GEO_BLOB_VMA,GEO_BLOB_SLOT_SIZE)&&
        apart(c,n,GEO_BLOB_VMA,GEO_BLOB_SLOT_SIZE);
}
static void video(const void *p,uint32_t w,uint32_t h,uint32_t pitch)
{
    int rc=geo_video_rgb565(mem.video,mem.video_pixels,p,w,h,pitch);
    if(rc<0){bad=1;return;}
    if(!rc){width=w;height=h;}
    else if(width&&(w!=width||h!=height)){bad=1;return;}
}
static uint32_t audio(const int16_t *p,uint32_t n)
{
    if((n&&!p)||n>4096u){bad=1;return 0;}
    /* Initialization/teardown audio is deliberately not played. */
    if(active&&rate&&n)out.audio(p,n,rate);
    return n;
}
static void log_line(const char *s){if(out.log)out.log(s);}
static const struct geo_blob_av av={video,audio,log_line};
static void clear(void)
{
    api=0;active=busy=bad=0;width=height=fps=rate=0;
    memset(&mem,0,sizeof mem);memset(&out,0,sizeof out);
}
int geo_session_begin(fat_vol *v,const char *path,uint32_t mode,
                      const struct geo_session_memory *m,
                      const struct geo_blob_host *host,
                      const struct geo_session_output *o)
{
    if(busy||!v||!v->mounted||!path||mode>1||!memory_ok(m)||!host||
       host->size!=sizeof *host||!host->log||!host->fatal||!host->put||!host->mkpath||
       !o||!o->present||!o->audio)return -1;
    busy=1;mem=*m;out=*o;
    int error;
    api=geo_core_load(v,&error);
    if(!api){clear();return -2;}
    if(api->init(mem.heap,mem.heap_size,host)){clear();return -3;}
    if(api->open(v,path,mode,mem.scratch,mem.scratch_size,&av)){clear();return -4;}
    /* Existing GammaOS audio submit supports 8-96 kHz. Do not silently
     * substitute a different rate. */
    if(bad||api->timing(&fps,&rate)||rate<8000u||rate>96000u||
       !fps||!api->state_size()||api->state_size()>mem.scratch_size){
        api->close();clear();return -5;
    }
    active=1;return 0;
}
int geo_session_frame(uint16_t a,uint16_t b)
{
    if(!active||bad)return -1;
    if(api->frame(a,b)||bad)return -2;
    if(width&&height)out.present(mem.video,width,height);
    return 0;
}
int geo_session_reset(void){return active&&!bad?api->reset():-1;}
int geo_session_save(int32_t slot)
{return active&&!bad&&slot>=0&&slot<10?api->save(slot,mem.scratch,mem.scratch_size):-1;}
int geo_session_load(int32_t slot)
{return active&&!bad&&slot>=0&&slot<10?api->load(slot,mem.scratch,mem.scratch_size):-1;}
int geo_session_timing(uint32_t *f,uint32_t *r)
{
    if(!active||!f||!r)return -1;
    *f=fps;*r=rate;return 0;
}
void geo_session_end(void)
{
    if(!active)return;
    active=0;api->close();clear();
}
