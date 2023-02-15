#include "rslogging.h"
#include <cerrno>
#include <cstring>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include "timestamp.h"
#include "curthread.h"
#include "rslog.h"

__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;


// 在多线程程序中，errno会按照每个线程来储存，因此具有线程安全性。
// 返回错误提示的字符串
// 对于函数strerror_r，第一个参数errnum是错误代码，第二个参数buf是用户提供的存储错误描述的缓存，第三个参数n是缓存的大小。
const char *strerror_tl(int savedErrno)
{
    return strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
}

const char* source_file(const char* filePath)
{
    const char* fileName = nullptr;
    const char *slash = strrchr(filePath, '/');
    if (slash)
    {
        fileName = slash + 1;
    }

    return fileName;
}

// 方便传递string对象
class T
{
public:
    T(const char *str, unsigned len)
        : str_(str),
          len_(len)
    {
        assert(strlen(str) == len_);
    }

    const char *str_;
    const unsigned len_;
};

// 重载T和SourceFile对象的输入
inline RsLogStream &operator<<(RsLogStream &s, T v)
{
    s.append(v.str_, v.len_);
    return s;
}

inline RsLogStream &operator<<(RsLogStream &s, const RsLogger::SourceFile &v)
{
    s.append(v.data_, v.size_);
    return s;
}

// time()返回自纪元 Epoch（1970-01-01 00:00:00 UTC）起经过的时间，以秒为单位
RsLogger::Impl::Impl(ERsLogLevel level, int savedErrno, const SourceFile &file, int line)
    : time_(RsTimestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(file)
{
    formatTime();         //先记录时间
    // 输入日志级别
    stream_ << " [" << T(g_RsLog.getLogLevelName(level), strlen(g_RsLog.getLogLevelName(level))) << "] - ";
    CurrentThread::tid(); //取得该线程的tid
    // 输入线程id
    stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength()) << " - ";

    // 如果出错则记录错误信息
    if (savedErrno != 0)
    {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

// 如果和上次记录的时间差距一秒以上，则在日志中记录时间
void RsLogger::Impl::formatTime()
{
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / RsTimestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % RsTimestamp::kMicroSecondsPerSecond);
    if (seconds != t_lastSecond)
    {
        t_lastSecond = seconds;
        struct tm tm_time;
        //获取当前时间结构，本地时间，有时区转换
        ::localtime_r(&seconds, &tm_time);

        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17);
        (void)len;
    }

    Fmt us(".%06d ", microseconds);
    assert(us.length() == 8);
    stream_ << T(t_time, 17) << T(us.data(), 8);
}

void RsLogger::Impl::finish()
{
    stream_ << " - " << basename_ << ':' << line_ << '\n';
}

// Loglevel默认是INFO
RsLogger::RsLogger(SourceFile file, int line)
    : impl_(INFO, 0, file, line)
{
}

RsLogger::RsLogger(SourceFile file, int line, ERsLogLevel level, const char *func)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << func << ": ";
}

RsLogger::RsLogger(SourceFile file, int line, ERsLogLevel level)
    : impl_(level, 0, file, line)
{
}

RsLogger::RsLogger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort ? FATAL : ERROR, errno, file, line)
{
}

// 析构Loggger时，将stream里的内容通过g_output写入设置的文件中
RsLogger::~RsLogger()
{
    impl_.finish();
    const RsLogStream::Buffer &buf(stream().buffer());
    g_RsLog.output(buf.data(), buf.length());
}

ERsLogLevel RsLogger::logLevel()
{
    return g_RsLog.getLogLevel();
}

void RsLogger::setLogLevel(ERsLogLevel level)
{
    g_RsLog.setLogLevel(level);
}

RsLogStream& RsLogger::stream()
{
    return impl_.stream_;
}

