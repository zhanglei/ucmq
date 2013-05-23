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
filename: mq_client.h
----------------------------------------------------------------------------------------------
Author                 Date                          Comments
yuancy                 2011-10-09                    mq c client common API
**********************************************************************************************/
#ifndef __MQ_C_CLIENT_H__
#define __MQ_C_CLIENT_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include   <stdint.h>

//////////////////////////////////////////////////////////////////////////////////////////////

#define MQ_TRUE               1        /* 本项目的true */
#define MQ_FALSE              0        /* 本项目的false */
#define MQ_OK                 0        /* 成功返回 */
#define MQ_FAIL               -1       /* 失败返回 */
#define MQ_RET_REASON_SIZE    256      /* 返回错误信息长度 */

//////////////////////////////////////////////////////////////////////////////////////////////

/* mq服务端对象 */
typedef struct mq_object mq_object_t;

/* 扩展配置 */
typedef struct mq_conf
{
   char*              charset;         /* 字符集类型 */
   int                ver;             /* protocol version flag */
}mq_conf_t;

/* mq 返回操作 */
typedef struct mq_return
{
    int                pos;             /* 插入数据 */
    int                result;          /* 业务操作返 */
    char               reason[MQ_RET_REASON_SIZE]; /* 错误信息 */
    char*              data;            /* 返回内容 */
    int                data_len;        /* 返回内容长度 */
    int                index;           /* 返回服务序列号 */
    uint32_t           opt_value;       /* 枚举中操做值 */
}mq_return_t;

/* 操作码枚举 */
typedef enum mq_opts
{
    /* 以下为mq_get_opt使用的opt选项选项 */
    MQ_OPT_STATUS      = 0,
    MQ_OPT_STATUS_JSON = 1,
    MQ_OPT_VIEW        = 2,

    /* 以下为mq_set_opt和mq_all_set_opt使用的opt选项选项 */
    MQ_OPT_RESET       = 10,
    MQ_OPT_MAXQUEUE    = 11,
    MQ_OPT_SYNCTIME    = 12,
    MQ_OPT_REMOVE      = 13,
    MQ_OPT_DELAY       = 14,
    MQ_OPT_READONLY    = 15,
}mq_opts_e;

/* 请求返回码 */
typedef enum mq_result
{
    MQ_OPT_SUCCESS     = 200,           /* 操作成功 */
    MQ_OPT_ERROR       = 400,           /* 请求错误 */
    MQ_GET_MSG_END     = 404,           /* 所有队列消息已去空 */
    MQ_GET_UNKNOWN_OPT = 405,
}mq_result_e;

/* mq访问机制 */
typedef enum mq_lookup
{
    MQ_LOOKUP_ORDER =    0,             /* 访问第一个，如不成功则到第二个递推 */
    MQ_LOOKUP_RANDOM =   1,             /* 随机访问 */
}mq_lookup_e;

/*********************************************************************************************
 * Function Name:  mq_init
 * Description  :  初始化mq对象
 * Inputs       :  server_group：mq服务器实例字符串数组（ip:port），同时ip也可以是长度不超过64位的域名；count:数组个数；
 *                 look_up:是否轮询（MQ_LOOKUP_ORDER，顺序；MQ_LOOKUP_RANDOM，随机）；timeout：客户端超时时间(单位：毫秒，建议设置：>1000)；
 *                 keep_alive:是否长连接（MQ_TRUE:是，MQ_FALSE:否）
 * Outputs      :  N/A
 * ErrorCodes   :  成功返回mq_object_t对象，失败返回NULL
 * History      :  
**********************************************************************************************/
extern struct mq_object* mq_init(const char** server_group , const int count, int look_up, int timeout, int keep_alive); 

/*********************************************************************************************
 * Function Name:  mq_fini
 * Description  :  销毁mq对象
 * Inputs       :  mq：mq对象
 * Outputs      :  N/A
 * ErrorCodes   :  成功返回0，失败则返回-1
 * History      :  
**********************************************************************************************/
extern int mq_fini(struct mq_object* mq);

/*********************************************************************************************
 * Function Name:  mq_ret_free
 * Description  :  销毁业务操作请求返回对象
 * Inputs       :  ret_info：业务操作的返回对象指针
 * Outputs      :  N/A
 * ErrorCodes   :  成功返回0，失败则返回-1
 * History      :  
**********************************************************************************************/
extern int mq_ret_free(struct mq_return* ret_info);

/*********************************************************************************************
 * Function Name:  mq_put_msg
 * Description  :  入队列接口
 * Inputs       :  mq_object_t：mq对象列表；name：队列名称；data：入队列消息；conf扩展字段：字符集(不需要时为空) 
 * Outputs      :  pos:插入数据位置；index：服务器序列；result：业务返回值；reason：错误信息；data：空； 
 *                 data_len：0；opt_value:-1     
 * ErrorCodes   :  
 * History      :  
**********************************************************************************************/
extern struct mq_return* mq_put_msg(struct mq_object* mq, const char* name, const char* data, const struct mq_conf* conf);

/*********************************************************************************************
 * Function Name:  mq_get_msg
 * Description  :  入队列接口
 * Inputs       :  mq_object_t：mq对象列表；name：队列名称；conf扩展字段：字符集 
 * Outputs      :  pos:读取数据位置；index：服务器序列；result：业务返回值；reason：错误信息；
 *                 data：读取消息内容；data_len：所读消息长度；opt_value:-1   
 * ErrorCodes   :  
 * History      :  
**********************************************************************************************/
extern struct mq_return* mq_get_msg(struct mq_object* mq, const char* name, const struct mq_conf* conf);

/*********************************************************************************************
 * Function Name:  mq_all_set_opt
 * Description  :  设置所有mq队列
 * Inputs       :  mq_object_t：mq对象列表；name：队列名称；opt：操作码；data：设置值；conf扩展字段：字符集 
 * Outputs      :  pos:无意义；index：返回错误的那台服务器序列；result：业务返回值（都成功返回则成功）；
 *                 reason：错误信息；data：空；data_len：0；opt_value:操作码     
 * ErrorCodes   :  
 * History      :  
**********************************************************************************************/
extern struct mq_return* mq_all_set_opt(struct mq_object* mq, const char* name, int opt, uint32_t data, const struct mq_conf* conf);

/*********************************************************************************************
 * Function Name:  mq_set_opt
 * Description  :  单一队列设置
 * Inputs       :  mq: mq对象列表；server：某一mq（ip:port）；name：队列名称；opt:操作码；data：设定值；conf扩展字段：字符集 
 * Outputs      :  pos:无意义；index：固定为0（无意义）；result：业务返回值；reason：错误信息；data：空；
 *                 data_len：0；opt_value：操作码       
 * ErrorCodes   :  
 * History      :  
**********************************************************************************************/
extern struct mq_return* mq_set_opt(struct mq_object* mq, const char* server, const char* name, int opt, uint32_t data, const struct mq_conf* conf);

/*********************************************************************************************
 * Function Name:  mq_get_opt
 * Description  :  获取某队列信息
 * Inputs       :  mq：mq对象列表；server：某一mq（ip:port）；name：队列名称；opt：操作码；data：指定队列位置（只有view用到）；conf扩展字段：字符集 
 * Outputs      :  pos：无意义(只在view时返回查看位置)；index：返回服务器序列；result：业务返回值；reason：错误信息；
 *                 data：返回内容；data_len：所返回内容长度；opt_value:操作码       
 * ErrorCodes   :  
 * History      :  
**********************************************************************************************/
extern struct mq_return* mq_get_opt(struct mq_object* mq, const char* server, const char* name, int opt, uint32_t data, const struct mq_conf* conf);

#ifdef __cplusplus
}
#endif
#endif /* #ifndef __MQ_C_CLIENT_H__ */
