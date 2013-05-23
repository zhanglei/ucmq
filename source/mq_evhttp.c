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

#include <time.h>
#include <evhttp.h>
#include <unistd.h>

#include "mq_evhttp.h"
#include "mq_config.h"
#include "mq_queue_manage.h"

/////////////////////////////////////////////////////////////////////////////////

#define MQ_SHOW_STATUS(x) ((&rtag_item != NULL) ? rtag_item.x : -1)

#define MQ_DO_REPLY(hc, hr, v1, v2, ver)                              \
{                                                                     \
    if (ver != NULL && strcmp(ver, "2") == 0)                         \
    {                                                                 \
        http_reason = "OK";                                           \
        http_code = HTTP_OK;                                          \
        evbuffer_add_printf(content_out, "%s\r\n", v2);               \
    }                                                                 \
    else                                                              \
    {                                                                 \
        http_code = hc;                                               \
        http_reason = hr;                                             \
        evbuffer_add_printf(content_out, "%s\r\n", v1);               \
    }                                                                 \
    mq_evhttp_ret_send(request, content_out, http_query_variables,    \
            http_code, http_reason);                                  \
}

#define MQ_DO_AND_REPLY(x, m1, m2, hc, hr, v1, v2, ver)               \
{                                                                     \
    if (x == true)                                                    \
    {                                                                 \
        MQ_DO_REPLY(HTTP_OK, "OK", m1, m2, ver);                      \
    }                                                                 \
    else                                                              \
    {                                                                 \
        MQ_DO_REPLY(hc, hr, v1, v2, ver);                             \
    }                                                                 \
}

/////////////////////////////////////////////////////////////////////////////////

int pipe_fd[2];
app_info_t g_info;
app_info_t g_last_info;

static const long long s_2_ns = 1000000000;
static const double ns_2_ms = 0.000001;
static char g_stat_path[PATH_MAX] = {'\0'};
static char g_statm_path[PATH_MAX] = {'\0'};

static struct evhttp *g_event_http;
static struct event_base *g_event_base;

/////////////////////////////////////////////////////////////////////////////////

static void mq_evhttp_ret_send(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables,
        int http_code,
        const char* http_reason);
static void mq_evhttp_set_headers(
        struct evkeyvalq *headers_out,
        const struct evkeyvalq *headers_in,
        const char* type,
        const char* charset,
        int force_close);
static void mq_evhttp_conn_close(struct evhttp_connection *connection, void *arg);

static void get_rtag_info(mq_queue_t *mq_queue, rtag_item_t *rtag_item);
static void get_queue_stat(mq_queue_t *mq_queue, void* arg);
static void get_queue_stat_json(mq_queue_t *mq_queue, int qlist_index, void* arg);
static void get_all_queue_stat(void* arg);
static void get_all_queue_stat_json(void* arg);
static void mq_evhttp_cb_do_opt_entry(struct evhttp_request *request, void *arg);
static void mq_evhttp_cb_do_view_stat(struct evhttp_request *request, void *arg);
static void mq_evhttp_cb_do_sync(int fd, short event, void *arg);
static void mq_evhttp_cb_do_dump(int fd, short event, void *arg);
static void mq_evhttp_cb_do_exec(struct evhttp_request *request, void *arg);
static void mq_evhttp_cb_do_write_pipe(int fd, short event, void *arg);

static bool req_qname_check(const char* qname);
static mq_queue_t* find_queue_by_qname(
        const char* qname,
        struct evkeyvalq* http_query_variables);
static mq_queue_t* touch_queue_by_qname(
        const char* qname,
        struct evkeyvalq* http_query_variables);
static void http_opt_put(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);
static void http_opt_get(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);
static void http_opt_status(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);
static void http_opt_status_json(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);
static void http_opt_maxqueue(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);
static void http_opt_delay(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);
static void http_opt_wlock(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);
static void http_opt_remove(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);
static void http_opt_reset(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);
static void http_opt_synctime(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);
static void http_unknown_opt(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);
static void http_unparse_req(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables);

/////////////////////////////////////////////////////////////////////////////////

int mq_http_init()
{
    int ret_code = 0;

    if((g_event_base = event_init()) == NULL)
    {
        log_error("event_init() ... failure");

        return RUN_STEP_EVENT_INIT;
    }

    /* Create a new HTTP server */
    if((g_event_http = evhttp_new(g_event_base)) == NULL)
    {
        log_error("evhttp_new() ... failure");
        event_base_free(g_event_base);

        return RUN_STEP_HTTP_SERVER_CREATE;
    }

    /* Binds an HTTP server on the specified address and port */
    if((ret_code = evhttp_bind_socket(g_event_http,
                    g_mq_conf.http_listen_addr,
                    (uint16_t)g_mq_conf.http_listen_port)))
    {
        log_error("evhttp_bind_socket(%s:%d ) ... failure",
                g_mq_conf.http_listen_addr, g_mq_conf.http_listen_port);
        event_base_free(g_event_base);
        evhttp_free(g_event_http);

        return RUN_STEP_ADDR_PORT_BIND;
    }

    evhttp_set_timeout(g_event_http, atoi(g_mq_conf.keep_alive));
    /* Set a callback for a specified URI */
    evhttp_set_cb(g_event_http, "/stat", mq_evhttp_cb_do_view_stat, NULL);
    evhttp_set_cb(g_event_http, "/exec", mq_evhttp_cb_do_exec, NULL);
    /* Set a callback for all requests that are not caught by specific callbacks */
    evhttp_set_gencb(g_event_http, mq_evhttp_cb_do_opt_entry, NULL);

    if (pipe(pipe_fd) < 0)
    {
        log_error("Create pipe  ... failure, errno : %d", errno);
        event_base_free(g_event_base);
        evhttp_free(g_event_http);
        return RUN_STEP_PIPE_CREATE;
    }

    static struct event ev;
    static struct event tev;
    static struct event tev2;
    static struct timeval tv;
    /* Prepare an event structure to be added */
    event_set(&ev, pipe_fd[0], EV_READ|EV_PERSIST, mq_evhttp_cb_do_write_pipe, NULL);
    /* Add an event to the set of monitored events */
    event_add(&ev, NULL);

    event_set(&tev, -1, EV_TIMEOUT, mq_evhttp_cb_do_sync, &tev);
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    event_add(&tev, &tv);

    event_set(&tev2, -1, EV_TIMEOUT, mq_evhttp_cb_do_dump, &tev2);
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    event_add(&tev2, &tv);

    return RUN_STEP_EVENT_END;
}

int mq_http_start(void)
{
    int exit_code = 0;
    /* Threadsafe event dispatching loop */
    if((exit_code = event_base_dispatch(g_event_base)))
    {
        log_error("event_base_dispatch ... failure %d", exit_code);
    }
    return exit_code;
}

void mq_http_stop(void)
{
    log_info("Program Exit Normally");
    event_base_free(g_event_base);
    evhttp_free(g_event_http);
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    remove(g_mq_conf.pid_file);
}

/////////////////////////////////////////////////////////////////////////////////

static void mq_evhttp_ret_send(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables,
        int http_code,
        const char* http_reason)
{
    evhttp_send_reply(request, http_code, http_reason, content_out);
    evhttp_clear_headers(http_query_variables);
    evbuffer_free(content_out);
}

static void mq_evhttp_set_headers(
        struct evkeyvalq *headers_out,
        const struct evkeyvalq *headers_in,
        const char* type,
        const char* charset,
        int force_close)
{
    char buffer[64];
    const char *connection;

    if (type != NULL)
    {
        strcpy(buffer, type);
        if(charset && *charset && strlen(charset) <= (size_t)MAX_CHARSET_SIZE)
        {
            sprintf(buffer + strlen(buffer), "; charset=%s", charset);
        }
        evhttp_add_header(headers_out, "Content-Type", buffer);
    }

    evhttp_add_header(headers_out, "Cache-Control", "no-cache");
    connection = evhttp_find_header(headers_in, "Connection");
    if ((force_close == 0)
            && (connection != NULL)
            && (strncasecmp(connection, "keep-alive", 10) == 0))
    {
        evhttp_add_header(headers_out, "Connection", "keep-alive");
        evhttp_add_header(headers_out, "keep-alive", g_mq_conf.keep_alive);
    }
    else
    {
        evhttp_add_header(headers_out, "Connection", "close");
    }
}

/* The server does not take the initiative to close the connection. */
static void mq_evhttp_conn_close(struct evhttp_connection *connection, void *arg)
{
    char* remote_ip = NULL;
    unsigned short remote_port = 0;

    /* Get the remote address and port associated with this connection */
    evhttp_connection_get_peer(connection, &remote_ip, &remote_port);

    if(remote_port > 0 && remote_ip)
    { 
        log_debug("Connection (%s : %d) closed", remote_ip, remote_port);
    }
}

/////////////////////////////////////////////////////////////////////////////////

/* User browse input command for get one's queue status */
static void get_queue_stat(mq_queue_t *mq_queue, void* arg)
{
    uint64_t     queue_unread_count = 0;
    struct       evbuffer* content_out = (struct evbuffer*)arg;
    rtag_item_t  info;

    get_rtag_info(mq_queue, &info);
    if (&info != NULL)
    {
        char output_value[62];
        evbuffer_add_printf(content_out, "<TR><TD align=left>%s</TD>", mq_queue->qname);

        sprintf(output_value, "%"PRIu64, info.put_num);
        evbuffer_add_printf(content_out, "<TD align=center>%s</TD>",
                (info.put_num >= 0 ? output_value : "N/A"));

        sprintf(output_value, "%"PRIu64, info.get_num);
        evbuffer_add_printf(content_out, "<TD align=center>%s</TD>",
                (info.get_num >= 0 ? output_value : "N/A"));

        if( info.put_num >= 0 && info.get_num >= 0 && info.maxqueue >= 0 )
        {
            queue_unread_count = info.unread;
            sprintf(output_value, "%"PRIu64" (%.1f%%)", queue_unread_count,
                    (double)queue_unread_count * 100.0 / info.maxqueue);
        }
        evbuffer_add_printf(content_out, "<TD align=center>%s</TD>",
                (queue_unread_count >= 0 ? output_value : "N/A"));

        sprintf(output_value, "%u", info.maxqueue );
        evbuffer_add_printf(content_out, "<TD align=center>%s</TD>",
                (info.maxqueue >= 0 ? output_value : "N/A"));

        sprintf(output_value, "%u", info.delay);
        evbuffer_add_printf(content_out, "<TD align=center>%s</TD>",
                (info.delay >= 0 ? output_value : "N/A"));

        sprintf(output_value, "%u", info.wlock);
        evbuffer_add_printf(content_out, "<TD align=center>%s</TD></TR>\r\n",
                (info.wlock >= 0 ? output_value : "N/A"));
    }
}

/* User browse input command for get one's queue status */
static void get_queue_stat_json(mq_queue_t *mq_queue, int qlist_index, void* arg)
{
    uint64_t     queue_unread_count = 0;
    struct       evbuffer* content_out = (struct evbuffer*)arg;
    rtag_item_t  info;

    get_rtag_info(mq_queue, &info);
    if (&info != NULL)
    {
        char output_value[62];
        if (qlist_index == 0)
        {
            evbuffer_add_printf(content_out, "{\"name\":\"%s\",", mq_queue->qname);
        }
        else
        {
            evbuffer_add_printf(content_out, ",{\"name\":\"%s\",", mq_queue->qname);
        }

        sprintf(output_value, "%"PRIu64, info.put_num);
        evbuffer_add_printf(content_out, "\"putnum\":\"%s\",",
                (info.put_num >= 0 ? output_value : "N/A"));

        sprintf(output_value, "%"PRIu64, info.get_num);
        evbuffer_add_printf(content_out, "\"getnum\":\"%s\",",
                (info.get_num >= 0 ? output_value : "N/A"));

        if( info.put_num >= 0 && info.get_num >= 0 && info.maxqueue >= 0 )
        {
            queue_unread_count = info.unread;
            sprintf(output_value, "%"PRIu64" (%.1f%%)", queue_unread_count,
                    (double)queue_unread_count * 100.0 / info.maxqueue);
        }
        evbuffer_add_printf(content_out, "\"unread\":\"%s\",",
                (queue_unread_count >= 0 ? output_value : "N/A"));

        sprintf(output_value, "%u", info.maxqueue );
        evbuffer_add_printf(content_out, "\"maxqueue\":\"%s\",",
                (info.maxqueue >= 0 ? output_value : "N/A"));

        sprintf(output_value, "%u", info.delay);
        evbuffer_add_printf(content_out, "\"delay\":\"%s\",",
                (info.delay >= 0 ? output_value : "N/A"));

        sprintf(output_value, "%u", info.wlock);
        evbuffer_add_printf(content_out, "\"wlock\":\"%s\"}",
                (info.wlock >= 0 ? output_value : "N/A"));
    }
}

static void get_all_queue_stat(void* arg)
{
    mq_queue_list_t* tmp_queue;

    for(tmp_queue = g_mq_qlist; tmp_queue != NULL; tmp_queue = tmp_queue->hh.next) 
    {
        get_queue_stat(&tmp_queue->mq_queue, arg);
    }
}


static void get_all_queue_stat_json(void* arg)
{
    int qlist_index = 0;
    mq_queue_list_t* tmp_queue;

    for(tmp_queue = g_mq_qlist; tmp_queue != NULL; tmp_queue = tmp_queue->hh.next) 
    {
        get_queue_stat_json(&tmp_queue->mq_queue, qlist_index, arg);
        qlist_index++;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

static void mq_evhttp_cb_do_sync(int fd, short event, void *arg)
{
    int sync_start = 0;
    int sync_end = 0;
    struct timeval tv;
    static int last_sync = 0;

    gettimeofday(&tv, NULL);

    if(last_sync == 0)
    {
        last_sync = tv.tv_sec;
        log_debug("Next sync [%d] ms",
                (g_mq_conf.sync_time_interval > 0) ? tv.tv_sec + g_mq_conf.sync_time_interval : INT_MAX);
    }
    if (g_mq_conf.sync_time_interval != 0)
    {
        if (tv.tv_sec >= last_sync + g_mq_conf.sync_time_interval)
        {
            sync_start = tv.tv_sec;
            /* Store sync */
            mq_qm_sync_store();
            /* Calculate sync time and sync */
            sync_end  = get_cur_timestamp();
            log_debug("Sync store use of time[%d] s", max((sync_end - sync_start), 1)); 
        }
        last_sync = tv.tv_sec;
        tv.tv_sec = g_mq_conf.sync_time_interval;
    }
    else
    {
        tv.tv_sec = 10;
    }

    tv.tv_usec = 0;
    struct event *tev = (struct event *)arg;
    event_del(tev);
    event_add(tev, &tv);
}

static void mq_evhttp_cb_do_dump(int fd, short event, void *arg)
{
    struct timeval tv;
    static int next_dump = 0;

    gettimeofday(&tv, NULL);
    if ((next_dump == 0))
    {
        /* If 0 no sync db file */
        next_dump = tv.tv_sec + DUMP_INFO_INTERVAL;
        log_debug("Next dump [%d] ms", next_dump);
    }

    /* Status dump on one minute */
    if (tv.tv_sec >= next_dump)
    {
        //uint32_t diff;
        int diff;
        diff = g_info.put - g_info.get;
        /* Every minute print global info about the mq object */
        log_info("Put:%u, get:%u, diff:%u, count:%"PRIu64","
                " avg msg size:%u, avg time:%fms, max time:%fms",
                g_info.put,
                g_info.get,
                diff,
                g_info.count,
                (g_info.put != 0) ? g_info.put_size / g_info.put : 0,
                (g_info.times != 0) ? ns_2_ms * g_info.total_time / g_info.times : 0,
                ns_2_ms * g_info.max_time);

        /* 
         * if the queue cumulate more than 10,000,000 
         * or this minute cumulate more then 100,000 message warn 
         * */
        if ((g_info.count > MAX_QUEUE_WARNING) || (diff > INC_QUEUE_WARNING))
        {
            //log_warn("There are [%"PRIu64"] messages in mq, and [%u] new messages in last minute",
            log_warn("There are [%"PRIu64"] messages in mq, and [%d] new messages in last minute",
                    g_info.count,
                    diff);
        }
        g_last_info       = g_info;
        g_info.put        = 0;
        g_info.get        = 0;
        g_info.put_size   = 0;
        g_info.total_time = 0;
        g_info.times      = 0;
        g_info.max_time   = 0;
        next_dump         += DUMP_INFO_INTERVAL;
    }
    
    tv.tv_sec = next_dump - tv.tv_sec;
    tv.tv_usec = 0;
    struct event *tev = (struct event *)arg;
    event_del(tev);
    event_add(tev, &tv);
}

static void mq_evhttp_cb_do_write_pipe(int fd, short event, void *arg)
{
    if (fd == pipe_fd[0])
    {
        int read_pipe = -1;
        char c[1024] = {'\0'};
        struct timeval tv;

        do
        {
            read_pipe = read(fd, c, sizeof(c) - 1);
        } while((read_pipe < 0) && (errno == EINTR));

        log_info("Program get an exit string : %s", c);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        event_base_loopexit(g_event_base, &tv);
    }
}

static void mq_evhttp_cb_do_exec(struct evhttp_request *request, void *arg)
{
    char* ip = NULL;
    uint16_t port = 0;
    int http_code = HTTP_OK;
    const char* http_reason = "OK";

    evhttp_connection_get_peer(request->evcon, &ip, &port);
    struct evbuffer* content_out = evbuffer_new();

    /* If allow_exec_ip == 0, allow all ip to exec ucmq */
    if ((strcmp(g_mq_conf.allow_exec_ip, ip) != 0) 
            && (str_to_ll(g_mq_conf.allow_exec_ip) != 0))
    {
        log_warn("Client ip not allow exec this command, allow ip[%s], my ip[%s]",
                g_mq_conf.allow_exec_ip, ip);
        evbuffer_add_printf(content_out, "You can not exec cmd from %s \r\n", ip);
        evhttp_send_reply(request, 401, "Unauthorized", content_out);
        evbuffer_free(content_out);
        return;
    }

    struct evkeyvalq http_query_variables;
    const  char* request_uri = evhttp_request_uri(request);
    if(request_uri != NULL)
    {
        evhttp_parse_query(request_uri, &http_query_variables);
    }
    const char* cmd = evhttp_find_header(&http_query_variables, "cmd");
    if (cmd == NULL)
    {
        evbuffer_add_printf(content_out, "You can not find exec cmd \r\n");
        mq_evhttp_ret_send(request, content_out, &http_query_variables, HTTP_BADREQUEST, "Unknown cmd");
        return;
    }

    /* Add Response Headers */
    mq_evhttp_set_headers(request->output_headers, request->input_headers, "text/plain",
            evhttp_find_header(&http_query_variables, "charset"), 1);
    log_info("Ready to exec cmd from %s:%d - %s", ip, port, cmd);

    bool    exec = false;
    bool    show_result = true;

    if (strcmp(cmd, "kill") == 0)
    {
        const char* exit_str = "kill by remote request";
        mq_qm_sync_store();
        write(pipe_fd[1], exit_str, strlen(exit_str));
        exec = true;
    }
    if (strcmp(cmd, "reload") == 0)
    {
        read_conf_file();
        exec = true;
    }
    if (strcmp(cmd, "get") == 0)
    {
        exec = true;
        show_result = false;

        const char* file = evhttp_find_header(&http_query_variables, "file");
        if (file == NULL)
        {
            evbuffer_add_printf(content_out, "no files\r\n");
        }
        else
        {
            int     size = 0;
            char    buf[MAX_INFO_SIZE];
            char    path[PATH_MAX];

            snprintf(path, sizeof(path) - 1, "/proc/%d/%s", getpid(), file);
            buf[sizeof(path) - 1] = '\0';
            size = read_file(buf, sizeof(buf), path);
            if (size < 0)
            {
                evbuffer_add_printf(content_out, "%s\r\n", buf);
            }
            else
            {
                evbuffer_add(content_out, buf, size);
            }
        }
    }
    if (exec == true)
    {
        if (show_result == true)
        {
            evbuffer_add_printf(content_out, "Exec cmd : %s from %s:%d \r\n", cmd, ip, port);
            evbuffer_add_printf(content_out, "Exec cmd result : ok\r\n");
        }
    }
    else
    {
        evbuffer_add_printf(content_out, "Unknown cmd : %s\r\n", cmd);
    }
    mq_evhttp_ret_send(request, content_out, &http_query_variables, http_code, http_reason);
}

static void mq_evhttp_cb_do_view_stat(struct evhttp_request *request, void *arg)
{
    struct evbuffer    *content_out = NULL;
    struct evkeyvalq   http_query_variables;

    assert((content_out = evbuffer_new()));
    mq_evhttp_set_headers(request->output_headers, request->input_headers, "text/html", NULL, 0);

    const char* request_uri = evhttp_request_uri(request);
    if(request_uri != NULL)
    {
        evhttp_parse_query(request_uri, &http_query_variables);
    }

    const char* lpcType = evhttp_find_header(&http_query_variables, "type");
    if (lpcType == NULL)
    {
        evbuffer_add_printf(content_out, 
                "<HTML>\r\n<HEAD>\r\n<TITLE>UCMQ STAT INFORMATION v%s (%s %s)</TITLE>\r\n",
                VERSION, __DATE__, __TIME__);
        evbuffer_add_printf(content_out, "<META HTTP-EQUIV=\"refresh\" CONTENT=\"5\">\r\n</HEAD>\r\n");
        evbuffer_add_printf(content_out, "<BODY style=\"font-family:verdana\">\r\n<TABLE width=90%% "
                "align=center border=1 cellspacing=0>\r\n");
        evbuffer_add_printf(content_out,
                "<TR bgcolor=yellow> \
                <TH width=20%% align=left>QUEUE NAME</TH> \
                <TH align=center>WRITE_POS</TH> \
                <TH align=center>READ_POS</TH> \
                <TH align=center>UNREAD(%%)</TH> \
                <TH align=center>MAXQUEUE</TH> \
                <TH align=center>DELAY</TH> \
                <TH align=center>WLOCK</TH> \
                </TR>\r\n");
        get_all_queue_stat(content_out);
        evbuffer_add_printf(content_out, "</TABLE>\r\n</BODY>\r\n</HTML>");
    }
    else
    {
        if (strncmp(lpcType, "json", strlen("json")) == 0)
        {
            evbuffer_add_printf(content_out, "%s", "[");
            get_all_queue_stat_json(content_out);
            evbuffer_add_printf(content_out, "%s", "]");
        }
        else if (strncmp(lpcType, "info", strlen("info")) == 0)
        {
            uint32_t avg_size;
            double   avg_time;
            double   max_time;

            avg_size = (g_last_info.put != 0) ? g_last_info.put_size / g_last_info.put : 0;
            avg_time = (g_last_info.times != 0) ? ns_2_ms * g_last_info.total_time / g_last_info.times : 0;
            max_time = ns_2_ms * g_last_info.max_time;
            evbuffer_add_printf(content_out,
                    "{\"put\":\"%u\",\"get\":%u,\"avg size\":%u,\"avg op time\":%f,\"max op time\":%f}", 
                    g_last_info.put,
                    g_last_info.get,
                    avg_size,
                    avg_time,
                    max_time);
        }
        else if (strncmp(lpcType, "mem", strlen("mem")) == 0)
        {
            if (g_statm_path[0] == '\0')
            {
                snprintf(g_statm_path, sizeof(g_statm_path) - 1, "/proc/%d/statm", getpid());
                g_statm_path[sizeof(g_statm_path) - 1] = '\0';
            }

            char    buf[MAX_INFO_SIZE];
            int     size = read_file(buf, sizeof(buf), g_statm_path);

            if (size < 0)
            {
                evbuffer_add_printf(content_out, "%s\r\n", buf);
            }
            else
            {
                unsigned long long values[7] = {0};
                sscanf(buf, "%llu%llu%llu%llu%llu%llu%llu",
                        &values[0], &values[1], &values[2], &values[3], &values[4], &values[5], &values[6]);

                evbuffer_add_printf(content_out, "{\"virt\":\"%llu\",\"res\":%llu,\"shr\":%llu,"
                        "\"trs\":%llu,\"lrs\":%llu,\"drs\":%llu,\"dt\":%llu}", values[0] << 2,
                        values[1] << 2, values[2] << 2, values[3] << 2, values[4] << 2, 
                        values[5] << 2, values[6] << 2);
            }
        }
        else if (strncmp(lpcType, "cpu", strlen("cpu")) == 0)
        {
            if (g_stat_path[0] == '\0')
            {
                snprintf(g_stat_path, sizeof(g_stat_path) - 1, "/proc/%d/stat", getpid());
                g_stat_path[sizeof(g_stat_path) - 1] = '\0';
            }

            char    buf[MAX_INFO_SIZE];
            bool    result = true;
            int     size = 0;
            unsigned long long values[13] = {0};

            size = read_file(buf, sizeof(buf), g_stat_path);
            if (size < 0)
            {
                evbuffer_add_printf(content_out, "%s\r\n", buf);
                result = false;
            }
            else
            {
                sscanf(buf, "%*s%*s%*s%*s%*s%*s%*s%*s%*s%*s%*s%*s%*s%llu%llu%llu%llu",
                        &values[0], &values[1], &values[2], &values[3]);
            }

            size = read_file(buf, sizeof(buf), "/proc/stat");
            if (size < 0)
            {
                evbuffer_add_printf(content_out, "%s\r\n", buf);
                result = false;
            }
            else
            {
                sscanf(buf, "%*s%llu%llu%llu%llu%llu%llu%llu%llu%llu\n", &values[4], &values[5],
                        &values[6], &values[7], &values[8], &values[9], &values[10], &values[11],
                        &values[12]);
            }
            if (result == true)
            {
                evbuffer_add_printf(content_out, "{\"utime\":\"%llu\",\"stime\":%llu,\"cutime\":%llu,"
                        "\"cstime\":%llu,\"user\":%llu,\"nice\":%llu,\"system\":%llu,\"idle\":%llu,"
                        "\"iowait\":%llu,\"irq\":%llu,\"softirq\":%llu,\"steal\":%llu,\"guest\":%llu}", 
                        values[0], values[1], values[2], values[3], values[4], values[5], values[6],
                        values[7], values[8], values[9], values[10], values[11], values[12]);
            }
        }
    }
    mq_evhttp_ret_send(request, content_out, &http_query_variables, HTTP_OK, "OK");
}

/////////////////////////////////////////////////////////////////////////////////

static bool req_qname_check(const char* qname)
{ 
    if(qname == NULL)
    {
        return false;
    }
    const int qname_len = (qname != NULL) ? strlen(qname) : -1;
    /* Queue name is too longer or empty */
    if (qname_len <= 0 || qname_len > MAX_QUEUE_NAME_LEN)
    {
        return false;
    }

    int i = 0;
    const char* str = qname;

    while (str[i] != '\0') 
    {   
        /* Check queue name */
        if (!((str[i] >= '0') && (str[i] <= '9'))    
                && (!(str[i] >= 'a' && str[i]  <= 'z') && !(str[i] >= 'A' && str[i]  <= 'Z')))
        {   
            if ((!(str[i] == '_'))
                    || ((str[i] == '_') && (i == 0)) 
                    || ((str[i + 1] == '\0') && (str[i] == '_')))
            {   
                log_warn("Queue name is invalid, invalid char[%c]", str[i]); 
                return false; 
            }   
        }   
        i++;
    }   

    log_debug("queue_name [%s] len [%d]", qname, qname_len); 

    return true;
}

/* Find queue by queue name */
static mq_queue_t* find_queue_by_qname(
        const char* qname,
        struct evkeyvalq* http_query_variables)
{
    mq_queue_t* tmp_queue = NULL;

    tmp_queue = mq_qm_find_queue(qname);
    if(tmp_queue == NULL)
    {
        log_debug("Can't find the queue [%s] from the queue list", qname); 
        return false;
    }
    log_debug("Find the queue [%s] from list... ok", tmp_queue->qname);
    return tmp_queue;
}

/* Touch queue by queue name */
static mq_queue_t* touch_queue_by_qname(
        const char* qname,
        struct evkeyvalq* http_query_variables)
{
    mq_queue_t* tmp_queue = NULL;

    tmp_queue = find_queue_by_qname(qname, http_query_variables);
    if (tmp_queue == NULL)
    {
        log_debug("Create the queue [%s]now", qname); 
        tmp_queue = mq_qm_add_queue(qname);
        if(tmp_queue == NULL)
        {
            log_warn("Unable create the queue[%s]", qname);
            return false;
        }
        log_info("Create the queue [%s] ...success", qname); 
    }
    return tmp_queue;
}

static void http_opt_put(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    char   buffer[64];
    int    result = -1;
    int    put_position = -1;
    int    http_code = HTTP_OK;
    const char* http_reason = "OK";
    msg_item_t msg_item;
    mq_queue_t* mq_queue = NULL;

    /* parse REQ */
    int   buffer_size = EVBUFFER_LENGTH(request->input_buffer);
    const char* ver   = evhttp_find_header(http_query_variables, "ver");
    const char* data  = evhttp_find_header(http_query_variables, "data");
    const char* qname = evhttp_find_header(http_query_variables, "name");

    if (!req_qname_check(qname))
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_UNKNOWN_NAME,
                "HTTPSQS_PUT_ERROR",
                "UCMQ_HTTP_ERR_INV_NAME",
                ver);
        return;
    }

    /* URI arg check */
    if ((buffer_size > 0 && data != NULL) 
            || (buffer_size <= 0 && data == NULL))
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_INVALID_ARG,
                "HTTPSQS_PUT_ERROR",
                "UCMQ_HTTP_ERR_INV_DATA", 
                ver);
        return;
    }
    /* HTTP POST Data */
    if (buffer_size > 0)
    {
        msg_item.msg = (char*)EVBUFFER_DATA(request->input_buffer);
        msg_item.len = buffer_size;
    }
    /* HTTP GET Data */
    if(data != NULL)
    {
        buffer_size = strlen(data);
        msg_item.msg = (char *)data;
        msg_item.len = buffer_size;
    }

    /* Data len check */
    if (msg_item.len <= 0
            || msg_item.len > MAX_DB_FILE_SIZE - DB_FILE_HEAD_LEN - MSG_HEAD_LEN)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_INVALID_ARG,
                "HTTPSQS_PUT_ERROR",
                "UCMQ_HTTP_ERR_INV_DATA", 
                ver);
        return;
    }

    /* Find name from queue list, if not find creat it */
    mq_queue = touch_queue_by_qname(qname, http_query_variables);
    if (mq_queue == NULL)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST, 
                MQ_HTTP_REASON_ADD_QUE_ERR,
                "HTTPSQS_PUT_ERROR",
                "UCMQ_HTTP_ERR_QUE_ADD_ERR",
                ver);
        return;
    }

    /* Write messsge */
    result = mq_qm_push_item(mq_queue, &msg_item);
    switch (result)
    {
        case QUEUE_WLOCK:
            MQ_DO_REPLY(HTTP_BADREQUEST, 
                    MQ_HTTP_REASON_QUE_WLOCK,
                    "HTTPSQS_PUT_READONLY", 
                    "UCMQ_HTTP_ERR_WLOCK",
                    ver);
            break;
        case QUEUE_FULL:
            MQ_DO_REPLY(HTTP_BADREQUEST,
                    MQ_HTTP_REASON_QUE_FULL,
                    "HTTPSQS_PUT_FULL",
                    "UCMQ_HTTP_ERR_QUE_FULL",
                    ver);
            break;
        case QUEUE_PUT_OK:
            mq_queue->unread_count += 1;
            g_info.put_size        += buffer_size;
            g_info.put             += 1;
            g_info.count           += 1;

            put_position = mq_queue->put_num;
            sprintf(buffer, "%d", put_position);
            evhttp_add_header(request->output_headers, "Pos", buffer);
            MQ_DO_REPLY(HTTP_OK, "OK", "HTTPSQS_PUT_OK", "UCMQ_HTTP_OK", ver);
            break;
        default:
            MQ_DO_REPLY(HTTP_BADREQUEST, 
                    MQ_HTTP_REASON_PUT_ERR,
                    "HTTPSQS_PUT_ERROR",
                    "UCMQ_HTTP_ERR_PUT_ERR", 
                    ver);
            break;
    }
}

static void http_opt_get(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    char   buffer[64];
    int    result = -1;
    int    get_position = -1;
    int    http_code = HTTP_OK;
    const char* http_reason = "OK";
    msg_item_t msg_item;
    mq_queue_t* mq_queue = NULL;

    /* Parse REQ */
    const char *ver = evhttp_find_header(http_query_variables, "ver");
    const char* qname = evhttp_find_header(http_query_variables, "name");

    if (!req_qname_check(qname))
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_UNKNOWN_NAME,
                "HTTPSQS_GET_ERROR",
                "UCMQ_HTTP_ERR_INV_NAME",
                ver);
        return;
    }

    /* Find name from queue list */
    mq_queue = find_queue_by_qname(qname, http_query_variables);
    if (mq_queue == NULL)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_QUE_NO_EXIST, 
                "HTTPSQS_GET_ERROR",
                "UCMQ_HTTP_ERR_QUE_NO_EXIST",
                ver);
        return;
    }

    /* Pop msg item from queue */
    result = mq_qm_pop_item(mq_queue, &msg_item);
    switch (result)
    {
        case QUEUE_GET_END:
            MQ_DO_REPLY(HTTP_NOTFOUND,
                    MQ_HTTP_REASON_QUE_EMPTY,
                    "HTTPSQS_GET_END",
                    "UCMQ_HTTP_ERR_QUE_EMPTY",
                    ver);
            break;
        case QUEUE_GET_OK:
            if (ver != NULL && strcmp(ver, "2") == 0)   
            {
                evbuffer_add_printf(content_out, "%s\r\n", "UCMQ_HTTP_OK");
            }
            /* Take the result insert into content_out */
            evbuffer_add(content_out, msg_item.msg, msg_item.len);

            g_info.get   += 1;
            g_info.count -= 1;
            http_code    = HTTP_OK;
            http_reason = "OK";
            get_position = mq_queue->get_num;
            sprintf(buffer, "%d", get_position);
            evhttp_add_header(request->output_headers, "Pos", buffer);
            mq_evhttp_ret_send(request, content_out, http_query_variables, http_code, http_reason);
            break;
        default:
            MQ_DO_REPLY(HTTP_BADREQUEST,
                    MQ_HTTP_REASON_GET_ERR,
                    "HTTPSQS_GET_ERROR", 
                    "UCMQ_HTTP_ERR_GET_ERR",
                    ver);
            break;
    }
}

static void http_opt_status(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    int http_code = HTTP_OK;
    const char* http_reason = "OK";
    rtag_item_t rtag_item;
    mq_queue_t* mq_queue = NULL;

    /* Parse REQ */
    const char *ver = evhttp_find_header(http_query_variables, "ver");
    const char* qname = evhttp_find_header(http_query_variables, "name");

    if (!req_qname_check(qname))
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_UNKNOWN_NAME,
                "HTTPSQS_ERROR",
                "UCMQ_HTTP_ERR_INV_NAME",
                ver);
        return;
    }

    /* Find name from queue list */
    mq_queue = find_queue_by_qname(qname, http_query_variables);
    if (mq_queue == NULL)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_QUE_NO_EXIST,
                "HTTPSQS_ERROR",
                "UCMQ_HTTP_ERR_QUE_NO_EXIST",
                ver);
        return;
    }

    get_rtag_info(mq_queue, &rtag_item);
    evbuffer_add_printf(content_out, "HTTP Simple Message Queue Service v%s (%s %s)\n\n",
            VERSION, __DATE__, __TIME__);
    evbuffer_add_printf(content_out, "Queue Name: %s\n", mq_queue->qname);
    evbuffer_add_printf(content_out, "Put the number of queue : %"PRIu64"\n", MQ_SHOW_STATUS(put_num));
    evbuffer_add_printf(content_out, "Get the number of queue : %"PRIu64"\n", MQ_SHOW_STATUS(get_num));
    evbuffer_add_printf(content_out, "Unread the number of queue : %"PRIu64"\n", MQ_SHOW_STATUS(unread));
    evbuffer_add_printf(content_out, "Maximum number of queue: %u\n", MQ_SHOW_STATUS(maxqueue));
    evbuffer_add_printf(content_out, "Delay time of queue: %u\n", MQ_SHOW_STATUS(delay));
    evbuffer_add_printf(content_out, "Queue write lock cut-OFF time: %d\n", MQ_SHOW_STATUS(wlock));

    mq_evhttp_ret_send(request, content_out, http_query_variables, http_code, http_reason);
}

static void http_opt_status_json(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    int http_code = HTTP_OK;
    const char* http_reason = "OK";
    rtag_item_t rtag_item;
    mq_queue_t* mq_queue = NULL;

    /* Parse REQ */
    const char *ver = evhttp_find_header(http_query_variables, "ver");
    const char* qname = evhttp_find_header(http_query_variables, "name");

    if (!req_qname_check(qname))
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_UNKNOWN_NAME,
                "HTTPSQS_ERROR",
                "UCMQ_HTTP_ERR_INV_NAME",
                ver);
        return;
    }

    /* Find name from queue list */
    mq_queue = find_queue_by_qname(qname, http_query_variables);
    if (mq_queue == NULL)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_QUE_NO_EXIST,
                "HTTPSQS_ERROR",
                "UCMQ_HTTP_ERR_QUE_NO_EXIST", 
                ver);
        return;
    }

    get_rtag_info(mq_queue, &rtag_item);
    evbuffer_add_printf(content_out,
            "{\"name\":\"%s\",\"putnum\":%"PRIu64",\"getnum\":%"PRIu64","
            "\"unread\":%"PRIu64",\"maxqueue\":%u,\"delay\":%u,\"wlock\":%d}",
            mq_queue->qname,
            MQ_SHOW_STATUS(put_num),
            MQ_SHOW_STATUS(get_num),
            MQ_SHOW_STATUS(unread),
            MQ_SHOW_STATUS(maxqueue),
            MQ_SHOW_STATUS(delay),
            MQ_SHOW_STATUS(wlock));

    mq_evhttp_ret_send(request, content_out, http_query_variables, http_code, http_reason);
}

static void http_opt_maxqueue(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    int dest_num = -1;
    int http_code = HTTP_OK;
    const char* http_reason = "OK";
    mq_queue_t* mq_queue;

    /* Parse REQ */
    const char *ver     = evhttp_find_header(http_query_variables, "ver");
    const char* qname   = evhttp_find_header(http_query_variables, "name");
    const char *tmp_num = evhttp_find_header(http_query_variables, "num");
    if(tmp_num != NULL)
    {
        dest_num = atoi(tmp_num);
    }   

    /* Queue name check */
    if (!req_qname_check(qname))
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_UNKNOWN_NAME,
                "HTTPSQS_MAXQUEUE_CANCEL",
                "UCMQ_HTTP_ERR_INV_NAME",
                ver);
        return;
    }

    /* NUM check , if 0 no limit */
    if(dest_num < 0 || dest_num > QUEUE_MAX_SIZE)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_INVALID_NUM,
                "HTTPSQS_MAXQUEUE_CANCEL",
                "UCMQ_HTTP_ERR_INV_NUM",
                ver);
        return;
    }

    /* Find name from queue list, if not find creat it */
    mq_queue = touch_queue_by_qname(qname, http_query_variables);
    if (mq_queue == NULL)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_ADD_QUE_ERR,
                "HTTPSQS_MAXQUEUE_CANCEL",
                "UCMQ_HTTP_ERR_QUE_ADD_ERR",
                ver);
        return;
    }

    MQ_DO_AND_REPLY(mq_qm_set_maxqueue(mq_queue, dest_num),
            "HTTPSQS_MAXQUEUE_OK",
            "UCMQ_HTTP_OK",
            HTTP_BADREQUEST,
            MQ_HTTP_REASON_SET_MAXQUE_ERR,
            "HTTPSQS_MAXQUEUE_CANCEL",
            "UCMQ_HTTP_ERR_MAXQUE_ERR",
            ver);
}

static void http_opt_delay(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    int dest_num = -1;
    int http_code = HTTP_OK;
    const char* http_reason = "OK";
    mq_queue_t* mq_queue;

    /* Parse REQ */
    const char *ver     = evhttp_find_header(http_query_variables, "ver");
    const char* qname   = evhttp_find_header(http_query_variables, "name");
    const char *tmp_num = evhttp_find_header(http_query_variables, "num");
    if(tmp_num != NULL)
    {
        dest_num = atoi(tmp_num);
    }   

    if (!req_qname_check(qname))
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_UNKNOWN_NAME,
                "HTTPSQS_DELAY_CANCEL",
                "UCMQ_HTTP_ERR_INV_NAME",
                ver);
        return;
    }

    if(dest_num < 0)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_INVALID_NUM,
                "HTTPSQS_DELAY_CANCEL",
                "UCMQ_HTTP_ERR_INV_NUM",
                ver);
    }

    /* Find name from queue list, if not find creat it */
    mq_queue = touch_queue_by_qname(qname, http_query_variables);
    if (mq_queue == NULL)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_ADD_QUE_ERR,
                "HTTPSQS_DELAY_CANCEL",
                "UCMQ_HTTP_ERR_QUE_ADD_ERR",
                ver);
        return;
    }

    MQ_DO_AND_REPLY(mq_qm_set_delay(mq_queue, dest_num),
            "HTTPSQS_DELAY_OK",
            "UCMQ_HTTP_OK",
            HTTP_BADREQUEST,
            MQ_HTTP_REASON_SET_DELAY_ERR,
            "HTTPSQS_DELAY_CANCEL",
            "UCMQ_HTTP_ERR_DELAY_ERR",
            ver);
}

static void http_opt_wlock(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    int dest_num = -1;
    int http_code = HTTP_OK;
    const char* http_reason = "OK";
    mq_queue_t* mq_queue;

    /* Parse REQ */
    const char* ver     = evhttp_find_header(http_query_variables, "ver");
    const char* qname   = evhttp_find_header(http_query_variables, "name");
    const char* tmp_num = evhttp_find_header(http_query_variables, "num");
    if(tmp_num != NULL)
    {
        dest_num = atoi(tmp_num);
    }   

    if (!req_qname_check(qname))
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_UNKNOWN_NAME,
                "HTTPSQS_READONLY_CANCEL",
                "UCMQ_HTTP_ERR_INV_NAME",
                ver);
        return;
    }

    if(dest_num < 0 && dest_num != -1)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_INVALID_NUM,
                "HTTPSQS_READONLY_CANCEL",
                "UCMQ_HTTP_ERR_INV_NUM",
                ver);
    }
    /* Find name from queue list, if not find creat it */
    mq_queue = find_queue_by_qname(qname, http_query_variables);
    if (mq_queue == NULL)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_ADD_QUE_ERR,
                "HTTPSQS_READONLY_CANCEL",
                "UCMQ_HTTP_ERR_QUE_ADD_ERR", 
                ver);
        return;
    }

    MQ_DO_AND_REPLY(mq_qm_set_wlock(mq_queue, dest_num),
            "HTTPSQS_READONLY_OK",
            "UCMQ_HTTP_OK",
            HTTP_BADREQUEST,
            MQ_HTTP_REASON_SET_WLOCK_ERR,
            "HTTPSQS_READONLY_CANCEL",
            "UCMQ_HTTP_ERR_WLOCK_ERR",
            ver);
}

static void http_opt_remove(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    int http_code = HTTP_OK;
    const char* http_reason = "OK";
    mq_queue_t* mq_queue;

    /* Parse REQ */
    const char *ver   = evhttp_find_header(http_query_variables, "ver");
    const char* qname = evhttp_find_header(http_query_variables, "name");

    if (!req_qname_check(qname))
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_UNKNOWN_NAME,
                "HTTPSQS_REMOVE_ERROR",
                "UCMQ_HTTP_ERR_INV_NAME",
                ver);
        return;
    }

    /* Find name from queue list */
    mq_queue = find_queue_by_qname(qname, http_query_variables);
    if (mq_queue == NULL)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_QUE_NO_EXIST,
                "HTTPSQS_REMOVE_ERROR",
                "UCMQ_HTTP_ERR_QUE_NO_EXIST",
                ver);
        return;
    }
    MQ_DO_AND_REPLY(mq_qm_del_queue(qname),
            "HTTPSQS_REMOVE_OK",
            "UCMQ_HTTP_OK",
            HTTP_BADREQUEST,
            MQ_HTTP_REASON_REMOVE_ERR,
            "HTTPSQS_REMOVE_ERROR",
            "UCMQ_HTTP_ERR_REMOVE_ERR",
            ver);
}

static void http_opt_reset(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    int http_code = HTTP_OK;
    const char* http_reason = "OK";
    mq_queue_t* mq_queue;

    /* Parse REQ */
    const char* ver   = evhttp_find_header(http_query_variables, "ver");
    const char* qname = evhttp_find_header(http_query_variables, "name");

    if (!req_qname_check(qname))
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_UNKNOWN_NAME,
                "HTTPSQS_RESET_ERROR",
                "UCMQ_HTTP_ERR_INV_NAME",
                ver);
        return;
    }

    /* Find name from queue list */
    mq_queue = find_queue_by_qname(qname, http_query_variables);
    if (mq_queue == NULL)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_QUE_NO_EXIST,
                "HTTPSQS_RESET_ERROR",
                "UCMQ_HTTP_ERR_QUE_NO_EXIST",
                ver);
        return;
    }
    mq_qm_del_queue(qname);
    MQ_DO_AND_REPLY((mq_qm_add_queue(qname) != NULL), 
            "HTTPSQS_RESET_OK",
            "UCMQ_HTTP_OK",
            HTTP_BADREQUEST,
            MQ_HTTP_REASON_RESET_ERR, 
            "HTTPSQS_RESET_ERROR",
            "UCMQ_HTTP_ERR_RESET_ERR",
            ver);
}

static void http_opt_synctime(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    int dest_num  = -1;
    int http_code = HTTP_OK;
    const char* http_reason = "OK";

    /* Parse REQ */
    const char *ver     = evhttp_find_header(http_query_variables, "ver");
    const char *tmp_num = evhttp_find_header(http_query_variables, "num");
    if(tmp_num != NULL)
    {
        dest_num = atoi(tmp_num);
    }   

    if(dest_num < 0 && dest_num != -1)
    {
        MQ_DO_REPLY(HTTP_BADREQUEST,
                MQ_HTTP_REASON_INVALID_NUM,
                "HTTPSQS_SYNC_TIME_CANCEL",
                "UCMQ_HTTP_ERR_INV_NUM",
                ver);
    }

    /* Set sync time interval */
    MQ_DO_AND_REPLY(mq_qm_set_synctime(dest_num),
            "HTTPSQS_SYNC_TIME_OK",
            "UCMQ_HTTP_OK",
            HTTP_BADREQUEST,
            MQ_HTTP_REASON_SET_SYNCTIME_ERR,
            "HTTPSQS_SYNC_TIME_CANCEL",
            "UCMQ_HTTP_ERR_SYNC_TIEM_ERR",
            ver);
}

static void http_unknown_opt(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    int http_code   = 405;
    const char* http_reason = MQ_HTTP_REASON_UNKNOWN_OPT;

    /* Parse REQ */
    const char *ver = evhttp_find_header(http_query_variables, "ver");

    MQ_DO_REPLY(http_code, http_reason, "HTTPSQS_UNKNWON_OPT", "UCMQ_HTTP_ERR_UNKNOWN_OPT", ver);
}

static void http_unparse_req(
        struct evhttp_request* request,
        struct evbuffer* content_out,
        struct evkeyvalq* http_query_variables)
{
    int http_code   = 400;
    const char* http_reason = MQ_HTTP_REASON_BAD_REQ;

    /* Parse REQ */
    const char *ver = evhttp_find_header(http_query_variables, "ver");

    MQ_DO_REPLY(http_code, http_reason, "HTTPSQS_UNPARSE_URI", "UCMQ_HTTP_ERR_BAD_REQ", ver);
}

/* Based business entrance */
static void mq_evhttp_cb_do_opt_entry(struct evhttp_request *request, void *arg)
{
    int           cur_time = 0;
    const char*   opt = NULL;
    const char*   request_uri = NULL;

    struct timespec     tm_start;
    struct timespec     tm_end;
    struct evbuffer*    content_out = NULL;
    struct evkeyvalq    http_query_variables;

    clock_gettime(CLOCK_MONOTONIC, &tm_start);
    assert((content_out = evbuffer_new()));
    evhttp_connection_set_closecb(request->evcon, mq_evhttp_conn_close, NULL);

    /* Get variables from request line, And parse REQ */
    if((request_uri = evhttp_request_uri(request)))
    {
        log_debug("Requst uri[%s]", request_uri);
        if(evhttp_parse_query(request_uri, &http_query_variables) < 0)
        {
            log_info("evhttp_parse_query_str parse error");
            http_unparse_req(request, content_out, &http_query_variables);
            return;
        }
    }

    /* Add response headers */
    mq_evhttp_set_headers(request->output_headers, request->input_headers, "text/plain",
            evhttp_find_header(&http_query_variables, "charset"), 0);

    /* protocol version flag */
    opt = evhttp_find_header(&http_query_variables, "opt");
    if (opt == NULL)
    {
        log_debug("Execution request fail, opt is NULL"); 
        http_unknown_opt(request, content_out, &http_query_variables);
    }
    /* Write a message into a queue */
    else if(strcmp(opt, "put") == 0)
    {
        log_debug("Do opt put"); 
        http_opt_put(request, content_out, &http_query_variables);
    }
    /* Read a message from a queue */
    else if(strcmp(opt, "get") == 0)
    {
        log_debug("Do opt get"); 
        http_opt_get(request, content_out, &http_query_variables);
    }
    /* Get queue status information */
    else if(strcmp(opt, "status") == 0)
    {
        log_debug("Do opt status"); 
        http_opt_status(request, content_out, &http_query_variables);
    }
    /* Get queue status information using json return */
    else if(strcmp(opt, "status_json") == 0)
    {
        log_debug("Do opt status_json"); 
        http_opt_status_json(request, content_out, &http_query_variables);
    }
    /* Set how many number of queue for store message */
    else if(strcmp(opt, "maxqueue") == 0)
    {
        log_debug("Do opt maxqueue"); 
        http_opt_maxqueue(request, content_out, &http_query_variables); 
    }
    /* Set queue delay for all message how many seconds */
    else if(strcmp(opt, "delay") == 0)
    {
        log_debug("Do opt delay"); 
        http_opt_delay(request, content_out, &http_query_variables); 
    }
    /* Set queue read only how many seconds */
    else if(strcmp(opt, "wlock") == 0 || strcmp(opt, "readonly") == 0)
    {
        log_debug("Do opt wlock"); 
        http_opt_wlock(request, content_out, &http_query_variables); 
    }
    /* Remove queue or reset queue */
    else if(strcmp(opt, "remove") == 0)
    {
        log_debug("Do opt remove"); 
        http_opt_remove(request, content_out, &http_query_variables); 
    }
    /* reset queue */
    else if (strcmp(opt, "reset") == 0)
    {    
        log_debug("Do opt reset"); 
        http_opt_reset(request, content_out, &http_query_variables); 
    }    
    else if (strcmp(opt, "synctime") == 0)
    {
        log_debug("Do opt synctime"); 
        http_opt_synctime(request, content_out, &http_query_variables); 
    }
    else
    {
        log_debug("Unknow opt"); 
        http_unknown_opt(request, content_out, &http_query_variables);
    }

    /* Record  processing time */
    clock_gettime(CLOCK_MONOTONIC, &tm_end);
    cur_time = (tm_end.tv_sec - tm_start.tv_sec) * s_2_ns + tm_end.tv_nsec - tm_start.tv_nsec;
    g_info.times        += 1;
    g_info.total_time   += cur_time;
    if (g_info.max_time < cur_time)
    {
        g_info.max_time = cur_time;
    }
}

static void get_rtag_info(mq_queue_t *mq_queue, rtag_item_t *rtag_item)
{
    rtag_item->rcount     = mq_queue->cur_rdb.opt_count;
    rtag_item->rpos       = mq_queue->cur_rdb.pos;
    rtag_item->wpos       = mq_queue->cur_wdb.pos;
    rtag_item->put_num    = mq_queue->put_num;
    rtag_item->get_num    = mq_queue->get_num;
    rtag_item->unread     = mq_queue->unread_count;
    rtag_item->maxqueue   = mq_queue->maxque;
    rtag_item->delay      = mq_queue->delay;
    rtag_item->wlock      = mq_queue->wlock;
}
