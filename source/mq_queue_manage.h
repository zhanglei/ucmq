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

#ifndef  __MQ_QUEUE_MANAGE_H__
#define  __MQ_QUEUE_MANAGE_H__
#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdbool.h>

#include "mq_util.h"
#include "mq_config.h"
#include "mq_store_manage.h"
#include "mq_store_rtag.h"
#include "mq_store_file.h"

////////////////////////////////////////////////////////////////////////

#define  MAX_QUEUE_NAME_LEN     32             /* max queue name length */

////////////////////////////////////////////////////////////////////////

typedef enum queue_put_ret
{
    QUEUE_PUT_OK      =  0,
    QUEUE_FULL        =  1,
    QUEUE_WLOCK       =  2,
    QUEUE_PUT_ERROR   =  3,
}queue_put_ret_e;

typedef enum queue_get_ret
{
    QUEUE_GET_OK      =  0,
    QUEUE_GET_END     =  1,
    QUEUE_GET_ERROR   =  2,
}queue_get_ret_e;

extern mq_queue_list_t* g_mq_qlist;
////////////////////////////////////////////////////////////////////////

/* ucmq queue manage */
bool mq_qm_del_queue(const char* qname);
mq_queue_t* mq_qm_find_queue(const char *qname);
mq_queue_t* mq_qm_add_queue(const char* qname);

/* ucmq queue store manage */
bool     mq_qm_open_store(void);
bool     mq_qm_close_store(void);
void     mq_qm_sync_store(void);
uint64_t mq_qm_get_store_count(void);

/* ucmq opt */
int  mq_qm_push_item(mq_queue_t *mq_queue, msg_item_t *msg_item);
int  mq_qm_pop_item(mq_queue_t *mq_queue, msg_item_t *msg_item);
bool mq_qm_set_maxqueue(mq_queue_t *mq_queue, uint32_t max_queue);
bool mq_qm_set_delay(mq_queue_t *mq_queue, uint32_t dest_delay);
bool mq_qm_set_wlock(mq_queue_t *mq_queue, uint32_t write_lock);
bool mq_qm_set_synctime(int sync_interval);

////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif /* __MQ_QUEUE_MANAGE_H__ */
