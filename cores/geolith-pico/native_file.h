#ifndef GEO_NATIVE_FILE_H
#define GEO_NATIVE_FILE_H
#include "../../emu/gba/fat_ro.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
void geo_native_file_bind(fat_vol *v);
int geo_native_open(const char *path,int flags);
int geo_native_close(int fd);
ssize_t geo_native_read(int fd,void *buf,size_t count);
off_t geo_native_seek(int fd,off_t offset,int whence);
int geo_native_fstat(int fd,struct stat *st);
/* Read-only metadata by absolute FAT path; timestamps unavailable. */
int geo_native_stat(const char *path,struct stat *st);
/* Synchronous diagnostic sink: return bytes accepted, or negative on error.
 * Only descriptors 1 and 2 may write; ROM/BIOS descriptors stay read-only.
 * A missing sink is an error, never a successful discarded write. */
typedef ssize_t (*geo_native_log_sink)(void *,int,const void *,size_t);
void geo_native_console_bind(geo_native_log_sink sink,void *ctx);
ssize_t geo_native_write(int fd,const void *buf,size_t count);
int geo_native_isatty(int fd);
int geo_native_truncate(int fd,off_t length);
/* The private POSIX view does not support filesystem mutation. Dedicated
 * geo_sd_save_* calls provide whole-file persistence separately. */
int geo_native_mutation_denied(void);
#endif
