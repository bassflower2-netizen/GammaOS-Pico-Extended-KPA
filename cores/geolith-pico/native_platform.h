#ifndef GEO_NATIVE_PLATFORM_H
#define GEO_NATIVE_PLATFORM_H
#include <stdint.h>
#include <sys/time.h>
/* A real wall-clock source returns Unix seconds and fractional microseconds.
 * Monotonic device uptime alone is not a wall clock. A missing or failing
 * provider must report an error; no fabricated date is returned. */
typedef int (*geo_clock_fn)(void *,int64_t *,int32_t *);
typedef void (*geo_fatal_fn)(void *,int);
void geo_native_platform_bind(geo_clock_fn,geo_fatal_fn,void *);
int geo_native_gettimeofday(struct timeval *);
int geo_native_getpid(void);
int geo_native_kill(int pid,int sig);
/* Unexpected process termination cannot return into an invalid core. The
 * device owner must supply a non-returning fatal handler before running it. */
_Noreturn void geo_native_exit(int status);
#endif
