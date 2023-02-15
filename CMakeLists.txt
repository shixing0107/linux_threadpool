cmake_minimum_required(VERSION 3.5.0)
project(threadpool VERSION 0.1.0)

# include(CTest)
# enable_testing()

file(GLOB RSLOGGER_DIR_SRC rslogger/*.cpp rslogger/*.h)
file(GLOB RSTHREADPOOL_DIR_SRC threadpool/*.cpp threadpool/*.h)

source_group("RsLogger" FILES ${RSLOGGER_DIR_SRC})
source_group("ThreadPool" FILES ${RSTHREADPOOL_DIR_SRC})

include_directories(${PROJECT_SOURCE_DIR}/rslogger)
include_directories(${PROJECT_SOURCE_DIR}/threadpool)

#if (CMAKE_VERSION VERSION_LESS "3.1")
#  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
#  endif ()
#else ()
#  set (CMAKE_CXX_STANDARD 11)
#endif ()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE "Debug")
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(PROJECT_NAME threadpool)
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME}
    ajob.h
    ajob.cpp
    bjob.h
    bjob.cpp
    cjob.h
    cjob.cpp
    main.cpp
    ${RSLOGGER_DIR_SRC}
    ${RSTHREADPOOL_DIR_SRC}
    )

# target_link_libraries(threadpool pthread)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
