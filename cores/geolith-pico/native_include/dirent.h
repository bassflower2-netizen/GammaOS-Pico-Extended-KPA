/* Native compile-stage directory ABI. No POSIX filesystem is supplied.
 * Keep directory calls unresolved until a real platform adapter is linked. */
#ifndef GEO_NATIVE_DIRENT_H
#define GEO_NATIVE_DIRENT_H
typedef struct geo_native_dir DIR;
struct dirent { char d_name[256]; unsigned char d_type; };
#define DT_UNKNOWN 0
#define DT_DIR 4
#define DT_REG 8
DIR *opendir(const char *path);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dir);
#endif
