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

#ifndef  __MQ_CONFIG_H__
#define  __MQ_CONFIG_H__
#ifdef __cplusplus
extern "C" 
{
#endif

/////////////////////////////////////////////////////////////////////////////////

#define  DEF_HTTP_LISTEN_ADDR   "0.0.0.0" /* default http listen addr ip */
#define  DEF_HTTP_LISTEN_PORT   1818
#define  DEF_ALLOW_EXEC_IP      "0"
#define  DEF_BINLOG_FILE_PATH   "../binlog"
#define  DEF_OUTPUT_LOG_PATH    "../log"
#define  DEF_OUTPUT_LOG_LEVEL   "info"
#define  DEF_KEEP_ALIVE         "300"
#define  DEF_CONF_FILE          "../conf/ucmq.ini"
#define  DEF_PID_FILE           "./ucmq_1818.pid"
#define  DEF_RES_STORE_SPACE    4
#define  DEF_MAX_QLIST_ITEMS    32       /* queue list items limit */

#define  DEF_SYNC_INTERVAL      100
#define  DEF_SYNC_TIME_INTERVAL 100

#define  DEF_DATA_FILE_PATH     "../data"
#define  DEF_DB_FILE_MAX_SIZE   64

#define  DEF_MAX_QUEUE          10000000
#define  DEF_DELAY              0

/////////////////////////////////////////////////////////////////////////////////

/* Load config */
#define LOAD_OPTIONAL_PARAM(a, b, c, d, e)                                 \
do{                                                                        \
    if ((a = ini_find(b, c, d, e)) == NULL)                                \
    {                                                                      \
        fprintf(stderr, "Not found params [%s]-[%s]-[%d], use default!\n", \
                c == NULL ? "nil" : c, d == NULL ? "nil" : d, e);          \
    }                                                                      \
}while(0)

/////////////////////////////////////////////////////////////////////////////////

typedef struct ucmq_conf
{
    char         http_listen_addr[16];   /* accept addr */
    int          http_listen_port;       /* accept port */
    char         allow_exec_ip[16];      /* exec_ip */
    char         output_log_path[128];   /* log_path */
    char         output_log_level[16];   /* log_level */
    char         binlog_file_path[128];  
    char         keep_alive[16];         /* keep_alive */
    char         conf_file[128];
    char         pid_file[128]; 
    uint64_t     res_store_space;
    int          max_qlist_itmes;        /* queue list items limit */

    int          sync_interval;
    int          sync_time_interval;

    uint32_t     def_max_queue;          /* queue_size */
    uint32_t     def_delay;              /* default delay */

    char         data_file_path[128];    /* data_path */
    int          db_file_max_size;       /* db file size */
}ucmq_conf_t;

extern ucmq_conf_t g_mq_conf;
extern bool read_conf_file(void);

/////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif /* __MQ_CONFIG_H__ */
