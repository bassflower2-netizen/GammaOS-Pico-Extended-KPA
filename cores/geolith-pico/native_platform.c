#include "native_platform.h"
#include <errno.h>
#include <stddef.h>
#include <time.h>
static geo_clock_fn clock_source;
static geo_fatal_fn fatal_handler;
static void *platform_context;
void geo_native_platform_bind(geo_clock_fn clock,geo_fatal_fn fatal,void *ctx)
{
    clock_source=clock;fatal_handler=fatal;
    platform_context=(clock||fatal)?ctx:NULL;
}
int geo_native_gettimeofday(struct timeval *tv)
{
    if(!tv){errno=EFAULT;return -1;}
    if(!clock_source){errno=ENOSYS;return -1;}
    int64_t sec=-1;int32_t usec=-1;
    if(clock_source(platform_context,&sec,&usec)){errno=EIO;return -1;}
    if(sec<0||usec<0||usec>=1000000){errno=EINVAL;return -1;}
    time_t seconds=(time_t)sec;
    if((int64_t)seconds!=sec){errno=EOVERFLOW;return -1;}
    tv->tv_sec=seconds;tv->tv_usec=usec;
    return 0;
}
/* This private runtime has exactly one logical process, no signal delivery. */
int geo_native_getpid(void){return 1;}
int geo_native_kill(int pid,int sig)
{
    if(sig<0){errno=EINVAL;return -1;}
    if(pid!=geo_native_getpid()){errno=ESRCH;return -1;}
    if(sig==0)return 0;
    errno=ENOSYS;return -1;
}
_Noreturn void geo_native_exit(int status)
{
    if(fatal_handler)fatal_handler(platform_context,status);
    /* A returning/missing fatal handler is a programming error. Fail closed
     * instead of continuing the terminated core or silently rebooting. */
    __builtin_trap();
}
#ifdef GEO_NEWLIB_PLATFORM
int _gettimeofday(struct timeval *tv,void *tz)
{
    if(tz){errno=ENOTSUP;return -1;}
    return geo_native_gettimeofday(tv);
}
int _getpid(void){return geo_native_getpid();}
int _kill(int pid,int sig){return geo_native_kill(pid,sig);}
_Noreturn void _exit(int status){geo_native_exit(status);}
/* No CRT fini section in this private C runtime. Session cleanup is explicit
 * in geo_pico_close; this hook never replaces that lifecycle. */
void _fini(void){}
#endif
