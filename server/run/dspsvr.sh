#/bin/bash
ps -ef | grep -w "login_server"    | grep $USER | grep -v grep
ps -ef | grep -w "msg_server"    | grep $USER | grep -v grep
ps -ef | grep -w "route_server"    | grep $USER | grep -v grep
ps -ef | grep -w "http_msg_server"    | grep $USER | grep -v grep
ps -ef | grep -w "file_server"    | grep $USER | grep -v grep
ps -ef | grep -w "push_server"    | grep $USER | grep -v grep
ps -ef | grep -w "db_proxy_server"    | grep $USER | grep -v grep
ps -ef | grep -w "msfs"    | grep $USER | grep -v grep
