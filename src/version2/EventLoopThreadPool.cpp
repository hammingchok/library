#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseloop, int numThreads) :
    baseLoop_(baseloop),
    started_(false),
    numThreads_(numThreads),
    next_(0)
{
    if(numThreads_ <= 0) {
        LOG << "numThreads_ <= 0";
        abort();
    }
}

//在主线程的server::start()中调用
//函数功能是创建IO线程并加入到IO线程池中
void EventLoopThreadPool::start() {
    //断言是在主线程中
    baseLoop_->assertInLoopThread();

    started_ = true;
    for(int i = 0; i < numThreads_; i++) {
        std::shared_ptr<EventLoopThread> t(new EventLoopThread()); //创建IO线程
        threads_.push_back(t); //添加到线程池中
        loops_.push_back(t->startLoop()); //
    }
}

//round robin
//在主线程accept之后调用，获得下一个线程，分发任务给它
EventLoop* EventLoopThreadPool::getNextLoop() {
    //断言在主线程中
    baseLoop_->assertInLoopThread();

    assert(started_);
    
    EventLoop *loop = baseLoop_;
    if(!loops_.empty()) {
        loop = loops_[next_];
        next_ = (next_ + 1) % numThreads_;
    }

    return loop;
}
