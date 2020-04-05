#ifndef CONDITION_H
#define CONDITION_H

#include <pthread.h>
#include <errno.h>
#include <cstdint>
#include <time.h> 
#include "noncopyable.h"
#include "MutexLock.h"

//条件变量的封装
class Condition: noncopyable {
public:
    explicit Condition(MutexLock &_mutex): mutex(_mutex) {
        //初始化条件变量
	pthread_cond_init(&cond, nullptr);
    }

    ~Condition() {
	//需要将条件变量destory
        pthread_cond_destroy(&cond);
    }
    
    void wait() {
	//条件变量配合锁
        pthread_cond_wait(&cond, mutex.get());
    }

    void notify() {
        pthread_cond_signal(&cond);
    }

    void notifyAll() {
        pthread_cond_broadcast(&cond);
    }

    bool waitForSeconds(int seconds) {
        struct timespec abstime; //time.h
        clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += static_cast<time_t>(seconds);
        return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.get(), &abstime);
    }

private:
    MutexLock &mutex; //拥有类对象
    pthread_cond_t cond; //pthread.h
};

#endif
