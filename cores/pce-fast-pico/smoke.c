/* Original synthetic HuC6280 fixture; no commercial code. GPL-2.0-only. */
#include "pce_adapter.h"
#include "libretro.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef PCE_FLAT_TEST
#include "flat_test.h"
#endif
static unsigned videos,frames;
static void video(const uint16_t *p,unsigned w,unsigned h,size_t pitch)
{ assert(w&&h&&w<=1024&&h<=512);if(p)assert(pitch>=w*2);++videos; }
static size_t audio(const int16_t *p,size_t n)
{ if(n)assert(p);frames+=(unsigned)n;return n; }
static void make_rom(uint8_t *rom)
{
    memset(rom,0xea,8192);
    /* Reset MPR7=0 maps physical bank zero at E000. SEI; JMP E001. */
    rom[0]=0x78;rom[1]=0x4c;rom[2]=0x01;rom[3]=0xe0;
    for(unsigned i=0x1ff6;i<8192;i+=2){rom[i]=0;rom[i+1]=0xe0;}
}
int main(void)
{
    static uint8_t rom[8192+512];
    struct pce_pico_callbacks cb={video,audio};
    #ifdef PCE_FLAT_TEST
    flat_load();
    #endif
    make_rom(rom+512);
    assert(pce_pico_frame(0)<0);
    assert(pce_pico_open(rom,512,&cb)<0);
    assert(pce_pico_open(rom+512,8192,&cb)==0);
    assert(pce_pico_ram_size()==8192 && pce_pico_bram_size()==2048);
    uint8_t *ram=pce_pico_ram(),*bram=pce_pico_bram();
    assert(ram&&bram);
    for(unsigned i=0;i<12;i++)assert(pce_pico_frame(i&1?1u<<RETRO_DEVICE_ID_JOYPAD_A:0)==0);
    assert(videos>=12&&frames>0);
    pce_pico_reset();
    assert(pce_pico_frame(0)==0);
    size_t n=pce_pico_state_size();assert(n>8192&&n<8u*1024u*1024u);
    void *state=malloc(n);assert(state);
    ram[0x100]=0x42;bram[0x100]=0x37;
    assert(pce_pico_save(state,n)==0);
    ram[0x100]=0x99;bram[0x100]=0x98;
    assert(pce_pico_load(state,n)==0);
    assert(ram[0x100]==0x42&&bram[0x100]==0x37);
    pce_pico_close();
    assert(!pce_pico_ram()&&!pce_pico_state_size());
    /* Separate session with copier header. */
    assert(pce_pico_open(rom,sizeof rom,&cb)==0);
    assert(pce_pico_frame(0)==0);
    assert(pce_pico_load(state,n)==0);
    assert(((uint8_t *)pce_pico_ram())[0x100]==0x42);
    assert(((uint8_t *)pce_pico_bram())[0x100]==0x37);
    pce_pico_close();free(state);
    printf("PCE memory smoke: %u video callbacks, %u stereo frames, %lu byte state\n",videos,frames,(unsigned long)n);
    return 0;
}
