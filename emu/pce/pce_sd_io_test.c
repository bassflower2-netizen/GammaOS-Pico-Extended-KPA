/* Real FAT image test. All fixture files are synthetic. */
#include "pce_sd_io.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static FILE *img;
static int fail_reads;
static unsigned read_blocks(void *ctx,uint32_t lba,uint32_t count,void *p)
{
    (void)ctx;
    if(fail_reads||fseek(img,(long)lba*512,SEEK_SET))return 0;
    return (unsigned)fread(p,512,count,img);
}
int main(int argc,char **argv)
{
    assert(argc==2);
    img=fopen(argv[1],"rb");assert(img);
    fat_vol v;assert(fat_mount(&v,read_blocks,0)==0);
    uint32_t n=0;
    assert(pce_sd_io.open("/pico/disc/track.bin",&n)<0);
    assert(pce_sd_io_begin(&v,"/roms/../bios")<0);
    assert(pce_sd_io_begin(&v,"/roms/pcecd")==0);
    assert(pce_sd_io_begin(&v,"/roms/pcecd")<0);
    assert(pce_sd_io.open("/pico/disc/../syscard3.pce",&n)<0);
    assert(pce_sd_io.open("/pico/disc/nested/track.bin",&n)<0);
    assert(pce_sd_io.open("/pico/disc/missing.bin",&n)<0);
    assert(pce_sd_io.open("/pico/disc/track.txt",&n)<0);
    int bios=pce_sd_io.open("/pico/bios/syscard3.pce",&n);
    assert(bios>=0&&n==262144);
    unsigned char data[4096];
    assert(pce_sd_io.read_at(bios,0,data,32)==32);
    for(unsigned i=0;i<32;i++)assert(data[i]==0xa5);
    int cue=pce_sd_io.open("/pico/disc/Test Disc.cue",&n);
    assert(cue>=0&&n>0&&n<sizeof data);
    assert(pce_sd_io.read_at(cue,0,data,n)==(int)n);
    assert(!memcmp(data,"FILE ",5));
    int track=pce_sd_io.open("/pico/disc/track.bin",&n);
    assert(track>=0&&n==524288);
    const uint32_t offsets[]={0,511,4093,409600,17,520192};
    for(unsigned j=0;j<sizeof offsets/sizeof offsets[0];j++){
        uint32_t at=offsets[j];
        assert(pce_sd_io.read_at(track,at,data,sizeof data)==sizeof data);
        for(unsigned i=0;i<sizeof data;i++)
            assert(data[i]==(unsigned char)(((at+i)>>11)^((at+i)&255)));
    }
    assert(pce_sd_io.read_at(track,n-1,data,2)<0);
    assert(pce_sd_io.read_at(track,0xffffffffu,data,8)<0);
    fail_reads=1;
    assert(pce_sd_io.read_at(track,0,data,512)<0);
    fail_reads=0;
    pce_sd_io.close(track);
    assert(pce_sd_io.read_at(track,0,data,1)<0);
    pce_sd_io.close(cue);pce_sd_io.close(bios);
    int ids[104];
    for(unsigned i=0;i<104;i++){
        ids[i]=pce_sd_io.open("/pico/disc/track.bin",&n);assert(ids[i]>=0);
    }
    assert(pce_sd_io.open("/pico/disc/track.bin",&n)<0);
    pce_sd_io_end();
    assert(pce_sd_io.read_at(ids[0],0,data,1)<0);
    assert(pce_sd_io_begin(&v,"/roms/pcecd")==0);
    assert(pce_sd_io.open("/pico/disc/track.bin",&n)>=0);
    pce_sd_io_end();fclose(img);
    puts("PCE SD CD bridge: real FAT fixture, random reads, path bounds, I/O failures, 104 handles and session cleanup passed");
    return 0;
}
