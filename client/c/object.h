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
* Author: shaneyuan
*/

/********************************************************************************************* 
 * filename: object.h
 * ----------------------------------------------------------------------------------------------
 *  Author                 Date                          Comments
 *  yuancy                 2011-10-09                    private object
 *  *****************************************************************************************/
#ifndef __MQ_CLIENT_C_OBJECT_H__
#define __MQ_CLIENT_C_OBJECT_H__
#ifdef __cplusplus
extern "C" 
{
#endif

//////////////////////////////////////////////////////////////////////////////////////////////

#define DOMAIN_MAX_SIZE 64                   /* 域名最长限制 */

struct mq_conf;

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct mq_opt_info                   /* 每一个列表中的实例 */
{
    char*             opt;                   /* 操作类型 */
    char*             name;                  /* 操作队列名 */
    char*             num;                   /* 设置目标值 */
    void*             data;                  /* 返回数据 */
    int               data_len;              /* 返回的数据长度 */
    char              mq_dest[DOMAIN_MAX_SIZE + 6 + 1]; /* 操作服务端实例对象 */

    struct timespec   time_start;            /* 开始使用时间标识 */
    struct mq_conf*   conf;                  /* 操作描述 */
}mq_opt_info_t;

struct mq_object
{
    int               count;                 /* 表中实例个数 */
    int               timeout;               /* 客户端超时设置 */
    int               look_up;               /* 遍历方式（轮询，随机，指定，，）*/
    int               keep_alive;            /* 测试存活 */
    struct mq_srv*    conn_list;             /* 链接池 */
};

typedef struct mq_srv
{
    char              mq_dest[DOMAIN_MAX_SIZE + 6 + 1];
    int               port;
    char              domain[DOMAIN_MAX_SIZE + 1]; /* ip 或 域名 */

    int               fd; 
    int               last_use;              /* 最近使用的时间标识 */
    int               timeout;               /* 客户端超时时间 */
}mq_srv_t;

//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif /* __MQ_CLIENT_C_OBJECT_H__ */
