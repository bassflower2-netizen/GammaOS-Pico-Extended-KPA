/* Portable frontend for the pinned Geolith libretro engine. */
#include "geo_adapter.h"
#include "libretro.h"
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
static geo_pico_callbacks cb;
static char path_buf[512],system_buf[512],save_buf[512];
static uint16_t pads[2];
static bool active,initialized,bad;
static void logline(enum retro_log_level level,const char *fmt,...)
{
    (void)level;
    if(!cb.log)return;
    char msg[512];va_list args;va_start(args,fmt);
    vsnprintf(msg,sizeof msg,fmt,args);va_end(args);
    cb.log(msg);
}
static bool env(unsigned cmd,void *data)
{
    switch(cmd){
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        ((struct retro_log_callback *)data)->log=logline;return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        *(const char **)data=system_buf;return true;
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data=save_buf;return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return *(enum retro_pixel_format *)data==RETRO_PIXEL_FORMAT_XRGB8888;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *(bool *)data=false;return true;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:return true;
    default:return false;
    }
}
static void video(const void *p,unsigned w,unsigned h,size_t pitch)
{
    if(!w||!h||w>320||h>264||(p&&pitch<w*4u)){bad=true;return;}
    cb.video(p,w,h,pitch);
}
static size_t audio(const int16_t *p,size_t n)
{
    if(n&&!p){bad=true;return 0;}
    size_t accepted=cb.audio(p,n);
    if(accepted>n){bad=true;return 0;}
    return accepted;
}
static void sample(int16_t l,int16_t r){int16_t a[]={l,r};(void)audio(a,1);}
static void poll(void){}
static int16_t input(unsigned port,unsigned dev,unsigned index,unsigned id)
{
    if(port>=2||index||dev!=RETRO_DEVICE_JOYPAD)return 0;
    if(id==RETRO_DEVICE_ID_JOYPAD_MASK)return (int16_t)pads[port];
    return id<16 && ((pads[port]>>id)&1u);
}
static int copy_path(char dst[512],const char *src)
{
    if(!src||!*src||strlen(src)>=512)return -1;
    strcpy(dst,src);return 0;
}
void geo_pico_close(void)
{
    if(active)retro_unload_game();
    if(initialized)retro_deinit();
    active=initialized=bad=false;
    pads[0]=pads[1]=0;memset(&cb,0,sizeof cb);
}
int geo_pico_open(const char *path,const char *system_dir,const char *save_dir,
                  const geo_pico_callbacks *out)
{
    if(initialized||!out||!out->video||!out->audio||
       copy_path(path_buf,path)||copy_path(system_buf,system_dir)||copy_path(save_buf,save_dir))return -1;
    cb=*out;bad=false;pads[0]=pads[1]=0;
    retro_set_environment(env);retro_set_video_refresh(video);
    retro_set_audio_sample(sample);retro_set_audio_sample_batch(audio);
    retro_set_input_poll(poll);retro_set_input_state(input);
    retro_init();initialized=true;
    struct retro_system_info si={0};retro_get_system_info(&si);
    if(!si.need_fullpath||!si.valid_extensions||!strstr(si.valid_extensions,"neo")){
        geo_pico_close();return -2;
    }
    struct retro_game_info game={path_buf,NULL,0,NULL};
    if(!retro_load_game(&game)){geo_pico_close();return -2;}
    active=true;
    if(bad){geo_pico_close();return -3;}
    return 0;
}
int geo_pico_frame(uint16_t p1,uint16_t p2)
{
    if(!active)return -1;
    pads[0]=p1;pads[1]=p2;retro_run();return bad?-2:0;
}
int geo_pico_reset(void){if(!active)return -1;retro_reset();return 0;}
size_t geo_pico_state_size(void){return active?retro_serialize_size():0;}
int geo_pico_save(void *p,size_t n)
{return active&&p&&n&&n==geo_pico_state_size()&&retro_serialize(p,n)?0:-1;}
int geo_pico_load(const void *p,size_t n)
{return active&&p&&n&&n==geo_pico_state_size()&&retro_unserialize(p,n)?0:-1;}
