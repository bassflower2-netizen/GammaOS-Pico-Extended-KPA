#ifndef GEO_BLOB_ABI_H
#define GEO_BLOB_ABI_H
#include <stdint.h>
#define GEO_BLOB_MAGIC 0x314f4547u /* GEO1 */
#define GEO_BLOB_VERSION 1u
#define GEO_BLOB_VMA 0x4F000000u
#define GEO_BLOB_SLOT_SIZE 0x01000000u
/* ARM32 soft-float ABI. All sizes and return codes at the boundary are fixed
 * width, avoiding LK/newlib typedef differences. The firmware retains its
 * mounted fat_vol and its existing synchronized FAT writer. No second writer
 * lock or independent filesystem is created by the overlay. */
struct geo_blob_host {
    uint32_t size;
    void *context;
    int32_t (*log)(void *,int32_t,const void *,uint32_t);
    int32_t (*clock)(void *,int64_t *,int32_t *);
    void (*fatal)(void *,int32_t);
    int32_t (*put)(void *volume,const char *dir,const char *name,const void *,uint32_t);
    int32_t (*mkpath)(void *volume,const char *path);
};
struct geo_blob_av {
    void (*video)(const void *,uint32_t,uint32_t,uint32_t);
    uint32_t (*audio)(const int16_t *,uint32_t);
    void (*log)(const char *);
};
struct geo_blob_exports {
    uint32_t magic,version,size;
    int32_t (*init)(void *heap,uint32_t capacity,const struct geo_blob_host *);
    int32_t (*open)(void *volume,const char *path,uint32_t save_namespace,
                    void *scratch,uint32_t capacity,const struct geo_blob_av *);
    void (*close)(void);
    int32_t (*frame)(uint16_t,uint16_t);
    int32_t (*reset)(void);
    uint32_t (*state_size)(void);
    int32_t (*save)(int32_t,void *,uint32_t);
    int32_t (*load)(int32_t,void *,uint32_t);
};
typedef const struct geo_blob_exports *(*geo_blob_entry_fn)(void);
#endif
