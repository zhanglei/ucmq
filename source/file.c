#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#ifndef IOV_MAX
#define IOV_MAX 1024
#endif

#include "file.h"
#include "util.h"
#include "internal.h"

/*********************************************************************************************
Function Name:  is_nonblock
Description  :  Checking file descriptor is non-blocking or not.
Inputs       :  int fd                             : File descriptor.
                fd_block_e block_type              : File descroptor block type:
                                                     FD_BLOCK    : fd is blocking.
                                                     FD_NONBLOCK : fd is non-blocking.
                                                     FD_UNKNOWN  : fd's block type is unknown.
                                                                      Would sys call fcntl() checking.
Outputs      :  return value                       :   1 : fd is non-blocking.
                                                       0 : fd is blocking.
                                                      -1 : Fail. See errno.
ErrorCodes   :  errno                              : sys call errno from fnctl().
                MQ_EINVAL                          : Invalid argument.
History      :
---------------------------------------------------------------------------------------------
Author                 Date                       Comments
lijz                   2011-6-24                  create
**********************************************************************************************/
static inline int is_nonblock(int fd, fd_block_e block_type);

/*********************************************************************************************
Function Name:  block_read_n
Description  :  Like sys call read(), synchronously attempt to read up to count bytes from
                blocking file descriptor fd into the buffer starting at buf,
                until end of file(EOF), timeout or other error.
Inputs       :  int fd                             : File descriptor. Must be blocking fd.
                void* buf                          : Buffer for reading into. Must be NOT NULL.
                int32_t size                       : Buffer size. Must great than zero.
                int32_t timeout                    : Timeout in milliseconds.
                                                     > 0 : Timeout in milliseconds.
                                                       0 : Means return immediately.
                                                     < 0 : Infinite timeout.
Outputs      :  return value                       : > 0 : Success. the number of bytes read return.
                                                       0 : end of file(EOF).
                                                      -1 : Fail or timeout. See errno.
                void* buf                          : readed data filled.
ErrorCodes   :  errno                              : sys call errno from poll() or read().
                MQ_ETIMEDOUT                       : timeout.
History      :
---------------------------------------------------------------------------------------------
Author                 Date                       Comments
lijz                   2011-6-24                  create
**********************************************************************************************/
static inline int32_t block_read_n(int fd, void* buf, int32_t size, int32_t timeout);

/*********************************************************************************************
Function Name:  nonblock_read_n
Description  :  Like sys call read(), synchronously attempt to read up to count bytes from
                non-blocking file descriptor fd into the buffer starting at buf,
                until end of file(EOF), timeout or other error.
Inputs       :  int fd                             : File descriptor. Must be non-blocking fd.
                void* buf                          : Buffer for reading into. Must be NOT NULL.
                int32_t size                       : Buffer size. Must great than zero.
                int32_t timeout                    : Timeout in milliseconds.
                                                     > 0 : Timeout in milliseconds.
                                                       0 : Means return immediately.
                                                     < 0 : Infinite timeout.
Outputs      :  return value                       : > 0 : Success. the number of bytes read return.
                                                       0 : end of file(EOF).
                                                      -1 : Fail or timeout. See errno.
                void* buf                          : readed data filled.
ErrorCodes   :  errno                              : sys call errno from read() or poll().
                MQ_ETIMEDOUT                       : timeout.
History      :
---------------------------------------------------------------------------------------------
Author                 Date                       Comments
lijz                   2011-6-24                  create
**********************************************************************************************/
static inline int32_t nonblock_read_n(int fd, void* buf, int32_t size, int32_t timeout);

/*********************************************************************************************
Function Name:  block_write_n
Description  :  Like sys call write(), synchronously write up to count bytes from the buffer
                pointed buf to the file referred to by blocking the file descriptor fd,
                until no more data written, timeout or other error.
Inputs       :  int fd                             : File descriptor. Must be blocking fd.
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
                MQ_ETIMEDOUT                       : timeout.
History      :
---------------------------------------------------------------------------------------------
Author                 Date                       Comments
lijz                   2011-6-24                  create
**********************************************************************************************/
static inline int32_t block_write_n(int fd, const void* buf, int32_t size, int32_t timeout);

/*********************************************************************************************
Function Name:  nonblock_write_n
Description  :  Like sys call write(), synchronously write up to count bytes from the buffer
                pointed buf to the file referred to by non-blocking the file descriptor fd,
                until no more data written, timeout or other error.
Inputs       :  int fd                             : File descriptor. Must be non-blocking fd.
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
                MQ_ETIMEDOUT                       : timeout.
History      :
---------------------------------------------------------------------------------------------
Author                 Date                       Comments
lijz                   2011-6-24                  create
**********************************************************************************************/
static inline int32_t nonblock_write_n(int fd, const void* buf, int32_t size, int32_t timeout);

/*********************************************************************************************/

int wait_fd(int fd, short events, int32_t timeout)
{
    struct pollfd fds;

    fds.fd      = fd;
    fds.events  = events;
    fds.revents = 0;

    errno = 0;
    do
    {
        int r = poll(&fds, 1, timeout);
        if (r >= 0)
        {
            return fds.revents;
        }
    } while (errno == EINTR);
    return -1;
}

int32_t read_n(int fd, fd_block_e block_type, void* buf, int32_t size, int32_t timeout)
{
    int is_nonblock_flag  = 0;

    assert(buf != NULL);

    is_nonblock_flag = is_nonblock(fd, block_type);
    if(is_nonblock_flag < 0)
    {
        return -1;
    }

    if(is_nonblock_flag)
    {
        return nonblock_read_n(fd, buf, size, timeout);
    }
    else
    {
        return block_read_n(fd, buf, size, timeout);
    }
}

int32_t write_n(int fd, fd_block_e block_type, const void* buf, int32_t size, int32_t timeout)
{
    int is_nonblock_flag  = 0;

    assert(buf != NULL);

    is_nonblock_flag = is_nonblock(fd, block_type);
    if(is_nonblock_flag < 0)
    {
        return -1;
    }

    if(is_nonblock_flag)
    {
        return nonblock_write_n(fd, buf, size, timeout);
    }
    else
    {
        return block_write_n(fd, buf, size, timeout);
    }
}

/********************************************************************************************/

static inline int is_nonblock(int fd, fd_block_e block_type)
{
    int ret   = -1;
    int flags = 0;

    switch(block_type)
    {
        case FD_BLOCK:
            ret = 0;
            break;
        case FD_NONBLOCK:
            ret = 1;
            break;
        case FD_UNKNOWN:
            flags = fcntl(fd, F_GETFL, 0);
            if(flags < 0)
            {
                ret = -1;
            }
            else if((flags & O_NONBLOCK))
            {
                ret = 1;
            }
            else
            {
                ret = 0;
            }
            break;
        default:
            set_errno(MQ_EINVAL);
            ret = -1;
            break;
    }

    return ret;
}

static inline int32_t block_read_n(int fd, void* buf, int32_t size, int32_t timeout)
{
    int exit_loop           = 0;
    int revents             = 0;
    int32_t next_to         = -1;
    int32_t read_size       = 0;
    int32_t total_read_size = 0;
    struct timespec abs_to;

    abs_to = get_abs_time(timeout);
    total_read_size = 0;
    exit_loop = 0;
    do
    {
        next_to = get_rel_time(&abs_to);

        if((timeout > 0 && next_to <= 0))
        {
            set_errno(MQ_ETIMEDOUT);
            total_read_size = -1;
            exit_loop = 1;
        }
        else
        {
            revents = wait_fd(fd, POLLIN, (timeout <= 0 ? timeout : next_to));
            switch (revents)
            {
            case POLLIN:
                do
                {
                    read_size = read(fd, buf + total_read_size, size - total_read_size);
                }while(read_size < 0 && errno == EINTR);
                switch (read_size)
                {
                case -1: /* ERROR */
                    total_read_size = -1;
                case 0:  /* EOF */
                    exit_loop = 1;
                    break;
                default:
                    total_read_size += read_size;
                    break;
                }
                break;
            case 0:
                set_errno(MQ_ETIMEDOUT);
                total_read_size = -1;
                exit_loop = 1;
                break;
            default: /* other revents or error */
                total_read_size = -1;
                exit_loop = 1;
                break;
            }
        }
    }while(!exit_loop && (total_read_size < size) && timeout != 0);

    return total_read_size;
}

static inline int32_t nonblock_read_n(int fd, void* buf, int32_t size, int32_t timeout)
{
    int exit_loop           = 0;
    int revents             = 0;
    int32_t next_to         = -1;
    int32_t read_size       = 0;
    int32_t total_read_size = 0;
    struct timespec abs_to;

    abs_to = get_abs_time(timeout);
    total_read_size = 0;
    exit_loop = 0;
    do
    {
        read_size = read(fd, buf + total_read_size, size - total_read_size);
        switch (read_size)
        {
        case -1:
            switch(errno)
            {
            case EINTR:
                continue;
                break;
            case EAGAIN:
                next_to = get_rel_time(&abs_to);

                if((timeout == 0) || (timeout > 0 && next_to <= 0))
                {
                    set_errno(MQ_ETIMEDOUT);
                    total_read_size = -1;
                    exit_loop = 1;
                }
                else
                {
                    revents = wait_fd(fd, POLLIN, (timeout < 0 ? timeout : next_to));

                    switch (revents)
                    {
                    case POLLIN:
                        continue;
                    case 0:
                        set_errno(MQ_ETIMEDOUT);
                        total_read_size = -1;
                        exit_loop = 1;
                        break;
                    default: /* other revents or error */
                        total_read_size = -1;
                        exit_loop = 1;
                        break;
                    }
                }
                break;
            default:
                total_read_size = -1;
                exit_loop = 1;
                break;
            } /* switch(errno) */
            break;
        case 0:
            exit_loop = 1;
            break;
        default:
            total_read_size += read_size;
            break;
        }
    }while(!exit_loop && (total_read_size < size) && timeout != 0);

    return total_read_size;
}

static inline int32_t block_write_n(int fd, const void* buf, int32_t size, int32_t timeout)
{
    int exit_loop            = 0;
    int revents              = 0;
    int32_t next_to          = -1;
    int32_t write_size       = 0;
    int32_t total_write_size = 0;
    struct timespec abs_to;

    abs_to = get_abs_time(timeout);
    total_write_size = 0;
    exit_loop = 0;
    do
    {
        next_to = get_rel_time(&abs_to);

        if((timeout > 0 && next_to <= 0))
        {
            set_errno(MQ_ETIMEDOUT);
            total_write_size = -1;
            exit_loop = 1;
        }
        else
        {
            revents = wait_fd(fd, POLLOUT, (timeout <= 0 ? timeout : next_to));

            if(revents == POLLOUT)
            {
                do
                {
                    write_size = write(fd, buf + total_write_size, size - total_write_size);
                }while(write_size < 0 && errno == EINTR);
                if(write_size > 0)
                {
                    total_write_size += write_size;
                }
                else if(write_size == 0) /* nothing was written */
                {
                    exit_loop = 1;
                }
                else /* write_size < 0 */
                {
                    total_write_size = -1;
                    exit_loop = 1;
                }
            }
            else if(revents == 0)
            {
                set_errno(MQ_ETIMEDOUT);
                total_write_size = -1;
                exit_loop = 1;
            }
            else /* other revents or error */
            {
                total_write_size = -1;
                exit_loop = 1;
            }
        }
    }while(!exit_loop && (total_write_size < size) && timeout != 0);

    return total_write_size;
}

static inline int32_t nonblock_write_n(int fd, const void* buf, int32_t size, int32_t timeout)
{
    int exit_loop            = 0;
    int revents              = 0;
    int32_t next_to          = -1;
    int32_t write_size       = 0;
    int32_t total_write_size = 0;
    struct timespec abs_to;

    abs_to = get_abs_time(timeout);
    total_write_size = 0;
    exit_loop = 0;
    do
    {
        write_size = write(fd, buf + total_write_size, size - total_write_size);
        if(write_size > 0)
        {
            total_write_size += write_size;
        }
        else if(write_size == 0) /* nothing was writen */
        {
            exit_loop = 1;
        }
        else /* write_size < 0 */
        {
            switch(errno)
            {
                case EINTR:
                    continue;
                    break;
                case EAGAIN:
                    next_to = get_rel_time(&abs_to);

                    if((timeout == 0) || (timeout > 0 && next_to <= 0))
                    {
                        set_errno(MQ_ETIMEDOUT);
                        total_write_size = -1;
                        exit_loop = 1;
                    }
                    else
                    {
                        revents = wait_fd(fd, POLLOUT, (timeout < 0 ? timeout : next_to));

                        if(revents == POLLOUT)
                        {
                            continue;
                        }
                        else if(revents == 0)
                        {
                            set_errno(MQ_ETIMEDOUT);
                            total_write_size = -1;
                            exit_loop = 1;
                        }
                        else /* other revents or error */
                        {
                            total_write_size = -1;
                            exit_loop = 1;
                        }
                    }
                    break;
                default:
                    total_write_size = -1;
                    exit_loop = 1;
                    break;
            } /* switch(errno) */
        } /* write_size < 0 */

    }while(!exit_loop && (total_write_size < size) && timeout != 0);

    return total_write_size;
}
