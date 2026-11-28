/* Native Geolith ABI 2: validate exact file bounds and the export table. */
#include "geo_core_loader.h"
#include "../gba/core_file.h"
static int check(fat_vol *v,uint32_t h[5])
{
    fat_file f;
    if(!v||!v->mounted)return CORE_FILE_NO_CARD;
    if(fat_open(v,CORE_PATH_GEO,&f))return CORE_FILE_MISSING;
    if(f.size<5u*sizeof *h||fat_read(&f,0,h,5u*sizeof *h)!=5u*sizeof *h||
       h[0]!=GEO_BLOB_MAGIC||h[1]!=GEO_BLOB_VERSION)return CORE_FILE_HEADER;
    if(h[2]<5u*sizeof *h||(h[2]&3u)||h[2]>=h[3]||
       h[3]!=f.size||h[4]<h[3]||h[4]>GEO_BLOB_SLOT_SIZE)return CORE_FILE_SIZE;
    return CORE_FILE_OK;
}
int geo_core_check(fat_vol *v){uint32_t h[5];return check(v,h);}
static int code_pointer(uintptr_t p,uintptr_t end)
{return p>=GEO_BLOB_VMA+20u&&p<end&&!(p&3u);}
const struct geo_blob_exports *geo_core_load(fat_vol *v,int *error)
{
    uint32_t h[5];int e=check(v,h);
    const struct geo_blob_exports *api=0;
    if(e==CORE_FILE_OK){
        geo_blob_entry_fn entry=(geo_blob_entry_fn)core_file_load(v,CORE_PATH_GEO,
            GEO_BLOB_VMA,GEO_BLOB_SLOT_SIZE,GEO_BLOB_MAGIC,
            GEO_BLOB_VERSION,GEO_BLOB_VERSION,&e);
        if(entry){
            uintptr_t end=GEO_BLOB_VMA+h[3];
            if((uintptr_t)entry!=GEO_BLOB_VMA+h[2])e=CORE_FILE_REJECTED;
            else{
                api=entry();uintptr_t p=(uintptr_t)api;
                if(p<GEO_BLOB_VMA+20u||p>end-sizeof *api||(p&3u)||
                   api->magic!=GEO_BLOB_MAGIC||api->version!=GEO_BLOB_VERSION||
                   api->size!=sizeof *api||
                   !code_pointer((uintptr_t)api->init,end)||
                   !code_pointer((uintptr_t)api->open,end)||
                   !code_pointer((uintptr_t)api->close,end)||
                   !code_pointer((uintptr_t)api->frame,end)||
                   !code_pointer((uintptr_t)api->reset,end)||
                   !code_pointer((uintptr_t)api->state_size,end)||
                   !code_pointer((uintptr_t)api->save,end)||
                   !code_pointer((uintptr_t)api->load,end)||
                   !code_pointer((uintptr_t)api->timing,end))e=CORE_FILE_REJECTED;
            }
        }
    }
    if(error)*error=e;
    return e==CORE_FILE_OK?api:0;
}
