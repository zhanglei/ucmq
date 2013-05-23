#ifndef __FILE_H__
#define __FILE_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/uio.h>

#include "common.h"

/********************************************************************************************/

typedef enum fd_block
{
    FD_INVALID  = -1,  /* use for avoid/detect some errors */
    FD_UNKNOWN  = 0,   /* unknown block or not */
    FD_BLOCK    = 1,   /* fd is block */
    FD_NONBLOCK = 2,   /* fd is non-block */
} fd_block_e;

/*********************************************************************************************
Function Name:  wait_fd
Description  :  Sys call poll(), wait for some event on a file descriptor.
Inputs       :  int fd                             : File descriptor.
                short events                       : Events to wait.
                int32_t timeout                    : Timeout in milliseconds.
                                                     > 0 : Timeout in milliseconds.
                                                       0 : Means return immediately.
                                                     < 0 : Infinite timeout.
Outputs      :  return value                       : > 0 : Success. Event number return.
                                                       0 : Timeout and no event.
                                                      -1 : Fail. See errno.
ErrorCodes   :  errno                              : sys call errno from poll().
History      :
---------------------------------------------------------------------------------------------
Author                 Date                       Comments
lijz                   2011-6-24                  create
**********************************************************************************************/
extern int wait_fd(int fd, short events, int32_t timeout);

/*********************************************************************************************
Function Name:  read_n
Description  :  Like sys call read(), synchronously attempt to read up to count bytes from
                file descriptor fd into the buffer starting at buf, until end of file(EOF),
                timeout or other error.
Inputs       :  int fd                             : File descriptor.
                fd_block_e block_type           : File descroptor block type:
                                                     FD_BLOCK    : fd is blocking.
                                                     FD_NONBLOCK : fd is non-blocking.
                void* buf                          : Buffer for reading into. Must be NOT NULL.
                int32_t size                       : Buffer size. Must great than zero.
                int32_t timeout                    : Timeout in milliseconds.
                                                     > 0 : Timeout in milliseconds.
                                                       0 : Means return immediately.
                                                     < 0 : Infinite timeout.
Outputs      :  return value                       : > 0 : Success. the number of bytes read return.
                                                       0 : end of file(EOF).
                                                      -1 : Fail or timeout. See errno.
                void* buf                          : Readed data filled.
ErrorCodes   :  errno                              : sys call errno from read() or poll().
                ETIMEDOUT                       : timeout.
History      :
---------------------------------------------------------------------------------------------
Author                 Date                       Comments
lijz                   2011-6-24                  create
**********************************************************************************************/
extern int32_t read_n(int fd,                    
                         fd_block_e block_type, 
                         void* buf,               
                         int32_t size,           
                         int32_t timeout);      

/*********************************************************************************************
Function Name:  write_n
Description  :  Like sys call write(), synchronously write up to count bytes from the buffer
                pointed buf to the file referred to by the file descriptor fd,
                until no more data written, timeout or other error.
Inputs       :  int fd                             : File descriptor.
                fd_block_e block_type           : File descroptor block type:
                                                     FD_BLOCK    : fd is blocking.
                                                     FD_NONBLOCK : fd is non-blocking.
                void* buf                          : Buffer data for writing to fd. Must be NOT NULL.
                int32_t size                       : Buffer size. Must great than zero.
                int32_t timeout                    : Timeout in milliseconds.
                                                     > 0 : Timeout in milliseconds.
                                                       0 : Means return immediately.
                                                     < 0 : Infinite timeout.
Outputs      :  return value                       : > 0 : Success. the number of bytes written return.
                                                       0 : Nothing was written.
                                                      -1 : Fail or timeout. See errno.
ErrorCodes   :  errno                              : sys call errno from write() or poll().
                LIB_ETIMEDOUT                       : timeout.
History      :
---------------------------------------------------------------------------------------------
Author                 Date                       Comments
lijz                   2011-6-24                  create
**********************************************************************************************/
extern int32_t write_n(int fd,                   
                          fd_block_e block_type, 
                          const void* buf,          
                          int32_t size,             
                          int32_t timeout);         

/******************************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* #ifndef __FILE_H__ */

