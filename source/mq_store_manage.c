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

#include <unistd.h>

#include "mq_util.h"
#include "mq_config.h"
#include "mq_store_file.h"
#include "mq_store_rtag.h"
#include "mq_store_msg.h"
#include "mq_store_manage.h"

/////////////////////////////////////////////////////////////////////////////////

static bool mq_sm_close_db_file(void* map_mem, int fd);

/////////////////////////////////////////////////////////////////////////////////

//char *asc_trace(char *buffer,int lg); //for test

/////////////////////////////////////////////////////////////////////////////////

bool mq_sm_put_data(msg_item_t *msg_item, mq_queue_t *mq_queue)
{
    if (!mq_sm_db_build_msg_crc16(&msg_item->crc16, msg_item->msg, msg_item->len))
    {
        log_info("calculate message's crc16 error");
        return false;
    }
    log_debug("msg crc16[%u]", msg_item->crc16);

    //log_trace("mq_sm_put_data, Message body [%s]", asc_trace(msg_item->msg, msg_item->len)); //for test
    if(!mq_sm_db_write_msg(mq_queue, msg_item))
    {
        log_info("write mssage fail");
        return false;
    }

    return true;
}

bool mq_sm_get_data(msg_item_t* msg_item, mq_queue_t* mq_queue)
{
    int       ret;
    int       cur_timestemp;

    while (1)
    {
        log_debug("Read index [%"PRIu64"] rpos: [%u], write index [%"PRIu64"] wpos: [%u]",
                mq_queue->cur_rdb.cur_index,
                mq_queue->cur_rdb.pos,
                mq_queue->cur_wdb.cur_index,
                mq_queue->cur_wdb.pos);

        /* Is read end of queue */
        if (mq_queue->cur_rdb.cur_index == mq_queue->cur_wdb.cur_index)
        {
            if (mq_queue->cur_rdb.pos + MSG_HEAD_LEN >= mq_queue->cur_wdb.pos)
            {
                log_info("Read mssage in to queue end");
                mq_queue->cur_rdb.pos  = mq_queue->cur_wdb.pos;
                mq_queue->unread_count = 0;
                return false; //httpcode 404
            }
        }

        /* Check msg */
        ret = mq_sm_db_parse_msg(mq_queue->cur_rdb.map_mem, mq_queue->cur_rdb.pos, msg_item);
        if (ret == MSG_CHECK_RET_OK)
        {
            /* Check the msg delay time, If delay equal '0' don't delay */
            cur_timestemp = get_cur_timestamp();
            log_debug("The time difference between current time and msg write time [%d]",
                    cur_timestemp - msg_item->delay);
            if ((mq_queue->delay != 0) && (cur_timestemp - msg_item->delay < mq_queue->delay))
            {
                log_debug("Mssage can't be read in this time");
                return false;  //httpcode 404
            }

            mq_queue->cur_rdb.pos       += (msg_item->len + MSG_HEAD_LEN);
            mq_queue->cur_rdb.opt_count += 1;
            mq_queue->get_num           += 1;
            mq_queue->unread_count      -= 1;

            return true;
        }
        else if (ret == MSG_CHECK_RET_MAG_NUM_ERR)
        {
            mq_queue->cur_rdb.pos++; 
            continue;
        }
        else if (ret == MSG_CHECK_RET_PARSE_ERR || ret == MSG_CHECK_RET_CRC_ERR)
        {
            mq_queue->cur_rdb.pos += 2; 
            continue;
        }
        else if (ret ==  MSG_CHECK_RET_MSG_ERR)
        {
            log_warn("DB file mmap is NULL");
            if (!get_next_read_file(mq_queue))
            {
                log_warn("Find new DB file fail");
                return false; //httpcode 404
            }
            continue;
        }
        else if (ret == MSG_CHECK_RET_LEN_BEY)
        {
            if (!get_next_read_file(mq_queue))
            {
                log_warn("Find new DB file fail");
                return false; //httpcode 404
            }
            continue;
        }
        else
        {
            mq_queue->cur_rdb.pos++; 
            continue;
        }
    }//exit while
}

static bool mq_sm_close_db_file(void* map_mem, int fd)
{
    /* msync synchronization(MS_SYNC) or asynchronous(MS_ASYNC) */
    if (map_mem != NULL)
    {
        msync(map_mem, MAX_DB_FILE_SIZE, MS_SYNC); 
        munmap(map_mem, MAX_DB_FILE_SIZE);
    }
    if(fsync(fd) < 0)
    {   
        log_warn("Sync data file fail, errno[%d], error msg[%s]", errno, strerror(errno));
    }   
    close(fd);

    return true;
}

/* Open read/write file */
bool mq_sm_open_db(mq_queue_t *mq_queue, queue_file_t *queue_file)
{
    char read_db_fname[MAX_FULL_FILE_NAME_LEN + 1];
    char write_db_fname[MAX_FULL_FILE_NAME_LEN + 1];

    mq_queue->cur_rdb.cur_index = queue_file->read_fid;
    mq_queue->cur_wdb.cur_index = queue_file->write_fid;

    log_info("Read db fid [%"PRIu64"], Write db fid [%"PRIu64"]", 
            mq_queue->cur_rdb.cur_index, mq_queue->cur_wdb.cur_index);

    GEN_DATA_FULL_PATH_BY_FNAME(write_db_fname, mq_queue->qname, queue_file->write_fname);

    if (mq_queue->cur_wdb.flag  != 0)
    {
        log_warn("Write file [%s] already mmap", queue_file->write_fname);
        return false;
    }

    /* Open write db file and mmap it */
    if((mq_queue->cur_wdb.map_mem 
                = mq_sm_open_db_file(&mq_queue->cur_wdb.fd, write_db_fname, FOPEN_FLAG_OPEN))
            == NULL)
    {
        log_warn("Open data file[%s] fail", write_db_fname);
        return false;
    }

    /* If read file also a write file */
    if (mq_queue->cur_rdb.cur_index == mq_queue->cur_wdb.cur_index)
    {
        log_info("Read mmap file same to write [%s]", write_db_fname);
        mq_queue->cur_rdb.map_mem = mq_queue->cur_wdb.map_mem; 
        mq_queue->cur_rdb.fd      = mq_queue->cur_wdb.fd;
    }
    else
    {
        GEN_DATA_FULL_PATH_BY_FNAME(read_db_fname, mq_queue->qname, queue_file->read_fname);

        if (mq_queue->cur_rdb.flag  != 0)
        {
            mq_sm_close_db_file(mq_queue->cur_wdb.map_mem, mq_queue->cur_wdb.fd);
            log_warn("Read file [%s] already mmap", read_db_fname);
            return false;
        }

        /* Open read db file and mmap it */
        if((mq_queue->cur_rdb.map_mem
                    = mq_sm_open_db_file(&mq_queue->cur_rdb.fd, read_db_fname, FOPEN_FLAG_OPEN))
                == NULL)
        {
            mq_sm_close_db_file(mq_queue->cur_wdb.map_mem, mq_queue->cur_wdb.fd);
            log_warn("Open read data file[%s] fail", read_db_fname);
            return false;
        }
    }
    mq_queue->cur_wdb.flag  = 1;
    mq_queue->cur_rdb.flag  = 1; /* mmaped */

    return true;
}

bool mq_sm_close_db(mq_queue_t *mq_queue)
{
    mq_queue->cur_rdb.flag = 0;  //unmap
    mq_queue->cur_wdb.flag = 0;

    if (mq_queue->cur_rdb.cur_index != mq_queue->cur_wdb.cur_index)
    {
        /* Close read db file, And unmap it */
        mq_sm_close_db_file(mq_queue->cur_rdb.map_mem, mq_queue->cur_rdb.fd);
    }
    /* Close write db file, And unmap it */
    mq_sm_close_db_file(mq_queue->cur_wdb.map_mem, mq_queue->cur_wdb.fd);

    return true;
}

bool mq_sm_creat_db(mq_queue_t *mq_queue)
{
    char    queue_db_path[MAX_DATA_PATH_NAME_LEN + 1];
    char    rtag_fname[MAX_FULL_FILE_NAME_LEN + 1];
    char    write_db_fname[MAX_FULL_FILE_NAME_LEN + 1];

    if (mq_queue->qname == NULL)
    {
        log_debug("Create queue error, Queue name is empty");
        return false;
    }
    GEN_QUEUE_PATH_BY_QNAME(queue_db_path, mq_queue->qname);
    log_debug("Creat DB directory:[%s]", queue_db_path);

    /* Check Storage Free */
    if (get_storage_free(g_mq_conf.data_file_path) < g_mq_conf.res_store_space)
    {
        log_error("No space left on device, free storage [%d]GB",
                get_storage_free(g_mq_conf.data_file_path));
        return false;
    }

    /* If exsit invalid same db path romve it */
    if ((access(queue_db_path, F_OK | R_OK | W_OK)) == 0)
    {
        delete_file(queue_db_path);
    }
    /* Creat data file directory, and map data file */
    if (mkdir(queue_db_path, 0770) != 0)
    {
        /* If the directory is not already exists */
        if (errno != EEXIST)
        {
            log_error( "Creat DB directory:[%s], errno [%d], error message[%s]",
                    queue_db_path, errno, strerror(errno));
            return false;
        }
    }

    GEN_RTAG_FULL_PATH_BY_QNAME_INDEX(rtag_fname, mq_queue->qname, (uint64_t)1); 
    GEN_DATA_FULL_PATH_BY_QNAME_INDEX(write_db_fname, mq_queue->qname, (uint64_t)1); 
    log_debug("Creat rtag:[%s] Creat db: [%s]", rtag_fname, write_db_fname);

    /* Creat rtag file, and first write */
    mq_queue->rtag_fd = open(rtag_fname, O_CREAT | O_RDWR | O_NONBLOCK, S_IRUSR | S_IWUSR);
    if (mq_queue->rtag_fd < 0)
    {
        log_warn("Creat rtag file [%s] ... fail,errno [%d] error message[%s]",
                rtag_fname, errno, strerror(errno));
        return false;
    }

    /* Open read db file and mmap it */
    if((mq_queue->cur_wdb.map_mem
                =  mq_sm_open_db_file(&mq_queue->cur_wdb.fd, write_db_fname, FOPEN_FLAG_CREATE))
            == NULL)
    {
        close(mq_queue->rtag_fd);
        log_warn("Open read data file[%s] fail", write_db_fname);
        return false;
    }

    mq_queue->maxque             = g_mq_conf.def_max_queue;
    mq_queue->delay              = g_mq_conf.def_delay;
    mq_sm_rtag_write_item(mq_queue);

    mq_queue->cur_wdb.cur_index  = 1;
    mq_queue->cur_wdb.flag       = 1;
    mq_queue->cur_rdb.cur_index  = mq_queue->cur_wdb.cur_index;
    mq_queue->cur_rdb.flag       = mq_queue->cur_wdb.flag;
    mq_queue->cur_rdb.map_mem    = mq_queue->cur_wdb.map_mem;
    mq_queue->cur_rdb.fd         = mq_queue->cur_wdb.fd;

    return true;
}

bool mq_sm_remove_db(mq_queue_t *mq_queue)
{
    char queue_db_path[MAX_DATA_PATH_NAME_LEN + 1];

    GEN_QUEUE_PATH_BY_QNAME(queue_db_path, mq_queue->qname); 

    /* If exsit invalid same db path romve it */
    if ((access(queue_db_path, F_OK | R_OK | W_OK)) == 0)
    {
        delete_file(queue_db_path);
        return true;
    }

    return false;
}

/* Return sum of all db file head note themself count */
uint64_t mq_sm_get_msg_count(mq_queue_t *mq_queue)
{
    int         fd;
    DIR         *p_dir;
    int         read_len;
    int         db_pattern_len;
    uint64_t    all_db_count = 0;

    char        path[MAX_DATA_PATH_NAME_LEN + 1];
    char        db_file_pattern[MAX_DATA_FILE_NAME_LEN + 1];
    char        full_file_name[MAX_FULL_FILE_NAME_LEN + 1];
    char        line[DB_FILE_HEAD_OF_COUNT_LEN + 1];
    struct      dirent* ent = NULL;

    /* if not start mq server */
    if(mq_queue->unread_count != 0)
    {
        return mq_queue->unread_count;
    }

    GEN_QUEUE_PATH_BY_QNAME(path, mq_queue->qname);
    snprintf(db_file_pattern, MAX_DATA_FILE_NAME_LEN, "%s_", "db");
    db_pattern_len = strlen(db_file_pattern);

    if ((p_dir = opendir(path)) == NULL)
    {
        log_error( "Cannot open DB directory:[%s]\n", path);
        return -1;
    }

    while ((ent = readdir(p_dir)) != NULL)
    {
        if (ent->d_type != DT_REG)
        {
            continue;
        }
        if (strncmp(ent->d_name, ".", 1) == 0)
        {
            continue;
        }

        /* Find read/write data file head for calculate queue count */
        if (strncmp(ent->d_name, db_file_pattern, db_pattern_len) == 0)
        {
            GEN_DATA_FULL_PATH_BY_FNAME(full_file_name, mq_queue->qname, ent->d_name);
            log_debug( "DB full file name[%s]", full_file_name);
            fd = open(full_file_name, O_RDWR | O_NONBLOCK, S_IRUSR | S_IWUSR);
            if (fd < 0)
            {
                log_warn("Can't open read file:[%s]; errno[%d], error msg[%s]",
                        full_file_name, errno, strerror(errno));
                continue;
            }

            /* Skip db file version flag */
            if (lseek(fd, DB_FILE_HEAD_OF_VERSION_LEN, SEEK_SET) < 0)
            {
                log_debug( "Lseek db file name error [%s]", full_file_name);
                close(fd);
                continue;
            }
            memset(line, '\0', sizeof(line));
            if ((read_len = read_n(fd, FD_UNKNOWN, &line, DB_FILE_HEAD_OF_COUNT_LEN, READ_OPT_TIMEOUT)) >= 0)
            {
                int tmp_count = 0;

                log_debug("Data file msg count [%s], real len[%d]", line, read_len);
                tmp_count = atoi(line);
                if(tmp_count != 0 && tmp_count < (MAX_DB_FILE_SIZE / MSG_HEAD_LEN))
                {
                    all_db_count += tmp_count;
                    close(fd);
                    continue;
                }
                else
                {
                    if(mq_queue->cur_wdb.cur_index == mq_queue->cur_rdb.cur_index)
                    {
                        tmp_count = mq_sm_db_cal_msg_count(fd, mq_queue->cur_wdb.pos);
                    }
                    else
                    {
                        tmp_count = mq_sm_db_cal_msg_count(fd, MAX_DB_FILE_SIZE);
                    }
                    all_db_count += tmp_count;
                    log_debug("All db count [%"PRIu64"]", all_db_count);
                    close(fd);
                    continue;
                }
            }
            else
            {
                log_debug("Data file msg count read fail, errno[%d], error msg[%s]",
                        errno, strerror(errno));
                close(fd);
                continue;
            }
        }
    }//exit while
    closedir(p_dir);

    mq_queue->get_num      = mq_queue->cur_rdb.opt_count;
    mq_queue->put_num      = all_db_count;
    all_db_count           = (all_db_count >= mq_queue->cur_rdb.opt_count) ? all_db_count - mq_queue->cur_rdb.opt_count : 0;
    mq_queue->unread_count = all_db_count;
    log_info("Queue [%s] unread count[%"PRIu64"]", mq_queue->qname, mq_queue->unread_count);

    return all_db_count;
}
