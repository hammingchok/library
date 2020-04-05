#ifndef CURRENTTHREAD_H
#define CURRENTTHREAD_H

#include <stdint.h>

namespace CurrentThread {
    //使用__thread，当前线程私有，保证线程唯一拥有
    extern __thread int         t_cacheTid;
    extern __thread char        t_tidString[32];
    extern __thread int         t_tidStringLength;
    extern __thread const char* t_threadName;
    
    void cacheTid();
    
    inline int tid() {
        if(__builtin_expect(t_cacheTid == 0, 0)) {
            cacheTid();
        }
        return t_cacheTid;
    }
    
    inline const char* tidString() {
        return t_tidString;
    }
    
    inline int tidStringLength() {
        return t_tidStringLength;
    }
    
    inline const char* name() {
        return t_threadName;
    }
}

#endif
