/////////////////////////////////////////////////////////////////////////////////
/// Copyright (C), 2022, 
/// Richinfo Corporation. All rights reserved.
/// @file    threadpoolcallback.cpp
/// @author  Simone
/// @date    2022/12/08
/// @brief   
///
/// @history v0.01 2022/12/08  单元创建
/////////////////////////////////////////////////////////////////////////////////


#include "threadpoolcallback.h"
#include "rslogging.h"



TPThreadPoolCallBack::TPThreadPoolCallBack()
{
    RSLOG_DEBUG << "construct";
}


TPThreadPoolCallBack::~TPThreadPoolCallBack()
{
    RSLOG_DEBUG << "desconstruct";
}


void TPThreadPoolCallBack::onJobBegin(int jobId, TPBaseJob* pJob )
{
    RSLOG_DEBUG << "entry ...";
    RSLOG_DEBUG << "end";
} 

void TPThreadPoolCallBack::onJobEnd(int jobId, TPBaseJob* pJob)
{
    RSLOG_DEBUG << "entry ...";
    RSLOG_DEBUG << "end";
}

//如果尚未到达运行状态就被取消的Job，会由Pool调用这个函数
void TPThreadPoolCallBack::onJobCancel(int jobId, TPBaseJob* pJob)
{
    RSLOG_DEBUG << "entry ...";
    RSLOG_DEBUG << "end";
}

//Progress 和 Error 由 JobBase 的子类激发
void TPThreadPoolCallBack::onJobProgress(int jobId, TPBaseJob* pJob,  unsigned long long curPos,  unsigned long long totalSize)
{
    RSLOG_DEBUG << "entry ...";
    RSLOG_DEBUG << "end";
}

void TPThreadPoolCallBack::onJobError(int jobId, TPBaseJob* pJob, int errCode, const char* desc)
{
    RSLOG_DEBUG << "entry ...";
    RSLOG_DEBUG << "end";
}