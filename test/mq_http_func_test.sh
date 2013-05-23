#  Copyright (c) 2013 UCWeb Inc.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
# You may obtain a copy of the License at
# 
#       http://www.gnu.org/licenses/gpl-2.0.html
# 
# Email: osucmq@ucweb.com
# Author: ShaneYuan

#!/bin/bash

###########################################################################
#  copyright: ucweb 
#  author: ShaneYuan   Email: yuancy@ucweb.com
#  describe: 本脚本是对ucmq进行功能性测试脚本
#  usager: ./$0 -n <queue name> [-i <mq ipaddr>] [-p <port>] [-h <help>]
###########################################################################

mq_ip=192.168.3.171 # 默认的ip
mq_port=1919        # 默认的port
queue_name=005      # 默认的队列名
ver=2

echo " ----------- ucmq feature test script-------- "
echo " -- mq_ip = ${mq_ip}"
echo " -- mq_port = ${mq_port}"
echo " -- queue_name = ${queue_name}"
echo " --------------`date`-----------------"

###########################################################################
# delay, test case
###########################################################################
# data clean
echo -e \\n"\033[44;37;1m [TESTF: REMOVE] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: remove the queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=remove&ver=$ver"

# set delay 10
echo -e \\n"\033[44;37;1m [TESTF: DELAY] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: set delay 10]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=delay&num=10&ver=$ver"

# gen data 
echo -e \\n"\033[44;37;1m [TESTF: PUT] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: put \"first\" to queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=first&ver=$ver"

# try error
echo -e \\n"\033[44;37;1m [TESTF: GET] [EXPECT: UCMQ_HTTP_ERR_QUE_EMPTY] [DESCRIBE: get msg]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=get&ver=$ver"

# try ok
sleep 10
echo -e \\n"\033[44;37;1m [TESTF: GET] [EXPECT: UCMQ_HTTP_OK ...][DESCRIBE: get msg]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=get&ver=$ver"

# gen data
echo -e \\n"\033[44;37;1m [TESTF: PUT] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: put \"002\" to queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=002&ver=$ver"

# set no delay
echo -e \\n"\033[44;37;1m [TESTF: DELAY] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: set delay 0]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=delay&num=0&ver=$ver"

# try ok
echo -e \\n"\033[44;37;1m [TESTF: GET] [EXPECT: UCMQ_HTTP_OK ...] [DESCRIBE: get msg]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=get&ver=$ver"

###########################################################################
# wlock, test case
###########################################################################
# data clean
echo -e \\n"\033[44;37;1m [TESTF: REMOVE] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: remove the queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=remove&ver=$ver"

# gen data
echo -e \\n"\033[44;37;1m [TESTF: PUT] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: put \"first\" to queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=first&ver=$ver"

# set wlock 5 second
echo -e \\n"\033[44;37;1m [TESTF: WLOCK] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: set delay 5]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=wlock&num=5&ver=$ver"

# try error
echo -e \\n"\033[44;37;1m [TESTF: PUT] [EXPECT: UCMQ_HTTP_ERR_WLOCK] [DESCRIBE: put \"002\" to queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=002&ver=$ver"

# try ok
sleep 5
echo -e \\n"\033[44;37;1m [TESTF: PUT] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: put \"002\" to queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=002&ver=$ver"

# set wlock forever
echo -e \\n"\033[44;37;1m [TESTF: WLOCK] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: set delay nil]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=wlock&ver=$ver"

# try error
sleep 5
echo -e \\n"\033[44;37;1m [TESTF: PUT] [EXPECT: UCMQ_HTTP_ERR_WLOCK] [DESCRIBE: put \"002\" to queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=002&ver=$ver"

# try get
echo -e \\n"\033[44;37;1m [TESTF: GET] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: get msg]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=get&ver=$ver"

# set wlock unlock 
echo -e \\n"\033[44;37;1m [TESTF: WLOCK] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: set wlock unlock]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=wlock&num=0&ver=$ver"

# try ok, write
echo -e \\n"\033[44;37;1m [TESTF: PUT] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: put \"002\" to queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=002&ver=$ver"

# try ok, read
echo -e \\n"\033[44;37;1m [TESTF: GET] [EXPECT: UCMQ_HTTP_OK ...] [DESCRIBE: get msg]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=get&ver=$ver"

###########################################################################
# queue not exist test case, find queue
###########################################################################
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=remove&ver=$ver"
echo -e \\n"\033[44;37;1m [TESTF: GET] [EXPECT: UCMQ_HTTP_ERR_QUE_NO_EXIST] [DESCRIBE: get msg from not exist queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=get&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: STATUS] [EXPECT: UCMQ_HTTP_ERR_QUE_NO_EXIST] [DESCRIBE: check status not exist queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=status&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: STATUS_JSON] [EXPECT: UCMQ_HTTP_ERR_QUE_NO_EXIST] [DESCRIBE: check status_json not exist queue] \033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=status_json&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: WLOCK] [EXPECT: UCMQ_HTTP_ERR_QUE_NO_EXIST] [DESCRIBE: set not exist queue write lock] \033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=wlock&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: REMOVE] [EXPECT: UCMQ_HTTP_ERR_QUE_NO_EXIST] [DESCRIBE: del not exist queue] \033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=remove&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: RESET] [EXPECT: UCMQ_HTTP_ERR_QUE_NO_EXIST] [DESCRIBE: reset not exist queue] \033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=reset&ver=$ver"

###########################################################################
# queue not exist test case, Create queue
###########################################################################
echo -e \\n"\033[44;37;1m [TESTF: PUT] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: put msg into not exist queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=$queue_name&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: MAXQUEUE] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: reset maxqueue number 30000000] \033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name"1"&opt=maxqueue&num=30000000&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: DELAY] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: set queue delay] \033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name"2"&opt=delay&num=1&ver=$ver"

###########################################################################
# set opt test case,
###########################################################################
echo -e \\n"\033[44;37;1m [TESTF: STATUS] [EXPECT: ....] [DESCRIBE: check status with an empty queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=status&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: SYNCTIME] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: set sync time 10]\033[0m "
curl "http://$mq_ip:$mq_port/?opt=synctime&num=10&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: PUT1] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: put \"first\" to queue] \033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=first&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: PUT2] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: put \"002\" to queue] \033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=002&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: PUT3] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: post data \"test---\" to queue]\033[0m "
curl -d "test----" "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: STATUS] [EXPECT: ...] [DESCRIBE: check status of the queue] \033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=status&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: GET] [EXPECT: UCMQ_HTTP_OK ...] [DESCRIBE: get msg from queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=get&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: STATUS] [EXPECT: ...] [DESCRIBE: now check status again ...]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=status&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: VIEW 0] [EXPECT: UCMQ_HTTP_ERR_UNKNOWN_OPT] [DESCRIBE: unknown opt]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=view&pos=0&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: RESET] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: reset the queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=reset&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: STATUS] [EXPECT: ...] [DESCRIBE: check status of the queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=status&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: REMOVE] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: remove the queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=remove&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: PUT] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: put \"first\" to queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=first&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: STATUS] [EXPECT: ...] [DESCRIBE: check status of the queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=status&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: REMOVE] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: remove the queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=remove&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: GET] [EXPECT: UCMQ_HTTP_ERR_QUE_NO_EXIST] [DESCRIBE: try to get message from queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=get&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: PUT] [EXPECT: UCMQ_HTTP_OK] [DESCRIBE: put "end" and "eeee" to queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=put&data=end&ver=$ver"

echo -e \\n"\033[44;37;1m [TESTF: STATUS] [EXPECT: ...] [DESCRIBE: check status of the queue]\033[0m "
curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=status&ver=$ver"

###########################################################################
# HELPTELL test case,
###########################################################################
# reload conf
echo -e \\n"\033[44;37;1m [TESTF: RELOAD] [EXPECT: ...] [DESCRIBE: conf reload]\033[0m "
curl "http://$mq_ip:$mq_port/exec?cmd=reload"

# get file stat
echo -e \\n"\033[44;37;1m [TESTF: GET STAT] [EXPECT: ...] [DESCRIBE: get stat]\033[0m "
curl "http://$mq_ip:$mq_port/exec?cmd=get&file=stat"

echo -e \\n"\033[44;37;1m [TESTF: STAT] [EXPECT: ...] [DESCRIBE: stat]\033[0m "
curl "http://$mq_ip:$mq_port/stat?type"

echo -e \\n"\033[44;37;1m [TESTF: STAT MEM] [EXPECT: ...] [DESCRIBE: stat mem]\033[0m "
curl "http://$mq_ip:$mq_port/stat?type=mem"

echo -e \\n"\033[44;37;1m [TESTF: STAT CPU] [EXPECT: ...] [DESCRIBE: stat cpu]\033[0m "
curl "http://$mq_ip:$mq_port/stat?type=cpu"

echo -e \\n"\033[44;37;1m [TESTF: STAT INFO] [EXPECT: ...] [DESCRIBE: stat info]\033[0m "
curl "http://$mq_ip:$mq_port/stat?type=info"

###########################################################################
# restart test case,
###########################################################################
# shut down
echo -e \\n"\033[44;37;1m [TESTF: KILL] [DESCRIBE: kill the ucmq program]\033[0m "
curl "http://$mq_ip:$mq_port/exec?cmd=kill"
sleep 5;

runner=$HOME/local/ucmq/bin/ucmq
echo $runner
if [ -f ${runner} ];then
    echo -e \\n"\033[44;37;1m [TESTF: RESTART] [DESCRIBE: restart ucmq program]\033[0m "
    `${runner} -c ../conf/ucmq.ini -d`
    prom=`ps -ef |grep ucmq |grep -v grep`
    run=`ps -ef |grep ucmq |grep -v grep|wc -l`
    echo "SHOW RUNNER :$prom"
    if [ ${run} -gt 0 ];then

        echo -e \\n"\033[44;37;1m [TESTF: STATUS] [DESCRIBE: check status of the queue]\033[0m "
        curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=status"

        echo -e \\n"\033[44;37;1m [TESTF: GET] [DESCRIBE: get message from queue]\033[0m "
        curl "http://$mq_ip:$mq_port/?name=$queue_name&opt=get"
    fi
else
    echo -e \\n"--------RESTART UCMQ fail :`date`------"
fi

echo -e \\n"--------END TEST :`date`------"
