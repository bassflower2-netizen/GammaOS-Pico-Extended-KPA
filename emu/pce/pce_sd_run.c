/* Experimental PC Engine session. Uses existing LK display/audio/IGM infrastructure.
 * The ROM scanner dispatches system ID 10 to this session.
 * The session owns the armSNES overlay and the shared emulator scratch arena
 * until all callbacks have ceased and the core has closed. */
#include <string.h>
#include <stdio.h>
#include <platform/ayaneo_volume_input.h>
#include "pce_core_loader.h"
#include "pce_sd_io.h"
#include "../gba/sd_fat.h"
#include "../gba/rom_archive.h"
#include "../gba/pico_igm.h"
#include "../gba/pico_input.h"
#include "../gba/gba_sd_save.h"
#include "../gba/card_writer.h"
#include "../gba/pico_state_shot.h"
#include "../gba/emu_stack.h"
#include <kernel/event.h>

#define ROM_BUF ((unsigned char *)0x52C00000u)
#define ROM_CAP (0x280000u+512u)
#define STATE_BUF ((unsigned char *)0x53400000u)
#define STATE_CAP 0x400000u
#define VIDEO_BUF ((uint16_t *)0x53800000u)
#define HELD(mask,k) ((mask)&(1u<<(k)))

extern void ayaneo_genesis_show_frame(const unsigned short *,unsigned,unsigned,unsigned);
extern void ayaneo_snes_audio_submit(const short *,unsigned,unsigned);
extern void ayaneo_menu_audio_silence(void);
extern void ayaneo_gbc_audio_pause(int);
extern void ayaneo_snes_rsz_restore(void);
extern void ayaneo_joypad_poll(void);
extern unsigned ayaneo_joypad_dpad(void);
extern void mtk_wdt_restart(void);
extern unsigned ayaneo_dsi_set_fps_milli(unsigned);
extern unsigned ayaneo_dsi_default_vfp(void);
extern void ayaneo_dsi_set_vfp(unsigned);
extern void ayaneo_menu_overlay(void (*)(unsigned *,unsigned,unsigned,unsigned),int);
extern void ayaneo_menu_overlay_mark_dirty(void);
extern void ayaneo_fill(unsigned *,unsigned,int,int,int,int,unsigned);
extern int ayaneo_text(unsigned *,unsigned,int,int,int,unsigned,const char *);
extern int ayaneo_brightness_pct(void), ayaneo_brightness_step(int);
extern int ayaneo_gbc_audio_get_volume(void);
extern void ayaneo_gbc_audio_set_volume(int);
extern void ayaneo_gbc_osd_show(int,int);
extern void ayaneo_menu_settings_persist(void);
extern void ayaneo_settings_persist_flush(void);
extern int ayaneo_power_hold(int,int);
extern void ayaneo_power_off_now(void);
extern void ayaneo_resume_mark(const char *);
extern int gba_launch_state(char *);
extern int priamry_display_wait_for_vsync(void);
extern void genesis_menu_arm_reverse(const unsigned short *,unsigned,unsigned,unsigned);
extern void ayaneo_fps_tick(void);
extern volatile int g_genesis_aspect,g_genesis_filter;
extern volatile unsigned g_genesis_aspect_x1000;
extern volatile int g_dbg_igm_toggle,g_dbg_force_close;
extern int gba_punch_ready;

static const struct pce_blob_exports *api;
static fat_vol *vol;
static const gba_rom_entry *rom;
static unsigned width,height;
static int cd_session,cd_io_started;
static const char *state_dir(void){return cd_session?"/states/pcecd":"/states/pce";}
static const char *save_dir(void){return cd_session?"/saves/pcecd":"/saves/pce";}
static int opened,exit_game,menu,rich_menu,slot,selection,dirty;
static char status[96];
static unsigned settings_age;
volatile unsigned g_pce_frames;
volatile int g_pce_loaderr,g_pce_loadrc;

enum { LM_SAVE=20, LM_LOAD, LM_SLOT, LM_RESET, LM_EXIT, LM_BRIGHT, LM_VOLUME, LM_ASPECT, LM_FILTER };
static void note(const char *s) { snprintf(status,sizeof status,"%s",s); }
static void video(const uint16_t *p,unsigned w,unsigned h,size_t pitch)
{
    if(!p || !w || !h || w>512 || h>256 || pitch<w*2u) return;
    for(unsigned y=0;y<h;y++) memcpy(VIDEO_BUF+y*w,(const unsigned char *)p+y*pitch,w*2u);
    width=w; height=h;
}
static size_t audio(const int16_t *p,size_t n)
{
    if(p && n && n<=4096 && !menu) ayaneo_snes_audio_submit(p,(unsigned)n,44100);
    return n;
}
static const struct pce_pico_callbacks callbacks={video,audio};
static void slot_ext(char e[4]) { e[0]='s'; e[1]='t'; e[2]='0'+slot; e[3]=0; }
static int save_state(const char *ext)
{
    size_t n=api->state_size();
    if(!n || n>STATE_CAP || api->save(STATE_BUF,n)) return -1;
    if(gba_sd_write_named(vol,state_dir(),rom->name,ext,STATE_BUF,(unsigned)n)) return -1;
    pico_shot_save(vol,state_dir(),rom->name,ext,0);
    return 0;
}
static int load_state(const char *ext)
{
    size_t n=api->state_size();
    if(!n || n>=STATE_CAP) return -1;
    card_writer_flush();
    unsigned got=gba_sd_read_named(vol,state_dir(),rom->name,ext,STATE_BUF,(unsigned)n+1);
    return got==n ? api->load(STATE_BUF,n) : -1;
}
static int battery_save(void)
{
    size_t n=api->bram_size();
    const void *p=api->bram();
    if(!n || !p || n>32768) return -1;
    return gba_sd_write_named(vol,save_dir(),rom->name,"sav",p,(unsigned)n);
}
static void persist(void)
{
    if(dirty) { ayaneo_menu_settings_persist(); dirty=0; settings_age=0; }
}
static const char *label(int i)
{
    switch(i) {
    case LM_SAVE:return "Save state"; case LM_LOAD:return "Load state";
    case LM_SLOT:return "Slot"; case LM_RESET:return "Reset Game"; case LM_EXIT:return "Exit Game";
    case LM_BRIGHT:return "Brightness"; case LM_VOLUME:return "Volume";
    case LM_ASPECT:return "Aspect Ratio"; case LM_FILTER:return "Filter"; default:return "";
    }
}
static const char *value(int i,char *buf)
{
    buf[0]=0;
    if(i==LM_SLOT || i==LM_SAVE || i==LM_LOAD) snprintf(buf,96,"%d",slot+1);
    if(i==LM_BRIGHT) snprintf(buf,96,"%d%%",ayaneo_brightness_pct());
    if(i==LM_VOLUME) snprintf(buf,96,"%d%%",ayaneo_gbc_audio_get_volume());
    if(i==LM_ASPECT) {
        static const char *const names[]={"Pixel","Core","Stretch","4:3","Square","16:9"};
        snprintf(buf,96,"%s",names[(unsigned)g_genesis_aspect<6u?g_genesis_aspect:1]);
    }
    if(i==LM_FILTER) {
        snprintf(buf,96,"%d",g_genesis_filter);
    }
    return buf;
}
static int change(int i,int dir,int act)
{
    char e[4];
    status[0]=0;
    switch(i) {
    case LM_SLOT: if(dir) slot=(slot+dir+3)%3; break;
    case LM_SAVE: if(act) { slot_ext(e); note(save_state(e)?"Save failed":"State saved"); } break;
    case LM_LOAD: if(act) { slot_ext(e); note(load_state(e)?"No save state":"State loaded"); } break;
    case LM_RESET: if(act) { api->reset(); return 1; } break;
    case LM_EXIT: if(act) { exit_game=1; return 1; } break;
    case LM_BRIGHT: if(dir) { ayaneo_brightness_step(dir); dirty=1; settings_age=0; } break;
    case LM_VOLUME: if(dir) {
        int v=ayaneo_gbc_audio_get_volume()+dir*5;
        ayaneo_gbc_audio_set_volume(v<0?0:v>100?100:v); dirty=1; settings_age=0;
    } break;
    case LM_ASPECT: if(dir) g_genesis_aspect=(g_genesis_aspect+dir+6)%6; break;
    case LM_FILTER: if(dir) g_genesis_filter=(g_genesis_filter+dir+6)%6; break;
    }
    return 0;
}
static const struct igm_row rows[]={
    {LM_SAVE,IGM_TOP,IGM_ACTION},{LM_LOAD,IGM_TOP,IGM_ACTION},
    {IGM_DISPLAY,IGM_TOP,IGM_LINK},{IGM_SOUND,IGM_TOP,IGM_LINK},
    {LM_RESET,IGM_TOP,IGM_ACTION},{LM_EXIT,IGM_TOP,IGM_ACTION},
    {LM_BRIGHT,IGM_DISPLAY,IGM_VALUE},{LM_ASPECT,IGM_DISPLAY,IGM_VALUE},
    {LM_FILTER,IGM_DISPLAY,IGM_VALUE},{LM_VOLUME,IGM_SOUND,IGM_VALUE}
};
static struct igm_core igm={
    .rows=rows,.nrows=sizeof rows/sizeof rows[0],
    .slot_item=LM_SLOT,.save_item=LM_SAVE,.load_item=LM_LOAD,.reset_item=LM_RESET,.exit_item=LM_EXIT,
    .label=label,.value=value,.change=change,.status=status,.system=GBA_CONSOLE_PCE
};
static const int simple_rows[]={LM_SAVE,LM_LOAD,LM_SLOT,LM_BRIGHT,LM_VOLUME,LM_ASPECT,LM_FILTER,LM_RESET,LM_EXIT};
#define SIMPLE_N ((int)(sizeof simple_rows/sizeof simple_rows[0]))
static void paint(unsigned *buf,unsigned pitch,unsigned w,unsigned h)
{
    int x=((int)w-480)/2,y=((int)h-420)/2;
    char b[96];
    ayaneo_fill(buf,pitch,x,y,480,420,0xe010141cu);
    ayaneo_text(buf,pitch,x+20,y+16,3,0xffffffffu,cd_session?"PC Engine CD":"PC Engine");
    for(int i=0;i<SIMPLE_N;i++) {
        unsigned fg=i==selection?0xff101018u:0xffffffffu;
        int yy=y+55+i*32;
        if(i==selection) ayaneo_fill(buf,pitch,x+8,yy-2,464,30,0xffa0c8ffu);
        ayaneo_text(buf,pitch,x+20,yy,2,fg,label(simple_rows[i]));
        ayaneo_text(buf,pitch,x+340,yy,2,fg,value(simple_rows[i],b));
    }
    ayaneo_text(buf,pitch,x+20,y+360,2,0xff80e080u,status);
    ayaneo_text(buf,pitch,x+20,y+392,1,0xffffffffu,"A Select  B Resume");
}
static unsigned menu_keys(unsigned raw)
{
    unsigned k=0,j=ayaneo_joypad_dpad();
    if(HELD(raw,PB_UP)||(j&1)) k|=IGM_K_UP;
    if(HELD(raw,PB_DOWN)||(j&2)) k|=IGM_K_DOWN;
    if(HELD(raw,PB_LEFT)||(j&4)) k|=IGM_K_LEFT;
    if(HELD(raw,PB_RIGHT)||(j&8)) k|=IGM_K_RIGHT;
    if(HELD(raw,PB_A)) k|=IGM_K_A;
    if(HELD(raw,PB_B)) k|=IGM_K_B;
    if(HELD(raw,PB_L)) k|=IGM_K_L;
    if(HELD(raw,PB_R)) k|=IGM_K_R;
    return k;
}
/* The scanner stores a shortened display key, not the full filename.
 * Recover the original CUE name by its cached directory entry identity. */
static int find_cue(char *name,size_t cap)
{
    const char *dir=gba_sd_rom_dir(rom);
    fat_dir d;fat_dirent e;
    if(!dir||rom->arc||fat_opendir(vol,dir,&d))return -1;
    while(fat_readdir(&d,&e)){
        if(e.is_dir||e.first_clus!=rom->first_clus||e.size!=rom->size||
           e.ex_nofat!=rom->ex_nofat)continue;
        const char *ext=strrchr(e.name,'.');
        size_t n=strlen(e.name);
        if(!ext||!fat_name_eq(ext,".cue")||n>=cap||n>240)continue;
        memcpy(name,e.name,n+1);return 0;
    }
    return -1;
}
static void session_body(void)
{
    unsigned n,sn;
    size_t bram_size;
    int oldaspect=g_genesis_aspect,oldfilter=g_genesis_filter;
    unsigned oldratio=g_genesis_aspect_x1000,prev=~0u,prevkeys=~0u;
    cd_session=rom->type==GBA_CONSOLE_PCECD; cd_io_started=0;
    g_pce_frames=0; g_pce_loadrc=-1; width=height=0;
    opened=exit_game=menu=rich_menu=slot=selection=dirty=0; settings_age=0; status[0]=0;
    /* Menu launch animation may have muted the game audio. No punch animation
     * in this first driver; restore the existing audio path explicitly. */
    gba_punch_ready=0; ayaneo_gbc_audio_pause(0);
    api=pce_core_load(vol,(int *)&g_pce_loaderr);
    if(!api) goto fail;
    if(cd_session){
        char cue[256];
        if(find_cue(cue,sizeof cue)||pce_sd_io_begin(vol,gba_sd_rom_dir(rom)))goto fail;
        cd_io_started=1;
        g_pce_loadrc=api->open_cd(cue,&pce_sd_io,&callbacks);
    }else{
        n=gba_sd_load_rom(vol,rom,ROM_BUF,ROM_CAP);
        if(!n) goto fail;
        g_pce_loadrc=api->open(ROM_BUF,n,&callbacks);
    }
    if(g_pce_loadrc) goto fail;
    opened=1;
    bram_size=api->bram_size();
    if(!bram_size || bram_size>32768 || !api->bram()) goto fail;
    sn=gba_sd_read_named(vol,save_dir(),rom->name,"sav",STATE_BUF,(unsigned)bram_size+1);
    if(sn && sn!=bram_size) goto fail;
    if(sn) memcpy(api->bram(),STATE_BUF,sn);
    igm.game=rom->uname[0]?rom->uname:rom->name;
    igm.system=rom->type;
    pico_input_game(vol,rom->type,rom->name,&igm,-1);
    g_genesis_aspect=1; g_genesis_filter=0; g_genesis_aspect_x1000=1200;
    ayaneo_dsi_set_fps_milli(59820);
    { char e[4]; if(!HELD(pico_input_raw(),PB_B) && gba_launch_state(e)) (void)load_state(e); }
    while(!exit_game) {
        unsigned raw,k,edge;
        ayaneo_joypad_poll(); raw=pico_input_raw(); pico_input_frame(menu);
        int tap=HELD(raw,PB_MENU)&&!HELD(prev,PB_MENU);
        if(g_dbg_igm_toggle) { g_dbg_igm_toggle=0; tap=1; }
        if(tap) {
            if(menu) { if(rich_menu) pico_igm_dismiss(); else menu=0; }
            else {
                menu=1; rich_menu=pico_igm_on(); selection=0; prevkeys=~0u; status[0]=0;
                ayaneo_menu_audio_silence();
                if(rich_menu) pico_igm_open(&igm);
            }
        }
        k=menu_keys(raw); edge=k&~prevkeys; prevkeys=k; prev=raw;
        if(menu) {
            if(rich_menu) { if(pico_igm_frame(k)>=0) menu=0; }
            else {
                if(edge&IGM_K_UP) selection=(selection+SIMPLE_N-1)%SIMPLE_N;
                if(edge&IGM_K_DOWN) selection=(selection+1)%SIMPLE_N;
                if(edge&IGM_K_LEFT) change(simple_rows[selection],-1,0);
                if(edge&IGM_K_RIGHT) change(simple_rows[selection],1,0);
                if(edge&IGM_K_A) { if(change(simple_rows[selection],0,1)) menu=0; }
                if(edge&IGM_K_B) menu=0;
                if(edge) ayaneo_menu_overlay_mark_dirty();
            }
            if(!menu) { pico_input_hold_back(); persist(); }
        }
        if(rich_menu) pico_igm_overlay(menu);
        else ayaneo_menu_overlay(paint,menu);
        if(g_dbg_force_close) { g_dbg_force_close=0; exit_game=1; }
        if(exit_game) break;
        if(!menu) {
            if(api->frame((uint16_t)pico_input_pad())) { gba_rom_set_error(RA_ERR_START); break; }
            ++g_pce_frames; ayaneo_fps_tick();
        }
        if(width&&height) {
            g_genesis_aspect_x1000=1200;
            ayaneo_genesis_show_frame(VIDEO_BUF,width,height,width);
        } else priamry_display_wait_for_vsync();
        { int dir=ayaneo_volume_dir(); if(dir) {
            if(HELD(raw,PB_SELECT)) { change(LM_BRIGHT,dir,0); ayaneo_gbc_osd_show(2,ayaneo_brightness_pct()); }
            else { change(LM_VOLUME,dir,0); ayaneo_gbc_osd_show(1,ayaneo_gbc_audio_get_volume()); }
        }}
        if(dirty && ++settings_age>=75) persist();
        mtk_wdt_restart();
        if(ayaneo_power_hold(menu,1)) {
            ayaneo_gbc_audio_pause(1); card_writer_flush();
            (void)battery_save(); (void)save_state("sus");
            ayaneo_resume_mark(rom->name); persist(); ayaneo_menu_settings_persist();
            ayaneo_settings_persist_flush(); ayaneo_power_off_now();
            exit_game=1;
        }
    }
    ayaneo_menu_audio_silence();
    card_writer_flush();
    (void)battery_save(); (void)save_state("sus");
    if(width&&height) genesis_menu_arm_reverse(VIDEO_BUF,width,height,width);
    api->close(); opened=0;
    if(cd_io_started){pce_sd_io_end();cd_io_started=0;}
    pico_input_game_end();
    pico_igm_overlay(0); ayaneo_menu_overlay(0,0);
    ayaneo_snes_rsz_restore(); ayaneo_dsi_set_vfp(ayaneo_dsi_default_vfp());
    g_genesis_aspect=oldaspect; g_genesis_filter=oldfilter; g_genesis_aspect_x1000=oldratio;
    persist(); api=0; return;
fail:
    gba_rom_set_error(RA_ERR_START);
    if(opened && api) api->close();
    if(cd_io_started){pce_sd_io_end();cd_io_started=0;}
    opened=0; api=0; ayaneo_menu_audio_silence();
}
static thread_t *worker;
static event_t kick,done;
static int run(void *arg)
{
    (void)arg;
    for(;;) { event_wait(&kick); session_body(); event_signal(&done,false); }
    return 0;
}
void pce_sd_session(fat_vol *v,const gba_rom_entry *r)
{
    if(!worker) {
        void *s=ayaneo_emu_stack(EMU_STACK_PCE,262144);
        if(!s) { gba_rom_set_error(RA_ERR_START); return; }
        event_init(&kick,false,EVENT_FLAG_AUTOUNSIGNAL);
        event_init(&done,false,EVENT_FLAG_AUTOUNSIGNAL);
        worker=thread_create_on_stack("pce_emu",run,NULL,DEFAULT_PRIORITY,s,262144);
        if(!worker) { gba_rom_set_error(RA_ERR_START); return; }
        thread_resume(worker);
    }
    vol=v; rom=r;
    event_signal(&kick,false); event_wait(&done);
    vol=0; rom=0;
}
