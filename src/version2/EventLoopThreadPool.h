#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include "base/Logger.h"
#include "base/noncopyable.h" 
#include "EventLoopThread.h"

#include <memory>
#include <vector>

class EventLoopThreadPool: noncopyable {
public:
    EventLoopThreadPool(EventLoop *baseloop, int numThreads); //传入main loop和线程池大小
    ~EventLoopThreadPool() {
        LOG << "~EventLoopThreadPool()";
    }

    //
    void start();

    //round robin
    EventLoop *getNextLoop();
private:
    EventLoop *baseLoop_; //main loop
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::shared_ptr<EventLoopThread>> threads_; //线程池
    std::vector<EventLoop*> loops_;
};

#endif
