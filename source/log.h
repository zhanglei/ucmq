#ifndef __LOG_H__
#define __LOG_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include <limits.h>
#include <stdarg.h>
#include <linux/limits.h>

#include "common.h"

/********************************************************************************************/

#define log_trace(format, ...)    log_write("", __FILE__, __LINE__, LOG_TRACE, format, ##__VA_ARGS__);
#define log_debug(format, ...)    log_write("", __FILE__, __LINE__, LOG_DEBUG, format, ##__VA_ARGS__);
#define log_info(format, ...)     log_write("", __FILE__, __LINE__, LOG_INFO, format, ##__VA_ARGS__);
#define log_warn(format, ...)     log_write("", __FILE__, __LINE__, LOG_WARN, format, ##__VA_ARGS__);
#define log_error(format, ...)    log_write("", __FILE__, __LINE__, LOG_ERROR, format, ##__VA_ARGS__);
#define log_fatal(format, ...)    log_write("", __FILE__, __LINE__, LOG_FATAL, format, ##__VA_ARGS__);

/********************************************************************************************/

typedef struct log_config
{
    char        log_path[PATH_MAX]; /* log dir */
    char        log_file[NAME_MAX]; /* log file prefix */
    uint16_t    log_level;          /* output log level */
    uint32_t    time_zone;          /* local time zone (GMT+8 should be 8) */
}log_config_t;

/* default log level
   higher level, more important */
enum LOG_LEVEL_VALUE
{
    LOG_TRACE = 0,               /* trace run path */
    LOG_DEBUG = 10,              /* debug */
    LOG_INFO  = 100,             /* runtime infomation */
    LOG_WARN  = 1000,            /* some error but can fix or ignore it */
    LOG_ERROR = 10000,           /* error */

    LOG_FATAL = 65535,           /* fatal, maybe need to reboot */
};

/*********************************************************************************************
Function Name:  log_init_config
Description  :  Help to init log_config_t
Inputs       :  log_config_t* config        :
Outputs      :  return value                   : None.
ErrorCodes   :
History      :
----------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
*********************************************************************************************/
extern void log_init_config(log_config_t* config);

/*********************************************************************************************
Function Name:  log_init
Description  :  Init log. Each process must init once
Inputs       :  log_config_t* config        :
Outputs      :  return value                   : 
ErrorCodes   :
History      :
----------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
*********************************************************************************************/
extern int log_init(const log_config_t* config);

/*********************************************************************************************
Function Name:  log_write
Description  :  Write log. If write log before init, it will print to stderr.
Inputs       :  const char* module             :
                const char* file               :
                int line                       :
                uint16_t level                 :
                const char* format             :
                ...                            :
Outputs      :  return value                   : 
ErrorCodes   :
History      :
----------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
*********************************************************************************************/
extern void log_write(const char* module, 
                         const char* file, 
                         int line, 
                         uint16_t level, 
                         const char* format, 
                         ...);

/*********************************************************************************************
Function Name:  log_writev
Description  :  Write log. If write log before init, it will print to stderr.
Inputs       :  const char* module             :
                const char* file               :
                int line                       :
                uint16_t level                 :
                const char* format             :
                va_list ap                     :
Outputs      :  return value                   : 
ErrorCodes   :
History      :
----------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
*********************************************************************************************/
extern void log_writev(const char* module, 
                          const char* file, 
                          int line, 
                          uint16_t level, 
                          const char* format, 
                          va_list ap);

/********************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* #ifndef __LOG_H__ */
