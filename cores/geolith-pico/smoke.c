/* No commercial ROM or BIOS data. This is a libretro host/ARM probe only. */
#include "neo_format.h"
#include "libretro.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *dir;
static unsigned videos,frames;
static void logline(enum retro_log_level level,const char *fmt,...){(void)level;(void)fmt;}
static bool env(unsigned cmd,void *data)
{
    switch(cmd){
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        ((struct retro_log_callback *)data)->log=logline;return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data=dir;return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return *(enum retro_pixel_format *)data==RETRO_PIXEL_FORMAT_XRGB8888;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *(bool *)data=false;return true;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:return true;
    default:return false;
    }
}
static void video(const void *p,unsigned w,unsigned h,size_t pitch)
{assert(w&&h&&w<=320&&h<=264);if(p)assert(pitch>=w*4u);++videos;}
static size_t audio(const int16_t *p,size_t n)
{if(n)assert(p);frames+=(unsigned)n;return n;}
static void sample(int16_t l,int16_t r){int16_t a[]={l,r};audio(a,1);}
static void poll(void){}
static int16_t input(unsigned p,unsigned d,unsigned i,unsigned id)
{(void)p;(void)d;(void)i;(void)id;return 0;}
int main(int argc,char **argv)
{
    assert(argc==2);dir=argv[1];
    char path[512];
    assert(snprintf(path,sizeof path,"%s/test.neo",dir)>0);
    FILE *fp=fopen(path,"rb");assert(fp);
    assert(fseek(fp,0,SEEK_END)==0);
    long size=ftell(fp);assert(size>0&&size<PICO_NEO_MAX);
    rewind(fp);uint8_t *rom=malloc((size_t)size);assert(rom);
    assert(fread(rom,1,size,fp)==(size_t)size);fclose(fp);
    assert(!pico_neo_validate(rom,size));
    assert(pico_neo_validate(rom,3)<0);
    assert(pico_neo_validate(rom,size-1)<0);
    assert(pico_neo_validate(rom,size+1)<0);
    uint8_t old[4];memcpy(old,rom+4,4);memset(rom+4,255,4);
    assert(pico_neo_validate(rom,size)<0);memcpy(rom+4,old,4);
    free(rom);
    retro_set_environment(env);
    retro_set_video_refresh(video);
    retro_set_audio_sample(sample);
    retro_set_audio_sample_batch(audio);
    retro_set_input_poll(poll);
    retro_set_input_state(input);
    retro_init();
    struct retro_system_info si={0};
    retro_get_system_info(&si);assert(si.need_fullpath&&strstr(si.valid_extensions,"neo"));
    struct retro_game_info game={path,NULL,0,NULL};
    assert(retro_load_game(&game));
    for(unsigned i=0;i<8;i++)retro_run();
    assert(videos>=8&&frames>0);
    size_t n=retro_serialize_size();assert(n>65536&&n<16u*1024u*1024u);
    void *state=malloc(n);assert(state);
    assert(retro_serialize(state,n));
    retro_run();assert(retro_unserialize(state,n));
    retro_run();free(state);
    retro_unload_game();retro_deinit();
    printf("Geolith synthetic cartridge: %u video callbacks, %u stereo frames, %lu byte state\n",videos,frames,(unsigned long)n);
    return 0;
}
