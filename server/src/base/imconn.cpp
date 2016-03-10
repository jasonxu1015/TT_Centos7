/*
 * imconn.cpp
 *
 *  Created on: 2013-6-5
 *      Author: ziteng@mogujie.com
 */

#include "imconn.h"

//static uint64_t g_send_pkt_cnt = 0;		// 发送数据包总数
//static uint64_t g_recv_pkt_cnt = 0;		// 接收数据包总数

static CImConn* FindImConn(ConnMap_t* imconn_map, net_handle_t handle)
{
	CImConn* pConn = NULL;
	ConnMap_t::iterator iter = imconn_map->find(handle);
	if (iter != imconn_map->end())
	{
		pConn = iter->second;
		pConn->AddRef();
	}

	return pConn;
}

//当msgserver有数据到达时，会回调到imconn_callback(),
void imconn_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam)//这个属于业务相关的，只处理连接成功、读写、断开。而没有监听相关
	//由void CBaseSocket::OnRead()调用
{
	log("enter[%s]", __FUNCTION__);

	NOTUSED_ARG(handle);
	NOTUSED_ARG(pParam);

	if (!callback_data)
		return;

	//根据hash表和socket，去找到对应的CImConn，CImConn里面有具体的业务处理
	ConnMap_t* conn_map = (ConnMap_t*)callback_data;
	CImConn* pConn = FindImConn(conn_map, handle);
	if (!pConn)
		return;

	log("msg=%d, handle=%d ", msg, handle);

	switch (msg)
	{
	case NETLIB_MSG_CONFIRM:
		pConn->OnConfirm();
		break;
	case NETLIB_MSG_READ:
		pConn->OnRead();
		break;
	case NETLIB_MSG_WRITE:
		pConn->OnWrite();
		break;
	case NETLIB_MSG_CLOSE:
		pConn->OnClose();
		break;
	default:
		log("!!!imconn_callback error msg: %d ", msg);
		break;
	}

	pConn->ReleaseRef();
	log("leave[%s]", __FUNCTION__);
}

//////////////////////////
CImConn::CImConn()
{
	//log("CImConn::CImConn ");

	m_busy = false;
	m_handle = NETLIB_INVALID_HANDLE;
	m_recv_bytes = 0;

	m_last_send_tick = m_last_recv_tick = get_tick_count();
}

CImConn::~CImConn()
{
	//log("CImConn::~CImConn, handle=%d ", m_handle);
}

int CImConn::Send(void* data, int len)
{
	m_last_send_tick = get_tick_count();
//	++g_send_pkt_cnt;

	if (m_busy)
	{
		m_out_buf.Write(data, len);
		return len;
	}

	int offset = 0;
	int remain = len;
	while (remain > 0) {
		int send_size = remain;
		if (send_size > NETLIB_MAX_SOCKET_BUF_SIZE) {
			send_size = NETLIB_MAX_SOCKET_BUF_SIZE;
		}

		int ret = netlib_send(m_handle, (char*)data + offset , send_size);
		if (ret <= 0) {
			ret = 0;
			break;
		}

		offset += ret;
		remain -= ret;
	}

	if (remain > 0)//当这次循环写入socket已经写不进去了，对方又不读数据导致socket缓存一直是满的，
	// 就只好把数据给放进我们自己的缓存中了，并设置m_busy为ture,表示还没发送完数据
	{
		m_out_buf.Write((char*)data + offset, remain);
		m_busy = true;
		log("send busy, remain=%d ", m_out_buf.GetWriteOffset());
	}
    else
    {
        OnWriteCompelete();
    }

	return len;
}

//负责读取数据，然后调用HandlePdu,HandlePdu是虚函数，由子类定义具体业务处理
void CImConn::OnRead()
{
	for (;;)//循环读取数据，直到socket 读完
	{
		uint32_t free_buf_len = m_in_buf.GetAllocSize() - m_in_buf.GetWriteOffset();
		if (free_buf_len < READ_BUF_SIZE)
			m_in_buf.Extend(READ_BUF_SIZE);

		//这里类里只对应了一个连接，所以不用加锁
		int ret = netlib_recv(m_handle, m_in_buf.GetBuffer() + m_in_buf.GetWriteOffset(), READ_BUF_SIZE);
		if (ret <= 0)
			break;

		m_recv_bytes += ret;
		m_in_buf.IncWriteOffset(ret);

		//更新收到最后一个报文的时间，心跳判断里面需要用到
		m_last_recv_tick = get_tick_count();
	}

    CImPdu* pPdu = NULL;
	try
    {
		while ( ( pPdu = CImPdu::ReadPdu(m_in_buf.GetBuffer(), m_in_buf.GetWriteOffset()) ) )
		{
            uint32_t pdu_len = pPdu->GetLength();
            
			HandlePdu(pPdu);

			//解析报文成功后，则会从缓冲区中将该报文删除掉
			m_in_buf.Read(NULL, pdu_len);
			delete pPdu;
            pPdu = NULL;
//			++g_recv_pkt_cnt;
		}
	} catch (CPduException& ex) {
		log("!!!catch exception, sid=%u, cid=%u, err_code=%u, err_msg=%s, close the connection ",
				ex.GetServiceId(), ex.GetCommandId(), ex.GetErrorCode(), ex.GetErrorMsg());
        if (pPdu) {
            delete pPdu;
            pPdu = NULL;
        }
        OnClose();
	}
}

void CImConn::OnWrite()
{
	if (!m_busy)//没有需要发送的数据，则跳过，这时候epoll不会再次通知你这个socket可写。但没有关系，直接写就是了
		return;

	while (m_out_buf.GetWriteOffset() > 0) {
		int send_size = m_out_buf.GetWriteOffset();
		if (send_size > NETLIB_MAX_SOCKET_BUF_SIZE) {
			send_size = NETLIB_MAX_SOCKET_BUF_SIZE;
		}

		//循环发送，直到返回值小于等于0跳出，等待下一次EPOLLOUT事件响应，或者是把发送缓冲区写完了，
		// 退出然后设置m_busy为false
		
		//简单地说：EPOLLOUT事件只有socket从unwritable变为writable时
		// ，才会触发一次。对于EPOLLOUT事件，必须要将该文件描述符缓冲
		// 区一直写满，让 errno 返回 EAGAIN 为止，或者发送完所有数据为止。
		

		int ret = netlib_send(m_handle, m_out_buf.GetBuffer(), send_size);
		if (ret <= 0) {
			ret = 0;
			break;
		}

		m_out_buf.Read(NULL, ret);//删除已发送的数据
	}

	if (m_out_buf.GetWriteOffset() == 0) {
		m_busy = false;
	}

	log("onWrite, remain=%d ", m_out_buf.GetWriteOffset());
}




