/* PCE ABI 2 loader. HuCard session enabled; CD session wiring pending. */
#include "pce_core_loader.h"
#include "../gba/core_file.h"

int pce_core_check(fat_vol *v)
{
    fat_file f;
    unsigned h[5];
    if (!v) return CORE_FILE_NO_CARD;
    if (fat_open(v, CORE_PATH_PCE, &f)) return CORE_FILE_MISSING;
    if (f.size < sizeof h || fat_read(&f,0,h,sizeof h)!=sizeof h ||
        h[0]!=PCE_BLOB_MAGIC || h[1]!=PCE_BLOB_VERSION) return CORE_FILE_HEADER;
    if (h[2]<sizeof h || (h[2]&3u) || h[2]>=h[3] ||
        h[3]!=f.size || h[4]<h[3] || h[4]>PCE_BLOB_SLOT_SIZE) return CORE_FILE_SIZE;
    return CORE_FILE_OK;
}

const struct pce_blob_exports *pce_core_load(fat_vol *v,int *error)
{
    int e=pce_core_check(v);
    const struct pce_blob_exports *api=0;
    if(e==CORE_FILE_OK) {
        pce_blob_entry_fn init=(pce_blob_entry_fn)core_file_load(v,CORE_PATH_PCE,
            PCE_BLOB_VMA,PCE_BLOB_SLOT_SIZE,PCE_BLOB_MAGIC,
            PCE_BLOB_VERSION,PCE_BLOB_VERSION,&e);
        if(init) {
            api=init();
            /* The table and every required callback must be present. */
            uintptr_t p=(uintptr_t)api;
            if(p<PCE_BLOB_VMA || p>PCE_BLOB_VMA+PCE_BLOB_SLOT_SIZE-sizeof *api ||
               (p&3u) || api->magic!=PCE_BLOB_MAGIC || api->version!=PCE_BLOB_VERSION ||
               !api->open || !api->close || !api->frame || !api->reset ||
               !api->state_size || !api->save || !api->load || !api->ram || !api->ram_size ||
               !api->bram || !api->bram_size || !api->open_cd) {
                api=0; e=CORE_FILE_REJECTED;
            }
        }
    }
    if(error) *error=e;
    return api;
}
