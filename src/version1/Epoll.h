#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <iostream>
#include <vector>
#include <map>
#include "Channel.h"

class EventLoop;//用于声明成员变量

class Epoll
{
public:
    typedef std::vector<Channel*> ChannelVector;
    typedef std::vector<struct epoll_event> EventVector;
    typedef std::map< int, Channel* > ChannelMap;

    Epoll(EventLoop *loop);

    ~Epoll();

    // 调用epoll_wait，并将其交给Event类的handleEvent函数处理
    void epWait( ChannelVector* activeChannels );

    //将channel注册到Epoll中，并调用update
    void updateChannel( Channel* channel );
    void removeChannel( Channel* channel );

protected:
    ChannelMap channels_;

private:
    EventLoop* loop_;
    int epollfd_;
    EventVector rEvents_;

    //调用epoll_ctl
    void update( int operation, Channel* channel );
    void fillEventsToActiveChannels( int evenNum, ChannelVector* activeChannels ) const;  

};

#endif
