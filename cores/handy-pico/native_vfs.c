/* Experimental memory-only VFS. No SD or OS filesystem access.
 * Explicit BIOS/EEPROM buffers will replace this stub before LK integration. */
#include <streams/file_stream.h>
#include <file/file_path.h>
#include <compat/strl.h>
#include <string.h>

void filestream_vfs_init(const struct retro_vfs_interface_info *info) { (void)info; }
RFILE *filestream_open(const char *path, unsigned mode, unsigned hints)
{ (void)path; (void)mode; (void)hints; return NULL; }
int64_t filestream_read(RFILE *s, void *data, int64_t n)
{ (void)s; (void)data; (void)n; return -1; }
int64_t filestream_write(RFILE *s, const void *data, int64_t n)
{ (void)s; (void)data; (void)n; return -1; }
int64_t filestream_seek(RFILE *s, int64_t n, int where)
{ (void)s; (void)n; (void)where; return -1; }
int64_t filestream_tell(RFILE *s) { (void)s; return -1; }
int filestream_close(RFILE *s) { (void)s; return -1; }
bool path_is_valid(const char *path) { (void)path; return false; }
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
