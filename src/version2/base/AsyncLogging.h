#ifndef ASYNCLOGGING_H
#define ASYNCLOGGING_H

#include "Thread.h"
#include "MutexLock.h"
#include "LogStream.h"
#include "noncopyable.h"
#include "CountDownLatch.h"
#include <functional>
#include <string>
#include <vector>


//实现异步日志类，启动一个线程来写日志
//持有一个LogFile以及缓冲区
class AsyncLogging: noncopyable {
public:
    AsyncLogging(const std::string basename, int flushInterval = 2);
    ~AsyncLogging() {
        if(running_) {
            stop();
        }
    }
    
    void append(const char* logline, int len);

    void start() {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop() {
        running_ = false;
        cond_.notify();
        thread_.join();
    }

private:
    void threadFunc();
    //灵活的buffer，在LogStream文件中
    typedef FixedBuffer<kLargeBuffer> Buffer;
    typedef std::shared_ptr<Buffer> BufferPtr;
    typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
    //flush
    const int flushInterval_;
    bool running_;
    std::string basename_;
    
    Thread thread_;
    //锁
    MutexLock mutex_;
    Condition cond_;
    //双缓冲数组指针
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    //缓冲数组指针的数组
    BufferVector buffers_;
    //计数器
    CountDownLatch latch_;
};

#endif
