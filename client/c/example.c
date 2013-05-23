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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mq_client.h"

/////////////////////////////////////////////////////////////////////////////////////

/* opt code declare */
//    MQ_OPT_STATUS      = 0,
//    MQ_OPT_STATUS_JSON = 1,
//    MQ_OPT_VIEW        = 2,
//    MQ_OPT_RESET       = 10,
//    MQ_OPT_MAXQUEUE    = 11,
//    MQ_OPT_SYNCTIME    = 12,
//    MQ_OPT_REMOVE      = 13,
//    MQ_OPT_DELAY       = 14,
//    MQ_OPT_READONLY    = 15,

#define MQ_OP_COUNT 5000

/////////////////////////////////////////////////////////////////////////////////////

static int put_msg(struct mq_object* mq);
static int get_msg(struct mq_object* mq);
static int get_opt(struct mq_object* mq, int opt);
static int set_opt(struct mq_object* mq, int opt);
static int all_set_opt(struct mq_object* mq, int opt);
static int handl_result(struct mq_return* pp, const int ver);

/////////////////////////////////////////////////////////////////////////////////////

int main()
{
    int count = 3;                      /* number of mq server */
    int look_up = MQ_LOOKUP_RANDOM;     /* MQ_LOOKUP_ORDER ; MQ_LOOKUP_RANDOM */
    int timeout = 10;                   /* number of time out, untis: millisecond(MS) */
    int keep_alive = MQ_TRUE;           /* enable keep alive :MQ_TRUE yes , MQ_FALSE no */
    /* mq server host */
    const char* server_group[3] = {"192.168.0.4:1818",
                                   "192.168.0.3:1818",
                                   "192.168.0.10:1818"};

    struct mq_object* mq;
    mq = mq_init(server_group, count, look_up, timeout, keep_alive);
    if(NULL == mq)
    {
        printf("init error!\n");
        exit(1);
    }

    int i = 0;
    while(i < MQ_OP_COUNT)
    {   
        put_msg(mq);
        get_msg(mq);
        i++;
    }
    int j = 0;
    while(j < 1)
    {
        int a;
        int a_opt[] = {10, 11, 12, 13, 14, 16};
        for(a = 0; a < 6; a++)
        {
            /* Set opt for all servers */
            all_set_opt(mq, a_opt[a]);
        }

        int s;
        int s_opt[] = {10, 11, 12, 13, 14, 16};
        for(s = 0; s < 6; s++)
        {
            /* Set opt for one server */
            set_opt(mq, s_opt[s]);
            set_opt(mq, 11);
        }

        int g;
        int g_opt[] = {0, 1, 2, 3};
        for(g = 0; g < 4; g++)
        {
            /* Get opt for one server */
            get_opt(mq, g_opt[g]);
            printf("show me uc: %d\n", g);
        }
        j++;
    }

    mq_fini(mq);
    printf("**** finish job ****\n");
    return 0;
}

static int put_msg(struct mq_object* mq)
{
    printf("-- put message --\n");
    struct mq_return* pp; 
    struct mq_conf conf;

    char *name = "01";
    char *data = "helloworld";
    conf.charset = NULL;
    conf.ver   = 2;

    pp = mq_put_msg(mq, name, data, &conf);

    handl_result(pp, conf.ver);

    return 0;
}

static int get_msg(struct mq_object* mq)
{
    printf("-- get message --\n");
    struct mq_return* pp; 
    struct mq_conf conf;

    char *name = "01";
    conf.charset = "utf-8";
    conf.ver   = 2;

    pp = mq_get_msg(mq, name, &conf);
    handl_result(pp, conf.ver);

    return 0;
}

static int get_opt(struct mq_object* mq, int opt)
{
    printf("-- get option --\n");
    struct mq_return* pp; 
    struct mq_conf conf;

    char* addr = "192.168.0.3:1818";
    char* name = "001";
    int data = 10;
    conf.charset = "utf-8";
    conf.ver   = 2;

    pp = mq_get_opt(mq, addr, name, opt, data, &conf);
    handl_result(pp, conf.ver);

    return 0;
}

static int all_set_opt(struct mq_object* mq, int opt)
{
    printf("-- all set option --\n");
    struct mq_return* pp; 
    struct mq_conf conf;

    char *name = "001";
    int data = 1000;
    conf.charset = "utf-8";
    conf.ver   = 2;

    pp = mq_all_set_opt(mq, name, opt, data, &conf);
    handl_result(pp, conf.ver);

    return 0;
}

static int set_opt(struct mq_object* mq, int opt)
{
    printf("-- set option --\n");
    struct mq_return* pp; 
    struct mq_conf conf;

    char* addr = "192.168.0.4:1818";
    char *name = "002";
    int data = 1000;
    conf.charset = "utf-8";
    conf.ver   = 2;

    pp = mq_set_opt(mq, addr, name, opt, data, &conf);
    handl_result(pp, conf.ver);

    return 0;
}

static int handl_result(struct mq_return* pp, const int ver)
{
    /* If version code is 2 */
    if (ver == 2)
    {
        if( pp != NULL)
        {
            printf("return inof object creat fail or req error!\n");
            return -1;

        }
        printf("-------------\n");
        printf("result: %d\n", pp->result);
        printf("pos: %d\n", pp->pos);
        printf("reason: %s\n", pp->reason);
        printf("data: %s\n", pp->data);
        printf("data_len: %d\n", pp->data_len);
        printf("index: %d\n", pp->index);
        printf("opt_value: %d\n", pp->opt_value);
        printf("-------------\n");
        if (pp->result ==  MQ_OPT_SUCCESS)
        {
           printf("MQ_OPT_SUCCESS\n")
        }
        else if (pp->result == MQ_OPT_ERROR)
        {
            switch(pp->data)
            {
                case MQ_OPT_SUCCESS:
                    printf("mq operate ok!\n");
                    break;
                case MQ_OPT_ERROR:
                    printf("requst error ?\n");
                    break;
                case MQ_GET_MSG_END:
                    printf("mq get mesage empty!\n");
                    break;
                default:
                    printf("errno: %d \t;error:  %s\n",pp->result, strerror(pp->result));
                    break;
            }
        }
        mq_ret_free(pp);
    }
    else    /* If version code is not 2 */
    {
        if( pp != NULL)
        {
            printf("return inof object creat fail or req error!\n");
        }
        printf("-------------\n");
        printf("result: %d\n", pp->result);
        printf("pos: %d\n", pp->pos);
        printf("reason: %s\n", pp->reason);
        printf("data: %s\n", pp->data);
        printf("data_len: %d\n", pp->data_len);
        printf("index: %d\n", pp->index);
        printf("opt_value: %d\n", pp->opt_value);
        printf("-------------\n");
        switch(pp->result)
        {
            case MQ_OPT_SUCCESS:
                printf("mq operate ok!\n");
                break;
            case MQ_OPT_ERROR:
                printf("requst error ?\n");
                break;
            case MQ_GET_MSG_END:
                printf("mq get mesage empty!\n");
                break;
            default:
                printf("errno: %d \t;error:  %s\n",pp->result, strerror(pp->result));
                break;
        }
        mq_ret_free(pp);
    }

    return 0;
}
