ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign

SUBDIRS = source client

EXTRA_DIST = test/mq_http_func_test.sh\
			 test/mq_php_func_test.php\
			 test/mq_test_by_ab.sh\
			 conf/ucmq.ini\
			 genall.sh\
			 LICENSE

confdir=$(prefix)/conf
conf_DATA=conf/ucmq.ini

testdir=$(prefix)/test
test_DATA=test/*.sh test/*.php

