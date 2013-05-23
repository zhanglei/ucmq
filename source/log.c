#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>

#include "log.h"
#include "util.h"
#include "atomic.h"
#include "internal.h"

/******************************************************************************************************/

#define LOG_NEED_INIT   0
#define LOG_INIT_ING    1
#define LOG_HAS_INIT    2

/******************************************************************************************************/

static log_config_t g_config;

static volatile int     g_has_init = 0;
static volatile int     g_has_fork = 0;
static volatile pid_t   g_pid      = 0;

typedef union time_info
{
    uint64_t        value;
    struct
    {
        uint16_t    day;
        uint16_t    msec;
        uint8_t     hour;
        uint8_t     min;
        uint8_t     sec;
    };
}time_info;

/********************************************************************************************/

/*********************************************************************************************
Function Name:  log_write_impl
Description  :  
Inputs       :  const char* module             :
                const char* file               :
                int line                       :
                uint16_t level                 :
                const char* level_str          :
                const char* format             :
                va_list ap                     :
Outputs      :  return value                   : 
ErrorCodes   :
History      :
----------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
*********************************************************************************************/
static void log_write_impl(const char* module, 
                              const char* file, 
                              int line, 
                              uint16_t level, 
                              const char* level_str, 
                              const char* format, 
                              va_list ap);

/*********************************************************************************************
Function Name:  write_2_file
Description  :  
Inputs       :  int fd                         :
                const char* buf                :
                int size                       :
Outputs      :  return value                   :  None.
ErrorCodes   :
History      :
----------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
*********************************************************************************************/
static inline void write_2_file(int fd, const char* buf, int size);

/*********************************************************************************************
Function Name:  open_log
Description  :  
Inputs       :  volatile int* fd               :
Outputs      :  return value                   :  
ErrorCodes   :
History      :
----------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
*********************************************************************************************/
static inline int open_log(volatile int* fd);

/*********************************************************************************************
Function Name:  get_time
Description  :  
Inputs       :  None.
Outputs      :  return value                   :  
ErrorCodes   :
History      :
----------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
*********************************************************************************************/
static time_info get_time(void);

/********************************************************************************************/

void log_init_config(log_config_t* config)
{
    struct timeval tv;
    struct timezone tz;

    strcpy(config->log_path, "./");
    strcpy(config->log_file, "log");
    config->log_level = LOG_TRACE;

    config->time_zone = (gettimeofday(&tv, &tz) == 0) ? -tz.tz_minuteswest/60 : 0;

    return;
}

int log_init(const log_config_t* config)
{
    switch(ATOMIC_CAS(g_has_init, LOG_NEED_INIT, LOG_INIT_ING))
    {
        case LOG_NEED_INIT:
            assert(config);
            memcpy(&g_config, config, sizeof(g_config));
            if(do_mkdir(g_config.log_path, 0770) < 0)
            {
                fprintf(stderr, 
                    "create log path : %s error : %d - %s\n", 
                    g_config.log_path, mq_errno(), mq_last_error());
                ATOMIC_SET(g_has_init, LOG_NEED_INIT);
                return MQ_ENOENT;
            }
            ATOMIC_SET(g_has_init, LOG_HAS_INIT);
        case LOG_INIT_ING:
            while(ATOMIC_GET(g_has_init) != LOG_HAS_INIT)
            {
                do_sleep(1);
            }
        case LOG_HAS_INIT:
            {
                pid_t last = g_pid;
                pid_t cur = do_getpid();
                if((last != -1) && (last != cur))
                {
                    ATOMIC_SET(g_pid, cur);
                    ATOMIC_SET(g_has_fork, 1);
                }
            }
            break;
        default:
            fprintf(stderr, "find unknown state %d\n", g_has_init);
            return -1;
    }

    return 0;
}

void log_write(const char* module, 
                  const char* file, 
                  int line, 
                  uint16_t level, 
                  const char* format, 
                  ...)
{
    va_list ap;

    va_start(ap, format);
    log_writev(module, file, line, level, format, ap);
    va_end(ap);

    return;
}

void log_writev(const char* module, 
                   const char* file, 
                   int line, 
                   uint16_t level, 
                   const char* format, 
                   va_list ap)
{
    char level_buf[8];
    const char* level_str = level_buf;

    switch(level)
    {
        case LOG_TRACE:
            level_str = "TRACE";
            break;
        case LOG_DEBUG:
            level_str = "DEBUG";
            break;
        case LOG_INFO:
            level_str = "INFO ";
            break;
        case LOG_WARN:
            level_str = "WARN ";
            break;
        case LOG_ERROR:
            level_str = "ERROR";
            break;
        case LOG_FATAL:
            level_str = "FATAL";
            break;
        default:
            sprintf(level_buf, "%05d", level);
            break;
    }
    log_write_impl(module, file, line, level, level_str, format, ap);

    return;
}

/********************************************************************************************/

static void log_write_impl(const char* module, 
                              const char* file, 
                              int line, 
                              uint16_t level, 
                              const char* level_str, 
                              const char* format, 
                              va_list ap)
{
    UNUSED(module);

    static volatile int last_day = -1;
    static volatile int log_fd = -1;
    static volatile int current_tid = 0;

    static __thread int t_tid = -1;

    int loop = 1000 * 1000;
    int old = 0;
    char buf[4096];
    int count = 0;

    if(level < g_config.log_level)
    {
        return;
    }

    time_info ti = get_time();

    /* t_tid is in tls, so no need to protected by lock */
    if(t_tid == -1)
    {
        t_tid = do_gettid();
    }

    int id = (g_has_fork == 1) ? g_pid : t_tid;

    /* create new file per day */
    if((ATOMIC_GET(g_has_init) == LOG_HAS_INIT) && (ti.day != ATOMIC_GET(last_day)))
    {
        while((old = ATOMIC_CAS(current_tid, 0, id)) != 0)
        {
            /* same thread enter here more than once, return */
            if(old == id)
            {
                fprintf(stderr, "uc_log_v has reentry\n");
                return;
            }

            /* wait other thread to exit
               maybe in signal, so can not sleep */
            if(--loop <= 0) 
            {
                return;
            }

        }

        if(ti.day != ATOMIC_GET(last_day))
        {
            //open_redir(ts);
            if (open_log(&log_fd) == 0)
            {
                ATOMIC_SET(last_day, ti.day);
            }
        }

        ATOMIC_CLEAR(current_tid);
    }

    count = sprintf(buf, "[ %s %5d %02d:%02d:%02d.%03d %s:%d ] ", 
                    level_str, id, ti.hour, ti.min, ti.sec, ti.msec, file, line);

    count += vsnprintf(buf + count, sizeof(buf) - count - 1, format, ap);

    if((count > 0) && (count < (signed)sizeof(buf) - 2))
    {
        if(buf[count - 1] != '\n')
        {
            buf[count] = '\n';
            buf[count + 1] = '\0';
            count += 1;
        }
        else
        {
            buf[count] = '\0';
        }

        if(ATOMIC_GET(g_has_init) == LOG_HAS_INIT)
        {
            write_2_file(ATOMIC_GET(log_fd), buf, count);
            //if(g_config.redir_mode == 1)
            //{
            //    write_2_file(STDOUT_FILENO, buf, count);
            //}
        }
        else
        {
            write_2_file(STDERR_FILENO, buf, count);
        }
    }

    return;
}

static void write_2_file(int fd, const char* buf, int size)
{
    int result = -1;

    do
    {
        result = write(fd, buf, size);
    } while((result < 0) && (errno == EINTR));

    return;
}

static int open_log(volatile int* fd)
{
    int result = -1;
    time_t tt;
    struct tm ts;
    char buf[PATH_MAX];
    int new_fd;
    int old_fd;

    time(&tt);
    localtime_r(&tt, &ts);

    snprintf(buf, sizeof(buf) - 1, "%s/%s_%04d%02d%02d.log", 
             g_config.log_path, g_config.log_file, 
             ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday);
    buf[sizeof(buf) - 1] = '\0';

    errno = 0;
    new_fd = open(buf, O_WRONLY|O_CREAT|O_APPEND, 0644);
    if(new_fd < 0)
    {
        fprintf(stderr, "open new log file : %s error, errno %d - %s\n", 
                buf, mq_errno(), mq_last_error());
        return -1;
    }

    old_fd = ATOMIC_SWAP(*fd, new_fd);
    if(old_fd != -1)
    {
        do
        {
            errno = 0;
            result = close(old_fd);
        } while((result < 0) && (errno == EINTR));
    }

    return 0;
}

static time_info get_time(void)
{
    static volatile time_t g_sec;
    static volatile time_info g_info;

    int t;
    int hour;
    int day;
    struct timespec cur;
    time_info ti;

    clock_gettime(CLOCK_REALTIME, &cur);

    ti.value = g_info.value;
    ti.msec = cur.tv_nsec / (1000 * 1000);

    if(g_sec != cur.tv_sec)
    {
        t = cur.tv_sec % (24 * 60 * 60);
        hour = (t / (60 * 60)) + g_config.time_zone;
        day = 0;
        if(hour >= 24)
        {
            hour -= 24;
            day = 1;
        }
        if(hour < 0)
        {
            hour += 24;
            day = -1;
        }
        if(hour != g_info.hour)
        {
            ti.hour = hour;
            ti.day = day + (cur.tv_sec / (24 * 60 * 60));
        }
        t %= (60 * 60);
        ti.min = t / 60;
        ti.sec = t % 60;

        ATOMIC_SET(*(volatile uint64_t*)&g_info, ti.value);
        ATOMIC_SET(g_sec, cur.tv_sec);
    }

    return ti;
}
