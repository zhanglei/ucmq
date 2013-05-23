ACLOCAL_AMFLAGS = -I m4 -fpermissive

AUTOMAKE_OPTIONS = foreign

if SERVER

bin_PROGRAMS= ucmq

ucmq_SOURCES= ini.c \
			  file.c \
			  crc16.c \
			  log.c \
			  mq_errno.c \
			  util.c \
			  mq_util.c \
			  mq_config.c \
			  mq_store_manage.c \
			  mq_store_file.c \
			  mq_store_rtag.c \
			  mq_store_msg.c \
			  mq_queue_manage.c \
			  mq_evhttp.c \
			  trace.c \
			  main.c 

ucmq_CFLAGS = -I$(top_srcdir)
ucmq_CXXFLAGS = -I$(top_srcdir)
ucmq_LDADD = $(TC_LIBS) $(EVENT_LIBS) 

endif

EXTRA_DIST = main.h\
			 mq_queue_manage.h\
			 mq_store_manage.h\
			 mq_store_msg.h\
			 mq_store_rtag.h\
			 mq_store_file.h\
			 mq_evhttp.h\
			 mq_config.h\
			 mq_util.h \
			 atomic.h\
			 crc16.h\
			 file.h \
			 ini.h\
			 log.h\
			 common.h\
			 mq_errno.h\
			 internal.h\
			 util.h\
			 uthash.h
