/////////////////////////////////////////////////////////////////////////////////
/// Copyright (C), 2015, 
/// Richinfo Corporation. All rights reserved.
/// @file    threadPool/threadpoolcallback.h
/// @author  Simone
/// @date    2022/12/08
/// @brief   
///
/// @history v0.01 2022/12/08  单元创建
/////////////////////////////////////////////////////////////////////////////////

#ifndef _THREAD_POOL_CALLBACK_H__
#define _THREAD_POOL_CALLBACK_H__

class TPBaseJob;



// 回调函数
class TPThreadPoolCallBack
{
public:

	TPThreadPoolCallBack();

	virtual ~TPThreadPoolCallBack();

	//当Job运行起来以后，会由 Pool 激发 Begin 和 End 两个函数

	virtual void onJobBegin(int jobId, TPBaseJob* pJob );

	virtual void onJobEnd(int jobId, TPBaseJob* pJob);

	//如果尚未到达运行状态就被取消的Job，会由Pool调用这个函数
	virtual void onJobCancel(int jobId, TPBaseJob* pJob);


	//Progress 和 Error 由 JobBase 的子类激发
	virtual void onJobProgress(int jobId , TPBaseJob* pJob,  unsigned long long curPos,  unsigned long long totalSize);


	virtual void onJobError(int jobId , TPBaseJob* pJob, int errCode, const char* desc);


};

#endif