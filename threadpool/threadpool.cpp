/////////////////////////////////////////////////////////////////////////////////
/// Copyright (C), 2022, 
/// Richinfo Corporation. All rights reserved.
/// @file    threadpool.cpp
/// @author  Simone
/// @date    2022/12/09
/// @brief   
///
/// @history v0.01 2022/12/09  单元创建
/////////////////////////////////////////////////////////////////////////////////

#include "threadpool.h"
#include <thread>				// for std::thread
#include "rslogging.h"
#include <pthread.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include "pevents.h"
#include <unistd.h>


/////////////////////////////////////////////////////////////////////////////////////////////////
/// waitting job fn
int fnWaittingJobThreadProc(void* arg)
{
    RSLOG_DEBUG << "entry ...";
    TPThreadPool* pThPool = (TPThreadPool*)arg;
    if (pThPool == NULL)
    {
        RSLOG_ERROR << "arg TPThreadPool is NULL";
        return -1;
    }

    while (true)
    {
        RSLOG_DEBUG << "wait waitting job sem!";
        sem_wait(&pThPool->sem_job_needed_to_do_);
        RSLOG_DEBUG << "had waited waitting job sem!";
        if (pThPool->exit_waitting_job_thread_)
        {
            RSLOG_DEBUG << "exit waitting job thread!";
            break;
        }

        RSLOG_DEBUG << "set job needed to do event!";
        SetEvent(pThPool->job_needed_to_do_event_);

        RSLOG_DEBUG << "wait notify got waitting job sem!";
        sem_wait(&pThPool->sem_notify_got_waitting_job_);
        RSLOG_DEBUG << "had waited notify got waitting job sem!";
    }

    RSLOG_DEBUG << "leave";
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
int fnSubstractThreadProc(void* arg)
{
    RSLOG_DEBUG << "entry ...";
    TPThreadPool* pThPool = (TPThreadPool*)arg;
    if (pThPool == NULL)
    {
        RSLOG_ERROR << "arg TPThreadPool is NULL";
        return -1;
    }

    while (true)
    {
        RSLOG_DEBUG << "wait substract sem!";
        sem_wait(&pThPool->sem_subtract_thread_);
        RSLOG_DEBUG << "had waited substract sem!";
        std::lock_guard<std::mutex> subtract_locker(pThPool->subtract_thread_lock_);
        int thCount = pThPool->queue_substract_thread_.front();
        RSLOG_DEBUG << "substract count = " << thCount;
        pThPool->queue_substract_thread_.pop();
        if (thCount == 0)
        {
            RSLOG_DEBUG << "exit substract thread, thread count is" << thCount;

            // 清理线程测试
            std::list<std::thread*>::iterator it = pThPool->list_removing_threads_.begin();
            int i = 0;
            while(it != pThPool->list_removing_threads_.end())
            {
                std::thread* removeTh = *it;
                std::thread::id tid = removeTh->get_id();
                RSLOG_DEBUG << "remove thread, tid = " << *(unsigned int*)&tid;
                removeTh->join();
                delete removeTh;
                it = pThPool->list_removing_threads_.erase(it);
                i++;
                RSLOG_DEBUG << "remove count onnce is " << i;
            }
            RSLOG_DEBUG << "exit substract thread, thread count is" << pThPool->running_threads_count_.load();
            goto final_end;
        }
        else
        {
            RSLOG_DEBUG << "set event substract_thread_event";
            SetEvent(pThPool->subtract_thread_event_);
            RSLOG_DEBUG << "wait notify compelete substract thread sem";
            sem_wait(&pThPool->sem_notify_complete_substract_thread_);
            RSLOG_DEBUG << "had wait notify compelete substract thread sem";

            // 清理线程测试
            std::list<std::thread*>::iterator it = pThPool->list_removing_threads_.begin();
            int i = 0;
            while(it != pThPool->list_removing_threads_.end())
            {
                std::thread* removeTh = *it;
                std::thread::id tid = removeTh->get_id();
                RSLOG_DEBUG << "remove thread, tid = " << *(unsigned int*)&tid;
                removeTh->join();
                delete removeTh;
                it = pThPool->list_removing_threads_.erase(it);
                i++;
                RSLOG_DEBUG << "remove count onnce is " << i;
            }
        }
    }

final_end:
    RSLOG_DEBUG << "leave";
    return 0;
}

/////////////////////////////////////////////////////////
/// @brief 
/// @param arg 
/// @return 
void* fnJobThreadProc(void* arg)
{
    RSLOG_DEBUG << "entry ...";
    TPThreadPool* pThPool = (TPThreadPool*)arg;
	if (pThPool == NULL)
    {
        RSLOG_ERROR << "arg TPThreadPool is NULL";
        return (void*)-1;
    }
    // pid_t pid = pthread_self();
    // RSLOG_DEBUG << "pid = " << *(unsigned int*)&pid;
    pid_t tid = syscall(SYS_gettid);
    RSLOG_DEBUG << "tid = " << *(unsigned int*)&tid;
    std::thread::id pid = std::this_thread::get_id();
    RSLOG_DEBUG << "thread id = " << *(unsigned int*)&pid;
    pThPool->map_thread_ids_[tid] = pid;
    pThPool->running_threads_count_.fetch_add(1, std::memory_order_seq_cst);
    RSLOG_DEBUG << "running_threads_count_ = " << pThPool->running_threads_count_.load();
    pThPool->doJobs();
    pThPool->running_threads_count_.fetch_sub(1, std::memory_order_seq_cst);
    RSLOG_DEBUG << "running_threads_count_ = " << pThPool->running_threads_count_.load();
	if (pThPool->running_threads_count_.load() == 0)
	{
		SetEvent(pThPool->all_thread_complete_event_);
	}

    RSLOG_DEBUG << "leave";
    return (void*)0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
TPThreadPool::TPThreadPool(TPThreadPoolCallBack* pCallBack)
    : m_pCallBack(pCallBack)
    , min_count_threads_(0)
    , max_count_threads_(1)
    , last_running_thread_(0)
    , running_threads_count_(0)
    , job_index_(0)
    , running_job_count_(0)
    , substract_thread_mgr_(NULL)
    , exit_waitting_job_thread_(false)
    , waitting_job_thread_mgr_(NULL)
{
    stop_event_ = CreateEvent(true, false);
	TPASSERT(NULL != stop_event_);

	all_thread_complete_event_ = CreateEvent(true, false);
	TPASSERT(NULL != all_thread_complete_event_);

    job_needed_to_do_event_ = CreateEvent(false, false);
	TPASSERT(NULL != job_needed_to_do_event_);

	subtract_thread_event_ = CreateEvent(false, false);
	TPASSERT(NULL != subtract_thread_event_);

	sem_init(&sem_job_needed_to_do_, false, 0);
    sem_init(&sem_subtract_thread_, false, 0);
    sem_init(&sem_notify_complete_substract_thread_, false, 0);
    sem_init(&sem_notify_got_waitting_job_, false, 0);

    // 创建管理waitting job thread线程
    waitting_job_thread_mgr_ = new std::thread(fnWaittingJobThreadProc, this);

    //创建调整线程个数的管理线程
    substract_thread_mgr_ = new std::thread(fnSubstractThreadProc, this);

}


TPThreadPool::~TPThreadPool()
{
    int res = 0;
	RSLOG_DEBUG << "Stop all threads,wait all exit";
	bool bRet = false;
	API_VERIFY(stopAndWait());

	RSLOG_DEBUG << "Destroy pool";
	_destroyPool();

	TPASSERT(set_waiting_jobs_.empty());
	TPASSERT(map_doing_hobs_.empty());
    RSLOG_DEBUG << "running_thread_number_ = " << last_running_thread_;

    {
        exit_waitting_job_thread_ = true;
        sem_post(&sem_job_needed_to_do_);
        sem_post(&sem_notify_got_waitting_job_);
    }
    waitting_job_thread_mgr_->join();
    RSLOG_DEBUG << "waitting job thread mgr exit!";
    delete waitting_job_thread_mgr_;
    waitting_job_thread_mgr_ = NULL;

    {
        std::lock_guard<std::mutex> subtract_locker(subtract_thread_lock_);
        queue_substract_thread_.push(0);
        RSLOG_DEBUG << "notify substract thread manager exit";
        sem_post(&sem_subtract_thread_);
    }

    substract_thread_mgr_->join();
    RSLOG_DEBUG << "substract thread manager exit!";
    delete substract_thread_mgr_;
    substract_thread_mgr_ = NULL;

    DestroyEvent(job_needed_to_do_event_);
    DestroyEvent(subtract_thread_event_);
    DestroyEvent(all_thread_complete_event_);
    DestroyEvent(stop_event_);

    sem_destroy(&sem_job_needed_to_do_);
    sem_destroy(&sem_subtract_thread_);
    sem_destroy(&sem_notify_complete_substract_thread_);
    sem_destroy(&sem_notify_got_waitting_job_);

    RSLOG_DEBUG << "~TPThreadPool";
}


bool TPThreadPool::start(int min_count_threads, int max_count_threads)
{
	RSLOG_DEBUG << "entry ...";
	bool bRet = true;
    std::lock_guard<std::mutex> threadLocker(running_threads_lock_);
    if (last_running_thread_ > 0)
    {
        RSLOG_DEBUG << "had created thread pool!";
        bRet = false;
        RSLOG_DEBUG << "leave";
        return bRet;
    }

    RSLOG_DEBUG << "thread min count = " << min_count_threads << ", max count = " << max_count_threads;
	min_count_threads_ = min_count_threads;
	max_count_threads_ = max_count_threads;

	int res = ResetEvent(stop_event_);
	RSLOG_DEBUG << "reset stop event, res = " << res;
	res = ResetEvent(all_thread_complete_event_);
	RSLOG_DEBUG << "reset all thread complete event, res = " << res;

    _addJobThread(min_count_threads);										// 开始时只创建 min_number_threads_ 个线程

	RSLOG_DEBUG << "leave";
	return bRet;
}

void TPThreadPool::stop()
{	
	RSLOG_DEBUG << "entry ...";

	int ret = SetEvent(stop_event_);

	RSLOG_DEBUG << "leave = " << ret;
}

bool TPThreadPool::wait(uint64_t dwTimeOut)
{
	RSLOG_DEBUG << "entry ...";
    RSLOG_DEBUG_F2("dwTimeOut = 0x%016LX", dwTimeOut);

	bool bRet = false;
	int dwRes = WaitForEvent(all_thread_complete_event_, dwTimeOut);
	if (dwRes == 0)
	{
		bRet = true;
	}
	else
	{
		RSLOG_DEBUG << "Not all thread over in" << dwTimeOut << "millisec";
		TPASSERT(false);
	}

	RSLOG_DEBUG << "leave";
	return bRet;
}

bool TPThreadPool::stopAndWait(uint64_t dwTimeOut)
{
	RSLOG_DEBUG << "entry ...";
	bool bRet = true;
	RSLOG_DEBUG << "call stop func";
	stop();
	RSLOG_DEBUG << "call wait func";
	API_VERIFY(wait(dwTimeOut));
	RSLOG_DEBUG << "leave";
	return bRet;
}

void TPThreadPool::clearUndoWork()
{
    RSLOG_DEBUG << "entry...";
    std::lock_guard<std::mutex> locker(waiting_jobs_lock_);
    RSLOG_DEBUG_F2("waitingJob Number is %d", set_waiting_jobs_.size());
    while (!set_waiting_jobs_.empty())
    {
        int semRet = sem_trywait(&sem_job_needed_to_do_);
        if (semRet != 0)
        {
            RSLOG_ERROR << "set waitting jobs sem wait error";
        }
        else
        {
            RSLOG_DEBUG << "wait sem, get job";
        }

        WaitingJobContainer::iterator iterBegin = set_waiting_jobs_.begin();
        TPBaseJob* pJob = *iterBegin;
        TPASSERT(pJob);
        _notifyJobCancel(pJob);
        pJob->_onCancelJob();
        set_waiting_jobs_.erase(iterBegin);
    }
    RSLOG_DEBUG << "leave...";
}

bool TPThreadPool::submitJob(TPBaseJob* pJob, int* pOutJobId)
{
	RSLOG_DEBUG <<"entry...";
	TPASSERT(NULL != stop_event_);									// 如果调用 _DestroyPool后，就不能再次调用该函数

	bool bRet = false;
    int waittingJobSize = 0;

	// 加入Job并且唤醒一个等待线程
	{
		job_index_.fetch_add(1, std::memory_order_seq_cst);
        RSLOG_DEBUG << "job id = " << job_index_;
		std::lock_guard<std::mutex> waitingLocker(waiting_jobs_lock_);
		pJob->thread_pool_ = this;								// 访问私有变量，并将自己赋值过去
		pJob->job_id_ = job_index_;								// 访问私有变量，设置JobIndex

		if (pOutJobId)
		{
			*pOutJobId = pJob->job_id_;
		}

        RSLOG_DEBUG << "insert waitting job" << job_index_;
		set_waiting_jobs_.insert(pJob);
        RSLOG_DEBUG << "post sem waitting job" << job_index_;
        waittingJobSize = set_waiting_jobs_.size();
        sem_post(&sem_job_needed_to_do_);
	}

    // 导致调用线程放弃CPU。线程被放置在运行队列的末尾，以保持其静态优先级，并计划另一个线程运行。
    std::this_thread::yield();

	{
		// 当所有的线程都在运行Job时，则需要增加线程  -- 不对 m_nRunningJobNumber 加保护(只是读取),读取的话也要加保护
		std::lock_guard<std::mutex>  threadLocker(running_threads_lock_);
        bool bNeedMoreThread = (waittingJobSize > 0) && (last_running_thread_ < max_count_threads_);
		if (bNeedMoreThread)
		{
			int res = _addJobThread(1L);				// 每次增加一个线程
			RSLOG_DEBUG << "Add job thread, res = " << res;
		}

        RSLOG_DEBUG_F6("pJob[%d] = %p, waitting_job_count = %d, cur_number_threads_ = %d, bNeedMoreThread = %d",
            pJob->job_id_, pJob, waittingJobSize, last_running_thread_, bNeedMoreThread);
	}

	RSLOG_DEBUG << "leave";
	return bRet;	
}

// STOP是否有信号
int TPThreadPool::hadRequestStop() const
{
	RSLOG_DEBUG << "entry...";

	TPASSERT(NULL != stop_event_);
    int ret = WaitForEvent(stop_event_, 0);

	RSLOG_DEBUG << "leave, ret = " << ret;

    return ret;
}

// 增加运行的线程,如果 当前线程数 + thread_num <= max_thread_num_ 时会成功执行
int  TPThreadPool::_addJobThread(int count)
{
	RSLOG_DEBUG << "entry...";	
	int ret = 0;
	int addCount = 0;
	if (max_count_threads_ - last_running_thread_ == 0)
	{
		RSLOG_DEBUG << "线程池数量已满，无法增加！";
		return 0;
	}
	else if (max_count_threads_ - last_running_thread_ < count)
	{
		addCount = max_count_threads_ - last_running_thread_;
	}
	else
	{
		addCount = count;
	}

	int i = 0;
    while (i < addCount)
    {
		int index = i + last_running_thread_;
        std::thread* pTh = new std::thread(fnJobThreadProc, this);
        list_running_threads_.push_back(pTh);
        last_running_thread_++;
        RSLOG_DEBUG << "sem post add thread operation";
		i++;
    }
    ret = i;

	RSLOG_DEBUG << "leave, ret = " << ret;

    return ret;
}

void TPThreadPool::_clearRunningMapThread()
{
    std::lock_guard<std::mutex> threadLocker(running_threads_lock_);
    pid_t cur_tid = syscall(SYS_gettid);
    RSLOG_DEBUG << "tid = " << *(unsigned int*)&cur_tid;
    std::map<pid_t, std::thread::id>::iterator it = map_thread_ids_.find(cur_tid);
    if (it != map_thread_ids_.end())
    {
        std::list<std::thread*>::iterator runningIt = list_running_threads_.begin();
        for (; runningIt != list_running_threads_.end(); runningIt++)
        {
            std::thread::id pid = (*runningIt)->get_id();
            if (pid == it->second)
            {
                list_removing_threads_.push_back(*runningIt);
                list_running_threads_.erase(runningIt);
                RSLOG_INFO_F6("TPThreadPool Subtract a thread, thread id = %u(0x%x) - tid = %u(0x%x), curThreadIndex = %d",
                                pid, pid, cur_tid, cur_tid, last_running_thread_-1);
                break;
            }
        }
        RSLOG_DEBUG_F5("Remove it from mapping item, tid = %u(Ox%0x), pid_t = %u(0x%x)", it->second, it->second, it->first, it->first);
        map_thread_ids_.erase(it);
    }

    last_running_thread_--;
    RSLOG_DEBUG << "op sub, last_running_thread_ = " << last_running_thread_;
}

void TPThreadPool::_destroyPool()
{
    RSLOG_DEBUG << "TPThreadPool::_destroyPool entry...";

    clearUndoWork();

    RSLOG_DEBUG << "TPThreadPool::_destroyPool leave...";
}

EThreadItemOp TPThreadPool::_getJob(TPBaseJob** ppJob)
{
	RSLOG_DEBUG << "entry...";
	EThreadItemOp retWaitType = ETh_Op_Undefine;
	smart_event_t waitEvents[] = 
	{
        // 优先响应 job_needed_to_do_event_ 还是 subtract_thread_event_ ?
        // 1.优先响应 job_needed_to_do_event_ 可以避免线程的波;动
        // 2.优先响应 subtract_thread_event_ 可以优先满足用户手动要求减少线程的需求(虽然目前尚未提供该接口)
		stop_event_,						// user stop thread pool
		job_needed_to_do_event_,			// there are waiting jobs
		subtract_thread_event_,				// need subtract thread
	};

    int waitIndex = -1;
    int res = WaitForMultipleEvents(waitEvents, 3, false, WAIT_INFINITE, waitIndex);
    RSLOG_DEBUG <<"wait multi events, res = " << res << ", wait index = " << waitIndex;
    if (res == 0)
    {
        if (waitIndex == 0)
        {
            retWaitType = ETh_Op_Exit_Th_Pool;
            RSLOG_DEBUG << "get stop event";
            goto ret_end;
        }
        else if (waitIndex == 1)
        {
            retWaitType = ETh_Op_Get_Job;
            RSLOG_DEBUG << "get job event";
        }
        else if (waitIndex == 2)
        {
            retWaitType = ETh_Op_Sub_Th;
            RSLOG_DEBUG << "sub thread event";
            goto ret_end;
        }
    }
    else
    {
        retWaitType = ETh_Op_Sub_Th;
        RSLOG_DEBUG << "sub thread event";
        goto ret_end;
    }


	{
		// 从等待容器中获取用户作业
        std::lock_guard<std::mutex> lockerWating(waiting_jobs_lock_);
        TPASSERT(!set_waiting_jobs_.empty());
        WaitingJobContainer::iterator iterBegin = set_waiting_jobs_.begin();
        TPBaseJob* pJob = *iterBegin;
        TPASSERT(pJob);
        *ppJob = pJob;
        set_waiting_jobs_.erase(iterBegin);
        RSLOG_DEBUG << "取到正在等待的工作的job";
        {
            // 放到进行作业的容器中
            std::lock_guard<std::mutex> lockerDoing(mutex_doing_jobs_);
            map_doing_hobs_.insert(MapDoingJobContainer::value_type(pJob->getJobId(), pJob));
        }
        RSLOG_DEBUG << "post sem notify got waitting job";
        sem_post(&sem_notify_got_waitting_job_);
	}

ret_end:
    RSLOG_DEBUG << "leave, typGetJob = " << retWaitType;
	return retWaitType;	
}

void TPThreadPool::doJobs()
{
	RSLOG_DEBUG << "entry ...";
	TPBaseJob* pJob = NULL;
	EThreadItemOp getJobType = ETh_Op_Undefine;

	// 一直等job的事件进来
	while(ETh_Op_Get_Job == (getJobType = _getJob(&pJob)))
	{
        RSLOG_DEBUG << "ETh_Op_Get_Job";
        int jobId = pJob->getJobId();
        RSLOG_DEBUG << "Begin Run Job, id = " << jobId;

        running_job_count_.fetch_add(1, std::memory_order_seq_cst);
        RSLOG_DEBUG << "running_job_count_ = " << running_job_count_.load();
		pJob->_initialize();
		{
			// 这个地方的设计和实现不是很好，是否有更好的方法?
			TPASSERT(NULL == pJob->stop_job_event_);
			pJob->stop_job_event_ = CreateEvent(true, false);

			_notifyJobBegin(pJob);
			pJob->_run();
			_notifyJobEnd(pJob);

			DestroyEvent(pJob->stop_job_event_);
			pJob->_finalize();
		}
		running_job_count_.fetch_sub(1, std::memory_order_seq_cst);
		RSLOG_DEBUG << "running_job_count_ = " << running_job_count_.load();

		RSLOG_DEBUG << "Run Job(" << jobId << ")";
		{
            RSLOG_DEBUG << "remove doing map entry...";
			// Job结束，首先从运行列表中删除
			std::lock_guard<std::mutex> lockerDoing(mutex_doing_jobs_);
			MapDoingJobContainer::iterator iter = map_doing_hobs_.find(jobId);
			if (map_doing_hobs_.end() != iter)
			{
				map_doing_hobs_.erase(iter);
			}
            RSLOG_DEBUG << "remove doing map leave";
		}

		// 检查一下是否需要减少线程
		bool bNeedSubtractThread = false;
		{
            RSLOG_DEBUG << "try need substract thread entry ...";
			std::lock_guard<std::mutex> waitingLocker(waiting_jobs_lock_);
			std::lock_guard<std::mutex> threadLocker(running_threads_lock_);
			// 当队列中没有Job，并且当前线程数大于最小线程数时
			int retHad = hadRequestStop(); // ? stop信号
            bNeedSubtractThread = (set_waiting_jobs_.empty() && (last_running_thread_ > min_count_threads_) && (retHad != 0));
            RSLOG_DEBUG << "bNeedSubtractThread = " << bNeedSubtractThread;
			if (bNeedSubtractThread)
			{
				// 通知减少一个线程
                RSLOG_DEBUG << "had waited substract sem!";
                std::lock_guard<std::mutex> subtract_locker(subtract_thread_lock_);
                RSLOG_DEBUG << "need sub thread, last_running_thread_ = " << last_running_thread_;
                queue_substract_thread_.push(last_running_thread_);
                RSLOG_DEBUG << "post substract sem!";
                sem_post(&sem_subtract_thread_);
			}
            RSLOG_DEBUG << "try need substract thread leave";
		}
	}
	if (ETh_Op_Sub_Th == getJobType)			// 需要减少线程,应该把自己退出 -- 注意：通知退出的线程和实际退出的线程可能不是同一个
	{
        RSLOG_DEBUG << "ETh_Op_Sub_Th";
        _clearRunningMapThread();
        sem_post(&sem_notify_complete_substract_thread_);
	}
	else
	{
        RSLOG_DEBUG << "ETh_Op_Exit_Th_Pool";
        _clearRunningMapThread();

	}

    RSLOG_DEBUG << "post notify substract sem " << last_running_thread_;
	RSLOG_DEBUG << "leave";
}

//////////////////////////////////////////////////////////////////////////
void TPThreadPool::_notifyJobBegin(TPBaseJob* pJob)
{

    RSLOG_DEBUG << "TPThreadPool::_notifyJobBegin entry...";
	TPASSERT(pJob);
	if (pJob && m_pCallBack)
	{
        m_pCallBack->onJobBegin(pJob->getJobId(), pJob);
	}
    RSLOG_DEBUG << "TPThreadPool::_notifyJobBegin leave";

}

void TPThreadPool::_notifyJobEnd(TPBaseJob* pJob)
{
    RSLOG_DEBUG << "TPThreadPool::_notifyJobEnd entry...";
	TPASSERT(pJob);
	if (pJob && m_pCallBack)
	{
        m_pCallBack->onJobEnd(pJob->getJobId(), pJob);
	}
    RSLOG_DEBUG << "TPThreadPool::_notifyJobEnd leave";
}

void TPThreadPool::_notifyJobCancel(TPBaseJob* pJob)
{
    RSLOG_DEBUG << "TPThreadPool::_notifyJobCancel entry...";
	TPASSERT(pJob);
	if (pJob && m_pCallBack)
	{
        m_pCallBack->onJobCancel(pJob->getJobId(), pJob);
	}
    RSLOG_DEBUG << "TPThreadPool::_notifyJobCancel leave";
}

 
