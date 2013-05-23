#ifndef __UTIL_H__
#define __UTIL_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>
#include <unistd.h>

#include "common.h"

/******************************************************************************************************/

/* redir mode */
enum STD_REDIR
{
    REDIR_IN  = 1,         /* redir stdin to /dev/null  */
    REDIR_OUT = 2,         /* redir stdout to /dev/null */
    REDIR_ERR = 4,         /* redir stderr to /dev/null */
};

/*********************************************************************************************
Function Name:  get_time_tick
Description  :  Get current time tick of system
Inputs       :  None. 
Outputs      :  return value                :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern struct timespec get_time_tick(void);

/*********************************************************************************************
Function Name:  get_time_diff_sec
Description  :  Get time pass between two ticks in seconds
Inputs       :  const struct timespec* start    :
                onst struct timespec* end       : 
Outputs      :  return value                    :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern double get_time_diff_sec(const struct timespec* start,
                                   const struct timespec* end);

/*********************************************************************************************
Function Name:  get_time_diff_nsec
Description  :  Get time pass between two ticks in nanoseconds 
Inputs       :  const struct timespec* start    :
                onst struct timespec* end       : 
Outputs      :  return value                    :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern int64_t get_time_diff_nsec(const struct timespec* start,
                                     const struct timespec* end);

/*********************************************************************************************
Function Name:  get_abs_time
Description  :  Convert relative time (in milliseconds) to absolute time
Inputs       :  int32_t time_affter             :
Outputs      :  return value                    :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern struct timespec get_abs_time(int32_t time_affter);

/*********************************************************************************************
Function Name:  get_rel_time
Description  :  Convert absolute time to relative time in milliseconds
Inputs       :  const struct timespec* abs_time :
Outputs      :  return value                    :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern int32_t get_rel_time(const struct timespec* abs_time);

/*********************************************************************************************
Function Name:  get_rand
Description  :  Get random number from /dev/urandom
Inputs       :  None.
Outputs      :  return value                    :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern int32_t get_rand(void);

/*********************************************************************************************
Function Name:  do_mkdir
Description  :  Just as "mkdir /p"
Inputs       :  const char* dir                 :
                mode_t mode                     :
Outputs      :  return value                    :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern int do_mkdir(const char* dir, mode_t mode);

/*********************************************************************************************
Function Name:  do_gettid
Description  :  Get current thread id
Inputs       :  None.
Outputs      :  return value                    :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern int do_gettid(void);

/*********************************************************************************************
Function Name:  set_sig_mask
Description  :  Set signal marks
Inputs       :  enum OPT opt                 :
                int count                       :
Outputs      :  return value                    :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern int set_sig_mask(enum OPT opt, int count, ...);

/*********************************************************************************************
Function Name:  do_sleep
Description  :  Sleep in milliseconds
Inputs       :  int32_t timeout                 :
Outputs      :  return value                    :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern int do_sleep(int32_t timeout);

/*********************************************************************************************
Function Name:  get_self_path
Description  :  Get exe path from /proc/self/exe 
Inputs       :  None.
Outputs      :  return value                    :
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern const char* get_self_path(void);


/* just rename */
#define do_getpid getpid

/******************************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* #ifndef __UTIL_H__ */
