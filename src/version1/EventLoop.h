#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <functional>
#include <memory>
#include <vector>
//#include "Epoll.h"
//#include "CurrentThread.h"

class Channel;
class Epoll;
///EventLoop.cpp调用Epoll类中成员函数，需要在里面声明头文件

class EventLoop 
{
public:
    typedef std::vector<Channel*> ChannelVector;
    EventLoop();
    ~EventLoop();
    void loop();
    void quit();

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    //bool isLooping_;
    //const pid_t threadId_; //did not
    bool isQuit_;
    std::unique_ptr<Epoll> ep_;
    ChannelVector activeChannels_;
};

#endif
