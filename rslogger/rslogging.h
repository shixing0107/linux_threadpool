#ifndef __RSLOGGING_H_
#define __RSLOGGING_H_

#include "logstream.h"
#include "timestamp.h"
#include <stdio.h>
#include <time.h>
#include "rslog.h"


class RsLogger
{
public:
  //嵌套类，用作外层类的实现
  //封装记录LOG的文件名
  class SourceFile
  {
  public:
    template <int N>
    SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1)
    {
        //通过传入的地址找到记录LOG的文件名
        //strrchr 查找一个字符c在另一个字符串str中末次出现的位置（也就是从str的右侧开始查找字符c首次出现的位置），并返回这个位置的地址
        //如果未能找到指定字符，那么函数将返回NULL
        //slash 斜线
        //地址中最后一个/后的就是文件名，前面的是路径
        const char *slash = strrchr(data_, '/');
        if (slash)
        {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
        }
    }

    explicit SourceFile(const char *filename) : data_(filename)
    {
        const char *slash = strrchr(filename, '/');
        if (slash)
        {
            data_ = slash + 1;
        }
        size_ = static_cast<int>(strlen(data_));
    }

    const char *data_;
    int size_;
  };

  //file的第line行输出了等级为level的日志
  RsLogger(SourceFile file, int line);
  RsLogger(SourceFile file, int line, ERsLogLevel level);
  RsLogger(SourceFile file, int line, ERsLogLevel level, const char *func);
  RsLogger(SourceFile file, int line, bool toAbort); //toAbort,是否终止程序运行
  ~RsLogger();

  RsLogStream &stream();

  static ERsLogLevel logLevel();
  static void setLogLevel(ERsLogLevel level);

  static void setTimeZone(const struct timezone &tz);

private:
  //实际工作都交割Imple类去完成
  //Imple代表一条日志消息
  //作用是为了更好的封装
  class Impl
  {
  public:
    Impl(ERsLogLevel level, int old_errno, const SourceFile &file, int line);
    void formatTime();
    void finish();

    RsTimestamp time_;
    RsLogStream stream_;
    ERsLogLevel level_;
    int line_;
    SourceFile basename_;
  };

  Impl impl_;
};

//下列宏先建立一个RsLogger对象，再返回其stream()成员函数，即一个LogStream对象
//把日志记录在LogStream中
//创建的是临时对象，RsLogger对象析构时将LogStream中的内容写入文件
//__LINE__：在源代码中插入当前源代码行号；
//__FILE__：在源文件中插入当前源文件名；
//__func__指示当前函数名
//检查输入是否为空
// #是“字符串化”的意思。出现在宏定义中的#是把跟在后面的参数转换成一个字符串
//##是一个连接符号，用于把参数连在一起
//邻近字符串连接 "'" #val "' Must be non NULL"会合并成一个大字符串
//////////////////////////////////////////////////////////////////////////
#define RSLOG_TRACE                                          \
  if (RsLogger::logLevel() <= ERsLogLevel::TRACE) \
  RsLogger(__FILE__, __LINE__, ERsLogLevel::TRACE, __func__).stream()
#define RSLOG_DEBUG                                          \
  if (RsLogger::logLevel() <= ERsLogLevel::DEBUG) \
  RsLogger(__FILE__, __LINE__, ERsLogLevel::DEBUG, __func__).stream()
#define RSLOG_INFO                                          \
  if (RsLogger::logLevel() <= ERsLogLevel::INFO) \
  RsLogger(__FILE__, __LINE__, ERsLogLevel::INFO, __func__).stream()
#define RSLOG_WARN RsLogger(__FILE__, __LINE__, ERsLogLevel::WARN).stream()
#define RSLOG_ERROR RsLogger(__FILE__, __LINE__, ERsLogLevel::ERROR).stream()
#define RSLOG_FATAL RsLogger(__FILE__, __LINE__, ERsLogLevel::FATAL).stream()

#define RSLOG_SYSERR RsLogger(__FILE__, __LINE__, false).stream()  //系统错误
#define RSLOG_SYSFATAL RsLogger(__FILE__, __LINE__, true).stream() //系统严重错误

const char *strerror_tl(int savedErrno);

//////////////////////////////////////////////////////////////////////////
const char* source_file(const char* filePath);
#define __RS_FILE__ source_file(__FILE__)
// LOG_TRACE
#define LOG_TRACE_F(x1) g_RsLog.trace(source_file(__FILE__), __LINE__, __FUNCTION__, x1)
#define LOG_TRACE_F2(x1, x2) g_RsLog.trace(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2)
#define LOG_TRACE_F3(x1, x2, x3) g_RsLog.trace(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3)
#define LOG_TRACE_F4(x1, x2, x3, x4) g_RsLog.trace(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4)
#define LOG_TRACE_F5(x1, x2, x3, x4, x5) g_RsLog.trace(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5)
#define LOG_TRACE_F6(x1, x2, x3, x4, x5, x6) g_RsLog.trace(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6)
#define LOG_TRACE_F7(x1, x2, x3, x4, x5, x6, x7) g_RsLog.trace(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7)
#define LOG_TRACE_F8(x1, x2, x3, x4, x5, x6, x7, x8) g_RsLog.trace(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8)
#define LOG_TRACE_F9(x1, x2, x3, x4, x5, x6, x7, x8, x9) g_RsLog.trace(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8, x9)

// RSLOG_INFO
#define RSLOG_INFO_F(x1) g_RsLog.info(source_file(__FILE__), __LINE__, __FUNCTION__, x1)
#define RSLOG_INFO_F2(x1, x2) g_RsLog.info(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2)
#define RSLOG_INFO_F3(x1, x2, x3) g_RsLog.info(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3)
#define RSLOG_INFO_F4(x1, x2, x3, x4) g_RsLog.info(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4)
#define RSLOG_INFO_F5(x1, x2, x3, x4, x5) g_RsLog.info(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5)
#define RSLOG_INFO_F6(x1, x2, x3, x4, x5, x6) g_RsLog.info(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6)
#define RSLOG_INFO_F7(x1, x2, x3, x4, x5, x6, x7) g_RsLog.info(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7)
#define RSLOG_INFO_F8(x1, x2, x3, x4, x5, x6, x7, x8) g_RsLog.info(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8)
#define RSLOG_INFO_F9(x1, x2, x3, x4, x5, x6, x7, x8, x9) g_RsLog.info(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8, x9)

// RSLOG_DEBUG
#define RSLOG_DEBUG_F(x1) g_RsLog.debug(source_file(__FILE__), __LINE__, __FUNCTION__, x1)
#define RSLOG_DEBUG_F2(x1, x2) g_RsLog.debug(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2)
#define RSLOG_DEBUG_F3(x1, x2, x3) g_RsLog.debug(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3)
#define RSLOG_DEBUG_F4(x1, x2, x3, x4) g_RsLog.debug(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4)
#define RSLOG_DEBUG_F5(x1, x2, x3, x4, x5) g_RsLog.debug(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5)
#define RSLOG_DEBUG_F6(x1, x2, x3, x4, x5, x6) g_RsLog.debug(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6)
#define RSLOG_DEBUG_F7(x1, x2, x3, x4, x5, x6, x7) g_RsLog.debug(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7)
#define RSLOG_DEBUG_F8(x1, x2, x3, x4, x5, x6, x7, x8) g_RsLog.debug(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8)
#define RSLOG_DEBUG_F9(x1, x2, x3, x4, x5, x6, x7, x8, x9) g_RsLog.debug(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8, x9)


// RSLOG_ERROR
#define RSLOG_ERROR_F(x1) g_RsLog.error(source_file(__FILE__), __LINE__, __FUNCTION__, x1)
#define RSLOG_ERROR_F2(x1, x2) g_RsLog.error(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2)
#define RSLOG_ERROR_F3(x1, x2, x3) g_RsLog.error(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3)
#define RSLOG_ERROR_F4(x1, x2, x3, x4) g_RsLog.error(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4)
#define RSLOG_ERROR_F5(x1, x2, x3, x4, x5) g_RsLog.error(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5)
#define RSLOG_ERROR_F6(x1, x2, x3, x4, x5, x6) g_RsLog.error(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6)
#define RSLOG_ERROR_F7(x1, x2, x3, x4, x5, x6, x7) g_RsLog.error(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7)
#define RSLOG_ERROR_F8(x1, x2, x3, x4, x5, x6, x7, x8) g_RsLog.error(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8)
#define RSLOG_ERROR_F9(x1, x2, x3, x4, x5, x6, x7, x8, x9) g_RsLog.error(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8, x9)


// LOG_WARNING
#define LOG_WARN_F(x1) g_RsLog.warn(source_file(__FILE__), __LINE__, __FUNCTION__, x1)
#define LOG_WARN_F2(x1, x2) g_RsLog.warn(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2)
#define LOG_WARN_F3(x1, x2, x3) g_RsLog.warn(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3)
#define LOG_WARN_F4(x1, x2, x3, x4) g_RsLog.warn(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4)
#define LOG_WARN_F5(x1, x2, x3, x4, x5) g_RsLog.warn(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5)
#define LOG_WARN_F6(x1, x2, x3, x4, x5, x6) g_RsLog.warn(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6)
#define LOG_WARN_F7(x1, x2, x3, x4, x5, x6, x7) g_RsLog.warn(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7)
#define LOG_WARN_F8(x1, x2, x3, x4, x5, x6, x7, x8) g_RsLog.warn(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8)
#define LOG_WARN_F9(x1, x2, x3, x4, x5, x6, x7, x8, x9) g_RsLog.warn(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8, x9)


// LOG_FATAL
#define LOG_FATAL_F(x1) g_RsLog.fatal(source_file(__FILE__), __LINE__, __FUNCTION__, x1)
#define LOG_FATAL_F2(x1, x2) g_RsLog.fatal(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2)
#define LOG_FATAL_F3(x1, x2, x3) g_RsLog.fatal(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3)
#define LOG_FATAL_F4(x1, x2, x3, x4) g_RsLog.fatal(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4)
#define LOG_FATAL_F5(x1, x2, x3, x4, x5) g_RsLog.fatal(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5)
#define LOG_FATAL_F6(x1, x2, x3, x4, x5, x6) g_RsLog.fatal(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6)
#define LOG_FATAL_F7(x1, x2, x3, x4, x5, x6, x7) g_RsLog.fatal(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7)
#define LOG_FATAL_F8(x1, x2, x3, x4, x5, x6, x7, x8) g_RsLog.fatal(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8)
#define LOG_FATAL_F9(x1, x2, x3, x4, x5, x6, x7, x8, x9) g_RsLog.fatal(source_file(__FILE__), __LINE__, __FUNCTION__, x1, x2, x3, x4, x5, x6, x7, x8, x9)


#endif // RSLOGGING_H
