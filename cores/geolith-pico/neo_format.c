#include "neo_format.h"
static uint32_t le32(const uint8_t *p)
{ return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
int pico_neo_validate_header(const void *data,size_t available,size_t size)
{
    const uint8_t *p=data;
    if(!p||available<PICO_NEO_HEADER||size<PICO_NEO_HEADER||size>PICO_NEO_MAX)return -1;
    if(p[0]!='N'||p[1]!='E'||p[2]!='O'||p[3]!=1)return -1;
    uint64_t total=PICO_NEO_HEADER;
    uint32_t n[6];
    for(unsigned i=0;i<6;i++){n[i]=le32(p+4+i*4);total+=n[i];}
    /* First-stage conservative cartridge layout. V2 may share V1. */
    if(n[0]<0x100000u||n[0]>0x900000u||(n[0]&1u)||
       n[1]<0x10000u||n[1]>0x80000u||
       n[2]<0x10000u||n[2]>0x80000u||
       !n[3]||n[3]>0x1000000u||n[4]>0x1000000u||
       n[5]<0x100000u||n[5]>0x4000000u||n[5]%128u)return -1;
    return total==size?0:-1;
}

int pico_neo_validate(const void *data,size_t size)
{ return pico_neo_validate_header(data,size,size); }
