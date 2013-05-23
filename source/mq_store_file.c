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

#include <inttypes.h>

#include "mq_store_file.h"
#include "mq_store_msg.h"
#include "mq_store_rtag.h"

//////////////////////////////////////////////////////////////////////////

static uint64_t get_id_by_fname(const char *qname);

//////////////////////////////////////////////////////////////////////////

bool touch_data_path(const char* path)
{
    if (path == NULL)
    {
        log_error("Invalid data path ");
        return false;
    }
    /* Touch data path */
    if ((access(path, F_OK | R_OK | W_OK)) != 0)
    {
        if (errno == EACCES)
        {
            log_error("[%s] has permission denied to handle, errno[%d], error message[%s]", 
                    path, errno, strerror(errno));
            return false;
        }
        else if (errno == ENOENT)
        {
            /* creat db file directory */
            if (mkdir(path, 0770) != 0)
            {
                log_error("Creat db path [%s] fail, errno[%d], error message[%s]", 
                        path, errno, strerror(errno));
                return false;
            }
        }
        else
        {
            log_error("UCMQ data path [%s] can't find and creat file, errno[%d], error message[%s]", 
                    path, errno, strerror(errno));
            return false;
        }
    }
    if (!is_dir(path))
    {
        log_error("DB path exist, But isn't dir");
        return false;
    }
    return true;
}

char* mq_sm_open_db_file(int* fd, const char* db_fname, int open_flag)
{
    int file_size = 0;
    int flag      =  O_RDWR | O_NONBLOCK;
    char* map_mem = NULL;

    if (open_flag == FOPEN_FLAG_CREATE)
    {
        flag |= O_CREAT;
    }

    /* db file open */
    *fd = open(db_fname, flag, S_IRUSR | S_IWUSR);
    if (*fd < 0)
    {
        log_warn("Can't open db file:[%s]; errno[%d], error msg[%s]",
                db_fname, errno, strerror(errno));
        return NULL;
    }

    /* If write data file size Is not big enough */
    if (open_flag == FOPEN_FLAG_CREATE)
    {
        /* extend file size by MAX_DB_FILE_SIZE */
        if (!extend_file_size(*fd, MAX_DB_FILE_SIZE))
        {
            log_info("Reszie file [%s] fail ,errno [%d] error message[%s]", 
                    db_fname, errno, strerror(errno));
            close(*fd);
            return false;
        }
        file_size = get_file_size(db_fname);

    }
    else
    {
        file_size = get_file_size(db_fname);
        if (file_size != MAX_DB_FILE_SIZE)
        {
            log_warn("Invalid \"db_file_max_size\" or data file is damaged");
            close(*fd);
            return NULL;
        }
    }

    /* Creat write data file mmap */
    map_mem = (char *)mmap(0, MAX_DB_FILE_SIZE, PROT_WRITE, MAP_SHARED, *fd, 0);
    if (map_mem == MAP_FAILED)
    {
        log_warn("can't mmap file [%s], errno[%d] error message[%s]",
                db_fname, errno, strerror(errno));
        close(*fd);
        return NULL;
    }

    log_info("Open data file[%s] success, file size [%d]", db_fname, file_size);
    return map_mem;
}

/* Get next read file */
bool get_next_read_file(mq_queue_t *mq_queue)
{
    uint64_t    next_index;
    int         old_db_file_fd;
    int         old_rtag_file_fd;
    char        *old_db_map_addr;
    char        new_db_fname[MAX_FULL_FILE_NAME_LEN + 1];
    char        new_rtag_fname[MAX_FULL_FILE_NAME_LEN + 1];
    char        old_db_fname[MAX_FULL_FILE_NAME_LEN + 1];
    char        old_rtag_fname[MAX_FULL_FILE_NAME_LEN + 1];

    log_debug("Current: read db id[%"PRIu64"], write db id[%"PRIu64"]", 
            mq_queue->cur_rdb.cur_index, mq_queue->cur_wdb.cur_index);

    /* If read of queue end */
    next_index = mq_queue->cur_rdb.cur_index + 1;
    if (next_index > mq_queue->cur_wdb.cur_index)
    {
        log_info("Get next read file fail, Get queue end");
        mq_queue->cur_rdb.cur_index = mq_queue->cur_wdb.cur_index;
        return false;
    }

    /* Current file name */
    GEN_RTAG_FULL_PATH_BY_QNAME_INDEX(old_rtag_fname, mq_queue->qname, mq_queue->cur_rdb.cur_index);
    GEN_DATA_FULL_PATH_BY_QNAME_INDEX(old_db_fname, mq_queue->qname, mq_queue->cur_rdb.cur_index);
    /* Get the next file name */
    GEN_RTAG_FULL_PATH_BY_QNAME_INDEX(new_rtag_fname, mq_queue->qname, next_index);
    GEN_DATA_FULL_PATH_BY_QNAME_INDEX(new_db_fname, mq_queue->qname, next_index);

    mq_queue->cur_rdb.flag   = 0;
    old_rtag_file_fd         = mq_queue->rtag_fd;
    old_db_file_fd           = mq_queue->cur_rdb.fd;
    old_db_map_addr          = mq_queue->cur_rdb.map_mem;

    /* 1. Open the next rtag file */
    if((mq_queue->rtag_fd = mq_sm_rtag_open_next_file(new_rtag_fname)) < 0)
    {
        log_warn("Get new rtag file fail, errno[%d], errno msg[%s]",
                errno, strerror(errno));
        return false;
    }

    /* 2. Init rtag info */
    mq_queue->cur_rdb.cur_index = next_index;
    mq_queue->cur_rdb.opt_count = 0;
    mq_queue->cur_rdb.pos       = DB_FILE_HEAD_LEN;
    /* 3. Write rtag info to the new file */
    mq_sm_rtag_write_item(mq_queue);
    /* 4. Close and remove old rtage file */
    close(old_rtag_file_fd);
    unlink(old_rtag_fname);

    /* 5. Munmap and close readed db file, And remove it */
    msync(old_db_map_addr, MAX_DB_FILE_SIZE, MS_SYNC); 
    munmap(old_db_map_addr, MAX_DB_FILE_SIZE);
    close(old_db_file_fd);
    unlink(old_db_fname);

    /* 6. Open the next read file, If cur db file be writing use it */
    if (next_index == mq_queue->cur_wdb.cur_index)
    {
        mq_queue->cur_rdb.map_mem = mq_queue->cur_wdb.map_mem;
        mq_queue->cur_rdb.fd      = mq_queue->cur_wdb.fd;
    }
    else
    {
        /* Open write db file and mmap it */
        if((mq_queue->cur_rdb.map_mem 
                    = mq_sm_open_db_file(&mq_queue->cur_rdb.fd, new_db_fname, FOPEN_FLAG_OPEN))
                == NULL)
        {
            log_warn("Open data file[%s] fail", new_db_fname);
            close(mq_queue->rtag_fd);
            return false;
        }
    }
    mq_queue->cur_rdb.flag = 1;

    return true;
}

/* Get next write file */
bool get_next_write_file(mq_queue_t *mq_queue)
{
    uint64_t    next_index;
    char        new_db_fname[MAX_FULL_FILE_NAME_LEN + 1];

    log_debug("Current: read db id[%"PRIu64"], write db id[%"PRIu64"]", 
            mq_queue->cur_rdb.cur_index, mq_queue->cur_wdb.cur_index);

    next_index = mq_queue->cur_wdb.cur_index + 1;
    GEN_DATA_FULL_PATH_BY_QNAME_INDEX(new_db_fname, mq_queue->qname, next_index);

    /* If cur db file not bee read */
    if (mq_queue->cur_wdb.cur_index > mq_queue->cur_rdb.cur_index)
    {
        /* Munmap and close readed db file */
        msync(mq_queue->cur_wdb.map_mem, MAX_DB_FILE_SIZE, MS_SYNC); 
        munmap(mq_queue->cur_wdb.map_mem, MAX_DB_FILE_SIZE);
        mq_queue->cur_wdb.flag = 0;
        close(mq_queue->cur_wdb.fd);
    }

    /* Check Storage Free */
    if (get_storage_free(g_mq_conf.data_file_path) < g_mq_conf.res_store_space)
    {
        log_error("No space left on device, free storage [%d]GB",
                get_storage_free(g_mq_conf.data_file_path));
        return false;
    }

    /* Creat a new write db file */
    if((mq_queue->cur_wdb.map_mem
                =  mq_sm_open_db_file(&mq_queue->cur_wdb.fd, new_db_fname, FOPEN_FLAG_CREATE))
            == NULL)
    {
        log_warn("Open read data file[%s] fail", new_db_fname);
        return false;
    }
    log_debug("Creat new db file [%s], size[%d]",
            new_db_fname, get_file_size(new_db_fname));

    mq_queue->cur_wdb.opt_count = 0;
    mq_queue->cur_wdb.flag      = 1;
    mq_queue->cur_wdb.pos       = DB_FILE_HEAD_LEN;
    mq_queue->cur_wdb.cur_index = next_index;

    /* Write the db file head befor Get new data file to write */
    mq_sm_db_write_file_head(mq_queue);
    mq_sm_rtag_write_item(mq_queue);

    return true;
}

/* Find handle file for one queue, include rtag/read/write file */
bool find_handle_file(queue_file_t *queue_file, char *qname)
{
    struct dirent* ent = NULL;
    DIR*   p_dir = NULL;
    int    db_pattern_len;
    int    rtag_pattern_len;
    char   path[MAX_DATA_PATH_NAME_LEN + 1];
    char   db_file_pattern[MAX_DATA_FILE_NAME_LEN + 1];
    char   rtag_file_pattern[MAX_DATA_FILE_NAME_LEN + 1];

    GEN_QUEUE_PATH_BY_QNAME(path, qname);
    snprintf(db_file_pattern, MAX_DATA_FILE_NAME_LEN, "%s_", "db");
    snprintf(rtag_file_pattern, MAX_DATA_FILE_NAME_LEN,"%s_", "rtag");
    db_pattern_len   = strlen(db_file_pattern);
    rtag_pattern_len = strlen(rtag_file_pattern);

    if ((p_dir = opendir(path)) == NULL)
    {
        log_warn( "Cannot open DB directory:[%s]", path);
        return false;
    }

    int i = 1; //for db file
    int j = 1; //for rtag file
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

        /* read/write db file find */
        if (strncmp(ent->d_name, db_file_pattern, db_pattern_len) == 0)
        {
            if (i == 1)
            {
                strcpy(queue_file->read_fname, ent->d_name);
                strcpy(queue_file->write_fname, ent->d_name);
            }
            else
            {
                strcpy(queue_file->read_fname,
                        strcmp(ent->d_name, queue_file->read_fname) < 0 ? ent->d_name : queue_file->read_fname); 
                log_debug( "Temp read db file name[%s]", queue_file->read_fname);

                strcpy(queue_file->write_fname,
                        strcmp(ent->d_name, queue_file->write_fname) > 0 ? ent->d_name : queue_file->write_fname); 
                log_debug( "Temp write db file name[%s]", queue_file->write_fname);
            }
            i++;
        }

        /* rtag file find */
        if (strncmp(ent->d_name, rtag_file_pattern, rtag_pattern_len) == 0)
        {
            if (j == 1)
            {
                strcpy(queue_file->rtag_fname, ent->d_name);
            }
            else
            {
                /* If rtag file is empty remove it */
                if (get_file_size(ent->d_name) < 0)
                {
                    unlink(ent->d_name);
                    continue;
                }
                strcmp(ent->d_name, queue_file->rtag_fname) < 0 ? unlink(ent->d_name) : unlink(queue_file->rtag_fname);

                /* Note new rtag file */
                strcpy(queue_file->rtag_fname,
                        strcmp(ent->d_name, queue_file->rtag_fname) > 0 ? ent->d_name : queue_file->rtag_fname); 
                log_debug( "Temp rtag file name[%s]", queue_file->rtag_fname);
            }
            j++;
        }
    }//while
    closedir(p_dir);
     
    if (i <= 1 || j <= 1)
    {
        log_warn("Rtag file or db file maybe lack");
        return false;
    }

    /* Get index number by file name */
    queue_file->rtag_fid  = get_id_by_fname(queue_file->rtag_fname);
    queue_file->read_fid  = get_id_by_fname(queue_file->read_fname);
    queue_file->write_fid = get_id_by_fname(queue_file->write_fname);

    log_info("Rtag [%s] id [%"PRIu64"], Read db [%s] id [%"PRIu64"], Write db [%s] id [%"PRIu64"]",
            queue_file->rtag_fname, queue_file->rtag_fid,
            queue_file->read_fname, queue_file->read_fid,
            queue_file->write_fname, queue_file->write_fid);

    if (queue_file->rtag_fid != queue_file->read_fid)
    {
        log_error("Rtag file index  doesn't match read file index");
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////

/* Get index number from file name */
static uint64_t get_id_by_fname(const char* qname)
{
    uint64_t    ret = 0;
    char*       p = NULL;
    char        str[64];

    memset(str, '\0', sizeof(str));
    strncpy(str, qname, sizeof(str));

    p = strtok(str, "_"); 
    while(p != NULL)
    {   
        log_debug("Split str [%s]", p);
        ret = (uint64_t)atoll(p);
        p = strtok(NULL, "_");
    }   
    log_debug("Queue name [%s] index [%"PRIu64"]", qname, ret);

    return ret;
}
