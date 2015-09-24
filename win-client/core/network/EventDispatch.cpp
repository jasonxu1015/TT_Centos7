#include "EventDispatch.h"
#include "BaseSocket.h"

#define MIN_TIMER_DURATION	100	// miliseconds

CEventDispatch::CEventDispatch()
:running(false)
{
#ifdef _MSC_VER
	FD_ZERO(&m_read_set);
	FD_ZERO(&m_write_set);
	FD_ZERO(&m_excep_set);
#elif __APPLE__
	m_kqfd = kqueue();
	if (m_kqfd == -1)
	{
		LOG__(NET,  "kqueue failed");
	}
#else
	m_epfd = epoll_create(1024);
	if (m_epfd == -1)
	{
		LOG__(NET,  "epoll_create failed");
	}
#endif
}

CEventDispatch::~CEventDispatch()
{
#ifdef _MSC_VER

#elif __APPLE__
	close(m_kqfd);
#else
	close(m_epfd);
#endif
}

void CEventDispatch::AddTimer(callback_t callback, void* user_data, uint64_t interval)
{
	list<TimerItem*>::iterator it;
	for (it = m_timer_list.begin(); it != m_timer_list.end(); it++)
	{
		TimerItem* pItem = *it;
		if (pItem->callback == callback && pItem->user_data == user_data)
		{
			pItem->interval = interval;
			pItem->next_tick = get_tick_count() + interval;
			return;
		}
	}

	TimerItem* pItem = new TimerItem;
	pItem->callback = callback;
	pItem->user_data = user_data;
	pItem->interval = interval;
	pItem->next_tick = get_tick_count() + interval;
	m_timer_list.push_back(pItem);
}

void CEventDispatch::RemoveTimer(callback_t callback, void* user_data)
{
	list<TimerItem*>::iterator it;
	for (it = m_timer_list.begin(); it != m_timer_list.end(); it++)
	{
		TimerItem* pItem = *it;
		if (pItem->callback == callback && pItem->user_data == user_data)
		{
			m_timer_list.erase(it);
			delete pItem;
			return;
		}
	}
}

void CEventDispatch::_CheckTimer()
{
	uint64_t curr_tick = get_tick_count();
	list<TimerItem*>::iterator it;

	for (it = m_timer_list.begin(); it != m_timer_list.end(); )
	{
		TimerItem* pItem = *it;
		it++;		// iterator maybe deleted in the callback, so we should increment it before callback
		if (curr_tick >= pItem->next_tick)
		{
			pItem->next_tick += pItem->interval;
			pItem->callback(pItem->user_data, NETLIB_MSG_TIMER, 0, NULL);
		}
	}
}

void CEventDispatch::AddLoop(callback_t callback, void* user_data)
{
	TimerItem* pItem = new TimerItem;
	pItem->callback = callback;
	pItem->user_data = user_data;
	m_loop_list.push_back(pItem);
}

void CEventDispatch::_CheckLoop()
{
	for (list<TimerItem*>::iterator it = m_loop_list.begin(); it != m_loop_list.end(); it++) {
		TimerItem* pItem = *it;
		pItem->callback(pItem->user_data, NETLIB_MSG_LOOP, 0, NULL);
	}
}

CEventDispatch* CEventDispatch::Instance()
{
	static CEventDispatch eventDispatch;
	return &eventDispatch;
}

#ifdef _MSC_VER

void CEventDispatch::AddEvent(SOCKET fd, uint8_t socket_event)
{
	CAutoLock func_lock(&m_lock);

	if ((socket_event & SOCKET_READ) != 0)
	{
		FD_SET(fd, &m_read_set);
	}
		
	if ((socket_event & SOCKET_WRITE) != 0)
	{
		FD_SET(fd, &m_write_set);
	}

	if ((socket_event & SOCKET_EXCEP) != 0)
	{
		FD_SET(fd, &m_excep_set);
	}
}

void CEventDispatch::RemoveEvent(SOCKET fd, uint8_t socket_event)
{
	CAutoLock func_lock(&m_lock);

	if ((socket_event & SOCKET_READ) != 0)
	{
		FD_CLR(fd, &m_read_set);
	}

	if ((socket_event & SOCKET_WRITE) != 0)
	{
		FD_CLR(fd, &m_write_set);
	}

	if ((socket_event & SOCKET_EXCEP) != 0)
	{
		FD_CLR(fd, &m_excep_set);
	}
}

void CEventDispatch::StartDispatch(uint32_t wait_timeout)
{
	fd_set read_set, write_set, excep_set;
	timeval timeout;
	timeout.tv_sec = 1;	//wait_timeout 1 second
	timeout.tv_usec = 0;

    while (running)
	{
		//_CheckTimer();
		//_CheckLoop();

		if (!m_read_set.fd_count && !m_write_set.fd_count && !m_excep_set.fd_count)
		{
			Sleep(MIN_TIMER_DURATION);
			continue;
		}

		m_lock.lock();
		FD_ZERO(&read_set);
		FD_ZERO(&write_set);
		FD_ZERO(&excep_set);
		memcpy(&read_set, &m_read_set, sizeof(fd_set));
		memcpy(&write_set, &m_write_set, sizeof(fd_set));
		memcpy(&excep_set, &m_excep_set, sizeof(fd_set));
		m_lock.unlock();

		if (!running)
			break;

		//for (int i = 0; i < read_set.fd_count; i++) {
		//	LOG__(NET,  "read fd: %d\n", read_set.fd_array[i]);
		//}
		int nfds = select(0, &read_set, &write_set, &excep_set, &timeout);
		if (nfds == SOCKET_ERROR)
		{
			//LOG__(NET,  "select failed, error code: %d\n", GetLastError());
			Sleep(MIN_TIMER_DURATION);
			continue;			// select again
		}
		if (nfds == 0)
		{
			continue;
		}
		for (u_int i = 0; i < read_set.fd_count; i++)
		{
			//LOG__(NET,  "select return read count=%d\n", read_set.fd_count);
			SOCKET fd = read_set.fd_array[i];
			CBaseSocket* pSocket = FindBaseSocket((net_handle_t)fd);
			if (pSocket)
			{
				pSocket->OnRead();
				pSocket->ReleaseRef();
			}
		}
		for (u_int i = 0; i < write_set.fd_count; i++)
		{
			//LOG__(NET,  "select return write count=%d\n", write_set.fd_count);
			SOCKET fd = write_set.fd_array[i];
			CBaseSocket* pSocket = FindBaseSocket((net_handle_t)fd);
			if (pSocket)
			{
				pSocket->OnWrite();
				pSocket->ReleaseRef();
			}
		}
		for (u_int i = 0; i < excep_set.fd_count; i++)
		{
			LOG__(NET,  _T("select return exception count=%d"), excep_set.fd_count);
			SOCKET fd = excep_set.fd_array[i];
			CBaseSocket* pSocket = FindBaseSocket((net_handle_t)fd);
			if (pSocket)
			{
				pSocket->OnClose();
				pSocket->ReleaseRef();
			}
		}
	}
}

void CEventDispatch::StopDispatch()
{
    running = false;
}

#elif __APPLE__

void CEventDispatch::AddEvent(SOCKET fd, uint8_t socket_event)
{
	struct kevent ke;

	if ((socket_event & SOCKET_READ) != 0)
	{
		EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
		kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
	}

	if ((socket_event & SOCKET_WRITE) != 0)
	{
		EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
		kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
	}
}

void CEventDispatch::RemoveEvent(SOCKET fd, uint8_t socket_event)
{
	struct kevent ke;

	if ((socket_event & SOCKET_READ) != 0)
	{
		EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
	}

	if ((socket_event & SOCKET_WRITE) != 0)
	{
		EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
	}
}

void CEventDispatch::StartDispatch(uint32_t wait_timeout)
{
	struct kevent events[1024];
	int nfds = 0;
	struct timespec timeout;
	timeout.tv_sec = 0;
	timeout.tv_nsec = wait_timeout * 1000000;
    
    while (running)
	{
		nfds = kevent(m_kqfd, NULL, 0, events, 1024, &timeout);

		for (int i = 0; i < nfds; i++)
		{
			int ev_fd = events[i].ident;
			CBaseSocket* pSocket = FindBaseSocket(ev_fd);
			if (!pSocket)
				continue;

			if (events[i].filter == EVFILT_READ)
			{
				//LOG__(NET,  "OnRead, socket=%d\n", ev_fd);
				pSocket->OnRead();
			}

			if (events[i].filter == EVFILT_WRITE)
			{
				//LOG__(NET,  "OnWrite, socket=%d\n", ev_fd);
				pSocket->OnWrite();
			}

			pSocket->ReleaseRef();
		}

		_CheckTimer();
		_CheckLoop();
	}
}

void CEventDispatch::StopDispatch()
{
    running = false;
}

#else

void CEventDispatch::AddEvent(SOCKET fd, uint8_t socket_event)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLPRI | EPOLLERR | EPOLLHUP;
	ev.data.fd = fd;
	if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev) != 0)
	{
		LOG__(NET,  "epoll_ctl() failed, errno=%d\n", errno);
	}
}

void CEventDispatch::RemoveEvent(SOCKET fd, uint8_t socket_event)
{
	if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL) != 0)
	{
		LOG__(NET,  "epoll_ctl failed, errno=%d\n", errno);
	}
}

void CEventDispatch::StartDispatch(uint32_t wait_timeout)
{
	struct epoll_event events[1024];
	int nfds = 0;
    
	while (running)
	{
		nfds = epoll_wait(m_epfd, events, 1024, wait_timeout);
		for (int i = 0; i < nfds; i++)
		{
			int ev_fd = events[i].data.fd;
			CBaseSocket* pSocket = FindBaseSocket(ev_fd);
			if (!pSocket)
				continue;

			if (events[i].events & EPOLLIN)
			{
				//LOG__(NET,  "OnRead, socket=%d\n", ev_fd);
				pSocket->OnRead();
			}

			if (events[i].events & EPOLLOUT)
			{
				//LOG__(NET,  "OnWrite, socket=%d\n", ev_fd);
				pSocket->OnWrite();
			}

			if (events[i].events & (EPOLLPRI | EPOLLERR | EPOLLHUP))
			{
				//LOG__(NET,  "OnClose, socket=%d\n", ev_fd);
				pSocket->OnClose();
			}

			pSocket->ReleaseRef();
		}

		_CheckTimer();
		_CheckLoop();
	}
}

void CEventDispatch::StopDispatch()
{
    running = false;
}


#endif
