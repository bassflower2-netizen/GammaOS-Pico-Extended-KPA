#ifndef HANDY_PICO_BLOB_ABI_H
#define HANDY_PICO_BLOB_ABI_H
#include "handy_adapter.h"
#define HANDY_BLOB_MAGIC 0x31584e4cu /* LNX1 */
#define HANDY_BLOB_VERSION 2u
#define HANDY_BLOB_VMA 0x4F000000u
#define HANDY_BLOB_SLOT_SIZE 0x01000000u
/* Experimental overlay of the armSNES slot. Only one resident emulator may
 * execute at a time. LK integration must verify lifetime and memory ownership.
 * The loader zeroes the BSS before calling the entry exactly once. */
struct handy_blob_exports {
    uint32_t magic, version;
    int (*open)(const void *, size_t, const struct handy_pico_callbacks *);
    void (*close)(void);
    int (*frame)(uint16_t);
    void (*reset)(void);
    size_t (*state_size)(void);
    int (*save)(void *, size_t);
    int (*load)(const void *, size_t);
    void *(*ram)(void);
    size_t (*ram_size)(void);
    /* V2: copied optional BIOS (512 bytes) and EEPROM from the caller.
     * Export EEPROM before closing; the caller persists it to SD. */
    int (*open_ex)(const void *,size_t,const struct handy_pico_callbacks *,
                   const void *,size_t,const void *,size_t);
    int (*eeprom)(void *,size_t);
    int (*bios_valid)(void);
};
typedef const struct handy_blob_exports *(*handy_blob_entry_fn)(void);
#endif
