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

#ifndef  __MQ_STORE_MSG_H__
#define  __MQ_STORE_MSG_H__
#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "mq_util.h"

//////////////////////////////////////////////////////////////////////////

typedef enum msg_check_ret
{
    MSG_CHECK_RET_OK          =  0,  
    MSG_CHECK_RET_MSG_ERR     =  1,  
    MSG_CHECK_RET_MAG_NUM_ERR =  2,  
    MSG_CHECK_RET_PARSE_ERR   =  3,  
    MSG_CHECK_RET_CRC_ERR     =  4,  
    MSG_CHECK_RET_LEN_BEY     =  5,  
}msg_check_ret_e;

/////////////////////////////////////////////////////////////////////////

bool mq_sm_db_build_msg_crc16(uint16_t* crc, const char* data, int len);
bool mq_sm_db_parse_msg_head(msg_item_t *item, const char *msg_head);
bool mq_sm_db_write_msg(mq_queue_t *mq_queue, msg_item_t *msg_item);
int  mq_sm_db_parse_msg(const char* msg, uint32_t cur_pos, msg_item_t* msg_item);

bool mq_sm_db_write_file_head(mq_queue_t *mq_queue);
uint32_t mq_sm_db_cal_msg_count(int fd, uint32_t end_pos);

//////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif /* __MQ_STORE_MSG__ */
