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
# Author: shaneyuan

#!/bin/bash

#####################
#  copyright: ucweb 
#  author: ShaneYuan   Email: yuancy@ucweb.com
#  describe: 本脚本是调用apache提供的ab测试工具对mq进行测试
#  usager: ./$0 -o <operate> -n <queue name> [-m <max queue size>] [-c <conn>] [-i <mq ipaddr>] [-p <port>] [-h <help>]
########################
ip=192.168.0.4
#ip=127.0.0.1
port=1818
opt=put
name=001
max_queue=30000000

conn=10          # 每次启动ab的并发数
data_len=1024    # 每条记录的长度
total_loops=10   # 启动ab的次数
each_times=100000 # 每条ab并发线程的循环次数
put_output=./put.txt # 写入测试过程中的每秒事务数
get_output=./get.txt 

# 启动ab测试工具
function single_test()
{
    if [ "${opt}" = "put" ];then  # 判断是入队列还是出队列
        cmd='ab -k -c '$conn' -n '$2' http://'$ip:$port'/?name='$name'&opt='$1'&data='$data
    else
        cmd='ab -k -c '$conn' -n '$2' http://'$ip:$port'/?name='$name'&opt='$1
    fi

    # 获取ab启动一次测试的每秒事务数
    result=$($cmd | grep "Requests per second")   
    value=$(expr match "$result" '[^0-9.]\+\([0-9.]\+\)')
    echo $value
}

function usage(){
echo "usage:./$0 -o <operate> -n <queue name> [-c <conn>] [-m < max queue size>] [-i <mq ipaddr>] [-p <port>] [-h <help>]";
}

# 构造入队列消息的函数
function gen_data()
{
    for((i=0; i<$data_len; i++))
    do
        data=1${data}
    done
}

# 脚本主体启动函数
function entrance()
{
    for((i=0; i<$total_loops; i++))
    do
        value=$(single_test "${opt}" $each_times)

        # 判断没执行多少次ab测试工具，才写入到文件中
        if [ 0 -eq $((${i}%1))  ];then
            if [ "${opt}" == "put" ];then
                echo $value >> $put_output;
            else 
                echo $value >> $get_output;
                #curl http://$ip:$port/?name=$name'&opt=status'
            fi
        fi
        sleep 1;
    done
}

main(){
    echo -e \\n "-------- initialization -------"
    echo -e \\n "Boot parameters:opt = $opt,queue name = $name,max queue = $max_queue,ab conn= $conn,ip = $ip,port = $port"
    echo -e \\n "---------- run start ----------"
    curl "http://${ip}:${port}/?name=${name}&opt=maxqueue&num=${max_queue}"
    gen_data
    entrance
}

while getopts "o:n:m:c:i:p:h" arg 
do  
    case $arg in
        o)  
        opt=$OPTARG
        ;;  
        n)    
        name=$OPTARG    
        ;;    
        m)    
        max_queue=$OPTARG    
        ;;  
        c)    
        conn=$OPTARG    
        ;;    
        i)  
        ip=$OPTARG
        ;;  
        p)  
        port=$OPTARG
        ;;  
        h)
        usage
        exit 1
        ;;  
        ?) 
        echo "unkonw argument"
        usage
        exit 1
        ;;  
    esac
done
main
