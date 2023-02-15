#include "rslog.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "timestamp.h"
#include "fmt.h"
#include <cassert>
#include "curthread.h"
#include <stdarg.h>
#include <string.h>
#include <vector>

RsLog* RsLog::instance_ = nullptr;
RsLog::RsLoggerDc RsLog::s_rs_log_dc_;
std::mutex RsLog::instance_mutex_;
// RsLog RsLog::instance_;

#define DEFAULT_LOG_DIR "/rs_default_dir"
#define RS_LOG_DIR "/log"
#define DEFAULT_FILE_NAME_PREFIXX "RS-LOG-Threadpool-"

#define BUF_SIZE 1024
#define MAX_BUF_SIZE 10*1024

#define RAY_SHAPE_HEADER "Ray Shape Header Ubuntu Log Start..."

#define MAX_LOG_SIZE_TEST 1024
#define MAX_LOG_SIZE 1024*1024*25

const char* RsLogLevelName[ERsLogLevel::NUM_LOG_LEVELS] =
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

static void get_pid_by_proc_name(pid_t *pid, char *task_name)
{
    DIR *dir;
    struct dirent *ptr;
    FILE *fp;
    char filepath[50];
    char cur_task_name[50];
    char buf[BUF_SIZE];

    dir = opendir("/proc");
    if (NULL != dir)
    {
        while ((ptr = readdir(dir)) != NULL) //循环读取/proc下的每一个文件/文件夹
        {
            //如果读取到的是"."或者".."则跳过，读取到的不是文件夹名字也跳过
            if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
                continue;
            if (DT_DIR != ptr->d_type)
                continue;

            sprintf(filepath, "/proc/%s/status", ptr->d_name);//生成要读取的文件的路径
            fp = fopen(filepath, "r");
            if (NULL != fp)
            {
                if( fgets(buf, BUF_SIZE-1, fp)== NULL ){
                    fclose(fp);
                    continue;
                }
                sscanf(buf, "%*s %s", cur_task_name);

                //如果文件内容满足要求则打印路径的名字（即进程的PID）
                if (!strcmp(task_name, cur_task_name)){
                    sscanf(ptr->d_name, "%d", pid);
                }
                fclose(fp);
            }
        }
        closedir(dir);
    }
}

//获取日志名
static std::string getLogFileName(const std::string &basename)
{
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    time_t now = time(NULL);
    // 把UTC秒数转换成时间结构，有时区转换，得到本地时间
    localtime_r(&now, &tm);
    strftime(timebuf, sizeof(timebuf), "%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;

    char hostname[256];
    if (::gethostname(hostname, sizeof(hostname)) == 0)
    {
        hostname[sizeof(hostname) - 1] = '\0';
        filename += hostname;
    }
    else
      filename += "unknownhost";

    char pidbuf[32];
    snprintf(pidbuf, sizeof pidbuf, ".%d", ::getpid());
    filename += pidbuf;

    filename += ".log";

    return filename;
}

static bool get_proc_name_by_pid(pid_t pid, char *task_name)
{
    bool res = false;
    char proc_pid_path[BUF_SIZE];
    char buf[BUF_SIZE];

    sprintf(proc_pid_path, "/proc/%d/status", pid);
    FILE* fp = fopen(proc_pid_path, "r");
    if(NULL != fp){
        if( fgets(buf, BUF_SIZE-1, fp)== NULL ){
            fclose(fp);
        }
        fclose(fp);
        sscanf(buf, "%*s %s", task_name);
        res = true;
    }

    return res;
}

static std::string formatCurrentTime()
{
    RsTimestamp timeStamp(RsTimestamp::now());
    int64_t microSecondsSinceEpoch = timeStamp.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / RsTimestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % RsTimestamp::kMicroSecondsPerSecond);

    struct tm tm_time;
    //获取当前时间结构，本地时间，有时区转换
    ::localtime_r(&seconds, &tm_time);
    char t_time[64];
    memset(t_time, 0, 64);
    int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                       tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                       tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 17);
    std::string currTime = t_time;

    Fmt us(".%06d ", microseconds);
    assert(us.length() == 8);
    currTime.append(us.data(), 8);

    return currTime;
}

RsLog& RsLog::Instance()
{
    if (instance_ == nullptr)
    {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (instance_ == nullptr)
        {
            instance_ = new RsLog();
        }
    }

    return *instance_;
    // return instance_;
}

RsLog::RsLog()
    : m_base_file_name_(DEFAULT_FILE_NAME_PREFIXX)
    , log_title_(RAY_SHAPE_HEADER)
    , log_level_(ERsLogLevel::WARN)
    , msg_(nullptr)
{
    char* homePath = getenv("HOME");
    m_log_path_ = homePath;
    char procName[256];
    memset(procName, 0, 256);
    pid_t pid = getpid();
    bool bRes = get_proc_name_by_pid(pid, procName);
    if (bRes)
    {
        m_log_path_.append("/.config/");
        m_log_path_.append(procName);
        if (access(m_log_path_.c_str(), 6) != 0)
        {
            mkdir(m_log_path_.c_str(), 0755);
        }
        m_log_path_.append(RS_LOG_DIR);
        if (access(m_log_path_.c_str(), 6) != 0)
        {
            mkdir(m_log_path_.c_str(), 0755);
        }
    }
    else
    {
        m_log_path_.append("/.config/");
        m_log_path_.append(DEFAULT_LOG_DIR);
        if (access(m_log_path_.c_str(), 6)  != 0)
        {
            mkdir(m_log_path_.c_str(), 0755);
        }
        m_log_path_.append(RS_LOG_DIR);
        if (access(m_log_path_.c_str(), 6)  != 0)
        {
            mkdir(m_log_path_.c_str(), 0755);
        }
    }

    m_file_name_ = getLogFileName(m_base_file_name_);

    msg_ = new char[MAX_BUF_SIZE];

    reset();
}

RsLog::~RsLog()
{
    if (msg_)
    {
        delete msg_;
        msg_ = nullptr;
    }
}


void RsLog::setLogPath(const char* logPath)
{
    if (logPath == nullptr)
        return;

    if (m_log_path_ == logPath)
    {
        return;
    }

    const char *tokseps = "/";
    char szLogPath[1024];
    memset(szLogPath, 0, 1024);
    snprintf(szLogPath, 1023, "%s", logPath);

    std::string tmpDirPath;
    if (szLogPath[0] == '/')
    {
        tmpDirPath = "/";
    }

    char* pt = strtok(szLogPath, tokseps);
    while (pt)
    {
        tmpDirPath.append(pt);
        if (access(tmpDirPath.c_str(), 6)  != 0)
        {
            mkdir(tmpDirPath.c_str(), 0755);
        }
        pt = strtok(NULL,tokseps);
        if (pt)
        {
            tmpDirPath.append("/");
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    m_log_path_ = tmpDirPath;
    reset(log_title_.c_str());
}

void RsLog::setBaseLogName(const char* logName)
{
    if (logName == nullptr)
        return;

    std::lock_guard<std::mutex> lock(mutex_);
    m_base_file_name_ = logName;
    m_file_name_ = getLogFileName(m_base_file_name_);
    reset(log_title_.c_str());
}

void RsLog::setLogLevel(ERsLogLevel logLevel)
{
    log_level_ = logLevel;
}

ERsLogLevel RsLog::getLogLevel()
{
    return log_level_;
}

void RsLog::reset(const char* title)
{
    log_title_ = (title) ? title : RAY_SHAPE_HEADER;

    char logFilePath[1024];
    memset(logFilePath, 0, 1024);
    snprintf(logFilePath, 1023, "%s/%s", m_log_path_.c_str(), m_file_name_.c_str());

    FILE* fp = fopen(logFilePath, "w+");
    if (fp)
    {
        std::string currentTime = formatCurrentTime();
        fprintf(fp, "%s - # %s \n", currentTime.c_str(), log_title_.c_str());
        fflush(fp);
        fclose(fp);
    }
}

void RsLog::trace(const char* file, int line, const char* func, const char* msg, ...)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_loggable(TRACE))
    {
        va_list args;
        va_start(args, msg);
        sprintf(TRACE, file, line, func, msg, args);
        va_end(args);
    }
}

void RsLog::debug(const char* file, int line, const char* func, const char* msg, ...)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_loggable(DEBUG))
    {
        va_list args;
        va_start(args, msg);
        sprintf(DEBUG, file, line, func, msg, args);
        va_end(args);
    }
}

void RsLog::info(const char* file, int line, const char* func, const char* msg, ...)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_loggable(INFO))
    {
        va_list args;
        va_start(args, msg);
        sprintf(INFO, file, line, func, msg, args);
        va_end(args);
    }
}

void RsLog::warn(const char* file, int line, const char* func, const char* msg, ...)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_loggable(WARN))
    {
        va_list args;
        va_start(args, msg);
        sprintf(WARN, file, line, nullptr, msg, args);
        va_end(args);
    }
}

void RsLog::error(const char* file, int line, const char* func, const char* msg, ...)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_loggable(ERROR))
    {
        va_list args;
        va_start(args, msg);
        sprintf(ERROR, file, line, nullptr, msg, args);
        va_end(args);
    }
}

void RsLog::fatal(const char* file, int line, const char* func, const char* msg, ...)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_loggable(FATAL))
    {
        va_list args;
        va_start(args, msg);
        sprintf(FATAL, file, line, nullptr, msg, args);
        va_end(args);
    }
}

void RsLog::output(const char* msg, int len)
{
     std::lock_guard<std::mutex> lock(mutex_);
     char logFilePath[1024];
     memset(logFilePath, 0, 1024);
     snprintf(logFilePath, 1023, "%s/%s", m_log_path_.c_str(), m_file_name_.c_str());

     FILE* fp = fopen(logFilePath, "a");
     if (fp)
     {
         fprintf(fp, "%s", msg);
         fflush(fp);
         fclose(fp);
         rollFile();
     }
}


const char* RsLog::getLogLevelName(ERsLogLevel logLevel)
{
    return RsLogLevelName[logLevel];
}

int RsLog::sprintf(ERsLogLevel log_level, const char* file, int line, const char* func, const char* msg, va_list args)
{
    int ret = -1;

    char logFilePath[1024];
    memset(logFilePath, 0, 1024);
    snprintf(logFilePath, 1023, "%s/%s", m_log_path_.c_str(), m_file_name_.c_str());

    FILE* fp = fopen(logFilePath, "a");
    if (fp)
    {
        CurrentThread::tid(); //取得该线程的tid
        // 输入线程id
        std::string currentTime = formatCurrentTime();
        const char* sourceFile = nullptr;
        if (func == nullptr)
        {
            fprintf(fp, "%s [%s] - %s - ",  currentTime.c_str(),
                    RsLogLevelName[log_level],
                    CurrentThread::tidString());
        }
        else
        {
            fprintf(fp, "%s [%s] - %s - %s: ",  currentTime.c_str(),
                    RsLogLevelName[log_level],
                    CurrentThread::tidString(),
                    func);
        }

        memset(msg_, 0, sizeof(char)*(MAX_BUF_SIZE));
        vsnprintf(msg_, MAX_BUF_SIZE-1, msg, args);
        fprintf(fp, "%s - %s:%d\n", msg_, file, line);

        fflush(fp);
        fclose(fp);

        rollFile();
        ret = 0;
    }

    return ret;
}

bool RsLog::is_loggable(ERsLogLevel level)
{
    return log_level_ <= level;
}

//日志滚动
bool RsLog::rollFile()
{
    struct stat statbuf;

    char logFilePath[1024];
    memset(logFilePath, 0, 1024);
    snprintf(logFilePath, 1023, "%s/%s", m_log_path_.c_str(), m_file_name_.c_str());
    int ret = stat(logFilePath, &statbuf);
    if (ret != 0)
    {
        return false;
    }

    if (statbuf.st_size >= MAX_LOG_SIZE)
    {
        std::string newFileName = getLogFileName(m_base_file_name_);
        if (newFileName == m_file_name_)
        {
            bool stopflag = false;
            unsigned int lastID = 1;
            char newname[1024];
            char oldname[1024];
            // find the next available name
            while(!stopflag)
            {
                memset(newname, 0, 1024);
                snprintf(newname, 1023, "%s/%s.%d", m_log_path_.c_str(), m_file_name_.c_str(), lastID);
                FILE* tmpfile = fopen(newname, "r");
                if (tmpfile)
                {
                    fclose(tmpfile);
                    lastID++;
                }
                else
                {
                    stopflag = true;
                }
            }

            // rename
            for(int t=lastID;t>1;t--)
            {
                memset(newname, 0, 1024);
                memset(oldname, 0, 1024);
                snprintf(newname, 1023, "%s/%s.%d", m_log_path_.c_str(), m_file_name_.c_str(), lastID);
                snprintf(oldname, 1023, "%s/%s.%d", m_log_path_.c_str(), m_file_name_.c_str(), lastID-1);
                rename(oldname, newname);
            }
            snprintf(newname, 1023, "%s/%s.%d", m_log_path_.c_str(), m_file_name_.c_str(), 1);
            remove(newname);
            rename(logFilePath, newname);
        }
        m_file_name_ = newFileName;
        reset(log_title_.c_str());

        return true;
    }

    return false;
}


void RsLog::setMaxLogFiles(int maxFiles)
{
    m_maxLogFiles = maxFiles;
}

bool RsLog::clearExpiredLogs()
{
    if(m_maxLogFiles <= 0)  // 不限制
        return false;

    DIR *d;
    struct dirent* file = new dirent();
    if(!(d = opendir(m_log_path_.c_str())))
    {
        printf("erroe dirs %s!!!\n", m_log_path_.c_str());
        return false;
    }

    std::vector<std::string> vecLogsFiles;
    int count = 0;
    while((file = readdir(d)) != NULL)
    {
        if(strncmp(file->d_name, ".", 1) == 0
                || strncmp(file->d_name, "..", 1) == 0)
             continue;

        // 如果是普通的文件
        if(file->d_type == 8)
        {
            int len = m_log_path_.length() + strlen(file->d_name) + 2;
            char filePath[1024];
            memset(filePath, 0, 1024);
            strcat(filePath, m_log_path_.c_str());
            strcat(filePath, "/");
            strcat(filePath, file->d_name);
            vecLogsFiles.push_back(filePath);
            count++;

         }
     }
     closedir(d);
     if (file)
     {
         delete file;
         file = nullptr;
     }

     for (int i = count-1; i >  m_maxLogFiles-1; i--)
     {
        remove(vecLogsFiles[i].c_str());
     }

     return true;
}
