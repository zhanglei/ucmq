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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#define _GNU_SOURCE
#define __USE_GNU

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "object.h"
#include "mq_client.h"

//////////////////////////////////////////////////////////////////////////////////////////////

#define SET_IOV(x,y)                                              \
{                                                                 \
    iovs[x].iov_base =  (y == NULL) ? "" : (void*)y;              \
    iovs[x].iov_len  =  strlen((y == NULL) ? "" : y);             \
    total_bytes      += iovs[x].iov_len;                          \
    x++;                                                          \
}

#define SHOW_ERROR(format, ...)                                   \
{                                                                 \
    if (reason[0] == '\0')                                        \
    {                                                             \
        snprintf(reason, sizeof(reason), format". errno %d - %s", \
            ##__VA_ARGS__, errno, strerror(errno));               \
        reason[sizeof(reason) - 1] = '\0';                        \
    }                                                             \
}

//////////////////////////////////////////////////////////////////////////////////////////////

static int in_add_to_list(struct mq_object* mq, int curr, int fd, char* dest, int sec_now);
static int in_del_from_list(struct mq_object* mq, int fd);
static int in_get_from_list(struct mq_object* mq,int curr, char* dest, int sec_now);
static int in_del_list(struct mq_object* mq);
static int in_mq_operate(struct mq_object* mq, int curr, struct mq_opt_info* info, struct mq_return* ret);
static int in_get_header_info_int(const char* header, const char* type);
static const char* in_get_header_info_str(const char* header, const char* type);
static const char* in_get_data(const char* data);
static int in_check_header_finish(char* data, size_t count);

static void in_init_return(struct mq_return* ret);
static void swap(int *a, int *b);
static void in_do_operate(struct mq_object* mq, struct mq_opt_info* info, struct mq_return* ret);

static int comm_wait_fd(int fd, short events, int timeout);
static int comm_set_timeout_by_fd(struct mq_object* mq, int fd, int timeout);
static int comm_get_rest_time(const struct mq_object* mq, const struct mq_opt_info* info);
static unsigned int str2ip(char* ip_str);
static int comm_create_socket(struct mq_object* mq,
        int curr,
        struct mq_opt_info* info,
        char* reason, 
        struct mq_return* ret);
static int comm_get_socket(struct mq_object* mq, int curr, struct mq_opt_info* info, char* reason,struct mq_return* ret);
static int comm_send_http_req(int s, const struct mq_opt_info* info, char* reason);
static int comm_recv_n(const struct mq_object* mq,
        int fd,
        char* data,
        size_t count,
        const struct mq_opt_info* info, 
        int (*is_finish)(char* data, size_t count));

//////////////////////////////////////////////////////////////////////////////////////////////

/* init mq object */
struct mq_object* mq_init(const char** server_group ,const int count, int look_up, int timeout, int keep_alive)
{               
    assert(server_group);
    assert(count > 0);
    assert(look_up >= 0);
    assert(timeout >= 0);
    assert(keep_alive >= 0);

    srand((unsigned)time(NULL)); 

    // creat mq object for manage all sign in servers
    struct mq_object* mq = NULL;
    mq = (struct mq_object*)malloc(sizeof(struct mq_object));
    if(mq == NULL)
    {
        return NULL;
    }

    mq->conn_list = (struct mq_srv*)malloc(sizeof(struct mq_srv) * count);
    if(mq == NULL)
    {
        free(mq);
        return NULL;
    }
    mq->count      = count;
    mq->look_up    = look_up;
    mq->timeout    = timeout;
    mq->keep_alive = keep_alive;

    int i = 0;
    for(; i < count; i++)
    {   
        int      len;
        char*    addr;

        addr = (char*)server_group[i];
        len = strlen(addr);
        strncpy(mq->conn_list[i].mq_dest, addr, sizeof(mq->conn_list[i].mq_dest));

        char* p = strchr(addr, ':');
        if(p != NULL)
        {   
            char domain[len];

            mq->conn_list[i].port = atoi(p + 1);
            strncpy(domain, addr, p - addr);
            domain[p - addr] = '\0';

            if(str2ip(domain) == MQ_TRUE) 
            {
                strcpy(mq->conn_list[i].domain, domain);
            }
            else
            {
                return NULL;
            }
        }
        else
        {
            return NULL;
        }
        mq->conn_list[i].fd       = -1; 
        mq->conn_list[i].timeout  = 0;
        mq->conn_list[i].last_use = 0;
    }  
    return mq;
}

/* fini mq object */
int mq_fini(struct mq_object* mq)
{
    if (NULL != mq)
    {
        in_del_list( mq);
        free(mq);
    }
    return MQ_OK;
}

/* free return infomation object */
int mq_ret_free(struct mq_return* ret_info)
{
    if (NULL != ret_info)
    {
        if (NULL != ret_info->data)
        {
            free(ret_info->data);
        }
        free(ret_info);
    }
    return MQ_OK;
}

/* put message to queque*/
struct mq_return* mq_put_msg(struct mq_object* mq, const char* name, const char* data, const struct mq_conf* conf)
{
    if(NULL == mq || NULL == name || NULL == data || NULL == conf)
    {
        return NULL;
    }

    //init return info
    struct mq_return* ret;
    struct mq_opt_info info;
    ret = (struct mq_return*)malloc(sizeof(struct mq_return));
    if(NULL == ret)
    {
        return NULL;
    }

    in_init_return(ret);

    memset(&info, 0, sizeof(struct mq_opt_info));
    info.opt      = "put";
    info.name     = (char*)name;
    info.data     = (char*)data;
    info.data_len = strlen(data);
    info.num      = "";
    info.conf     = (struct mq_conf*)conf;

    in_do_operate(mq, &info, ret);

    return ret;
}

struct mq_return* mq_get_msg(struct mq_object* mq, const char* name, const struct mq_conf* conf)
{
    if (NULL == mq || NULL == name || NULL == conf)
    {
        return NULL;
    }

    struct mq_return* ret;
    struct mq_opt_info info;

    ret = (struct mq_return*)malloc(sizeof(struct mq_return));
    if (NULL == ret)
    {
        return NULL;
    }
    in_init_return(ret);

    memset(&info, 0, sizeof(struct mq_opt_info));
    info.opt   = "get";
    info.name  = (char*)name;
    info.conf  = (struct mq_conf*)conf;

    in_do_operate(mq, &info, ret);

    return ret;
} 

struct mq_return* mq_all_set_opt(struct mq_object* mq, const char* name, int opt, uint32_t data, const struct mq_conf* conf)
{
    if(NULL == mq || NULL == name || 0 > opt || 0 > data || NULL == conf)
    {
        return NULL;
    }

    /* init return info */
    struct mq_return* ret;
    struct mq_opt_info info;

    ret = (struct mq_return*)malloc(sizeof(struct mq_return));
    if (NULL == ret)
    {
        return NULL;
    }
    in_init_return(ret);

    memset(&info, 0, sizeof(struct mq_opt_info));
    char buf[128];
    sprintf(buf, "num=%d", data);
    info.num        = buf;
    info.name       = (char*)name;
    info.data       = (void*)&data;
    info.data_len   = sizeof(data);
    info.conf       = (struct mq_conf*)conf;
    ret->opt_value  = opt;

    switch (opt)
    {   
        case MQ_OPT_RESET:
            info.opt = "reset";
            info.num = ""; 
            break;
        case MQ_OPT_MAXQUEUE:
            info.opt = "maxqueue";
            break;
        case MQ_OPT_SYNCTIME:
            info.opt = "synctime";
            break;
        case MQ_OPT_REMOVE:
            info.opt = "remove";
            info.num = ""; 
        case MQ_OPT_DELAY:
            info.opt = "delay";
            break;
        case MQ_OPT_READONLY:
            info.opt = "readonly";
            break;
        default:
            strcpy(ret->reason, "Unknown mq_set_opt");
            ret->result = 22;
            return ret;
    }   
    mq->look_up = MQ_LOOKUP_ORDER;
    in_do_operate(mq, &info, ret);

    return ret;
} 

struct mq_return* mq_set_opt(struct mq_object* mq, const char* server, const char* name, int opt, uint32_t data, const struct mq_conf* conf)
{
    if(NULL == mq || NULL == server || NULL == name || 0 > opt || 0 > data || NULL == conf)
    {
        return NULL;
    }

    //init return info
    struct mq_return* ret;
    struct mq_opt_info info;

    ret = (struct mq_return*)malloc(sizeof(struct mq_return));
    if(NULL == ret)
    {
        return NULL;
    }
    in_init_return(ret);

    memset(&info, 0, sizeof(struct mq_opt_info));
    char buf[128];
    sprintf(buf, "num=%d", data);
    info.num        = buf;
    info.name       = (char*)name;
    info.data       = (void*)&data;
    info.data_len   = sizeof(data);
    info.conf       = (struct mq_conf*)conf;

    ret->opt_value = opt;
    switch (opt)
    {   
        case MQ_OPT_RESET:
            info.opt = "reset";
            info.num = ""; 
            break;
        case MQ_OPT_MAXQUEUE:
            info.opt = "maxqueue";
            break;
        case MQ_OPT_SYNCTIME:
            info.opt = "synctime";
            break;
        case MQ_OPT_REMOVE:
            info.opt = "remove";
            info.num = ""; 
        case MQ_OPT_DELAY:
            info.opt = "delay";
            break;
        case MQ_OPT_READONLY:
            info.opt = "readonly";
            break;
        default:
            strcpy(ret->reason, "Unknown mq_set_opt");
            ret->result = 22;
            return ret;
    }   

    char* addr;
    addr = (char*)server;

    int i = 0;
    for(; i < mq->count; i++)
    {
        if(0 == strcmp(mq->conn_list[i].mq_dest, addr))
        {
            strcpy(info.mq_dest , mq->conn_list[i].mq_dest);
            in_mq_operate( mq, i, &info, ret); 
            break;
        }
    }
    return ret;
} 

struct mq_return* mq_get_opt(struct mq_object* mq, const char* server, const char* name, int opt, uint32_t data, const struct mq_conf* conf)
{
    if(NULL == mq || NULL == server || NULL == name || 0 > opt || 0 > data || NULL == conf)
    {
        return NULL;
    }

    struct mq_return* ret;
    struct mq_opt_info info;
    ret = (struct mq_return*)malloc(sizeof(struct mq_return));
    if(NULL == ret)
    {
        return NULL;
    }
    in_init_return(ret);

    memset(&info, 0, sizeof(struct mq_opt_info));
    char buf[128];
    info.num       = buf;
    info.name      = (char*)name;
    info.data      = (void*)&data;
    info.data_len  = sizeof(data);
    info.conf      = (struct mq_conf*)conf;
    ret->opt_value = opt;

    switch (opt)
    {   
        case MQ_OPT_STATUS:
            info.opt = "status";
            info.num = ""; 
            break;
        case MQ_OPT_STATUS_JSON:
            info.opt = "status_json";
            info.num = ""; 
            break;
        case MQ_OPT_VIEW:
            info.opt = "view";
            break;
        default:
            strcpy(ret->reason, "Unknown mq_get_opt");
            ret->result = 22;
            return ret;
    }   
    char* addr;
    addr = (char*)server;

    int i = 0;
    for(; i < mq->count; i++)
    {
        if(0 == strcmp(mq->conn_list[i].mq_dest, addr))
        {
            strcpy(info.mq_dest , mq->conn_list[i].mq_dest);
            in_mq_operate( mq, i, &info, ret); 
            break;
        }
    }
    return ret;
} 

//////////////////////////////////////////////////////////////////////////////////////////////

/* Check fd timeout */
static int comm_wait_fd(int fd, short events, int timeout)
{
    struct pollfd fds;

    fds.fd       = fd;
    fds.events   = events;
    fds.revents  = 0;
    errno        = 0;

    do
    {
        int r = poll(&fds, 1, timeout);
        if (r >= 0)
        {
            return fds.revents;
        }
    } while (errno == EINTR);

    return -1;
}

/* Set fd timeout */
static int comm_set_timeout_by_fd(struct mq_object* mq, int fd, int timeout)
{
    int i = 0;
    for (; i < mq->count; i++)
    {
        if (mq->conn_list[i].fd == fd)
        {
            mq->conn_list[i].timeout = timeout;
            return 0;
        }
    }
    return -1;
}

static int comm_get_rest_time(const struct mq_object* mq, const struct mq_opt_info* info)
{
    int       rest_time = 0;
    struct    timespec tm;

    clock_gettime(CLOCK_MONOTONIC, &tm);
    rest_time = mq->timeout - (((tm.tv_sec - info->time_start.tv_sec) * 1000) \
            + ((tm.tv_nsec - info->time_start.tv_nsec) >> 20));

    return (rest_time > 0) ? rest_time : 0;
}

static unsigned int str2ip(char* ip_str)
{
    int     i = 0;
    int     ip = 0;
    char    tmp[32];
    char    *p = ip_str;

    if (ip_str == NULL)
    {
        return MQ_FAIL;
    }
    /*¹ýÂË·Ç·¨×Ö·û*/
    while (*p != 0 && i < 32)
    {
        if (*p == '.' || (*p >= '0' && *p <= '9'))
        {
            tmp[i] = *p;
            i++;
        }
        p++;
    }
    tmp[i] = 0;
    ip = inet_addr(tmp);
    if (ip == -1)
    {
        return MQ_FAIL; 
    }

    return MQ_TRUE;
}

static int comm_create_socket(struct mq_object* mq,
        int curr,
        struct mq_opt_info* info,
        char* reason,
        struct mq_return* ret)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_addr.s_addr = inet_addr(mq->conn_list[curr].domain);
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(mq->conn_list[curr].port);

    int s = socket(AF_INET,SOCK_STREAM, 0);
    if (s < 0)
    {
        SHOW_ERROR("Create socket error");
        return s;
    }

    int flags = fcntl(s, F_GETFL, 0);
    if (fcntl(s, F_SETFL, flags|O_NONBLOCK) != 0)
    {
        SHOW_ERROR("Can not set socket %d to non-blocking mode", s);
        close(s);
        return -1;
    }

    while (connect(s, (const struct sockaddr*)&addr, sizeof(struct sockaddr)) < 0)
    {
        switch(errno)
        {
            case EINPROGRESS:
                if (comm_wait_fd(s, POLLOUT, comm_get_rest_time(mq, info)) != 0)
                {
                    int error = -1;
                    socklen_t len = sizeof(error);
                    if ((getsockopt(s, SOL_SOCKET, SO_ERROR, &error, &len) == 0) && (error == 0))
                    {
                        return s;
                    }
                    else
                    {
                        errno = error;
                        SHOW_ERROR("Connect to mq %s:%d error", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                    }
                }
                else
                {
                    errno = ETIMEDOUT;
                    SHOW_ERROR("Connect to mq %s:%d timeout", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port)); 
                }
                break;
            case EISCONN:
                return s;
            case EINTR:
                continue;
            default:
                SHOW_ERROR("Connect to mq %s:%d error", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                break;
        }
        close(s);
        break;
    }
    return -1;
}

static int comm_get_socket(struct mq_object* mq, int curr, struct mq_opt_info* info, char* reason,struct mq_return* ret)
{
    int s;
    if ((s = in_get_from_list(mq, curr, info->mq_dest, info->time_start.tv_sec)) < 0)
    {
        s = comm_create_socket(mq, curr, info, reason, ret);
        if (s < 0)
        {
            return -1;
        }
        in_add_to_list(mq, curr, s, info->mq_dest, info->time_start.tv_sec);
    }
    return s;
}

static int comm_send_http_req(int s, const struct mq_opt_info* info, char* reason)
{
    char      buffer[128];
    struct    iovec iovs[16];
    int       iovs_count = 0;
    int       total_bytes = 0;

    if ((info->data == NULL) || (info->data_len <= 0))
    {
        SET_IOV(iovs_count, "GET /?name=");
        SET_IOV(iovs_count, info->name);
        SET_IOV(iovs_count, "&opt=");
        SET_IOV(iovs_count, info->opt);
        if (info->conf->charset != NULL)
        {
            SET_IOV(iovs_count, "&charset=");
            SET_IOV(iovs_count, info->conf->charset);
        }
        if (info->num != NULL)
        {
            SET_IOV(iovs_count, "&");
            SET_IOV(iovs_count, info->num);
        }
        SET_IOV(iovs_count, " HTTP/1.1\r\nHost: ");
        SET_IOV(iovs_count, info->mq_dest);
        SET_IOV(iovs_count, "\r\nConnection: keep-alive\r\n\r\n");
    }
    else
    {
        SET_IOV(iovs_count, "POST /?name=");
        SET_IOV(iovs_count, info->name);
        SET_IOV(iovs_count, "&opt=");
        SET_IOV(iovs_count, info->opt);
        if (info->conf->charset != NULL)
        {
            SET_IOV(iovs_count, "&charset=");
            SET_IOV(iovs_count, info->conf->charset);
        }
        if (info->num != NULL)
        {
            SET_IOV(iovs_count, "&");
            SET_IOV(iovs_count, info->num);
        }
        SET_IOV(iovs_count, " HTTP/1.1\r\nHost: ");
        SET_IOV(iovs_count, info->mq_dest);
        SET_IOV(iovs_count, "\r\nContent-Length: ");
        snprintf(buffer, sizeof(buffer), "%d", info->data_len);
        SET_IOV(iovs_count, buffer);
        SET_IOV(iovs_count, "\r\nConnection: keep-alive\r\n\r\n");
        iovs[iovs_count].iov_base = (void*)info->data;
        iovs[iovs_count].iov_len = info->data_len;
        total_bytes += info->data_len;
        iovs_count += 1;
    }

    int send_size = -1;
    do
    {
        send_size = writev(s, iovs, iovs_count);
    } while ((send_size < 0) && ((errno == EINTR)||(errno == EAGAIN)));

    if (send_size != total_bytes)
    {
        SHOW_ERROR("Send request error, Meybe timeout, Need to send %d bytes, But only send %d bytes",
                total_bytes, send_size);
        errno = 70;

        return -1;
    }
    return 0;
}

static int comm_recv_n(const struct mq_object* mq,
        int fd,
        char* data,
        size_t count,
        const struct mq_opt_info* info, 
        int (*is_finish)(char* data, size_t count))
{
    size_t recv_size = 0;
    do
    {
        int r;
        r = recv(fd, data + recv_size, count - recv_size, MSG_NOSIGNAL);
        switch (r)
        {
            case 0:
                return recv_size;
            case -1:
                switch (errno)
                {
                    case EAGAIN:
                        if (comm_wait_fd(fd, POLLIN, comm_get_rest_time(mq, info)) == 0)
                        {
                            errno = ETIMEDOUT;
                            return recv_size;
                        }
                        break;
                    case EINTR:
                        continue;
                    default:
                        return -1;
                }
                break;
            default:
                recv_size += r;
                break;
        }
    } while((recv_size < count) && ((is_finish == NULL) || (is_finish(data, recv_size) < 0)));
    return recv_size;
}

static int in_add_to_list(struct mq_object* mq, int curr, int fd, char* dest, int sec_now)
{
    int choose_id = curr;

    if (mq->conn_list[choose_id].fd != -1)
    {
        close(mq->conn_list[choose_id].fd);
    }
    struct mq_srv* conn_info = &mq->conn_list[choose_id];
    conn_info->fd       = fd;
    conn_info->last_use = sec_now;
    conn_info->timeout  = 3600;
    if (NULL == conn_info->mq_dest)
    {
        return -1;
    }
    strcpy(conn_info->mq_dest, dest);
    return 0;
}

static int in_del_from_list(struct mq_object* mq, int fd)
{
    int    count = 0;
    int    i = 0;

    for (; i < mq->count; i++)
    {
        if (mq->conn_list[i].fd == fd && mq->conn_list[i].fd != -1)
        {
            close(mq->conn_list[i].fd);
            mq->conn_list[i].fd = -1;
            count++;
        }
    }
    return count;
}

static int in_get_from_list(struct mq_object* mq,int curr, char* dest, int sec_now)
{
    int    i = curr;
    int    events = 0;

    if ((mq->conn_list[i].fd != -1) 
            && (mq->conn_list[i].mq_dest != NULL) 
            && (strcmp(dest, mq->conn_list[i].mq_dest) == 0))
    {
        events = comm_wait_fd(mq->conn_list[i].fd, POLLIN | POLLERR | POLLHUP | POLLNVAL, 0);
        if (events == 0)
        {
            mq->conn_list[i].last_use = sec_now;
            return mq->conn_list[i].fd;
        }
        close(mq->conn_list[i].fd);
        mq->conn_list[i].fd = -1;
    }

    return -1;
}

/* Remove if there's nothing to do at request end */
static int in_del_list(struct mq_object* mq)
{
    int i = 0;
    for (; i < mq->count; i++)
    {  
        if (mq->conn_list[i].fd != -1)       
        {
            close(mq->conn_list[i].fd);      
        }
    }  
    if (NULL != mq->conn_list)
    {
        free(mq->conn_list);
    }

    return MQ_OK;
} 

/* init  mq opt*/
static void in_init_return(struct mq_return* ret)
{
    ret->result     = -1;
    ret->pos        = -1;
    ret->reason[0]  = '\0';
    ret->data       = NULL;
    ret->data_len   = 0;
    ret->index      = -1;
    ret->opt_value  = -1;
}

static void swap(int *a, int *b)
{
    int t;
    t  = *a;
    *a = *b;
    *b = t;
}

static void in_do_operate(struct mq_object* mq, struct mq_opt_info* info, struct mq_return* ret)
{
    int    loop = 0;
    int    curr = 0;
    int    a[mq->count];

    if(MQ_LOOKUP_RANDOM == mq->look_up)
    {
        int i;
        for (i = 0; i < mq->count; ++i)
        {   
            a[i] = i;
        }   
        /* Shuffle algorithm */
        int j = 1;
        for (j = mq->count - 1; j > 0; --j)
        {
            int* s = &a[j];
            int* d = &a[rand() % mq->count];
            swap(s, d);
        }
    }

    for (;loop < mq->count; loop++)
    {
        if (MQ_LOOKUP_RANDOM == mq->look_up)
        {
            curr = a[loop];
        }
        else
        {
            curr = loop;
        }

        strcpy(info->mq_dest , mq->conn_list[curr].mq_dest);
        /* send the request message */
        if ( MQ_FAIL == in_mq_operate(mq, curr, info, ret))
        {
            continue;
        }
        break;
    }
    ret->index = curr;
}

// mq_op
static int in_mq_operate(struct mq_object* mq, int curr, struct mq_opt_info* info, struct mq_return* ret)
{
    int     s = -1;
    int     is_ok = MQ_FAIL;
    char    reason[256];

    reason[0] = '\0';
    if (mq->timeout == 0)
    {
        mq->timeout = 1000;
    }
    clock_gettime(CLOCK_MONOTONIC, &info->time_start);

    do
    {
        /* get connection */
        if (MQ_TRUE == mq->keep_alive)
        {
            s = comm_get_socket(mq, curr, info, reason, ret);
            if (s < 0)
            {
                strncpy(ret->reason, reason, MQ_RET_REASON_SIZE);
                ret->result = errno;
                break;
            }
        }
        else
        {
            s = comm_create_socket(mq, curr, info, reason, ret);
            if(s < 0)
            {
                strncpy(ret->reason, reason, MQ_RET_REASON_SIZE);
                ret->result = errno;
                break;
            }
        }

        /* send request message */
        if (comm_send_http_req(s, info, reason) < 0)
        {
            ret->result = errno;
            break;
        }

        int     timeout;
        int     has_recv = 0;
        int     body_len = 0;
        int     total_recv = 0;
        char    buffer[1024];
        // recv result message
        total_recv = comm_recv_n(mq, s, buffer, sizeof(buffer) - 1, info, in_check_header_finish);
        buffer[(total_recv < 0) ? 0 : total_recv] = '\0';
        const char* data = in_get_data(buffer);
        if (data == NULL)
        {
            SHOW_ERROR("recv http header error");
            ret->result = errno;
            break;
        }

        char http_reason[strcspn(buffer, "\r\n")];
        sscanf(buffer, "HTTP/1.1 %d %[^\r\n]\r\n", &ret->result, http_reason);

        // uc patch , 2011-12-08
        if(ret->result == MQ_GET_MSG_END)
        {
            break;
        }

        ret->pos = in_get_header_info_int(buffer, "Pos");

        body_len = in_get_header_info_int(buffer, "Content-Length");
        if (body_len < 0)
        {
            SHOW_ERROR("data length is invalid : %d", body_len);
            break;
        }

        timeout = in_get_header_info_int(buffer, "\nkeep-alive");
        if (timeout > 0)
        {
            comm_set_timeout_by_fd(mq, s, timeout);
        }

        char body[body_len + 1];
        has_recv = total_recv - (data - buffer);
        if (has_recv > body_len)
        {
            SHOW_ERROR("Invalid body len");
            break;
        }
        else
        {
            memcpy(body, data, has_recv);
            if (body_len > has_recv)
            {
                if (comm_recv_n(mq, s, body + has_recv, body_len - has_recv, info, NULL) != body_len - has_recv)
                {
                    SHOW_ERROR("recv body error");
                    break;
                }
            }
        }
        body[body_len] = '\0';
        ret->data = (char*)malloc(sizeof(body));
        strncpy(ret->data, body, sizeof(body));
        ret->data_len = strlen(ret->data);
        strncpy(ret->reason, http_reason, MQ_RET_REASON_SIZE);

        is_ok = MQ_OK;

    } while (0);

    if(MQ_FALSE == mq->keep_alive && s != -1)
    { 
        close(s);
    }

    if(ret->result != MQ_GET_MSG_END && MQ_TRUE == mq->keep_alive && is_ok != MQ_OK)
    {
        in_del_from_list(mq, s);
    }

    return is_ok;
}

static const char* in_get_header_info_str(const char* header, const char* type)
{
    const char* p = strcasestr(header, type);
    if (p == NULL) 
    {
        return NULL;
    }
    p += (strlen(type) + 1);
    while (isblank(*p) != 0)
    {
        p++;
    }
    return p;
}

/* example: header = "Pos: 22", type = "pos" */
static int in_get_header_info_int(const char* header, const char* type)
{
    const char* p = in_get_header_info_str(header, type);
    return (p != NULL) ? atoi(p) : -1;
}

static const char* in_get_data(const char* data)
{
    const char* p = strstr(data, "\r\n\r\n");
    return (p != NULL) ? p + 4 : NULL;
}

static int in_check_header_finish(char* data, size_t count)
{
    data[count] = '\0';
    return (in_get_data(data) == NULL) ? -1 : 0;
}
