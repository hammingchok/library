#include "Util.h"
#include "EventLoop.h"
#include "base/Logger.h"

#include "sys/eventfd.h"
#include "sys/epoll.h"
#include <iostream>

using namespace std;

//每个线程唯一拥有
__thread EventLoop* t_loopInThisThread = 0;

//获取eventfd
int createEventfd() {
    int fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    
    if(fd < 0) {
        LOG << "eventfd failed";
        abort();
    }
    
    return fd;
}

//事件循环
EventLoop::EventLoop() :
    looping_(false),
    poller_(new Epoll()),
    wakeupFd_(createEventfd()),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    pwakeupChannel_(new Channel(this, wakeupFd_)) 
    {
	//判断是否已经被初始化
        if(t_loopInThisThread) {
            LOG << "this eventloop owned by other thread";
        } else {
            t_loopInThisThread = this;
        }
	//设置所关注的事件、回调函数并添加到epoll中
        pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
        pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead, this)); //当被唤醒时执行的函数，其实就是把数据读了
        pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));
        poller_->epoll_add(pwakeupChannel_, 0);
    }

EventLoop::~EventLoop() {
    close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleConn() {
    updatePoller(pwakeupChannel_, 0);
}

//线程唤醒，向指定线程的eventfd写入一点数据，epoll_wait返回
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);
    if(n != sizeof one) {
        LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

//当被唤醒，epoll返回后channel绑定的回调函数，也就是将数据读走
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
    //继续关注
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}

//让线程执行某个任务的函数实现
void EventLoop::runInLoop(Functor &&cb) {
    if(isInLoopThread()) 
        cb();
    else 
        queueInLoop(std::move(cb)); //添加到队列中，再异步唤醒
}

void EventLoop::queueInLoop(Functor&& cb) {
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }
    if(!isInLoopThread() || callingPendingFunctors_) //不是本线程或正在处理队列中的任务
        wakeup();
}

//事件循环实现
void EventLoop::loop() {
    assert(!looping_);
    assert(isInLoopThread());
    
    looping_ = true;
    quit_ = false;
    std::vector<SP_Channel> ret;
    
    while(!quit_) {
        ret.clear();
        ret = poller_->poll(); //epoll返回channel

        eventHandling_ = true;
        for(auto &it : ret) 
             it->handleEvents(); //处理回调函数
        eventHandling_ = false;
        
	doPendingFunctors(); //处理队列任务
        poller_->handleExpired();
    }
    
    looping_ = false;
}

//处理队列中任务的函数实现
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    
    //这里的技巧是：1、锁的临界区拉小；2、将队列取出来，避免阻塞其他线程
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    //一个一个处理队列中的任务
    for(size_t i = 0; i < functors.size(); i++) 
        functors[i]();

    callingPendingFunctors_ = false;
}

//退出事件循环函数的实现
void EventLoop::quit() {
    quit_ = true;
    //如果不是本线程，异步唤醒
    if(!isInLoopThread()) {
        wakeup();
    }
}
