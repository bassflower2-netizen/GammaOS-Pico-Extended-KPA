/* ARM Linux runner ONLY. Execute the exact freestanding overlay with its own
 * newlib, using a real read-only FAT image. No Linux code enters the blob. */
#define _GNU_SOURCE
#include "geo_blob_abi.h"
#include "../../emu/gba/fat_ro.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>
#include <time.h>
static unsigned videos,audio_frames,reads,write_requests;
static unsigned rd(void *ctx,uint32_t lba,uint32_t n,void *p)
{
    FILE *disk=ctx;
    ++reads;
    if(fseek(disk,(long)lba*512,SEEK_SET))return 0;
    return (unsigned)fread(p,512,n,disk);
}
static int32_t log_out(void *ctx,int32_t fd,const void *p,uint32_t n)
{
    (void)ctx;
    if(fd!=1&&fd!=2)return -1;
    return (int32_t)fwrite(p,1,n,fd==1?stdout:stderr);
}
static int32_t clock_out(void *ctx,int64_t *sec,int32_t *us)
{
    (void)ctx;struct timespec t;
    if(clock_gettime(CLOCK_REALTIME,&t))return -1;
    *sec=t.tv_sec;*us=(int32_t)(t.tv_nsec/1000);return 0;
}
static void fatal(void *ctx,int32_t status)
{(void)ctx;fprintf(stderr,"Unexpected overlay fatal status %d\n",(int)status);exit(99);}
static int32_t reject_put(void *v,const char *d,const char *n,const void *p,uint32_t len)
{(void)v;(void)d;(void)n;(void)p;(void)len;++write_requests;return -1;}
static int32_t reject_mkpath(void *v,const char *p)
{(void)v;(void)p;++write_requests;return -1;}
static void video(const void *p,uint32_t w,uint32_t h,uint32_t pitch)
{assert(w&&h&&w<=320&&h<=264);if(p)assert(pitch>=w*4u);++videos;}
static uint32_t audio(const int16_t *p,uint32_t n)
{if(n)assert(p);audio_frames+=n;return n;}
static void av_log(const char *msg){fputs(msg,stderr);}
int main(int argc,char **argv)
{
    assert(argc==3);
    FILE *blob=fopen(argv[1],"rb");assert(blob);
    uint32_t hdr[5];assert(fread(hdr,1,sizeof hdr,blob)==sizeof hdr);
    assert(hdr[0]==GEO_BLOB_MAGIC&&hdr[1]==GEO_BLOB_VERSION);
    assert(hdr[2]>=sizeof hdr&&!(hdr[2]&3u)&&hdr[2]<hdr[3]);
    assert(hdr[3]<=hdr[4]&&hdr[4]<=GEO_BLOB_SLOT_SIZE);
    assert(!fseek(blob,0,SEEK_END)&&ftell(blob)==(long)hdr[3]);
    rewind(blob);
    void *slot=mmap((void *)(uintptr_t)GEO_BLOB_VMA,GEO_BLOB_SLOT_SIZE,
                    PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(slot==(void *)(uintptr_t)GEO_BLOB_VMA);
    assert(fread(slot,1,hdr[3],blob)==hdr[3]);assert(!fclose(blob));
    memset((unsigned char *)slot+hdr[3],0,hdr[4]-hdr[3]);
    __builtin___clear_cache(slot,(char *)slot+hdr[3]);
    geo_blob_entry_fn entry=(geo_blob_entry_fn)((unsigned char *)slot+hdr[2]);
    const struct geo_blob_exports *api=entry();
    assert(api&&api->magic==GEO_BLOB_MAGIC&&api->version==GEO_BLOB_VERSION&&api->size==sizeof *api);
    assert(api->init&&api->open&&api->close&&api->frame&&api->reset&&api->state_size&&api->save&&api->load);
    /* Test allocation only, not a reservation recommendation for the device. */
    const uint32_t heap_size=64u*1024u*1024u;
    void *heap=mmap(NULL,heap_size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(heap!=MAP_FAILED);
    void *scratch=malloc(1024u*1024u);assert(scratch);
    FILE *disk=fopen(argv[2],"rb");assert(disk);
    fat_vol v;assert(!fat_mount(&v,rd,disk));
    const struct geo_blob_host host={sizeof host,NULL,log_out,clock_out,fatal,reject_put,reject_mkpath};
    const struct geo_blob_av av={video,audio,av_log};
    assert(api->open(&v,"/roms/neo/test.neo",1,scratch,1024u*1024u,&av)==-1);
    assert(!api->init(heap,heap_size,&host));
    assert(api->init(heap,heap_size,&host)==-1);
    assert(api->open(&v,"/roms/neo/missing.neo",1,scratch,1024u*1024u,&av)==-2);
    int rc=api->open(&v,"/roms/neo/test.neo",1,scratch,1024u*1024u,&av);
    fprintf(stderr,"Native flat open returned %d, %u sector read calls\n",rc,reads);
    assert(!rc);
    assert(api->open(&v,"/roms/neo/test.neo",1,scratch,1024u*1024u,&av)==-1);
    for(unsigned i=0;i<8;i++)assert(!api->frame(0,0));
    assert(videos>=8&&audio_frames>0);
    uint32_t state=api->state_size();
    assert(state>65536&&state<=1024u*1024u);
    assert(api->save(0,scratch,1024u*1024u)==-1); /* read-only volume */
    assert(api->load(0,scratch,1024u*1024u)==-3); /* absent slot */
    assert(!api->frame(0,0));
    api->close();api->close();
    assert(api->frame(0,0)==-1&&!api->state_size());
    assert(!write_requests);
    printf("Geolith native flat runtime passed: %u video callbacks, %u stereo frames, %u byte state, %u sector reads\n",
           videos,audio_frames,state,reads);
    assert(!fclose(disk));free(scratch);
    assert(!munmap(heap,heap_size));assert(!munmap(slot,GEO_BLOB_SLOT_SIZE));
    return 0;
}
