/* No commercial ROM or BIOS data. This is a libretro host/ARM probe only. */
#include "neo_format.h"
#include "sd_cart.h"
#include "geo_zlib.h"
#include "cart_io.h"
#ifdef GEO_RUNTIME_FAT_SAVE
#include "sd_save.h"
#endif
#include "geo_adapter.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *dir;
static unsigned videos,frames;
static void logline(const char *msg){fputs(msg,stderr);}
static void video(const void *p,unsigned w,unsigned h,size_t pitch)
{assert(w&&h&&w<=320&&h<=264);if(p)assert(pitch>=w*4u);++videos;}
static size_t audio(const int16_t *p,size_t n)
{if(n)assert(p);frames+=(unsigned)n;return n;}
/* Host-only fullpath adapter: load the fixture from FAT, not the original
 * host cartridge. Native memory handoff remains a separate porting step. */
static unsigned disk_read(void *ctx,uint32_t lba,uint32_t n,void *p)
{
    FILE *fp=ctx;
    if(fseek(fp,(long)lba*512,SEEK_SET))return 0;
    return (unsigned)fread(p,512,n,fp);
}
#ifdef GEO_RUNTIME_FAT_SAVE
static unsigned disk_write(void *ctx,uint32_t lba,uint32_t n,const void *p)
{
    FILE *fp=ctx;
    if(fseek(fp,(long)lba*512,SEEK_SET))return 0;
    return (unsigned)fwrite(p,512,n,fp);
}
/* The host test uses the MVS storage namespace as a fixture label; engine
 * mode detection and actual device writer binding are separate work. */
static void state_roundtrip(const char *image,uint64_t key,const void *state,void *restored,uint32_t n)
{
    FILE *disk=fopen(image,"r+b");assert(disk);
    fat_vol vol;assert(!fat_mount(&vol,disk_read,disk));
    fat_set_writer(&vol,disk_write);
    assert(!geo_sd_save_put(&vol,key,0,0,state,n));
    assert(!fclose(disk));
    disk=fopen(image,"rb");assert(disk);
    assert(!fat_mount(&vol,disk_read,disk));
    memset(restored,0,n);
    assert(!geo_sd_save_get(&vol,key,0,0,restored,n));
    assert(!memcmp(restored,state,n));
    assert(geo_sd_save_get(&vol,key^1u,0,0,restored,n)==-2);
    assert(geo_sd_save_get(&vol,key,1,0,restored,n)==-2);
    assert(geo_sd_save_get(&vol,key,0,1,restored,n)==-2);
    assert(!fclose(disk));
    puts("Geolith runtime state restored from remounted FAT image");
}
#endif
/* Compare every staged BIOS byte with the original synthetic fixture
 * contract before publishing the cartridge. */
static void verify_bios(const geo_bios_slot slots[4])
{
    const unsigned char *p=slots[0].data;
    static const unsigned char reset[]={0x10,0,0,0xff,0xc0,0,0,1};
    static const unsigned char vector[]={0xc0,0,0,1};
    assert(!memcmp(p,reset,sizeof reset));
    for(size_t i=8;i<0x100;i+=4)assert(!memcmp(p+i,vector,4));
    assert(p[0x100]==0xfe&&p[0x101]==0x60);
    for(size_t i=0x102;i<slots[0].capacity;i++)assert(!p[i]);
    for(unsigned s=1;s<4;s++){
        p=slots[s].data;
        for(size_t i=0;i<slots[s].capacity;i++)assert(p[i]==(s==3?0x76:0));
    }
}
static int publish(void *ctx,int aes,const geo_bios_slot slots[4],const void *cart,size_t size)
{
    assert(aes==0 && slots[0].capacity==0x20000 && slots[1].capacity==0x10000);
    assert(slots[2].capacity==0x20000 && slots[3].capacity==0x20000);
    verify_bios(slots);
    FILE *fp=fopen(ctx,"wb");if(!fp)return -1;
    size_t written=fwrite(cart,1,size,fp);
    int closed=fclose(fp);
    return written==size && !closed ? 0 : -1;
}
static void stage(const char *image,const char *path)
{
    FILE *disk=fopen(image,"rb");assert(disk);
    fat_vol vol;assert(!fat_mount(&vol,disk_read,disk));
    static unsigned char archive[0x100000],cart[0x400000];
    static unsigned char program[0x20000],lo[0x10000],sf[0x20000],sm[0x20000];
    geo_bios_slot slots[4]={{program,sizeof program},{lo,sizeof lo},{sf,sizeof sf},{sm,sizeof sm}};
    assert(!geo_sd_session_handoff(&vol,0,"sp-u2.sp1","/roms/neo/test.neo",
        archive,sizeof archive,slots,cart,sizeof cart,geo_zip_zlib_raw_inflate,NULL,publish,(void *)path));
    /* Export the exact archive read from FAT for the fullpath engine.
     * The original host BIOS archive is removed by CI before this test. */
    size_t zip_size=0;
    assert(!geo_sd_bios_load(&vol,0,archive,sizeof archive,&zip_size));
    char bios_path[512];
    int len=snprintf(bios_path,sizeof bios_path,"%s/neogeo.zip",dir);
    assert(len>0&&(size_t)len<sizeof bios_path);
    FILE *bios=fopen(bios_path,"wb");assert(bios);
    assert(fwrite(archive,1,zip_size,bios)==zip_size);
    assert(!fclose(bios));
    /* The upstream default may select AES; restore both system archives
     * exclusively from FAT, as in the previously passing fixture layout. */
    assert(!geo_sd_bios_set_load(&vol,1,"neo-po.bin",archive,sizeof archive,
        slots,geo_zip_zlib_raw_inflate,NULL));
    verify_bios(slots);
    assert(!geo_sd_bios_load(&vol,1,archive,sizeof archive,&zip_size));
    len=snprintf(bios_path,sizeof bios_path,"%s/aes.zip",dir);
    assert(len>0&&(size_t)len<sizeof bios_path);
    bios=fopen(bios_path,"wb");assert(bios);
    assert(fwrite(archive,1,zip_size,bios)==zip_size);
    assert(!fclose(bios));
    fclose(disk);
    printf("FAT session cartridge delivered to runtime fullpath adapter\n");
}
int main(int argc,char **argv)
{
    assert(argc==3);dir=argv[1];
    char path[512];
    assert(snprintf(path,sizeof path,"%s/test.neo",dir)>0);
    stage(argv[2],path);
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
    uint64_t key=geo_cart_key(rom,(size_t)size);
    assert(key==geo_cart_key(rom,(size_t)size));
    free(rom);
    const geo_pico_callbacks callbacks={video,audio,logline};
    assert(geo_pico_frame(0,0)==-1 && geo_pico_state_size()==0);
    assert(geo_pico_open(path,dir,dir,NULL)==-1);
    assert(!geo_pico_open(path,dir,dir,&callbacks));
    assert(geo_pico_open(path,dir,dir,&callbacks)==-1);
    for(unsigned i=0;i<8;i++)assert(!geo_pico_frame(0,0));
    assert(videos>=8&&frames>0);
    size_t n=geo_pico_state_size();assert(n>65536&&n<16u*1024u*1024u);
    void *state=malloc(n);assert(state);
    assert(geo_pico_save(state,n-1)==-1);
    assert(!geo_pico_save(state,n));
    assert(!geo_pico_frame(0,0));
#ifdef GEO_RUNTIME_FAT_SAVE
    void *restored=malloc(n);assert(restored);
    state_roundtrip(argv[2],key,state,restored,(uint32_t)n);
    assert(!geo_pico_load(restored,n));
    free(restored);
#else
    (void)key;
    assert(!geo_pico_load(state,n));
#endif
    assert(geo_pico_load(state,n-1)==-1);
    assert(!geo_pico_frame(0,0));free(state);
    geo_pico_close();geo_pico_close();
    assert(geo_pico_frame(0,0)==-1 && geo_pico_state_size()==0);
    puts("Geolith frontend lifecycle passed");
    printf("Geolith synthetic cartridge: %u video callbacks, %u stereo frames, %lu byte state\n",videos,frames,(unsigned long)n);
    return 0;
}
