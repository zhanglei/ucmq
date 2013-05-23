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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <dirent.h>

#include "mq_evhttp.h"
#include "mq_queue_manage.h"

/////////////////////////////////////////////////////////////////////////////////

mq_queue_list_t* g_mq_qlist;

/////////////////////////////////////////////////////////////////////////////////

static bool init_queue(mq_queue_t *mq_queue);
static bool mq_qm_db_sync(mq_queue_t *mq_queue);

////////////////////////////// hash ///////////////////////////////////////////////////

/* Add queue into hash table */
mq_queue_t* hash_add_queue_kv(const char* qname, mq_queue_t* queue)
{
    int qname_len = 0;
    mq_queue_list_t* tmp_queue = NULL;

    qname_len = strlen(qname);
    if (qname_len < 0)
    {
        log_warn("Add hash fail, Invalid key");
        return false;
    }

    tmp_queue = (mq_queue_list_t*)malloc(sizeof(mq_queue_list_t));
    if (tmp_queue == NULL)
    {
        log_warn("Add hash fail, errno[%d], error msg[%s]", errno, strerror(errno));
        return false;
    }
    memset(tmp_queue, '\0', sizeof(mq_queue_list_t));
    /* Key setvar */
    memcpy(tmp_queue->qname, qname, strlen(qname));
    /* Value setvar */
    memcpy(&(tmp_queue->mq_queue), queue, sizeof(mq_queue_t));
    /* Qname: name of key field */
    HASH_ADD_STR(g_mq_qlist, qname, tmp_queue);  
    
    return &tmp_queue->mq_queue;
}

/* Print queue hash table */
void hash_print_table()
{
    int count = 0;
    mq_queue_list_t* tmp_queue;

    for(tmp_queue = g_mq_qlist; tmp_queue != NULL; tmp_queue = tmp_queue->hh.next) 
    {
        log_debug("Hash table key name [%s]\n", tmp_queue->qname);
        count++;
    }
    log_debug("-----Hash table numbers[%d]-----\n", count);
}

/* Get count from hash table */
int hash_get_queue_num()
{
    int queue_num;
    queue_num = HASH_COUNT(g_mq_qlist);
    return queue_num;
}

/* Finde specify key from hash table */
mq_queue_t* hash_find_queue_by_key(const char* qname)
{ 
    mq_queue_list_t* tmp_queue = NULL;
    /* Queue: output pointer */
    HASH_FIND_STR(g_mq_qlist, qname, tmp_queue);
    if (tmp_queue != NULL)
    {
        return &tmp_queue->mq_queue;
    }
    return NULL;
}

/* Delete specify queue by queue name from hash table */
void hash_del_queue(const char* qname)
{
    mq_queue_list_t* tmp_queue = NULL;

    HASH_FIND_STR(g_mq_qlist, qname, tmp_queue);
    if (tmp_queue != NULL)
    {
        log_debug("Del queue [%s]\n", tmp_queue->qname);
        HASH_DEL(g_mq_qlist, tmp_queue); /* user: pointer to deletee */
        free(tmp_queue);                     /* optional; it¡¯s up to you! */
    }
}

/* Clear queue list hash table */
bool hash_clear_queue()
{
    mq_queue_list_t* cur = NULL;
    mq_queue_list_t* tmp = NULL;

    if (g_mq_qlist == NULL)
    {
        return true;
    }
    HASH_ITER(hh, g_mq_qlist, cur, tmp)
    {
        HASH_DEL(g_mq_qlist, cur);
        free(cur);
    }   
    if (g_mq_qlist != NULL)
    {
        free(g_mq_qlist);
    }
    return true;
}   

/////////////////////////////////////////////////////////////////////////////////

bool mq_qm_open_store(void)
{
    DIR *p_dir;
    struct dirent* ent = NULL;
    queue_file_t queue_file;

    /* Touch data path */
    if (!touch_data_path(g_mq_conf.data_file_path))
    {
        log_error("Cann't find and creat data dir [%s] fail", g_mq_conf.data_file_path);
        return false;
    }

    /* Open data path */
    if ((p_dir = opendir(g_mq_conf.data_file_path)) == NULL)
    {
        log_error("Cann't open DB directory:[%s]", g_mq_conf.data_file_path);
        return false;
    }

    while ((ent = readdir(p_dir)) != NULL)
    {
        if (ent->d_type != DT_DIR)
        {
            continue;
        }
        if (strncmp(ent->d_name, ".", 1) == 0)
        {
            continue;
        }
        log_info( "--- Open [%s]'s Store file ---", ent->d_name);

        mq_queue_list_t tmp_queue;
        memset(&tmp_queue, '\0', sizeof(mq_queue_list_t));
        memset(&queue_file, '\0', sizeof(queue_file_t));

        strncpy(tmp_queue.qname, ent->d_name, sizeof(tmp_queue.qname) - 1);
        strncpy(tmp_queue.mq_queue.qname, ent->d_name, sizeof(tmp_queue.qname) - 1);

        /* Find handle file for one queue */
        if(!find_handle_file(&queue_file, tmp_queue.qname))
        {
            log_warn("Can't find queue [%s] Store to open", ent->d_name);
            continue;
        }

        if (!mq_sm_rtag_open_file(&tmp_queue.mq_queue, &queue_file))
        {
            log_warn("Can't open rtag file [%s]", queue_file.rtag_fname);
            continue;
        }

        if (!mq_sm_rtag_read_item(&tmp_queue.mq_queue))
        {
            log_warn("Can't read rtag [%s]'s item", queue_file.rtag_fname);
            continue;
        }

        /* Open data file of one queue */
        if(!mq_sm_open_db(&tmp_queue.mq_queue, &queue_file))
        {
            log_warn( "Can't open queue [%s]'s DB file", ent->d_name);
            continue;
        }
        hash_add_queue_kv(ent->d_name, &tmp_queue.mq_queue);
    }/* end while */
    closedir(p_dir);

    log_info( "--- queue array num[%d] ---", hash_get_queue_num()); //fixme
    hash_print_table();

    return true;
}

bool mq_qm_close_store(void)
{
    mq_queue_list_t* tmp_queue = NULL;

    for(tmp_queue = g_mq_qlist; tmp_queue != NULL; tmp_queue = tmp_queue ->hh.next) 
    {
        if (!mq_sm_rtag_close_file(&tmp_queue->mq_queue))
        {
            log_warn( "Close queue [%s] rtage file fail",
                    tmp_queue->qname);
        }
        if (!mq_sm_close_db(&tmp_queue->mq_queue))
        {
            log_warn( "Close queue [%s] data file fail",
                    tmp_queue->qname);
        }
    }
    hash_clear_queue();

    return true;
}

mq_queue_t* mq_qm_find_queue(const char *qname)
{
    mq_queue_t* tmp_queue = NULL;

    tmp_queue = hash_find_queue_by_key(qname);
    if (tmp_queue != NULL)
    {
        log_debug("Find queue [%s] from queue list", tmp_queue->qname);
        return tmp_queue;
    }

    return NULL;
}

mq_queue_t* mq_qm_add_queue(const char *qname)
{
    mq_queue_t mq_queue;
    mq_queue_t* dest_queue = NULL;

    if (qname == NULL)
    {
        log_info("Add queue error, Invalide name");
        return false;
    }

    log_info("Queue items limit[%d], Queue list num [%d]",g_mq_conf.max_qlist_itmes, hash_get_queue_num()); 
    if (g_mq_conf.max_qlist_itmes <= hash_get_queue_num())
    {
        log_warn("Creat queue fail, Queue list [%d] is full", hash_get_queue_num()); 
        return false;
    }

    memset(&mq_queue, '\0', sizeof(mq_queue_t));
    strncpy(mq_queue.qname, qname, MAX_QUEUE_NAME_LEN);

    if(!mq_sm_creat_db(&mq_queue))
    {
        log_warn("Creat queue[%s] fail, errno [%d], errno msg [%s]", 
                mq_queue.qname, errno, strerror(errno));
        return false;
    }
    init_queue(&mq_queue);
    dest_queue = hash_add_queue_kv(qname, &mq_queue);
    log_info("Creat queue[%s] db dir and db file ... success", mq_queue.qname);

    hash_print_table();

    return dest_queue;
}

bool mq_qm_del_queue(const char *qname)
{
    mq_queue_t* tmp_queue = NULL;

    tmp_queue = hash_find_queue_by_key(qname);

    if (!mq_sm_close_db(tmp_queue))
    {
        log_warn("Queue [%s] store closer ... fail", qname);
        return false;
    }
    if (!mq_sm_remove_db(tmp_queue))
    {
        log_warn("Queue [%s] remove ... fail", qname);
        return false;
    }

    hash_del_queue(qname);
    log_info("Remove queue [%s] ... Ok", qname);

    return true;
}

int mq_qm_push_item(mq_queue_t *mq_queue, msg_item_t *msg_item)
{
    uint32_t     cur_time;
    bool         result = false;

    msg_item->magic_num1 = MSG_HEAD_OF_FIRST_MAGIC_NUM;
    msg_item->magic_num2 = MSG_HEAD_OF_SECOND_MAGIC_NUM;

    /* then def_max_queue is equal to 0 no limit */
    if (mq_queue->maxque != 0)
    {
        if (mq_queue->unread_count + 1 > mq_queue->maxque)
        {
            log_debug("Put error, uread [%d], Is full", mq_queue->unread_count);
            return QUEUE_FULL;
        }
    }

    /* If queue set wlock don't to write msg */
    cur_time = get_cur_timestamp();
    if (mq_queue->wlock == -1 || mq_queue->wlock > cur_time)
    {
        log_debug("Put error, Queue is read only");
        return QUEUE_WLOCK;
    }

    msg_item->delay = cur_time;
    log_debug("Msg current time[%u], Msg bady [%s]",msg_item->delay, msg_item->msg);

    result = mq_sm_put_data(msg_item, mq_queue);
    if (!result)
    {
        log_debug("Put error");
        return QUEUE_PUT_ERROR;
    }
    if (!mq_sm_rtag_write_item(mq_queue))
    {
        log_warn("Queue [%s] rtag sync ... fail", mq_queue->qname);
    }

    /* Every sync interval be sync rtag info */ 
    if (mq_queue->sync_intv > 0)
    {
        if (mq_queue->sync_intv == 1)
        {
            mq_qm_db_sync(mq_queue);
            mq_queue->sync_intv = (g_mq_conf.sync_interval <= 0) ? 10 : g_mq_conf.sync_interval;
        }
        else
        {
            mq_queue->sync_intv--;
        }
    }
    log_debug("Put ...OK");

    return QUEUE_PUT_OK;
}

int mq_qm_pop_item(mq_queue_t *mq_queue, msg_item_t *msg_item)
{
    msg_item->magic_num1 = MSG_HEAD_OF_FIRST_MAGIC_NUM;
    msg_item->magic_num2 = MSG_HEAD_OF_SECOND_MAGIC_NUM;

    /* Check queue isn't empty */
    if (mq_queue->unread_count < 1)
    {
        log_debug("Get msg error, Queue is empty");
        return QUEUE_GET_END;
    }

    /* Read message */
    if(!mq_sm_get_data(msg_item, mq_queue))
    {
        log_debug("Get message error, Queue is empty");
        return QUEUE_GET_END;
    }

    /* Note rtag info */
    if (!mq_sm_rtag_write_item(mq_queue))
    {
        log_warn("Queue [%s] rtag sync ... fail", mq_queue->qname);
    }

    return QUEUE_GET_OK;
}

bool mq_qm_set_maxqueue(mq_queue_t *mq_queue, uint32_t dest_max_queue_size)
{
    if ((mq_queue == NULL))
    {
        log_info("Set [%s] max queue fail; Maybe queue is not exist", mq_queue->qname);
        return  false;
    }
    if (dest_max_queue_size < 0)
    {
        log_info("Set [%u] max queue fail, Size is invaild", dest_max_queue_size);
        return  false;
    }

    mq_queue->maxque = dest_max_queue_size;
    log_info("Set max queue (%s = %u) ... Ok", mq_queue->qname, dest_max_queue_size);
    if (!mq_sm_rtag_write_item(mq_queue))
    {
        log_info("Queue [%s] rtag sync ... fail", mq_queue->qname);
    }
    return true;
}

bool mq_qm_set_delay(mq_queue_t *mq_queue, uint32_t dest_delay)
{
    if((mq_queue == NULL))
    {
        log_warn("set [%s] delay fail, Maybey queue is not exist", mq_queue->qname);
        return  false;
    }
    if(dest_delay >= 0)
    {
        mq_queue->delay = dest_delay; 
        log_info("Set delay[%s = %u]...Ok", mq_queue->qname, dest_delay);
        if (!mq_sm_rtag_write_item(mq_queue))
        {
            log_info("Queue [%s] rtag sync ... fail", mq_queue->qname);
        }
        return true;
    }
}

bool mq_qm_set_wlock(mq_queue_t *mq_queue, uint32_t wlock)
{
    int cur_time;

    if ((mq_queue == NULL))
    {
        log_warn("Set [%s] write lock fail; Maybe queue is not exist", mq_queue->qname);
        return  false;
    }

    /*
     * If wlock dest value equal to "1", this queue will be wlock forever,
     * until set it other value.
     */
    if (wlock == -1) 
    {   
        mq_queue->wlock = -1; 
        log_info("Set write lock [%s] forever...Ok", mq_queue->qname);
    }   
    else if (wlock == 0)
    {   
        mq_queue->wlock = 0;  
        log_info("Unlock queue write [%s] ...Ok", mq_queue->qname);
    }   
    /* this queue will be wlock on number of value seconds */
    else if (wlock > 0)
    {   
        cur_time = get_cur_timestamp();
        mq_queue->wlock = cur_time + wlock; 
        log_info("Set write locak [%s] [%ld]...Ok", mq_queue->qname, wlock);
    }   
    else 
    {   
        log_info("Set write lock [%s] ...error, Inavlid arg", mq_queue->qname);
        return false;
    }   
    if (!mq_sm_rtag_write_item(mq_queue))
    {
        log_info("Queue [%s] rtag sync ... fail", mq_queue->qname);
    }
    return true;
}

bool mq_qm_set_synctime(int sync_interval)
{
    if(sync_interval >= 0)
    {    
        log_info("Set data sync time (%d) ... Ok", sync_interval);
        g_mq_conf.sync_time_interval = sync_interval;
        return true;
    }    
    return false;
}

void mq_qm_sync_store(void)
{
    int ret;
    mq_queue_list_t* tmp_queue;

    log_debug("All number [%d] queue's rtag are syncing ...", hash_get_queue_num());

    for(tmp_queue = g_mq_qlist; tmp_queue != NULL; tmp_queue = tmp_queue->hh.next) 
    {
        ret = fsync(tmp_queue->mq_queue.rtag_fd);
        if(ret < 0)
        {
            log_warn("Fsync [%s]'s rtag file ... fail, errno[%d], error msg[%s]",
                    tmp_queue->mq_queue.qname, errno, strerror(errno));
        }

        /* Regular sync read data file map */
        if (tmp_queue->mq_queue.cur_wdb.map_mem == NULL)
        {
            log_warn("Current write file map sync error, Map is NULL");
            break;
        }

        ret = msync(tmp_queue->mq_queue.cur_wdb.map_mem, tmp_queue->mq_queue.cur_wdb.pos, MS_SYNC);
        if (ret < 0)
        {
            log_warn("Msync [%s]'s write data file ... fail, errno[%d], error msg[%s]",
                    tmp_queue->mq_queue.qname, errno, strerror(errno));
        }

        ret = fsync(tmp_queue->mq_queue.cur_wdb.fd);
        if(ret < 0)
        {
            log_warn("Fsync [%s]'s write data file ... fail, errno[%d], error msg[%s]",
                    tmp_queue->mq_queue.qname, errno, strerror(errno));
        }
    }
}

uint64_t mq_qm_get_store_count(void)
{
    uint64_t all_count = 0;
    uint64_t queue_count = 0;
    mq_queue_list_t* tmp_queue = NULL;

    log_debug("All number [%d] queue's get count...", hash_get_queue_num());

    for(tmp_queue = g_mq_qlist; tmp_queue != NULL; tmp_queue = tmp_queue->hh.next) 
    {
        queue_count = mq_sm_get_msg_count(&tmp_queue->mq_queue);
        log_debug("Queue name[%s], count[%d]", &tmp_queue->mq_queue.qname, queue_count);

        if(queue_count < 0)
        {
            log_warn("Queue [%s] get unread ... fail, please chacke db file",
                    tmp_queue->mq_queue.qname);
            queue_count = 0;
        }
        all_count += queue_count;
    }
    log_debug("Store unread msg items[%"PRIu64"]", all_count);

    return all_count;
}

/////////////////////////////////////////////////////////////////////////////////

static bool mq_qm_db_sync(mq_queue_t *mq_queue)
{
    int ret;

    /* Regular sync read data file map */
    if (mq_queue->cur_wdb.map_mem == NULL)
    {
        log_warn("invlid write map porint ");
    }

    ret = msync(mq_queue->cur_wdb.map_mem, mq_queue->cur_wdb.pos, MS_SYNC);
    if (ret < 0)
    {
        log_warn("Queue [%s] msync ... fail, errno[%d], error msg[%s]",
                mq_queue->qname, errno, strerror(errno));
        return false;
    }

    return true;
}

static bool init_queue(mq_queue_t *mq_queue)
{
    mq_queue->cur_rdb.pos       = DB_FILE_HEAD_LEN;
    mq_queue->cur_wdb.pos       = DB_FILE_HEAD_LEN;
    mq_queue->cur_rdb.opt_count = 0;
    mq_queue->cur_wdb.opt_count = 0;
    mq_queue->delay             = g_mq_conf.def_delay;
    mq_queue->maxque            = g_mq_conf.def_max_queue;
    mq_queue->put_num           = 0;
    mq_queue->get_num           = 0;
    mq_queue->unread_count      = 0;
    mq_queue->wlock             = 0;
    return true;
}
