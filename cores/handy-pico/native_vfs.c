/* Memory-only BIOS and EEPROM VFS. No SD or OS filesystem calls. */
#include "native_vfs.h"
#include <streams/file_stream.h>
#include <file/file_path.h>
#include <compat/strl.h>
#include <string.h>

static unsigned char bios_data[512], save_data[2048];
static size_t bios_len, save_len;
struct handy_memory_file { unsigned char *data; size_t pos, len, cap; unsigned mode; int busy; };
/* Use private handles, cast to the opaque upstream RFILE type. */
static struct handy_memory_file bios_handle, save_handle;
int handy_vfs_prepare(const void *bios, size_t bn, const void *save, size_t sn)
{
    if ((bn && (!bios || bn != sizeof bios_data)) || (sn && !save) || sn > sizeof save_data) return -1;
    bios_len=bn; save_len=sn;
    if (bn) memcpy(bios_data,bios,bn);
    memset(save_data,0xff,sizeof save_data);
    if (sn) memcpy(save_data,save,sn);
    memset(&bios_handle,0,sizeof bios_handle);
    memset(&save_handle,0,sizeof save_handle);
    return 0;
}
int handy_vfs_export(void *dst, size_t cap)
{
    if (save_len > cap || (save_len && !dst)) return -1;
    if (save_len) memcpy(dst,save_data,save_len);
    return (int)save_len;
}
void filestream_vfs_init(const struct retro_vfs_interface_info *info) { (void)info; }
RFILE *filestream_open(const char *path, unsigned mode, unsigned hints)
{
    (void)hints;
    if (!path) return NULL;
    struct handy_memory_file *h;
    if (!strcmp(path,"/pico/lynxboot.img") && mode==RETRO_VFS_FILE_ACCESS_READ && bios_len) {
        h=&bios_handle; if(h->busy) return NULL;
        h->data=bios_data; h->len=bios_len; h->cap=bios_len;
    } else if (!strcmp(path,"/pico/pico-test.eeprom") &&
        (mode==RETRO_VFS_FILE_ACCESS_WRITE || (mode==RETRO_VFS_FILE_ACCESS_READ && save_len))) {
        h=&save_handle; if(h->busy) return NULL;
        h->data=save_data; h->cap=sizeof save_data;
        h->len=mode==RETRO_VFS_FILE_ACCESS_WRITE ? 0 : save_len;
    } else return NULL;
    h->pos=0; h->mode=mode; h->busy=1;
    return (RFILE *)h;
}
int64_t filestream_read(RFILE *s, void *data, int64_t n)
{
    struct handy_memory_file *h=(struct handy_memory_file *)s;
    if (!h || !h->busy || h->mode!=RETRO_VFS_FILE_ACCESS_READ || n<0 || (!data && n)) return -1;
    size_t avail=h->len-h->pos;
    size_t take=(uint64_t)n>avail ? avail : (size_t)n;
    if(take) memcpy(data,h->data+h->pos,take);
    h->pos+=take; return take;
}
int64_t filestream_write(RFILE *s, const void *data, int64_t n)
{
    struct handy_memory_file *h=(struct handy_memory_file *)s;
    if (!h || !h->busy || h!=&save_handle || h->mode!=RETRO_VFS_FILE_ACCESS_WRITE ||
        n<0 || (uint64_t)n>h->cap-h->pos || (!data && n)) return -1;
    if(n) memcpy(h->data+h->pos,data,(size_t)n);
    h->pos+=(size_t)n; if(h->pos>h->len) h->len=h->pos;
    return n;
}
int64_t filestream_seek(RFILE *s, int64_t n, int where)
{
    struct handy_memory_file *h=(struct handy_memory_file *)s;
    if(!h || !h->busy) return -1;
    int64_t base=where==RETRO_VFS_SEEK_POSITION_START ? 0 :
        where==RETRO_VFS_SEEK_POSITION_CURRENT ? (int64_t)h->pos :
        where==RETRO_VFS_SEEK_POSITION_END ? (int64_t)h->len : -1;
    if(base<0 || n < -base || n > (int64_t)h->len-base) return -1;
    h->pos=(size_t)(base+n); return h->pos;
}
int64_t filestream_tell(RFILE *s)
{ struct handy_memory_file *h=(struct handy_memory_file *)s; return h && h->busy ? (int64_t)h->pos : -1; }
int filestream_close(RFILE *s)
{
    struct handy_memory_file *h=(struct handy_memory_file *)s;
    if(!h || !h->busy) return -1;
    if(h==&save_handle && h->mode==RETRO_VFS_FILE_ACCESS_WRITE) save_len=h->len;
    h->busy=0; return 0;
}
bool path_is_valid(const char *path)
{ return path && !strcmp(path,"/pico/lynxboot.img") && bios_len==sizeof bios_data; }
const char *path_basename(const char *p)
{
    const char *last = p;
    if (!p) return NULL;
    while (*p) { if (*p == '/' || *p == '\\') last = p + 1; ++p; }
    return last;
}
char *path_remove_extension(char *p)
{
    if (!p) return NULL;
    char *dot = strrchr((char *)path_basename(p), '.');
    if (dot) *dot = 0;
    return p;
}
size_t fill_pathname_join(char *out, const char *dir, const char *path, size_t size)
{
    size_t n = strlcpy(out, dir ? dir : "", size);
    if (n && n < size && out[n-1] != '/') strlcat(out, "/", size);
    return strlcat(out, path ? path : "", size);
}
void fill_pathname_resolve_relative(char *out, const char *ref, const char *path, size_t size)
{
    if (!size) return;
    out[0] = 0;
    if (path && path[0] == '/') { strlcpy(out, path, size); return; }
    if (ref) {
        size_t n = (size_t)(path_basename(ref) - ref);
        if (n >= size) n = size - 1;
        memcpy(out, ref, n);
        out[n] = 0;
    }
    strlcat(out, path ? path : "", size);
}
