#!/bin/sh

# begin
#-------------------------------------------

echo "do you really want to stop all services? (y/n): "
read ans
if [ "$ans" = "y" ];
then

ps -ef | grep -w "login_server" | awk '{print $2}' | xargs kill -9 > /dev/null 2>&1
ps -ef | grep -w "msg_server" | awk '{print $2}' | xargs kill -9 > /dev/null 2>&1
ps -ef | grep -w "route_server" | awk '{print $2}' | xargs kill -9 > /dev/null 2>&1
ps -ef | grep -w "http_msg_server" | awk '{print $2}' | xargs kill -9 > /dev/null 2>&1
ps -ef | grep -w "file_server" | awk '{print $2}' | xargs kill -9 > /dev/null 2>&1
ps -ef | grep -w "push_server" | awk '{print $2}' | xargs kill -9 > /dev/null 2>&1
ps -ef | grep -w "db_proxy_server" | awk '{print $2}' | xargs kill -9 > /dev/null 2>&1
ps -ef | grep -w "msfs" | awk '{print $2}' | xargs kill -9 > /dev/null 2>&1


fi


#-------------------------------------------
# end

