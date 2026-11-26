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


static unsigned zip16(const unsigned char *p)
{ return (unsigned)p[0]|((unsigned)p[1]<<8); }
static uint32_t zip32(const unsigned char *p)
{ return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
/* Validate each classic ZIP central entry and its local-file bounds.
 * Reject encryption and Zip64. This does not verify decompressed bytes,
 * CRC values or the authenticity of the BIOS itself. */
int geo_sd_bios_zip_directory_valid(const void *bytes,size_t len)
{
    if(!bytes||len<22||len>8u*1024u*1024u)return -1;
    const unsigned char *b=bytes;
    if(memcmp(b,"PK\003\004",4))return -1;
    size_t first=len>65557u?len-65557u:0;
    for(size_t i=len-22;;--i){
        if(!memcmp(b+i,"PK\005\006",4)&&zip16(b+i+20)==len-i-22){
            unsigned entries=zip16(b+i+10);
            uint32_t off=zip32(b+i+16),size=zip32(b+i+12);
            if(zip16(b+i+4)||zip16(b+i+6)||!entries||
               entries==65535u||zip16(b+i+8)!=entries||
               off==UINT32_MAX||size==UINT32_MAX||size<46u||
               (size_t)off>i||(size_t)size>i-(size_t)off||
               (size_t)off+(size_t)size!=i)return -1;
            size_t pos=off;
            for(unsigned n=0;n<entries;n++){
                if(pos>i||i-pos<46||memcmp(b+pos,"PK\001\002",4))return -1;
                unsigned flags=zip16(b+pos+8),method=zip16(b+pos+10);
                unsigned name=zip16(b+pos+28),extra=zip16(b+pos+30),comment=zip16(b+pos+32);
                uint32_t compressed=zip32(b+pos+20),local=zip32(b+pos+42);
                size_t record=46u+(size_t)name+extra+comment;
                if(!name||record>i-pos||flags&1u||(method!=0&&method!=8)||
                   zip16(b+pos+34)||compressed==UINT32_MAX||
                   zip32(b+pos+24)==UINT32_MAX||local==UINT32_MAX||
                   (size_t)local>off||off-(size_t)local<30||
                   memcmp(b+local,"PK\003\004",4))return -1;
                unsigned local_name=zip16(b+local+26),local_extra=zip16(b+local+28);
                size_t data=(size_t)local+30u+local_name+local_extra;
                if(local_name!=name||data>off||compressed>off-data||
                   memcmp(b+local+30,b+pos+46,name))return -1;
                pos+=record;
            }
            return pos==i?0:-1;
        }
        if(i==first)break;
    }
    return -1;
}


/* Caller-owned, in-memory ZIP lookup for later native BIOS decoding.
 * Data is compressed when method=8; no decompression/CRC check is implied. */
int geo_sd_bios_member_find(const void *zip,size_t len,const char *name,
                            geo_zip_member *member)
{
    if(!member)return -1;
    memset(member,0,sizeof(*member));
    if(!name||!name[0]||!zip||geo_sd_bios_zip_directory_valid(zip,len))return -1;
    size_t requested=strlen(name);
    if(requested>255)return -1;
    const unsigned char *b=zip;
    size_t start=len>65557u?len-65557u:0;
    for(size_t i=len-22;;--i){
        if(!memcmp(b+i,"PK\005\006",4)&&zip16(b+i+20)==len-i-22){
            size_t pos=zip32(b+i+16);
            unsigned entries=zip16(b+i+10);
            int found=0;
            for(unsigned n=0;n<entries;n++){
                unsigned name_len=zip16(b+pos+28);
                unsigned extra=zip16(b+pos+30),comment=zip16(b+pos+32);
                if(requested==name_len&&!memcmp(b+pos+46,name,requested)){
                    if(found)return -1; /* ambiguous duplicate member */
                    uint32_t local=zip32(b+pos+42);
                    size_t data=(size_t)local+30u+zip16(b+local+26)+zip16(b+local+28);
                    member->data=b+data;
                    member->compressed_size=zip32(b+pos+20);
                    member->uncompressed_size=zip32(b+pos+24);
                    member->crc32=zip32(b+pos+16);
                    member->method=(unsigned short)zip16(b+pos+10);
                    found=1;
                }
                pos+=46u+name_len+extra+comment;
            }
            if(found)return 0;
            memset(member,0,sizeof(*member));
            return -1;
        }
        if(i==start)break;
    }
    return -1;
}


/* First native BIOS decoding stage: bounded ZIP STORED (method 0) members.
 * Deflate is intentionally not accepted until a raw-inflate adapter is linked.
 * CRC32 is computed independently of zlib to keep this stage bare-metal safe. */
int geo_sd_bios_member_copy_stored(const geo_zip_member *member,void *dst,
                                   size_t capacity,size_t *written)
{
    if(!written)return -1;
    *written=0;
    if(!member||!member->data||!dst||member->method!=0||
       !member->uncompressed_size||
       member->compressed_size!=member->uncompressed_size||
       (size_t)member->uncompressed_size>capacity)return -1;
    uint32_t crc=UINT32_MAX;
    for(uint32_t i=0;i<member->compressed_size;i++){
        crc^=member->data[i];
        for(unsigned bit=0;bit<8;bit++)
            crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
    }
    if((crc^UINT32_MAX)!=member->crc32)return -1;
    memcpy(dst,member->data,member->uncompressed_size);
    *written=member->uncompressed_size;
    return 0;
}


/* Raw DEFLATE is supplied by the platform: no host zlib dependency in the
 * bare-metal translation unit. Never report success without full input
 * consumption, exact output size and the ZIP central-directory CRC32. */
int geo_sd_bios_member_decode(const geo_zip_member *member,void *dst,size_t cap,
                              geo_zip_raw_inflater inflater,void *context,
                              size_t *written)
{
    if(!written)return -1;
    *written=0;
    if(!member||!member->data||!dst||!member->uncompressed_size||
       member->uncompressed_size>8u*1024u*1024u||
       (size_t)member->uncompressed_size>cap)return -1;
    if(member->method==0)
        return geo_sd_bios_member_copy_stored(member,dst,cap,written);
    if(member->method!=8||!inflater||!member->compressed_size)return -1;
    uint32_t consumed=0,produced=0;
    if(inflater(context,member->data,member->compressed_size,dst,
                member->uncompressed_size,&consumed,&produced)||
       consumed!=member->compressed_size||
       produced!=member->uncompressed_size)return -1;
    uint32_t crc=UINT32_MAX;
    const unsigned char *p=dst;
    for(uint32_t i=0;i<produced;i++){
        crc^=p[i];
        for(unsigned bit=0;bit<8;bit++)
            crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
    }
    if((crc^UINT32_MAX)!=member->crc32)return -1;
    *written=produced;
    return 0;
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
    if(geo_sd_bios_zip_directory_valid(dst,f.size))return -1;
    *loaded=f.size;
    return 0;
}
