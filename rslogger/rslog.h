#ifndef __RSLOG_H_
#define __RSLOG_H_

#include <mutex>
#include <string>

typedef enum ERsLogLevel
{
    TRACE,
    DEBUG,
    INFO,   // 打印一些感兴趣的或者重要信息
    WARN,   // 警告，进行一些修复性的工作，应该还可以把系统修复到正常状态中来，系统应该可以继续进行下去
    ERROR,  // 错误，可以进行一些修复性的工作，但无法确定系统会正常的工作下去
    FATAL,  // 严重错误，无法修复的错误，尽可能地保留系统有效数据并停止运行
    NUM_LOG_LEVELS
}ERsLogLevel;



class RsLog
{
public:
    static RsLog& Instance();

    void setLogPath(const char* logPath);

    void setBaseLogName(const char* logName);

    void setLogLevel(ERsLogLevel logLevel);
    ERsLogLevel getLogLevel();

    void reset(const char* title = NULL);

    void trace(const char* file, int line, const char* func, const char* msg, ...);

    void debug(const char* file, int line, const char* func, const char* msg, ...);

    void info(const char* file, int line, const char* func, const char* msg, ...);

    void warn(const char* file, int line, const char* func, const char* msg, ...);

    void error(const char* file, int line, const char* func, const char* msg, ...);

    void fatal(const char* file, int line, const char* func, const char* msg, ...);

    void output(const char* msg, int len);

    const char* getLogLevelName(ERsLogLevel logLevel);

    void setMaxLogFiles(int maxFiles);


protected:
   int sprintf(ERsLogLevel log_level, const char* file, int line, const char* func, const char* msg, va_list args);

   bool is_loggable(ERsLogLevel level);

   bool rollFile();

   bool clearExpiredLogs();

private:
    RsLog();

    RsLog(const RsLog& log);
    RsLog& operator=(const RsLog& log);

    ~RsLog();

    static RsLog* instance_;
    static std::mutex instance_mutex_;
    // static RsLog instance_;
    class RsLoggerDc
    {
    public:
        RsLoggerDc()
        {

        }
        ~RsLoggerDc()
        {
            if (instance_!= nullptr)
            {
                delete instance_;
                instance_ = nullptr;
            }
        }
    };

    static RsLoggerDc s_rs_log_dc_;

private:
    std::string m_log_path_;
    std::string m_file_name_;
    std::string m_base_file_name_;
    std::string log_title_;
    ERsLogLevel log_level_;
    char* msg_;
    std::mutex mutex_;
    int m_maxLogFiles;
};

//////////////////////////////////////////////////////////////////////////
#define g_RsLog (RsLog::Instance())


#endif // RSLOG_H
