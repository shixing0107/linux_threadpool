﻿/////////////////////////////////////////////////////////////////////////////////
/// Copyright (C), 2022, 
/// Richinfo Corporation. All rights reserved.
/// @file    threadpool.h
/// @author  Simone
/// @date    2022/12/08
/// @brief   
///
/// @history v0.01 2022/12/08  单元创建
/////////////////////////////////////////////////////////////////////////////////

#include "basejob.h"
#include "threadpoolcallback.h"
#include "threadpooldefine.h"

#include <set>
#include <map>
#include <mutex> 
#include <semaphore.h>
#include <atomic>
#include <queue>
#include <list>
#include <thread>    // std::thread, std::this_thread::sleep_for

#ifndef _TP_THREAD_POOL_H__
#define _TP_THREAD_POOL_H__

typedef enum ThreadItemOperation
{
    ETh_Op_Exit_Th_Pool = 0,
    ETh_Op_Get_Job,
    ETh_Op_Sub_Th,
    ETh_Op_Undefine
}EThreadItemOp;

///
/// \brief The TPThreadPool class
///
class TPThreadPool
{
	friend class TPBaseJob;  //允许Job在 GetJobWaitType 中获取 m_hEventStop/m_hEventContinue

public:

	TPThreadPool(TPThreadPoolCallBack* pCallBack = NULL);
	virtual ~TPThreadPool(void);

	// 开始线程池,此时会创建 min_number_threads 个线程，
	// 然后会根据任务数在 min_number_threads -- max_number_threads
	// 之间自行调节线程的个数
	bool start(int min_count_threads, int max_count_threads);

	// 请求停止线程池
	// 注意：
	//   1.只是设置StopEvent，需要Job根据GetJobWaitType处理 
	//   2.不会清除当前注册的但尚未进行的工作，如果需要删除，需要调用ClearUndoWork
	void stop();

	bool stopAndWait(uint64_t dwTimeOut = WAIT_INFINITE);

	// 等待所有线程都结束并释放Start中分配的线程资源
	bool wait(uint64_t dwTimeOut = WAIT_INFINITE);

	// 清除当前未完成的工作，
    void clearUndoWork();

	// 向线程池中注册工作 -- 如果当前没有空闲的线程，并且当前线程数小于最大线程数，则会自动创建新的线程，
	// 成功后会通过 outJobIndex 返回Job的索引号，可通过该索引定位、取消特定的Job
	bool submitJob(TPBaseJob* pJob, int* pOutJobId);

	// 是否已经请求了停止线程池
	int hadRequestStop() const;

public:

	// 屏蔽赋值构造和拷贝构造
	DISABLE_COPY_AND_ASSIGNMENT(TPThreadPool);

public:
    void doJobs();

public:
	std::atomic<int> job_index_;                    // Job的索引，每 SubmitJob 一次，则递增1
    std::atomic<int> running_job_count_;			// 当前正在运行的Job个数

	std::atomic<int> running_threads_count_;		// 当前运行着的线程个数(用来在所有的线程结束时激发 Complete 事件)
	smart_event_t all_thread_complete_event_;       // 所有的线程都结束时激发这个事件

protected:
	// 获取工作类型
	EThreadItemOp _getJob(TPBaseJob** ppJob);

	// 增加运行的线程,如果 当前线程数 
	void _destroyPool();

	void _notifyJobBegin(TPBaseJob* pJob);
	void _notifyJobEnd(TPBaseJob* pJob);
	void _notifyJobCancel(TPBaseJob* pJob);
	void _notifyJobError(TPBaseJob* pJob, int errCode, const char* errDesc); 
	// 增加运行的线程,如果 当前线程数 + thread_num <= max_thread_num_ 时会成功执行
	int _addJobThread(int count);
    void _clearRunningMapThread();

private:
    TPThreadPoolCallBack* m_pCallBack;              // 回调接口

    smart_event_t stop_event_;                      // 停止Pool的事件
	int min_count_threads_;
	int max_count_threads_;
	int last_running_thread_;					// 并行运行线程数量    


	// 保存等待Job的信息，由于有优先级的问题，而且一般是从最前面开始取，因此保存成 set，
	// 保证优先级高、JobIndex小(同优先级时FIFO) 的Job在最前面
	typedef UnreferenceLess<TPBaseJob *> JobBaseUnreferenceLess;
	typedef std::set<TPBaseJob*, JobBaseUnreferenceLess > WaitingJobContainer;
	WaitingJobContainer	set_waiting_jobs_;									// 等待运行的Job
	std::mutex waiting_jobs_lock_;

	// 保存运行Job的信息， 由于会频繁加入、删除，且需要按照JobIndex查找，因此保存成 map
	typedef std::map<int, TPBaseJob*> MapDoingJobContainer;
	MapDoingJobContainer map_doing_hobs_;					// 正在运行的Job
    std::mutex mutex_doing_jobs_;

public:
	// 用标准的C++11 thread
    std::map<pid_t, std::thread::id> map_thread_ids_;
    std::list<std::thread*> list_running_threads_;
    std::mutex running_threads_lock_;
    std::list<std::thread*> list_removing_threads_;
    std::mutex removing_threads_lock_;

public:
    smart_event_t job_needed_to_do_event_;			// there are waiting jobs
    smart_event_t subtract_thread_event_;			// need subtract thread

    sem_t sem_subtract_thread_;
    std::mutex subtract_thread_lock_;
    smart_event_t notify_subtract_thread_event_;
    std::queue<int> queue_substract_thread_;
    sem_t sem_notify_complete_substract_thread_;

    bool exit_waitting_job_thread_;
    sem_t sem_job_needed_to_do_;            // job需要等待执行信号量
    sem_t sem_notify_got_waitting_job_;     // 通知得到等待的job

private:
    std::thread *substract_thread_mgr_; // 减少线程管理线程
    std::thread *waitting_job_thread_mgr_;


};

#endif
