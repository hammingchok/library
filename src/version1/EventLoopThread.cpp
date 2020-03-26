#include "EventLoopThread.h"

EventLoopThread::EventLoopThread()
    : _loop( new EventLoop() ),
      _thread( threadFunc, _loop )
{ 
    _thread.start();
}

EventLoopThread::~EventLoopThread()
{
    _loop->quit();
    _thread.join();
}

void* EventLoopThread::threadFunc(void *arg)
{
    EventLoop *loop = (EventLoop*)arg;
    loop->loop();
}

//不需要等待子线程初始化返回了，因为在主函数里面已经创建了线程池
/*
EventLoop* EventLoopThread::startLoop()
{
    assert(!_thread.isStarted());
    _thread.start();
    {
        MutexLock lock(_mutex);
        while(_loop == NULL)
            _cond.wait();
    }
    return _loop;
}

void* EventLoopThread::threadFunc(void* arg)
{
    EventLoopThread *ptr = (EventLoopThread*)arg;
    EventLoop loop = ptr->getLoop();
    {
        MutexLock lock(_mutex);
        _loop = &loop;
        _cond.notify();
    }
    loop.loop();
}
*/
