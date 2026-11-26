#include "sd_cart.h"
#include "cart_io.h"
#include <string.h>
static int read_at(void *ctx,uint32_t off,void *dst,uint32_t n)
{
    return fat_read(ctx,off,dst,n)==n?(int)n:-1;
}
int geo_sd_cart_load(fat_vol *v,const char *path,void *dst,size_t cap)
{
    if(!v||!v->mounted||!path||path[0]!='/'||!dst)return -1;
    size_t len=strlen(path);
    if(len<5||len>=640||!fat_name_eq(path+len-4,".neo"))return -1;
    for(const char *p=path+1;*p;){
        const char *e=strchr(p,'/');
        size_t n=e?(size_t)(e-p):strlen(p);
        if(!n||(n==1&&p[0]=='.')||(n==2&&p[0]=='.'&&p[1]=='.'))return -1;
        for(size_t i=0;i<n;i++)if((unsigned char)p[i]<32||p[i]=='\\'||p[i]==':')return -1;
        if(!e)break;
        p=e+1;
    }
    fat_file f;
    if(fat_open(v,path,&f))return -1;
    return geo_cart_load(read_at,&f,f.size,dst,cap);
}

/* Fixed BIOS names only; caller owns buffer and validates ZIP contents. */
int geo_sd_bios_load(fat_vol *v,int aes,void *dst,size_t cap,size_t *loaded)
{
    if(!loaded)return -1;
    *loaded=0;
    if(!v||!v->mounted||!dst||(aes!=0&&aes!=1))return -1;
    fat_file f;
    if(fat_open(v,aes?"/bios/aes.zip":"/bios/neogeo.zip",&f))return -1;
    if(f.size<22||f.size>cap||f.size>8u*1024u*1024u)return -1;
    unsigned char sig[4];
    if(fat_read(&f,0,sig,4)!=4||memcmp(sig,"PK\003\004",4))return -1;
    for(uint32_t off=0;off<f.size;){
        uint32_t n=f.size-off;
        if(n>32768u)n=32768u;
        if(fat_read(&f,off,(unsigned char *)dst+off,n)!=n)return -1;
        off+=n;
    }
    *loaded=f.size;
    return 0;
}
