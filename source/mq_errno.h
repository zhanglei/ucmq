/*
*  Copyright (c) 2013 UCWeb Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
* You may obtain a copy of the License at
*
*       http://www.gnu.org/licenses/gpl-2.0.html
*
* Email: osucmq@ucweb.com
* Author: ShaneYuan
*/

#ifndef __MQ_ERRNO_H__
#define __MQ_ERRNO_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include <errno.h>

#include "common.h"

/******************************************************************************************************/

/*********************************************************************************************
Function Name:  mq_strerror
Description  :  Just as strerror.
Inputs       :  
Outputs      :  
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern const char* mq_strerror(int32_t error_number);

/*********************************************************************************************
Function Name:  mq_errno
Description  :  Get errno.
Inputs       :  
Outputs      :  
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern int32_t mq_errno(void);

/*********************************************************************************************
Function Name:  mq_last_error 
Description  :  The same as mq_strerror(mq_errno()).
Inputs       :  
Outputs      :  
ErrorCodes   :
History      :
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-06-25                    create
**********************************************************************************************/
extern const char* mq_last_error(void);

/* Define errno */

#define MQ_EOK             0                  /* No error, everything is ok */

#define MQ_EPERM           -(EPERM)           /* Operation not permitted */
#define MQ_ENOENT          -(ENOENT)          /* No such file or directory */
#define MQ_ESRCH           -(ESRCH)           /* No such process */
#define MQ_EINTR           -(EINTR)           /* Interrupted system call */
#define MQ_EIO             -(EIO)             /* I/O error */
#define MQ_ENXIO           -(ENXIO)           /* No such device or address */
#define MQ_E2BIG           -(E2BIG)           /* Arg list too long */
#define MQ_ENOEXEC         -(ENOEXEC)         /* Exec format error */
#define MQ_EBADF           -(EBADF)           /* Bad file number */
#define MQ_ECHILD          -(ECHILD)          /* No child processes */
#define MQ_EAGAIN          -(EAGAIN)          /* Try again */
#define MQ_ENOMEM          -(ENOMEM)          /* Out of memory */
#define MQ_EACCES          -(EACCES)          /* Permission denied */
#define MQ_EFAULT          -(EFAULT)          /* Bad address */
#define MQ_ENOTBLK         -(ENOTBLK)         /* Block device required */
#define MQ_EBUSY           -(EBUSY)           /* Device or resource busy */
#define MQ_EEXIST          -(EEXIST)          /* File exists */
#define MQ_EXDEV           -(EXDEV)           /* Cross-device link */
#define MQ_ENODEV          -(ENODEV)          /* No such device */
#define MQ_ENOTDIR         -(ENOTDIR)         /* Not a directory */
#define MQ_EISDIR          -(EISDIR)          /* Is a directory */
#define MQ_EINVAL          -(EINVAL)          /* Invalid argument */
#define MQ_ENFILE          -(ENFILE)          /* File table overflow */
#define MQ_EMFILE          -(EMFILE)          /* Too many open files */
#define MQ_ENOTTY          -(ENOTTY)          /* Not a typewriter */
#define MQ_ETXTBSY         -(ETXTBSY)         /* Text file busy */
#define MQ_EFBIG           -(EFBIG)           /* File too large */
#define MQ_ENOSPC          -(ENOSPC)          /* No space left on device */
#define MQ_ESPIPE          -(ESPIPE)          /* Illegal seek */
#define MQ_EROFS           -(EROFS)           /* Read-only file system */
#define MQ_EMLINK          -(EMLINK)          /* Too many links */
#define MQ_EPIPE           -(EPIPE)           /* Broken pipe */
#define MQ_EDOM            -(EDOM)            /* Math argument out of domain of func */
#define MQ_ERANGE          -(ERANGE)          /* Math result not representable */
#define MQ_EDEADLK         -(EDEADLK)         /* Resource deadlock would occur */
#define MQ_ENAMETOOLONG    -(ENAMETOOLONG)    /* File name too long */
#define MQ_ENOLCK          -(ENOLCK)          /* No record locks available */
#define MQ_ENOSYS          -(ENOSYS)          /* Function not implemented */
#define MQ_ENOTEMPTY       -(ENOTEMPTY)       /* Directory not empty */
#define MQ_ELOOP           -(ELOOP)           /* Too many symbolic links encountered */
#define MQ_EWOULDBLOCK     -(EWOULDBLOCK)     /* Operation would block */
#define MQ_ENOMSG          -(ENOMSG)          /* No message of desired type */
#define MQ_EIDRM           -(EIDRM)           /* Identifier removed */
#define MQ_ECHRNG          -(ECHRNG)          /* Channel number out of range */
#define MQ_EL2NSYNC        -(EL2NSYNC)        /* Level 2 not synchronized */
#define MQ_EL3HLT          -(EL3HLT)          /* Level 3 halted */
#define MQ_EL3RST          -(EL3RST)          /* Level 3 reset */
#define MQ_ELNRNG          -(ELNRNG)          /* Link number out of range */
#define MQ_EUNATCH         -(EUNATCH)         /* Protocol driver not attached */
#define MQ_ENOCSI          -(ENOCSI)          /* No CSI structure available */
#define MQ_EL2HLT          -(EL2HLT)          /* Level 2 halted */
#define MQ_EBADE           -(EBADE)           /* Invalid exchange */
#define MQ_EBADR           -(EBADR)           /* Invalid request descriptor */
#define MQ_EXFULL          -(EXFULL)          /* Exchange full */
#define MQ_ENOANO          -(ENOANO)          /* No anode */
#define MQ_EBADRQC         -(EBADRQC)         /* Invalid request code */
#define MQ_EBADSLT         -(EBADSLT)         /* Invalid slot */
#define MQ_EDEADLOCK       -(EDEADLOCK)       /* Dead lock */
#define MQ_EBFONT          -(EBFONT)          /* Bad font file format */
#define MQ_ENOSTR          -(ENOSTR)          /* Device not a stream */
#define MQ_ENODATA         -(ENODATA)         /* No data available */
#define MQ_ETIME           -(ETIME)           /* Timer expired */
#define MQ_ENOSR           -(ENOSR)           /* Out of streams resources */
#define MQ_ENONET          -(ENONET)          /* Machine is not on the network */
#define MQ_ENOPKG          -(ENOPKG)          /* Package not installed */
#define MQ_EREMOTE         -(EREMOTE)         /* Object is remote */
#define MQ_ENOLINK         -(ENOLINK)         /* Link has been severed */
#define MQ_EADV            -(EADV)            /* Advertise error */
#define MQ_ESRMNT          -(ESRMNT)          /* Srmount error */
#define MQ_ECOMM           -(ECOMM)           /* Communication error on send */
#define MQ_EPROTO          -(EPROTO)          /* Protocol error */
#define MQ_EMULTIHOP       -(EMULTIHOP)       /* Multihop attempted */
#define MQ_EDOTDOT         -(EDOTDOT)         /* RFS specific error */
#define MQ_EBADMSG         -(EBADMSG)         /* Not a data message */
#define MQ_EOVERFLOW       -(EOVERFLOW)       /* Value too large for defined data type */
#define MQ_ENOTUNIQ        -(ENOTUNIQ)        /* Name not unique on network */
#define MQ_EBADFD          -(EBADFD)          /* File descriptor in bad state */
#define MQ_EREMCHG         -(EREMCHG)         /* Remote address changed */
#define MQ_ELIBACC         -(ELIBACC)         /* Can not access a needed shared library */
#define MQ_ELIBBAD         -(ELIBBAD)         /* Accessing a corrupted shared library */
#define MQ_ELIBSCN         -(ELIBSCN)         /* .lib section in a.out corrupted */
#define MQ_ELIBMAX         -(ELIBMAX)         /* Attempting to link in too many shared libraries */
#define MQ_ELIBEXEC        -(ELIBEXEC)        /* Cannot exec a shared library directly */
#define MQ_EILSEQ          -(EILSEQ)          /* Illegal byte sequence */
#define MQ_ERESTART        -(ERESTART)        /* Interrupted system call should be restarted */
#define MQ_ESTRPIPE        -(ESTRPIPE)        /* Streams pipe error */
#define MQ_EUSERS          -(EUSERS)          /* Too many users */
#define MQ_ENOTSOCK        -(ENOTSOCK)        /* Socket operation on non-socket */
#define MQ_EDESTADDRREQ    -(EDESTADDRREQ)    /* Destination address required */
#define MQ_EMSGSIZE        -(EMSGSIZE)        /* Message too long */
#define MQ_EPROTOTYPE      -(EPROTOTYPE)      /* Protocol wrong type for socket */
#define MQ_ENOPROTOOPT     -(ENOPROTOOPT)     /* Protocol not available */
#define MQ_EPROTONOSUPPORT -(EPROTONOSUPPORT) /* Protocol not supported */
#define MQ_ESOCKTNOSUPPORT -(ESOCKTNOSUPPORT) /* Socket type not supported */
#define MQ_EOPNOTSUPP      -(EOPNOTSUPP)      /* Operation not supported on transport endpoint */
#define MQ_EPFNOSUPPORT    -(EPFNOSUPPORT)    /* Protocol family not supported */
#define MQ_EAFNOSUPPORT    -(EAFNOSUPPORT)    /* Address family not supported by protocol */
#define MQ_EADDRINUSE      -(EADDRINUSE)      /* Address already in use */
#define MQ_EADDRNOTAVAIL   -(EADDRNOTAVAIL)   /* Cannot assign requested address */
#define MQ_ENETDOWN        -(ENETDOWN)        /* Network is down */
#define MQ_ENETUNREACH     -(ENETUNREACH)     /* Network is unreachable */
#define MQ_ENETRESET       -(ENETRESET)       /* Network dropped connection because of reset */
#define MQ_ECONNABORTED    -(ECONNABORTED)    /* Software caused connection abort */
#define MQ_ECONNRESET      -(ECONNRESET)      /* Connection reset by peer */
#define MQ_ENOBUFS         -(ENOBUFS)         /* No buffer space available */
#define MQ_EISCONN         -(EISCONN)         /* Transport endpoint is already connected */
#define MQ_ENOTCONN        -(ENOTCONN)        /* Transport endpoint is not connected */
#define MQ_ESHUTDOWN       -(ESHUTDOWN)       /* Cannot send after transport endpoint shutdown */
#define MQ_ETOOMANYREFS    -(ETOOMANYREFS)    /* Too many references: cannot splice */
#define MQ_ETIMEDOUT       -(ETIMEDOUT)       /* Connection timed out */
#define MQ_ECONNREFUSED    -(ECONNREFUSED)    /* Connection refused */
#define MQ_EHOSTDOWN       -(EHOSTDOWN)       /* Host is down */
#define MQ_EHOSTUNREACH    -(EHOSTUNREACH)    /* No route to host */
#define MQ_EALREADY        -(EALREADY)        /* Operation already in progress */
#define MQ_EINPROGRESS     -(EINPROGRESS)     /* Operation now in progress */
#define MQ_ESTALE          -(ESTALE)          /* Stale NFS file handle */
#define MQ_EUCLEAN         -(EUCLEAN)         /* Structure needs cleaning */
#define MQ_ENOTNAM         -(ENOTNAM)         /* Not a XENIX named type file */
#define MQ_ENAVAIL         -(ENAVAIL)         /* No XENIX semaphores available */
#define MQ_EISNAM          -(EISNAM)          /* Is a named type file */
#define MQ_EREMOTEIO       -(EREMOTEIO)       /* Remote I/O error */
#define MQ_EDQUOT          -(EDQUOT)          /* Quota exceeded */
#define MQ_ENOMEDIUM       -(ENOMEDIUM)       /* No medium found */
#define MQ_EMEDIUMTYPE     -(EMEDIUMTYPE)     /* Wrong medium type */
#define MQ_ECANCELED       -(ECANCELED)       /* Operation Canceled */
#define MQ_ENOKEY          -(ENOKEY)          /* Required key not available */
#define MQ_EKEYEXPIRED     -(EKEYEXPIRED)     /* Key has expired */
#define MQ_EKEYREVOKED     -(EKEYREVOKED)     /* Key has been revoked */
#define MQ_EKEYREJECTED    -(EKEYREJECTED)    /* Key was rejected by service */

#define MQ_EOWNERDEAD      -(EOWNERDEAD)      /* Owner died */
#define MQ_ENOTRECOVERABLE -(ENOTRECOVERABLE) /* State not recoverable */


/******************************************************************************************************/
#ifdef __cplusplus
}
#endif
#endif /* #ifndef __MQ_ERROR_H__ */
