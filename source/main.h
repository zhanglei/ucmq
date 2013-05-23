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

#ifndef  __MAIN_H__
#define  __MAIN_H__
#ifdef __cplusplus
extern "C" 
{
#endif

/////////////////////////////////////////////////////////////////////////////////

typedef struct app_info
{
    uint32_t     put;
    uint32_t     get;
    uint32_t     times;
    uint32_t     total_time;
    uint32_t     max_time;
    uint64_t     count;
    uint64_t 	 put_size;
}app_info_t;

typedef enum ucmq_run_step
{
    RUN_STEP_BEGIN                  = 1,
    RUN_STEP_STORE_INIT             = 2,         /* storage init */
    RUN_STEP_STORE_END              = 3,         /* storage init */
    RUN_STEP_EVENT_INIT             = 4,         /* event init */
    RUN_STEP_HTTP_SERVER_CREATE     = 5,         /* http server creat */
    RUN_STEP_ADDR_PORT_BIND         = 6,
    RUN_STEP_PIPE_CREATE            = 7,
    RUN_STEP_EVENT_END              = 8,
    RUN_STEP_END                    = 0,
}ucmq_run_step_e;

/////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif /* __MAIN_H__ */
