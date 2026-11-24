/* Experimental LK adapter. Cartridge dispatch remains disabled pending the
 * Lynx session runner and menu work. */
#include "handy_core_loader.h"
#include "../gba/core_file.h"

int handy_core_check(fat_vol *v)
{
    fat_file f;
    unsigned h[5];
    if (!v) return CORE_FILE_NO_CARD;
    if (fat_open(v, CORE_PATH_HANDY, &f)) return CORE_FILE_MISSING;
    if (f.size < sizeof h || fat_read(&f,0,h,sizeof h)!=sizeof h ||
        h[0]!=HANDY_BLOB_MAGIC || h[1]!=HANDY_BLOB_VERSION) return CORE_FILE_HEADER;
    if (h[2]<sizeof h || (h[2]&3u) || h[2]>=h[3] ||
        h[3]!=f.size || h[4]<h[3] || h[4]>HANDY_BLOB_SLOT_SIZE) return CORE_FILE_SIZE;
    return CORE_FILE_OK;
}

const struct handy_blob_exports *handy_core_load(fat_vol *v,int *error)
{
    int e=handy_core_check(v);
    const struct handy_blob_exports *api=0;
    if(e==CORE_FILE_OK) {
        handy_blob_entry_fn init=(handy_blob_entry_fn)core_file_load(v,CORE_PATH_HANDY,
            HANDY_BLOB_VMA,HANDY_BLOB_SLOT_SIZE,HANDY_BLOB_MAGIC,
            HANDY_BLOB_VERSION,HANDY_BLOB_VERSION,&e);
        if(init) {
            api=init();
            /* The table and every required callback must be present. */
            uintptr_t p=(uintptr_t)api;
            if(p<HANDY_BLOB_VMA || p>HANDY_BLOB_VMA+HANDY_BLOB_SLOT_SIZE-sizeof *api ||
               (p&3u) || api->magic!=HANDY_BLOB_MAGIC || api->version!=HANDY_BLOB_VERSION ||
               !api->open || !api->close || !api->frame || !api->reset ||
               !api->state_size || !api->save || !api->load || !api->ram || !api->ram_size ||
               !api->open_ex || !api->eeprom || !api->bios_valid) {
                api=0; e=CORE_FILE_REJECTED;
            }
        }
    }
    if(error) *error=e;
    return api;
}
