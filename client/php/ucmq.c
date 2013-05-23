/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id: header 297205 2010-03-30 21:09:07Z johannes $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE
#define __USE_GNU
#include <string.h>
#include <strings.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_ucmq.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

/////////////////////////////////////////////////////////////////////////////////

struct mq_op_info
{
	int		   		port;
	const char*		mq_dest;
	const char*		domain;
	const char*		opt;
	const char*		name;
	const char*		charset;
	const char*		extra;
	const void*		data;
	int				data_len;
	int				timeout;
	struct timespec	time_start;

	zval*			return_value;
	int				return_pos;
	int				return_index;
	int				return_result;
	int				return_httpcode;
	int				return_reason;
	int				return_data;
};

struct mq_srv_info
{
	char*			mq_dest;
	int				fd;
	int				last_use;
	int				timeout;
};

/////////////////////////////////////////////////////////////////////////////////

#define SHOW_ERROR(format, ...)                                         \
{                                                                       \
	if (reason[0] == '\0')                                              \
	{                                                                   \
		snprintf(reason, sizeof(reason), format". errno %d - %s",       \
        	##__VA_ARGS__, errno, strerror(errno));                     \
		reason[sizeof(reason) - 1] = '\0';                              \
	}                                                                   \
}

#define INIT_INFO(x)													\
{                                                                       \
	if (argc < x) WRONG_PARAM_COUNT;									\
	if (zend_get_parameters_array_ex(argc, argv) == FAILURE)			\
	{																	\
		WRONG_PARAM_COUNT;												\
	}																	\
	memset(info, '\0', sizeof(struct mq_op_info));						\
	info->return_value = return_value;								   	\
	array_init(info->return_value);									  	\
	info->return_pos      = -1;                                         \
	info->return_result   = -1;                                         \
	info->return_httpcode = -1;                                         \
	info->return_reason   = -1;                                         \
	info->return_data     = -1;                                         \
}

#define GET_ARG_LONG(x, y)												\
{                                                                       \
	convert_to_long_ex(argv[y - 1]);									\
	info->x = Z_LVAL_PP(argv[y - 1]);                                   \
}

#define GET_ARG_STR(x, y)												\
{                                                                       \
	convert_to_string_ex(argv[y - 1]);									\
	info->x = Z_STRVAL_PP(argv[y - 1]);                                 \
}

#define GET_ARG_STR_EX(x, y, z)											\
{                                                                       \
	convert_to_string_ex(argv[z - 1]);									\
	info->x = Z_STRVAL_PP(argv[z - 1]);									\
	info->y = Z_STRLEN_PP(argv[z - 1]);                                 \
}

#define SET_RETURN_LONG(x, y)											\
{                                                                       \
	if(info->return_##x == -1)										 	\
	{																   	\
		add_assoc_long(info->return_value, #x, y);					   	\
	}																	\
	else																\
	{																	\
		add_assoc_long(info->return_value, #x, info->return_##x);	   	\
	}                                                                   \
}

#define SET_RETURN_STR(x, y)											\
{                                                                       \
	if(info->return_##x == -1)										 	\
	{																   	\
		add_assoc_string(info->return_value, #x, y, 1);			   		\
		info->return_##x = 0;											\
	}                                                                   \
}

#define MQ_SET_RETURN													\
{                                                                       \
	SET_RETURN_LONG(pos, -1);											\
	SET_RETURN_LONG(index, -1);											\
	SET_RETURN_LONG(result, -1);										\
	SET_RETURN_LONG(httpcode, -1);										\
	SET_RETURN_STR(reason, (reason[0]=='\0')?"UNKNOWN ERROR":reason);	\
	SET_RETURN_STR(data, "");											\
	return;                                                             \
}

#define MQ_TRY_ONE(x, y, z)												\
{                                                                       \
	info->mq_dest = x;													\
	const char* p = strchr(x, ':');										\
	if (p != NULL)														\
	{																	\
		info->port = atoi(p + 1);										\
		char domain[y];													\
		strncpy(domain, x, p - x);										\
		domain[p - x] = '\0';											\
		info->domain = domain;											\
		if (mq_op(info) == 0)											\
		{																\
			info->return_index = z;										\
			MQ_SET_RETURN;												\
		}																\
	}                                                                   \
}

#define SET_IOV(x,y)                                                    \
{                                                                       \
	iovs[x].iov_base = (y == NULL) ? "" : (void*)y;                     \
	iovs[x].iov_len = strlen((y == NULL) ? "" : y);                     \
	total_bytes += iovs[x].iov_len;                                     \
	x++;                                                                \
}

/////////////////////////////////////////////////////////////////////////////////

struct mq_srv_info	mq_list[10];
static char			reason[1024];
/* True global resources - no need for thread safety here */
static int          le_ucmq;

/////////////////////////////////////////////////////////////////////////////////

/* If you declare any globals in php_ucmq.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(ucmq)
*/

/* {{{ ucmq_functions[]
 *
 * Every user visible function must have an entry in ucmq_functions[].
 */
const zend_function_entry ucmq_functions[] = {
	//PHP_FE(confirm_ucmq_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE(mq_put_msg,	NULL)
	PHP_FE(mq_get_msg,	NULL)
	PHP_FE(mq_set_opt,	NULL)
	PHP_FE(mq_get_opt,	NULL)
	{NULL, NULL, NULL}	/* Must be the last line in ucmq_functions[] */
};
/* }}} */

/* {{{ ucmq_module_entry
 */
zend_module_entry ucmq_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"ucmq",
	ucmq_functions,
	PHP_MINIT(ucmq),
	PHP_MSHUTDOWN(ucmq),
	PHP_RINIT(ucmq),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(ucmq),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(ucmq),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_UCMQ
ZEND_GET_MODULE(ucmq)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("ucmq.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_ucmq_globals, ucmq_globals)
    STD_PHP_INI_ENTRY("ucmq.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_ucmq_globals, ucmq_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_ucmq_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_ucmq_init_globals(zend_ucmq_globals *ucmq_globals)
{
	ucmq_globals->global_value = 0;
	ucmq_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(ucmq)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	int i = 0;
	for(; i < sizeof(mq_list)/sizeof(mq_list[0]); i++)
	{
		mq_list[i].fd = -1;
	}
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(ucmq)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	int i = 0;
	for(; i < sizeof(mq_list)/sizeof(mq_list[0]); i++)
	{
		if (mq_list[i].fd != -1)
		{
			close(mq_list[i].fd);
		}
		if (mq_list[i].mq_dest != NULL)
		{
			free(mq_list[i].mq_dest);
		}
	}
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(ucmq)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(ucmq)
{
	struct timespec tm;
	clock_gettime(CLOCK_MONOTONIC, &tm);

	int i = 0;
	for(; i < sizeof(mq_list)/sizeof(mq_list[0]); i++)
	{
		char buf;
		if ((mq_list[i].fd != -1) && 
				((mq_list[i].last_use + mq_list[i].timeout < tm.tv_sec) 
				 || (recv(mq_list[i].fd, &buf, 1, MSG_PEEK|MSG_DONTWAIT) == 0)))
		{
			close(mq_list[i].fd);
			mq_list[i].fd = -1;
			if (mq_list[i].mq_dest != NULL)
			{
				free(mq_list[i].mq_dest);
				mq_list[i].mq_dest = NULL;
			}
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(ucmq)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "ucmq support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_ucmq_compiled(string arg)
   Return a string to confirm that the module is compiled in */
//PHP_FUNCTION(confirm_ucmq_compiled)
//{
//	char *arg = NULL;
//	int arg_len, len;
//	char *strg;
//
//	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
//		return;
//	}
//
//	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "ucmq", arg);
//	RETURN_STRINGL(strg, len, 0);
//}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/

int mq_op(struct mq_op_info* info);

PHP_FUNCTION(mq_put_msg)
{
	struct mq_op_info info_val;
	struct mq_op_info* info = &info_val;
	const int argc = ZEND_NUM_ARGS();
	zval** argv[argc];

	INIT_INFO(3);
	info->opt = "put";

	switch (argc)
	{
	case 5:
		GET_ARG_STR(charset, 5);
	case 4:
		GET_ARG_LONG(timeout, 4);
	case 3:
		GET_ARG_STR_EX(data, data_len, 3);
		GET_ARG_STR(name, 2);
		convert_to_array_ex(argv[0]);
		break;
	default:
		SHOW_ERROR("invalid argument");
		MQ_SET_RETURN;
	}

	zval** data_value;
	int i = 0;
	for (zend_hash_internal_pointer_reset(Z_ARRVAL_PP(argv[0])); zend_hash_get_current_data(Z_ARRVAL_PP(argv[0]), (void**)&data_value) == SUCCESS; zend_hash_move_forward(Z_ARRVAL_PP(argv[0])))
	{
		convert_to_string_ex(data_value);
		const char* addr = Z_STRVAL_PP(data_value);
		const int len = Z_STRLEN_PP(data_value);

		MQ_TRY_ONE(addr, len, i);

		++i;
	}

	MQ_SET_RETURN;
} 

PHP_FUNCTION(mq_get_msg)
{
	struct mq_op_info info_val;
	struct mq_op_info* info = &info_val;
	const int argc = ZEND_NUM_ARGS();
	zval** argv[argc];

	INIT_INFO(2);
	info->opt = "get";

	switch (argc)
	{
	case 4:
		GET_ARG_STR(charset, 4);
	case 3:
		GET_ARG_LONG(timeout, 3);
	case 2:
		GET_ARG_STR(name, 2);
		convert_to_array_ex(argv[0]);
		break;
	default:
		SHOW_ERROR("invalid argument");
		MQ_SET_RETURN;
	}

	zval** data_value;
	int i = 0;
	for (zend_hash_internal_pointer_reset(Z_ARRVAL_PP(argv[0])); zend_hash_get_current_data(Z_ARRVAL_PP(argv[0]), (void**)&data_value) == SUCCESS; zend_hash_move_forward(Z_ARRVAL_PP(argv[0])))
	{
		convert_to_string_ex(data_value);
		const char* addr = Z_STRVAL_PP(data_value);
		const int len = Z_STRLEN_PP(data_value);

		MQ_TRY_ONE(addr, len, i);

		++i;
	}

	MQ_SET_RETURN;
} 

PHP_FUNCTION(mq_set_opt)
{
	struct mq_op_info info_val;
	struct mq_op_info* info = &info_val;
	const int argc = ZEND_NUM_ARGS();
	zval** argv[argc];

	INIT_INFO(3);

	char ext_buf[128] = {'\0'};
	const char* server = NULL;
	int len = -1;
	switch (argc)
	{
	case 6:
		GET_ARG_STR(charset, 6);
	case 5:
		GET_ARG_LONG(timeout, 5);
	case 4:
		convert_to_long_ex(argv[3]);
		{
			long value = Z_LVAL_PP(argv[3]);
			sprintf(ext_buf, "num=%d", value);
			info->extra = ext_buf;
		}
	case 3:
		GET_ARG_STR(opt, 3);
		GET_ARG_STR(name, 2);
		convert_to_string_ex(argv[0]);
		server = Z_STRVAL_PP(argv[0]);
		len = Z_STRLEN_PP(argv[0]);
		break;
	default:
		SHOW_ERROR("invalid argument");
		MQ_SET_RETURN;
	}

	MQ_TRY_ONE(server, len, 0);

	MQ_SET_RETURN;
} 

PHP_FUNCTION(mq_get_opt)
{
	struct mq_op_info info_val;
	struct mq_op_info* info = &info_val;
	const int argc = ZEND_NUM_ARGS();
	zval** argv[argc];

	INIT_INFO(3);

	char ext_buf[128] = {'\0'};
	const char* server = NULL;
	int len = -1;
	switch (argc)
	{
	case 6:
		GET_ARG_STR(charset, 6);
	case 5:
		GET_ARG_LONG(timeout, 5);
	case 4:
		convert_to_long_ex(argv[3]);
		{
			long value = Z_LVAL_PP(argv[3]);
			sprintf(ext_buf, "pos=%d", value);
			info->extra = ext_buf;
		}
	case 3:
		GET_ARG_STR(opt, 3);
		GET_ARG_STR(name, 2);
		convert_to_string_ex(argv[0]);
		server = Z_STRVAL_PP(argv[0]);
		len = Z_STRLEN_PP(argv[0]);
		break;
	default:
		SHOW_ERROR("invalid argument");
		MQ_SET_RETURN;
	}

	MQ_TRY_ONE(server, len, 0);

	MQ_SET_RETURN;
} 

const char* get_header_info_str(const char* header, const char* type)
{
	const char* p = strcasestr(header, type);
	if (p == NULL) return NULL;
	p += (strlen(type) + 1);
	while (isblank(*p) != 0)
	{
		p++;
	}
	return p;
}

int get_header_info_int(const char* header, const char* type)
{
	const char* p = get_header_info_str(header, type);
	return (p != NULL) ? atoi(p) : -1;
}

const char* get_data(const char* data)
{
	const char* p = strstr(data, "\r\n\r\n");
	return (p != NULL) ? p + 4 : NULL;
}

int wait_fd(int fd, short events, int timeout)
{
	struct pollfd fds;
	fds.fd = fd;
	fds.events = events;
	fds.revents = 0;
	errno = 0;
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

int add_to_list(int fd, const char* dest, int sec_now)
{
	int i = 0;
	int oldest = __INT_MAX__ ;
	int choose_id = -1;
	for (; i < sizeof(mq_list)/sizeof(mq_list[0]); i++)
	{
		if (mq_list[i].fd == -1)
		{
			choose_id = i;
			break;
		}
		if (mq_list[i].last_use < oldest)
		{
			choose_id = i;
			oldest = mq_list[i].last_use;
		}
	}

	struct mq_srv_info* info = &mq_list[choose_id];
	if (info->fd != -1)
	{
		close(info->fd);
	}
	info->fd = fd;
	info->last_use = sec_now;
	info->timeout = 3600;
	if (info->mq_dest != NULL)
	{
		free(info->mq_dest);
	}
	info->mq_dest = (char*)malloc(strlen(dest) + 1);
	strcpy(info->mq_dest, dest);

	return 0;
}

int get_from_list(const char* dest, int sec_now)
{
	int i = 0;
	for (; i < sizeof(mq_list)/sizeof(mq_list[0]); i++)
	{
		if ((mq_list[i].fd != -1) && (mq_list[i].mq_dest != NULL) && (strcmp(dest, mq_list[i].mq_dest) == 0))
		{
			int events = wait_fd(mq_list[i].fd, POLLIN|POLLERR|POLLHUP|POLLNVAL, 0);
			if ((events == 0) || ((events == POLLIN) && (recv(mq_list[i].fd, &events, 1, MSG_PEEK|MSG_DONTWAIT) != 0)))
			{
				mq_list[i].last_use = sec_now;
				return mq_list[i].fd;
			}
			close(mq_list[i].fd);
			mq_list[i].fd = -1;
		}
	}
	return -1;
}

int del_from_list(int fd)
{
	int count = 0;
	int i = 0;
	for (; i < sizeof(mq_list)/sizeof(mq_list[0]); i++)
	{
		if (mq_list[i].fd == fd)
		{
			close(mq_list[i].fd);
			mq_list[i].fd = -1;
			count++;
		}
	}
	return count;
}

int set_timeout_by_fd(int fd, int timeout)
{
	int i = 0;
	for (; i < sizeof(mq_list)/sizeof(mq_list[0]); i++)
	{
		if (mq_list[i].fd == fd)
		{
			mq_list[i].timeout = timeout;
			return 0;
		}
	}
	return -1;
}

int get_rest_time(const struct mq_op_info* info)
{
	struct timespec tm;
	clock_gettime(CLOCK_MONOTONIC, &tm);
	int t = info->timeout - (((tm.tv_sec - info->time_start.tv_sec) * 1000) + ((tm.tv_nsec - info->time_start.tv_nsec) >> 20));
	return (t > 0) ? t : 0;
}

int create_socket(const struct mq_op_info* info)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	struct hostent* site = gethostbyname(info->domain);
	if (site == NULL)
	{
		SHOW_ERROR("Can not get host : %s", info->domain);
		return -1;
	}

	memcpy(&addr.sin_addr, site->h_addr, site->h_length);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(info->port);

	int s = socket(AF_INET,SOCK_STREAM, 0);
	if(s < 0)
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

	errno = 0;
	while (connect(s, (const struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
	{
		switch (errno)
		{
		case EINPROGRESS:
			if (wait_fd(s, POLLOUT, get_rest_time(info)) != 0)
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

int get_socket(const struct mq_op_info* info)
{
	int s = get_from_list(info->mq_dest, info->time_start.tv_sec);
	if (s < 0)
	{
		s = create_socket(info);
		if (s < 0)
		{
			SHOW_ERROR("Create socket error");
			return -1;
		}
		add_to_list(s, info->mq_dest, info->time_start.tv_sec);
	}
	return s;
}

int send_http_req(int s, const struct mq_op_info* info)
{
	char buffer[128];
	struct iovec iovs[16];
	int iovs_count = 0;
	int total_bytes = 0;

	if ((info->data == NULL) || (info->data_len <= 0))
	{
		SET_IOV(iovs_count, "GET /?name=");
		SET_IOV(iovs_count, info->name);
		SET_IOV(iovs_count, "&opt=");
		SET_IOV(iovs_count, info->opt);
		if (info->charset != NULL)
		{
			SET_IOV(iovs_count, "&charset=");
			SET_IOV(iovs_count, info->charset);
		}
		if (info->extra != NULL)
		{
			SET_IOV(iovs_count, "&");
			SET_IOV(iovs_count, info->extra);
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
		if (info->charset != NULL)
		{
			SET_IOV(iovs_count, "&charset=");
			SET_IOV(iovs_count, info->charset);
		}
		if (info->extra != NULL)
		{
			SET_IOV(iovs_count, "&");
			SET_IOV(iovs_count, info->extra);
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
#undef SET_IOV

	int send_size = -1;
	do
	{
		send_size = writev(s, iovs, iovs_count);
	} while ((send_size < 0) && ((errno == EINTR)||(errno == EAGAIN)));
	if (send_size != total_bytes)
	{
		SHOW_ERROR("send request error, need to send %d bytes, but only send %d bytes", total_bytes, send_size);
		return -1;
	}

	return 0;
}

int recv_n(int fd, char* data, size_t count, const struct mq_op_info* info, int (*is_finish)(char* data, size_t count))
{
	size_t recv_size = 0;
	errno = 0;
	do
	{
		int r = recv(fd, data + recv_size, count - recv_size, MSG_NOSIGNAL);
		switch (r)
		{
		case 0:
			return recv_size;
		case -1:
			switch (errno)
			{
			case EAGAIN:
				if (wait_fd(fd, POLLIN, get_rest_time(info)) == 0)
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

int check_header_finish(char* data, size_t count)
{
	data[count] = '\0';
	return (get_data(data) == NULL) ? -1 : 0;
}

int mq_op(struct mq_op_info* info)
{
	if (info->timeout == 0)
	{
		info->timeout = 1000;
	}

	clock_gettime(CLOCK_MONOTONIC, &info->time_start);

	reason[0] = '\0';

	int is_ok = -1;
	int s = -1;
	do
	{
		s = get_socket(info);
		if (s < 0) break;

		if (send_http_req(s, info) < 0) break;

		char buffer[1024];
		int total_recv = recv_n(s, buffer, sizeof(buffer) - 1, info, check_header_finish);
		buffer[(total_recv < 0) ? 0 : total_recv] = '\0';
		const char* data = get_data(buffer);
		if (data == NULL)
		{
			SHOW_ERROR("recv http header error");
			break;
		}

		char http_reason[strcspn(buffer, "\r\n")];
		sscanf(buffer, "HTTP/1.1 %d %[^\r\n]\r\n", &info->return_httpcode, http_reason);

		info->return_pos = get_header_info_int(buffer, "Pos");

		int body_len = get_header_info_int(buffer, "Content-Length");
		if (body_len < 0)
		{
			SHOW_ERROR("data length is invalid : %d", body_len);
			break;
		}

		int timeout = get_header_info_int(buffer, "keep-alive");
		if (timeout > 0)
		{
			set_timeout_by_fd(s, timeout);
		}

		char body[body_len + 1];
		int has_recv = total_recv - (data - buffer);
		if (has_recv > body_len)
		{
			SHOW_ERROR("invalid body len");
			break;
		}
		if (has_recv > 0)
		{
			memcpy(body, data, has_recv);
		}

		if (body_len > has_recv)
		{
			if (recv_n(s, body + has_recv, body_len - has_recv, info, NULL) != body_len - has_recv)
			{
				SHOW_ERROR("recv body error");
				break;
			}
		}
		body[body_len] = '\0';

		SET_RETURN_STR(data, body);
		SET_RETURN_STR(reason, http_reason);
		info->return_result = (info->return_httpcode == 200) ? 0 : -1;

		is_ok = 0;
	} while (0);

	if (is_ok == -1)
	{
		del_from_list(s);
	}

	return is_ok;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
