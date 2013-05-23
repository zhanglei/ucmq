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
#include <string.h>

#include "ini.h"
#include "mq_util.h"
#include "mq_config.h"

/////////////////////////////////////////////////////////////////////////////////

/* -- initial global conf -- */
ucmq_conf_t g_mq_conf =
{
    DEF_HTTP_LISTEN_ADDR,   /* http_listen_addr[16]; */
    DEF_HTTP_LISTEN_PORT,   /* http_listen_port; */
    DEF_ALLOW_EXEC_IP,      /* allow_exec_ip[16]; */
    DEF_OUTPUT_LOG_PATH,    /* output_log_path[128]; */
    DEF_OUTPUT_LOG_LEVEL,   /* output_log_level; */
    DEF_BINLOG_FILE_PATH,   /* binlog_file_path[128]; */
    DEF_KEEP_ALIVE,         /* keep_alive; */
    DEF_CONF_FILE,          /* conf_file[128]; */
    DEF_PID_FILE,           /* pid_file[64]; */
    DEF_RES_STORE_SPACE,    /* real_store_space; */
    DEF_MAX_QLIST_ITEMS,    /* queue list items limit */

    DEF_SYNC_INTERVAL,      /* sync_interval; */
    DEF_SYNC_TIME_INTERVAL, /* sync_time_interval second; */

    DEF_MAX_QUEUE,          /* def_max_queue; */
    DEF_DELAY,              /* default delay */

    DEF_DATA_FILE_PATH,     /* data_file_path[128]; */
    DEF_DB_FILE_MAX_SIZE,   /* db_file_max_size; */
};

/////////////////////////////////////////////////////////////////////////////////

bool read_conf_file(void)
{
    const char *param = NULL;
    struct ini *myini = NULL;

    fprintf(stdout, "INFO: Reading config file[%s]\n", g_mq_conf.conf_file);

    myini = ini_load(g_mq_conf.conf_file);
    if (myini == NULL)
    {
        fprintf(stderr, "ERROR: Read config fail\n");
        return false;
    }

    LOAD_OPTIONAL_PARAM(param, myini, "server", "http_listen_addr", 0);
    if (param)
    {
        strncpy(g_mq_conf.http_listen_addr, param, sizeof(g_mq_conf.http_listen_addr) - 1);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "server", "http_listen_port", 0);
    if (param && is_num_str(param))
    {
        g_mq_conf.http_listen_port = atoi(param);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "server", "allow_exec_ip", 0);
    if (param)
    {
        strncpy(g_mq_conf.allow_exec_ip, param, sizeof(g_mq_conf.allow_exec_ip) - 1);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "server", "output_log_path", 0);
    if (param)
    {
        strncpy(g_mq_conf.output_log_path, param, sizeof(g_mq_conf.output_log_path) - 1);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "server", "output_log_level", 0);
    //if (param && is_num_str(param))
    if (param)
    {
        strncpy(g_mq_conf.output_log_level, param, sizeof(g_mq_conf.output_log_level) - 1);
        //g_mq_conf.output_log_level = atoi(param);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "server", "binlog_file_path", 0);
    if (param)
    {
        strncpy(g_mq_conf.binlog_file_path, param, sizeof(g_mq_conf.binlog_file_path) - 1);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "server", "keep_alive", 0);
    if (param)
    {
        strncpy(g_mq_conf.keep_alive, param, sizeof(g_mq_conf.keep_alive) - 1);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "server", "conf_file", 0);
    if (param)
    {
        strncpy(g_mq_conf.conf_file, param, sizeof(g_mq_conf.conf_file) - 1);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "server", "pid_file", 0);
    if (param)
    {
        strncpy(g_mq_conf.pid_file, param, sizeof(g_mq_conf.pid_file) - 1);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "server", "res_store_space", 0);
    if (param && is_num_str(param))
    {
        g_mq_conf.res_store_space = (uint64_t)atoll(param);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "server", "max_qlist_itmes", 0);
    if (param && is_num_str(param))
    {
        g_mq_conf.max_qlist_itmes = atoi(param);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "rtag", "sync_interval", 0);
    if (param && is_num_str(param))
    {
        g_mq_conf.sync_interval = atoi(param);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "rtag", "sync_time_interval", 0);
    if (param && is_num_str(param))
    {
        g_mq_conf.sync_time_interval = atoi(param);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "queue", "def_max_queue", 0);
    if (param && is_num_str(param))
    {
        g_mq_conf.def_max_queue = (uint32_t)atoi(param);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "queue", "def_delay", 0);
    if (param && is_num_str(param))
    {
        g_mq_conf.def_delay = (uint32_t)atoi(param);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "db", "data_file_path", 0);
    if (param)
    {
        strncpy(g_mq_conf.data_file_path, param, sizeof(g_mq_conf.data_file_path) - 1);
        assert(g_mq_conf.data_file_path);
    }

    LOAD_OPTIONAL_PARAM(param, myini, "db", "db_file_max_size", 0);
    if (param && is_num_str(param))
    {
        int db_file_max_size;
        db_file_max_size = atoi(param);
        assert(db_file_max_size <= MAX_DB_FILE_SIZE_LIMIT);
        g_mq_conf.db_file_max_size = db_file_max_size;
    }
    ini_clear(myini);

    return true;
}
