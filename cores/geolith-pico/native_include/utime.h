#ifndef GEOLITH_NATIVE_UTIME_H
#define GEOLITH_NATIVE_UTIME_H
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
struct utimbuf { time_t actime; time_t modtime; };
int utime(const char *path, const struct utimbuf *times);
#ifdef __cplusplus
}
#endif
#endif
