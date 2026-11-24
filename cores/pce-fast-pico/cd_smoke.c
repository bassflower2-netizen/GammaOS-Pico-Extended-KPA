/* Original synthetic disc and BIOS fixtures. No copyrighted game content. */
#include "pce_adapter.h"
#include "mednafen/cdrom/cdromif.h"
#include <streams/file_stream.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static uint8_t bios[262144];
static const char cue[]="FILE \"track.bin\" BINARY\n  TRACK 01 MODE1/2048\n    INDEX 01 00:00:00\n";
#define DISC_BYTES (2048u*256u)
static unsigned opens,closes,reads,track_reads,video_frames,audio_frames;
static int missing_bios;
static int open_file(const char *path,uint32_t *size)
{
    int id;
    if(!strcmp(path,"/pico/bios/syscard3.pce")&&!missing_bios){id=1;*size=sizeof bios;}
    else if(!strcmp(path,"/pico/disc/test.cue")){id=2;*size=sizeof cue-1;}
    else if(!strcmp(path,"/pico/disc/track.bin")){id=3;*size=DISC_BYTES;}
    else return -1;
    ++opens;return id;
}
static int read_file(int id,uint32_t at,void *dst,uint32_t n)
{
    ++reads;
    if(id==1&&at<=sizeof bios&&n<=sizeof bios-at)memcpy(dst,bios+at,n);
    else if(id==2&&at<=sizeof cue-1&&n<=sizeof cue-1-at)memcpy(dst,cue+at,n);
    else if(id==3&&at<=DISC_BYTES&&n<=DISC_BYTES-at){
        uint8_t *p=dst;
        ++track_reads;
        for(uint32_t i=0;i<n;i++)p[i]=(uint8_t)(((at+i)>>11)^((at+i)&255u));
    }else return -1;
    return (int)n;
}
static void close_file(int id){assert(id>=1&&id<=3);++closes;}
static void video(const uint16_t *p,unsigned w,unsigned h,size_t pitch)
{assert(w&&h&&w<=1024&&h<=512);if(p)assert(pitch>=w*2);++video_frames;}
static size_t audio(const int16_t *p,size_t n)
{if(n)assert(p);audio_frames+=(unsigned)n;return n;}
int main(void)
{
    /* Each 8 KiB bank has a reset-safe HuC6280 loop, not a real System Card. */
    memset(bios,0xea,sizeof bios);
    for(unsigned b=0;b<sizeof bios;b+=8192){
        bios[b]=0x78;bios[b+1]=0x4c;bios[b+2]=1;bios[b+3]=0xe0;
        for(unsigned i=0x1ff6;i<8192;i+=2){bios[b+i]=0;bios[b+i+1]=0xe0;}
    }
    const struct pce_cd_io io={open_file,read_file,close_file};
    const struct pce_pico_callbacks cb={video,audio};
    missing_bios=1;
    assert(pce_pico_open_cd("test.cue",&io,&cb)<0);
    missing_bios=0;
    assert(pce_pico_open_cd("../test.cue",&io,&cb)<0);
    assert(pce_pico_open_cd("test.cue",&io,&cb)==0);
    /* Exercise actual upstream CD sector decoding at non-sequential LBAs.
     * Separate interface on the same read-only fixture. */
    CDIF *disc=CDIF_Open("/pico/disc/test.cue",false);
    assert(disc);
    static uint8_t sector[2048];
    const unsigned lbas[]={3,201,4,255};
    for(unsigned j=0;j<sizeof lbas/sizeof lbas[0];j++){
        unsigned lba=lbas[j];
        assert(CDIF_ReadSector(disc,sector,lba,1)==1);
        for(unsigned i=0;i<sizeof sector;i++)assert(sector[i]==(uint8_t)(lba^(i&255)));
    }
    CDIF_Close(disc);
    for(unsigned i=0;i<8;i++)assert(pce_pico_frame(0)==0);
    assert(video_frames>=8&&audio_frames>0&&track_reads>0);
    size_t n=pce_pico_state_size();assert(n>8192&&n<4u*1024u*1024u);
    void *state=malloc(n);assert(state);
    uint8_t *bram=pce_pico_bram();assert(bram&&pce_pico_bram_size()==2048);
    bram[100]=0x43;assert(pce_pico_save(state,n)==0);
    bram[100]=0;assert(pce_pico_load(state,n)==0&&bram[100]==0x43);
    free(state);
    pce_pico_close();
    assert(opens==closes);
    printf("PCE CD synthetic smoke: %u video, %u audio, %u reads (%u track), %u balanced opens/closes, %lu byte state\n",video_frames,audio_frames,reads,track_reads,opens,(unsigned long)n);
    return 0;
}
