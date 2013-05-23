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

#ifndef  __MQ_EVHTTP_H__
#define  __MQ_EVHTTP_H__
#ifdef __cplusplus
extern "C" 
{
#endif

#include "config.h"
#include "main.h"
#include "mq_util.h"
#include "mq_store_manage.h"

/////////////////////////////////////////////////////////////////////////////////

#define  QUEUE_MAX_SIZE         1000000000     /* max queue items limit */
#define  MAX_QUEUE_WARNING      10000 * 10000  /* max queue items warnning */
#define  INC_QUEUE_WARNING      100 * 10000    /* put queue warnning one minute */
#define  MAX_INFO_SIZE          1024 * 1024    /* max info size */
#define  DUMP_INFO_INTERVAL     60             /* dump info interval on log */
#define  MAX_CHARSET_SIZE       40             /* char set max lenght */

/////////////////////////////////////////////////////////////////////////////////

#define MQ_HTTP_REASON_OK               "OK"
#define MQ_HTTP_REASON_BAD_REQ          "Bad Request"
#define MQ_HTTP_REASON_UNKNOWN_NAME     "Invalid requst, Can't find name"
#define MQ_HTTP_REASON_INVALID_ARG      "Invalid requst, Invalid requst"
#define MQ_HTTP_REASON_INVALID_NUM      "Invalid requst, Invalid dest number"
#define MQ_HTTP_REASON_QUE_NO_EXIST     "Can't find queue"
#define MQ_HTTP_REASON_ADD_QUE_ERR      "Create queue fail"
#define MQ_HTTP_REASON_QUE_WLOCK        "Put error, queue is write lock"
#define MQ_HTTP_REASON_QUE_FULL         "Put error, queue is full"
#define MQ_HTTP_REASON_PUT_ERR          "Put error"
#define MQ_HTTP_REASON_QUE_EMPTY        "queue is empty"
#define MQ_HTTP_REASON_GET_ERR          "Get error"
#define MQ_HTTP_REASON_SET_MAXQUE_ERR   "Set maxqueue error"
#define MQ_HTTP_REASON_SET_DELAY_ERR    "Set delay error"
#define MQ_HTTP_REASON_SET_WLOCK_ERR    "Set readonly error"
#define MQ_HTTP_REASON_SET_SYNCTIME_ERR "Set sync time error"
#define MQ_HTTP_REASON_REMOVE_ERR       "Remove error"
#define MQ_HTTP_REASON_RESET_ERR        "Reset error"
#define MQ_HTTP_REASON_UNKNOWN_OPT      "Unknown option"

/////////////////////////////////////////////////////////////////////////////////

extern int pipe_fd[2];
extern app_info_t g_info;
extern app_info_t g_last_info;

/////////////////////////////////////////////////////////////////////////////////

int mq_http_init(void);
int mq_http_start(void);
void mq_http_stop(void);

/////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif /* __MQ_EVHTTP_H__ */
