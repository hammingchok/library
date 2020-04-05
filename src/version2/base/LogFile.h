#ifndef LOGFILE_H
#define LOGFILE_H

#include "FileUtil.h"
#include "noncopyable.h"
#include "MutexLock.h"
#include <memory>
#include <string>

//日志类，该类持有一个AppendFile对象实现AppendFile
//为了保证线程安全，还需要一个互斥锁，然后提供appendn次去flush一次
//这个通过一个计数器实现，当counter到达flushEveryN的时候就会去触发flush操作，将内容刷入硬盘
class LogFile : noncopyable {
public:
    LogFile(const std::string& basename, int flushEveryN = 1024);
    ~LogFile();

    //append、flush
    void append(const char* logline, const size_t len);
    void flush();

    bool rollFile();
private:
    void append_unlocked(const char* logline, const size_t len);
    
    const std::string basename_;
    const int flushEveryN_;

    int count_;
    //线程安全
    std::unique_ptr<MutexLock> mutex_;

    //文件操作类对象
    std::unique_ptr<AppendFile> file_;
};

#endif
