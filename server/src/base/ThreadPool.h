/*
 * ThreadPool.h
 *
 *  Created on: 2014年7月21日
 *      Author: ziteng
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_
#include "ostype.h"
#include "Thread.h"
#include "Task.h"
#include <pthread.h>
#include <list>
using namespace std;

class CWorkerThread {//每个工作线程都有自己的任务队列，在一个死循环里面，判断如果任务队列为空，则休眠该线程，如果未空，则开始处理任务
public:
	CWorkerThread();
	~CWorkerThread();

	static void* StartRoutine(void* arg);

	void Start();
	void Execute();
	void PushTask(CTask* pTask);

	void SetThreadIdx(uint32_t idx) { m_thread_idx = idx; }
private:

	uint32_t		m_thread_idx;
	uint32_t		m_task_cnt;
	pthread_t		m_thread_id;
	CThreadNotify	m_thread_notify;//?在子线程里面调用wait,然后由主线程去放入任务后，调用signal唤醒
	list<CTask*>	m_task_list;
};

class CThreadPool {
public:
	CThreadPool();
	virtual ~CThreadPool();

	int Init(uint32_t worker_size);
	void AddTask(CTask* pTask);
	void Destory();
private:
	uint32_t 		m_worker_size;
	CWorkerThread* 	m_worker_list;
};



#endif /* THREADPOOL_H_ */
