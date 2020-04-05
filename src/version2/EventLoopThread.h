#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H 

#include "base/Condition.h"
#include "base/MutexLock.h"
#include "base/Thread.h"
#include "base/noncopyable.h"
#include "EventLoop.h"

class EventLoopThread: noncopyable {
public: 
    EventLoopThread();
    ~EventLoopThread();
    EventLoop *startLoop();

private:
    void threadFunc();
    EventLoop *loop_;
    bool existing_;
    Thread thread_; //拥有一个线程对象
    //需要线程同步
    MutexLock mutex_;
    Condition cond_;
};

#endif
