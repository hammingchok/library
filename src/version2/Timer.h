#ifndef TIMER_H
#define TIMER_H

#include "base/noncopyable.h"
#include "base/MutexLock.h"
#include "HttpData.h"
#include <unistd.h>
#include <memory>
#include <queue>
#include <deque>

class HttpData;

//超时结点类
class TimerNode {
public:
    TimerNode(std::shared_ptr<HttpData> requestData, int timeout);
    TimerNode(TimerNode &timerNode);
    ~TimerNode();

    void update(int timeout);
    bool isValid();
    void clearReq();
    void setDeleted() {deleted_ = true;}
    bool isDeleted() const {return deleted_;}
    size_t getExpTime() const {return expiredTime_;}

private:
    bool deleted_;
    size_t expiredTime_;
    std::shared_ptr<HttpData> SPHttpData; //Http请求
};

struct TimerCmp {
    bool operator()(std::shared_ptr<TimerNode>& first, std::shared_ptr<TimerNode>& second) const {
        return first->getExpTime() > second->getExpTime();
    }
};

//超时结点管理类
class TimerManager {
public:
    TimerManager();
    ~TimerManager();
    //加入timer，参数为：HttpData + timeout
    void addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout);
    void handleExpiredEvent();

private:
    typedef std::shared_ptr<TimerNode> SPTimerNode;
    //管理一个timer的优先级队列
    std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp> timerNodeQueue;
};

#endif
