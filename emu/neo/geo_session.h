#ifndef LK_GEO_SESSION_H
#define LK_GEO_SESSION_H
#include "geo_core_loader.h"
#include "geo_video.h"
/* Caller owns the shared overlay and all three disjoint buffers exclusively
 * until end() returns. All calls and synchronous callbacks on one thread.
 * No allocation or automatic selection of physical addresses here. */
#define GEO_SESSION_HEAP_MIN (64u*1024u*1024u)
#define GEO_SESSION_SCRATCH_MIN (4u*1024u*1024u)
struct geo_session_memory {
    void *heap; uint32_t heap_size;
    void *scratch; uint32_t scratch_size;
    uint16_t *video; uint32_t video_pixels;
};
/* Partition an already reserved, exclusively owned arena. No reservation
 * or ownership transfer is performed. On failure output is unchanged. */
int geo_session_partition(void *arena,uint32_t bytes,struct geo_session_memory *result);
struct geo_session_output {
    void (*present)(const uint16_t *,uint32_t,uint32_t);
    void (*audio)(const int16_t *,uint32_t,uint32_t);
    void (*log)(const char *);
};
int geo_session_begin(fat_vol *,const char *,uint32_t,
                      const struct geo_session_memory *,
                      const struct geo_blob_host *,
                      const struct geo_session_output *);
/* Start using a platform-reserved exclusive arena. Caller retains ownership
 * until end returns; no fallback to unreserved physical memory. */
int geo_session_begin_arena(fat_vol *v,const char *path,uint32_t mode,
                            void *arena,uint32_t bytes,
                            const struct geo_blob_host *host,
                            const struct geo_session_output *o);
int geo_session_frame(uint16_t,uint16_t);
int geo_session_reset(void);
int geo_session_save(int32_t);
int geo_session_load(int32_t);
int geo_session_timing(uint32_t *,uint32_t *);
void geo_session_end(void);
#endif
