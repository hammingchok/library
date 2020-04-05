#include "LogFile.h"
#include "FileUtil.h"
#include <time.h>
#include <stdio.h>
#include <assert.h>

using namespace std;

//对AppendFile文件工具类的进一步封装，主要是append和flush操作
LogFile::LogFile(const string& basename, int flushEveryN):
    basename_(basename),
    flushEveryN_(flushEveryN),
    count_(0),
    mutex_(new MutexLock) //使用unique_ptr
{
    //对AppendFile对象的操作
    file_.reset(new AppendFile(basename));
}

LogFile::~LogFile() {}

//对AppendFile append的封装，加锁保证线程安全
void LogFile::append(const char* logline, const size_t len) {
    //加锁
    MutexLockGuard lockGuard(*mutex_);
    append_unlocked(logline, len);
}

//对flush的封装，加锁
void LogFile::flush() {
    MutexLockGuard lockGuard(*mutex_);
    file_->flush();
}

void LogFile::append_unlocked(const char* logline, const size_t len) {
    //对文件进行append操作
    file_->append(logline, len);
    //append次数加一
    count_++;
    //当到达刷新次数
    if(count_ >= flushEveryN_) {
        count_ = 0; //归零
	//对文件进行flush
        file_->flush();
    }
}
