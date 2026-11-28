/* Bridge the tested session API to the real GammaOS hardware services. */
#include "geo_lk_host.h"
#include "../gba/pico_clock.h"
#include "../gba/fat_wr.h"
#include "../gba/card_writer.h"
#include <debug.h>

extern void ayaneo_genesis_show_frame(const unsigned short *,unsigned,unsigned,unsigned);
extern void ayaneo_snes_audio_submit(const short *,unsigned,unsigned);
extern void ayaneo_menu_audio_silence(void);
extern void ayaneo_gbc_audio_pause(int);
extern void ayaneo_snes_rsz_restore(void);
extern unsigned ayaneo_dsi_set_fps_milli(unsigned);
extern unsigned ayaneo_dsi_default_vfp(void);
extern void ayaneo_dsi_set_vfp(unsigned);
extern volatile int g_genesis_aspect,g_genesis_filter;
extern volatile unsigned g_genesis_aspect_x1000;
static int running,old_aspect,old_filter;
static unsigned old_ratio;
static int32_t log_bytes(void *ctx,int32_t fd,const void *p,uint32_t n)
{
    (void)ctx;
    if((fd!=1&&fd!=2)||(!p&&n)||n>0x7fffffffu)return -1;
    /* Release builds retain the existing LK logging policy. */
    const char *s=p;
    for(uint32_t k=0;k<n;k++)dputc(INFO,s[k]);
    (void)s;
    return (int32_t)n;
}
static void log_line(const char *s){if(s)dputs(INFO,s);}
static int32_t wall_clock(void *ctx,int64_t *sec,int32_t *usec)
{
    (void)ctx;
    if(!sec||!usec||!pico_clock_is_set())return -1;
    *sec=pico_clock_utc();*usec=0;return 0;
}
static void fatal(void *ctx,int32_t code)
{
    (void)ctx;
    panic("Neo Geo fatal core exit: %d\n",(int)code);
}
static int32_t put(void *v,const char *dir,const char *name,const void *p,uint32_t n)
{return fat_wr_put(v,dir,name,p,n);}
static int32_t mkpath(void *v,const char *path){return fat_wr_mkpath(v,path);}
static void present(const uint16_t *p,uint32_t w,uint32_t h)
{
    if(running)ayaneo_genesis_show_frame(p,w,h,w);
}
static void audio(const int16_t *p,uint32_t n,uint32_t rate)
{
    if(running)ayaneo_snes_audio_submit(p,n,rate);
}
static const struct geo_blob_host host={sizeof host,0,log_bytes,wall_clock,fatal,put,mkpath};
static const struct geo_session_output output={present,audio,log_line};
int geo_lk_begin(fat_vol *v,const char *path,uint32_t mode,void *arena,uint32_t bytes)
{
    if(running)return -1;
    card_writer_flush();
    if(card_writer_pending())return -1;
    int rc=geo_session_begin_arena(v,path,mode,arena,bytes,&host,&output);
    if(rc)return rc;
    uint32_t fps,rate;
    if(geo_session_timing(&fps,&rate)){geo_session_end();return -5;}
    old_aspect=g_genesis_aspect;old_filter=g_genesis_filter;
    old_ratio=g_genesis_aspect_x1000;
    g_genesis_aspect=1;g_genesis_filter=0;g_genesis_aspect_x1000=1333;
    ayaneo_dsi_set_fps_milli(fps);
    ayaneo_gbc_audio_pause(0);running=1;
    return 0;
}
void geo_lk_end(void)
{
    if(!running)return;
    running=0;ayaneo_menu_audio_silence();
    geo_session_end();
    ayaneo_snes_rsz_restore();
    ayaneo_dsi_set_vfp(ayaneo_dsi_default_vfp());
    g_genesis_aspect=old_aspect;g_genesis_filter=old_filter;
    g_genesis_aspect_x1000=old_ratio;
}
