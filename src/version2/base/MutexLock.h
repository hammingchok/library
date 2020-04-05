#ifndef MUTEXLOCK_H
#define MUTEXLOCK_H

#include <pthread.h>
#include <cstdio>
#include "noncopyable.h"

//互斥锁的封装
class MutexLock: noncopyable { //不可拷贝，禁止了拷贝构造和重载=
public:
    MutexLock() {
        pthread_mutex_init(&mutex, nullptr);
    }

    ~MutexLock() {
        pthread_mutex_lock(&mutex); //保护锁自身的销毁
        pthread_mutex_destroy(&mutex);
    }

    void lock() {
        pthread_mutex_lock(&mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_t* get() {
        return &mutex;
    }

private:
    pthread_mutex_t mutex;

    friend class Condition; //声明友元
};

//自动加解锁类的封装
class MutexLockGuard: noncopyable {
public: 
    //函数构造并自动加锁
    explicit MutexLockGuard(MutexLock& _mutex): mutex(_mutex) {
        mutex.lock();
    }

    //函数析构自动解锁
    ~MutexLockGuard() {
        mutex.unlock();
    }

private:
    MutexLock &mutex;
};

#endif
