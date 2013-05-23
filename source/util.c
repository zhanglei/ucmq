#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <math.h>

#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "util.h"
#include "atomic.h"
#include "internal.h"

#define NS_2_S(x)  ((x) * 0.000000001)
#define S_2_NS(x)  ((x) * 1000000000LL)
#define NS_2_MS(x) ((x) * 0.000001)
#define MS_2_NS(x) (((x) % 1000) * 1000000LL)
#define S_2_MS(x)  ((x) * 1000)
#define MS_2_S(x)  ((x) / 1000)

struct timespec get_time_tick(void)
{
    struct timespec ts;
    long ret;

    ts.tv_sec = 0;
    ts.tv_nsec = 0;

    ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    assert(ret == 0);

    return ts;
}

double get_time_diff_sec(const struct timespec* start, const struct timespec* end)
{
    assert(start);
    assert(end);

    return (NS_2_S(end->tv_nsec - start->tv_nsec)) + end->tv_sec - start->tv_sec;
}

int64_t get_time_diff_nsec(const struct timespec* start, const struct timespec* end)
{
    assert(start);
    assert(end);

    return (S_2_NS(end->tv_sec - start->tv_sec)) + end->tv_nsec - start->tv_nsec;
}

struct timespec get_abs_time(int32_t time_affter)
{
    struct timespec ts = get_time_tick();

    ts.tv_sec  += MS_2_S(time_affter);
    ts.tv_nsec += MS_2_NS(time_affter);

    return ts;
}

int32_t get_rel_time(const struct timespec* abs_time)
{
    struct timespec now = get_time_tick();

    assert(abs_time);

    return S_2_MS(abs_time->tv_sec - now.tv_sec) + NS_2_MS(abs_time->tv_nsec - now.tv_nsec);
}

int32_t set_rand(void)
{
    /* this fd will not close */
    static volatile int fd = -1;
    int32_t value          = 0;
    int tmp                = 0;

    if(ATOMIC_GET(fd) == -1)
    {
        tmp = open("/dev/urandom", O_RDONLY);
        if(tmp == -1)
        {
            return mq_errno();
        }

        if(ATOMIC_CAS(fd, -1, tmp) != -1)
        {
            close(tmp);
        }
    }
    return (read(fd, &value, sizeof(value)) == sizeof(value)) ? (value & 0x7FFFFFFF) : mq_errno();
}

int do_mkdir(const char* dir, mode_t mode)
{
    assert(dir);

    const char* p = dir;
    int   len = 0;
    char  tmp[strlen(dir) + 1];

    while((p = strchr(p, '/')) != NULL)
    {
        len = p - dir;
        if(len > 0)
        {
            memcpy(tmp, dir, len);
            tmp[len] = '\0';

            if((mkdir(tmp, mode) < 0) && (errno != EEXIST))
            {
                return mq_errno();
            }
        }
        p += 1;
    }

    if((mkdir(dir, mode) < 0) && (errno != EEXIST))
    {
        return mq_errno();
    }

    return 0;
}

int do_gettid(void)
{
    return syscall(__NR_gettid);
}

int set_sig_mask(enum OPT opt, int count, ...)
{
    va_list ap;
    int i = 0;
    int how = 0;

    sigset_t block_set;
    sigemptyset(&block_set);

    va_start(ap, count);
    for(i = 0; i < count; i++)
    {
        sigaddset(&block_set, va_arg(ap, int));
    }
    va_end(ap);

    switch(opt)
    {
        case ON:
            how = SIG_BLOCK;
            break;
        case OFF:
            how = SIG_UNBLOCK;
            break;
        default:
            return MQ_EINVAL;
    }

    return (pthread_sigmask(how, &block_set, NULL) == 0) ? 0 : mq_errno();
}

int do_sleep(int32_t timeout)
{
    struct timespec ts[2];
    struct timespec* pts[2];
    int ret = 0;
    struct timespec* tmp = NULL;

    if(timeout < 0)
    {
        return MQ_EINVAL;
    }

    pts[0] = &ts[0];
    pts[1] = &ts[1];

    pts[0]->tv_sec = MS_2_S(timeout);
    pts[0]->tv_nsec = MS_2_NS(timeout);

    while(((ret = nanosleep(pts[0], pts[1])) != 0) && (errno == EINTR))
    {
        tmp = pts[0];
        pts[0] = pts[1];
        pts[1] = tmp;
    }

    return (ret == 0) ? 0 : mq_errno();
}

const char* get_self_path(void)
{
    static char buf[PATH_MAX] = {0};
    int ret = -1;

    if(buf[0] == '\0')
    {
        ret = readlink("/proc/self/exe", buf, sizeof(buf));
        if(ret < 0 || ret >= (signed)sizeof(buf))
        {
            return NULL;
        }
        buf[ret] = '\0';
    }

    return buf;
}
