#include "Epoll.h"
#include "Util.h"
#include "base/Logger.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <queue>
#include <deque>
#include <iostream>

using namespace std;

const int EVENTSNUM = 4 * 1024;
const int EPOLLWAIT_TIME = 10000;

typedef shared_ptr<Channel> SP_Channel; 

Epoll::Epoll() :
    epollFd_(epoll_create1(EPOLL_CLOEXEC)),
    events_(EVENTSNUM)
{
    assert(epollFd_ > 0);
}

Epoll::~Epoll() {}

//关注channel事件
void Epoll::epoll_add(SP_Channel request, int timeout) {
    int fd = request->getFd(); //获得channel关联的fd
    
    //
    if(timeout > 0) {
        add_timer(request, timeout);
	//保存HttpData
        fd2http_[fd] = request->getHolder();
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();

    request->EqualAndUpdateLastEvents();

    //保存HttpData
    fd2chan_[fd] = request;
    //关注事件
    if(epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
        perror("epoll_add failed");
        //如果失败就对channel重置
	fd2chan_[fd].reset();
    }
}

//modify
void Epoll::epoll_mod(SP_Channel request, int timeout) {
    if(timeout > 0) {
        add_timer(request, timeout);
    }

    int fd = request->getFd();
    
    if(!request->EqualAndUpdateLastEvents()) {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = request->getEvents();
        
	if(epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
            perror("epoll_mod error");
            fd2chan_[fd].reset();
        }
    }
}

//delete
void Epoll::epoll_del(SP_Channel request) {
    int fd = request->getFd();
    
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getLastEvents();

    if(epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0) {
        perror("epoll_del error");
    }

    fd2chan_[fd].reset();
    fd2http_[fd].reset();
}

//epoll_wait
std::vector<SP_Channel> Epoll::poll() {
    while(true) {
        int event_count = epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
        
	if(event_count < 0) {
            perror("epoll wait error");
        }
        
	std::vector<SP_Channel> req_data = getEventsRequest(event_count);
        
	if(req_data.size() > 0) {
            return req_data;
        }
    } 
}

//
void Epoll::handleExpired() {
    timerManager_.handleExpiredEvent();
}

//返回请求
std::vector<SP_Channel> Epoll::getEventsRequest(int events_num) {
    std::vector<SP_Channel> req_data;
   
    //events_是epoll_wait返回的数组 
    for(int i = 0; i < events_num; i++) {
        int fd = events_[i].data.fd;
        SP_Channel cur_req = fd2chan_[fd];

        if(cur_req) {
            cur_req->setRevents(events_[i].events);
            cur_req->setEvents(0);

            req_data.push_back(cur_req);
        } else {
            LOG << "SP_Channel cur_req is invalid";
        }
    }
    
    return req_data;
}

//向timer管理器添加待处理的HTTP请求
void Epoll::add_timer(SP_Channel request, int timeout) {
    shared_ptr<HttpData> t = request->getHolder();
    
    if(t) {
        timerManager_.addTimer(t, timeout);
    } else {
        LOG << "timer add failed";
    }
}
