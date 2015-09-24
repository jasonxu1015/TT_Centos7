/*
 * LoginConn.cpp
 *
 *  Created on: 2013-6-21
 *      Author: ziteng@mogujie.com
 */

#include "LoginConn.h"
#include "IM.Server.pb.h"
#include "IM.Other.pb.h"
#include "IM.Login.pb.h"
#include "public_define.h"
using namespace IM::BaseDefine;
static ConnMap_t g_client_conn_map;
static ConnMap_t g_msg_serv_conn_map;//这两个map关联的是socket和CImConn ,使用的是hash表
static uint32_t g_total_online_user_cnt = 0;	// 并发在线总人数
map<uint32_t, msg_serv_info_t*> g_msg_serv_info;//保存了消息服务器的信息和相应的socket

void login_conn_timer_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam)
{
	log("enter[%s]", __FUNCTION__);

	uint64_t cur_time = get_tick_count();
	for (ConnMap_t::iterator it = g_client_conn_map.begin(); it != g_client_conn_map.end(); ) {
		ConnMap_t::iterator it_old = it;
		it++;

		CLoginConn* pConn = (CLoginConn*)it_old->second;
		pConn->OnTimer(cur_time);
	}

	for (ConnMap_t::iterator it = g_msg_serv_conn_map.begin(); it != g_msg_serv_conn_map.end(); ) {
		ConnMap_t::iterator it_old = it;
		it++;

		CLoginConn* pConn = (CLoginConn*)it_old->second;
		pConn->OnTimer(cur_time);
	}
	log("leave[%s]", __FUNCTION__);

}

void init_login_conn()
{
	log("enter[%s]", __FUNCTION__);
	//定时器的触发在 StartDispatch(uint32_t wait_timeout),里面，由epoll_wait控制最小的触发间隔，由_CheckTimer
	//去判断是否有定时器需要触发回调函数
	netlib_register_timer(login_conn_timer_callback, NULL, 1000);
	log("leave[%s]", __FUNCTION__);
}

CLoginConn::CLoginConn()
{
	log("enter[%s]", __FUNCTION__);

	log("leave[%s]", __FUNCTION__);

}

CLoginConn::~CLoginConn()
{

	log("enter[%s]", __FUNCTION__);
	log("leave[%s]", __FUNCTION__);


}

void CLoginConn::Close()
{
	log("enter[%s]", __FUNCTION__);

	if (m_handle != NETLIB_INVALID_HANDLE) {
		netlib_close(m_handle);
		if (m_conn_type == LOGIN_CONN_TYPE_CLIENT) {
			g_client_conn_map.erase(m_handle);
		} else {
			g_msg_serv_conn_map.erase(m_handle);

			// remove all user count from this message server
			map<uint32_t, msg_serv_info_t*>::iterator it = g_msg_serv_info.find(m_handle);
			if (it != g_msg_serv_info.end()) {
				msg_serv_info_t* pMsgServInfo = it->second;

				g_total_online_user_cnt -= pMsgServInfo->cur_conn_cnt;
				log("onclose from MsgServer: %s:%u ", pMsgServInfo->hostname.c_str(), pMsgServInfo->port);
				delete pMsgServInfo;
				g_msg_serv_info.erase(it);
			}
		}
	}

	ReleaseRef();
	log("leave[%s]", __FUNCTION__);

}

void CLoginConn::OnConnect2(net_handle_t handle, int conn_type)
{
	log("enter[%s]", __FUNCTION__);

	m_handle = handle;
	m_conn_type = conn_type;
	ConnMap_t* conn_map = &g_msg_serv_conn_map;
	if (conn_type == LOGIN_CONN_TYPE_CLIENT) {
		conn_map = &g_client_conn_map;
	}else

	//使用一个map，把socket和对应的CIMConn映射起来
	conn_map->insert(make_pair(handle, this));

	//将新连接进来的msgserver的socketbase，重新赋值回调函数和回调参数,
	netlib_option(handle, NETLIB_OPT_SET_CALLBACK, (void*)imconn_callback);//这个回调函数是会调用到基类CImConn的虚函数
	netlib_option(handle, NETLIB_OPT_SET_CALLBACK_DATA, (void*)conn_map);
	log("leave[%s]", __FUNCTION__);

}

void CLoginConn::OnClose()
{
	log("enter[%s]", __FUNCTION__);

	Close();
	log("leave[%s]", __FUNCTION__);

}

void CLoginConn::OnTimer(uint64_t curr_tick)
{
	log("enter[%s]", __FUNCTION__);

	if (m_conn_type == LOGIN_CONN_TYPE_CLIENT) {
		if (curr_tick > m_last_recv_tick + CLIENT_TIMEOUT) {
			Close();
		}
	} else {
		if (curr_tick > m_last_send_tick + SERVER_HEARTBEAT_INTERVAL) {//每隔5s就给msg_server发送一个心跳包
            IM::Other::IMHeartBeat msg;
            CImPdu pdu;
            pdu.SetPBMsg(&msg);
            pdu.SetServiceId(SID_OTHER);
            pdu.SetCommandId(CID_OTHER_HEARTBEAT);
			log("Send HeartBeat To MsgServer");
			SendPdu(&pdu);
		}

		if (curr_tick > m_last_recv_tick + SERVER_TIMEOUT) {
			log("connection to MsgServer timeout ");
			Close();
		}
	}
	log("leave[%s]", __FUNCTION__);

}

void CLoginConn::HandlePdu(CImPdu* pPdu)
{
	log("enter[%s]", __FUNCTION__);

	switch (pPdu->GetCommandId()) {
        case CID_OTHER_HEARTBEAT:
            break;
        case CID_OTHER_MSG_SERV_INFO:
            _HandleMsgServInfo(pPdu);
            break;
        case CID_OTHER_USER_CNT_UPDATE:
            _HandleUserCntUpdate(pPdu);
            break;
        case CID_LOGIN_REQ_MSGSERVER:
            _HandleMsgServRequest(pPdu);
            break;

        default:
            log("wrong msg, cmd id=%d ", pPdu->GetCommandId());
            break;
	}
	log("leave[%s]", __FUNCTION__);

}

void CLoginConn::_HandleMsgServInfo(CImPdu* pPdu)
{
	log("enter[%s]", __FUNCTION__);

	msg_serv_info_t* pMsgServInfo = new msg_serv_info_t;
    IM::Server::IMMsgServInfo msg;
    msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength());
    
	pMsgServInfo->ip_addr1 = msg.ip1();
	pMsgServInfo->ip_addr2 = msg.ip2();
	pMsgServInfo->port = msg.port();
	pMsgServInfo->max_conn_cnt = msg.max_conn_cnt();
	pMsgServInfo->cur_conn_cnt = msg.cur_conn_cnt();
	pMsgServInfo->hostname = msg.host_name();
	g_msg_serv_info.insert(make_pair(m_handle, pMsgServInfo));

	g_total_online_user_cnt += pMsgServInfo->cur_conn_cnt;

	log("MsgServInfo, ip_addr1=%s, ip_addr2=%s, port=%d, max_conn_cnt=%d, cur_conn_cnt=%d, "\
		"hostname: %s. ",
		pMsgServInfo->ip_addr1.c_str(), pMsgServInfo->ip_addr2.c_str(), pMsgServInfo->port,pMsgServInfo->max_conn_cnt,
		pMsgServInfo->cur_conn_cnt, pMsgServInfo->hostname.c_str());
	log("leave[%s]", __FUNCTION__);

}

void CLoginConn::_HandleUserCntUpdate(CImPdu* pPdu)
{
	log("enter[%s]", __FUNCTION__);

	map<uint32_t, msg_serv_info_t*>::iterator it = g_msg_serv_info.find(m_handle);
	if (it != g_msg_serv_info.end()) {
		msg_serv_info_t* pMsgServInfo = it->second;
        IM::Server::IMUserCntUpdate msg;
        msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength());

		uint32_t action = msg.user_action();
		if (action == USER_CNT_INC) {
			pMsgServInfo->cur_conn_cnt++;
			g_total_online_user_cnt++;
		} else {
			pMsgServInfo->cur_conn_cnt--;
			g_total_online_user_cnt--;
		}

		log("%s:%d, cur_cnt=%u, total_cnt=%u ", pMsgServInfo->hostname.c_str(),
            pMsgServInfo->port, pMsgServInfo->cur_conn_cnt, g_total_online_user_cnt);
	}
	log("leave[%s]", __FUNCTION__);

}

void CLoginConn::_HandleMsgServRequest(CImPdu* pPdu)
{
	log("enter[%s]", __FUNCTION__);

    IM::Login::IMMsgServReq msg;
    msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength());

	log("HandleMsgServReq. ");

	// no MessageServer available
	if (g_msg_serv_info.size() == 0) {
        IM::Login::IMMsgServRsp msg;
        msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_NO_MSG_SERVER);
        CImPdu pdu;
        pdu.SetPBMsg(&msg);
        pdu.SetServiceId(SID_LOGIN);
        pdu.SetCommandId(CID_LOGIN_RES_MSGSERVER);
        pdu.SetSeqNum(pPdu->GetSeqNum());
        SendPdu(&pdu);
        Close();
		return;
	}

	// return a message server with minimum concurrent connection count
	msg_serv_info_t* pMsgServInfo;
	uint32_t min_user_cnt = (uint32_t)-1;
	map<uint32_t, msg_serv_info_t*>::iterator it_min_conn = g_msg_serv_info.end(),it;

	for (it = g_msg_serv_info.begin() ; it != g_msg_serv_info.end(); it++) {
		pMsgServInfo = it->second;
		if ( (pMsgServInfo->cur_conn_cnt < pMsgServInfo->max_conn_cnt) &&
			 (pMsgServInfo->cur_conn_cnt < min_user_cnt))
        {
			it_min_conn = it;
			min_user_cnt = pMsgServInfo->cur_conn_cnt;
		}
	}

	if (it_min_conn == g_msg_serv_info.end()) {
		log("All TCP MsgServer are full ");
        IM::Login::IMMsgServRsp msg;
        msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_MSG_SERVER_FULL);
        CImPdu pdu;
        pdu.SetPBMsg(&msg);
        pdu.SetServiceId(SID_LOGIN);
        pdu.SetCommandId(CID_LOGIN_RES_MSGSERVER);
        pdu.SetSeqNum(pPdu->GetSeqNum());
        SendPdu(&pdu);
	}
    else
    {
        IM::Login::IMMsgServRsp msg;
        msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_NONE);
        msg.set_prior_ip(it_min_conn->second->ip_addr1);
        msg.set_backip_ip(it_min_conn->second->ip_addr2);
        msg.set_port(it_min_conn->second->port);
        CImPdu pdu;
        pdu.SetPBMsg(&msg);
        pdu.SetServiceId(SID_LOGIN);
        pdu.SetCommandId(CID_LOGIN_RES_MSGSERVER);
        pdu.SetSeqNum(pPdu->GetSeqNum());
        SendPdu(&pdu);
    }

	Close();	// after send MsgServResponse, active close the connection
	log("leave[%s]", __FUNCTION__);

}
