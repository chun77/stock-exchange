#!/bin/bash

# 要并发运行的客户端数量
CLIENT_COUNT=10

# 客户端程序的路径
CLIENT="./mockclient"

# 使用循环并发启动多个客户端实例
for ((i=0; i<$CLIENT_COUNT; i++))
do
   $CLIENT &
done

# 等待所有后台进程结束
wait
