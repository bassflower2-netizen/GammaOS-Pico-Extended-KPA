#ifndef HANDY_MEMORY_VFS_H
#define HANDY_MEMORY_VFS_H
#include <stddef.h>
int handy_vfs_prepare(const void *bios, size_t bios_size, const void *save, size_t save_size);
int handy_vfs_export(void *dst, size_t cap);
#endif
