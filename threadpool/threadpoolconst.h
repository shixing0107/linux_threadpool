/////////////////////////////////////////////////////////////////////////////////
/// Copyright (C), 2022, 
/// Richinfo Corporation. All rights reserved.
/// @file    threadpoolconst.h
/// @author  Simone
/// @date    2022/12/08
/// @brief   
///
/// @history v0.01 2022/12/08  单元创建
/////////////////////////////////////////////////////////////////////////////////

#ifndef __THREAD_POOL_CONST_H__
#define __THREAD_POOL_CONST_H__

#include <cstdint>

#define MAX_THREAD_COUNT 64
#define MIN_THREAD_COUNT 5

// Constant declarations
const uint64_t WAIT_INFINITE = ~((uint64_t)0);

#endif
