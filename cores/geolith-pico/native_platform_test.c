#include "native_platform.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
struct source {int64_t sec;int32_t usec;int fail;int status;};
static int clock_read(void *ctx,int64_t *sec,int32_t *usec)
{
    struct source *s=ctx;
    if(s->fail)return -1;
    *sec=s->sec;*usec=s->usec;return 0;
}
static void fatal(void *ctx,int status)
{
    struct source *s=ctx;
    _Exit(status==s->status?37:38);
}
int main(void)
{
    struct timeval tv={17,23};
    assert(geo_native_gettimeofday(&tv)==-1 && errno==ENOSYS);
    assert(tv.tv_sec==17 && tv.tv_usec==23);
    assert(geo_native_gettimeofday(NULL)==-1 && errno==EFAULT);
    struct source s={1700000000,123456,0,42};
    geo_native_platform_bind(clock_read,fatal,&s);
    assert(!geo_native_gettimeofday(&tv));
    assert(tv.tv_sec==1700000000 && tv.tv_usec==123456);
    s.fail=1;assert(geo_native_gettimeofday(&tv)==-1 && errno==EIO);
    assert(tv.tv_sec==1700000000 && tv.tv_usec==123456);
    s.fail=0;s.usec=1000000;assert(geo_native_gettimeofday(&tv)==-1 && errno==EINVAL);
    s.usec=0;s.sec=-1;assert(geo_native_gettimeofday(&tv)==-1 && errno==EINVAL);
    s.sec=INT64_C(2147483648);
    if(sizeof(tv.tv_sec)==4)assert(geo_native_gettimeofday(&tv)==-1 && errno==EOVERFLOW);
    else assert(!geo_native_gettimeofday(&tv));
    assert(geo_native_getpid()==1 && !geo_native_kill(1,0));
    assert(geo_native_kill(2,0)==-1 && errno==ESRCH);
    assert(geo_native_kill(1,-1)==-1 && errno==EINVAL);
    assert(geo_native_kill(1,SIGTERM)==-1 && errno==ENOSYS);
    pid_t p=fork();assert(p>=0);
    if(!p)geo_native_exit(42);
    int status;assert(waitpid(p,&status,0)==p && WIFEXITED(status) && WEXITSTATUS(status)==37);
    geo_native_platform_bind(NULL,NULL,NULL);
    assert(geo_native_gettimeofday(&tv)==-1 && errno==ENOSYS);
    puts("Native platform clock, process and fatal callback passed");
    return 0;
}
