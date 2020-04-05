#ifndef THREAD_H
#define THREAD_H

#include <memory>
#include <string>
#include <functional>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "CountDownLatch.h"
#include "noncopyable.h"

//封装线程类，不可拷贝
class Thread: noncopyable {
public:
    typedef std::function<void()> ThreadFunc;
    explicit Thread(const ThreadFunc&, const std::string& name = std::string());
    ~Thread();
    //对外接口
    void start();
    int join();
    bool started() const {return started_;}
    pid_t tid() const {return tid_;}
    const std::string& name() const {return name_;}
private:
    void setDefaultName();
    //成员变量
    bool started_;
    bool joined_;
    pthread_t pthreadId_;
    pid_t tid_;
    ThreadFunc func_; //回调函数
    std::string name_;
    CountDownLatch latch_; //计数器
};

#endif
