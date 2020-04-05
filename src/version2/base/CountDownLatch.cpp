#include "CountDownLatch.h"

//计数器实现
CountDownLatch::CountDownLatch(int count):
    mutex_(),
    condition_(mutex_),
    count_(count) 
{}

void CountDownLatch::wait() {
    MutexLockGuard lock(mutex_);
    while(count_ > 0) //使用while，防止惊群效应
        condition_.wait();
}

void CountDownLatch::countDown() {
    MutexLockGuard lock(mutex_); //保护临界区
    count_--;
    if(count_ == 0) {
        condition_.notifyAll();
    }
}
