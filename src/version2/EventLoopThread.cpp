#include "EventLoopThread.h"
#include <functional>

EventLoopThread::EventLoopThread() :
    loop_(nullptr),
    existing_(false),
    //线程对象绑定回调函数
    thread_(bind(&EventLoopThread::threadFunc, this), "EventLoopThread"), 
    mutex_(),
    cond_(mutex_)
{}

//析构函数
EventLoopThread::~EventLoopThread() {
    existing_ = true;
    
    //退出循环、回收子线程
    if(loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

//在EventLoopThreadPool::start()中调用
//启动IO子线程，并返回loop指针给主线程
EventLoop* EventLoopThread::startLoop() {
    //断言未启动线程
    assert(!thread_.started());
    //启动线程，线程会调用EventLoopThread::threadFunc()
    thread_.start();

    //这里是主线程和子线程并发执行
    //需要使用条件变量，等待子线程创建loop
    {
        MutexLockGuard lock(mutex_);
        while(loop_ == nullptr) {
            cond_.wait();
        }
    }

    return loop_;
}

//子线程执行的回调函数
void EventLoopThread::threadFunc() {
    //创建loop
    EventLoop loop;
    
    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }
    
    loop.loop(); //开启事件循环，等待主线程分配任务，接受客户端数据
    
    loop_ = nullptr;
}
