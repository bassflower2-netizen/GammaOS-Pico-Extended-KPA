#include "geo_video.h"
int geo_video_rgb565(uint16_t *dst,uint32_t capacity,const void *src,
                     uint32_t w,uint32_t h,uint32_t pitch)
{
    if(!dst||!w||!h||w>GEO_VIDEO_MAX_W||h>GEO_VIDEO_MAX_H||capacity<w*h)return -1;
    if(!src)return 1;
    if(pitch<w*4u||pitch>4096u)return -1;
    const unsigned char *p=src;
    for(uint32_t y=0;y<h;y++){
        const unsigned char *line=p+y*pitch;
        for(uint32_t x=0;x<w;x++){
            /* ARM little-endian XRGB8888 stores B,G,R,X in memory. */
            const unsigned char *q=line+x*4u;
            dst[y*w+x]=(uint16_t)(((uint16_t)(q[2]&0xf8u)<<8)|
                                  ((uint16_t)(q[1]&0xfcu)<<3)|(q[0]>>3));
        }
    }
    return 0;
}
