/* Experimental GammaOS Pico adapter; upstream Handy remains unmodified. */
#include "handy_adapter.h"
#include "libretro.h"
#include <stdbool.h>
#include <string.h>
#ifdef HANDY_NATIVE_MEMORY_FS
#include "native_vfs.h"
extern void handy_native_flush_eeprom(void);
extern int handy_native_bios_valid(void);
#endif

static struct handy_pico_callbacks output;
static struct retro_game_info_ext game;
static uint16_t pad;
static bool active;
static bool rgb565;
static bool callback_error;

static void silent_log(enum retro_log_level level, const char *fmt, ...)
{ (void)level; (void)fmt; }

static bool environment(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        ((struct retro_log_callback *)data)->log = silent_log;
        return true;
    case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT:
        *(const struct retro_game_info_ext **)data = &game;
        return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        rgb565 = *(enum retro_pixel_format *)data == RETRO_PIXEL_FORMAT_RGB565;
        return rgb565;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *(bool *)data = false;
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable *v = data;
        v->value = NULL;
        if (!strcmp(v->key, "handy_refresh_rate")) v->value = "75";
        return v->value != NULL;
    }
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
#ifdef HANDY_NATIVE_MEMORY_FS
        *(const char **)data = "/pico";
        return true;
#else
        *(const char **)data = NULL;
        return false;
#endif
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
        return true;
    default: return false;
    }
}
static void video(const void *data, unsigned w, unsigned h, size_t pitch)
{
    if (!rgb565 || !w || !h || w > 160 || h > 160 ||
        (data && pitch < w * sizeof(uint16_t))) {
        callback_error = true;
        return;
    }
    if (output.video) output.video(data, w, h, pitch);
}
static size_t audio_batch(const int16_t *data, size_t frames)
{
    return output.audio ? output.audio(data, frames) : frames;
}
static void audio_sample(int16_t left, int16_t right)
{
    int16_t pair[2] = {left, right};
    (void)audio_batch(pair, 1);
}
static void poll(void) {}
static int16_t input(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (port || index || device != RETRO_DEVICE_JOYPAD) return 0;
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return (int16_t)pad;
    return id < 16 && ((pad >> id) & 1u);
}
/* Reject obviously invalid or unsafe sizes before upstream parses the header.
 * Raw cartridge support and the full LNX bank-header validation are deferred. */
static bool valid_homebrew(const uint8_t *rom, size_t size)
{
    if (size < 11 || size > 65536 || memcmp(rom+6, "BS93", 4)) return false;
    unsigned addr = ((unsigned)rom[2] << 8) | rom[3];
    unsigned length = ((unsigned)rom[4] << 8) | rom[5];
    return addr >= 10 && length == size && addr-10+length <= 65536;
}
#ifdef HANDY_NATIVE_MEMORY_FS
/* Strict headered cartridge subset: complete banks, known page sizes, no
 * legacy headerless Howard loader, terminated name fields, supported EEPROM. */
static bool valid_cartridge(const uint8_t *p, size_t n)
{
    if(n<64 || memcmp(p,"LYNX",4) || p[8]!=1 || p[9]!=0) return false;
    unsigned b0=p[4]|((unsigned)p[5]<<8), b1=p[6]|((unsigned)p[7]<<8);
    if(!(b0==256 || b0==512 || b0==1024 || b0==2048) ||
       !(b1==0 || b1==256 || b1==512 || b1==1024 || b1==2048)) return false;
    if(p[58]>2 || (p[59]&~1u) || (p[60]&~0x87u) || (p[60]&7u)>5u ||
       (p[60] && !(p[60]&7u))) return false;
    if(!memchr(p+10,0,32) || !memchr(p+42,0,16)) return false;
    if(!strcmp((const char *)p+10,"NO HEADER") && !strcmp((const char *)p+42,"HANDY")) return false;
    size_t total=64u+(size_t)(b0+b1)*256u*(p[59]&1u ? 2u : 1u);
    if(n!=total) return false;
    for(unsigned i=0;i<410;i++) if(p[64+i]) return true;
    return false;
}
#endif
static int open_impl(const void *rom, size_t size, const struct handy_pico_callbacks *cb,
                     const void *bios, size_t bn, const void *save, size_t sn)
{
    if (active || !rom || !cb || !cb->video || !cb->audio ||
        !size) return -1;
    bool home=valid_homebrew(rom,size);
#ifdef HANDY_NATIVE_MEMORY_FS
    if(!home && !valid_cartridge(rom,size)) return -1;
    if(home && sn) return -1;
    if(!home && sn) {
        unsigned type=((const uint8_t *)rom)[60]&7u;
        if(!type || sn!=(128u<<(type-1))) return -1;
    }
    if(handy_vfs_prepare(bios,bn,save,sn)<0) return -1;
#else
    if(!home || bios || bn || save || sn) return -1;
#endif
    output = *cb;
    memset(&game, 0, sizeof(game));
    game.data = rom; game.size = size; game.name = "pico-test";
    game.ext = home ? "o" : "lnx"; game.persistent_data = true;
    pad = 0; rgb565 = false; callback_error = false;
    retro_set_environment(environment);
    retro_set_video_refresh(video);
    retro_set_audio_sample(audio_sample);
    retro_set_audio_sample_batch(audio_batch);
    retro_set_input_poll(poll);
    retro_set_input_state(input);
    retro_init();
    struct retro_game_info info = {NULL, rom, size, NULL};
    if (!retro_load_game(&info)) { retro_deinit(); return -2; }
    active = true;
    if (!rgb565 || callback_error) { handy_pico_close(); return -3; }
    return 0;
}
int handy_pico_open(const void *rom,size_t size,const struct handy_pico_callbacks *cb)
{ return open_impl(rom,size,cb,NULL,0,NULL,0); }
#ifdef HANDY_NATIVE_MEMORY_FS
int handy_pico_open_ex(const void *rom,size_t size,const struct handy_pico_callbacks *cb,
                       const void *bios,size_t bn,const void *save,size_t sn)
{ return open_impl(rom,size,cb,bios,bn,save,sn); }
int handy_pico_eeprom(void *dst,size_t cap)
{
    if(!active) return -1;
    handy_native_flush_eeprom();
    return handy_vfs_export(dst,cap);
}
int handy_pico_bios_valid(void) { return active ? handy_native_bios_valid() : 0; }
#endif
void handy_pico_close(void)
{
    if (active) { retro_unload_game(); retro_deinit(); }
    active = false;
    memset(&game, 0, sizeof(game));
    memset(&output, 0, sizeof(output));
}
int handy_pico_frame(uint16_t buttons)
{
    if (!active) return -1;
    pad = buttons;
    retro_run();
    return callback_error ? -2 : 0;
}
void handy_pico_reset(void) { if (active) retro_reset(); }
size_t handy_pico_state_size(void) { return active ? retro_serialize_size() : 0; }
int handy_pico_save(void *dst, size_t size)
{
    return active && dst && size && size == handy_pico_state_size() &&
           retro_serialize(dst, size) ? 0 : -1;
}
int handy_pico_load(const void *src, size_t size)
{
    return active && src && size && size == handy_pico_state_size() &&
           retro_unserialize(src, size) ? 0 : -1;
}
void *handy_pico_ram(void) { return active ? retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM) : NULL; }
size_t handy_pico_ram_size(void) { return active ? retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM) : 0; }
