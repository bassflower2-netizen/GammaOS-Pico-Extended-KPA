/* Included only by the QEMU test runner. Linux mmap is not in the core. */
#include "pce_blob_abi.h"
extern const unsigned char probe_blob_start[],probe_blob_end[];
static const struct pce_blob_exports *flat_api;
static void flat_load(void)
{
    uint32_t hdr[5];
    size_t n=(size_t)(probe_blob_end-probe_blob_start);
    assert(n>=sizeof hdr);memcpy(hdr,probe_blob_start,sizeof hdr);
    assert(hdr[0]==PCE_BLOB_MAGIC&&hdr[1]==PCE_BLOB_VERSION);
    assert(hdr[2]>=sizeof hdr&&!(hdr[2]&3u)&&hdr[2]<hdr[3]);
    assert(hdr[3]==n&&hdr[4]>=n&&hdr[4]<=PCE_BLOB_SLOT_SIZE);
    register long r0 asm("r0")=PCE_BLOB_VMA;
    register long r1 asm("r1")=PCE_BLOB_SLOT_SIZE;
    register long r2 asm("r2")=7;
    register long r3 asm("r3")=0x32;
    register long r4 asm("r4")=-1;
    register long r5 asm("r5")=0;
    register long r7 asm("r7")=192;
    asm volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5),"r"(r7):"memory","cc");
    assert((uintptr_t)r0==PCE_BLOB_VMA);
    unsigned char *slot=(unsigned char *)r0;
    memcpy(slot,probe_blob_start,n);memset(slot+n,0,hdr[4]-n);
    pce_blob_entry_fn entry=(pce_blob_entry_fn)(slot+hdr[2]);
    flat_api=entry();
    assert(flat_api&&flat_api->magic==PCE_BLOB_MAGIC&&flat_api->version==PCE_BLOB_VERSION);
    printf("PCE flat loader: %lu bytes, span %lu\n",(unsigned long)n,(unsigned long)hdr[4]);
}
#define pce_pico_open flat_api->open
#define pce_pico_close flat_api->close
#define pce_pico_frame flat_api->frame
#define pce_pico_reset flat_api->reset
#define pce_pico_state_size flat_api->state_size
#define pce_pico_save flat_api->save
#define pce_pico_load flat_api->load
#define pce_pico_ram flat_api->ram
#define pce_pico_ram_size flat_api->ram_size
#define pce_pico_bram flat_api->bram
#define pce_pico_bram_size flat_api->bram_size
