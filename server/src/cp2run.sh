#!/bin/bash
# author: yang
# date: 09/09/2015

cp slog/log4cxx.properties ../run/login_server/
cp slog/libslog.so  ../run/login_server/
cp slog/lib/liblog4cxx.so.* ../run/login_server/
cp login_server/loginserver.conf ../run/login_server/

cp slog/log4cxx.properties ../run/route_server/
cp slog/libslog.so  ../run/route_server/
cp slog/lib/liblog4cxx.so.* ../run/route_server/
cp route_server/routeserver.conf ../run/route_server/

cp slog/log4cxx.properties ../run/msg_server/
cp slog/libslog.so  ../run/msg_server/
cp slog/lib/liblog4cxx.so.* ../run/msg_server/
cp msg_server/msgserver.conf ../run/msg_server/


cp slog/log4cxx.properties ../run/http_msg_server/
cp slog/libslog.so  ../run/http_msg_server/
cp slog/lib/liblog4cxx.so.* ../run/http_msg_server/
cp http_msg_server/httpmsgserver.conf ../run/http_msg_server/


cp slog/log4cxx.properties ../run/file_server/
cp slog/libslog.so  ../run/file_server/
cp slog/lib/liblog4cxx.so.* ../run/file_server/
cp file_server/fileserver.conf ../run/file_server/

cp slog/log4cxx.properties ../run/push_server/
cp slog/libslog.so  ../run/push_server/
cp slog/lib/liblog4cxx.so.* ../run/push_server/
cp push_server/pushserver.conf ../run/push_server/

cp slog/log4cxx.properties ../run/db_proxy_server/
cp slog/libslog.so  ../run/db_proxy_server/
cp slog/lib/liblog4cxx.so.* ../run/db_proxy_server/
cp db_proxy_server/dbproxyserver.conf ../run/db_proxy_server/

cp slog/log4cxx.properties ../run/msfs/
cp slog/libslog.so  ../run/msfs/
cp slog/lib/liblog4cxx.so.* ../run/msfs/

