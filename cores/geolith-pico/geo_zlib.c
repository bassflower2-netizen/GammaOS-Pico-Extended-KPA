#include "geo_zlib.h"
#include <zlib.h>
#include <string.h>

/* Native-compatible raw DEFLATE adapter; no host filesystem or gzFile API.
 * zlib allocates a temporary inflate state through its configured allocator.
 * The platform must initialize its bounded newlib heap before calling this.
 */
int geo_zip_zlib_raw_inflate(void *context,const unsigned char *input,
                             uint32_t input_size,void *output,
                             uint32_t output_capacity,uint32_t *consumed,
                             uint32_t *produced)
{
    (void)context;
    if(!consumed||!produced)return -1;
    *consumed=0;
    *produced=0;
    if(!input||!output||!input_size||!output_capacity||
       input_size>8u*1024u*1024u||output_capacity>8u*1024u*1024u)return -1;
    z_stream stream;
    memset(&stream,0,sizeof(stream));
    stream.next_in=(Bytef *)input;
    stream.avail_in=(uInt)input_size;
    stream.next_out=(Bytef *)output;
    stream.avail_out=(uInt)output_capacity;
    if(inflateInit2(&stream,-MAX_WBITS)!=Z_OK)return -1;
    int status=inflate(&stream,Z_FINISH);
    *consumed=(uint32_t)stream.total_in;
    *produced=(uint32_t)stream.total_out;
    int cleanup=inflateEnd(&stream);
    return status==Z_STREAM_END&&cleanup==Z_OK?0:-1;
}
