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

#ifndef  __MQ_STORE_FILE_H__
#define  __MQ_STORE_FILE_H__
#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdio.h>
#include <stdint.h>

#include "mq_util.h"
#include "mq_config.h"

/////////////////////////////////////////////////////////////////////////

typedef enum file_open_flag 
{
    FOPEN_FLAG_CREATE =  0,  
    FOPEN_FLAG_OPEN   =  1,
}file_open_flag_e;

//////////////////////////////////////////////////////////////////////////

#define GEN_QUEUE_PATH_BY_QNAME(path, qname)                             \
{                                                                        \
    snprintf(path, MAX_DATA_PATH_NAME_LEN, "%s/%s",                      \
            g_mq_conf.data_file_path, qname);                            \
}

#define GEN_RTAG_FULL_PATH_BY_QNAME_INDEX(full_name, qname, index)       \
{                                                                        \
    snprintf(full_name, MAX_FULL_FILE_NAME_LEN, "%s/%s/rtag_%012"PRIu64, \
            g_mq_conf.data_file_path, qname, index);                     \
}

#define GEN_DATA_FULL_PATH_BY_QNAME_INDEX(full_name, qname, index)       \
{                                                                        \
    snprintf(full_name, MAX_FULL_FILE_NAME_LEN, "%s/%s/db_%012"PRIu64,   \
            g_mq_conf.data_file_path, qname, index);                     \
}

#define GEN_DATA_FULL_PATH_BY_FNAME(full_name, qname, fname)             \
{                                                                        \
    snprintf(full_name, MAX_FULL_FILE_NAME_LEN, "%s/%s/%s",              \
            g_mq_conf.data_file_path, qname, fname);                     \
}

//////////////////////////////////////////////////////////////////////////

char* mq_sm_open_db_file(int* fd, const char* db_fname, int open_flag);
bool touch_data_path(const char* path);
bool find_handle_file(queue_file_t *queue_file, char *qname);
bool get_next_read_file(mq_queue_t *mq_queue);
bool get_next_write_file(mq_queue_t *mq_queue);

//////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif /* __MQ_STORE_FILE_H__ */
