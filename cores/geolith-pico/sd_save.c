/* Save storage is deliberately separate from the read-only ROM/BIOS descriptors. */
#include "sd_save.h"
#include "../../emu/gba/fat_wr.h"
#include <string.h>
static void hex8(char *out,uint32_t x)
{
    static const char digits[]="0123456789ABCDEF";
    for(int i=7;i>=0;i--){out[i]=digits[x&15u];x>>=4;}
}
static int paths(uint64_t key,unsigned aes,int slot,char dir[32],char name[13])
{
    if(aes>1 || slot < -1 || slot>9)return -1;
    strcpy(dir,slot<0?"/saves/neo/":"/states/neo/");
    strcat(dir,aes?"AES/":"MVS/");
    size_t n=strlen(dir);
    hex8(dir+n,(uint32_t)(key>>32));dir[n+8]=0;
    hex8(name,(uint32_t)key);
    if(slot<0)memcpy(name+8,".SAV",5);
    else{memcpy(name+8,".ST0",5);name[11]=(char)('0'+slot);}
    return 0;
}
int geo_sd_save_put(fat_vol *v,uint64_t key,unsigned aes,int slot,const void *buf,uint32_t len)
{
    char dir[32],name[13];
    if(!v || !v->mounted || !v->wr || !buf || !len || paths(key,aes,slot,dir,name))return -1;
    if(fat_wr_mkpath(v,dir) || fat_wr_put(v,dir,name,buf,len))return -3;
    return 0;
}
int geo_sd_save_get(fat_vol *v,uint64_t key,unsigned aes,int slot,void *buf,uint32_t len)
{
    char dir[32],name[13],path[48];fat_file f;
    if(!v || !v->mounted || !buf || !len || paths(key,aes,slot,dir,name))return -1;
    strcpy(path,dir);strcat(path,"/");strcat(path,name);
    if(fat_open(v,path,&f) || f.size!=len)return -2;
    /* Full exact-size read, no partial state accepted. */
    return fat_read(&f,0,buf,len)==len?0:-3;
}
