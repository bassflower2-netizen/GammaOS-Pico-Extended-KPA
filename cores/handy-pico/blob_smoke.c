/* QEMU Linux test runner, NOT part of the blob. Emulates core_file_load's
 * header validation, copy, BSS zero, then dispatches through exported pointers. */
#include "handy_blob_abi.h"
#include "libretro.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern const unsigned char probe_blob_start[], probe_blob_end[];
static unsigned videos, audio_frames;
static void video(const uint16_t *p, unsigned w, unsigned h, size_t pitch)
{
    assert(w && h && w <= 160 && h <= 160);
    if (p) assert(pitch >= 2 * w);
    ++videos;
}
static size_t audio(const int16_t *p, size_t n)
{
    if (n) assert(p);
    audio_frames += n;
    return n;
}
static void *map_slot(void)
{
    register long r0 asm("r0") = HANDY_BLOB_VMA;
    register long r1 asm("r1") = HANDY_BLOB_SLOT_SIZE;
    register long r2 asm("r2") = 7;    /* PROT_READ|WRITE|EXEC */
    register long r3 asm("r3") = 0x32; /* MAP_PRIVATE|FIXED|ANONYMOUS */
    register long r4 asm("r4") = -1;
    register long r5 asm("r5") = 0;
    register long r7 asm("r7") = 192; /* mmap2 */
    asm volatile("svc 0" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5),"r"(r7) : "memory","cc");
    return (void *)r0;
}
int main(void)
{
    static const uint8_t rom[] = {
        0x80,0x08,0x20,0x0a,0,0x0d,'B','S','9','3',0x4c,0x0a,0x20
    };
    size_t len = (size_t)(probe_blob_end-probe_blob_start);
    uint32_t hdr[5];
    assert(len >= sizeof hdr);
    memcpy(hdr, probe_blob_start, sizeof hdr);
    assert(hdr[0] == HANDY_BLOB_MAGIC && hdr[1] == HANDY_BLOB_VERSION);
    assert(hdr[2] >= sizeof hdr && hdr[2] < hdr[3] && hdr[3] == len);
    assert(hdr[4] >= hdr[3] && hdr[4] <= HANDY_BLOB_SLOT_SIZE);
    unsigned char *slot = map_slot();
    assert((uintptr_t)slot == HANDY_BLOB_VMA);
    memcpy(slot, probe_blob_start, len);
    memset(slot+len, 0, hdr[4]-len);
    /* QEMU user mode sees freshly mapped code; on real LK core_file_load
     * performs privileged cache maintenance before invoking the entry. */
    handy_blob_entry_fn init = (handy_blob_entry_fn)(slot+hdr[2]);
    const struct handy_blob_exports *api = init();
    assert(api && api->magic == HANDY_BLOB_MAGIC && api->version == HANDY_BLOB_VERSION);
    struct handy_pico_callbacks cb = {video,audio};
    for (unsigned session=0; session<2; ++session) {
        assert(api->open(rom, sizeof rom, &cb) == 0);
        assert(api->ram_size() == 65536);
        uint8_t *ram = api->ram();
        assert(ram && !memcmp(ram+0x2000,rom,sizeof rom));
        for (unsigned i=0;i<8;++i) assert(api->frame(i&1 ? 1u<<RETRO_DEVICE_ID_JOYPAD_A:0) == 0);
        api->reset();
        assert(api->frame(0)==0);
        size_t n=api->state_size();
        assert(n>65536 && n<16u*1024u*1024u);
        void *state=malloc(n);
        assert(state);
        ram[0x1000]=0x42;
        assert(api->save(state,n)==0);
        ram[0x1000]=0x99;
        assert(api->load(state,n)==0 && ram[0x1000]==0x42);
        free(state);
        api->close();
        assert(!api->ram() && !api->state_size());
    }
    assert(videos>=18 && audio_frames>0);
    /* Synthetic cartridge fixture: test loading/header validation and the entire
     * 2048-byte EEPROM (including the second half), not commercial game boot. */
    static uint8_t cart[64+65536], saved[2048], restored[2048], badbios[512];
    memcpy(cart,"LYNX",4); cart[5]=1; cart[8]=1;
    memcpy(cart+10,"Pico test",10); memcpy(cart+42,"Test",5);
    cart[60]=5; cart[64]=1;
    for(unsigned i=0;i<sizeof saved;i++) saved[i]=(uint8_t)(i^(i>>8));
    assert(api->open_ex(cart,sizeof cart-1,&cb,NULL,0,NULL,0)<0);
    assert(api->open_ex(cart,sizeof cart,&cb,NULL,0,saved,1024)<0);
    assert(api->open_ex(cart,sizeof cart,&cb,badbios,511,NULL,0)<0);
    assert(api->open_ex(cart,sizeof cart,&cb,badbios,sizeof badbios,saved,sizeof saved)==0);
    assert(!api->bios_valid()); /* fake BIOS is rejected, fallback selected */
    assert(api->eeprom(restored,1024)<0);
    assert(api->eeprom(restored,sizeof restored)==sizeof restored);
    assert(!memcmp(saved,restored,sizeof saved));
    api->close();
    assert(api->open_ex(cart,sizeof cart,&cb,NULL,0,restored,sizeof restored)==0);
    memset(saved,0,sizeof saved);
    assert(api->eeprom(saved,sizeof saved)==sizeof saved);
    assert(!memcmp(saved,restored,sizeof saved));
    api->close();
    printf("Cartridge validation, BIOS rejection and 2048-byte EEPROM round-trip passed\n");
    printf("Flat blob executed: %lu bytes, span %lu, %u video callbacks, %u stereo samples\n",
        (unsigned long)len,(unsigned long)hdr[4],videos,audio_frames);
    return 0;
}
