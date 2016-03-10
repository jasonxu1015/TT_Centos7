/*================================================================
*   Copyright (C) 2014 All rights reserved.
*   
*   文件名称：Thread.h
*   创 建 者：Zhang Yuanhao
*   邮    箱：bluefoxah@gmail.com
*   创建日期：2014年09月10日
*   描    述：
*
#pragma once
================================================================*/

#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>

class CThread
{
public:
	CThread();
	virtual ~CThread();
    
#ifdef _WIN32
	static DWORD WINAPI StartRoutine(LPVOID lpParameter);
#else
	static void* StartRoutine(void* arg);
#endif
    
	virtual void StartThread(void);
	virtual void OnThreadRun(void) = 0;
protected:
#ifdef _WIN32
	DWORD		m_thread_id;
#else
	pthread_t	m_thread_id;
#endif
};

class CEventThread : public CThread
{
public:
	CEventThread();
	virtual ~CEventThread();
    
 	virtual void OnThreadTick(void) = 0;
	virtual void OnThreadRun(void);
	virtual void StartThread();
	virtual void StopThread();
	bool IsRunning() { return m_bRunning; }
private:
	bool 		m_bRunning;
};

//  pthread_cond_wait() 用于阻塞当前线程，等待别的线程使用pthread_cond_signal()或pthread_cond_broadcast来唤醒它。
//  pthread_cond_wait() 必须与pthread_mutex 配套使用。pthread_cond_wait()函数一进入wait状态就会自动release mutex。
//  当其他线程通过pthread_cond_signal()或pthread_cond_broadcast，把该线程唤醒，使pthread_cond_wait()通过（返回）时，
//  该线程又自动获得该mutex。
class CThreadNotify
{
public:
	CThreadNotify();
	~CThreadNotify();
	void Lock() { pthread_mutex_lock(&m_mutex); }
	void Unlock() { pthread_mutex_unlock(&m_mutex); }
	void Wait() { pthread_cond_wait(&m_cond, &m_mutex); }
	void Signal() { pthread_cond_signal(&m_cond); }
private:
	pthread_mutex_t 	m_mutex;
	pthread_mutexattr_t	m_mutexattr;
    
	pthread_cond_t 		m_cond;
};

#endif
