#ifndef EPOLL_H
#define EPOLL_H

#include "Channel.h"
#include "HttpData.h"
#include "Timer.h"

#include <sys/epoll.h>
#include <memory>
#include <vector>
#include <unordered_map>

class Epoll {
public:
    Epoll();
    ~Epoll();
    void epoll_add(SP_Channel request, int timeout);
    void epoll_mod(SP_Channel request, int timeout);
    void epoll_del(SP_Channel request);
    std::vector<std::shared_ptr<Channel>> poll();
    std::vector<std::shared_ptr<Channel>> getEventsRequest(int events_num);
    void add_timer(std::shared_ptr<Channel> request_data, int timeout);

    int getEpollFd() {
        return epollFd_;
    }
    void handleExpired();

private:
    static const int MAXFDS = 100000;
    int epollFd_;
    std::vector<epoll_event> events_; //epoll_wait传入参数
    std::shared_ptr<Channel> fd2chan_[MAXFDS];
    std::shared_ptr<HttpData> fd2http_[MAXFDS];
    TimerManager timerManager_; //超时管理器
};

#endif
