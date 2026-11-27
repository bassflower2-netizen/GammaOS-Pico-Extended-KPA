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
#endif
