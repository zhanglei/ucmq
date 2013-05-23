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

#ifndef  __MQ_STORE_MANAGE_H__
#define  __MQ_STORE_MANAGE_H__
#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include <stdbool.h>

#include "mq_util.h"

/////////////////////////////////////////////////////////////////////////

/* ucmq store manage data files handle */
bool mq_sm_open_db(mq_queue_t *mq_queue, queue_file_t *queue_file);
bool mq_sm_close_db(mq_queue_t *mq_queue);
bool mq_sm_creat_db(mq_queue_t *mq_queue);
bool mq_sm_remove_db(mq_queue_t *mq_queue);

/* ucmq store manage message items handle */
bool mq_sm_put_data(msg_item_t *msg_item, mq_queue_t *mq_queue);
bool mq_sm_get_data(msg_item_t *msg_item, mq_queue_t *mq_queue);
uint64_t mq_sm_get_msg_count(mq_queue_t *mq_queue);

///////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif /* __MQ_STORE_MANAGE_H__ */
