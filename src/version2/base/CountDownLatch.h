#ifndef COUNTDOWNLATCH_H
#define COUNTDOWNLATCH_H

#include "MutexLock.h"
#include "Condition.h"
#include "noncopyable.h"

//计数器：让拥有计数器的线程等待其他线程执行完后再执行 
class CountDownLatch: noncopyable {
public: 
    explicit CountDownLatch(int count); //防止隐性转换
    void wait();
    void countDown();
private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_; //为防止时序竞态，需要加锁
};

#endif
