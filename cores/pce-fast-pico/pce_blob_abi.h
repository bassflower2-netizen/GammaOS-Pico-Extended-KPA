#ifndef PCE_PICO_BLOB_ABI_H
#define PCE_PICO_BLOB_ABI_H
#include "pce_adapter.h"
#include "pce_cd_io.h"
#define PCE_BLOB_MAGIC 0x31454350u /* PCE1 */
#define PCE_BLOB_VERSION 2u
#define PCE_BLOB_VMA 0x4F000000u
#define PCE_BLOB_SLOT_SIZE 0x01000000u
/* Shared emulator overlay: only one core may be resident/active.
 * The loader copies file bytes, zeroes BSS and maintains caches before entry. */
struct pce_blob_exports {
    uint32_t magic,version;
    int (*open)(const void *,size_t,const struct pce_pico_callbacks *);
    void (*close)(void);
    int (*frame)(uint16_t);
    void (*reset)(void);
    size_t (*state_size)(void);
    int (*save)(void *,size_t);
    int (*load)(const void *,size_t);
    void *(*ram)(void);
    size_t (*ram_size)(void);
    void *(*bram)(void);
    size_t (*bram_size)(void);
    int (*open_cd)(const char *,const struct pce_cd_io *,const struct pce_pico_callbacks *);
};
typedef const struct pce_blob_exports *(*pce_blob_entry_fn)(void);
#endif
