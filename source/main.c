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

#define __USE_XOPEN_EXTENDED
#define __USE_POSIX199309
#include <evhttp.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>

#include "log.h"
#include "mq_util.h"
#include "mq_config.h"
#include "mq_evhttp.h"
#include "mq_queue_manage.h"

/////////////////////////////////////////////////////////////////////////////////

#define SET_STR_OPT(x)                                  \
{                                                       \
    strncpy(x,optarg,sizeof(x));                        \
    x[sizeof(x)-1]='\0';                                \
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

static void print_version(void);
static void print_usage(void);
static void wait4_child(const int sig);
static void set_daemon(void);
static void mq_store_destroy(void);
static void exec_safe_exit(int sig);
static void set_signal_handle(void);
static int  mq_init_store(void);
static int  main_entrance(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////

static void print_version(void)
{
    fprintf(stdout, "ucmq :\n");
    fprintf(stdout, "\tversion - v%s\n", VERSION);
    fprintf(stdout, "\tstorage mode - file\n");
    fprintf(stdout, "\tbuild date - %s %s\n", __DATE__, __TIME__);
    fprintf(stdout, "dependency packages :\n");
    fprintf(stdout, "\tlibevent - v%s\n", event_get_version());
}

static void print_usage(void)
{
    fprintf(stdout, "-----------------------------------------------------------------------------------\n");
    fprintf(stdout, "HTTP Simple Message Queue Service - ucmq v%s (%s %s)\n\n", VERSION, __DATE__, __TIME__);
    fprintf(stdout, "   -c            config file path\n");
    fprintf(stdout, "   -d            run as a daemon\n");
    fprintf(stdout, "   -v, --version print version and exit\n");
    fprintf(stdout, "   -h, --help    print this help and exit\n\n");
    fprintf(stdout, "Note1: Use command \"killall ucmq\" and \"kill `cat /tmp/ucmq.pid`\" to stop ucmq.\n");
    fprintf(stdout, "Note2: Please don't use the command \"pkill -9 ucmq\" and \"kill -9 PID of ucmq\" !\n");
    fprintf(stdout, "-----------------------------------------------------------------------------------\n");
}

static void wait4_child(const int sig)
{
    int save_errno;

    save_errno = errno;
    errno = 0;

    while(wait3(NULL, WNOHANG, NULL) >=0 || errno == EINTR);
    signal(sig, wait4_child);
    errno = save_errno;
}

static void write_process_id(void)
{
    int     p_fd;
    char    pid[16];

    unlink(g_mq_conf.pid_file);
    if((p_fd = open(g_mq_conf.pid_file, O_RDWR | O_CREAT | O_NONBLOCK, 0770)) > 0)
    {
        sprintf(pid, "%d", (int)getpid());
        write(p_fd, pid, sizeof(pid));
        close(p_fd);
        log_info("Creat process id[%s] file[%s] success", pid, g_mq_conf.pid_file);
        fprintf(stderr, "INFO: Start ok, process id[%s]\n", pid);
    }
    else
    {
        log_error("Creat process id file fail");
    }
}

static void set_daemon(void)
{
    int    rc; 
    int    fd; 

    rc = fork();
    if (rc < 0)
    {   
        log_error("Set daemon error!");
        exit(-1);
    }   
    else if (rc > 0)
    {   
        exit(0);    
    }   
    fd = open("/dev/null", O_RDWR);
    /*we want close stdin stdout*/
    if (fd > 0)
    {   
        close(0);
        close(1);
        dup(fd);
        dup(fd);
        close(fd);
    }   
    setsid();
    rc = fork();
    if (rc < 0)
    { 
        log_error("Set daemon error!");
        exit(-1);
    }   
    else if (rc > 0)
    {   
        exit(0);    
    }   
}

static void mq_store_destroy(void)
{
    if (!mq_qm_close_store())
    {
        log_warn("Data base close fail");
    }
}

static void exec_safe_exit(int sig)
{
    const char* exit_str = "exec_safe_exit";
    log_info("Program Prepare Exit ...");
    mq_qm_sync_store();
    write(pipe_fd[1], exit_str, strlen(exit_str));
}

static void set_signal_handle(void)
{
    signal(SIGALRM, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGHUP,  SIG_IGN);
    signal(SIGUSR1, SIG_IGN);

    signal(SIGCHLD, wait4_child);
    signal(SIGINT,  exec_safe_exit);
    signal(SIGTERM, exec_safe_exit);
    signal(SIGQUIT, exec_safe_exit);
}

static int mq_init_store(void)
{
    memset(&g_info, '\0', sizeof(app_info_t));
    memset(&g_last_info, '\0', sizeof(app_info_t));

    /* UCMQ store open */
    if (!mq_qm_open_store())
    {
        log_error("Store Open fail");
        return RUN_STEP_STORE_INIT;
    }

    /* Get sum of all queue unread count */
    g_info.count = mq_qm_get_store_count();
    log_info("All queue's total unread msg count[%"PRIu64"]", g_info.count);
    
    return RUN_STEP_STORE_END;
}

static int main_entrance(void)
{
    int    ret_code = 0;
    int    exit_code = RUN_STEP_BEGIN;
    
    /* Init UCMQ store */
    exit_code = mq_init_store();
    if (exit_code != RUN_STEP_STORE_END)
    {
        log_error("MQ store init error [%d]", exit_code);
        return exit_code;
    }

    /* Init http */
    exit_code = mq_http_init();
    if (exit_code != RUN_STEP_EVENT_END)
    {
        log_error("Evhttp init error [%d]", exit_code);
        mq_store_destroy();
        return exit_code;
    }

    /* Write process id file */
    write_process_id();

    /* Http event dispatching */
    if((ret_code = mq_http_start()))
    {
        mq_store_destroy();
        mq_http_stop();
        return RUN_STEP_END;
    }
    mq_store_destroy();
    mq_http_stop();
    return exit_code;
}

int main(int argc, char *argv[])
{
    int     opt_char;
    int     ret_code;
    bool    daemon_flag = false;

    static const char *short_opts = "c:dhv";
    struct option long_opts[] = {
        {"help",   no_argument, 0, 'h'},
        {"version",no_argument, 0, 'v'},
        {"deamon", no_argument, 0, 'd'},
        {"config", required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    while ((opt_char = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1)
    {
        switch (opt_char)
        {
            case 'c':
                SET_STR_OPT(g_mq_conf.conf_file);
                break;
            case 'd':
                daemon_flag = true;
                break;
            case 'v':
                print_version();
                return 0;
            case 'h':
            default :
                print_usage();
                return 0; 
        }
    }

    /* If arguments contain -c, the config file was already processed */
    if (!read_conf_file())
    {    
        fprintf(stderr, "ERROR: Read config fail, Please use the right conf\n");
        return 1;
    }    

    /* Init log */
    {
        log_config_t mq_log;

        log_init_config(&mq_log);
        strcpy(mq_log.log_path, g_mq_conf.output_log_path);
        strcpy(mq_log.log_file,"ucmq_log");
        if (strcmp(g_mq_conf.output_log_level, "TRACE") == 0)
        {
            mq_log.log_level = 0;
        }
        else if (strcmp(g_mq_conf.output_log_level, "DEBUG") == 0)
        {
            mq_log.log_level = 10;
        }
        else if (strcmp(g_mq_conf.output_log_level, "INFO") == 0)
        {
            mq_log.log_level = 100;
        }
        else if (strcmp(g_mq_conf.output_log_level, "WARN") == 0)
        {
            mq_log.log_level = 1000;
        }
        else if (strcmp(g_mq_conf.output_log_level, "ERROR") == 0)
        {
            mq_log.log_level = 10000;
        }
        else if (strcmp(g_mq_conf.output_log_level, "FATAL") == 0)
        {
            mq_log.log_level = 65535;
        }
        else
        {
            mq_log.log_level = 100;
        }
        log_init(&mq_log);
    }

    if (daemon_flag)
    {
        /* Set daemon */
        set_daemon();
        set_signal_handle();
        log_info("<%s> Startup ... Daemon mode %s", argv[0], "on");
        ret_code = main_entrance();
        log_info("<%s> ShutDown Ok", argv[0]);
    }
    else
    {
        set_signal_handle();
        log_info("<%s> Startup ... Daemon mode %s", argv[0], "off");
        ret_code = main_entrance();
        log_info("<%s> ShutDown Ok", argv[0]);
    }

    return ret_code;
}
