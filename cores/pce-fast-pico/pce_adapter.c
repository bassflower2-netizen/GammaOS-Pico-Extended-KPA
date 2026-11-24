/* PCE Fast experimental memory-backed frontend. GPL-2.0-only.
 * No original game or BIOS data is included. */
#include "pce_adapter.h"
#include "libretro.h"
#include <stdbool.h>
#include <string.h>

static struct retro_game_info_ext game;
static struct pce_pico_callbacks cb;
static bool active,rgb565,bad;
static uint16_t pad;
static void quiet(enum retro_log_level l,const char *fmt,...)
{ (void)l; (void)fmt; }
static bool env(unsigned cmd,void *data)
{
    switch(cmd) {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        ((struct retro_log_callback *)data)->log=quiet; return true;
    case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT:
        *(const struct retro_game_info_ext **)data=&game; return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data="."; return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        rgb565=*(enum retro_pixel_format *)data==RETRO_PIXEL_FORMAT_RGB565;
        return rgb565;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *(bool *)data=false; return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable *v=data;
        v->value=NULL;
        /* Explicit nonzero turbo delay and disable turbo toggling in probe. */
        if(!strcmp(v->key,"pce_fast_turbo_delay")){v->value="3";return true;}
        if(!strcmp(v->key,"pce_fast_turbo_toggling")){v->value="disabled";return true;}
        return false;
    }
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE:
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
    case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
        return true;
    default:return false;
    }
}
static void video(const void *p,unsigned w,unsigned h,size_t pitch)
{
    if(!rgb565||!w||!h||w>1024||h>512||(p&&(pitch<w*2u||pitch%2u))) {
        bad=true; return;
    }
    cb.video(p,w,h,pitch);
}
static size_t audio(const int16_t *p,size_t n)
{
    if(n&&!p){bad=true;return 0;}
    return cb.audio(p,n);
}
static void sample(int16_t l,int16_t r)
{ int16_t a[2]={l,r}; (void)audio(a,1); }
static void poll(void){}
static int16_t input(unsigned port,unsigned device,unsigned index,unsigned id)
{
    if(port||index||device!=RETRO_DEVICE_JOYPAD)return 0;
    if(id==RETRO_DEVICE_ID_JOYPAD_MASK)return (int16_t)pad;
    return id<16 && ((pad>>id)&1u);
}
static bool valid_rom(size_t n)
{
    /* HuCard data aligned to 8 KiB, with optional 512-byte copier header.
     * Bound to the largest supported mapper (Street Fighter II, 2.5 MiB). */
    size_t payload=n;
    if(n%8192==512)payload-=512;
    return payload>=8192 && payload<=2621440u && payload%8192==0;
}
int pce_pico_open(const void *rom,size_t n,const struct pce_pico_callbacks *out)
{
    if(active||!rom||!out||!out->video||!out->audio||!valid_rom(n))return -1;
    cb=*out; pad=0; bad=false; rgb565=false;
    memset(&game,0,sizeof game);
    game.data=rom; game.size=n; game.ext="pce"; game.name="pico-test";
    game.persistent_data=true;
    retro_set_environment(env);
    retro_set_video_refresh(video);
    retro_set_audio_sample(sample);
    retro_set_audio_sample_batch(audio);
    retro_set_input_poll(poll);
    retro_set_input_state(input);
    retro_init();
    struct retro_game_info info={NULL,rom,n,NULL};
    if(!retro_load_game(&info)){retro_deinit();return -2;}
    active=true;
    if(!rgb565||bad){pce_pico_close();return -3;}
    return 0;
}
void pce_pico_close(void)
{
    if(active){retro_unload_game();retro_deinit();}
    active=false;memset(&game,0,sizeof game);memset(&cb,0,sizeof cb);
}
int pce_pico_frame(uint16_t p)
{ if(!active)return -1;pad=p;retro_run();return bad?-2:0; }
void pce_pico_reset(void){if(active)retro_reset();}
size_t pce_pico_state_size(void){return active?retro_serialize_size():0;}
int pce_pico_save(void *p,size_t n)
{ return active&&p&&n&&n==pce_pico_state_size()&&retro_serialize(p,n)?0:-1; }
int pce_pico_load(const void *p,size_t n)
{ return active&&p&&n&&n==pce_pico_state_size()&&retro_unserialize(p,n)?0:-1; }
void *pce_pico_ram(void){return active?retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM):NULL;}
size_t pce_pico_ram_size(void){return active?retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM):0;}
void *pce_pico_bram(void){return active?retro_get_memory_data(RETRO_MEMORY_SAVE_RAM):NULL;}
size_t pce_pico_bram_size(void){return active?retro_get_memory_size(RETRO_MEMORY_SAVE_RAM):0;}
