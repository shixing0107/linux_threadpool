/////////////////////////////////////////////////////////////////////////////////
/// Copyright (C), 2022, 
/// Richinfo Corporation. All rights reserved.
/// @file    threadPool\BaseJob.cpp
/// @author  Simone
/// @date    2022/12/08
/// @brief   
///
/// @history v0.01 2022/12/08  单元创建
/////////////////////////////////////////////////////////////////////////////////

#include "basejob.h"
#include "threadpooldefine.h"
#include "rslogging.h"
#include "pevents.h"
#include "threadpool.h"


///////////////////////////////////////////// TPBaseJob ///////////////////////////////////////////////////
//  初始化
TPBaseJob::TPBaseJob(int job_priority):
job_priority_(job_priority)
{
    job_id_                     = 0;
	thread_pool_				= NULL;
	stop_job_event_				= NULL;
}

TPBaseJob::~TPBaseJob()
{
	if (stop_job_event_)
	{
		DestroyEvent(stop_job_event_);
		stop_job_event_ = NULL;
	}
}

bool TPBaseJob::operator< (const TPBaseJob& other) const
{
	COMPARE_MEM_LESS(job_priority_, other);
	COMPARE_MEM_LESS(job_id_, other);

	return true;
}

int TPBaseJob::getJobId() const
{
	return job_id_;
}

bool TPBaseJob::requestCancel()
{
	RSLOG_DEBUG << "entry ...";
	RSLOG_DEBUG << "set stop event";
	SetEvent(stop_job_event_);
	RSLOG_DEBUG << "end";

    return true;
}

void TPBaseJob::_notifyCancel()
{	
	RSLOG_DEBUG << "entry ...";
	if (thread_pool_)
	{
		RSLOG_DEBUG << "notify job cancel";
		thread_pool_->_notifyJobCancel(this);
	}
    RSLOG_DEBUG << "end";
}


void TPBaseJob::_initialize()
{
	// TPASSERT(NULL == stop_job_event_);
	RSLOG_DEBUG << "entry ...";
	if (stop_job_event_)
	{
		DestroyEvent(stop_job_event_);
	}
	stop_job_event_ = NULL;
	RSLOG_DEBUG << "end";
}
