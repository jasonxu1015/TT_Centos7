/*
 * login_server.cpp
 *
 *  Created on: 2013-6-21
 *      Author: ziteng@mogujie.com
 */

#include "LoginConn.h"
#include "netlib.h"
#include "ConfigFileReader.h"
#include "version.h"
#include "HttpConn.h"
#include "ipparser.h"

IpParser* pIpParser = NULL;
string strMsfsUrl;
string strDiscovery;//发现获取地址
void client_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam)
{
	log("enter[%s]", __FUNCTION__);

	if (msg == NETLIB_MSG_CONNECT)
	{
		CLoginConn* pConn = new CLoginConn();
		pConn->OnConnect2(handle, LOGIN_CONN_TYPE_CLIENT);
	}
	else
	{
		log("!!!error msg: %d ", msg);
	}
	log("leave[%s]", __FUNCTION__);

}

// this callback will be replaced by imconn_callback() in OnConnect()
void msg_serv_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam)
{
	log("enter[%s]", __FUNCTION__);

    log("msg_server come in");

	if (msg == NETLIB_MSG_CONNECT)
	{
		//这个回调函数的工作就是new 一个CLoginConn,并将它和新连接的socket连接起来。并叫该socket对应的CBaseSocket里面的回调函数、回调参数
		// 换成imconn_callback 和对应的map
		CLoginConn* pConn = new CLoginConn();
		pConn->OnConnect2(handle, LOGIN_CONN_TYPE_MSG_SERV);
	}
	else
	{
		log("!!!error msg: %d ", msg);
	}
	log("leave[%s]", __FUNCTION__);

}


void http_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam)
{
	log("enter[%s]", __FUNCTION__);

	//当有客户端连接到login_server的8080端口时，该函数被回调，会new一个CHttpConn来进行业务处理
    if (msg == NETLIB_MSG_CONNECT)
    {
        CHttpConn* pConn = new CHttpConn();
        pConn->OnConnect(handle);
    }
    else
    {
        log("!!!error msg: %d ", msg);
    }
	log("leave[%s]", __FUNCTION__);

}

int main(int argc, char* argv[])
{
	log("enter[%s]", __FUNCTION__);

    log("My Test");
	if ((argc == 2) && (strcmp(argv[1], "-v") == 0)) {
		printf("Server Version: LoginServer/%s\n", VERSION);
		printf("Server Build: %s %s\n", __DATE__, __TIME__);
		return 0;
	}

	signal(SIGPIPE, SIG_IGN);

	CConfigFileReader config_file("loginserver.conf");

    char* client_listen_ip = config_file.GetConfigName("ClientListenIP");
    char* str_client_port = config_file.GetConfigName("ClientPort");
    char* http_listen_ip = config_file.GetConfigName("HttpListenIP");
    char* str_http_port = config_file.GetConfigName("HttpPort");
	char* msg_server_listen_ip = config_file.GetConfigName("MsgServerListenIP");
	char* str_msg_server_port = config_file.GetConfigName("MsgServerPort");
    char* str_msfs_url = config_file.GetConfigName("msfs");
    char* str_discovery = config_file.GetConfigName("discovery");

	if (!msg_server_listen_ip || !str_msg_server_port || !http_listen_ip
        || !str_http_port || !str_msfs_url || !str_discovery) {
		log("config item missing, exit... ");
		return -1;
	}

	uint16_t client_port = atoi(str_client_port);
	uint16_t msg_server_port = atoi(str_msg_server_port);
    uint16_t http_port = atoi(str_http_port);
    strMsfsUrl = str_msfs_url;
    strDiscovery = str_discovery;
    
    
    pIpParser = new IpParser();
    
	int ret = netlib_init();

	if (ret == NETLIB_ERROR)
		return ret;
	CStrExplode client_listen_ip_list(client_listen_ip, ';');
	for (uint32_t i = 0; i < client_listen_ip_list.GetItemCnt(); i++) {
		ret = netlib_listen(client_listen_ip_list.GetItem(i), client_port, client_callback, NULL);
		if (ret == NETLIB_ERROR)
			return ret;
	}

	CStrExplode msg_server_listen_ip_list(msg_server_listen_ip, ';');
	for (uint32_t i = 0; i < msg_server_listen_ip_list.GetItemCnt(); i++) {
		ret = netlib_listen(msg_server_listen_ip_list.GetItem(i), msg_server_port, msg_serv_callback, NULL);
		if (ret == NETLIB_ERROR)
			return ret;
	}
    
    CStrExplode http_listen_ip_list(http_listen_ip, ';');
    for (uint32_t i = 0; i < http_listen_ip_list.GetItemCnt(); i++) {
        ret = netlib_listen(http_listen_ip_list.GetItem(i), http_port, http_callback, NULL);
        if (ret == NETLIB_ERROR)
            return ret;
    }
    

			printf("server start listen on:\nFor client %s:%d\nFor MsgServer: %s:%d\nFor http:%s:%d\n",
			client_listen_ip, client_port, msg_server_listen_ip, msg_server_port, http_listen_ip, http_port);
	init_login_conn();
    init_http_conn();

	printf("now enter the event loop...\n");
    
    writePid();

	netlib_eventloop();

	log("leave[%s]", __FUNCTION__);

	return 0;
}
