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

#include "mq_util.h"
#include "mq_config.h"
#include "mq_store_rtag.h"
#include "mq_store_file.h"

//////////////////////////////////////////////////////////////////////////

#define STR_TO_LL(dest, str, len)                \
{                                                 \
    uint32_t num = 0;                             \
    if (str_to_num(&num, str, len))               \
    {                                             \
        dest = num;                               \
    }                                             \
    else                                          \
    {                                             \
        return false;                             \
    }                                             \
}

//////////////////////////////////////////////////////////////////////////

static bool str_to_num(uint32_t* num, const char* str, int len);
static bool check_rtag_item(mq_queue_t *mq_queue);
static bool parse_rtag_item(mq_queue_t *mq_queue, char *rtag_str);

//////////////////////////////////////////////////////////////////////////

/* Open next rtag file */
int mq_sm_rtag_open_next_file(const char* file_name)
{
    int fd;

    fd = open(file_name, O_RDWR | O_CREAT | O_NONBLOCK, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        log_debug("Can't open rtag file[%s],errno [%d] error message[%s]",
                file_name, errno, strerror(errno));
        return -1;
    }

    return fd;
}

/* Open rtag file */
bool mq_sm_rtag_open_file(mq_queue_t *mq_queue, queue_file_t *queue_file)
{
    int     fd;
    char    rtag_file_name[MAX_FULL_FILE_NAME_LEN + 1];

    GEN_DATA_FULL_PATH_BY_FNAME(rtag_file_name, mq_queue->qname, queue_file->rtag_fname);
    log_debug("Open rtag file for read item [%s]", rtag_file_name);

    /* Check rtag file size */
    if (get_file_size(rtag_file_name) <= 0)
    {
        log_debug("Rtag file is empty!");
        return false;
    }

    fd = open(rtag_file_name, O_RDWR | O_NONBLOCK | O_APPEND, S_IRUSR | S_IWUSR);
    if(fd < 0)
    {
        log_warn("Rtag file open fail, errno[%d], error msg[%s]",
                errno, strerror(errno));
        return false;
    }
    mq_queue->rtag_fd = fd;
    return true;
}

/* Close rtag file */
bool mq_sm_rtag_close_file(mq_queue_t *mq_queue)
{
    if (mq_queue->rtag_fd < 0)
    {
        return true;
    }
    if (fsync(mq_queue->rtag_fd) < 0)
    {   
        log_warn("Sync [%s] data file ... fail, errno[%d], error msg[%s]",
                mq_queue->qname, errno, strerror(errno));
    }   
    close(mq_queue->rtag_fd);

    return true;
}

bool mq_sm_rtag_read_item(mq_queue_t *mq_queue)
{
    int     i = 1;
    int     offset;
    int     read_len;
    int     read_interval;                                   /* Displacement interval */
    char    line[2 * (RTAG_ITEM_LEN + 1) + 1] = {'\0'};      /* ..rtag_item..\n..rtag_item..\n */

    /* 
     * Move the read position at last line,
     * If read line error then move the position in to prior line
     */
    do
    {
        read_interval = -(i * (RTAG_ITEM_LEN + 1));  
        offset = lseek(mq_queue->rtag_fd, read_interval, SEEK_END);
        if (offset < RTAG_FILE_HEAD_LEN)
        {
            log_debug("Real len is no enough");
            break;
        }

        log_debug("Rtag file read_interval [%d], offset[%d]", read_interval, offset);
        if ((read_len = read_n(mq_queue->rtag_fd, FD_UNKNOWN, &line, 2 * (RTAG_ITEM_LEN + 1), READ_OPT_TIMEOUT)) >= 0)
        {
            if (RTAG_ITEM_LEN + 1 <= read_len)
            {
                log_debug("Rtag item [%s], read len [%d]", line, read_len);
                if (parse_rtag_item(mq_queue, line))
                {
                    log_debug("Rtag item parse success, Read line length [%d], source line [%s]",
                            read_len, line);
                    /* If the last line parse failure, skip this db file to avoid coverage */
                    if (i > 1)
                    {
                        mq_queue->cur_wdb.pos = MAX_DB_FILE_SIZE;
                    }
                    return true;
                }
            }
            log_warn("Rtag item read fail, read len [%d], error msg[%s]", read_len, line);
            i++;
            continue;
        }
        else
        {
            log_warn("Rtag file read fail, errno[%d], error msg[%s]", errno, strerror(errno));
            return false;
        }
    }while(1);

    mq_queue->cur_rdb.opt_count = 0;
    mq_queue->cur_rdb.pos       = 0;
    mq_queue->cur_wdb.pos       = MAX_DB_FILE_SIZE;
    mq_queue->maxque            = g_mq_conf.def_max_queue;
    mq_queue->delay             = g_mq_conf.def_delay;
    mq_queue->wlock             = 0;

    return true;
}

bool mq_sm_rtag_write_item(mq_queue_t *mq_queue)
{
    int     ret = 0;
    int     buf_size;
    char    rtag_buf[RTAG_ITEM_LEN + 2];

    buf_size = sprintf(rtag_buf, "%010u%010u%010u%010u%010u%010u\n",
            mq_queue->cur_rdb.opt_count,
            mq_queue->cur_rdb.pos,
            mq_queue->cur_wdb.pos,
            mq_queue->maxque,
            mq_queue->delay,
            mq_queue->wlock);
    log_debug("Write [%s]'s rtag, Rtag item: [%s], Write size[%d]", 
            mq_queue->qname, rtag_buf, buf_size);

    ret = write_n(mq_queue->rtag_fd, FD_UNKNOWN, rtag_buf, buf_size, READ_OPT_TIMEOUT);  
    if (ret < 0)
    {
        log_warn("Rtag write error, errno[%d] , errno msg[%s]", errno, strerror(errno));
        return false;
    }
    else if (ret == 0)
    {
        log_warn("Rtag write error, End of file");
        return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////

static bool check_rtag_item(mq_queue_t *mq_queue)
{
    bool ret = false;

    do
    {
        if (mq_queue->cur_rdb.opt_count > (MAX_DB_FILE_SIZE / MSG_HEAD_LEN) 
                || mq_queue->cur_rdb.opt_count < 0)
        {
            log_debug("Parse rtag item (rcount) error");
            break;
        }
        /* if read position is invaild, read this data file again from file head */
        if (mq_queue->cur_rdb.pos > MAX_DB_FILE_SIZE || mq_queue->cur_rdb.pos < 0)
        {
            log_debug("Parse rtag item (rpos) error");
            break;
        }
        /* if write position is invaild ,get next write data file */
        if (mq_queue->cur_wdb.pos > MAX_DB_FILE_SIZE || mq_queue->cur_wdb.pos < 0)
        {
            log_debug("Parse rtag item (wpos) error");
            break;
        }
        if (mq_queue->maxque > UINT_MAX || mq_queue->maxque < 0)
        {
            log_debug("Parse rtag item (max queue) error");
            break;
        }
        if (mq_queue->delay < 0)
        {
            log_debug("Parse rtag item (delay) error");
            break;
        }
        if (mq_queue->wlock > INT_MAX)
        {
            log_debug("Parse rtag item (write lock) error");
            break;
        }
        ret = true;
        return ret;
    }while(1);

    return ret;
}

static bool str_to_num(uint32_t* num, const char* str, int len)
{
    if (num == NULL || str == NULL || len <= 0)
    {   
        return false;
    }   

    *num = 0;
    int i;
    for(i = 0; i < len; i++)
    {   
        if (str[i] >= '0' && str[i] <= '9')
        {   
            *num = *num * 10 + (str[i] - '0');
        }   
        else
        {   
            log_debug("str invalid[%c]", str[i]);
            return false;
        }   
    }   
    log_debug("dest num[%u]", *num);
    return true;
}

/* Parse rtag item */
static bool parse_rtag_item(mq_queue_t *mq_queue, char *rtag_str)
{
    char    *str1;
    char    *token;

    str1  = rtag_str;
    token = strtok(str1, "\n"); 
    while(token != NULL)
    {   
        log_debug("Token len [%d], token [%s]", strlen(token), token);
        if (strlen(token) >= RTAG_ITEM_LEN)
        {
            //rtag_item = (rtag_item_t *)token;
            char* str;
            str = token;

            STR_TO_LL(mq_queue->cur_rdb.opt_count, str, RCOUNT_LEN);
            str += RCOUNT_LEN;             
            STR_TO_LL(mq_queue->cur_rdb.pos, str, RPOS_LEN);
            str += RPOS_LEN;               
            STR_TO_LL(mq_queue->cur_wdb.pos, str, WPOS_LEN);
            str += WPOS_LEN;               
            STR_TO_LL(mq_queue->maxque, str, QUEUE_MAX_SIZE_LEN);
            str += QUEUE_MAX_SIZE_LEN;     
            STR_TO_LL(mq_queue->delay, str, DELAY_LEN);
            str += DELAY_LEN;     
            STR_TO_LL(mq_queue->wlock, str, WLOCK_LEN);

            if(check_rtag_item(mq_queue))
            {
                log_debug("rcount [%u] rpos [%u] wpos[%u] max queue [%u] delay [%u] wlock [%d]",
                        mq_queue->cur_rdb.opt_count,
                        mq_queue->cur_rdb.pos,
                        mq_queue->cur_wdb.pos,
                        mq_queue->maxque,
                        mq_queue->delay,
                        mq_queue->wlock);
                return true;
            }
        }
        token = strtok(NULL, "\n");
    }
    return false;
}
